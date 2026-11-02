#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace conductor::core {

enum class Lane { Research, Coding, General };

// Inbox -> Triaged -> Assigned -> Running -> Review -> Done | Rejected
enum class TaskState { Inbox, Triaged, Assigned, Running, Review, Done, Rejected };

struct Task {
  std::int64_t id = 0;
  std::string title;
  Lane lane = Lane::General;
  TaskState state = TaskState::Inbox;
  std::string brief;
};

bool can_transition(TaskState from, TaskState to);

std::string_view to_string(Lane lane);
std::string_view to_string(TaskState state);
std::optional<Lane> lane_from_string(std::string_view s);
std::optional<TaskState> state_from_string(std::string_view s);

}  // namespace conductor::core
