// Author: watsonryan
// Purpose: Verify testdata-based path resolution for required HWM14 files.

#include <cstdlib>
#include <filesystem>

#include "hwm14/data_paths.hpp"

int main() {
  hwm14::Options options{};
  options.data_dir = std::filesystem::path(HWM14_SOURCE_DIR) / "testdata";
  const auto paths = hwm14::ResolveDataPaths(options);
  if (!paths) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
