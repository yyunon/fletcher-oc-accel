# fletcher-snap

[![Build Status](https://dev.azure.com/abs-tudelft/fletcher/_apis/build/status/abs-tudelft.fletcher-snap?branchName=master)](https://dev.azure.com/abs-tudelft/fletcher/_build/latest?definitionId=6&branchName=master)

Fletcher oc-accel platform support.

This platform has been derived from https://github.com/abs-tudelft/fletcher-snap, much like oc-accel is the successor of SNAP (SNAP = CAPI 1.0 and 2.0, ocxl aka. oc-accel is OpenCAPI 3.0).

As this repo contains only the needed platform support, the main fletcher library is needed for all the platform-agnostic parts (including VHDL code, common runtime library, example hardware and software code). That repo is included as a git submodule, but you can also check it out elsewhere yourself. In that case, you need to make sure that the FLETCHER_DIR, to be set in oc-accel/env.sh, points to it.

Platform-specific changes needed in fletcher itself (patch):
```
diff --git a/hardware/interconnect/BusReadBuffer.vhd b/hardware/interconnect/BusReadBuffer.vhd
index 976cbf5..dc55010 100644
--- a/hardware/interconnect/BusReadBuffer.vhd
+++ b/hardware/interconnect/BusReadBuffer.vhd
@@ -205,12 +205,12 @@ begin
           -- Check if the request burst length is not larger than the FIFO depth
           assert unsigned(ms_req_len) < 2**DEPTH_LOG2
             report "Violated burst length requirement. ms_req_len(=" & slvToUDec(ms_req_len) & ") < 2**DEPTH_LOG2(=" & integer'image(2**DEPTH_LOG2) & ") not met, deadlock!"
-            severity FAILURE;
+            severity WARNING;
 
           -- Check if either the amount of space reserved is larger than 0 or the fifo is ready
           assert reserved_v > 0 or fifo_ready = '1'
             report "Bus buffer deadlock!"
-            severity FAILURE;
+            severity WARNING;
 
           -- Check if the amount of space reserved is equal or larger than 0 after the reservation
           assert reserved_v >= 0
@@ -218,7 +218,7 @@ begin
                    "Check if BUS_LEN_WIDTH is wide enough to contain log2(slv_rreq_len)+2 bits. " &
                    "reserved_v=" & sgnToDec(reserved_v) & ">= 0. " &
                    "Reserved (if accepted):" & sgnToDec(reserved_if_accepted)
-            severity FAILURE;
+            severity WARNING;
 
           -- pragma translate_on

diff --git a/examples/sum/hardware/vhdl/Sum.vhd b/examples/sum/hardware/vhdl/Sum.vhd
index 8b878b9..abee117 100644
--- a/examples/sum/hardware/vhdl/Sum.vhd
+++ b/examples/sum/hardware/vhdl/Sum.vhd
@@ -80,7 +80,10 @@ begin
   -- (registers).
   
   -- Combinatorial part:
-  combinatorial_proc : process (all) is 
+  combinatorial_proc : process (ExampleBatch_number, ExampleBatch_number_last, 
+ExampleBatch_number_valid, accumulator, ExampleBatch_firstIdx, 
+ExampleBatch_lastIdx, state, start, reset, ExampleBatch_number_cmd_ready, 
+ExampleBatch_number_unl_valid) is 
   begin
     
     -- We first determine the default outputs of our combinatorial circuit.
     
diff --git a/stream/StreamPipelineControl.vhd b/stream/StreamPipelineControl.vhd
index 034643a..cefce8b 100644
--- a/stream/StreamPipelineControl.vhd
+++ b/stream/StreamPipelineControl.vhd
@@ -226,12 +226,12 @@ begin
         end if;
         if out_valid_s = '1' and out_ready = '1' then
             assert fifo_reserved_v > 0
-            report "FIFO underflow possible!" severity failure;
+            report "FIFO underflow possible!" severity warning;
             fifo_reserved_v := fifo_reserved_v - 1;
         end if;
         if pipe_valid_s(pipe_valid_s'HIGH) = '1' and pipe_delete = '1' then
             assert fifo_reserved_v > 0
-            report "FIFO underflow possible!" severity failure;
+            report "FIFO underflow possible!" severity warning;
             fifo_reserved_v := fifo_reserved_v - 1;
         end if;
         fifo_reserved <= fifo_reserved_v;
```

Important: When you wish to run one of the examples, you will need to run `make` in the corresponding example directory in the fletcher directory. In addition, you need to add the flag `--axi` to the `fletchgen` command in the hw Makefile. Otherwise, the AxiTop file is not generated and will remain a black box. This is not an error for oc-accel so it will crash (hang) the ocse simulation without notice.
