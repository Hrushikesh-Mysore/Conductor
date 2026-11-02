#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/router.hpp"

namespace conductor::core {

// Who decides which worker gets a task.
//   Router:  Conductor's rules decide; a requested worker is ignored.
//   Planner: the planner AI names the worker. It is used only if Ready and is never
//            silently swapped; with no request, the router decides.
enum class DispatchMode { Router, Planner };

std::optional<std::string> assign(DispatchMode mode, const Router& router, Lane lane,
                                  const std::vector<std::string>& tags,
                                  const std::optional<std::string>& requested_worker,
                                  const std::map<std::string, ConnectionStatus>& worker_status);

}  // namespace conductor::core
