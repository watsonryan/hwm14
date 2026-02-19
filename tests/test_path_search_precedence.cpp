// Author: watsonryan
// Purpose: Validate legacy search-path precedence and explicit-dir behavior.

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "hwm14/data_paths.hpp"
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

bool CopyDataset(const std::filesystem::path& src, const std::filesystem::path& dst) {
  std::error_code ec;
  std::filesystem::create_directories(dst, ec);
  if (ec) {
    return false;
  }
  return CopyFile(src / "hwm123114.bin", dst / "hwm123114.bin") && CopyFile(src / "dwm07b104i.dat", dst / "dwm07b104i.dat") &&
         CopyFile(src / "gd2qd.dat", dst / "gd2qd.dat");
}

void SetEnvVar(const char* name, const std::string& value) {
#if defined(_WIN32)
  _putenv_s(name, value.c_str());
#else
  setenv(name, value.c_str(), 1);
#endif
}

void UnsetEnvVar(const char* name) {
#if defined(_WIN32)
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}

}  // namespace

int main() {
  const auto src = std::filesystem::path(HWM14_SOURCE_DIR) / "testdata";
  const auto root = std::filesystem::temp_directory_path() / "hwm14_search_precedence";
  const auto work = root / "work";
  const auto meta = root / "Meta";
  const auto env = root / "env";

  std::error_code ec;
  std::filesystem::remove_all(root, ec);
  std::filesystem::create_directories(work, ec);
  if (ec) {
    return 10;
  }

  if (!CopyDataset(src, meta) || !CopyDataset(src, env)) {
    return 11;
  }

  const auto old_cwd = std::filesystem::current_path(ec);
  if (ec) {
    return 12;
  }
  const char* old_hwmpath = std::getenv("HWMPATH");
  const std::string old_hwmpath_value = old_hwmpath ? old_hwmpath : "";

  std::filesystem::current_path(work, ec);
  if (ec) {
    return 13;
  }
  SetEnvVar("HWMPATH", env.string());

  hwm14::Options opt{};
  opt.allow_env_hwmpath = true;
  const auto p1 = hwm14::ResolveDataPathsWithSearchPaths(opt);
  if (!p1) {
    return 14;
  }
  ec.clear();
  const auto p1_parent = std::filesystem::weakly_canonical(p1.value().hwm_bin.parent_path(), ec);
  if (ec) {
    return 14;
  }
  ec.clear();
  if (p1_parent != std::filesystem::weakly_canonical(meta, ec) || ec) {
    return 14;
  }

  std::filesystem::remove(meta / "hwm123114.bin", ec);
  if (ec) {
    return 15;
  }
  const auto p2 = hwm14::ResolveDataPathsWithSearchPaths(opt);
  if (!p2) {
    return 16;
  }
  ec.clear();
  const auto p2_parent = std::filesystem::weakly_canonical(p2.value().hwm_bin.parent_path(), ec);
  if (ec) {
    return 16;
  }
  ec.clear();
  if (p2_parent != std::filesystem::weakly_canonical(env, ec) || ec) {
    return 16;
  }

  const auto strict = hwm14::Model::LoadFromDirectory(std::filesystem::path{});
  if (strict || strict.error().code != hwm14::ErrorCode::kDataPathNotFound) {
    return 17;
  }

  std::filesystem::current_path(old_cwd, ec);
  if (!old_hwmpath_value.empty()) {
    SetEnvVar("HWMPATH", old_hwmpath_value);
  } else {
    UnsetEnvVar("HWMPATH");
  }
  return ec ? 18 : EXIT_SUCCESS;
}
