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
