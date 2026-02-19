// Author: watsonryan
// Purpose: Error formatting and construction helpers.

#include "hwm14/error.hpp"

#include <sstream>
#include <utility>

namespace hwm14 {

std::string_view ToString(ErrorCode code) {
  switch (code) {
    case ErrorCode::kNone:
      return "none";
    case ErrorCode::kInvalidInput:
      return "invalid_input";
    case ErrorCode::kDataPathNotFound:
      return "data_path_not_found";
    case ErrorCode::kDataFileOpenFailed:
      return "data_file_open_failed";
    case ErrorCode::kDataFileParseFailed:
      return "data_file_parse_failed";
    case ErrorCode::kNotImplemented:
      return "not_implemented";
  }
  return "unknown";
}

std::string FormatError(const Error& error) {
  std::ostringstream out;
  out << "code=" << ToString(error.code) << " message=\"" << error.message << "\"";
  if (!error.detail.empty()) out << " detail=\"" << error.detail << "\"";
  if (!error.location.empty()) out << " location=\"" << error.location << "\"";
  return out.str();
}

Error MakeError(ErrorCode code, std::string message, std::string detail, std::string location) {
  return Error{code, std::move(message), std::move(detail), std::move(location)};
}

}  // namespace hwm14
