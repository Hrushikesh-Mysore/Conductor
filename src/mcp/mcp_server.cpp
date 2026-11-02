#include "mcp/mcp_server.hpp"

#include <stdexcept>

namespace conductor::mcp {

// Phase 2 (see docs/setup-plan.md). Needs a JSON library; add it when this is implemented.
int McpServer::serve_stdio() {
  (void)api_;
  throw std::logic_error("McpServer::serve_stdio: not implemented");
}

}  // namespace conductor::mcp
