# HWM14 (jacobwilliams/HWM14) — Production‑Quality C++20 Port Plan

**Repository to port:** `https://github.com/jacobwilliams/HWM14`  
**Objective:** Re-implement the HWM14 model (quiet-time HWM + disturbance winds DWM07 + QD coordinate conversion) as a **robust, efficient, flight-software-quality** **C++20** library with:

- **CMake + CMakePresets** (macOS/Linux/Windows): Debug / Release / Sanitize / Profile  
- **GoogleTest** for unit tests  
- **CPM.cmake** for third-party dependencies  
- **No Docker / dev containers**  
- **Small, targeted git commits** with concise messages (**do not mention any tooling in commit messages**)  
- **All files must include** `Author: waston` in a short header at the top

---

## 0) Licensing / redistribution gate (must be the first commit after scaffolding)

This repo is an “unofficial mirror” and `fpm.toml` lists `license = "?"`. Treat licensing as **unknown** until proven otherwise.

**Hard rule:** before implementing algorithms or redistributing coefficient/data files, a human must confirm that:
- derivative works are permitted,
- data files may be redistributed,
- intended “production/flight” use is permitted.

Deliverable:
- `docs/licensing.md` documenting:
  - upstream origin and terms (if any),
  - what can be redistributed,
  - whether a “clean-room” reimplementation is required,
  - how data files are sourced (bundled vs user-supplied).

> If redistribution is not allowed, design the build so the library can be built without bundling the data files; users must supply them at runtime.

---

## 1) What HWM14 is in this repo (scope + required behavior)

### 1.1 Model pieces included
From repo docs and source structure, the port must cover:

- **HWM14 total winds** (`hwm14`) = quiet-time + disturbance (if magnetic activity provided)
- **Quiet-time model** (QWM) using coefficients from **`hwm123114.bin`** (Fortran unformatted file)
- **Disturbance winds** (DWM07):
  - geographic disturbance winds (`dwm07`)
  - magnetic-coordinate disturbance winds (`dwm07b`)
  - parameters from **`dwm07b104i.dat`**
- **QD coordinate conversion** (`gd2qd`) using **`gd2qd.dat`**
- Utility routines: local time handling (`pershift`), magnetic local time (`mltcalc`), ap↔kp conversions used in DWM, Legendre / vector spherical harmonics routines (`alfbasis`), B-splines (`bspline`, `findspan`), etc.

### 1.2 Fortran interface semantics that must be preserved
The reference interface (Fortran) is:

```
subroutine hwm14(iyd, sec, alt, glat, glon, stl, f107a, f107, ap, w)
```

Key semantics to match:
- `iyd` is `YYDDD` (year-of-century + day-of-year)
- `sec` is UT seconds
- `alt` is altitude in km
- `glat/glon` are geodetic latitude/longitude in degrees
- `stl`, `f107a`, `f107` are effectively ignored (HWM14 in this version has no solar dependence)
- `ap(2)` is the **current 3‑hour ap index** (ap(1) ignored)
- If `ap(2) < 0`, the disturbance wind contribution is not added (quiet-time only)
- Outputs `w(1)=meridional (north)`, `w(2)=zonal (east)` in m/s

### 1.3 Numerical behavior expectations
Upstream notes indicate small numerical differences can occur across compilers/optimizations (~1e‑4 m/s). The port must:
- be deterministic given the same build options and data,
- default to “strict FP” (no fast-math),
- accept tiny platform differences via well-chosen test tolerances,
- provide an opt-in “fast” build if needed (after parity is locked in).

---

## 2) Overall strategy (tests-first, golden vectors)

**Do not attempt a big-bang rewrite.** Port behavior incrementally using the reference test driver and expected outputs provided in the repo.

The repo provides:
- `test/checkhwm14.f90` — a deterministic scenario generator
- `test/gfortran.txt`, `test/ifort.opt.txt`, `test/ifort.nopt.txt` — reference outputs

Treat these outputs as the observable specification and build C++ tests that reproduce them.

---

## 3) Commit discipline (required)

Work must proceed through **small, targeted commits**.

### 3.1 Author identity
Configure git author once:
```bash
git config user.name "waston"
git config user.email "waston@example.com"
```

### 3.2 Commit flow
Each step:
```bash
git status
git diff
git add <small set of files>
git commit -m "Concise message"
```

