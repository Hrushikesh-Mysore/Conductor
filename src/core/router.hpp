#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/task.hpp"

namespace conductor::core {

enum class ConnectionStatus { Ready, Paused, NeedsLogin, Suspended };

// One policy line: tasks in `lane` (optionally carrying `tag`) go to `worker_id`.
struct RoutingRule {
  Lane lane;
  std::optional<std::string> tag;
  std::string worker_id;
};

// Deterministic v1 router: first matching rule whose worker is Ready wins.
// No match means the task waits; it never rotates around limits.
class Router {
 public:
  explicit Router(std::vector<RoutingRule> rules) : rules_(std::move(rules)) {}

  std::optional<std::string> route(Lane lane, const std::vector<std::string>& tags,
                                   const std::map<std::string, ConnectionStatus>& worker_status) const;

 private:
  std::vector<RoutingRule> rules_;
};

}  // namespace conductor::core
