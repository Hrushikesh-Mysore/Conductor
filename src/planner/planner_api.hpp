#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/task.hpp"
#include "core/worker.hpp"
#include "store/store.hpp"

namespace conductor::planner {

// The surface any planner AI (Claude, Codex, agy, any LLM) uses to drive Conductor.
// Exposed through the `conductor` CLI for planners that run shell commands, and through
// src/mcp for planners that speak MCP. Keep it small and model-agnostic.
class PlannerApi {
 public:
  explicit PlannerApi(store::Store& store) : store_(store) {}

  std::int64_t create_task(const std::string& title, core::Lane lane, const core::Brief& brief);
  std::vector<core::Task> list_tasks(std::optional<core::TaskState> state = std::nullopt);
  // Planner mode: name the worker. Router mode: pass nullopt and let the rules decide.
  std::optional<std::string> assign(std::int64_t task_id, const std::optional<std::string>& worker_id);
  std::optional<std::filesystem::path> get_result(std::int64_t task_id);
  // accept: Review -> Done. reject: Review -> Assigned, with notes fed into the next brief.
  void review(std::int64_t task_id, bool accept, const std::string& notes);

 private:
  [[maybe_unused]] store::Store& store_;
};

}  // namespace conductor::planner
