/**
 * @file hwm_bin_loader.cpp
 * @brief Implementation of quiet-time HWM14 binary header/data loader.
 */

#include "hwm14/detail/hwm_bin_loader.hpp"

#include <fstream>
#include <vector>

namespace hwm14::detail {

namespace {

template <typename T>
bool ReadValue(std::ifstream& in, T& out) {
  in.read(reinterpret_cast<char*>(&out), static_cast<std::streamsize>(sizeof(T)));
  return static_cast<bool>(in);
}

template <typename T>
bool ReadArray(std::ifstream& in, T* out, std::size_t n) {
  in.read(reinterpret_cast<char*>(out), static_cast<std::streamsize>(n * sizeof(T)));
  return static_cast<bool>(in);
}

}  // namespace

Result<HwmBinHeader, Error> LoadHwmBinHeader(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    return Result<HwmBinHeader, Error>::Err(
        MakeError(ErrorCode::kDataFileOpenFailed, "failed to open hwm .bin file", path.string(), "LoadHwmBinHeader"));
  }

  HwmBinHeader h;
  if (!ReadValue(in, h.nbf) || !ReadValue(in, h.maxs) || !ReadValue(in, h.maxm) || !ReadValue(in, h.maxl) ||
      !ReadValue(in, h.maxn) || !ReadValue(in, h.ncomp) || !ReadValue(in, h.nlev) || !ReadValue(in, h.p)) {
    return Result<HwmBinHeader, Error>::Err(MakeError(
        ErrorCode::kDataFileParseFailed, "failed reading fixed-size header", path.string(), "LoadHwmBinHeader"));
  }

  h.nnode = h.nlev + h.p;
  if (h.nnode < 0 || h.nnode > 10000) {
    return Result<HwmBinHeader, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "invalid nnode derived from header", path.string(), "LoadHwmBinHeader"));
  }

  h.vnode.resize(static_cast<std::size_t>(h.nnode + 1));
  if (!ReadArray(in, h.vnode.data(), h.vnode.size())) {
    return Result<HwmBinHeader, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "failed reading vnode array", path.string(), "LoadHwmBinHeader"));
  }
  if (h.vnode.size() > 3) {
    h.vnode[3] = 0.0;  // Fortran parity adjustment in initqwm.
  }

  h.nb.assign(static_cast<std::size_t>(h.nnode + 1), 0);
  h.order.assign(static_cast<std::size_t>(h.ncomp) * static_cast<std::size_t>(h.nnode + 1), 0);
  h.mparm.assign(static_cast<std::size_t>(h.nbf) * static_cast<std::size_t>(h.nlev + 1), 0.0);

  const std::int32_t last_level = h.nlev - h.p - 1;
  if (last_level < 0) {
    return Result<HwmBinHeader, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "invalid level span from nlev/p", path.string(), "LoadHwmBinHeader"));
  }

  std::vector<std::int32_t> order_row(static_cast<std::size_t>(h.ncomp));
  for (std::int32_t i = 0; i <= last_level; ++i) {
    if (!ReadArray(in, order_row.data(), order_row.size())) {
      return Result<HwmBinHeader, Error>::Err(MakeError(
          ErrorCode::kDataFileParseFailed, "failed reading order row", path.string(), "LoadHwmBinHeader"));
    }
    for (std::int32_t c = 0; c < h.ncomp; ++c) {
      h.order[static_cast<std::size_t>(c) + static_cast<std::size_t>(h.ncomp) * static_cast<std::size_t>(i)] =
          order_row[static_cast<std::size_t>(c)];
    }

    if (!ReadValue(in, h.nb[static_cast<std::size_t>(i)])) {
      return Result<HwmBinHeader, Error>::Err(
          MakeError(ErrorCode::kDataFileParseFailed, "failed reading nb entry", path.string(), "LoadHwmBinHeader"));
    }

    const std::size_t offset = static_cast<std::size_t>(h.nbf) * static_cast<std::size_t>(i);
    if (!ReadArray(in, h.mparm.data() + offset, static_cast<std::size_t>(h.nbf))) {
      return Result<HwmBinHeader, Error>::Err(MakeError(
          ErrorCode::kDataFileParseFailed, "failed reading mparm column", path.string(), "LoadHwmBinHeader"));
    }
  }

  if (!ReadArray(in, h.e1.data(), h.e1.size()) || !ReadArray(in, h.e2.data(), h.e2.size())) {
    return Result<HwmBinHeader, Error>::Err(MakeError(
        ErrorCode::kDataFileParseFailed, "failed reading transition vectors", path.string(), "LoadHwmBinHeader"));
  }

  return Result<HwmBinHeader, Error>::Ok(std::move(h));
}

}  // namespace hwm14::detail
