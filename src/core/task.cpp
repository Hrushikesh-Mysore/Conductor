#include "core/task.hpp"

#include <array>
#include <utility>

namespace conductor::core {

namespace {
constexpr std::array<std::string_view, 3> kLaneNames{"research", "coding", "general"};
constexpr std::array<std::string_view, 7> kStateNames{
    "inbox", "triaged", "assigned", "running", "review", "done", "rejected"};

using S = TaskState;
constexpr std::array<std::pair<S, S>, 11> kTransitions{{
    {S::Inbox, S::Triaged},
    {S::Inbox, S::Rejected},
    {S::Triaged, S::Assigned},
    {S::Triaged, S::Rejected},
    {S::Assigned, S::Running},
    {S::Assigned, S::Triaged},   // reassign: back to the router
    {S::Running, S::Review},
    {S::Running, S::Assigned},   // worker paused or failed: retry
    {S::Review, S::Done},
    {S::Review, S::Rejected},
    {S::Review, S::Assigned},    // changes requested
}};
}  // namespace

bool can_transition(TaskState from, TaskState to) {
  for (auto [f, t] : kTransitions)
    if (f == from && t == to) return true;
  return false;
}

std::string_view to_string(Lane lane) { return kLaneNames[static_cast<std::size_t>(lane)]; }
std::string_view to_string(TaskState state) { return kStateNames[static_cast<std::size_t>(state)]; }

std::optional<Lane> lane_from_string(std::string_view s) {
  for (std::size_t i = 0; i < kLaneNames.size(); ++i)
    if (kLaneNames[i] == s) return static_cast<Lane>(i);
  return std::nullopt;
}

std::optional<TaskState> state_from_string(std::string_view s) {
  for (std::size_t i = 0; i < kStateNames.size(); ++i)
    if (kStateNames[i] == s) return static_cast<TaskState>(i);
  return std::nullopt;
}

}  // namespace conductor::core
