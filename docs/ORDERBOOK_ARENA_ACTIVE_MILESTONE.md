# Active Milestone: A1 Product and Documentation Alignment

**Status:** Review complete, awaiting approval

## Goal

Align the planning documents around one product direction:

- a LeetCode-style website for trading and market-structure challenges;
- Python and C++ as the strategy languages;
- the existing C++ matching engine as the judge;
- local Python and C++ runners as an early development bridge;
- hosted sandboxed execution as the long-term Run/Submit path.

This milestone is documentation-only. It does not add runners, website code, or
server-side execution.

## Tasks

- [x] Update the Arena overview with the final and early-MVP user flows.
- [x] Separate local and hosted execution paths in the architecture.
- [x] State that JavaScript or TypeScript is frontend technology only.
- [x] Explain why hosted Python should be investigated before hosted C++.
- [x] Reorder the roadmap and milestone queue around both target languages.
- [x] Document sandbox requirements without claiming they are implemented.
- [x] Align the README with the same scope and limitations.
- [x] Review the documentation diff against the acceptance criteria.
- [ ] Obtain user approval for the documentation diff.
- [ ] Commit the approved documentation changes.

## Acceptance Criteria

- every Arena planning document names Python and C++ as target strategy
  languages;
- the website is consistently described as the main product surface;
- the local runner is consistently described as temporary;
- local result files are not treated as trusted leaderboard submissions;
- hosted execution is described as unimplemented and security-sensitive;
- the first implementation milestones contain no hosted execution work;
- no source code changes are included in this milestone.

## Checks

- inspect the six requested documents for contradictions;
- search for stale local-first or Python-only strategy language;
- run `git diff --check`;
- confirm only documentation and README files changed;
- review `git status` before committing.

## Exit Condition

The user approves the revised plan and the documentation is committed. The next
milestone is A2, Challenge Definition Schema.
