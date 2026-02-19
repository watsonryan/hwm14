// Author: watsonryan
// Purpose: Implement YYDDD decoding and UT normalization helpers.

#include "hwm14/detail/time_utils.hpp"

#include <cmath>

namespace hwm14::detail {

Result<DecodedYyddd, Error> DecodeYyddd(std::int32_t yyddd) {
  if (yyddd < 0) {
    return Result<DecodedYyddd, Error>::Err(
        MakeError(ErrorCode::kInvalidInput, "yyddd must be non-negative", {}, "DecodeYyddd"));
  }

  const std::int32_t yy = yyddd / 1000;
  const std::int32_t ddd = yyddd % 1000;
  if (ddd < 0 || ddd > 366) {
    return Result<DecodedYyddd, Error>::Err(
        MakeError(ErrorCode::kInvalidInput, "yyddd day-of-year must be in [0, 366]", {}, "DecodeYyddd"));
  }

  DecodedYyddd out{};
  out.yy = yy;
  out.day_of_year = ddd;
  return Result<DecodedYyddd, Error>::Ok(out);
}

double NormalizeUtSeconds(double ut_seconds) {
  constexpr double kDaySec = 86400.0;
  if (!std::isfinite(ut_seconds)) {
    return ut_seconds;
  }
  double t = std::fmod(ut_seconds, kDaySec);
  if (t < 0.0) {
    t += kDaySec;
  }
  return t;
}

}  // namespace hwm14::detail
