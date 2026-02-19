/**
 * @file gd2qd_loader.hpp
 * @brief Internal loader for `gd2qd.dat` spherical harmonic metadata and coefficients.
 */
#pragma once

// Author: watsonryan
// Purpose: Loader for gd2qd.dat spherical harmonic coefficients.

#include <cstdint>
#include <filesystem>
#include <vector>

#include "hwm14/error.hpp"
#include "hwm14/result.hpp"

namespace hwm14::detail {

/** @brief Parsed contents of `gd2qd.dat` required for geographic/quasi-dipole transforms. */
struct Gd2qdData {
  std::int32_t nmax{};
  std::int32_t mmax{};
  std::int32_t nterm{};
  float epoch{};
  float alt{};
  std::vector<double> coeff_flat{};  // flattened (nterm * 3)
};

/** @brief Load and parse `gd2qd.dat` Fortran-unformatted records. */
[[nodiscard]] Result<Gd2qdData, Error> LoadGd2qdData(const std::filesystem::path& path);

}  // namespace hwm14::detail
