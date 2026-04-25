# stb_truetype-hardened

This project aims to harden `stb_truetype` so it can be safely used with **untrusted font inputs**.

The process is designed to be **agent-driven**, reproducible, and auditable.

---

# Goals

- Eliminate memory safety issues (OOB reads/writes, UAF, integer overflows)
- Improve robustness against malformed or malicious fonts
- Maintain API compatibility with upstream `stb_truetype`
- Minimize performance regressions

---

# Threat Model

Assume:

- Fonts are fully untrusted
- Attackers aim to trigger memory corruption, crashes, or excessive resource usage

Out of scope:

- Side-channel attacks
- Rendering correctness unless tied to safety

---

# Techniques

Agents may use one of the following approaches:

- Web Research
- Fuzzing
- Static Analysis
- Runtime Analysis
- Unit Testing

---

# Workflow

All work MUST follow this pipeline:

1. **Find Bug**
2. **Report Bug (BUGS.md)**
3. **Validate Bug**
4. **Fix Bug**
5. **Document Fix (FIXES.md)**

Agents MUST NOT skip steps.

---

# Agent Rules

## General

- Never modify `stb_truetype.h` without completing the full workflow
- Always produce deterministic and reproducible outputs
- Prefer minimal, localized fixes
- Do not change public APIs unless explicitly approved

---

## Bug Discovery

- The user MUST specify the approach:
  - `web`, `fuzz`, or `static`
- If not specified → ASK for clarification and STOP

Output MUST include:

- A clear description of the suspected issue
- Affected function(s)
- Hypothesis of root cause

## Web Research

When the selected approach is `web`, the agent MUST use the following sources:

1. **GitHub Issues**
   - Repository: https://github.com/nothings/stb
   - Focus on:
     - Issues related to `stb_truetype`
     - Unresolved or partially fixed bugs
     - Security discussions

2. **CVE Databases**
   - https://nvd.nist.gov/
   - Search for:
     - `stb_truetype`
     - `stb`
     - Font parsing vulnerabilities

3. **General Web Search**
   - Blog posts
   - Security writeups
   - Fuzzing reports
   - Discussions on font parsing bugs

## Bug Reporting (BUGS.md)

Each bug MUST follow this format:

```md
## [BUG-ID]

- **Title**:
- **Location**:
- **Type**: (OOB read, integer overflow, etc.)
- **Description**:
- **Impact**:
- **Reproduction idea**:
- **Status**: unvalidated
```

## Validation Phase

When the user says "proceed with validation":
You MUST:

1. Create a test directory:

```
tests/<BUG-ID>/
```

2. Add:

- `test.c` (Unity-based test)
- `run.sh` (build + execute)
- Malicious font generator or sample

3. Test MUST:

- Trigger the bug reliably
- Fail before the fix

4. Execution MUST include:

- Normal run
- `valgrind` run

Expected result:

- Crash, assertion failure, or valgrind error

Update BUGS.md:

```md
- **Status**: validated
```

## Fix Phase

When the user says "proceed with fix":
You MUST:

- Apply the smallest possible fix
- Avoid:
  - Large rewrites
  - Style-only changes
  - Unrelated modifications

Fix should:

- Eliminate the bug
- Preserve behavior for valid inputs

## Post-Fix Validation

- Re-run the same test
- Confirm:
  - No crash
  - No valgrind errors
  - Test passes

## Fix Documentation (FIXES.md)

Each fix MUST include:

```md
## [BUG-ID]

- **Summary**:
- **Root Cause**:
- **Fix Description**:
- **Changed Code**:
- **Regression Risk**:
```

## Cleanup

- Remove bug from `BUGS.md`
- Ensure test remains (regression test)

## Test Execution

- Per-bug execution: run `tests/<BUG-ID>/run.sh`
- Aggregate execution: run `tests/run-all.sh` from repository root
- Any final validation claim MUST be supported by passing `tests/run-all.sh`

## Testing Requirements

- Use Unity framework
- Each bug has isolated test directory
- Tests MUST be reproducible
- Tests MUST not depend on external resources
- `tests/run-all.sh` MUST discover and execute all `tests/<BUG-ID>/run.sh` scripts

## Generated Artifact Hygiene

When adding a new test under `tests/<BUG-ID>/`, the agent MUST identify generated artifacts and add them to `.gitignore` in the same change.

At minimum, ignore:
- Generated fonts/samples (e.g. `*.ttf`, `*.otf` if generated)
- Built test binaries (e.g. `test_web_*`)
- Crash dumps/core files (e.g. `core*`, `vgcore.*`)

Do not rely on manual cleanup alone; generated outputs must be ignored via `.gitignore`.

## Safety Guidelines

Focus on:

- Bounds checking
- Integer overflow/underflow
- Pointer arithmetic
- Invalid offsets in font tables
- Infinite loops / excessive allocations

## Non-Goals

- Refactoring entire library
- Rewriting in another language
- Adding new features

## Definition of Done

A bug is considered fixed when:

- It is reproducible before the fix
- It is no longer reproducible after the fix
- `valgrind` reports no errors
- A regression test exists
- FIXES.md entry is complete
