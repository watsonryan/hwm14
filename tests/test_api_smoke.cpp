// Author: watsonryan
// Purpose: Verify API error behavior for current pure C++ evaluator status.

#include <cstdlib>
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

  const auto out = model.value().Evaluate(in);
  if (out) {
    return EXIT_FAILURE;
  }
  if (out.error().code != hwm14::ErrorCode::kNotImplemented) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
