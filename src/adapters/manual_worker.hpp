#pragma once

#include <string>
#include <vector>

#include "core/worker.hpp"

namespace conductor::adapters {

// Fallback when no API or CLI can reach a model: Conductor writes the rendered prompt for a
// human to paste into any AI (web chat included), and the human submits the answer back with
// `conductor submit <task> <file>`. The task waits in Running until then.
class ManualWorker : public core::Worker {
 public:
  ManualWorker(std::string id, std::vector<core::Lane> lanes) : id_(std::move(id)), lanes_(std::move(lanes)) {}

  const std::string& id() const override { return id_; }
  const std::vector<core::Lane>& lanes() const override { return lanes_; }
  core::RunOutcome run(const core::Brief& brief, const std::filesystem::path& sandbox) override;

 private:
  std::string id_;
  std::vector<core::Lane> lanes_;
};

}  // namespace conductor::adapters
