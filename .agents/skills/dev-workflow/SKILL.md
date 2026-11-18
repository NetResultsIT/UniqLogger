---
name: dev-workflow
description: The procedure for any non trivial change (feature, bug fix, refactoring) with a lead model and a worker - plan with tests designed first, the plan challenge by a second model, the stop-and-ask gate, the brief for the worker, the review checklist, the report (verified, not verified, assumed), where new knowledge is written down, and what to do after the merge. Use before planning, implementing or reviewing a change; the project's own workflow skill adds its specific steps.
---

# Development workflow

A shared skill: the same text in every project. The project's workflow skill (`<project>-dev-workflow`) adds what
is specific to it: where to look in the code, how its tests are designed, its design rules, its review items, the
names of its documents. It may add steps or make one stricter; a step is relaxed only through "Overrides of the
core rules" in `AGENTS.md`.

## 1. Roles

- **Lead**: the most capable model available. It reads the code, designs the tests, writes the plan, decides at the
  gate, briefs the worker, reviews the result and reports to the user.
- **Worker**: a subagent on a lighter, faster model. It implements one plan exactly, builds, runs the tests and
  reports. It does not widen the scope and does not commit.
- **Challenger**: a second high-capacity model, preferably from another provider, that attacks the plan (phase 1).
- **Web searcher**: the lightest model available. It returns URLs and quoted passages; it does not decide.

A change of a few lines that the lead can verify alone needs no worker. Which model plays which role in each agent
tool is in the table of the core rules in `AGENTS.md`.

## 2. Phase 1: plan (lead)

1. **Who else works here.** List the worktrees and the running builds and tests (`git worktree list`,
   `git status`). Work in your own worktree; do not touch the build output of another checkout.
2. **Read** the code involved and its callers, the architecture document and the design notes of the area. When
   there is none, say so in the plan.
3. **Reuse.** For anything not specific to the project, check the project, its dependencies and existing
   third-party components (skill `third-party`). Put what you found in the plan.
4. **Design the tests first.** Which test program or suite, which cases:
   - the normal path and the boundary values;
   - wrong arguments, wrong state, and the error each must give;
   - concurrency where the code is used from more than one thread, process or request;
   - cleanup: no leaked resource, timer, file, connection or background job.
   A test must show the effect (the message sent, the row written, the event emitted), not only that a getter
   returns what a setter stored. A bug fix starts with the test that reproduces it. Without a test the change is
   not done; going without one needs the user's explicit request, recorded in the commit message.
5. **Dynamic checks.** Decide which sanitizer, leak check, profiler or load test the change needs (for C and C++:
   skill `cpp-diagnostics`) and plan when it runs.
6. **Design note.** A new component, protocol, data format or thread gets a short design note before the code: the
   problem, the options considered, the choice and why, what changes for the users of the code.
7. **Write the plan**: the files and functions to change, the tests, the checks, the risks, the gate items it
   touches, what is out of scope.
8. **Challenge the plan** when the task is complex: more than one component, a gate item, a concurrency or data
   format change. Propose it to the user first, then give the challenger the plan and
   pointers to the code, not your reasoning, and ask it to find what is wrong, missing or untestable. For each
   objection: accept it and change the plan, or reject it with the reason. Show the user the objections and what
   you did with each.

## 3. Phase 2: gate (lead)

When the plan touches an item of "Stop and ask first" in `AGENTS.md`, present it and stop until the user answers:
what changes, why, the alternatives, which files, which tests, who is affected, the version number you would set.
Stop too when the requirements are ambiguous: ask, do not choose for the user.

## 4. Phase 3: implementation (worker)

The brief is complete, so the worker does not research the repository again:
1. the worktree and the branch; "do not commit, do not push";
2. the tests to write first, with the case list;
3. the production change: files, functions, what to change, in words;
4. the files not to touch (generated files, build files, third-party code unless the plan says so);
5. the line endings of the files touched, and "keep the existing comments";
6. the exact build and test commands, and the baseline: what the tests give before the change;
7. what to report: the files changed, every deviation from the brief and why, the test results with their summary
   lines, what could not be built or run, anything surprising.

## 5. Phase 4: review (lead)

Read the whole diff, rerun the tests yourself, and check:
- [ ] the change does what was asked, nothing more; the plan's out-of-scope items are untouched;
- [ ] the tests were written for the change and show its effect; they fail without it (check at least one);
- [ ] error paths and cleanup are tested;
- [ ] the dynamic checks of the plan ran, or the report says why not;
- [ ] gate items: untouched, or approved by the user;
- [ ] names follow the project's naming rules, and its check script adds no findings;
- [ ] line endings, comments and documentation of the touched code are kept or updated;
- [ ] documents that describe the changed behaviour (architecture, design note, API migration notes, README) are
      updated in the same change;
- [ ] a new third-party component has its license text and the user's acknowledgement;
- [ ] nothing machine-specific, no secret, no internal host name where the project forbids it;
- [ ] the project's own review items.

## 6. The report

To the user, after phase 4: what changed (files, behaviour), the tests and checks run with their results, and three
lists: **verified** (and how), **not verified** (and why: platform not available, needs hardware, needs a person),
**assumed**. Then the open decisions and what you propose next. No claim without the evidence behind it.

## 7. Where new knowledge goes

Write down what you learn while working, in the same change that produced it, in one place:

| What was learned | Where it is written |
|---|---|
| A rule for every change in this project | `AGENTS.md`, "Conventions", one line with a pointer |
| What only the maintainer knew (a consumer, a decision) | `AGENTS.md`, "Project facts"; for more, skill `agentify` |
| A procedure, or a step that was missing or wrong | the `SKILL.md` of that skill, corrected in place |
| A dated result, a baseline, what one machine did | a reference file of the skill |
| A trap that applies to other projects too | the `agent-skills` repository (a merge request or a note in yours) |
| Why the code is the way it is | the architecture document or a design note; a comment when it is local |
| An open problem, something not verified | the project's list of open checks, not a skill |
| One person's machine or preferences | not in the repository |

- One place per fact; everything else points to it. Correct a wrong statement where it stands.
- A worker reports what it learned; the lead decides where it goes and reviews the edit like code.
- Shared skills, the core rules block and `sync_skills.py` are copies pinned in `.agents/shared-skills.lock`: the
  check fails if they are edited in the project. Change them in `agent-skills`.

## 8. After the merge

Check that the branch is merged, then suggest to the user (never do it unasked) what can be deleted: the worktree,
the local and remote branch, backup branches, scratch build folders. List the full paths and wait for the yes.
Never remove a worktree or a branch another session uses.
