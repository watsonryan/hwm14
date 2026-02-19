#pragma once

// Author: watsonryan
// Purpose: Public C++20 API for the HWM14 model.

#include <memory>

#include "hwm14/data_paths.hpp"
#include "hwm14/error.hpp"
#include "hwm14/result.hpp"
#include "hwm14/types.hpp"

namespace hwm14 {

class Model {
 public:
  [[nodiscard]] static Result<Model, Error> LoadFromDirectory(std::filesystem::path data_dir,
                                                              Options options = {});
  [[nodiscard]] static Result<Model, Error> LoadWithSearchPaths(Options options = {});

  [[nodiscard]] Result<Winds, Error> TotalWinds(const Inputs& in) const;
  [[nodiscard]] Result<Winds, Error> QuietWinds(const Inputs& in) const;
  [[nodiscard]] Result<Winds, Error> DisturbanceWindsGeo(const Inputs& in) const;
  [[nodiscard]] Result<Winds, Error> DisturbanceWindsMag(double mlt_h, double mlat_deg, double kp) const;

  [[nodiscard]] Result<Winds, Error> Evaluate(const Inputs& in) const;

 private:
  struct Impl;
  [[nodiscard]] static Result<Model, Error> LoadFromResolvedPaths(DataPaths paths, Options options);

  explicit Model(std::shared_ptr<const Impl> impl, Options options) : impl_(std::move(impl)), options_(std::move(options)) {}

  std::shared_ptr<const Impl> impl_{};
  Options options_{};
};

}  // namespace hwm14
