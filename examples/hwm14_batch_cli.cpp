// Author: watsonryan
// Purpose: Batch-style example evaluating multiple points with one loaded model.

#include <cstdlib>
#include <iostream>
#include <vector>

#include "hwm14/hwm14.hpp"
#include "hwm14/logging.hpp"

int main(int argc, char** argv) {
  const auto log = hwm14::MakeStderrLogSink();
  if (argc != 2) {
    hwm14::Log(log, hwm14::LogLevel::kError, "usage: hwm14_batch_cli <data_dir>");
    return EXIT_FAILURE;
  }

  auto model = hwm14::Model::LoadFromDirectory(argv[1]);
  if (!model) {
    hwm14::LogError(log, "model load failed", model.error());
    return EXIT_FAILURE;
  }

  std::vector<hwm14::Inputs> batch;
  for (int i = 0; i < 8; ++i) {
    hwm14::Inputs in{};
    in.yyddd = 95001 + i * 10;
    in.ut_seconds = i * 3000.0;
    in.altitude_km = 150.0 + 20.0 * i;
    in.geodetic_lat_deg = -60.0 + 15.0 * i;
    in.geodetic_lon_deg = -170.0 + 40.0 * i;
    in.ap3 = 30.0;
    batch.push_back(in);
  }

  for (std::size_t i = 0; i < batch.size(); ++i) {
    auto out = model.value().Evaluate(batch[i]);
    if (!out) {
      hwm14::LogError(log, "evaluate failed at row " + std::to_string(i), out.error());
      return EXIT_FAILURE;
    }
    std::cout << i << "," << out.value().meridional_mps << "," << out.value().zonal_mps << "\n";
  }

  return EXIT_SUCCESS;
}
