#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/task.hpp"

namespace conductor::core {

// A self-contained handoff written by the planner: workers get one precise brief, never the
// whole project. core::render (brief.hpp) turns it into the prompt every worker receives.
struct Brief {
  std::int64_t task_id = 0;
  std::string goal;           // what to produce, in one or two sentences
  std::string inputs;         // the only context the worker gets: files, snippets, links
  std::string output_format;  // exact shape of the deliverable
  std::string done_criteria;  // how the reviewer will judge it
  std::string review_notes;   // feedback from a rejected attempt, empty on the first try
};

// Only what the tool itself reports; never estimated or scraped.
struct Usage {
  std::int64_t tokens_in = 0;
  std::int64_t tokens_out = 0;
  std::optional<double> cost_usd;
};

struct RunOutcome {
  int exit_code = 0;
  std::filesystem::path result_file;
  std::optional<Usage> usage;
};

class Worker {
 public:
  virtual ~Worker() = default;
  virtual const std::string& id() const = 0;
  virtual const std::vector<Lane>& lanes() const = 0;
  virtual RunOutcome run(const Brief& brief, const std::filesystem::path& sandbox) = 0;
};

}  // namespace conductor::core
