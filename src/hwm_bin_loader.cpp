// Author: watsonryan
// Purpose: Stream-binary parser for HWM14 .bin coefficient metadata.

#include "hwm14/detail/hwm_bin_loader.hpp"

#include <fstream>

namespace hwm14::detail {

namespace {

template <typename T>
bool ReadValue(std::ifstream& in, T& out) {
  in.read(reinterpret_cast<char*>(&out), static_cast<std::streamsize>(sizeof(T)));
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
  for (auto& v : h.vnode) {
    if (!ReadValue(in, v)) {
      return Result<HwmBinHeader, Error>::Err(
          MakeError(ErrorCode::kDataFileParseFailed, "failed reading vnode array", path.string(), "LoadHwmBinHeader"));
    }
  }

  return Result<HwmBinHeader, Error>::Ok(std::move(h));
}

}  // namespace hwm14::detail
