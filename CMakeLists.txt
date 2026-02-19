cmake_minimum_required(VERSION 3.25)

project(hwm14_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HWM14_BUILD_TESTS "Build tests" ON)
option(HWM14_BUILD_EXAMPLES "Build examples" ON)
option(HWM14_STRICT_FP "Use strict floating-point flags" ON)
option(HWM14_ENABLE_PROFILE "Enable profiling-oriented flags" OFF)
set(HWM14_SANITIZER "" CACHE STRING "Sanitizer: , address, undefined, thread")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(warnings)
include(sanitizers)

add_library(hwm14
  src/hwm14.cpp
  src/error_utils.cpp
  src/data_paths.cpp
  src/hwm_bin_loader.cpp
  src/gd2qd_loader.cpp
  src/dwm_loader.cpp
)

add_library(hwm14::hwm14 ALIAS hwm14)

target_include_directories(hwm14
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(hwm14 PRIVATE HWM14_DEFAULT_DATA_DIR="${CMAKE_SOURCE_DIR}/testdata")

if(HWM14_STRICT_FP)
  hwm14_apply_strict_fp_flags(hwm14)
endif()

hwm14_apply_common_warnings(hwm14)
hwm14_apply_runtime_flags(hwm14)

if(HWM14_BUILD_EXAMPLES)
  add_executable(hwm14_cli examples/hwm14_cli.cpp)
  target_link_libraries(hwm14_cli PRIVATE hwm14)
  hwm14_apply_common_warnings(hwm14_cli)
  hwm14_apply_runtime_flags(hwm14_cli)
endif()

if(HWM14_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()
