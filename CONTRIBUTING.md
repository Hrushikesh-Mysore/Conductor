# Contributing

## Build and test

```sh
cmake -B build -DCONDUCTOR_WITH_ML=OFF        # core only, no ONNX Runtime needed
cmake --build build -j
ctest --test-dir build --output-on-failure
```

For the ML component, see [README](README.md#build).

## Workflow

- Branch from `main` (`feature/<area>-<short-name>`) and open a pull request. CI must pass before merging.
- Pick a component from the phases in [docs/setup-plan.md](docs/setup-plan.md#7-build-plan-with-exit-criteria). Each folder under `src/` is one component, so people can work in parallel.
- Replace a stub's `not implemented` body with real code, add tests in `tests/`, and tick its box in the setup plan.
- Keep `src/core` free of third-party dependencies, and keep ONNX Runtime inside `src/ml`.
- Add a new dependency only in the phase that needs it, find it through CMake, and never vendor it.

## Code style

- C++20, `-Wall -Wextra` clean (CI uses `-Werror`).
- Comment the *why*, not the *what*. Mark known limits with `// shortcut: <limit>, <when to upgrade>`.

## AI assistants

Use whichever assistant you like. Keep its files local: `.claude/`, `.codex/`, `.agy/`, `.gemini/`, `CLAUDE.md`, `AGENTS.md`, `GEMINI.md`, `agy.md` and similar are gitignored, and CI fails if one is committed. Shared instructions for planner AIs live in [docs/planner-guide.md](docs/planner-guide.md).
