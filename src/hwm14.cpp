// Author: watsonryan
// Purpose: Core HWM14 model API implementation (pure C++ port in progress).

#include "hwm14/hwm14.hpp"

#include <cmath>
#include <memory>

#include "hwm14/detail/dwm_loader.hpp"
#include "hwm14/detail/gd2qd_loader.hpp"
#include "hwm14/detail/hwm_bin_loader.hpp"

namespace hwm14 {

struct Model::Impl {
  DataPaths paths{};
  detail::HwmBinHeader hwm{};
  detail::Gd2qdData gd2qd{};
  detail::DwmData dwm{};
};

Result<Model, Error> Model::LoadFromDirectory(std::filesystem::path data_dir, Options options) {
  options.data_dir = std::move(data_dir);
  auto paths = ResolveDataPaths(options);
  if (!paths) {
    return Result<Model, Error>::Err(paths.error());
  }

  auto hwm = detail::LoadHwmBinHeader(paths.value().hwm_bin);
  if (!hwm) {
    return Result<Model, Error>::Err(hwm.error());
  }

  auto gd2qd = detail::LoadGd2qdData(paths.value().gd2qd_dat);
  if (!gd2qd) {
    return Result<Model, Error>::Err(gd2qd.error());
  }

  auto dwm = detail::LoadDwmData(paths.value().dwm_dat);
  if (!dwm) {
    return Result<Model, Error>::Err(dwm.error());
  }

  auto impl = std::make_shared<Impl>();
  impl->paths = std::move(paths.value());
  impl->hwm = std::move(hwm.value());
  impl->gd2qd = std::move(gd2qd.value());
  impl->dwm = std::move(dwm.value());

  return Result<Model, Error>::Ok(Model(std::move(impl), options));
}

Result<Winds, Error> Model::Evaluate(const Inputs& in) const {
  if (!std::isfinite(in.ut_seconds) || !std::isfinite(in.altitude_km) || !std::isfinite(in.geodetic_lat_deg) ||
      !std::isfinite(in.geodetic_lon_deg) || !std::isfinite(in.ap3)) {
    return Result<Winds, Error>::Err(
        MakeError(ErrorCode::kInvalidInput, "inputs must be finite", {}, "Model::Evaluate"));
  }
  if (in.altitude_km < 0.0 || in.altitude_km > 5000.0) {
    return Result<Winds, Error>::Err(
        MakeError(ErrorCode::kInvalidInput, "altitude_km must be in [0, 5000]", {}, "Model::Evaluate"));
  }

  // Translation of HWM14 internals to pure C++ is still in progress.
  return Result<Winds, Error>::Err(
      MakeError(ErrorCode::kNotImplemented,
                "pure C++ HWM14 evaluator not implemented yet",
                "data files resolved successfully",
                "Model::Evaluate"));
}

}  // namespace hwm14
