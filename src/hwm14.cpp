/**
 * @file hwm14.cpp
 * @brief Core HWM14 model API implementation and evaluator kernels.
 */

#include "hwm14/hwm14.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <numeric>
#include <string_view>
#include <vector>

#include "hwm14/detail/dwm_loader.hpp"
#include "hwm14/detail/gd2qd_loader.hpp"
#include "hwm14/detail/hwm_bin_loader.hpp"
#include "hwm14/detail/time_utils.hpp"

namespace hwm14 {

namespace {

constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kTwoPi = 2.0 * kPi;
constexpr double kDeg2Rad = kTwoPi / 360.0;
constexpr double kDtor = kPi / 180.0;
constexpr double kSineps = 0.39781868;
constexpr double kQwmScaleHeightKm = 60.0;

inline std::size_t Idx2(int n, int m, int mmax) {
  return static_cast<std::size_t>(n) * static_cast<std::size_t>(mmax + 1) + static_cast<std::size_t>(m);
}

inline std::size_t HwmOrderIdx(int c0, int level, int ncomp) {
  return static_cast<std::size_t>(c0) + static_cast<std::size_t>(ncomp) * static_cast<std::size_t>(level);
}

inline double DotN(const double* a, const double* b, int n) {
  double out = 0.0;
  for (int i = 0; i < n; ++i) {
    out += a[i] * b[i];
  }
  return out;
}

inline double Clamp(double x, double lo, double hi) {
  return std::max(lo, std::min(hi, x));
}

struct AlfState {
  int nmax0{0};
  int mmax0{0};
  std::vector<double> anm;
  std::vector<double> bnm;
  std::vector<double> dnm;
  std::vector<double> cm;
  std::vector<double> en;
  std::vector<double> marr;
  std::vector<double> narr;

  [[nodiscard]] double Anm(int n, int m) const { return anm[Idx2(n, m, mmax0)]; }
  [[nodiscard]] double Bnm(int n, int m) const { return bnm[Idx2(n, m, mmax0)]; }
  [[nodiscard]] double Dnm(int n, int m) const { return dnm[Idx2(n, m, mmax0)]; }

  void Init(int nmax, int mmax) {
    nmax0 = nmax;
    mmax0 = mmax;
    anm.assign(static_cast<std::size_t>(nmax0 + 1) * static_cast<std::size_t>(mmax0 + 1), 0.0);
    bnm.assign(static_cast<std::size_t>(nmax0 + 1) * static_cast<std::size_t>(mmax0 + 1), 0.0);
    dnm.assign(static_cast<std::size_t>(nmax0 + 1) * static_cast<std::size_t>(mmax0 + 1), 0.0);
    cm.assign(static_cast<std::size_t>(mmax0 + 1), 0.0);
    en.assign(static_cast<std::size_t>(nmax0 + 1), 0.0);
    marr.assign(static_cast<std::size_t>(mmax0 + 1), 0.0);
    narr.assign(static_cast<std::size_t>(nmax0 + 1), 0.0);

    for (int n = 1; n <= nmax0; ++n) {
      narr[static_cast<std::size_t>(n)] = static_cast<double>(n);
      en[static_cast<std::size_t>(n)] = std::sqrt(static_cast<double>(n * (n + 1)));
      anm[Idx2(n, 0, mmax0)] =
          std::sqrt(static_cast<double>((2 * n - 1) * (2 * n + 1))) / narr[static_cast<std::size_t>(n)];
      bnm[Idx2(n, 0, mmax0)] =
          std::sqrt(static_cast<double>((2 * n + 1) * (n - 1) * (n - 1)) / static_cast<double>(2 * n - 3)) /
          narr[static_cast<std::size_t>(n)];
    }

    for (int m = 1; m <= mmax0; ++m) {
      marr[static_cast<std::size_t>(m)] = static_cast<double>(m);
      cm[static_cast<std::size_t>(m)] = std::sqrt(static_cast<double>(2 * m + 1) / static_cast<double>(2 * m * m * (m + 1)));
      for (int n = m + 1; n <= nmax0; ++n) {
        anm[Idx2(n, m, mmax0)] = std::sqrt(static_cast<double>((2 * n - 1) * (2 * n + 1) * (n - 1)) /
                                           static_cast<double>((n - m) * (n + m) * (n + 1)));
        bnm[Idx2(n, m, mmax0)] =
            std::sqrt(static_cast<double>((2 * n + 1) * (n + m - 1) * (n - m - 1) * (n - 2) * (n - 1)) /
                      static_cast<double>((n - m) * (n + m) * (2 * n - 3) * n * (n + 1)));
        dnm[Idx2(n, m, mmax0)] = std::sqrt(static_cast<double>((n - m) * (n + m) * (2 * n + 1) * (n - 1)) /
                                           static_cast<double>((2 * n - 1) * (n + 1)));
      }
    }
  }

