#pragma once

#include <string>

#include "core/worker.hpp"

namespace conductor::core {

// One fixed prompt template for every worker, so any model (free API, CLI, or a human
// pasting into a web chat) gets the same precise instructions. Empty sections are omitted.
std::string render(const Brief& brief);

}  // namespace conductor::core
