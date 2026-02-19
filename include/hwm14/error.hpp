/**
 * @file error.hpp
 * @brief Error codes and structured error payloads for HWM14 APIs.
 */
#pragma once

// Author: watsonryan
// Purpose: Error types and formatting utilities for HWM14 APIs.

#include <string>
#include <string_view>

namespace hwm14 {

/** @brief Stable library error categories. */
enum class ErrorCode {
  kNone,
  kInvalidInput,
  kDataPathNotFound,
  kDataFileOpenFailed,
  kDataFileParseFailed,
  kNotImplemented,
};

/** @brief Rich error payload used in `Result<T, Error>`. */
struct Error {
  /** @brief Machine-readable category. */
  ErrorCode code{ErrorCode::kNone};
  /** @brief Human-readable summary. */
  std::string message{};
  /** @brief Optional contextual detail (path/value/etc.). */
  std::string detail{};
  /** @brief Optional function/location breadcrumb. */
  std::string location{};
};

/** @brief Convert error code to stable token string. */
[[nodiscard]] std::string_view ToString(ErrorCode code);
/** @brief Format a structured error as a single log-friendly line. */
[[nodiscard]] std::string FormatError(const Error& error);
/** @brief Construct a structured error with optional detail/location. */
[[nodiscard]] Error MakeError(ErrorCode code,
                              std::string message,
                              std::string detail = {},
                              std::string location = {});

}  // namespace hwm14