  void Basis(int nmax, int mmax, double theta, std::vector<double>& P, std::vector<double>& V, std::vector<double>& W) const {
    P.assign(static_cast<std::size_t>(nmax + 1) * static_cast<std::size_t>(mmax + 1), 0.0);
    V.assign(static_cast<std::size_t>(nmax + 1) * static_cast<std::size_t>(mmax + 1), 0.0);
    W.assign(static_cast<std::size_t>(nmax + 1) * static_cast<std::size_t>(mmax + 1), 0.0);

    constexpr double p00 = 0.70710678118654746;
    P[Idx2(0, 0, mmax)] = p00;
    const double x = std::cos(theta);
    const double y = std::sin(theta);

    for (int m = 1; m <= mmax; ++m) {
      W[Idx2(m, m, mmax)] = cm[static_cast<std::size_t>(m)] * P[Idx2(m - 1, m - 1, mmax)];
      P[Idx2(m, m, mmax)] = y * en[static_cast<std::size_t>(m)] * W[Idx2(m, m, mmax)];
      for (int n = m + 1; n <= nmax; ++n) {
        W[Idx2(n, m, mmax)] = Anm(n, m) * x * W[Idx2(n - 1, m, mmax)] - Bnm(n, m) * W[Idx2(n - 2, m, mmax)];
        P[Idx2(n, m, mmax)] = y * en[static_cast<std::size_t>(n)] * W[Idx2(n, m, mmax)];
        V[Idx2(n, m, mmax)] = narr[static_cast<std::size_t>(n)] * x * W[Idx2(n, m, mmax)] - Dnm(n, m) * W[Idx2(n - 1, m, mmax)];
        W[Idx2(n - 2, m, mmax)] = marr[static_cast<std::size_t>(m)] * W[Idx2(n - 2, m, mmax)];
      }
      W[Idx2(nmax - 1, m, mmax)] = marr[static_cast<std::size_t>(m)] * W[Idx2(nmax - 1, m, mmax)];
      W[Idx2(nmax, m, mmax)] = marr[static_cast<std::size_t>(m)] * W[Idx2(nmax, m, mmax)];
      V[Idx2(m, m, mmax)] = x * W[Idx2(m, m, mmax)];
    }

    if (nmax >= 1) {
      P[Idx2(1, 0, mmax)] = Anm(1, 0) * x * P[Idx2(0, 0, mmax)];
      if (mmax >= 1) {
        V[Idx2(1, 0, mmax)] = -P[Idx2(1, 1, mmax)];
      }
    }
    for (int n = 2; n <= nmax; ++n) {
      P[Idx2(n, 0, mmax)] = Anm(n, 0) * x * P[Idx2(n - 1, 0, mmax)] - Bnm(n, 0) * P[Idx2(n - 2, 0, mmax)];
      if (mmax >= 1) {
        V[Idx2(n, 0, mmax)] = -P[Idx2(n, 1, mmax)];
      }
    }
  }
};

double Ap2Kp(double ap0) {
  static constexpr std::array<float, 28> apgrid = {0.F,  2.F,  3.F,  4.F,  5.F,  6.F,  7.F,  9.F,  12.F, 15.F,
                                                   18.F, 22.F, 27.F, 32.F, 39.F, 48.F, 56.F, 67.F, 80.F, 94.F,
                                                   111.F, 132.F, 154.F, 179.F, 207.F, 236.F, 300.F, 400.F};
  static constexpr std::array<float, 28> kpgrid = {0.F / 3.F,   1.F / 3.F,   2.F / 3.F,   3.F / 3.F,   4.F / 3.F,
                                                   5.F / 3.F,   6.F / 3.F,   7.F / 3.F,   8.F / 3.F,   9.F / 3.F,
                                                   10.F / 3.F,  11.F / 3.F,  12.F / 3.F,  13.F / 3.F,  14.F / 3.F,
                                                   15.F / 3.F,  16.F / 3.F,  17.F / 3.F,  18.F / 3.F,  19.F / 3.F,
                                                   20.F / 3.F,  21.F / 3.F,  22.F / 3.F,  23.F / 3.F,  24.F / 3.F,
                                                   25.F / 3.F,  26.F / 3.F,  27.F / 3.F};

  double ap = Clamp(ap0, 0.0, 400.0);
  int i = 1;
  while (i < 28 && ap > static_cast<double>(apgrid[static_cast<std::size_t>(i)])) {
    ++i;
  }
  if (i >= 28) {
    return static_cast<double>(kpgrid.back());
  }
  if (ap == static_cast<double>(apgrid[static_cast<std::size_t>(i)])) {
    return static_cast<double>(kpgrid[static_cast<std::size_t>(i)]);
  }
  const double a0 = static_cast<double>(apgrid[static_cast<std::size_t>(i - 1)]);
  const double a1 = static_cast<double>(apgrid[static_cast<std::size_t>(i)]);
  const double k0 = static_cast<double>(kpgrid[static_cast<std::size_t>(i - 1)]);
  return k0 + (ap - a0) / (3.0 * (a1 - a0));
}

void KpSpl3(double kp, std::array<double, 3>& out) {
  static constexpr std::array<double, 8> node = {-10.0, -8.0, 0.0, 2.0, 5.0, 8.0, 18.0, 20.0};
  double x = Clamp(kp, 0.0, 8.0);
  std::array<double, 7> kpspl{};

  for (int i = 0; i <= 6; ++i) {
    if (x >= node[static_cast<std::size_t>(i)] && x < node[static_cast<std::size_t>(i + 1)]) {
      kpspl[static_cast<std::size_t>(i)] = 1.0;
    }
  }
  for (int j = 2; j <= 3; ++j) {
    for (int i = 0; i <= 8 - j - 1; ++i) {
      kpspl[static_cast<std::size_t>(i)] =
          kpspl[static_cast<std::size_t>(i)] * (x - node[static_cast<std::size_t>(i)]) /
              (node[static_cast<std::size_t>(i + j - 1)] - node[static_cast<std::size_t>(i)]) +
          kpspl[static_cast<std::size_t>(i + 1)] * (node[static_cast<std::size_t>(i + j)] - x) /
              (node[static_cast<std::size_t>(i + j)] - node[static_cast<std::size_t>(i + 1)]);
    }
  }

  out[0] = kpspl[0] + kpspl[1];
  out[1] = kpspl[2];
  out[2] = kpspl[3] + kpspl[4];
}

double LatWgt2(double mlat, double mlt, double kp0, double twidth) {
  static constexpr std::array<double, 6> coeff = {65.7633, -4.60256, -3.53915, -1.99971, -0.752193, 0.972388};

  const double mltrad = mlt * 15.0 * kDtor;
  const double sinmlt = std::sin(mltrad);
  const double cosmlt = std::cos(mltrad);
  const double kp = Clamp(kp0, 0.0, 8.0);
  const double tlat = coeff[0] + coeff[1] * cosmlt + coeff[2] * sinmlt +
                      kp * (coeff[3] + coeff[4] * cosmlt + coeff[5] * sinmlt);
  return 1.0 / (1.0 + std::exp(-(std::abs(mlat) - tlat) / twidth));
}

int FindSpan(int n, int p, double u, const std::vector<double>& V) {
  if (u >= V[static_cast<std::size_t>(n + 1)]) {
    return n;
  }
  int low = p;
  int high = n + 1;
  int mid = (low + high) / 2;
  while (u < V[static_cast<std::size_t>(mid)] || u >= V[static_cast<std::size_t>(mid + 1)]) {
    if (u < V[static_cast<std::size_t>(mid)]) {
      high = mid;
    } else {
      low = mid;
    }
    mid = (low + high) / 2;
  }
  return mid;
}

double BSpline(int p, int m, const std::vector<double>& V, int i, double u) {
  if (i == 0 && u == V[0]) {
    return 1.0;
  }
  if (i == (m - p - 1) && u == V[static_cast<std::size_t>(m)]) {
    return 1.0;
  }
  if (u < V[static_cast<std::size_t>(i)] || u >= V[static_cast<std::size_t>(i + p + 1)]) {
    return 0.0;
  }

  std::vector<double> N(static_cast<std::size_t>(p + 2), 0.0);
  for (int j = 0; j <= p; ++j) {
    if (u >= V[static_cast<std::size_t>(i + j)] && u < V[static_cast<std::size_t>(i + j + 1)]) {
      N[static_cast<std::size_t>(j)] = 1.0;
    }
  }

  for (int k = 1; k <= p; ++k) {
    double saved = 0.0;
    if (N[0] != 0.0) {
      saved = ((u - V[static_cast<std::size_t>(i)]) * N[0]) /
              (V[static_cast<std::size_t>(i + k)] - V[static_cast<std::size_t>(i)]);
    }
    for (int j = 0; j <= p - k; ++j) {
      const double vleft = V[static_cast<std::size_t>(i + j + 1)];
      const double vright = V[static_cast<std::size_t>(i + j + k + 1)];
      if (N[static_cast<std::size_t>(j + 1)] == 0.0) {
        N[static_cast<std::size_t>(j)] = saved;
        saved = 0.0;
      } else {
        const double temp = N[static_cast<std::size_t>(j + 1)] / (vright - vleft);
        N[static_cast<std::size_t>(j)] = saved + (vright - u) * temp;
        saved = (u - vleft) * temp;
      }
    }
  }

  return N[0];
}

void VertWght(double alt, const detail::HwmBinHeader& hwm, std::vector<double>& wght, int& iz) {
  const int p = hwm.p;
  const int nnode = hwm.nnode;
  const int nlev = hwm.nlev;
  wght.assign(static_cast<std::size_t>(p + 1), 0.0);

  iz = FindSpan(nnode - p - 1, p, alt, hwm.vnode) - p;
  iz = std::min(iz, 26);

  wght[0] = BSpline(p, nnode, hwm.vnode, iz, alt);
  wght[1] = BSpline(p, nnode, hwm.vnode, iz + 1, alt);
  if (iz <= 25) {
    wght[2] = BSpline(p, nnode, hwm.vnode, iz + 2, alt);
    wght[3] = BSpline(p, nnode, hwm.vnode, iz + 3, alt);
    return;
  }

  std::array<double, 5> we{};
  const double alttns = hwm.vnode[static_cast<std::size_t>(nlev - 2)];
  if (alt > alttns) {
    we[3] = std::exp(-(alt - alttns) / kQwmScaleHeightKm);
    we[4] = 1.0;
  } else {
    we[0] = BSpline(p, nnode, hwm.vnode, iz + 2, alt);
    we[1] = BSpline(p, nnode, hwm.vnode, iz + 3, alt);
    we[2] = BSpline(p, nnode, hwm.vnode, iz + 4, alt);
  }

  wght[2] = std::inner_product(we.begin(), we.end(), hwm.e1.begin(), 0.0);
  wght[3] = std::inner_product(we.begin(), we.end(), hwm.e2.begin(), 0.0);
}

void ParityColumn(const std::array<int, 8>& order,
                  int nb,
                  std::vector<double>& mcol,
                  std::vector<double>& tcol,
                  int nbf) {
  (void)nb;
  tcol.assign(static_cast<std::size_t>(nbf), 0.0);

  const int amaxs = order[0];
  const int amaxn = order[1];
  const int pmaxm = order[2];
  const int pmaxs = order[3];
  const int pmaxn = order[4];
  const int tmaxl = order[5];
  const int tmaxs = order[6];
  const int tmaxn = order[7];

  int c = 1;
  auto M = [&](int idx1) -> double& { return mcol[static_cast<std::size_t>(idx1 - 1)]; };
  auto T = [&](int idx1) -> double& { return tcol[static_cast<std::size_t>(idx1 - 1)]; };

  for (int n = 1; n <= amaxn; ++n) {
    (void)n;
    T(c) = 0.0;
    T(c + 1) = -M(c + 1);
    M(c + 1) = 0.0;
    c += 2;
  }
  for (int s = 1; s <= amaxs; ++s) {
    (void)s;
    for (int n = 1; n <= amaxn; ++n) {
      (void)n;
      T(c) = 0.0;
      T(c + 1) = 0.0;
      T(c + 2) = -M(c + 2);
      T(c + 3) = -M(c + 3);
      M(c + 2) = 0.0;
      M(c + 3) = 0.0;
      c += 4;
    }
  }

  for (int m = 1; m <= pmaxm; ++m) {
    for (int n = m; n <= pmaxn; ++n) {
      (void)n;
      T(c) = M(c + 2);
      T(c + 1) = M(c + 3);
      T(c + 2) = -M(c);
      T(c + 3) = -M(c + 1);
      c += 4;
    }
    for (int s = 1; s <= pmaxs; ++s) {
      (void)s;
      for (int n = m; n <= pmaxn; ++n) {
        (void)n;
        T(c) = M(c + 2);
        T(c + 1) = M(c + 3);
        T(c + 2) = -M(c);
        T(c + 3) = -M(c + 1);
        T(c + 4) = M(c + 6);
        T(c + 5) = M(c + 7);
        T(c + 6) = -M(c + 4);
        T(c + 7) = -M(c + 5);
        c += 8;
      }
    }
  }

  for (int l = 1; l <= tmaxl; ++l) {
    for (int n = l; n <= tmaxn; ++n) {
      (void)n;
      T(c) = M(c + 2);
      T(c + 1) = M(c + 3);
      T(c + 2) = -M(c);
      T(c + 3) = -M(c + 1);
      c += 4;
    }
    for (int s = 1; s <= tmaxs; ++s) {
      (void)s;
      for (int n = l; n <= tmaxn; ++n) {
        (void)n;
        T(c) = M(c + 2);
        T(c + 1) = M(c + 3);
        T(c + 2) = -M(c);
        T(c + 3) = -M(c + 1);
        T(c + 4) = M(c + 6);
        T(c + 5) = M(c + 7);
        T(c + 6) = -M(c + 4);
        T(c + 7) = -M(c + 5);
        c += 8;
      }
    }
  }
}

struct Gd2qdTransform {
  double qlat{};
  double qlon{};
  double f1e{};
  double f1n{};
  double f2e{};
  double f2n{};
};

}  // namespace

struct Model::Impl {
  DataPaths paths{};
  detail::HwmBinHeader hwm{};
  detail::Gd2qdData gd2qd{};
  detail::DwmData dwm{};

