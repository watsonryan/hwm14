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
