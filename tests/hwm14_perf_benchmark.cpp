// Author: watsonryan
// Purpose: Measure HWM14 evaluation throughput for profiling and regression tracking.

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "hwm14/hwm14.hpp"

namespace {

int ReadEnvInt(const char* name, int fallback) {
  if (const char* v = std::getenv(name)) {
    const int x = std::atoi(v);
    if (x > 0) {
      return x;
    }
  }
  return fallback;
}

}  // namespace

int main() {
  const int samples = ReadEnvInt("HWM14_PERF_SAMPLES", 20);
  const int iterations = ReadEnvInt("HWM14_PERF_ITERATIONS", 2000);

  auto model = hwm14::Model::LoadFromDirectory(std::filesystem::path(HWM14_SOURCE_DIR) / "testdata");
  if (!model) {
    std::cerr << hwm14::FormatError(model.error()) << "\n";
    return EXIT_FAILURE;
  }

  std::vector<hwm14::Inputs> in;
  in.reserve(static_cast<std::size_t>(samples));
  for (int i = 0; i < samples; ++i) {
    hwm14::Inputs x{};
    x.yyddd = 95001 + (i * 13) % 365;
    x.ut_seconds = (i * 1234) % 86400;
    x.altitude_km = 100.0 + (i * 17) % 400;
    x.geodetic_lat_deg = -80.0 + (i * 9) % 160;
    x.geodetic_lon_deg = -180.0 + (i * 23) % 360;
    x.ap3 = static_cast<double>((i * 7) % 200);
    in.push_back(x);
  }

  volatile double checksum = 0.0;
  for (const auto& x : in) {
    auto w = model.value().Evaluate(x);
    if (!w) {
      return EXIT_FAILURE;
    }
    checksum += w.value().meridional_mps + w.value().zonal_mps;
  }

  const auto t0 = std::chrono::steady_clock::now();
  for (int it = 0; it < iterations; ++it) {
    for (const auto& x : in) {
      auto w = model.value().Evaluate(x);
      if (!w) {
        return EXIT_FAILURE;
      }
      checksum += w.value().meridional_mps + w.value().zonal_mps;
    }
  }
  const auto t1 = std::chrono::steady_clock::now();

  const auto evals = static_cast<double>(iterations) * static_cast<double>(samples);
  const double sec = std::chrono::duration<double>(t1 - t0).count();
  const double ns_per_eval = (sec * 1e9) / evals;

  std::cout << "samples=" << samples << " iterations=" << iterations << " evals=" << static_cast<long long>(evals)
            << " seconds=" << sec << " ns_per_eval=" << ns_per_eval << " checksum=" << checksum << "\n";
  return EXIT_SUCCESS;
}
