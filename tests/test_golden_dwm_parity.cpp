// Author: watsonryan
// Purpose: Validate magnetic disturbance winds against golden DWM CSV.

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "hwm14/hwm14.hpp"

namespace {

std::vector<std::string> Split(const std::string& s) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
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
  const auto data = std::filesystem::path(HWM14_SOURCE_DIR) / "testdata" / "golden_dwm07b.csv";
  std::ifstream in(data);
  if (!in) {
    return EXIT_FAILURE;
  }

  auto model = hwm14::Model::LoadFromDirectory(std::filesystem::path(HWM14_SOURCE_DIR) / "testdata");
  if (!model) {
    return EXIT_FAILURE;
  }

  std::string line;
  if (!std::getline(in, line)) {
    return EXIT_FAILURE;
  }

  constexpr double kTol = 2e-2;
  int checked = 0;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    const auto c = Split(line);
    if (c.size() != 5) {
      return EXIT_FAILURE;
    }

    const std::string& scenario = c[0];
    const double x = std::stod(c[2]);
    const double em = std::stod(c[3]);
    const double ez = std::stod(c[4]);

    double mlt = 3.0;
    double mlat = -50.0;
    double kp = 6.0;

    if (scenario == "dwm: magnetic latitude profile") {
      mlat = x;
      mlt = 3.0;
      kp = 6.0;
    } else if (scenario == "dwm: magnetic local time profile") {
      mlat = 45.0;
      mlt = x;
      kp = 6.0;
    } else if (scenario == "dwm: kp profile") {
      mlat = -50.0;
      mlt = 3.0;
      kp = x;
    } else {
      return EXIT_FAILURE;
    }

    const auto got = model.value().DisturbanceWindsMag(mlt, mlat, kp);
    if (!got) {
      return EXIT_FAILURE;
    }
    if (std::abs(got.value().meridional_mps - em) > kTol || std::abs(got.value().zonal_mps - ez) > kTol) {
      return EXIT_FAILURE;
    }
    ++checked;
  }

  return checked > 50 ? EXIT_SUCCESS : EXIT_FAILURE;
}
