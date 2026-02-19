// Author: watsonryan
// Purpose: Minimal CLI for HWM14 single-point evaluation.

#include <cstdlib>
#include <iostream>

#include "hwm14/hwm14.hpp"
#include "hwm14/logging.hpp"

int main(int argc, char** argv) {
  const auto log = hwm14::MakeStderrLogSink();

  if (argc != 7) {
    hwm14::Log(log,
               hwm14::LogLevel::kError,
               "usage: hwm14_cli <data_dir> <yyddd> <ut_seconds> <alt_km> <glat_deg> <glon_deg>");
    return EXIT_FAILURE;
  }

  auto model = hwm14::Model::LoadFromDirectory(argv[1]);
  if (!model) {
    hwm14::LogError(log, "model load failed", model.error());
    return EXIT_FAILURE;
  }

  hwm14::Inputs in{};
  in.yyddd = std::atoi(argv[2]);
  in.ut_seconds = std::atof(argv[3]);
  in.altitude_km = std::atof(argv[4]);
  in.geodetic_lat_deg = std::atof(argv[5]);
  in.geodetic_lon_deg = std::atof(argv[6]);
  in.ap3 = -1.0;

  auto out = model.value().Evaluate(in);
  if (!out) {
    hwm14::LogError(log, "evaluate failed", out.error());
    return EXIT_FAILURE;
  }

  std::cout << "meridional_mps=" << out.value().meridional_mps << "\n";
  std::cout << "zonal_mps=" << out.value().zonal_mps << "\n";
  return EXIT_SUCCESS;
}
