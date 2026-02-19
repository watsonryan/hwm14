// Author: watsonryan
// Purpose: Validate generated golden CSV datasets for expected shape and scenarios.

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<std::string> SplitCsvLine(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : line) {
    if (c == ',') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

}  // namespace

int main() {
  const auto root = std::filesystem::path(HWM14_SOURCE_DIR) / "testdata";

  std::ifstream p(root / "golden_profiles.csv");
  std::ifstream d(root / "golden_dwm07b.csv");
  if (!p || !d) {
    return EXIT_FAILURE;
  }

  std::string line;
  if (!std::getline(p, line) || !std::getline(d, line)) {
    return EXIT_FAILURE;
  }

  int profile_rows = 0;
  int dwm_rows = 0;
  std::set<std::string> profile_scenarios;
  std::set<std::string> dwm_scenarios;

  while (std::getline(p, line)) {
    if (line.empty()) {
      continue;
    }
    const auto cols = SplitCsvLine(line);
    if (cols.size() != 9) {
      return EXIT_FAILURE;
    }
    profile_scenarios.insert(cols[0]);
    ++profile_rows;
  }

  while (std::getline(d, line)) {
    if (line.empty()) {
      continue;
    }
    const auto cols = SplitCsvLine(line);
    if (cols.size() != 5) {
      return EXIT_FAILURE;
    }
    dwm_scenarios.insert(cols[0]);
    ++dwm_rows;
  }

  if (profile_rows != 105 || dwm_rows != 55) {
    return EXIT_FAILURE;
  }
  if (profile_scenarios.size() != 6 || dwm_scenarios.size() != 3) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
