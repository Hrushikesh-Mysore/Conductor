# Conductor: A Personal AI Operations Hub

Working name. Draft pitch, November 2026. C++20 core, single-user first.

> Converted from the original Word draft and updated: the core is now C++20 (it was Rust), and the product is centred on a pluggable planner AI that dispatches briefs to free workers and reviews their results.

## The one-line pitch

Conductor lets one strong **planner AI** run a project the way a lead runs a team. The planner plans with you, then hands precise, bite-sized briefs to many **free** AI workers (free API tiers, official CLIs, or you pasting a prompt into any chat), and reviews what comes back. You get more total tokens, small focused contexts, and one reviewer who signs off on everything.

## The problem

A developer today juggles Claude, Codex, Gemini or Antigravity, plus email, calendar, LinkedIn, and code hosts. Each lives in its own app with its own usage meter. The result:

- Expensive models get used for cheap work (summarizing, triage, boilerplate), so limits burn out before the hard problems arrive.
- Nobody has a single view of what is connected, what is running, and what is left.
- Context is re-explained to every tool, every time.
- Nothing is auditable. There is no record of which agent did what, or why.
- Generic routers (OmniRoute-style gateways) solve model switching but not life and work management: tasks, approvals, lanes, handoffs, and personal connectors.

## The idea

You are the head. The planner AI is your lead. Conductor is the staff office.

1. **Plan.** You and the planner agree on the project. The planner can be any capable AI: Claude, Codex, agy, or another LLM. Nothing in Conductor assumes a specific one.
2. **Brief.** The planner splits the plan into small tasks, each with a precise brief: goal, inputs, output format, and done-criteria. A worker sees only its brief, never the whole project, so its context stays small.
3. **Dispatch.** Each brief goes to a worker, decided one of two ways:
   - **Router mode:** Conductor's rules pick the worker by lane, tags, budget and availability.
   - **Planner mode:** the planner names the worker itself. Conductor checks that it is ready and within budget, and never swaps it out.
4. **Work.** Workers are free API tiers, official CLIs, or the **manual fallback**: Conductor prints the prompt, you paste it into any AI, and you submit the answer back. Workers can use web research tools.
5. **Review.** The planner reviews every result. Accepted results are done; rejected ones go back with notes, to the same worker or another.
6. **Done.** When every task is accepted, the planner does a final review and the project is complete.

Why it works: the expensive model spends tokens only on planning and review, while the bulk work runs on free quota, and precise briefs let small models succeed.

## Three lanes

- **Research.** Long-context reading, web gathering, summaries. Output is a file in `/research`, never a chat log.
- **Main coding (highest priority).** Claude plans and reviews; Codex or another coding worker implements well-specified chunks in an isolated git worktree. Every change comes back as a diff for review.
- **Jack of all trades.** Inbox triage, scheduling, drafting, cleanup, LinkedIn drafts. Anything that touches the outside world goes through an approval gate.

## Design principles

- **Routing, not rotation.** Savings come from sending the right task to the right tool, not from hopping between accounts. A connection that is out of quota is paused and shown as paused. Conductor never auto-rotates around limits, and never works around phone or identity verification. This keeps every provider account in good standing and the system stable.
- **Official interfaces only.** Workers are driven through each vendor's supported surface: API keys, or the official CLIs run headless (for example Claude Code, Codex CLI, Gemini CLI). Conductor does not extract or replay consumer session tokens.
- **Human in the loop by default.** Reads are free. Writes to the outside world (send email, post, merge, delete) need approval until you explicitly grant a standing rule.
- **Files over chat.** Agents hand off through briefs and result files. A 300-word brief is cheaper and more reliable than a long conversation.
- **Local-first, single binary.** SQLite, your machine, your secrets in the OS keychain.
- **Everything is logged.** Every run records the task, worker, prompt hash, tokens, cost, duration, and outcome.

## Architecture

```
          You  ⇄  Planner AI (any: Claude, Codex, agy, other LLM)
                        │  via `conductor` CLI  or  MCP
                        ▼
   +--------------- Conductor Core (C++20) ----------------+
   |  Planner API   Task queue   Dispatch (router|planner)  |
   |  Review loop   Usage ledger   Approvals   Event bus    |
   |  Lane classifier (ONNX Runtime, optional, router mode) |
   +---------------------------+----------------------------+
                               │  one rendered brief per task
        +----------------------+-----------------------+
   Free API workers       Official CLI workers     Manual worker
   (OpenAI-compatible)    (codex, agy, claude...)  (you paste the prompt
                                                    into any AI, submit back)
        +----------------------+-----------------------+
                               │  results → review
              Sandboxes (git worktrees, temp dirs) + web research tools
                               │
                     SQLite + artifacts folder
```

## Core components

