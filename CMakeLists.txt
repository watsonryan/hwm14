cmake_minimum_required(VERSION 3.25)

project(hwm14_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HWM14_BUILD_TESTS "Build tests" ON)
option(HWM14_BUILD_EXAMPLES "Build examples" ON)
option(HWM14_BUILD_DOCS "Build Doxygen API docs target" OFF)
option(HWM14_STRICT_FP "Use strict floating-point flags" ON)
option(HWM14_ENABLE_PROFILE "Enable profiling-oriented flags" OFF)
set(HWM14_SANITIZER "" CACHE STRING "Sanitizer: , address, undefined, thread")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(warnings)
include(sanitizers)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(hwm14
  src/hwm14.cpp
  src/error_utils.cpp
  src/data_paths.cpp
  src/hwm_bin_loader.cpp
  src/gd2qd_loader.cpp
  src/dwm_loader.cpp
  src/time_utils.cpp
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

install(TARGETS hwm14
  EXPORT hwm14Targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

set(HWM14_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/hwm14")

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/hwm14ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/hwm14Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/hwm14Config.cmake"
  INSTALL_DESTINATION "${HWM14_INSTALL_CMAKEDIR}"
)

install(EXPORT hwm14Targets
  FILE hwm14Targets.cmake
  NAMESPACE hwm14::
  DESTINATION "${HWM14_INSTALL_CMAKEDIR}"
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/hwm14Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/hwm14ConfigVersion.cmake"
  DESTINATION "${HWM14_INSTALL_CMAKEDIR}"
)

if(HWM14_BUILD_DOCS)
  find_package(Doxygen QUIET)
  if(DOXYGEN_FOUND)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in"
      "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
      @ONLY
    )
    add_custom_target(hwm14_docs
      COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM
    )
  else()
    message(STATUS "Doxygen not found; hwm14_docs target will not be generated.")
  endif()
endif()

if(HWM14_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()
