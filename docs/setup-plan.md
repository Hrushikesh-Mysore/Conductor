# Conductor: Setup Plan

Nov 2, 2026 · Hrushikesh M

> Sections 1 to 4 are converted from the original Word draft (corrections are marked). Sections 5 onward complete the plan around the product loop: **planner AI → briefs → free workers → review**. See [pitch](pitch.md#the-idea).

## 1. My take

The core idea is sound, but the "free plans plus many Gmail accounts" half is the risky half, and it cuts against the rules in your own pitch.

**What holds up:** one planner agent on top, workers sorted into lanes, and MCP as the join between them. That is how I would build it, and it matches your Phase 4.

**What worries me:**

- Free plans have no API. The supported way in is each vendor's own CLI, logged in by hand, so the "connector" is a profile folder, not a key.
- Stacking several free accounts at one vendor to multiply quota is what the terms target. GitHub allows one free account per person. Google has suspended Gemini CLI and Antigravity access over third-party use. Anthropic limits Claude subscription logins to Claude Code and claude.ai.
- OmniRoute is open source (MIT) and already routes through a Subscription, API key, Cheap, Free cascade. A copy repeats that work.

**What I would change:**

- Keep accounts as a registry, not a rotation pool: one login per vendor per real identity (your personal and your work one, say), with honest "paused" states. That is your pitch's own "routing, not rotation" rule.
- Use OmniRoute or a similar gateway as one worker for API keys and real free tiers. Spend your effort on what it lacks: tasks, sandboxes, approvals and the planner.
- Give the top agent web access, but behind the approval gate, because web pages can carry prompt injection.

The sections below show how to connect everything under those three rules.

## 2. Four ways to plug an AI in

Start with lanes A and C. Lanes B and D are opt-in, because that is where bans and breakage happen.

| Lane | How the AI plugs in | Good for | Ban risk (my read) |
|---|---|---|---|
| A. Official CLI, own login | Conductor spawns the vendor's own CLI headless (`claude -p`, `codex exec --json`, `gemini -p`, `copilot --acp`) with that account's profile folder | Coding and research workers on a free or paid plan | Lowest of the subscription routes: the vendor's own program, your own login |
| B. Token gateway | A local gateway such as OmniRoute exposes one OpenAI-style endpoint and holds the provider logins | Many models behind one URL | Medium to high wherever it reuses subscription logins. OmniRoute itself marks 12 providers "avoid" and leaves them out of automatic routing by default |
| C. Real free API tiers | Free API keys from vendors that offer them, added to the gateway or called directly | Cheap triage, summaries, classification | Low. Limits are set by the vendor and change, so read them before building numbers into the router |
| D. Browser automation | Playwright drives the web chat, with one saved browser profile per account | Chat-only free plans such as Perplexity | High and fragile. Anthropic's consumer terms bar unapproved automated access outside an API key |

The risk column is my judgment from the sources (see section 11), not a statement from any vendor.

**Update:** lane D (automating AI chat websites) is replaced by the **manual fallback**. When no API or CLI can reach a model, Conductor prints the rendered prompt, you paste it into any AI yourself, and you submit the answer back. Browser tools are used only for **web research** (search and read pages), which carries little risk; see `tools/web/`.

## 3. Linking multiple Gmail accounts

A Gmail account never connects to the top agent directly. Each vendor login does, and Conductor records which Gmail owns which login.

Pick identities first. An example layout:

| Identity | Used for | Logged in to |
|---|---|---|
| Main Gmail | Real mail and calendar only | No AI tools |
| Work-AI Gmail | Coding and research workers | Gemini CLI, Copilot CLI, Codex, Claude Code |

Keep your main Gmail out of lanes B and D. The reported Google suspensions hit the AI services on the account, and you do not want to find out whether they spread.

Then wire each account in five steps:

1. **One profile folder per account.** Use `~/.conductor/profiles/<identity>/<vendor>/`, readable only by you. Point the vendor CLI at it: `CLAUDE_CONFIG_DIR` for Claude Code, `CODEX_HOME` for Codex (it holds `auth.json`). For a CLI with no such variable, run it with `HOME` set to the profile folder, and check its docs for a dedicated setting.
2. **Log in by hand, once per profile.** Run the CLI's own login with that profile's variable set and finish the Google or GitHub browser sign-in yourself. Conductor only ever sees the folder path, never the password.
3. **Register it.** Add `identity` and `profile_dir` columns to the connections table already in your pitch. *(Done: see `src/store/store.cpp`.)*
4. **Health-check it.** Run one cheap headless call per profile and store a status: ready, paused (with reset time), needs-login or suspended.
5. **Launch workers through the profile.** The adapter sets the variable, spawns the CLI, and parses its JSON output.

Watch for a shell variable such as `ANTHROPIC_API_KEY` silently overriding a subscription login.

**Rotation rules:**

- The router picks by lane, task tag and budget, then takes the first account whose status is ready.
- A profile that hits its limit is paused until reset. The task moves to another worker or waits.
- Fail over across vendors, or across your own real identities. Do not fail over across look-alike accounts at the same vendor.
- Never create accounts automatically or work around phone or identity checks.

If you also want Conductor to read those inboxes for triage, that is a separate step: a Gmail API OAuth client in your own Google Cloud project, read-only scope first. It has nothing to do with logging in to the AIs.

## 4. Provider by provider

Every provider gets the same adapter shape: set the profile, spawn the CLI, parse its output, report usage. Only the command and the free-plan reality differ.

| Provider | Connect through | Free-plan reality | Watch out for |
|---|---|---|---|
| Claude Code | `claude -p` headless, with `CLAUDE_CONFIG_DIR` per account. Also your planner, driving Conductor over MCP | I believe the Free chat plan does not include Claude Code; confirm before counting on it | Subscription logins are allowed only in Claude Code and claude.ai. Billing for `claude -p` has been in flux: a May 2026 change was paused on 15 June, and Max and Team plans now get monthly API credits. Check Anthropic's help page before relying on it |
| Codex | `codex exec --json --ephemeral --cd <worktree>` with `CODEX_HOME` per account, or `CODEX_API_KEY` for API billing | ChatGPT login or API key. I did not confirm which free tiers include Codex | OpenClaw's docs say OpenAI supports subscription logins in external tools. I could not open OpenAI's own terms to confirm |
| GitHub Copilot | `copilot --acp --stdio` (ACP server, public preview and subject to change) | Free plan: 2,000 completions a month and Auto model only. The CLI is on every plan. GitHub's plan page currently notes that new sign-ups are temporarily paused | One free account per person |
| Gemini CLI and agy | `agy` (Antigravity CLI) for personal logins; `gemini -p` headless only with a paid Gemini API key. One `HOME` per profile | **Corrected:** the original draft had the direction reversed. Google's May 19, 2026 post says the Antigravity CLI (`agy`) *replaces* Gemini CLI, and legacy Gemini CLI stopped working for individual users on 18 June 2026. The old 1,000 requests/user/day Google-login quota belonged to Gemini CLI. Confirm `agy`'s quota before relying on any number | Third-party tools on Antigravity logins have drawn "403 ToS" suspensions. Run only the official program |
| Perplexity | API with prepaid credit (lane C), or the browser lane as a last resort | No free API. A Pro plan includes $5 of monthly API credit and the API is billed separately (third-party source) | Simplest fix: give the planner its own search tool and make Perplexity optional |
| OpenClaw | As a worker or chat front door through its own gateway, not as the base layer | Supports OpenAI and Codex subscription login and named auth profiles | Pointing it at Claude or Antigravity subscription logins is the route those vendors' terms ban |
| UI agents | Playwright with one saved browser profile per account. You sign in once by hand | The only way into chat-only free plans | Keep it read-only and low volume, never bypass captchas or verification, and expect it to break when a page changes |

ACP is a standard way to drive coding agents. Zed runs Claude, Codex, Gemini CLI and Copilot through it, each keeping its own login. It is worth testing as an alternative to parsing every CLI's output yourself.

---

## 5. Decisions

| Decision | Choice | Why |
|---|---|---|
| Core language | C++20, CMake | One language and one build system with the ML runtime |
| ML inference | ONNX Runtime C++ API, **pinned to 1.31.x** | Runs models trained anywhere (PyTorch, scikit-learn) and has a stable C++ API |
| ORT delivery | System-installed SDK found by `cmake/FindONNXRuntime.cmake`. No vendoring, no submodules | Keeps the repo small and lets the runtime be upgraded on its own |
| ML isolation | `src/ml/` behind `ml::LaneClassifier`. ORT headers never leave `src/ml` | The core builds and tests with `-DCONDUCTOR_WITH_ML=OFF` |
| Models | Shipped separately in `models/` (gitignored), with the contract in `models/README.md` | Model churn never touches the code history |
| Storage | SQLite, one file, WAL mode | Local-first, crash-safe task state |
| Account model | Registry, not rotation (section 3) | Keeps accounts in good standing |
| Start lanes | A (official CLIs) and C (real free API tiers). B and D are opt-in later | Lowest ban and breakage risk |
| Planner | Any AI (Claude, Codex, agy, other LLM) through the `conductor` CLI or MCP. Nothing assumes one vendor | The team uses different assistants; the loop must work with all of them |
| Dispatch | `dispatch_mode = router \| planner` (`src/core/dispatch.hpp`). In planner mode the named worker is used only if ready, never swapped | The planner keeps control without being able to bypass pauses or budgets |
| Brief contract | Goal, inputs, output format, done-criteria and review notes, rendered by one template (`core::render`) | Precise, small prompts are what make free models succeed |
| Workers | Free API tiers (OpenAI-compatible), official CLIs, and a manual copy-paste fallback | Free quota first, with a human as the last resort |
| Web access | A research sidecar in `tools/web/`. Fetched content is data, never instructions | Workers need knowledge; prompt injection must not become action |
| Repo hygiene | AI agent files (`.claude/`, `.codex/`, `.agy/`, `CLAUDE.md`, `AGENTS.md`, `agy.md`, ...) gitignored and blocked in CI | Per-developer tooling stays out of the product repo |

## 6. Repository layout

See [README](../README.md#layout). Each phase below says which folder it grows.

## 7. Build plan with exit criteria

Legend: [x] done and tested, [~] boilerplate (interface + stub) in place, [ ] not started.

### Phase 0: Spike
- [x] `CliWorker`: rendered brief on stdin, stdout to `result.md`, stderr to `stderr.log`, profile env vars, no shell (`src/adapters/cli_worker.cpp`)
- [ ] Run `claude -p --output-format json`, `codex exec --json` and `agy` through it with per-profile env
- [ ] Parse token and cost fields into `core::Usage`
- [ ] Timeout and kill the process group on overrun

**Exit:** `conductor spike` produces a result file and a usage record for two vendors.

### Phase 1: Core
- [x] Task state machine (`src/core/task.cpp`)
- [x] SQLite schema v1, versioned migrations, `Store` (`src/store`)
- [x] Deterministic router (`src/core/router.cpp`)
- [x] Dispatch modes: router and planner (`src/core/dispatch.cpp`)
- [x] Brief contract and prompt template (`src/core/brief.cpp`)
- [ ] Load `config/policy.toml` (see `config/policy.example.toml`) and version it in `policies`
- [ ] Health check per connection that writes `status` and `paused_until`
- [ ] Git worktree sandbox per coding run
- [ ] Usage ledger: `runs` and `usage_events` for every run

**Exit:** a task is dispatched in either mode, runs in a sandbox, is logged, and lands in Review.

### Phase 2: Planner API + MCP
- [~] `PlannerApi`: `create_task`, `list_tasks`, `assign`, `get_result`, `review` (`src/planner`)
- [~] CLI commands `assign`, `submit`, `review` (`src/app/main.cpp`, they print "not implemented yet")
- [~] MCP stdio server mapping the same five tools (`src/mcp`)
- [ ] `docs/planner-guide.md` tested with at least two different planners (for example Claude Code and Codex)

**Exit:** a planner AI creates, assigns and reviews tasks end to end without a human typing commands.

### Phase 3: Free workers
- [~] `HttpWorker` for OpenAI-compatible free API tiers (`src/adapters/http_worker.*`)
- [~] `ManualWorker` copy-paste fallback (`src/adapters/manual_worker.*`); how the prompt reaches the human is still to be decided
- [ ] Keychain lookup for `key_ref`
- [ ] Per-worker rate limits from the provider's published free-tier numbers

### Phase 4: Review loop
- [x] Review → Assigned transition exists for rejects (`core::can_transition`)
- [ ] Reject notes stored and fed into the next brief's `review_notes`
- [ ] `max_review_rounds`, then escalate to a human
- [ ] Final project review step

### Phase 5: Control surface
- [ ] TUI (FTXUI): connections, task board, run detail, approvals
- [ ] HTTP API + SSE; web UI later

### Phase 6: Web research tools and connectors
- [~] `tools/web/` placeholder with its rules
- [ ] Research sidecar: search, fetch, clean text, cite URLs
- [ ] Approval gate for every external write; Gmail (read-only first), Calendar, GitHub through official APIs

### Phase 1.5 (any time): ML lane classifier
- [x] `OnnxLaneClassifier` (`src/ml`), used only in router mode as a suggestion
- [ ] First model trained offline on labelled briefs

## 8. Security checklist

- Profile folders are `0700`. Conductor stores only their paths, never tokens or passwords.
- Scrub `ANTHROPIC_API_KEY`, `OPENAI_API_KEY` and similar variables from the worker environment unless the connection is an API-key connection.
- The brief goes in through stdin or a file, never on the command line (already enforced in `CliWorker`, with a test).
- Worker results are untrusted until the planner reviews them. A result can never trigger an external action by itself.
- The model file is untrusted input: validate the output shape (done) and load models only from `models/` or an explicit path.
- No standing write permissions until you grant them, one rule at a time.

## 9. Testing

- The core, store and adapter tests run in CI with ML off, so ONNX Runtime is not needed.
- A second CI job installs the pinned ORT SDK outside the repo and builds with ML on.
- ML behaviour tests need a model fixture. Add a small exported test model to CI artifacts when the first real model exists.

## 10. Open questions

1. Does the Claude Free plan include Claude Code headless use? (Section 4 is unconfirmed.)
2. Which Codex free tiers exist, and what do OpenAI's own terms say about subscription login in external tools?
3. Copilot ACP is in public preview. Should ACP replace per-CLI output parsing for coding workers?
4. Which platforms do we support beyond Linux? `CliWorker` is POSIX-only today, so Windows would need a `CreateProcess` adapter.
5. Training data for the lane classifier: where do labelled briefs come from before Conductor has history?
6. Manual fallback: how should the prompt reach the human (print, file, clipboard, dashboard) and how fast must the answer come back?
7. Which free API providers go into the first `HttpWorker` test? Read each one's current free-tier terms first.

## 11. Sources

The original draft's source list was missing. The links below were gathered on 2026-11-02 to back each claim. **Official** means the vendor's own page; the rest are forum reports or press coverage. Re-check them before relying on a number.

| Claim | Sources |
|---|---|
| Claude subscription logins are for Claude Code and claude.ai only | Official: [Claude Code legal and compliance](https://code.claude.com/docs/en/legal-and-compliance), [Anthropic consumer terms](https://www.anthropic.com/legal/consumer-terms). Coverage: [WinBuzzer, Feb 2026](https://winbuzzer.com/2026/02/19/anthropic-bans-claude-subscription-oauth-in-third-party-apps-xcxwbn/) |
| Google "403 ToS" suspensions for third-party tools on Antigravity / Gemini CLI logins | Forum reports (no official Google statement found): [mass 403 ToS bans thread](https://discuss.ai.google.dev/t/urgent-mass-403-tos-bans-on-gemini-api-antigravity-for-open-source-cli-users-paid-tier/124508), [suspended after using OpenClaw](https://discuss.ai.google.dev/t/account-disabled-403-tos-after-unknowingly-using-openclaw-new-subscriber-seeking-account-restoration/125862) |
| Gemini CLI → Antigravity CLI transition, 18 June 2026 | Official: [Google Developers Blog](https://developers.googleblog.com/an-important-update-transitioning-gemini-cli-to-antigravity-cli/) |
| Gemini CLI 1,000 requests/user/day (Google login) | Official: [Gemini CLI quotas and pricing](https://geminicli.com/docs/resources/quota-and-pricing/) |
| GitHub: one free personal account per person (plus one machine account) | Official: [GitHub Terms of Service](https://docs.github.com/en/site-policy/github-terms/github-terms-of-service) |
| Copilot Free: 2,000 completions a month | Coverage: [SD Times, launch](https://sdtimes.com/ai/github-copilot-free-launches-to-expand-reach-of-platform-to-all-developers/). Confirm on GitHub's current Copilot plans page |
| OmniRoute is MIT, self-hosted, with provider fallback | Maintainer: [diegosouzapw on GitHub](https://github.com/diegosouzapw). Guide: [Agentpedia OmniRoute guide](https://agentpedia.codes/blog/omniroute-ai-gateway-routing-setup-guide.md) |

**Not yet sourced.** I could not find a source for these claims; verify them or remove them:

- OmniRoute marks 12 providers "avoid" and leaves them out of automatic routing.
- The `claude -p` billing change in May 2026, paused on 15 June, and the monthly API credits for Max and Team plans.
- Perplexity Pro includes $5 of monthly API credit.
- OpenClaw's docs saying OpenAI allows subscription logins in external tools.
- Copilot CLI's `--acp --stdio` mode, and Zed running agents over ACP.
