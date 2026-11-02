#undef NDEBUG  // asserts must run in every build type
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include <sqlite3.h>

#include "adapters/cli_worker.hpp"
#include "core/brief.hpp"
#include "core/dispatch.hpp"
#include "core/router.hpp"
#include "core/task.hpp"
#include "store/store.hpp"

using namespace conductor;
using core::Lane;
using core::TaskState;

static void test_state_machine() {
  assert(core::can_transition(TaskState::Inbox, TaskState::Triaged));
  assert(core::can_transition(TaskState::Review, TaskState::Done));
  assert(!core::can_transition(TaskState::Inbox, TaskState::Done));
  assert(!core::can_transition(TaskState::Done, TaskState::Running));
  for (auto s : {TaskState::Inbox, TaskState::Review, TaskState::Rejected})
    assert(core::state_from_string(core::to_string(s)) == s);
  assert(!core::lane_from_string("bogus"));
}

static void test_router() {
  using core::ConnectionStatus;
  core::Router router({
      {Lane::Coding, std::string("hard"), "claude"},
      {Lane::Coding, std::nullopt, "codex"},
      {Lane::Research, std::nullopt, "gemini"},
  });
  std::map<std::string, ConnectionStatus> status{
      {"claude", ConnectionStatus::Ready}, {"codex", ConnectionStatus::Ready},
      {"gemini", ConnectionStatus::Paused}};

  assert(router.route(Lane::Coding, {"hard"}, status) == "claude");
  assert(router.route(Lane::Coding, {}, status) == "codex");
  assert(!router.route(Lane::Research, {}, status));  // paused: wait, don't rotate
  status["claude"] = ConnectionStatus::Paused;
  assert(router.route(Lane::Coding, {"hard"}, status) == "codex");
}

static core::Brief brief_of(std::int64_t id, std::string goal) {
  core::Brief b;
  b.task_id = id;
  b.goal = std::move(goal);
  return b;
}

static void test_brief() {
  auto b = brief_of(1, "Summarize X");
  b.output_format = "Markdown, 5 bullets";
  auto first = core::render(b);
  assert(first.find("# Task\nSummarize X") == 0);
  assert(first.find("## Output format\nMarkdown, 5 bullets") != std::string::npos);
  assert(first.find("## Inputs") == std::string::npos);  // empty sections are omitted
  b.review_notes = "Too long";
  auto retry = core::render(b);
  assert(retry.find("## Reviewer notes from the previous attempt\nToo long") != std::string::npos);
}

static void test_dispatch() {
  using core::ConnectionStatus;
  using core::DispatchMode;
  core::Router router({{Lane::Coding, std::nullopt, "codex"}});
  std::map<std::string, ConnectionStatus> status{{"codex", ConnectionStatus::Ready},
                                                 {"groq", ConnectionStatus::Ready},
                                                 {"agy", ConnectionStatus::Paused}};
  // Router mode ignores the planner's request.
  assert(core::assign(DispatchMode::Router, router, Lane::Coding, {}, "groq", status) == "codex");
  // Planner mode honours a ready request, and never substitutes a paused one.
  assert(core::assign(DispatchMode::Planner, router, Lane::Coding, {}, "groq", status) == "groq");
  assert(!core::assign(DispatchMode::Planner, router, Lane::Coding, {}, "agy", status));
  assert(!core::assign(DispatchMode::Planner, router, Lane::Coding, {}, "unknown", status));
  // Planner mode with no request falls back to the router.
  assert(core::assign(DispatchMode::Planner, router, Lane::Coding, {}, std::nullopt, status) == "codex");
}

static void test_store() {
  store::Store db(":memory:");
  db.migrate();
  auto id = db.add_task("summarize paper", Lane::Research, "Read X, write /research/x.md");
  auto t = db.get_task(id);
  assert(t && t->title == "summarize paper" && t->lane == Lane::Research && t->state == TaskState::Inbox);
  db.set_state(id, TaskState::Triaged);
  assert(db.get_task(id)->state == TaskState::Triaged);
  bool threw = false;
  try { db.set_state(id, TaskState::Done); } catch (const std::runtime_error&) { threw = true; }
  assert(threw);
  assert(!db.get_task(id + 1));

  // Foreign keys are enforced: a run cannot point at a task that does not exist.
  int rc = sqlite3_exec(db.handle(),
                        "INSERT INTO workers (id, adapter, lanes) VALUES ('w', 'cli', 'coding');"
                        "INSERT INTO runs (task_id, worker_id, started_at) VALUES (9999, 'w', 'now');",
                        nullptr, nullptr, nullptr);
  assert(rc == SQLITE_CONSTRAINT);
}

static void test_migrations() {
  auto path = std::filesystem::temp_directory_path() / "conductor_test_migrate.db";
  std::filesystem::remove(path);
  {
    store::Store db(path.string());
    assert(db.schema_version() == 0);
    db.migrate();
    assert(db.schema_version() == 1);
    db.add_task("keep me", Lane::General, "x");
  }
  {
    store::Store db(path.string());
    db.migrate();  // already current: no-op, data survives
    assert(db.schema_version() == 1 && db.get_task(1)->title == "keep me");
    sqlite3_exec(db.handle(), "PRAGMA user_version = 99", nullptr, nullptr, nullptr);
    bool threw = false;
    try { db.migrate(); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);  // a database from a newer build is never touched
  }
  for (auto suffix : {"", "-wal", "-shm"}) std::filesystem::remove(path.string() + suffix);
}

static void test_cli_worker() {
  auto sandbox = std::filesystem::temp_directory_path() / "conductor_test_sandbox";
  std::filesystem::remove_all(sandbox);
  adapters::CliWorker worker("cat", {Lane::General}, {"cat"});
  auto brief = brief_of(1, "hello; rm -rf / $(whoami)");
  auto out = worker.run(brief, sandbox);
  assert(out.exit_code == 0);
  std::stringstream got;
  got << std::ifstream(out.result_file).rdbuf();
  assert(got.str() == core::render(brief));  // passed as data, never as shell

  assert((std::filesystem::status(sandbox / "brief.md").permissions() & std::filesystem::perms::others_read) ==
         std::filesystem::perms::none);

  // A missing binary is a startup error, not a worker exit code.
  adapters::CliWorker missing("missing", {Lane::General}, {"conductor-no-such-binary"});
  bool threw = false;
  try { missing.run(brief_of(2, ""), sandbox); } catch (const std::system_error&) { threw = true; }
  assert(threw);

  // A real non-zero exit is reported, and stderr lands in the sandbox, not on our terminal.
  adapters::CliWorker failing("sh", {Lane::General}, {"sh", "-c", "echo oops >&2; exit 3"});
  assert(failing.run(brief_of(3, ""), sandbox).exit_code == 3);
  std::stringstream err;
  err << std::ifstream(sandbox / "stderr.log").rdbuf();
  assert(err.str() == "oops\n");
  std::filesystem::remove_all(sandbox);
}

int main() {
  test_state_machine();
  test_router();
  test_brief();
  test_dispatch();
  test_store();
  test_migrations();
  test_cli_worker();
  std::cout << "all tests passed\n";
}
