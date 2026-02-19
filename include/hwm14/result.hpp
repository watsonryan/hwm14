/**
 * @file result.hpp
 * @brief Minimal value-or-error result container used by HWM14 APIs.
 */
#pragma once

// Author: watsonryan
// Purpose: Lightweight C++20 result container for value-or-error APIs.

#include <cassert>
#include <utility>
#include <variant>

namespace hwm14 {

template <typename T, typename E>
class Result {
 public:
  /** @brief Construct a successful result. */
  static Result Ok(T value) { return Result(std::move(value)); }
  /** @brief Construct a failed result. */
  static Result Err(E error) { return Result(std::move(error)); }

  /** @brief True when this result contains a value. */
  [[nodiscard]] bool has_value() const { return std::holds_alternative<T>(state_); }
  /** @brief Bool conversion that mirrors `has_value()`. */
  [[nodiscard]] explicit operator bool() const { return has_value(); }

  /** @brief Access successful value; asserts in debug on error state. */
  [[nodiscard]] const T& value() const {
    assert(has_value());
    return std::get<T>(state_);
  }
  /** @brief Mutable access to successful value; asserts in debug on error state. */
  [[nodiscard]] T& value() {
    assert(has_value());
    return std::get<T>(state_);
  }

  /** @brief Access error payload; asserts in debug on success state. */
  [[nodiscard]] const E& error() const {
    assert(!has_value());
    return std::get<E>(state_);
  }

 private:
  explicit Result(T value) : state_(std::move(value)) {}
  explicit Result(E error) : state_(std::move(error)) {}

  std::variant<T, E> state_;
};

}  // namespace hwm14
