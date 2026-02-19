/**
 * @file hwm_bin_loader.hpp
 * @brief Internal loader for HWM14 quiet-time binary coefficient headers.
 */
#pragma once

// Author: watsonryan
// Purpose: Stream-binary loader for HWM14 quiet-time coefficient metadata.

#include <cstdint>
#include <filesystem>
#include <array>
#include <vector>

#include "hwm14/error.hpp"
#include "hwm14/result.hpp"

namespace hwm14::detail {

/** @brief Parsed header and arrays from `hwm123114.bin`. */
struct HwmBinHeader {
  std::int32_t nbf{};
  std::int32_t maxs{};
  std::int32_t maxm{};
  std::int32_t maxl{};
  std::int32_t maxn{};
  std::int32_t ncomp{};
  std::int32_t nlev{};
  std::int32_t p{};
  std::int32_t nnode{};
  std::vector<double> vnode{};
  std::vector<std::int32_t> nb{};      // size nnode + 1
  std::vector<std::int32_t> order{};   // column-major [ncomp x (nnode + 1)]
  std::vector<double> mparm{};         // column-major [nbf x (nlev + 1)]
  std::array<double, 5> e1{};
  std::array<double, 5> e2{};
};

/** @brief Load and parse the HWM14 binary header and associated arrays. */
[[nodiscard]] Result<HwmBinHeader, Error> LoadHwmBinHeader(const std::filesystem::path& path);

}  // namespace hwm14::detail
