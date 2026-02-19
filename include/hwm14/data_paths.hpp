/**
 * @file data_paths.hpp
 * @brief Runtime resolution of required HWM14 data file paths.
 */
#pragma once

// Author: watsonryan
// Purpose: Runtime data path resolution for HWM14 coefficient files.

#include <filesystem>

#include "hwm14/error.hpp"
#include "hwm14/result.hpp"
#include "hwm14/types.hpp"

namespace hwm14 {

/** @brief Canonical absolute/relative paths to required HWM14 data files. */
struct DataPaths {
  /** @brief Quiet-time model binary coefficients. */
  std::filesystem::path hwm_bin;
  /** @brief Disturbance wind coefficient file. */
  std::filesystem::path dwm_dat;
  /** @brief Geographic<->quasi-dipole transform coefficients. */
  std::filesystem::path gd2qd_dat;
};

/** @brief Resolve required files from a caller-provided directory. */
[[nodiscard]] Result<DataPaths, Error> ResolveDataPathsFromDirectory(const std::filesystem::path& data_dir);
/** @brief Resolve required files from search paths (cwd, options, env). */
[[nodiscard]] Result<DataPaths, Error> ResolveDataPathsWithSearchPaths(const Options& options);
/** @brief Resolve required files using options/default policy. */
[[nodiscard]] Result<DataPaths, Error> ResolveDataPaths(const Options& options);

}  // namespace hwm14
