#pragma once

// Author: watsonryan
// Purpose: Runtime data path resolution for HWM14 coefficient files.

#include <filesystem>

#include "hwm14/error.hpp"
#include "hwm14/result.hpp"
#include "hwm14/types.hpp"

namespace hwm14 {

struct DataPaths {
  std::filesystem::path hwm_bin;
  std::filesystem::path dwm_dat;
  std::filesystem::path gd2qd_dat;
};

[[nodiscard]] Result<DataPaths, Error> ResolveDataPathsFromDirectory(const std::filesystem::path& data_dir);
[[nodiscard]] Result<DataPaths, Error> ResolveDataPathsWithSearchPaths(const Options& options);
[[nodiscard]] Result<DataPaths, Error> ResolveDataPaths(const Options& options);

}  // namespace hwm14
