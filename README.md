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

## API docs (Doxygen)

```bash
cmake --preset macos-debug -DHWM14_BUILD_DOCS=ON
cmake --build --preset macos-debug -j --target hwm14_docs
```

Generated HTML is written under `build/<preset>/docs/html/`.

## Golden dataset conversion

```bash
tools/convert_gfortran_to_csv.py \
  --input testdata/gfortran.txt \
  --profiles-out testdata/golden_profiles.csv \
  --dwm-out testdata/golden_dwm07b.csv
```

## Additional docs

- `docs/design_notes.md`
- `docs/numerical_fidelity.md`
- `docs/thread_safety_and_performance.md`
- `docs/licensing.md`
