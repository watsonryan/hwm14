// Author: watsonryan
// Purpose: Resolve required HWM14 data file locations.

#include "hwm14/data_paths.hpp"

#include <cstdlib>

namespace hwm14 {

Result<DataPaths, Error> ResolveDataPaths(const Options& options) {
  std::filesystem::path base = options.data_dir;

  if (base.empty() && options.allow_env_hwmpath) {
    if (const char* env = std::getenv("HWMPATH"); env != nullptr && *env != 0) {
      base = std::filesystem::path(env);
    }
  }

  if (base.empty()) {
#ifdef HWM14_DEFAULT_DATA_DIR
    base = std::filesystem::path(HWM14_DEFAULT_DATA_DIR);
#endif
  }

  if (base.empty()) {
    return Result<DataPaths, Error>::Err(
        MakeError(ErrorCode::kDataPathNotFound, "No data directory provided", {}, "ResolveDataPaths"));
  }

  DataPaths p;
  p.hwm_bin = base / "hwm123114.bin";
  p.dwm_dat = base / "dwm07b104i.dat";
  p.gd2qd_dat = base / "gd2qd.dat";

  if (!std::filesystem::exists(p.hwm_bin) || !std::filesystem::exists(p.dwm_dat) ||
      !std::filesystem::exists(p.gd2qd_dat)) {
    return Result<DataPaths, Error>::Err(
        MakeError(ErrorCode::kDataPathNotFound,
                  "One or more required data files are missing",
                  base.string(),
                  "ResolveDataPaths"));
  }

  return Result<DataPaths, Error>::Ok(p);
}

}  // namespace hwm14
