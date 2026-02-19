/**
 * @file data_paths.cpp
 * @brief Implementation of runtime HWM14 data file path resolution.
 */

#include "hwm14/data_paths.hpp"

#include <cstdlib>
#include <vector>

namespace hwm14 {

namespace {

Result<DataPaths, Error> BuildPathsFromBase(const std::filesystem::path& base) {
  DataPaths p;
  p.hwm_bin = base / "hwm123114.bin";
  p.dwm_dat = base / "dwm07b104i.dat";
  p.gd2qd_dat = base / "gd2qd.dat";
  if (std::filesystem::exists(p.hwm_bin) && std::filesystem::exists(p.dwm_dat) && std::filesystem::exists(p.gd2qd_dat)) {
    return Result<DataPaths, Error>::Ok(std::move(p));
  }
  return Result<DataPaths, Error>::Err(
      MakeError(ErrorCode::kDataPathNotFound, "One or more required data files are missing", base.string(), "BuildPathsFromBase"));
}

}  // namespace

Result<DataPaths, Error> ResolveDataPathsFromDirectory(const std::filesystem::path& data_dir) {
  if (data_dir.empty()) {
    return Result<DataPaths, Error>::Err(
        MakeError(ErrorCode::kDataPathNotFound, "No data directory provided", {}, "ResolveDataPathsFromDirectory"));
  }
  return BuildPathsFromBase(data_dir);
}

Result<DataPaths, Error> ResolveDataPathsWithSearchPaths(const Options& options) {
  std::vector<std::filesystem::path> candidates;
  candidates.reserve(4);
  candidates.push_back(std::filesystem::current_path());
  candidates.push_back(std::filesystem::current_path() / ".." / "Meta");

  if (options.allow_env_hwmpath) {
    if (const char* env = std::getenv("HWMPATH"); env != nullptr && *env != 0) {
      candidates.emplace_back(env);
    }
  }

#ifdef HWM14_DEFAULT_DATA_DIR
  candidates.emplace_back(HWM14_DEFAULT_DATA_DIR);
#endif

  for (const auto& base : candidates) {
    const auto p = BuildPathsFromBase(base);
    if (p) {
      return p;
    }
  }

  return Result<DataPaths, Error>::Err(MakeError(ErrorCode::kDataPathNotFound,
                                                  "No valid HWM14 data directory found in search paths",
                                                  {},
                                                  "ResolveDataPathsWithSearchPaths"));
}

Result<DataPaths, Error> ResolveDataPaths(const Options& options) {
  if (!options.data_dir.empty()) {
    return ResolveDataPathsFromDirectory(options.data_dir);
  }
  return ResolveDataPathsWithSearchPaths(options);
}

}  // namespace hwm14