  int maxo{};
  int nmaxgeo{};
  int mmaxgeo{};
  int nvshterm{};

  AlfState alf{};

  std::vector<double> xcoeff{};
  std::vector<double> ycoeff{};
  std::vector<double> zcoeff{};
  std::vector<double> normadj{};

  std::vector<double> tparm{};  // [nbf x (nlev+1)]
};

namespace {

Result<Winds, Error> ValidateCommonInputs(const Inputs& in, std::string_view where) {
  const auto decoded = detail::DecodeYyddd(in.yyddd);
  if (!decoded) {
    return Result<Winds, Error>::Err(
        MakeError(decoded.error().code, decoded.error().message, decoded.error().detail, std::string(where)));
  }

  if (!std::isfinite(in.ut_seconds) || !std::isfinite(in.altitude_km) || !std::isfinite(in.geodetic_lat_deg) ||
      !std::isfinite(in.geodetic_lon_deg) || !std::isfinite(in.ap3)) {
    return Result<Winds, Error>::Err(MakeError(ErrorCode::kInvalidInput, "inputs must be finite", {}, std::string(where)));
  }
  if (in.geodetic_lat_deg < -90.0 || in.geodetic_lat_deg > 90.0) {
    return Result<Winds, Error>::Err(
        MakeError(ErrorCode::kInvalidInput, "geodetic_lat_deg must be in [-90, 90]", {}, std::string(where)));
  }
  if (in.altitude_km < 0.0 || in.altitude_km > 5000.0) {
    return Result<Winds, Error>::Err(
        MakeError(ErrorCode::kInvalidInput, "altitude_km must be in [0, 5000]", {}, std::string(where)));
  }
  return Result<Winds, Error>::Ok(Winds{});
}

Result<Winds, Error> QuietWindsImpl(const Model::Impl& impl, const Inputs& in) {
  const auto& h = impl.hwm;

  struct QuietScratch {
    std::vector<double> fs;
    std::vector<double> fm;
    std::vector<double> fl;
    std::vector<double> gpbar;
    std::vector<double> gvbar;
    std::vector<double> gwbar;
    std::vector<double> zwght;
    std::vector<double> bz;
  };
  thread_local QuietScratch scratch;

  scratch.fs.assign(static_cast<std::size_t>(h.maxs + 1) * 2U, 0.0);
  scratch.fm.assign(static_cast<std::size_t>(h.maxm + 1) * 2U, 0.0);
  scratch.fl.assign(static_cast<std::size_t>(h.maxl + 1) * 2U, 0.0);

  const double day = static_cast<double>(in.yyddd % 1000);
  double aa = day * kTwoPi / 365.25;
  for (int s = 0; s <= h.maxs; ++s) {
    const double bb = static_cast<double>(s) * aa;
    scratch.fs[static_cast<std::size_t>(2 * s)] = std::cos(bb);
    scratch.fs[static_cast<std::size_t>(2 * s + 1)] = std::sin(bb);
  }

  const double stl = std::fmod(in.ut_seconds / 3600.0 + in.geodetic_lon_deg / 15.0 + 48.0, 24.0);
  aa = stl * kTwoPi / 24.0;
  for (int l = 0; l <= h.maxl; ++l) {
    const double cc = static_cast<double>(l) * aa;
    scratch.fl[static_cast<std::size_t>(2 * l)] = std::cos(cc);
    scratch.fl[static_cast<std::size_t>(2 * l + 1)] = std::sin(cc);
  }

  aa = in.geodetic_lon_deg * kDeg2Rad;
  for (int m = 0; m <= h.maxm; ++m) {
    const double bb = static_cast<double>(m) * aa;
    scratch.fm[static_cast<std::size_t>(2 * m)] = std::cos(bb);
    scratch.fm[static_cast<std::size_t>(2 * m + 1)] = std::sin(bb);
  }

  const double theta = (90.0 - in.geodetic_lat_deg) * kDeg2Rad;
  impl.alf.Basis(h.maxn, impl.maxo, theta, scratch.gpbar, scratch.gvbar, scratch.gwbar);

  int lev = 0;
  VertWght(in.altitude_km, h, scratch.zwght, lev);

  scratch.bz.assign(static_cast<std::size_t>(h.nbf), 0.0);
  static constexpr std::array<double, 4> wavefactor = {0.0, 1.0, 1.0, 1.0};
  static constexpr std::array<double, 4> tidefactor = {0.0, 1.0, 1.0, 1.0};

  double u = 0.0;
  double v = 0.0;

  for (int b = 0; b <= h.p; ++b) {
    if (scratch.zwght[static_cast<std::size_t>(b)] == 0.0) {
      continue;
    }

    const int d = b + lev;
    int c = 1;

    const int amaxs = h.order[HwmOrderIdx(0, d, h.ncomp)];
    const int amaxn = h.order[HwmOrderIdx(1, d, h.ncomp)];
    const int pmaxm = h.order[HwmOrderIdx(2, d, h.ncomp)];
    const int pmaxs = h.order[HwmOrderIdx(3, d, h.ncomp)];
    const int pmaxn = h.order[HwmOrderIdx(4, d, h.ncomp)];
    const int tmaxl = h.order[HwmOrderIdx(5, d, h.ncomp)];
    const int tmaxs = h.order[HwmOrderIdx(6, d, h.ncomp)];
    const int tmaxn = h.order[HwmOrderIdx(7, d, h.ncomp)];

    for (int n = 1; n <= amaxn; ++n) {
      const double sc = std::sin(static_cast<double>(n) * theta);
      scratch.bz[static_cast<std::size_t>(c - 1)] = -sc;
      scratch.bz[static_cast<std::size_t>(c)] = sc;
      c += 2;
    }
    for (int s = 1; s <= amaxs; ++s) {
      const double cs = scratch.fs[static_cast<std::size_t>(2 * s)];
      const double ss = scratch.fs[static_cast<std::size_t>(2 * s + 1)];
      for (int n = 1; n <= amaxn; ++n) {
        const double sc = std::sin(static_cast<double>(n) * theta);
        scratch.bz[static_cast<std::size_t>(c - 1)] = -sc * cs;
        scratch.bz[static_cast<std::size_t>(c)] = sc * ss;
        scratch.bz[static_cast<std::size_t>(c + 1)] = sc * cs;
        scratch.bz[static_cast<std::size_t>(c + 2)] = -sc * ss;
        c += 4;
      }
    }

    for (int m = 1; m <= pmaxm; ++m) {
      const double cm = scratch.fm[static_cast<std::size_t>(2 * m)] * wavefactor[static_cast<std::size_t>(m)];
      const double sm = scratch.fm[static_cast<std::size_t>(2 * m + 1)] * wavefactor[static_cast<std::size_t>(m)];
      for (int n = m; n <= pmaxn; ++n) {
        const double vb = scratch.gvbar[Idx2(n, m, impl.maxo)];
        const double wb = scratch.gwbar[Idx2(n, m, impl.maxo)];
        scratch.bz[static_cast<std::size_t>(c - 1)] = -vb * cm;
        scratch.bz[static_cast<std::size_t>(c)] = vb * sm;
        scratch.bz[static_cast<std::size_t>(c + 1)] = -wb * sm;
        scratch.bz[static_cast<std::size_t>(c + 2)] = -wb * cm;
        c += 4;
      }
      for (int s = 1; s <= pmaxs; ++s) {
        const double cs = scratch.fs[static_cast<std::size_t>(2 * s)];
        const double ss = scratch.fs[static_cast<std::size_t>(2 * s + 1)];
        for (int n = m; n <= pmaxn; ++n) {
          const double vb = scratch.gvbar[Idx2(n, m, impl.maxo)];
          const double wb = scratch.gwbar[Idx2(n, m, impl.maxo)];
          scratch.bz[static_cast<std::size_t>(c - 1)] = -vb * cm * cs;
          scratch.bz[static_cast<std::size_t>(c)] = vb * sm * cs;
          scratch.bz[static_cast<std::size_t>(c + 1)] = -wb * sm * cs;
          scratch.bz[static_cast<std::size_t>(c + 2)] = -wb * cm * cs;
          scratch.bz[static_cast<std::size_t>(c + 3)] = -vb * cm * ss;
          scratch.bz[static_cast<std::size_t>(c + 4)] = vb * sm * ss;
          scratch.bz[static_cast<std::size_t>(c + 5)] = -wb * sm * ss;
          scratch.bz[static_cast<std::size_t>(c + 6)] = -wb * cm * ss;
          c += 8;
        }
      }
    }

    for (int l = 1; l <= tmaxl; ++l) {
      const double cl = scratch.fl[static_cast<std::size_t>(2 * l)] * tidefactor[static_cast<std::size_t>(l)];
      const double sl = scratch.fl[static_cast<std::size_t>(2 * l + 1)] * tidefactor[static_cast<std::size_t>(l)];
      for (int n = l; n <= tmaxn; ++n) {
        const double vb = scratch.gvbar[Idx2(n, l, impl.maxo)];
        const double wb = scratch.gwbar[Idx2(n, l, impl.maxo)];
        scratch.bz[static_cast<std::size_t>(c - 1)] = -vb * cl;
        scratch.bz[static_cast<std::size_t>(c)] = vb * sl;
        scratch.bz[static_cast<std::size_t>(c + 1)] = -wb * sl;
        scratch.bz[static_cast<std::size_t>(c + 2)] = -wb * cl;
        c += 4;
      }
      for (int s = 1; s <= tmaxs; ++s) {
        const double cs = scratch.fs[static_cast<std::size_t>(2 * s)];
        const double ss = scratch.fs[static_cast<std::size_t>(2 * s + 1)];
        for (int n = l; n <= tmaxn; ++n) {
          const double vb = scratch.gvbar[Idx2(n, l, impl.maxo)];
          const double wb = scratch.gwbar[Idx2(n, l, impl.maxo)];
          scratch.bz[static_cast<std::size_t>(c - 1)] = -vb * cl * cs;
          scratch.bz[static_cast<std::size_t>(c)] = vb * sl * cs;
          scratch.bz[static_cast<std::size_t>(c + 1)] = -wb * sl * cs;
          scratch.bz[static_cast<std::size_t>(c + 2)] = -wb * cl * cs;
          scratch.bz[static_cast<std::size_t>(c + 3)] = -vb * cl * ss;
          scratch.bz[static_cast<std::size_t>(c + 4)] = vb * sl * ss;
          scratch.bz[static_cast<std::size_t>(c + 5)] = -wb * sl * ss;
          scratch.bz[static_cast<std::size_t>(c + 6)] = -wb * cl * ss;
          c += 8;
        }
      }
    }

    c -= 1;
    const double* mcol = impl.hwm.mparm.data() + static_cast<std::size_t>(h.nbf) * static_cast<std::size_t>(d);
    const double* tcol = impl.tparm.data() + static_cast<std::size_t>(h.nbf) * static_cast<std::size_t>(d);
    u += scratch.zwght[static_cast<std::size_t>(b)] * DotN(scratch.bz.data(), mcol, c);
    v += scratch.zwght[static_cast<std::size_t>(b)] * DotN(scratch.bz.data(), tcol, c);
  }

  Winds w{};
  w.meridional_mps = v;
  w.zonal_mps = u;
  return Result<Winds, Error>::Ok(w);
}

Result<Gd2qdTransform, Error> Gd2qdImpl(const Model::Impl& impl, double glat_in, double glon) {
  struct Gd2qdScratch {
    std::vector<double> gpbar;
    std::vector<double> gvbar;
    std::vector<double> gwbar;
    std::vector<double> sh;
    std::vector<double> shgradtheta;
    std::vector<double> shgradphi;
  };
  thread_local Gd2qdScratch scratch;

  const double theta = (90.0 - glat_in) * kDtor;
  impl.alf.Basis(impl.gd2qd.nmax, impl.gd2qd.mmax, theta, scratch.gpbar, scratch.gvbar, scratch.gwbar);

  const double phi = glon * kDtor;
  scratch.sh.assign(static_cast<std::size_t>(impl.gd2qd.nterm), 0.0);
  scratch.shgradtheta.assign(static_cast<std::size_t>(impl.gd2qd.nterm), 0.0);
  scratch.shgradphi.assign(static_cast<std::size_t>(impl.gd2qd.nterm), 0.0);

  int i = 0;
  for (int n = 0; n <= impl.gd2qd.nmax; ++n) {
    scratch.sh[static_cast<std::size_t>(i)] = scratch.gpbar[Idx2(n, 0, impl.gd2qd.mmax)];
    scratch.shgradtheta[static_cast<std::size_t>(i)] =
        scratch.gvbar[Idx2(n, 0, impl.gd2qd.mmax)] * impl.normadj[static_cast<std::size_t>(n)];
    scratch.shgradphi[static_cast<std::size_t>(i)] = 0.0;
    ++i;
  }
  for (int m = 1; m <= impl.gd2qd.mmax; ++m) {
    const double mphi = static_cast<double>(m) * phi;
    const double cosmphi = std::cos(mphi);
    const double sinmphi = std::sin(mphi);
    for (int n = m; n <= impl.gd2qd.nmax; ++n) {
      scratch.sh[static_cast<std::size_t>(i)] = scratch.gpbar[Idx2(n, m, impl.gd2qd.mmax)] * cosmphi;
      scratch.sh[static_cast<std::size_t>(i + 1)] = scratch.gpbar[Idx2(n, m, impl.gd2qd.mmax)] * sinmphi;
      scratch.shgradtheta[static_cast<std::size_t>(i)] =
          scratch.gvbar[Idx2(n, m, impl.gd2qd.mmax)] * impl.normadj[static_cast<std::size_t>(n)] * cosmphi;
      scratch.shgradtheta[static_cast<std::size_t>(i + 1)] =
          scratch.gvbar[Idx2(n, m, impl.gd2qd.mmax)] * impl.normadj[static_cast<std::size_t>(n)] * sinmphi;
      scratch.shgradphi[static_cast<std::size_t>(i)] =
          -scratch.gwbar[Idx2(n, m, impl.gd2qd.mmax)] * impl.normadj[static_cast<std::size_t>(n)] * sinmphi;
      scratch.shgradphi[static_cast<std::size_t>(i + 1)] =
          scratch.gwbar[Idx2(n, m, impl.gd2qd.mmax)] * impl.normadj[static_cast<std::size_t>(n)] * cosmphi;
      i += 2;
    }
  }

  const double x = std::inner_product(scratch.sh.begin(), scratch.sh.end(), impl.xcoeff.begin(), 0.0);
  const double y = std::inner_product(scratch.sh.begin(), scratch.sh.end(), impl.ycoeff.begin(), 0.0);
  const double z = std::inner_product(scratch.sh.begin(), scratch.sh.end(), impl.zcoeff.begin(), 0.0);

  const double qlonrad = std::atan2(y, x);
  const double cosqlon = std::cos(qlonrad);
  const double sinqlon = std::sin(qlonrad);
  const double cosqlat = x * cosqlon + y * sinqlon;

  const double qlat = std::atan2(z, cosqlat) / kDtor;
  const double qlon = qlonrad / kDtor;

  const double xgradtheta =
      std::inner_product(scratch.shgradtheta.begin(), scratch.shgradtheta.end(), impl.xcoeff.begin(), 0.0);
  const double ygradtheta =
      std::inner_product(scratch.shgradtheta.begin(), scratch.shgradtheta.end(), impl.ycoeff.begin(), 0.0);
  const double zgradtheta =
      std::inner_product(scratch.shgradtheta.begin(), scratch.shgradtheta.end(), impl.zcoeff.begin(), 0.0);

  const double xgradphi =
      std::inner_product(scratch.shgradphi.begin(), scratch.shgradphi.end(), impl.xcoeff.begin(), 0.0);
  const double ygradphi =
      std::inner_product(scratch.shgradphi.begin(), scratch.shgradphi.end(), impl.ycoeff.begin(), 0.0);
  const double zgradphi =
      std::inner_product(scratch.shgradphi.begin(), scratch.shgradphi.end(), impl.zcoeff.begin(), 0.0);

  Gd2qdTransform out{};
  out.qlat = qlat;
  out.qlon = qlon;
  out.f1e = -zgradtheta * cosqlat + (xgradtheta * cosqlon + ygradtheta * sinqlon) * z;
  out.f1n = -zgradphi * cosqlat + (xgradphi * cosqlon + ygradphi * sinqlon) * z;
  out.f2e = ygradtheta * cosqlon - xgradtheta * sinqlon;
  out.f2n = ygradphi * cosqlon - xgradphi * sinqlon;
  return Result<Gd2qdTransform, Error>::Ok(out);
}

double MltCalcImpl(const Model::Impl& impl, double qlat, double qlon, double day, double ut) {
  (void)qlat;
  const double asunglat = -std::asin(std::sin((day + ut / 24.0 - 80.0) * kDtor) * kSineps) / kDtor;
  const double asunglon = -ut * 15.0;

  struct MltScratch {
    std::vector<double> spbar;
    std::vector<double> svbar;
    std::vector<double> swbar;
    std::vector<double> sh;
  };
  thread_local MltScratch scratch;

  const double theta = (90.0 - asunglat) * kDtor;
  impl.alf.Basis(impl.gd2qd.nmax, impl.gd2qd.mmax, theta, scratch.spbar, scratch.svbar, scratch.swbar);

  const double phi = asunglon * kDtor;
  scratch.sh.assign(static_cast<std::size_t>(impl.gd2qd.nterm), 0.0);
  int i = 0;
  for (int n = 0; n <= impl.gd2qd.nmax; ++n) {
    scratch.sh[static_cast<std::size_t>(i)] = scratch.spbar[Idx2(n, 0, impl.gd2qd.mmax)];
    ++i;
  }
  for (int m = 1; m <= impl.gd2qd.mmax; ++m) {
    const double mphi = static_cast<double>(m) * phi;
    const double cosmphi = std::cos(mphi);
    const double sinmphi = std::sin(mphi);
    for (int n = m; n <= impl.gd2qd.nmax; ++n) {
      scratch.sh[static_cast<std::size_t>(i)] = scratch.spbar[Idx2(n, m, impl.gd2qd.mmax)] * cosmphi;
      scratch.sh[static_cast<std::size_t>(i + 1)] = scratch.spbar[Idx2(n, m, impl.gd2qd.mmax)] * sinmphi;
      i += 2;
    }
  }

  const double x = std::inner_product(scratch.sh.begin(), scratch.sh.end(), impl.xcoeff.begin(), 0.0);
  const double y = std::inner_product(scratch.sh.begin(), scratch.sh.end(), impl.ycoeff.begin(), 0.0);
  const double asunqlon = std::atan2(y, x) / kDtor;

  return (qlon - asunqlon) / 15.0;
}

Result<Winds, Error> DisturbanceWindsMagImpl(const Model::Impl& impl, double mlt_h, double mlat_deg, double kp) {
  struct DwmScratch {
    std::vector<double> dpbar;
    std::vector<double> dvbar;
    std::vector<double> dwbar;
    std::vector<std::array<double, 2>> mltterms;
    std::vector<std::array<double, 2>> vshterms;
  };
  thread_local DwmScratch scratch;

  const double theta = (90.0 - mlat_deg) * kDtor;
  impl.alf.Basis(impl.dwm.nmax, impl.dwm.mmax, theta, scratch.dpbar, scratch.dvbar, scratch.dwbar);

  scratch.mltterms.assign(static_cast<std::size_t>(impl.dwm.mmax + 1), {0.0, 0.0});
  const double phi = mlt_h * kDtor * 15.0;
  for (int m = 0; m <= impl.dwm.mmax; ++m) {
    const double mphi = static_cast<double>(m) * phi;
    scratch.mltterms[static_cast<std::size_t>(m)][0] = std::cos(mphi);
    scratch.mltterms[static_cast<std::size_t>(m)][1] = std::sin(mphi);
  }

  scratch.vshterms.assign(static_cast<std::size_t>(impl.nvshterm), {0.0, 0.0});
  int ivshterm = 0;
  for (int n = 1; n <= impl.dwm.nmax; ++n) {
    scratch.vshterms[static_cast<std::size_t>(ivshterm)][0] =
        -scratch.dvbar[Idx2(n, 0, impl.dwm.mmax)] * scratch.mltterms[0][0];
    scratch.vshterms[static_cast<std::size_t>(ivshterm + 1)][0] =
        scratch.dwbar[Idx2(n, 0, impl.dwm.mmax)] * scratch.mltterms[0][0];
    scratch.vshterms[static_cast<std::size_t>(ivshterm)][1] =
        -scratch.vshterms[static_cast<std::size_t>(ivshterm + 1)][0];
    scratch.vshterms[static_cast<std::size_t>(ivshterm + 1)][1] =
        scratch.vshterms[static_cast<std::size_t>(ivshterm)][0];
    ivshterm += 2;

    for (int m = 1; m <= impl.dwm.mmax; ++m) {
      if (m > n) {
        continue;
      }
      scratch.vshterms[static_cast<std::size_t>(ivshterm)][0] =
          -scratch.dvbar[Idx2(n, m, impl.dwm.mmax)] * scratch.mltterms[static_cast<std::size_t>(m)][0];
      scratch.vshterms[static_cast<std::size_t>(ivshterm + 1)][0] =
          scratch.dvbar[Idx2(n, m, impl.dwm.mmax)] * scratch.mltterms[static_cast<std::size_t>(m)][1];
      scratch.vshterms[static_cast<std::size_t>(ivshterm + 2)][0] =
          scratch.dwbar[Idx2(n, m, impl.dwm.mmax)] * scratch.mltterms[static_cast<std::size_t>(m)][1];
      scratch.vshterms[static_cast<std::size_t>(ivshterm + 3)][0] =
          scratch.dwbar[Idx2(n, m, impl.dwm.mmax)] * scratch.mltterms[static_cast<std::size_t>(m)][0];
      scratch.vshterms[static_cast<std::size_t>(ivshterm)][1] =
          -scratch.vshterms[static_cast<std::size_t>(ivshterm + 2)][0];
      scratch.vshterms[static_cast<std::size_t>(ivshterm + 1)][1] =
          -scratch.vshterms[static_cast<std::size_t>(ivshterm + 3)][0];
      scratch.vshterms[static_cast<std::size_t>(ivshterm + 2)][1] =
          scratch.vshterms[static_cast<std::size_t>(ivshterm)][0];
      scratch.vshterms[static_cast<std::size_t>(ivshterm + 3)][1] =
          scratch.vshterms[static_cast<std::size_t>(ivshterm + 1)][0];
      ivshterm += 4;
    }
  }

  std::array<double, 3> kpterms{};
  KpSpl3(kp, kpterms);
  const double latwgtterm = LatWgt2(mlat_deg, mlt_h, kp, impl.dwm.twidth);

  double mmpwind = 0.0;
  double mzpwind = 0.0;
  for (int iterm = 0; iterm < impl.dwm.nterm; ++iterm) {
    double term0 = 1.0;
    double term1 = 1.0;

    const int t0 = impl.dwm.termarr_flat[static_cast<std::size_t>(3 * iterm)];
    const int t1 = impl.dwm.termarr_flat[static_cast<std::size_t>(3 * iterm + 1)];
    const int t2 = impl.dwm.termarr_flat[static_cast<std::size_t>(3 * iterm + 2)];

    if (t0 != 999) {
      term0 *= scratch.vshterms[static_cast<std::size_t>(t0)][0];
      term1 *= scratch.vshterms[static_cast<std::size_t>(t0)][1];
    }
    if (t1 != 999) {
      term0 *= kpterms[static_cast<std::size_t>(t1)];
      term1 *= kpterms[static_cast<std::size_t>(t1)];
    }
    if (t2 != 999) {
      term0 *= latwgtterm;
      term1 *= latwgtterm;
    }

    const double c = impl.dwm.coeff[static_cast<std::size_t>(iterm)];
    mmpwind += c * term0;
    mzpwind += c * term1;
  }

  Winds out{};
  out.meridional_mps = mmpwind;
  out.zonal_mps = mzpwind;
  return Result<Winds, Error>::Ok(out);
}

}  // namespace

Result<Model, Error> Model::LoadFromResolvedPaths(DataPaths paths, Options options) {
  auto hwm = detail::LoadHwmBinHeader(paths.hwm_bin);
  if (!hwm) {
    return Result<Model, Error>::Err(hwm.error());
  }

  auto gd2qd = detail::LoadGd2qdData(paths.gd2qd_dat);
  if (!gd2qd) {
    return Result<Model, Error>::Err(gd2qd.error());
  }

  auto dwm = detail::LoadDwmData(paths.dwm_dat);
  if (!dwm) {
    return Result<Model, Error>::Err(dwm.error());
  }

  auto impl = std::make_shared<Model::Impl>();
  impl->paths = std::move(paths);
  impl->hwm = std::move(hwm.value());
  impl->gd2qd = std::move(gd2qd.value());
  impl->dwm = std::move(dwm.value());

  impl->maxo = std::max({impl->hwm.maxs, impl->hwm.maxm, impl->hwm.maxl});
  impl->nmaxgeo = std::max(impl->hwm.maxn, impl->gd2qd.nmax);
  impl->mmaxgeo = std::max(impl->maxo, impl->gd2qd.mmax);

  const int nmax0 = std::max(impl->nmaxgeo, impl->dwm.nmax);
  const int mmax0 = std::max(impl->mmaxgeo, impl->dwm.mmax);
  impl->alf.Init(nmax0, mmax0);

  impl->xcoeff.assign(static_cast<std::size_t>(impl->gd2qd.nterm), 0.0);
  impl->ycoeff.assign(static_cast<std::size_t>(impl->gd2qd.nterm), 0.0);
  impl->zcoeff.assign(static_cast<std::size_t>(impl->gd2qd.nterm), 0.0);
  for (int iterm = 0; iterm < impl->gd2qd.nterm; ++iterm) {
    impl->xcoeff[static_cast<std::size_t>(iterm)] =
        impl->gd2qd.coeff_flat[static_cast<std::size_t>(iterm) + static_cast<std::size_t>(impl->gd2qd.nterm) * 0U];
    impl->ycoeff[static_cast<std::size_t>(iterm)] =
        impl->gd2qd.coeff_flat[static_cast<std::size_t>(iterm) + static_cast<std::size_t>(impl->gd2qd.nterm) * 1U];
    impl->zcoeff[static_cast<std::size_t>(iterm)] =
        impl->gd2qd.coeff_flat[static_cast<std::size_t>(iterm) + static_cast<std::size_t>(impl->gd2qd.nterm) * 2U];
  }

  impl->normadj.assign(static_cast<std::size_t>(impl->gd2qd.nmax + 1), 0.0);
  for (int n = 0; n <= impl->gd2qd.nmax; ++n) {
    impl->normadj[static_cast<std::size_t>(n)] = std::sqrt(static_cast<double>(n * (n + 1)));
  }

  impl->tparm.assign(impl->hwm.mparm.size(), 0.0);
  const int last_level = impl->hwm.nlev - impl->hwm.p - 1;
  for (int i = 0; i <= last_level; ++i) {
    std::array<int, 8> order{};
    for (int k = 0; k < 8; ++k) {
      order[static_cast<std::size_t>(k)] = impl->hwm.order[HwmOrderIdx(k, i, impl->hwm.ncomp)];
    }

    std::vector<double> mcol(static_cast<std::size_t>(impl->hwm.nbf), 0.0);
    std::vector<double> tcol(static_cast<std::size_t>(impl->hwm.nbf), 0.0);
    const std::size_t off = static_cast<std::size_t>(impl->hwm.nbf) * static_cast<std::size_t>(i);
    std::copy_n(impl->hwm.mparm.begin() + static_cast<std::ptrdiff_t>(off), impl->hwm.nbf, mcol.begin());

    ParityColumn(order, impl->hwm.nb[static_cast<std::size_t>(i)], mcol, tcol, impl->hwm.nbf);

    std::copy(mcol.begin(), mcol.end(), impl->hwm.mparm.begin() + static_cast<std::ptrdiff_t>(off));
    std::copy(tcol.begin(), tcol.end(), impl->tparm.begin() + static_cast<std::ptrdiff_t>(off));
  }

  impl->nvshterm =
      ((((impl->dwm.nmax + 1) * (impl->dwm.nmax + 2) - (impl->dwm.nmax - impl->dwm.mmax) * (impl->dwm.nmax - impl->dwm.mmax + 1)) /
        2) -
       1) *
          4 -
      2 * impl->dwm.nmax;

  return Result<Model, Error>::Ok(Model(std::move(impl), std::move(options)));
}

Result<Model, Error> Model::LoadFromDirectory(std::filesystem::path data_dir, Options options) {
  options.data_dir = std::move(data_dir);
  auto paths = ResolveDataPathsFromDirectory(options.data_dir);
  if (!paths) {
    return Result<Model, Error>::Err(paths.error());
  }
  return LoadFromResolvedPaths(std::move(paths.value()), std::move(options));
}

Result<Model, Error> Model::LoadWithSearchPaths(Options options) {
  auto paths = ResolveDataPathsWithSearchPaths(options);
  if (!paths) {
    return Result<Model, Error>::Err(paths.error());
  }
  return LoadFromResolvedPaths(std::move(paths.value()), std::move(options));
}

Result<Winds, Error> Model::TotalWinds(const Inputs& in) const {
  const auto valid = ValidateCommonInputs(in, "Model::TotalWinds");
  if (!valid) {
    return Result<Winds, Error>::Err(valid.error());
  }

  auto q = QuietWinds(in);
  if (!q) {
    return q;
  }
  if (in.ap3 < 0.0) {
    return q;
  }

  auto d = DisturbanceWindsGeo(in);
  if (!d) {
    return d;
  }

  Winds out{};
  out.meridional_mps = q.value().meridional_mps + d.value().meridional_mps;
  out.zonal_mps = q.value().zonal_mps + d.value().zonal_mps;
  return Result<Winds, Error>::Ok(out);
}

Result<Winds, Error> Model::QuietWinds(const Inputs& in) const {
  const auto valid = ValidateCommonInputs(in, "Model::QuietWinds");
  if (!valid) {
    return Result<Winds, Error>::Err(valid.error());
  }
  return QuietWindsImpl(*impl_, in);
}

Result<Winds, Error> Model::DisturbanceWindsGeo(const Inputs& in) const {
  const auto valid = ValidateCommonInputs(in, "Model::DisturbanceWindsGeo");
  if (!valid) {
    return Result<Winds, Error>::Err(valid.error());
  }
  if (in.ap3 < 0.0) {
    return Result<Winds, Error>::Ok(Winds{});
  }

  const auto tr = Gd2qdImpl(*impl_, in.geodetic_lat_deg, in.geodetic_lon_deg);
  if (!tr) {
    return Result<Winds, Error>::Err(tr.error());
  }

  const double day = static_cast<double>(in.yyddd % 1000);
  const double ut = detail::NormalizeUtSeconds(in.ut_seconds) / 3600.0;
  const double kp = Ap2Kp(in.ap3);
  const double mlt = MltCalcImpl(*impl_, tr.value().qlat, tr.value().qlon, day, ut);

  auto mag = DisturbanceWindsMag(mlt, tr.value().qlat, kp);
  if (!mag) {
    return mag;
  }

  Winds dw{};
  dw.meridional_mps = tr.value().f2n * mag.value().meridional_mps + tr.value().f1n * mag.value().zonal_mps;
  dw.zonal_mps = tr.value().f2e * mag.value().meridional_mps + tr.value().f1e * mag.value().zonal_mps;

  const double height_scale = 1.0 + std::exp(-(in.altitude_km - 125.0) / impl_->dwm.twidth);
  dw.meridional_mps /= height_scale;
  dw.zonal_mps /= height_scale;

  return Result<Winds, Error>::Ok(dw);
}

Result<Winds, Error> Model::DisturbanceWindsMag(double mlt_h, double mlat_deg, double kp) const {
  if (!std::isfinite(mlt_h) || !std::isfinite(mlat_deg) || !std::isfinite(kp)) {
    return Result<Winds, Error>::Err(
        MakeError(ErrorCode::kInvalidInput, "inputs must be finite", {}, "Model::DisturbanceWindsMag"));
  }
  return DisturbanceWindsMagImpl(*impl_, mlt_h, mlat_deg, kp);
}

Result<Winds, Error> Model::Evaluate(const Inputs& in) const {
  return TotalWinds(in);
}

}  // namespace hwm14
