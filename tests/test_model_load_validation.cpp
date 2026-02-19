// Author: watsonryan
// Purpose: Validate model init performs full file parsing, not path-only checks.

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "hwm14/hwm14.hpp"

namespace {

bool CopyFile(const std::filesystem::path& from, const std::filesystem::path& to) {
  std::ifstream in(from, std::ios::binary);
  if (!in) {
    return false;
  }
  std::ofstream out(to, std::ios::binary);
  if (!out) {
    return false;
  }
  out << in.rdbuf();
  return static_cast<bool>(out);
}

}  // namespace

int main() {
  const auto src = std::filesystem::path(HWM14_SOURCE_DIR) / "testdata";
  const auto good = hwm14::Model::LoadFromDirectory(src);
  if (!good) {
    return EXIT_FAILURE;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "hwm14_load_validation";
  std::error_code ec;
  std::filesystem::remove_all(tmp, ec);
  std::filesystem::create_directories(tmp, ec);
  if (ec) {
    return EXIT_FAILURE;
  }

  if (!CopyFile(src / "hwm123114.bin", tmp / "hwm123114.bin") || !CopyFile(src / "dwm07b104i.dat", tmp / "dwm07b104i.dat")) {
    return EXIT_FAILURE;
  }

  {
    std::ofstream bad(tmp / "gd2qd.dat", std::ios::binary | std::ios::trunc);
    if (!bad) {
      return EXIT_FAILURE;
    }
    // Invalid/truncated payload should fail parse, even though file exists.
    bad.write("bad", 3);
  }

  const auto broken = hwm14::Model::LoadFromDirectory(tmp);
  if (broken) {
    return EXIT_FAILURE;
  }
  if (broken.error().code != hwm14::ErrorCode::kDataFileParseFailed) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
