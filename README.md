# hwm14-cpp

Author: watsonryan

C++20 port workspace for HWM14.

## Status

This repository is being migrated to a pure C++20 implementation.
Reference Fortran files under `fortran/` are used only as translation reference,
not compiled into the C++ build.

## Build

```bash
cmake --preset macos-debug
cmake --build --preset macos-debug -j
ctest --preset macos-debug --output-on-failure
```

## Install and package consumption

```bash
cmake --preset macos-release
cmake --build --preset macos-release -j
cmake --install build/macos-release --prefix /tmp/hwm14-install
```

Downstream CMake usage:

```cmake
find_package(hwm14 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE hwm14::hwm14)
```

End-to-end package smoke check:

```bash
tools/validate_package_install.sh
```
