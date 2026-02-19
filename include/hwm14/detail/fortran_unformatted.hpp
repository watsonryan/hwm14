#pragma once

// Author: watsonryan
// Purpose: Helpers for reading Fortran sequential-unformatted records.

#include <cstdint>
#include <cstring>
#include <fstream>
#include <type_traits>
#include <vector>

namespace hwm14::detail {

inline bool ReadFortranRecordRaw(std::ifstream& in, std::vector<char>& payload) {
  std::int32_t n1 = 0;
  in.read(reinterpret_cast<char*>(&n1), static_cast<std::streamsize>(sizeof(n1)));
  if (!in) {
    return false;
  }
  if (n1 < 0) {
    return false;
  }

  payload.resize(static_cast<std::size_t>(n1));
  if (n1 > 0) {
    in.read(payload.data(), static_cast<std::streamsize>(payload.size()));
    if (!in) {
      return false;
    }
  }

  std::int32_t n2 = 0;
  in.read(reinterpret_cast<char*>(&n2), static_cast<std::streamsize>(sizeof(n2)));
  if (!in || n2 != n1) {
    return false;
  }
  return true;
}

template <typename T>
inline bool UnpackRecordScalar(const std::vector<char>& payload, T& out) {
  static_assert(std::is_trivially_copyable_v<T>);
  if (payload.size() != sizeof(T)) {
    return false;
  }
  std::memcpy(&out, payload.data(), sizeof(T));
  return true;
}

template <typename T>
inline bool UnpackRecordArray(const std::vector<char>& payload, std::vector<T>& out) {
  static_assert(std::is_trivially_copyable_v<T>);
  if (payload.size() % sizeof(T) != 0) {
    return false;
  }
  const auto n = payload.size() / sizeof(T);
  out.resize(n);
  if (n > 0) {
    std::memcpy(out.data(), payload.data(), payload.size());
  }
  return true;
}

}  // namespace hwm14::detail
