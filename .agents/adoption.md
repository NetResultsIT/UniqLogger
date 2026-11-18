# Adoption checklist

The items a person decides; `sync_skills.py --doctor` checks the others in each clone. Mark `[x]` done,
`[-]` not applicable, `[>]` deferred, `[ ]` open; write the reason or the date after "Note:".

- [ ] `ci.check` CI runs sync_skills.py --check on every merge request (the hook is opt-in per clone); not applicable only without CI, with the reason.
- [x] `hook.documented` the README or AGENTS.md says how to enable the hook in a new clone. Note: AGENTS.md, section "Skills", gives git config core.hooksPath .githooks.
- [ ] `facts.reviewed` a maintainer checked every line of "Project facts" against the repository.
- [ ] `docs.architecture` an architecture document exists, or writing one is planned (with a date).
- [ ] `docs.naming` naming rules exist (a document or a skill) with a check command, or are planned.
- [ ] `tests.command` the test command of "Project facts" runs on a clean clone.
- [ ] `tests.coverage` coverage is measured, or "not measured" is a recorded decision.
- [ ] `licenses.folder` the license texts of all components are where "Project facts" says.
- [ ] `tools.agents` the agent tools used here are known, and their row of the core rules table works (a subagent definition exists where the tool needs one).
- [ ] `eval.prompts` the three behaviour prompts (language level, new component, relaxing a core rule) were run in each tool used, with the date and the result.
