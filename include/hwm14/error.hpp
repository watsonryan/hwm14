#pragma once

// Author: watsonryan
// Purpose: Error types and formatting utilities for HWM14 APIs.

#include <string>
#include <string_view>

namespace hwm14 {

enum class ErrorCode {
  kNone,
  kInvalidInput,
  kDataPathNotFound,
  kDataFileOpenFailed,
  kDataFileParseFailed,
  kNotImplemented,
};

struct Error {
  ErrorCode code{ErrorCode::kNone};
  std::string message{};
  std::string detail{};
  std::string location{};
};

[[nodiscard]] std::string_view ToString(ErrorCode code);
[[nodiscard]] std::string FormatError(const Error& error);
[[nodiscard]] Error MakeError(ErrorCode code,
                              std::string message,
                              std::string detail = {},
                              std::string location = {});

}  // namespace hwm14
