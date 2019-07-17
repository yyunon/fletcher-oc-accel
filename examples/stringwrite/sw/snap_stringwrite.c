#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/time.h>
#include <getopt.h>
#include <ctype.h>

#include <libsnap.h>
#include <snap_tools.h>
#include <snap_s_regs.h>

#include "snap_stringwrite.h"
#include <omp.h>

#define LOG(...) fprintf(log, __VA_ARGS__); fflush(log);

/* Structure to easily convert from 64-bit addresses to 2x32-bit registers */
typedef struct _lohi {
  uint32_t lo;
  uint32_t hi;
}
    lohi;

typedef union _addr_lohi {
  uint64_t full;
  lohi half;
}
    addr_lohi;

int initialize_snap(struct snap_card **device, struct snap_action **action, FILE *log) {
  int rc = 0;
  uint64_t cir;
  char device_name[64];
  int card_no = 0;
  unsigned long ioctl_data;

  snap_action_flag_t attach_flags = 0;

  sprintf(device_name, "/dev/cxl/afu%d.0s", card_no);
  *device = snap_card_alloc_dev(device_name, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);

  if (device == NULL) {
    errno = ENODEV;
    fprintf(log, "ERROR: snap_card_alloc_dev(%s)\n", device_name);
    return -1;
  }

  /// Read Card Capabilities
  snap_card_ioctl(*device, GET_CARD_TYPE, (unsigned long) &ioctl_data);
  LOG("SNAP on ");
  switch (ioctl_data) {
    case 0:fprintf(log, "ADKU3");
      break;
    case 1:fprintf(log, "N250S");
      break;
    case 16:fprintf(log, "N250SP");
      break;
    default:fprintf(log, "Unknown");
      break;
  }
  snap_card_ioctl(*device, GET_SDRAM_SIZE, (unsigned long) &ioctl_data);
  LOG(" Card, %d MB of Card Ram avilable.\n", (int) ioctl_data);

  snap_mmio_read64(*device, SNAP_S_CIR, &cir);
  LOG("Read from MMIO. Attaching action.\n");

  // Attach action
  *action = snap_attach_action(*device, ACTION_TYPE_EXAMPLE, attach_flags, 100);
  if (*action == NULL) {
    errno = ENODEV;
    fprintf(log, "ERROR: snap_attach_action(%s)\n", device_name);
    return -1;
  }
  LOG("Action attached.\n");

  return rc;
}

int terminate_snap(struct snap_card *device, struct snap_action *action, FILE *log) {
  LOG("Detaching action...\n");
  snap_detach_action(action);

  LOG("Detaching freeing card...\n");
  snap_card_free(device);

  return 0;
}

int allocate_buffers(uint32_t **off_buf, char **val_buf, int num_strings, int strlen_max, FILE *log) {
  int rc;
  // Create offsets buffer
  LOG("Allocating %lu for offsets buffer.\n", sizeof(uint32_t) * (num_strings + 1));
  rc = posix_memalign((void **) off_buf, BURST_LENGTH, sizeof(uint32_t) * (num_strings + 1));
  // Clear offset buffer
  for (uint32_t i = 0; i < num_strings + 1; i++) {
    (*off_buf)[i] = 0xDEADBEEF;
  }
  // Create value buffer
  uint32_t num_chars = strlen_max * num_strings;
  LOG("Allocated %lu for values buffer.\n", sizeof(char) * num_chars);
  rc = posix_memalign((void **) val_buf, BURST_LENGTH, sizeof(char) * num_chars);
  // Clear values buffer
  for (uint32_t i = 0; i < num_chars; i++) {
    (*val_buf)[i] = '\0';
  }
  return rc;
}

