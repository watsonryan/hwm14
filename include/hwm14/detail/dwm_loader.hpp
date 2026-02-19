#pragma once

// Author: watsonryan
// Purpose: Loader for dwm07b104i.dat disturbance wind coefficients.

#include <cstdint>
#include <filesystem>
#include <vector>

#include "hwm14/error.hpp"
#include "hwm14/result.hpp"

namespace hwm14::detail {

struct DwmData {
  std::int32_t nterm{};
  std::int32_t mmax{};
  std::int32_t nmax{};
  std::vector<std::int32_t> termarr_flat{};  // flattened (3 * nterm)
  std::vector<float> coeff{};                // length nterm
  float twidth{};
};

[[nodiscard]] Result<DwmData, Error> LoadDwmData(const std::filesystem::path& path);

}  // namespace hwm14::detail
