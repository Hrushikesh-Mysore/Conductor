#include "core/dispatch.hpp"

namespace conductor::core {

std::optional<std::string> assign(DispatchMode mode, const Router& router, Lane lane,
                                  const std::vector<std::string>& tags,
                                  const std::optional<std::string>& requested_worker,
                                  const std::map<std::string, ConnectionStatus>& worker_status) {
  if (mode == DispatchMode::Planner && requested_worker) {
    auto it = worker_status.find(*requested_worker);
    if (it != worker_status.end() && it->second == ConnectionStatus::Ready) return *requested_worker;
    return std::nullopt;  // the planner's choice is paused or unknown: wait, don't substitute
  }
  return router.route(lane, tags, worker_status);
}

}  // namespace conductor::core