### 3.3 Commit message examples
- `Add CMake project skeleton and presets`
- `Integrate CPM and GoogleTest`
- `Add strict warnings and sanitizer options`
- `Add Fortran unformatted reader utility`
- `Implement HWM14 coefficient loader`
- `Port check driver scenarios as golden tests`
- `Implement quiet-time evaluation`
- `Implement DWM07 disturbance winds`
- `Implement gd2qd coordinate conversion`
- `Add batch evaluation API`

Never mention any assistant/tooling in commit messages.

---

## 4) Target repository layout (C++)

```
/CMakeLists.txt
/CMakePresets.json
/cmake/
  CPM.cmake
  warnings.cmake
  sanitizers.cmake
  profiling.cmake
/include/hwm14/
  hwm14.hpp
  types.hpp
  error.hpp
  data_paths.hpp
  detail/fortran_unformatted.hpp
  detail/math.hpp
/src/
  hwm14.cpp
  qwm.cpp
  dwm07.cpp
  gd2qd.cpp
  coeff_hwm_bin.cpp
  coeff_dwm_dat.cpp
  coeff_gd2qd_dat.cpp
  data_paths.cpp
/tests/
  CMakeLists.txt
  test_golden_profiles.cpp
  test_dwm07b_profiles.cpp
  test_io_unformatted.cpp
  test_path_resolution.cpp
/testdata/
  gfortran.txt          # copied from upstream (or converted CSV)
/examples/
  hwm14_cli.cpp
/docs/
  licensing.md
  numerical_fidelity.md
  design_notes.md
/.clang-format
/.clang-tidy
/README.md
```

All new files: include a header:
```cpp
// Author: waston
// Purpose: <one-line description>
```

---

## 5) Build system requirements

### 5.1 CMake
- Require **CMake ≥ 3.25**
- `CMAKE_CXX_STANDARD 20`, required.
- Provide targets:
  - `hwm14` (library)
  - `hwm14_tests` (gtest)
  - `hwm14_cli` (example)

### 5.2 Presets (no containers)
Provide **configure + build presets**:

**macOS**
- `macos-debug`, `macos-release`, `macos-sanitize`, `macos-profile`

**Linux**
- `linux-debug`, `linux-release`, `linux-sanitize`, `linux-profile`
- optionally `linux-tsan`

**Windows**
- `windows-debug`, `windows-release`, `windows-sanitize`, `windows-profile`
  - sanitizer preset uses MSVC ASan or clang-cl if available; document constraints

Each preset must set:
- Generator: Ninja preferred
- Warnings: high
- Strict FP defaults:
  - GCC/Clang: `-fno-fast-math -ffp-contract=off`
  - MSVC: `/fp:precise`

### 5.3 Options
- `HWM14_BUILD_TESTS` (ON)
- `HWM14_BUILD_EXAMPLES` (OFF)
- `HWM14_WERROR` (OFF default; ON in CI)
- `HWM14_STRICT_FP` (ON default)
- `HWM14_ENABLE_CACHE` (OFF default; see thread safety below)
- `HWM14_BUNDLE_DATA` (OFF by default until licensing clarified)
- `HWM14_NO_EXCEPTIONS` (OFF; optional)

---

## 6) Dependencies via CPM

Use **CPM.cmake** (committed) for:
- **GoogleTest** (required)
- Optional (keep core library minimal):
  - `tl::expected` (or implement a tiny internal `expected`)
  - `fmt` and/or `CLI11` for CLI example only

No runtime dependency bloat in the core library.

---

## 7) C++ API design (two layers)

### 7.1 Parity layer (reference-faithful)
Provide functions that mirror the Fortran entry points:

```cpp
namespace hwm14 {

struct Inputs {
  int yyddd;
  double ut_seconds;
  double altitude_km;
  double geodetic_lat_deg;
  double geodetic_lon_deg;
  double ap3;     // ap(2); negative => quiet only
};

struct Winds {
  double meridional_mps; // north
  double zonal_mps;      // east
};

class Model {
public:
  struct Options {
    bool strict_fp = true;
    bool enable_cache = false;    // per-instance cache, see thread safety
    bool allow_env_hwmpath = true;
  };

  static expected<Model, Error> LoadFromDirectory(std::filesystem::path data_dir, Options);
  static expected<Model, Error> LoadWithSearchPaths(Options); // replicate HWMPATH + relative search

  Winds TotalWinds(const Inputs&) const;          // equivalent to hwm14
  Winds QuietWinds(const Inputs&) const;          // equivalent to hwm14 with ap3 < 0
  Winds DisturbanceWindsGeo(const Inputs&) const; // equivalent to dwm07 (geo)
  Winds DisturbanceWindsMag(double mlt_h, double mlat_deg, double kp) const; // dwm07b
};

} // namespace hwm14
```

