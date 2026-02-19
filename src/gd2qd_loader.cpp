// Author: watsonryan
// Purpose: Fortran-unformatted parser for gd2qd.dat.

#include "hwm14/detail/gd2qd_loader.hpp"

#include <cstring>
#include <fstream>
#include <vector>

#include "hwm14/detail/fortran_unformatted.hpp"

namespace hwm14::detail {

Result<Gd2qdData, Error> LoadGd2qdData(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    return Result<Gd2qdData, Error>::Err(
        MakeError(ErrorCode::kDataFileOpenFailed, "failed to open gd2qd.dat", path.string(), "LoadGd2qdData"));
  }

  std::vector<char> rec;
  if (!ReadFortranRecordRaw(in, rec)) {
    return Result<Gd2qdData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "failed reading gd2qd header record", path.string(), "LoadGd2qdData"));
  }

  if (rec.size() != (3 * sizeof(std::int32_t) + 2 * sizeof(float))) {
    return Result<Gd2qdData, Error>::Err(MakeError(ErrorCode::kDataFileParseFailed,
                                                    "unexpected gd2qd header size",
                                                    path.string(),
                                                    "LoadGd2qdData"));
  }

  Gd2qdData out;
  const auto* p = rec.data();
  std::memcpy(&out.nmax, p, sizeof(out.nmax));
  p += sizeof(out.nmax);
  std::memcpy(&out.mmax, p, sizeof(out.mmax));
  p += sizeof(out.mmax);
  std::memcpy(&out.nterm, p, sizeof(out.nterm));
  p += sizeof(out.nterm);
  std::memcpy(&out.epoch, p, sizeof(out.epoch));
  p += sizeof(out.epoch);
  std::memcpy(&out.alt, p, sizeof(out.alt));

  if (out.nmax < 0 || out.mmax < 0 || out.nterm <= 0 || out.nterm > 200000) {
    return Result<Gd2qdData, Error>::Err(MakeError(ErrorCode::kDataFileParseFailed,
                                                    "invalid gd2qd dimensions",
                                                    path.string(),
                                                    "LoadGd2qdData"));
  }

  if (!ReadFortranRecordRaw(in, rec)) {
    return Result<Gd2qdData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "failed reading gd2qd coeff record", path.string(), "LoadGd2qdData"));
  }

  std::vector<double> coeff;
  if (!UnpackRecordArray<double>(rec, coeff)) {
    return Result<Gd2qdData, Error>::Err(
        MakeError(ErrorCode::kDataFileParseFailed, "invalid gd2qd coeff payload", path.string(), "LoadGd2qdData"));
  }

  const std::size_t expected = static_cast<std::size_t>(out.nterm) * 3U;
  if (coeff.size() != expected) {
    return Result<Gd2qdData, Error>::Err(MakeError(ErrorCode::kDataFileParseFailed,
                                                    "gd2qd coeff count mismatch",
                                                    path.string(),
                                                    "LoadGd2qdData"));
  }

  out.coeff_flat = std::move(coeff);
  return Result<Gd2qdData, Error>::Ok(std::move(out));
}

}  // namespace hwm14::detail
