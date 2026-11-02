#pragma once

#include <filesystem>
#include <memory>

#include "ml/lane_classifier.hpp"

namespace conductor::ml {

// Model contract (see models/README.md):
//   input  "text"   string tensor [1]
//   output "scores" float tensor  [1, 3]  in core::Lane order (research, coding, general)
// ONNX Runtime headers stay out of this header so nothing outside src/ml depends on them.
class OnnxLaneClassifier : public LaneClassifier {
 public:
  explicit OnnxLaneClassifier(const std::filesystem::path& model_path);
  ~OnnxLaneClassifier() override;
  LanePrediction classify(std::string_view brief) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace conductor::ml