### 7.2 Modern layer (production-facing)
Add an idiomatic interface after parity:
- separate `Year`, `DayOfYear`, `UTSeconds`, `Degrees`, etc.
- batch evaluation with `std::span`
- explicit data-path injection (no environment dependency by default)

---

## 8) Thread safety and caching

The Fortran implementation uses global module state and `save` variables for caching last inputs. This is **not thread-safe**.

C++ design must avoid global mutable state. Offer:

### 8.1 Safe default (recommended)
- No caching in the model object
- Every evaluation recomputes intermediate basis values
- Deterministic, thread-safe, simpler

### 8.2 Optional caching (performance)
- `Options::enable_cache = true` enables a per-instance mutable cache guarded by a mutex, or:
- Provide a caller-owned `Workspace` object:

```cpp
struct Workspace {
  // preallocated arrays for alfbasis, vsh terms, spline weights, etc.
};
Winds TotalWinds(const Inputs&, Workspace&) const;
```

Workspace approach gives performance without shared mutation and is flight-software friendly.

---

## 9) Data file handling (critical)

### 9.1 Required data files
The port must support the same three files:
- `hwm123114.bin` (Fortran unformatted sequential binary)
- `dwm07b104i.dat` (DWM parameters; format defined by Fortran reader)
- `gd2qd.dat` (QD conversion parameters; format defined by Fortran reader)

### 9.2 Search path behavior
The Fortran `findandopen()` searches:
1) working directory,
2) `../Meta/` relative to run directory,
3) directory in environment variable `HWMPATH`.

For production reliability:
- Provide an explicit `LoadFromDirectory(data_dir)` API (preferred).
- Provide `LoadWithSearchPaths()` to preserve legacy behavior.
- Never rely solely on CWD in tests; tests must set an explicit data dir.

### 9.3 Fortran unformatted reader (hwm123114.bin)
Implement a robust reader for Fortran unformatted sequential records:
- detect record-marker size (4 vs 8 bytes)
- detect endianness (swap if needed)
- verify leading/trailing record sizes match
- expose typed reads:
  - `int32`, `float32`, `float64`
- provide clear errors on mismatch

Use this to replicate `initqwm()`’s record sequence:
- integers: `nbf,maxs,maxm,maxl,maxn,ncomp`
- integers: `nlev,p`
- array: `vnode`
- loops of `order`, `nb`, `mparm`
- final: `e1,e2`

**Safety rule:** initialize every array to known values (0) before use.

---

## 10) Numerical fidelity plan

### 10.1 Default compile settings
- Strict FP mode ON by default.
- Avoid re-association and FMA contraction where possible.

### 10.2 Data type choices
The Fortran uses mixed precision (many `real(4)` inputs/outputs; many internal arrays in `real(8)`).
To match behavior and remain robust:
- store coefficients in `double` where they originate as double in the binary
- do computations in `double`
- return `double` winds
- for parity tests vs printed 3-decimal outputs, use tolerances

After parity:
- optional “float core” build for speed can be explored, but only if tests remain stable.

### 10.3 Document platform drift
Create `docs/numerical_fidelity.md`:
- expected tolerances,
- what flags are required to reproduce reference results,
- known differences across compilers.

---

## 11) Unit testing plan (GTest)

### 11.1 Port the reference scenarios
`test/checkhwm14.f90` prints these profiles; replicate them in C++ tests:

- height profile (quiet, disturbed, total)
- latitude profile
- local time profile
- longitude profile
- day-of-year profile
- magnetic activity (ap) profile
- DWM07b magnetic latitude profile
- DWM07b magnetic local time profile
- DWM07b kp profile

Use `test/gfortran.txt` as the golden reference dataset:
- either parse the text format directly, or
- convert once into a structured CSV/JSON committed into `testdata/`

