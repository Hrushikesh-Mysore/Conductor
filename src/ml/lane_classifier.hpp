#pragma once

#include <string_view>

#include "core/task.hpp"

namespace conductor::ml {

struct LanePrediction {
  core::Lane lane;
  float confidence;  // 0..1
};

// The only ML surface the rest of Conductor sees. The deterministic router stays the
// safety net: callers treat a prediction as a suggestion and fall back to rules or Inbox.
class LaneClassifier {
 public:
  virtual ~LaneClassifier() = default;
  virtual LanePrediction classify(std::string_view brief) = 0;
};

}  // namespace conductor::ml
