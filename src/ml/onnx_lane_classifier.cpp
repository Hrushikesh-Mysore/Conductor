#include "ml/onnx_lane_classifier.hpp"

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace conductor::ml {

namespace {
constexpr std::size_t kLaneCount = 3;
const char* kInputNames[] = {"text"};
const char* kOutputNames[] = {"scores"};
}  // namespace

struct OnnxLaneClassifier::Impl {
  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "conductor"};
  Ort::Session session;

  explicit Impl(const std::filesystem::path& model_path)
      : session(env, model_path.c_str(), Ort::SessionOptions{}) {}
};

OnnxLaneClassifier::OnnxLaneClassifier(const std::filesystem::path& model_path) {
  if (!std::filesystem::is_regular_file(model_path))
    throw std::runtime_error("model not found: " + model_path.string());
  impl_ = std::make_unique<Impl>(model_path);
}

OnnxLaneClassifier::~OnnxLaneClassifier() = default;

LanePrediction OnnxLaneClassifier::classify(std::string_view brief) {
  Ort::AllocatorWithDefaultOptions alloc;
  const int64_t shape[] = {1};
  Ort::Value input = Ort::Value::CreateTensor(alloc, shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING);
  const std::string text(brief);
  const char* strings[] = {text.c_str()};
  input.FillStringTensor(strings, 1);

  auto outputs = impl_->session.Run(Ort::RunOptions{nullptr}, kInputNames, &input, 1, kOutputNames, 1);

  // The model file is external input: check the output shape before reading it.
  auto& scores = outputs.at(0);
  if (scores.GetTensorTypeAndShapeInfo().GetElementCount() != kLaneCount)
    throw std::runtime_error("model output 'scores' must have exactly 3 values");
  const float* s = scores.GetTensorData<float>();
  auto best = std::max_element(s, s + kLaneCount) - s;
  return {static_cast<core::Lane>(best), s[best]};
}

}  // namespace conductor::ml
