/**
 * @file time_utils.hpp
 * @brief Internal helpers for legacy YYDDD decoding and UTC second normalization.
 */
#pragma once

// Author: watsonryan
// Purpose: Time/date helpers for HWM14 parity input semantics.

#include <cstdint>

#include "hwm14/error.hpp"
#include "hwm14/result.hpp"

namespace hwm14::detail {

/** @brief Decoded YY and day-of-year components from YYDDD input. */
struct DecodedYyddd {
  std::int32_t yy{};
  std::int32_t day_of_year{};
};

/** @brief Decode YYDDD integer into (yy, day_of_year) with validation. */
[[nodiscard]] Result<DecodedYyddd, Error> DecodeYyddd(std::int32_t yyddd);
/** @brief Wrap UTC seconds into [0, 86400) when finite. */
[[nodiscard]] double NormalizeUtSeconds(double ut_seconds);

}  // namespace hwm14::detail
