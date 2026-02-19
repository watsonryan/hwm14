// Author: watsonryan
// Purpose: Validate YYDDD decoding and UT normalization utility behavior.

#include <cmath>
#include <cstdlib>

#include "hwm14/detail/time_utils.hpp"

int main() {
  const auto d = hwm14::detail::DecodeYyddd(95150);
  if (!d) {
    return EXIT_FAILURE;
  }
  if (d.value().yy != 95 || d.value().day_of_year != 150) {
    return EXIT_FAILURE;
  }

  const auto bad0 = hwm14::detail::DecodeYyddd(-1);
  if (bad0 || bad0.error().code != hwm14::ErrorCode::kInvalidInput) {
    return EXIT_FAILURE;
  }

  const auto bad1 = hwm14::detail::DecodeYyddd(95000);
  if (bad1 || bad1.error().code != hwm14::ErrorCode::kInvalidInput) {
    return EXIT_FAILURE;
  }

  const auto bad2 = hwm14::detail::DecodeYyddd(95367);
  if (bad2 || bad2.error().code != hwm14::ErrorCode::kInvalidInput) {
    return EXIT_FAILURE;
  }

  if (std::abs(hwm14::detail::NormalizeUtSeconds(90000.0) - 3600.0) > 1e-12) {
    return EXIT_FAILURE;
  }
  if (std::abs(hwm14::detail::NormalizeUtSeconds(-1.0) - 86399.0) > 1e-12) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
