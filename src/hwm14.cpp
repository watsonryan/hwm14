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

namespace {

Result<Winds, Error> ValidateCommonInputs(const Inputs& in, std::string_view where) {
  if (!std::isfinite(in.ut_seconds) || !std::isfinite(in.altitude_km) || !std::isfinite(in.geodetic_lat_deg) ||
      !std::isfinite(in.geodetic_lon_deg) || !std::isfinite(in.ap3)) {
    return Result<Winds, Error>::Err(MakeError(ErrorCode::kInvalidInput, "inputs must be finite", {}, std::string(where)));
  }
  if (in.altitude_km < 0.0 || in.altitude_km > 5000.0) {
    return Result<Winds, Error>::Err(
        MakeError(ErrorCode::kInvalidInput, "altitude_km must be in [0, 5000]", {}, std::string(where)));
  }
  return Result<Winds, Error>::Ok(Winds{});
}

}  // namespace

Result<Model, Error> Model::LoadFromResolvedPaths(DataPaths paths, Options options) {
  auto hwm = detail::LoadHwmBinHeader(paths.hwm_bin);
  if (!hwm) {
    return Result<Model, Error>::Err(hwm.error());
  }

  auto gd2qd = detail::LoadGd2qdData(paths.gd2qd_dat);
  if (!gd2qd) {
    return Result<Model, Error>::Err(gd2qd.error());
  }

  auto dwm = detail::LoadDwmData(paths.dwm_dat);
  if (!dwm) {
    return Result<Model, Error>::Err(dwm.error());
  }

  auto impl = std::make_shared<Model::Impl>();
  impl->paths = std::move(paths);
  impl->hwm = std::move(hwm.value());
  impl->gd2qd = std::move(gd2qd.value());
  impl->dwm = std::move(dwm.value());

  return Result<Model, Error>::Ok(Model(std::move(impl), std::move(options)));
}

Result<Model, Error> Model::LoadFromDirectory(std::filesystem::path data_dir, Options options) {
  options.data_dir = std::move(data_dir);
  auto paths = ResolveDataPathsFromDirectory(options.data_dir);
  if (!paths) {
    return Result<Model, Error>::Err(paths.error());
  }
  return LoadFromResolvedPaths(std::move(paths.value()), std::move(options));
}

Result<Model, Error> Model::LoadWithSearchPaths(Options options) {
  auto paths = ResolveDataPathsWithSearchPaths(options);
  if (!paths) {
    return Result<Model, Error>::Err(paths.error());
  }
  return LoadFromResolvedPaths(std::move(paths.value()), std::move(options));
}

Result<Winds, Error> Model::TotalWinds(const Inputs& in) const {
  const auto valid = ValidateCommonInputs(in, "Model::TotalWinds");
  if (!valid) {
    return Result<Winds, Error>::Err(valid.error());
  }

  auto q = QuietWinds(in);
  if (!q) {
    return q;
  }
  if (in.ap3 < 0.0) {
    return q;
  }

  auto d = DisturbanceWindsGeo(in);
  if (!d) {
    return d;
  }

  Winds out{};
  out.meridional_mps = q.value().meridional_mps + d.value().meridional_mps;
  out.zonal_mps = q.value().zonal_mps + d.value().zonal_mps;
  return Result<Winds, Error>::Ok(out);
}

Result<Winds, Error> Model::QuietWinds(const Inputs& in) const {
  const auto valid = ValidateCommonInputs(in, "Model::QuietWinds");
  if (!valid) {
    return Result<Winds, Error>::Err(valid.error());
  }
  return Result<Winds, Error>::Err(MakeError(ErrorCode::kNotImplemented,
                                             "pure C++ quiet-time evaluator not implemented yet",
                                             "data files loaded successfully",
                                             "Model::QuietWinds"));
}

Result<Winds, Error> Model::DisturbanceWindsGeo(const Inputs& in) const {
  const auto valid = ValidateCommonInputs(in, "Model::DisturbanceWindsGeo");
  if (!valid) {
    return Result<Winds, Error>::Err(valid.error());
  }
  if (in.ap3 < 0.0) {
    return Result<Winds, Error>::Ok(Winds{});
  }
  return Result<Winds, Error>::Err(MakeError(ErrorCode::kNotImplemented,
                                             "pure C++ geographic disturbance evaluator not implemented yet",
                                             "data files loaded successfully",
                                             "Model::DisturbanceWindsGeo"));
}

Result<Winds, Error> Model::DisturbanceWindsMag(double mlt_h, double mlat_deg, double kp) const {
  if (!std::isfinite(mlt_h) || !std::isfinite(mlat_deg) || !std::isfinite(kp)) {
    return Result<Winds, Error>::Err(
        MakeError(ErrorCode::kInvalidInput, "inputs must be finite", {}, "Model::DisturbanceWindsMag"));
  }
  return Result<Winds, Error>::Err(MakeError(ErrorCode::kNotImplemented,
                                             "pure C++ magnetic disturbance evaluator not implemented yet",
                                             "data files loaded successfully",
                                             "Model::DisturbanceWindsMag"));
}

Result<Winds, Error> Model::Evaluate(const Inputs& in) const {
  return TotalWinds(in);
}

}  // namespace hwm14
