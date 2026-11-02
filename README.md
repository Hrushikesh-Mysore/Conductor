# Conductor
You manage the goals, Conductor manages the AI workforce that gets them done.

Conductor lets one **planner AI** (Claude, Codex, agy, or any LLM) run a project like a team lead:
1. It plans with you.
2. It splits the plan into precise briefs.
3. It dispatches them to **free workers**: free API tiers, official CLIs, or you pasting a prompt into any AI.
4. It reviews every result before the project is done.

Conductor is self-hosted, local-first and written in C++20.

- [Pitch](docs/pitch.md): what Conductor is and why
- [Setup plan](docs/setup-plan.md): decisions, providers, build phases with status
- [Planner guide](docs/planner-guide.md): the file you hand to the planner AI
- [Contributing](CONTRIBUTING.md): how the team works on this repo

## Layout

```
src/core/       tasks + state machine, router, dispatch modes (router|planner), Brief + prompt template,
                Worker interface. No third-party deps
src/store/      SQLite schema, versioned migrations, task persistence
src/planner/    PlannerApi: create_task, list_tasks, assign, get_result, review (stub)
src/mcp/        MCP stdio server exposing PlannerApi to MCP-capable planners (stub)
src/adapters/   workers: cli_worker (done), http_worker for free APIs (stub), manual_worker copy-paste (stub)
src/ml/         optional ONNX Runtime lane classifier, isolated behind ml::LaneClassifier
src/app/        the `conductor` CLI
tests/          assert-based tests run by CTest
config/         policy.example.toml: dispatch mode, workers, routes, budgets
tools/web/      web research sidecar (placeholder)
cmake/          FindONNXRuntime.cmake
models/         where .onnx models go locally (never committed, see models/README.md)
docs/           pitch, setup plan, planner guide
```

Status: `done` parts are implemented and tested. `stub` parts have their interface and compile, and throw "not implemented" until their phase.

## Dependencies

| Dependency | Version | Needed by | How it is found |
|---|---|---|---|
| C++20 compiler | GCC 12+ / Clang 15+ | everything | |
| CMake | 3.24+ | build | |
| SQLite3 | 3.x | `src/store` | `find_package(SQLite3)` (e.g. `apt install libsqlite3-dev`) |
| ONNX Runtime | **1.31.x** (pinned, tested with 1.31.1) | `src/ml` only | `cmake/FindONNXRuntime.cmake`, hinted by `ONNXRUNTIME_ROOT` |

ONNX Runtime is never vendored or added as a submodule. Install the SDK from the [v1.31.1 release](https://github.com/microsoft/onnxruntime/releases/tag/v1.31.1) (for example `onnxruntime-linux-x64-1.31.1.tgz`) and point CMake at it. The pinned version lives in `src/ml/CMakeLists.txt`.

## Build

```sh
# Full build, with ML
cmake -B build -DONNXRUNTIME_ROOT=/opt/onnxruntime-linux-x64-1.31.1
cmake --build build -j
ctest --test-dir build --output-on-failure

# Core only, no ONNX Runtime needed
cmake -B build -DCONDUCTOR_WITH_ML=OFF
```

If ONNX Runtime is missing, configure stops with an error that says how to install it or how to turn ML off.

At runtime the ML build needs `libonnxruntime.so` on the loader path (`LD_LIBRARY_PATH=$ONNXRUNTIME_ROOT/lib`, or install it system-wide).

## Try it

```sh
./build/conductor init conductor.db
./build/conductor spike brief.md ./sandbox -- claude -p      # Phase 0: one headless worker
./build/conductor review 1 accept                            # planner commands: stubs for now
./build/conductor classify models/lane.onnx "summarize this paper"
```

## Repository rules

AI agent files (`.claude/`, `.codex/`, `.agy/`, `.gemini/`, `CLAUDE.md`, `AGENTS.md`, `GEMINI.md`, `agy.md`, ...) stay local. `.gitignore` excludes them, and CI fails if one is ever committed.
