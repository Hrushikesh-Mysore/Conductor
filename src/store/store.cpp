#include "store/store.hpp"

#include <sqlite3.h>

#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>

namespace conductor::store {

namespace {

// Schema v1. Credentials are keychain references or profile folder paths, never secrets.
constexpr const char* kSchemaV1 = R"sql(
CREATE TABLE IF NOT EXISTS connections (
  id           INTEGER PRIMARY KEY,
  provider     TEXT NOT NULL,
  kind         TEXT NOT NULL CHECK (kind IN ('api_key', 'cli_session')),
  identity     TEXT NOT NULL,
  profile_dir  TEXT,
  credential_ref TEXT,
  status       TEXT NOT NULL DEFAULT 'needs-login'
               CHECK (status IN ('ready', 'paused', 'needs-login', 'suspended')),
  paused_until TEXT,
  budget_cap_usd REAL
);
CREATE TABLE IF NOT EXISTS workers (
  id            TEXT PRIMARY KEY,
  adapter       TEXT NOT NULL,
  lanes         TEXT NOT NULL,
  model         TEXT,
  connection_id INTEGER REFERENCES connections(id),
  concurrency   INTEGER NOT NULL DEFAULT 1
);
CREATE TABLE IF NOT EXISTS tasks (
  id         INTEGER PRIMARY KEY,
  title      TEXT NOT NULL,
  lane       TEXT NOT NULL,
  priority   INTEGER NOT NULL DEFAULT 0,
  state      TEXT NOT NULL DEFAULT 'inbox',
  brief      TEXT NOT NULL,
  parent_id  INTEGER REFERENCES tasks(id),
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),
  due_at     TEXT
);
CREATE TABLE IF NOT EXISTS runs (
  id          INTEGER PRIMARY KEY,
  task_id     INTEGER NOT NULL REFERENCES tasks(id),
  worker_id   TEXT NOT NULL REFERENCES workers(id),
  prompt_hash TEXT,
  started_at  TEXT NOT NULL,
  ended_at    TEXT,
  exit_status INTEGER,
  tokens_in   INTEGER,
  tokens_out  INTEGER,
  cost_usd    REAL,
  artifact_dir TEXT
);
CREATE TABLE IF NOT EXISTS usage_events (
  id            INTEGER PRIMARY KEY,
  connection_id INTEGER NOT NULL REFERENCES connections(id),
  run_id        INTEGER REFERENCES runs(id),
  tokens        INTEGER,
  cost_usd      REAL,
  at            TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS approvals (
  id         INTEGER PRIMARY KEY,
  run_id     INTEGER NOT NULL REFERENCES runs(id),
  summary    TEXT NOT NULL,
  payload    TEXT NOT NULL,
  decision   TEXT CHECK (decision IN ('approved', 'edited', 'rejected')),
  decided_at TEXT
);
CREATE TABLE IF NOT EXISTS policies (
  version    INTEGER PRIMARY KEY,
  body       TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))
);
)sql";

// Append-only: migration N upgrades user_version N-1 -> N. Never edit a shipped entry.
constexpr const char* kMigrations[] = {kSchemaV1};
constexpr int kSchemaVersion = static_cast<int>(std::size(kMigrations));

using Stmt = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

[[noreturn]] void fail(sqlite3* db, const char* what) {
  throw std::runtime_error(std::string(what) + ": " + sqlite3_errmsg(db));
}

Stmt prepare(sqlite3* db, const char* sql) {
  sqlite3_stmt* s = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) fail(db, "prepare");
  return Stmt(s, sqlite3_finalize);
}

std::string column_text(sqlite3_stmt* s, int col) {
  auto* p = sqlite3_column_text(s, col);
  return p ? reinterpret_cast<const char*>(p) : "";
}

}  // namespace

Store::Store(const std::string& path) {
  if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
    std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
    sqlite3_close(db_);
    throw std::runtime_error("open " + path + ": " + msg);
  }
  // foreign_keys is per-connection and off by default in SQLite; WAL keeps readers unblocked.
  if (sqlite3_exec(db_, "PRAGMA foreign_keys = ON; PRAGMA journal_mode = WAL; PRAGMA busy_timeout = 5000;",
                   nullptr, nullptr, nullptr) != SQLITE_OK) {
    std::string msg = sqlite3_errmsg(db_);
    sqlite3_close(db_);
    throw std::runtime_error("configure " + path + ": " + msg);
  }
}

