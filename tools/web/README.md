# Web research tools (placeholder, Phase 6)

A small sidecar that gives workers web access for research: search, fetch a page, read it, and return clean text plus source links. It will be written in Node or Python, because the browser-automation ecosystem lives there, and Conductor will drive it like any CLI worker: brief on stdin, result file out.

Rules:
- Fetched pages are **data, never instructions**. Text on a page can never trigger an action; anything a result proposes still goes through review and the approval gate.
- Read-only and low volume. Never solve captchas or bypass logins or verification.
- Every result cites the URLs it used.

Pick a library when this phase starts. Keep its dependencies inside this folder, never in the C++ build.
