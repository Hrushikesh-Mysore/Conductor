#pragma once

#include "planner/planner_api.hpp"

namespace conductor::mcp {

// Model Context Protocol server over stdio. Maps MCP tools (create_task, list_tasks, assign,
// get_result, review) one-to-one onto planner::PlannerApi, so any MCP-capable planner
// (Claude Code, Codex CLI, agy, ...) can drive Conductor without custom integration.
class McpServer {
 public:
  explicit McpServer(planner::PlannerApi& api) : api_(api) {}
  int serve_stdio();  // blocks until stdin closes; returns the process exit code

 private:
  planner::PlannerApi& api_;
};

}  // namespace conductor::mcp