int main(int argc, char *argv[]) {
  // Log to a file.
  FILE *log = fopen("swlog.log", "w");

  // Program parameters
  uint32_t num_strings = 16;  /// Number of rows (strings) in the recordbatch
  uint32_t strlen_min = 0;    /// Minimum string length
  uint32_t strlen_mask = 255; /// Mask for PRNG unit, str length is strlen_min + [prng] & strlen_mask
  uint32_t strlen_max = 256;  /// Number of bytes to allocate per string

  // Parse optional arguments
  if (argc > 1) sscanf(argv[1], "%u", &num_strings);
  if (argc > 2) sscanf(argv[2], "%u", &strlen_min);
  if (argc > 3) sscanf(argv[3], "%u", &strlen_mask);

  // Initialize SNAP platform.
  int rc = 0;
  struct snap_card *device = NULL;
  struct snap_action *action = NULL;
  rc = initialize_snap(&device, &action, log);
  if (rc != 0) {
    LOG("ERROR: initialize_snap failed. RC=%d\n", rc);
    return (-1);
  }

  // Allocate Arrow buffers.
  uint32_t *off_buf = NULL;
  char *val_buf = NULL;

  allocate_buffers(&off_buf, &val_buf, num_strings, strlen_max, log);

  // Prepare addresses for registers.
  addr_lohi off, val;
  off.full = (uint64_t) off_buf;
  val.full = (uint64_t) val_buf;
  LOG("Offsets buffer @ %016lX\n", off.full);
  LOG("Values buffer @ %016lX\n", val.full);

  LOG("Setting control registers...\n");
  snap_mmio_write32(device, REG_CONTROL, CONTROL_RESET);
  snap_mmio_write32(device, REG_CONTROL, 0);
  snap_mmio_write32(device, REG_OFF_ADDR_LO, off.half.lo);
  snap_mmio_write32(device, REG_OFF_ADDR_HI, off.half.hi);
  snap_mmio_write32(device, REG_UTF8_ADDR_LO, val.half.lo);
  snap_mmio_write32(device, REG_UTF8_ADDR_HI, val.half.hi);
  snap_mmio_write32(device, REG_FIRST_IDX, 0);
  snap_mmio_write32(device, REG_LAST_IDX, num_strings);
  snap_mmio_write32(device, REG_STRLEN_MIN, strlen_min);
  snap_mmio_write32(device, REG_PRNG_MASK, strlen_mask);

  LOG("Registers set, starting kernel and polling for completion...\n");

  double start = omp_get_wtime();

  // Starting occurs when control start bit was high, then goes low.
  snap_mmio_write32(device, REG_CONTROL, CONTROL_START);
  snap_mmio_write32(device, REG_CONTROL, CONTROL_STOP);

  // Poll for completion.
  uint32_t status = 0;
  uint32_t last_off = 0xDEADBEEF;
  do {
    snap_mmio_read32(device, REG_STATUS, &status);
    LOG("S: %08X\n", status);
    sleep(1);
  } while ((status & STATUS_DONE) != STATUS_DONE);

  double stop = omp_get_wtime();
  LOG("Time: %f\n", stop - start);

  // Print offsets buffer.
  for (uint32_t i = 0; i < num_strings + 1; i++) {
    LOG("%8u: %u\n", i, off_buf[i]);
  }

  // Get the last offset
  last_off = off_buf[num_strings];
  LOG("Last offset: %d\n", last_off);

  uint64_t total_bytes = num_strings * sizeof(uint32_t) + last_off;
  double total_time = stop - start;
  double gib = (double) (num_strings * sizeof(uint32_t) + last_off) / (double) (1 << 30);
  double gbps = (double) total_bytes / total_time * 1E-9;

  LOG("Total bytes written: %lu\n", num_strings * sizeof(uint32_t) + last_off);
  LOG("%f GiB\n", gib);
  LOG("Throughput: %f\n", (double) total_bytes / total_time);
  LOG("%f GB/s\n", gbps);
  LOG("Last char: %2X ... %c\n", (int) val_buf[last_off - 1], val_buf[last_off - 1]);

  printf("%u, %u, %u, %u, %lu, %f, %f, %f\n",
         strlen_min,
         strlen_mask,
         strlen_max,
         num_strings,
         total_bytes,
         total_time,
         gib,
         gbps);

  // print values buffer
  for (uint32_t i = 0; i < last_off;) {
    uint32_t j = i;
    LOG("%8u: ", i);
    for (j = i;
         (j < i + 16) && (j < last_off); j++) {
      LOG("%2X ", (int) val_buf[j]);
    }
    LOG(" ");
    for (j = i;
         (j < i + 16) && (j < last_off); j++) {
      LOG("%c", val_buf[j]);
    }
    LOG("\n");
    i = j;
  }

  terminate_snap(device, action, log);

  LOG("Freeing offsets buffer...\n");
  free(off_buf);
  LOG("Freeing values buffer...\n");
  free(val_buf);

  return 0;

}
