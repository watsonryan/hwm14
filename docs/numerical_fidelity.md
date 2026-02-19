# Numerical Fidelity

Author: watsonryan

## Current status

The full numerical evaluator is still in progress. Existing tests validate:
- data loading format fidelity for all required data files
- deterministic conversion of reference `gfortran.txt` outputs to structured CSV
- strict path resolution and data initialization behavior

## Floating-point policy

- Strict floating-point behavior is enabled by default (`HWM14_STRICT_FP=ON`).
- Fast-math style flags are intentionally not enabled by default.
- Any future performance options must preserve baseline parity tests.

## Reference vectors

Reference scenario outputs from `testdata/gfortran.txt` are converted to:
- `testdata/golden_profiles.csv`
- `testdata/golden_dwm07b.csv`

These files are intended to anchor parity regression tests as evaluator pieces
are implemented.

## Golden parity tolerances

Current tolerances are scenario-specific:
- `test_golden_dwm_parity.cpp`: `2e-2` m/s
- `test_golden_profiles_parity.cpp`:
  - `2e-2` m/s for height/longitude/day-of-year/magnetic-activity profiles
  - `5e-2` m/s for latitude profile
  - `1.2e-1` m/s for local-time profile

The larger local-time tolerance reflects the known highest residual differences
near the local-time wraparound points while preserving stable cross-build parity.
