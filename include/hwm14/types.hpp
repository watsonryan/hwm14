#pragma once

// Author: watsonryan
// Purpose: Public types for HWM14 inputs, outputs, and options.

#include <filesystem>

namespace hwm14 {

struct Inputs {
  int yyddd{};
  double ut_seconds{};
  double altitude_km{};
  double geodetic_lat_deg{};
  double geodetic_lon_deg{};
  double ap3{};
};

struct Winds {
  double meridional_mps{};
  double zonal_mps{};
};

struct Options {
  bool strict_fp{true};
  bool enable_cache{false};
  bool allow_env_hwmpath{true};
  std::filesystem::path data_dir{};
};

}  // namespace hwm14
