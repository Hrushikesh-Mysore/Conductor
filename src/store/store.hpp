#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "core/task.hpp"

struct sqlite3;

namespace conductor::store {

// SQLite-backed state. Every task transition is persisted, so a crash never loses work.
class Store {
 public:
  explicit Store(const std::string& path);  // ":memory:" for tests
  ~Store();
  Store(const Store&) = delete;
  Store& operator=(const Store&) = delete;

  // Applies pending migrations in order; refuses a database from a newer build.
  void migrate();
  int schema_version();
  sqlite3* handle() { return db_; }  // escape hatch for tests and future repositories
  std::int64_t add_task(std::string_view title, core::Lane lane, std::string_view brief);
  std::optional<core::Task> get_task(std::int64_t id);
  // Throws if the transition is not allowed by core::can_transition.
  void set_state(std::int64_t id, core::TaskState to);

 private:
  sqlite3* db_ = nullptr;
};

}  // namespace conductor::store
