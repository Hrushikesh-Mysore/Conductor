#include "adapters/manual_worker.hpp"

#include <stdexcept>

namespace conductor::adapters {

// Phase 3 (see docs/setup-plan.md). How the prompt reaches the human is still to be decided.
core::RunOutcome ManualWorker::run(const core::Brief&, const std::filesystem::path&) {
  throw std::logic_error("ManualWorker::run: not implemented");
}

}  // namespace conductor::adapters
