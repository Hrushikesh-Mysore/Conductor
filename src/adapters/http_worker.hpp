#pragma once

#include <string>
#include <vector>

#include "core/worker.hpp"

namespace conductor::adapters {

// Worker for OpenAI-compatible chat endpoints: covers most free API tiers behind one adapter.
// The API key is never stored here, only a keychain reference resolved at run time.
struct HttpEndpoint {
  std::string base_url;  // e.g. https://<provider>/v1
  std::string model;
  std::string key_ref;   // keychain entry name, never the secret itself
};

class HttpWorker : public core::Worker {
 public:
  HttpWorker(std::string id, std::vector<core::Lane> lanes, HttpEndpoint endpoint)
      : id_(std::move(id)), lanes_(std::move(lanes)), endpoint_(std::move(endpoint)) {}

  const std::string& id() const override { return id_; }
  const std::vector<core::Lane>& lanes() const override { return lanes_; }
  core::RunOutcome run(const core::Brief& brief, const std::filesystem::path& sandbox) override;

 private:
  std::string id_;
  std::vector<core::Lane> lanes_;
  HttpEndpoint endpoint_;
};

}  // namespace conductor::adapters
