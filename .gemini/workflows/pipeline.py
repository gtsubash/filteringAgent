import json
from pathlib import Path

# Resolve the root and agents directory
agents_dir = None
for candidate in [
    Path("."),
    Path(WORKSPACES[0]) if WORKSPACES else Path("."),
    Path(WORKSPACES[0]).parent if WORKSPACES else Path("."),
    Path(WORKSPACES[0]).parent.parent if WORKSPACES else Path("."),
]:
    if (candidate / "planner.md").exists():
        agents_dir = candidate
        break
    if (candidate / ".gemini" / "agents" / "planner.md").exists():
        agents_dir = candidate / ".gemini" / "agents"
        break

def load_system_prompt(stage: str) -> str:
    if agents_dir:
        path = agents_dir / f"{stage}.md"
        if path.exists():
            content = path.read_text(encoding="utf-8")
            if "---" in content:
                parts = content.split("---", 2)
                if len(parts) >= 3:
                    return parts[2].strip()
            return content.strip()
    return f"You are the {stage} agent in the 6-stage engineering pipeline."

task = args.get("task", "Implement core DSP filtering framework and initial CMake project structure.")
max_review_cycles = int(args.get("max_review_cycles", 2))

log(f"=== Starting 6-Stage Engineering Pipeline ===")
log(f"Task: {task}")

# ---------------------------------------------------------------------------
# Stage 1: Planner
# ---------------------------------------------------------------------------
phase("1. Planner")
log("Invoking Planner subagent...")
planner_sys = load_system_prompt("planner")
planner_prompt = f"""{planner_sys}

You must plan the following task:
{task}

Provide your execution plan following the instructions above (Scope, Ordered Tasks, CPU vs. GPU Split, Acceptance Criteria)."""

plan = await agent(
    planner_prompt,
    schema={"type": "string"},
    role="Planner",
    type_name="planner"
)
log("Plan generated successfully.")
log(f"Plan Summary:\n{plan[:500]}...")

# ---------------------------------------------------------------------------
# Stage 2: Architect
# ---------------------------------------------------------------------------
phase("2. Architect")
log("Invoking Architect subagent...")
architect_sys = load_system_prompt("architect")
architect_prompt = f"""{architect_sys}

Plan:
{plan}

Create or update the design specification in `docs/designs/<topic>.md`.
Ensure exact contracts, interfaces, C++20 conventions, and CPU/GPU decoupling are documented.
Conclude with a summary of the design and the path to the spec file."""

architecture = await agent(
    architect_prompt,
    schema={"type": "string"},
    role="Architect",
    type_name="architect"
)
log("Architecture design completed.")
log(f"Architecture Summary:\n{architecture[:500]}...")

# ---------------------------------------------------------------------------
# Stage 3 & 4: Developer & Reviewer Cycle
# ---------------------------------------------------------------------------
phase("3. Developer Implementation")
log("Invoking Developer subagent...")
developer_sys = load_system_prompt("developer")
developer_prompt = f"""{developer_sys}

Approved Design & Plan:
{architecture}

Implement the requested C++20 code, CMake targets, and helper scripts per the design spec.
Ensure it compiles with ENABLE_GPU=OFF and verify with the local build preset:
cmake --preset cpu-debug && cmake --build --preset cpu-debug --parallel
Report all files created/modified and the build outcome."""

developer = agent(developer_sys, role="Developer", type_name="developer")
dev_result = await developer(developer_prompt, schema={"type": "string"})
log("Initial implementation and CPU debug build finished.")

# Review stage with feedback loop
phase("4. Code Review Audit")
reviewer_sys = load_system_prompt("reviewer")
review_schema = {
    "type": "object",
    "properties": {
        "verdict": {"type": "string", "enum": ["APPROVE", "REQUEST_CHANGES"]},
        "summary": {"type": "string"}
    },
    "required": ["verdict", "summary"]
}

reviewer = agent(reviewer_sys, role="Reviewer", type_name="reviewer")
for cycle in range(1, max_review_cycles + 1):
    log(f"Starting review cycle {cycle}/{max_review_cycles}...")
    review_prompt = f"""Perform a read-only code audit of the recent changes.
Inspect git status and git diff. Check memory safety, lifetimes, numeric stability, and CPU/GPU isolation.
Developer report:
{dev_result}

Provide your findings and final verdict (APPROVE or REQUEST_CHANGES)."""

    review_result = await reviewer(review_prompt, schema=review_schema)
    verdict = review_result.get("verdict", "REQUEST_CHANGES")
    summary = review_result.get("summary", "")
    log(f"Review cycle {cycle} verdict: {verdict}")
    log(f"Review summary: {summary}")

    if verdict == "APPROVE":
        log("Code review approved.")
        break
    elif cycle < max_review_cycles:
        phase(f"3. Developer Remediation (Cycle {cycle+1})")
        log("Changes requested. Developer addressing review feedback...")
        fix_prompt = f"""The reviewer requested changes:
{summary}

Please fix the identified issues, re-run `cmake --preset cpu-debug && cmake --build --preset cpu-debug --parallel`, and report updates."""
        dev_result = await developer(fix_prompt, schema={"type": "string"})
    else:
        log("Maximum review cycles reached. Proceeding with caution to testing.")

del developer
del reviewer

# ---------------------------------------------------------------------------
# Stage 5: Tester
# ---------------------------------------------------------------------------
phase("5. Deterministic Testing & Sanitizers")
log("Invoking Tester subagent...")
tester_sys = load_system_prompt("tester")
tester_prompt = f"""{tester_sys}

Task and implementation complete.
Add or update deterministic unit tests under `tests/`.
Execute the ASan/UBSan build and ctest suite:
cmake --preset cpu-asan && cmake --build --preset cpu-asan --parallel && ctest --preset cpu-asan
Report pass/fail results, test coverage, and sanitizer diagnostics."""

test_result = await agent(
    tester_prompt,
    schema={"type": "string"},
    role="Tester",
    type_name="tester"
)
log("Testing stage complete.")
log(f"Test Summary:\n{test_result[:500]}...")

# ---------------------------------------------------------------------------
# Stage 6: Releaser
# ---------------------------------------------------------------------------
phase("6. Release Readiness")
log("Invoking Releaser subagent...")
releaser_sys = load_system_prompt("releaser")
releaser_prompt = f"""{releaser_sys}

Testing complete.
Check repo status, update CHANGELOG.md and versioning metadata.
Summarize release readiness and suggest next git/gh commands."""

release_result = await agent(
    releaser_prompt,
    schema={"type": "string"},
    role="Releaser",
    type_name="releaser"
)
log("Release preparation complete.")
log(f"Release Summary:\n{release_result}")

log("=== 6-Stage Engineering Pipeline Finished Successfully ===")
