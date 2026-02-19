# hwm14-cpp

Author: watson

HWM14 -- empirical model of the horizontal neutral wind in the upper thermosphere -- https://ccmc.gsfc.nasa.gov/models/HWM14~2014/

C++20 port workspace for HWM14.

## Status

This repository provides a pure C++20 implementation of HWM14.

## Build

```bash
cmake --preset macos-debug
cmake --build --preset macos-debug -j
ctest --preset macos-debug --output-on-failure
```

## API quick start

```cpp
#include "hwm14/hwm14.hpp"

auto model = hwm14::Model::LoadFromDirectory(data_dir);
if (!model) {
  // handle model.error()
}

hwm14::Inputs in{};
in.yyddd = 95150;
in.ut_seconds = 12.0 * 3600.0;
in.altitude_km = 250.0;
in.geodetic_lat_deg = -45.0;
in.geodetic_lon_deg = -85.0;
in.ap3 = 80.0;

auto out = model.value().Evaluate(in);
if (!out) {
  // handle out.error()
}
```

Batch-style example:

```bash
./build/macos-debug/hwm14_batch_cli /path/to/hwm14/data_dir
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

## Profiling

```bash
tools/profile_hwm14.sh
```

Optional overrides:
- `HWM14_PERF_SAMPLES`
- `HWM14_PERF_ITERATIONS`

## Additional docs

- `docs/design_notes.md`
- `docs/numerical_fidelity.md`
- `docs/thread_safety_and_performance.md`
- `docs/licensing.md`
- `docs/perf_baseline.md`
- `docs/api_usage.md`

Parity tolerances are documented in `docs/numerical_fidelity.md`.

## Code quality config

- `.clang-format`
- `.clang-tidy`
- `.github/workflows/ci.yml`

CI currently validates:
- Linux debug build/tests
- Linux ASan build/tests
- Linux UBSan build/tests
- install/package consumer smoke path

## Error handling and logging

- Library APIs return `Result<T, Error>` and do not emit logs directly.
- CLI/tests can use `include/hwm14/logging.hpp` (`LogSink`, `Log`, `LogError`).
