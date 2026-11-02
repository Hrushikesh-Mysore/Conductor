#include "core/brief.hpp"

namespace conductor::core {

std::string render(const Brief& brief) {
  std::string out = "# Task\n" + brief.goal + "\n";
  auto section = [&out](const char* title, const std::string& body) {
    if (!body.empty()) out += std::string("\n## ") + title + "\n" + body + "\n";
  };
  section("Inputs", brief.inputs);
  section("Output format", brief.output_format);
  section("Done when", brief.done_criteria);
  section("Reviewer notes from the previous attempt", brief.review_notes);
  out += "\nReply with the deliverable only. No preamble, no questions.\n";
  return out;
}

}  // namespace conductor::core
