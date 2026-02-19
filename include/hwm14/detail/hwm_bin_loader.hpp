#pragma once

// Author: watsonryan
// Purpose: Stream-binary loader for HWM14 quiet-time coefficient metadata.

#include <cstdint>
#include <filesystem>
#include <vector>

#include "hwm14/error.hpp"
#include "hwm14/result.hpp"

namespace hwm14::detail {

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
};

[[nodiscard]] Result<HwmBinHeader, Error> LoadHwmBinHeader(const std::filesystem::path& path);

}  // namespace hwm14::detail
