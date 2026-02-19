// Author: watsonryan
// Purpose: Verify implemented API behavior and composition semantics.

#include <cstdlib>
#include <cmath>
#include <filesystem>

#include "hwm14/hwm14.hpp"

int main() {
  auto model = hwm14::Model::LoadFromDirectory(std::filesystem::path(HWM14_SOURCE_DIR) / "testdata");
  if (!model) {
    return EXIT_FAILURE;
  }

  hwm14::Inputs in{};
  in.yyddd = 95150;
  in.ut_seconds = 43200.0;
  in.altitude_km = 250.0;
  in.geodetic_lat_deg = -45.0;
  in.geodetic_lon_deg = -85.0;
  in.ap3 = 80.0;

  const auto quiet = model.value().QuietWinds(in);
  if (!quiet) {
    return EXIT_FAILURE;
  }

  const auto dist_geo = model.value().DisturbanceWindsGeo(in);
  if (!dist_geo) {
    return EXIT_FAILURE;
  }
  auto quiet_only_in = in;
  quiet_only_in.ap3 = -1.0;
  const auto quiet_only_dist = model.value().DisturbanceWindsGeo(quiet_only_in);
  if (!quiet_only_dist) {
    return EXIT_FAILURE;
  }
  if (quiet_only_dist.value().meridional_mps != 0.0 || quiet_only_dist.value().zonal_mps != 0.0) {
    return EXIT_FAILURE;
  }

  const auto dist_mag = model.value().DisturbanceWindsMag(3.0, 45.0, 4.0);
  if (!dist_mag) {
    return EXIT_FAILURE;
  }

  const auto total = model.value().TotalWinds(in);
  if (!total) {
    return EXIT_FAILURE;
  }
  if (!std::isfinite(total.value().meridional_mps) || !std::isfinite(total.value().zonal_mps)) {
    return EXIT_FAILURE;
  }
  const double dm = std::abs((quiet.value().meridional_mps + dist_geo.value().meridional_mps) - total.value().meridional_mps);
  const double dz = std::abs((quiet.value().zonal_mps + dist_geo.value().zonal_mps) - total.value().zonal_mps);
  if (dm > 1e-9 || dz > 1e-9) {
    return EXIT_FAILURE;
  }

  const auto eval = model.value().Evaluate(in);
  if (!eval) {
    return EXIT_FAILURE;
  }
  if (std::abs(eval.value().meridional_mps - total.value().meridional_mps) > 1e-9 ||
      std::abs(eval.value().zonal_mps - total.value().zonal_mps) > 1e-9) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
