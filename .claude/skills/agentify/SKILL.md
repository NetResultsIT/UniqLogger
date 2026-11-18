---
name: agentify
description: Gather and improve what agents must know about a repository - read the code and propose the project facts with their sources, ask the maintainer one question at a time only what the code cannot answer (consumers, decisions, plans), write AGENTS.md, the workflow skill and the missing architecture and naming documents, and repeat until the maintainer is satisfied. Use after update_project.py --init, when a fact is missing or not decided, or to improve the agent files.
---

# Agentify: gather and improve the project's knowledge

A shared skill: the same text in every project. It fills in, and later improves, the project-owned parts of the
agent files: the paragraph and the sections "Project facts", "Conventions" and "Skills" of `AGENTS.md`, the
`<repo>-dev-workflow` skill, the architecture document, the naming rules and the decisions in
`.agents/adoption.md`. It never edits the core rules block, the overrides or a shared skill (core rule `gate.rules`).

The repository is the source of what it can show. The maintainer is the source of what it cannot: who uses the
project, what is decided, what is planned. A fact that comes from neither is not written.

Reference files: `questions.md`, the questions by topic, with where to look first and where the answer goes (read
it before phase 1); `documents.md`, the skeletons of the architecture document, the naming rules and a design note
(read it before writing one).

## 1. Modes

- **Adopt**: the files were just made by `update_project.py --init` (placeholders `<...>` left), or "Project facts"
  is mostly empty. All phases, all topics of `questions.md`.
- **Improve**: the files exist. Start from what is known to be missing, in this order: what the maintainer names;
  the failing and open items of `sync_skills.py --doctor` (in the scripts folder named in `AGENTS.md`), deferred
  ones whose date has passed first; every "missing", "not decided" and "(not verified)" in `AGENTS.md` and the
  workflow skill; every line of "Project facts" in `questions.md` that `AGENTS.md` lacks. Then check the existing
  facts against the repository: a fact the code contradicts is a finding, shown in phase 2. Then phases 2 to 5 for
  those topics only.
- **One topic**: the maintainer names one ("consumers", "naming", "tests"): phases 1 to 5 for that topic.

During other work, an agent that needs a fact only the maintainer knows asks one question as in phase 3, records
the answer as in phase 4, and proposes this skill when more of the same topic is missing.

## 2. Phase 1: discover, without questions

1. Ask once who you are talking to (name, role): it is written next to the facts only they could state.
2. Read the README and the documentation, the build files and dependency lists, the CI configuration, the test
   folders, the branches and tags (`git branch -r`, `git tag`), the license files, `.gitattributes`, and the agent
   files already there in any format or language (they are kept and pointed to, not copied). For a large codebase a
   code-map or knowledge-graph tool the user already has can help with orientation; check what it says in the code.
3. For each question of `questions.md`, prepare an answer with its source: **found** (the file and line, or the
   command and its output), **inferred** (from what, and why it may be wrong) or **unknown**.
4. Do not run builds, tests or anything that writes outside your worktree without asking: they can be slow, need a
   service, or touch what another checkout uses.

## 3. Phase 2: confirm what was found

Show the found and inferred answers as the lines that would be written, each with its source, one topic at a time
and no more than about ten lines at once. Ask the maintainer to correct them. When the maintainer and the
repository disagree, show both and let the maintainer decide (core rule `truth`); write down what the repository
says too when it is the code that must change.

## 4. Phase 3: ask what is unknown

One question at a time; wait for the answer before the next one.
- Never ask what the repository answers: look first.
- Say what the answer decides and where it will be written.
- Give your recommended answer with the reason, and the alternatives with what each costs. In a new repository
  the recommendation comes from the profile skills installed here (for example `qt-naming`) or from common practice
  for the stack: say which.
- Follow up until the answer is one line an agent can act on and a reviewer can check: "the mobile app" becomes
  "the mobile app (repository `shop-app`), through the REST endpoints under `/api/v2`, follows the main branch".
- Accept "I don't know", "not decided" and "later". Write the fact as `not decided: <the question>`, a document as
  "missing", a checklist item as `[>]` with a date. Do not choose for the maintainer.
- A question that opens a design decision larger than writing down what exists (splitting a module, replacing the
  build system) is not settled here: write it as not decided and go on.
- Use the agent tool's question prompt when it has one. Say where you are ("Consumers, question 3 of about 6") so
  the maintainer can stop and continue later.

## 5. Phase 4: write

Show the lines you will add or change in each file; write them after the maintainer agrees.
- `AGENTS.md`, project part: the paragraph; one line per fact with a pointer to where it can be checked; "none"
  when there is no such thing. A fact only the maintainer could state ends with `(stated by <name>, <YYYY-MM-DD>)`.
  Target 60 lines, limit 100 before the core rules block: put the detail in a document and point to it.
- `<repo>-dev-workflow`: the sections the file already has (orientation, tests, worker brief, review items, design
  rules, the documents for the knowledge table), only what is specific here; never repeat `dev-workflow`.
- A document the maintainer chose to write now (architecture, naming rules): from `documents.md`, at the path they
  choose; then name it in "Project facts".
- `.agents/adoption.md`: change an item only on the maintainer's word: `[x]` done, `[-]` with the reason, `[>]` with
  the date; the note says what was decided. When the file is missing, run `sync_skills.py --doctor --write` first.
- Writing rules: English (documents in other languages are pointed to as they are); plain sentences; about 120
  columns, no line over 200; every command, path and name checked in the repository, or marked "(not verified)";
  no secret, token or personal path; in a public repository no internal host name or URL. Keep line endings.
- Then run `sync_skills.py` (refreshes `.claude/skills`), `sync_skills.py --check` and `sync_skills.py --doctor`.

## 6. Phase 5: review until the maintainer is satisfied

Show what was written (files and sections), the facts left "not decided" or "missing", the doctor output and what
you could not check. Then ask, one at a time:
1. Is anything wrong?
2. Is anything missing, or too vague for an agent to act on?
3. The open questions of `questions.md`, "Every run".

Each answer goes back to phase 3 (a follow-up question) or phase 4 (a change); then repeat this round. Stop only
when the maintainer says they are satisfied or want to stop for now; never declare it finished yourself. When they
stop for now, list what remains open and where it is recorded, and say that this skill continues from there. When
the maintainer has checked "Project facts" line by line, propose `[x]` for `facts.reviewed` with their name and
the date.

## 7. Report

The files created and changed; the facts taken from the repository, with their sources; the facts stated by the
maintainer; what is not decided or missing; what you could not verify; the output of `--check` and `--doctor`.
Do not commit or push unless asked (core rule `commit`); the reviewers of the merge request check the facts again.

A question `questions.md` lacked, or a trap that other repositories of the same language will hit: propose it for
the `agent-skills` repository (core rule `knowledge`), it is not written into this shared skill here.
