# API Usage

Author: watsonryan

## Core types

- `hwm14::Inputs`
- `hwm14::Winds`
- `hwm14::Model`

## Loading model data

Preferred explicit load:

```cpp
auto model = hwm14::Model::LoadFromDirectory(data_dir);
if (!model) {
  // handle model.error()
}
```

Legacy search behavior:

```cpp
hwm14::Options options{};
options.allow_env_hwmpath = true;
auto model = hwm14::Model::LoadWithSearchPaths(options);
```

## Evaluators

- `QuietWinds(const Inputs&)`
- `DisturbanceWindsGeo(const Inputs&)`
- `DisturbanceWindsMag(double mlt_h, double mlat_deg, double kp)`
- `TotalWinds(const Inputs&)`
- `Evaluate(const Inputs&)` (alias of `TotalWinds`)

If `ap3 < 0`, total winds are quiet-only by design.

## Error handling

All API functions return `Result<T, Error>`.

```cpp
auto out = model.value().Evaluate(in);
if (!out) {
  // FormatError(out.error())
}
```

## Batch-style usage

The library is single-point API focused. For throughput-oriented usage,
call `Evaluate` in a loop over your input batch and reuse one loaded model.
See `examples/hwm14_batch_cli.cpp` for a concrete pattern.
