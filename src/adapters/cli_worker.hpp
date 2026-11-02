#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/worker.hpp"

namespace conductor::adapters {

// Runs a vendor CLI headless (e.g. `claude -p`, `codex exec --json`, `gemini -p`).
// The brief goes in on stdin, stdout lands in <sandbox>/result.md. The brief is never
// placed on the command line, so its contents cannot inject shell arguments.
class CliWorker : public core::Worker {
 public:
  CliWorker(std::string id, std::vector<core::Lane> lanes, std::vector<std::string> argv,
            std::map<std::string, std::string> env = {})
      : id_(std::move(id)), lanes_(std::move(lanes)), argv_(std::move(argv)), env_(std::move(env)) {}

  const std::string& id() const override { return id_; }
  const std::vector<core::Lane>& lanes() const override { return lanes_; }
  core::RunOutcome run(const core::Brief& brief, const std::filesystem::path& sandbox) override;

 private:
  std::string id_;
  std::vector<core::Lane> lanes_;
  std::vector<std::string> argv_;
  std::map<std::string, std::string> env_;  // e.g. CLAUDE_CONFIG_DIR, CODEX_HOME per profile
};

}  // namespace conductor::adapters
