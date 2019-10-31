cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(fletcher_oc_accel VERSION 0.0.0 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(cmake-modules
  GIT_REPOSITORY  https://github.com/abs-tudelft/cmake-modules.git
  GIT_TAG         master
)
FetchContent_MakeAvailable(cmake-modules)
include(CompileUnits)

# Attempt to find oc-accel
find_library(OSNAP osnap HINTS "$ENV{SNAP_ROOT}/software/lib")
if(OSNAP)
  add_library(osnap SHARED IMPORTED GLOBAL)
  set_target_properties(osnap PROPERTIES IMPORTED_LOCATION ${OSNAP})
  target_include_directories(osnap INTERFACE "$ENV{SNAP_ROOT}/software/include")
else()
  message(FATAL_ERROR "OSNAP not found. Please source snap_path.sh in the oc-accel repository...")
endif()

if(DEFINED ENV{FLETCHER_DIR})
  add_subdirectory($ENV{FLETCHER_DIR}/common/c fletcher-c)
else()
  message(FATAL_ERROR "FLETCHER_DIR not defined. Please source env.sh in the Fletcher repository..." )
endif()

option(OSNAP_SIM "Build oc-accel run-time in simulation mode." ON)
if (OSNAP_SIM)
  message("Building fletcher_oc_accel for simulation.")
else()
  add_definitions(-DOSNAP_NSIM)
endif()

add_compile_unit(
  NAME fletcher::oc_accel
  TYPE SHARED
  PRPS
    C_STANDARD 99
  SRCS
    src/fletcher_oc_accel.c
  DEPS
    fletcher::c
    osnap
)

compile_units()
