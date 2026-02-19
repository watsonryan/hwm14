// Author: watsonryan
// Purpose: Smoke tests for DWM coefficient data parsing.

#include <cmath>
#include <filesystem>

#include "hwm14/detail/dwm_loader.hpp"

int main() {
  const auto path = std::filesystem::path(HWM14_SOURCE_DIR) / "testdata" / "dwm07b104i.dat";
  const auto d = hwm14::detail::LoadDwmData(path);
  if (!d) {
    return 1;
  }

  if (d.value().nterm != 300 || d.value().mmax != 3 || d.value().nmax != 10) {
    return 2;
  }
  if (d.value().termarr_flat.size() != static_cast<std::size_t>(d.value().nterm) * 3U) {
    return 3;
  }
  if (d.value().coeff.size() != static_cast<std::size_t>(d.value().nterm)) {
    return 4;
  }
  if (!std::isfinite(d.value().twidth) || d.value().twidth <= 0.0f) {
    return 5;
  }

  return 0;
}
