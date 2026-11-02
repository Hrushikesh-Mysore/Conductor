#include "core/router.hpp"

#include <algorithm>

namespace conductor::core {

std::optional<std::string> Router::route(
    Lane lane, const std::vector<std::string>& tags,
    const std::map<std::string, ConnectionStatus>& worker_status) const {
  for (const auto& rule : rules_) {
    if (rule.lane != lane) continue;
    if (rule.tag && std::find(tags.begin(), tags.end(), *rule.tag) == tags.end()) continue;
    auto it = worker_status.find(rule.worker_id);
    if (it != worker_status.end() && it->second == ConnectionStatus::Ready) return rule.worker_id;
  }
  return std::nullopt;
}

}  // namespace conductor::core
