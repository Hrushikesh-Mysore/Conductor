#include "planner/planner_api.hpp"

#include <stdexcept>

namespace conductor::planner {

// Phase 2 (see docs/setup-plan.md). Build on store::Store, core::assign and core::can_transition.

std::int64_t PlannerApi::create_task(const std::string&, core::Lane, const core::Brief&) {
  throw std::logic_error("PlannerApi::create_task: not implemented");
}

std::vector<core::Task> PlannerApi::list_tasks(std::optional<core::TaskState>) {
  throw std::logic_error("PlannerApi::list_tasks: not implemented");
}

std::optional<std::string> PlannerApi::assign(std::int64_t, const std::optional<std::string>&) {
  throw std::logic_error("PlannerApi::assign: not implemented");
}

std::optional<std::filesystem::path> PlannerApi::get_result(std::int64_t) {
  throw std::logic_error("PlannerApi::get_result: not implemented");
}

void PlannerApi::review(std::int64_t, bool, const std::string&) {
  throw std::logic_error("PlannerApi::review: not implemented");
}

}  // namespace conductor::planner
