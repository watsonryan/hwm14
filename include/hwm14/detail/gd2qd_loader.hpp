#pragma once

// Author: watsonryan
// Purpose: Loader for gd2qd.dat spherical harmonic coefficients.

#include <cstdint>
#include <filesystem>
#include <vector>

#include "hwm14/error.hpp"
#include "hwm14/result.hpp"

namespace hwm14::detail {

struct Gd2qdData {
  std::int32_t nmax{};
  std::int32_t mmax{};
  std::int32_t nterm{};
  float epoch{};
  float alt{};
  std::vector<double> coeff_flat{};  // flattened (nterm * 3)
};

[[nodiscard]] Result<Gd2qdData, Error> LoadGd2qdData(const std::filesystem::path& path);

}  // namespace hwm14::detail
