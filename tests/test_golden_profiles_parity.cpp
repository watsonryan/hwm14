// Author: watsonryan
// Purpose: Validate total/quiet/disturbance winds against golden profile CSV.

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "hwm14/hwm14.hpp"

namespace {

struct Row {
  std::string scenario;
  std::string x_name;
  double x{};
  double qmer{};
  double qzon{};
  double dmer{};
  double dzon{};
  double tmer{};
  double tzon{};
};

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

bool BuildInputs(const Row& r, hwm14::Inputs& in) {
  if (r.scenario == "height profile") {
    in.yyddd = 95150;
    in.ut_seconds = 12.0 * 3600.0;
    in.altitude_km = r.x;
    in.geodetic_lat_deg = -45.0;
    in.geodetic_lon_deg = -85.0;
    in.ap3 = 80.0;
    return true;
  }
  if (r.scenario == "latitude profile") {
    in.yyddd = 95305;
    in.ut_seconds = 18.0 * 3600.0;
    in.altitude_km = 250.0;
    in.geodetic_lat_deg = r.x;
    in.geodetic_lon_deg = 30.0;
    in.ap3 = 48.0;
    return true;
  }
  if (r.scenario == "local time profile") {
    in.yyddd = 95075;
    const double glon = -70.0;
    const double uth = std::fmod(r.x - glon / 15.0 + 24.0, 24.0);
    in.ut_seconds = uth * 3600.0;
    in.altitude_km = 125.0;
    in.geodetic_lat_deg = 45.0;
    in.geodetic_lon_deg = glon;
    in.ap3 = 30.0;
    return true;
  }
  if (r.scenario == "longitude profile") {
    in.yyddd = 95330;
    in.ut_seconds = 6.0 * 3600.0;
    in.altitude_km = 40.0;
    in.geodetic_lat_deg = -5.0;
    in.geodetic_lon_deg = r.x;
    in.ap3 = 4.0;
    return true;
  }
  if (r.scenario == "day of year profile") {
    in.yyddd = 95000 + static_cast<int>(std::llround(r.x));
    in.ut_seconds = 21.0 * 3600.0;
    in.altitude_km = 200.0;
    in.geodetic_lat_deg = -65.0;
    in.geodetic_lon_deg = -135.0;
    in.ap3 = 15.0;
    return true;
  }
  if (r.scenario == "magnetic activity profile") {
    in.yyddd = 95280;
    in.ut_seconds = 21.0 * 3600.0;
    in.altitude_km = 350.0;
    in.geodetic_lat_deg = 38.0;
    in.geodetic_lon_deg = 125.0;
    in.ap3 = r.x;
    return true;
  }
  return false;
}

}  // namespace

int main() {
  const auto data = std::filesystem::path(HWM14_SOURCE_DIR) / "testdata" / "golden_profiles.csv";
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

  constexpr double kTol = 1.2e-1;
  int checked = 0;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    const auto c = Split(line);
    if (c.size() != 9) {
      return EXIT_FAILURE;
    }

    Row r{};
    r.scenario = c[0];
    r.x_name = c[1];
    r.x = std::stod(c[2]);
    r.qmer = std::stod(c[3]);
    r.qzon = std::stod(c[4]);
    r.dmer = std::stod(c[5]);
    r.dzon = std::stod(c[6]);
    r.tmer = std::stod(c[7]);
    r.tzon = std::stod(c[8]);

    hwm14::Inputs req{};
    if (!BuildInputs(r, req)) {
      return EXIT_FAILURE;
    }

    const auto q = model.value().QuietWinds(req);
    const auto d = model.value().DisturbanceWindsGeo(req);
    const auto t = model.value().TotalWinds(req);
    if (!q || !d || !t) {
      return EXIT_FAILURE;
    }

    if (std::abs(q.value().meridional_mps - r.qmer) > kTol || std::abs(q.value().zonal_mps - r.qzon) > kTol ||
        std::abs(d.value().meridional_mps - r.dmer) > kTol || std::abs(d.value().zonal_mps - r.dzon) > kTol ||
        std::abs(t.value().meridional_mps - r.tmer) > kTol || std::abs(t.value().zonal_mps - r.tzon) > kTol) {
      return EXIT_FAILURE;
    }
    ++checked;
  }

  return checked > 100 ? EXIT_SUCCESS : EXIT_FAILURE;
}
