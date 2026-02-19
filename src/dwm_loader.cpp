/**
 * @file dwm_loader.cpp
 * @brief Implementation of `dwm07b104i.dat` loader/parser.
 */

#include "hwm14/detail/dwm_loader.hpp"

#include <cstring>
#include <fstream>
#include <vector>

#include "hwm14/detail/fortran_unformatted.hpp"

namespace hwm14::detail {

Result<DwmData, Error> LoadDwmData(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileOpenFailed, "failed to open dwm07b104i.dat", path.string(), "LoadDwmData"));
  }

  std::vector<char> rec;
  if (!ReadFortranRecordRaw(in, rec)) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "failed reading DWM header", path.string(), "LoadDwmData"));
  }

  if (rec.size() != 3 * sizeof(std::int32_t)) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "unexpected DWM header size", path.string(), "LoadDwmData"));
  }

  DwmData out;
  const auto* p = rec.data();
  std::memcpy(&out.nterm, p, sizeof(out.nterm));
  p += sizeof(out.nterm);
  std::memcpy(&out.mmax, p, sizeof(out.mmax));
  p += sizeof(out.mmax);
  std::memcpy(&out.nmax, p, sizeof(out.nmax));

  if (out.nterm <= 0 || out.nterm > 500000 || out.mmax < 0 || out.nmax < 0) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "invalid DWM dimensions", path.string(), "LoadDwmData"));
  }

  if (!ReadFortranRecordRaw(in, rec)) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "failed reading DWM termarr", path.string(), "LoadDwmData"));
  }

  std::vector<std::int32_t> termarr;
  if (!UnpackRecordArray<std::int32_t>(rec, termarr)) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "invalid DWM termarr payload", path.string(), "LoadDwmData"));
  }

  if (termarr.size() != static_cast<std::size_t>(out.nterm) * 3U) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "DWM termarr count mismatch", path.string(), "LoadDwmData"));
  }

  if (!ReadFortranRecordRaw(in, rec)) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "failed reading DWM coefficients", path.string(), "LoadDwmData"));
  }

  std::vector<float> coeff;
  if (!UnpackRecordArray<float>(rec, coeff)) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "invalid DWM coefficient payload", path.string(), "LoadDwmData"));
  }

  if (coeff.size() != static_cast<std::size_t>(out.nterm)) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "DWM coefficient count mismatch", path.string(), "LoadDwmData"));
  }

  if (!ReadFortranRecordRaw(in, rec)) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "failed reading DWM transition width", path.string(), "LoadDwmData"));
  }

  if (!UnpackRecordScalar<float>(rec, out.twidth)) {
    return Result<DwmData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "invalid DWM transition width payload", path.string(), "LoadDwmData"));
  }

  out.termarr_flat = std::move(termarr);
  out.coeff = std::move(coeff);
  return Result<DwmData, Error>::Ok(std::move(out));
}

}  // namespace hwm14::detail
