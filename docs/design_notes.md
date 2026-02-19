# Design Notes

Author: watsonryan

## Scope

This repository is a pure C++20 port of HWM14 with these components:
- quiet-time model coefficient loading (`hwm123114.bin`)
- QD conversion coefficient loading (`gd2qd.dat`)
- disturbance model coefficient loading (`dwm07b104i.dat`)

Fortran sources under `fortran/` are translation references only and are not
compiled by this build.

## API layers

- Parity-facing API (`hwm14::Model`, `Inputs`, `Winds`) mirrors the reference
  function inputs (`yyddd`, UT seconds, geodetic lat/lon, altitude, ap3).
- Explicit runtime loading is preferred via `LoadFromDirectory(data_dir)`.
- Legacy behavior is available via `LoadWithSearchPaths()` with search order:
  1. current working directory
  2. `../Meta` relative to current working directory
  3. `HWMPATH` (if enabled)
  4. compile-time default test-data directory (if configured)

## Error handling

Library APIs return `Result<T, Error>` and never write logs directly.
Errors include:
- code
- message
- detail
- location

## Thread safety

Model instances hold immutable parsed data after initialization.
No global mutable state is used in the C++ port.
