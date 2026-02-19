/**
 * @file types.hpp
 * @brief Public input/output/options value types for HWM14.
 */
#pragma once

// Author: watsonryan
// Purpose: Public types for HWM14 inputs, outputs, and options.

#include <filesystem>

namespace hwm14 {

/** @brief Inputs expected by the HWM14 evaluator. */
struct Inputs {
  /** @brief YYDDD date code matching the legacy interface convention. */
  int yyddd{};
  /** @brief UTC seconds-of-day. */
  double ut_seconds{};
  /** @brief Geodetic altitude in kilometers. */
  double altitude_km{};
  /** @brief Geodetic latitude in degrees. */
  double geodetic_lat_deg{};
  /** @brief Geodetic longitude in degrees. */
  double geodetic_lon_deg{};
  /** @brief 3-hour geomagnetic index proxy used by HWM14 disturbance terms. */
  double ap3{};
};

/** @brief Horizontal wind components in meters per second. */
struct Winds {
  /** @brief Meridional (+northward) component. */
  double meridional_mps{};
  /** @brief Zonal (+eastward) component. */
  double zonal_mps{};
};

/** @brief Runtime options controlling model load and evaluation policy. */
struct Options {
  /** @brief Enforce strict floating-point behavior for parity-sensitive runs. */
  bool strict_fp{true};
  /** @brief Enable optional runtime caches for repeated evaluations. */
  bool enable_cache{false};
  /** @brief Allow `HWMPATH` environment variable in path resolution. */
  bool allow_env_hwmpath{true};
  /** @brief Optional explicit data directory override. */
  std::filesystem::path data_dir{};
};

}  // namespace hwm14
