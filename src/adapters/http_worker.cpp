#include "adapters/http_worker.hpp"

#include <stdexcept>

namespace conductor::adapters {

// Phase 3 (see docs/setup-plan.md). Needs an HTTP client and JSON; pick them when implementing.
core::RunOutcome HttpWorker::run(const core::Brief&, const std::filesystem::path&) {
  throw std::logic_error("HttpWorker::run: not implemented (" + endpoint_.base_url + ")");
}

}  // namespace conductor::adapters