### 11.2 Parsing strategy (recommended)
Do not write a brittle parser for the full text blocks. Instead:
- build a small offline converter (in `tools/`) that reads `gfortran.txt` and emits:
  - `golden_profiles.csv` with columns:
    - scenario, independent_variable, quiet_mer, quiet_zon, dist_mer, dist_zon, total_mer, total_zon
  - `golden_dwm07b.csv` with columns:
    - scenario, x, mag_mer, mag_zon
- commit the CSV outputs and use them in tests.

(If licensing forbids copying the text outputs, generate these vectors internally and store only the numeric tables that are allowed.)

### 11.3 Tolerances
Since reference outputs are printed to 3 decimals, start with:
- `EXPECT_NEAR(value, expected, 5e-4)` for winds (0.5 mm/s)
- adjust if any platform fails, but keep tight

Also test internal invariants:
- quiet + disturbed ≈ total (within tolerance) when ap3 >= 0
- for ap3 < 0, total == quiet (within tolerance)

### 11.4 IO tests
- verify `LoadFromDirectory()` fails gracefully if a file is missing
- verify Fortran unformatted reader detects corruption/truncation
- verify search path resolution matches expected precedence

---

## 12) Performance plan (after parity lock)

- Add batch evaluation:
  ```cpp
  void TotalWindsBatch(std::span<const Inputs> in, std::span<Winds> out) const;
  ```
- Provide `Workspace` to eliminate allocations and enable caching of repeated lat/lon/time patterns.
- Profile with the `profile` preset before introducing micro-optimizations.

---

## 13) CI plan (recommended for “production quality”)

Matrix:
- Ubuntu: GCC + Clang
- macOS: Clang
- Windows: MSVC

Build configurations:
- Debug + tests
- Release + tests
- Sanitizer preset where supported:
  - Linux/macOS ASan+UBSan
  - Windows ASan (if supported)

Quality gates:
- clang-format check
- clang-tidy (start non-gating; later gate)

---

## 14) Definition of Done (acceptance criteria)

- ✅ Builds on macOS/Linux/Windows using presets (Debug/Release/Sanitize/Profile)
- ✅ All golden tests pass and reproduce reference scenario outputs within documented tolerances
- ✅ No global mutable state; model instance is re-entrant and thread-safe by default
- ✅ Robust data loading with explicit path API + legacy search API
- ✅ Sanitizers clean in supported configurations (or documented exceptions)
- ✅ Clear documentation: usage, data files, fidelity notes, known limitations
- ✅ No Docker/devcontainer artifacts present

---

## 15) Suggested implementation sequence (small commits)

1. Scaffold CMake project + presets + formatting configs  
   `Add CMake project skeleton and presets`

2. Add CPM + GoogleTest + test runner  
   `Integrate CPM and GoogleTest`

3. Add core types + error/expected utilities  
   `Add core types and error handling`

4. Implement data path resolution (explicit dir + search paths)  
   `Add data path resolution utilities`

5. Implement Fortran unformatted reader + IO unit tests  
   `Add Fortran unformatted reader`

6. Implement `hwm123114.bin` loader and validate shapes  
   `Implement HWM quiet-time coefficient loader`

7. Implement math primitives (Legendre, alfbasis, bsplines) with unit tests  
   `Add spherical harmonic and spline utilities`

8. Implement quiet-time evaluation (`hwmqt`) and golden tests for quiet profiles  
   `Implement quiet-time winds`

9. Implement QD conversion (`gd2qd`) + DWM07b + golden tests  
   `Implement QD conversion and magnetic disturbance winds`

10. Implement geographic disturbance (`dwm07`) and total winds (`hwm14`) + golden tests  
   `Implement total winds composition`

11. Add batch API + workspace performance path (optional)  
   `Add batch evaluation API`

12. Add CLI example (optional)  
   `Add hwm14 CLI example`

13. Documentation pass  
   `Add documentation for usage and fidelity`

---

## 16) File header requirement (Author: waston)

All new `.hpp/.cpp/.cmake` files must include:

**C++**
```cpp
// Author: waston
// Purpose: <one-line description>
```

**CMake**
```cmake
# Author: waston
# Purpose: <one-line description>
```

Keep headers short and consistent.
