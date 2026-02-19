/**
 * @file hwm14.hpp
 * @brief Public C++20 API for loading and evaluating HWM14 winds.
 */
#pragma once

// Author: watsonryan
// Purpose: Public C++20 API for the HWM14 model.

#include <memory>

#include "hwm14/data_paths.hpp"
#include "hwm14/error.hpp"
#include "hwm14/result.hpp"
#include "hwm14/types.hpp"

namespace hwm14 {

/**
 * @brief Immutable HWM14 model instance.
 *
 * Create via one of the static load factories, then evaluate winds for inputs.
 */
class Model {
 public:
  struct Impl;

  /**
   * @brief Load model data from an explicit directory.
   * @param data_dir Directory containing required coefficient/data files.
   * @param options Runtime options for strictness/cache/path policy.
   * @return Loaded model or an error with code/message/detail.
   */
  [[nodiscard]] static Result<Model, Error> LoadFromDirectory(std::filesystem::path data_dir,
                                                              Options options = {});
  /**
   * @brief Load model data using search path policy.
   * @param options Runtime options controlling search behavior.
   * @return Loaded model or an error with code/message/detail.
   */
  [[nodiscard]] static Result<Model, Error> LoadWithSearchPaths(Options options = {});

  /** @brief Evaluate total (quiet + disturbance) winds in m/s. */
  [[nodiscard]] Result<Winds, Error> TotalWinds(const Inputs& in) const;
  /** @brief Evaluate quiet-time HWM14 winds in m/s. */
  [[nodiscard]] Result<Winds, Error> QuietWinds(const Inputs& in) const;
  /** @brief Evaluate disturbance winds in geographic coordinates in m/s. */
  [[nodiscard]] Result<Winds, Error> DisturbanceWindsGeo(const Inputs& in) const;
  /** @brief Evaluate disturbance winds in magnetic coordinates in m/s. */
  [[nodiscard]] Result<Winds, Error> DisturbanceWindsMag(double mlt_h, double mlat_deg, double kp) const;

  /** @brief Alias of TotalWinds for API ergonomics. */
  [[nodiscard]] Result<Winds, Error> Evaluate(const Inputs& in) const;

 private:
  [[nodiscard]] static Result<Model, Error> LoadFromResolvedPaths(DataPaths paths, Options options);

  explicit Model(std::shared_ptr<const Impl> impl, Options options) : impl_(std::move(impl)), options_(std::move(options)) {}

  std::shared_ptr<const Impl> impl_{};
  Options options_{};
};

}  // namespace hwm14
