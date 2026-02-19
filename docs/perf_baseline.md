# Performance Baseline

Author: watsonryan

## Environment

- preset: `macos-profile`
- benchmark: `tests/hwm14_perf_benchmark`
- dataset: `testdata/`
- settings: `HWM14_PERF_SAMPLES=40`, `HWM14_PERF_ITERATIONS=5000`

## Commands

```bash
cmake --preset macos-profile
cmake --build --preset macos-profile -j
HWM14_PERF_SAMPLES=40 HWM14_PERF_ITERATIONS=5000 ./build/macos-profile/tests/hwm14_perf_benchmark
```

## Results

Before allocation optimization:
- `ns_per_eval=4082.35`

After allocation optimization:
- `ns_per_eval=3801.48`
- `ns_per_eval=3806.46`
- `ns_per_eval=3664.19`

After second hot-path scratch-buffer optimization:
- `ns_per_eval=3496.52`
- `ns_per_eval=3504.88`

Observed improvement range:
- first optimization: ~6.8% to ~10.2% faster
- second optimization vs original baseline: ~14.1% to ~14.3% faster

## Numerical safety

- Full test/parity suite remains passing after optimization.
- Benchmark checksum remains stable across compared runs.
