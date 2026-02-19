#pragma once

// Author: watsonryan
// Purpose: Time/date helpers for HWM14 parity input semantics.

#include <cstdint>

#include "hwm14/error.hpp"
#include "hwm14/result.hpp"

namespace hwm14::detail {

struct DecodedYyddd {
  std::int32_t yy{};
  std::int32_t day_of_year{};
};

[[nodiscard]] Result<DecodedYyddd, Error> DecodeYyddd(std::int32_t yyddd);
[[nodiscard]] double NormalizeUtSeconds(double ut_seconds);

}  // namespace hwm14::detail