- **Task queue.** Tasks move through Inbox, Triaged, Assigned, Running, Review, Done or Rejected. State lives in SQLite so a crash never loses work.
- **Router and policy engine.** Version 1 is deterministic: a TOML policy maps lane, task tags, and budget to a worker. A model-assisted classifier (ONNX Runtime, `src/ml`) is added later, with the rule engine as the safety net.
- **Worker adapters.** One C++ interface, many implementations. Each adapter spawns the tool headless, streams output, and normalizes results and usage.
- **Usage ledger.** Records tokens and cost where the provider reports them (API responses, CLI JSON output). For subscription tools that expose no meter, the dashboard shows what the tool itself reports and your own run counts, and does not scrape private endpoints.
- **Approval gate.** Any action tagged as external-side-effect pauses until you approve, edit, or reject.
- **Event bus.** Internal broadcast channel that feeds live dashboard updates and the audit log.
- **Planner API and MCP server.** `create_task`, `list_tasks`, `assign`, `get_result` and `review`, available as `conductor` CLI commands for planners that run shell commands, and as MCP tools for planners that speak MCP. This is how any planner AI drives Conductor.
- **Review loop.** Every result lands in Review. The planner accepts it or rejects it with notes, and the notes are fed into the next attempt's brief.

## Worker interface (as built in `src/core/worker.hpp`)

```cpp
class Worker {
 public:
  virtual ~Worker() = default;
  virtual const std::string& id() const = 0;
  virtual const std::vector<Lane>& lanes() const = 0;
  virtual RunOutcome run(const Brief& brief, const std::filesystem::path& sandbox) = 0;
  // RunOutcome carries the usage the tool honestly reports, or none.
};
```

## Data model (SQLite)

- **connections:** provider, kind (api_key or cli_session), identity, profile_dir, credential reference (keychain key, never the secret), status, budget cap.
- **workers:** adapter type, lanes, model, linked connection, concurrency limit.
- **tasks:** title, lane, priority, state, brief, parent task, created and due times.
- **runs:** task, worker, started and ended, exit status, tokens in and out, cost estimate, artifact paths.
- **usage_events:** connection, run, tokens, cost, timestamp.
- **approvals:** run, action summary, payload, decision, decided at.
- **policies:** versioned routing rules.

## Dashboard

The first version needs five views:

1. **Overview.** Connections and their status, running tasks, pending approvals, spend today and this week.
2. **Connections.** Each provider connection with health, usage as reported, budget cap, and a pause switch.
3. **Task board.** Kanban by state. Drag a task to a lane or worker to override the router.
4. **Run detail.** Brief, live output, diff or result file, token and cost breakdown, retry and reassign buttons.
5. **Approvals.** A queue of pending external actions with one-click approve, edit, or reject.

## C++ stack

In use now: C++20, CMake, SQLite3 (C API), ONNX Runtime 1.31 C++ API (`src/ml`), POSIX `fork`/`execvp` for CLI workers.

Planned. Verify current versions before pinning, and add each one only in the phase that needs it:

| Need | Candidate |
|---|---|
| HTTP API + server-sent events | cpp-httplib or Boost.Beast |
| JSON (CLI output, MCP) | nlohmann/json |
| Policy file | toml++ |
| HTTP client (API-key providers, connectors) | libcurl |
| Secrets | libsecret (Linux), Keychain (macOS), Credential Manager (Windows) |
| Logging and tracing | spdlog |
| TUI dashboard | FTXUI |
| Git sandboxes | shell out to `git worktree` first, libgit2 if more control is needed |
| MCP | a small server module speaking the Model Context Protocol over stdio |

## Build plan

- **Phase 0: Spike.** One CLI worker headless: brief in, result file and usage out.
- **Phase 1: Core.** Task state machine, store, router, dispatch modes, policy file, usage ledger, git worktree sandboxes.
- **Phase 2: Planner API + MCP.** The commands any planner uses (`create_task`, `list_tasks`, `assign`, `get_result`, `review`), on the CLI and over MCP.
- **Phase 3: Free workers.** OpenAI-compatible HTTP worker for free API tiers, and the manual copy-paste flow.
- **Phase 4: Review loop.** Reject with notes, retry or reassign, review-round limits, final project review.
- **Phase 5: Control surface.** TUI dashboard, then HTTP API and web UI.
- **Phase 6: Web research tools and connectors.** A web/browser research sidecar; email, calendar and GitHub through official APIs, behind the approval gate.

## Why the rails come first

The planner is only as safe as the rails under it: budgets, sandboxes, the approval gate, review limits and the audit log. Those are built in C++ and tested before any planner gets to run many workers at once.

## Risks and how the design handles them

- **Runaway cost or loops.** Hard per-connection and per-task budgets, step limits, and a global kill switch.
- **Destructive actions.** Sandboxed worktrees, approval gate, and no standing write permissions at the start.
- **Provider terms and policy changes.** Official CLIs and API keys only, with adapters isolated behind one interface so one vendor change touches one file.
- **Usage visibility gaps.** Some subscription tools expose limited meters. The dashboard says so rather than guessing.
- **Prompt injection through email or web content.** Content fetched by connectors is treated as data, never as instructions, and anything it triggers still hits the approval gate.

## What success looks like

- One place to see every connected tool, task, and cost.
- Most routine work handled by the cheapest capable worker, with the strongest model reserved for planning and review.
- Coding tasks arrive as reviewed diffs, not chat transcripts.
- A complete audit trail of what every agent did.
- A planner AI that runs a whole project on free workers, within your rules, and hands you reviewed results.
