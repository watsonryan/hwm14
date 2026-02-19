// Author: watsonryan
// Purpose: Smoke tests for gd2qd.dat loader parity-critical metadata.

#include <filesystem>

#include "hwm14/detail/gd2qd_loader.hpp"

int main() {
  const auto path = std::filesystem::path(HWM14_SOURCE_DIR) / "testdata" / "gd2qd.dat";
  const auto d = hwm14::detail::LoadGd2qdData(path);
  if (!d) {
    return 1;
  }

  if (d.value().nmax != 8 || d.value().mmax != 4 || d.value().nterm != 61) {
    return 2;
  }

  if (d.value().coeff_flat.size() != static_cast<std::size_t>(d.value().nterm) * 3U) {
    return 3;
  }

  return 0;
}