Store::~Store() { sqlite3_close(db_); }

int Store::schema_version() {
  auto s = prepare(db_, "PRAGMA user_version");
  if (sqlite3_step(s.get()) != SQLITE_ROW) fail(db_, "schema_version");
  return sqlite3_column_int(s.get(), 0);
}

void Store::migrate() {
  int current = schema_version();
  if (current > kSchemaVersion)
    throw std::runtime_error("database schema v" + std::to_string(current) + " is newer than this build (v" +
                             std::to_string(kSchemaVersion) + "); refusing to touch it");
  for (int v = current; v < kSchemaVersion; ++v) {
    // One transaction per step: a failed migration leaves the previous version intact.
    std::string sql = "BEGIN IMMEDIATE;\n" + std::string(kMigrations[v]) +
                      "\nPRAGMA user_version = " + std::to_string(v + 1) + ";\nCOMMIT;";
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
      std::string msg = err ? err : "unknown";
      sqlite3_free(err);
      sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
      throw std::runtime_error("migrate to v" + std::to_string(v + 1) + ": " + msg);
    }
  }
}

std::int64_t Store::add_task(std::string_view title, core::Lane lane, std::string_view brief) {
  auto s = prepare(db_, "INSERT INTO tasks (title, lane, brief) VALUES (?, ?, ?)");
  auto lane_name = core::to_string(lane);
  sqlite3_bind_text(s.get(), 1, title.data(), static_cast<int>(title.size()), SQLITE_TRANSIENT);
  sqlite3_bind_text(s.get(), 2, lane_name.data(), static_cast<int>(lane_name.size()), SQLITE_STATIC);
  sqlite3_bind_text(s.get(), 3, brief.data(), static_cast<int>(brief.size()), SQLITE_TRANSIENT);
  if (sqlite3_step(s.get()) != SQLITE_DONE) fail(db_, "add_task");
  return sqlite3_last_insert_rowid(db_);
}

std::optional<core::Task> Store::get_task(std::int64_t id) {
  auto s = prepare(db_, "SELECT id, title, lane, state, brief FROM tasks WHERE id = ?");
  sqlite3_bind_int64(s.get(), 1, id);
  int rc = sqlite3_step(s.get());
  if (rc == SQLITE_DONE) return std::nullopt;
  if (rc != SQLITE_ROW) fail(db_, "get_task");

  auto lane = core::lane_from_string(column_text(s.get(), 2));
  auto state = core::state_from_string(column_text(s.get(), 3));
  if (!lane || !state) throw std::runtime_error("get_task: corrupt lane/state in row");
  return core::Task{sqlite3_column_int64(s.get(), 0), column_text(s.get(), 1), *lane, *state,
                    column_text(s.get(), 4)};
}

void Store::set_state(std::int64_t id, core::TaskState to) {
  auto task = get_task(id);
  if (!task) throw std::runtime_error("set_state: no task " + std::to_string(id));
  if (!core::can_transition(task->state, to))
    throw std::runtime_error("set_state: illegal transition " + std::string(core::to_string(task->state)) +
                             " -> " + std::string(core::to_string(to)));
  // Compare-and-set: if another process moved the task since we read it, nothing changes.
  auto s = prepare(db_, "UPDATE tasks SET state = ? WHERE id = ? AND state = ?");
  auto name = core::to_string(to);
  auto from = core::to_string(task->state);
  sqlite3_bind_text(s.get(), 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
  sqlite3_bind_int64(s.get(), 2, id);
  sqlite3_bind_text(s.get(), 3, from.data(), static_cast<int>(from.size()), SQLITE_STATIC);
  if (sqlite3_step(s.get()) != SQLITE_DONE) fail(db_, "set_state");
  if (sqlite3_changes(db_) != 1)
    throw std::runtime_error("set_state: task " + std::to_string(id) + " changed concurrently");
}

}  // namespace conductor::store
