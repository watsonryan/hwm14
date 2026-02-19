// Author: watsonryan
// Purpose: Validate stream-binary metadata loading from hwm123114.bin.

#include <cstdlib>
#include <filesystem>

#include "hwm14/detail/hwm_bin_loader.hpp"

int main() {
  const auto p = std::filesystem::path(HWM14_SOURCE_DIR) / "testdata" / "hwm123114.bin";
  const auto r = hwm14::detail::LoadHwmBinHeader(p);
  if (!r) {
    return EXIT_FAILURE;
  }
  const auto& h = r.value();
  if (h.nbf != 800 || h.maxs != 2 || h.maxm != 4 || h.maxl != 3 || h.maxn != 8 || h.ncomp != 9 || h.nlev != 33 ||
      h.p != 3 || h.nnode != 36) {
    return EXIT_FAILURE;
  }
  if (h.vnode.size() != 37) {
    return EXIT_FAILURE;
  }
  if (h.vnode[4] != 5.0 || h.vnode[5] != 10.0 || h.vnode[6] != 15.0 || h.vnode[7] != 20.0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
