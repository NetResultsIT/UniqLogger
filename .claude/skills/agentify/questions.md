# Agentify: the questions

For each question: what to look at before asking ("Look"), and what to recommend when nothing is decided
("Recommend"). Where the answer is written is the heading of each section. Ask only what the repository does not
answer; skip a question whose answer is plainly "none" and say so in phase 2.

## Project: the paragraph at the top of `AGENTS.md`

- What the project is, and who uses it (people, not code: that is "Consumers"). Look: the README, the package
  description.
- Language and framework versions. Look: the build files.
- The branch model: where merge requests go, maintenance branches, how releases are tagged. Look: `git branch -r`,
  `git tag`, the CI configuration.

## "Project facts" of `AGENTS.md`, one line each

- Language level and oldest supported toolchain or runtime. Look: `CMAKE_CXX_STANDARD` and the CI matrix;
  `composer.json` (`require.php`); `package.json` (`engines`), `.nvmrc`, `tsconfig.json`; `pyproject.toml`
  (`requires-python`). Recommend: the oldest version CI builds with today.
- Public API and other contracts: what other code or systems rely on. Look: public headers and exported symbols,
  HTTP routes and their JSON, the database schema and migrations, message and file formats, command line options
  used by scripts, base classes others derive from.
- Consumers: the other projects, applications and libraries that use this repository, each with the contract it
  uses, the version it takes (a release, or the main branch) and who to tell before a breaking change. Look: other
  repositories the maintainer names, documentation that mentions clients, the package name in other projects'
  dependency lists. Mostly asked; more than three: a document (`docs/consumers.md`) and a pointer. Recommend: ask
  for each contract of the line above who uses it. Never write "none" because you found no consumer: "none" is
  the maintainer's statement, written `none (stated by <name>, <YYYY-MM-DD>)`, and the doctor checks it.
- Versioning: where the version is defined, the rule, whether a major version is released. Look: the build files,
  tags, the changelog. Recommend: semantic versioning for a library (skill `semver-abi` in C++); "none (deployed,
  not versioned)" for a service deployed from the main branch.
- Concurrency model: threads, processes, queue workers, cron jobs, event loops, the lock order. Look: thread and
  worker classes, queue configuration, crontabs.
- Security-sensitive code: authentication, permissions, cryptography, secrets handling, where personal data is
  stored, logged or exported. Look: the security configuration, the folders with those names.
- Build files and dependency lists. Look: the files at the root and in each module.
- Build output and dependency folders; whether two worktrees can build and run side by side (shared ports,
  databases, caches, fixed paths). Look: `.gitignore`, the build scripts. Ask about side by side.
- Line endings. Look: `.gitattributes`, `git ls-files --eol`.
- Outside the repository: package and artifact servers, deployments, CI settings, shared databases. Mostly asked.
- License texts, the license report command, the approved licenses. Look: `LICENSE*`, a third-party folder, the
  package manager's license command (skill `third-party`).
- Architecture document and design notes. Look: `docs/`, the README. When missing, ask: write it now (skeleton in
  `documents.md`, questions below), plan it with a date (`docs.architecture` deferred), or not needed (a small
  project: say why).
- Naming rules and their check command. Look: a profile skill installed here (`qt-naming` and its level), the
  formatter and linter configuration (`.clang-format`, `.php-cs-fixer*`, `phpcs.xml`, `.eslintrc*`). When missing,
  ask as for the architecture document.
- Tests and coverage: the command, and the coverage command or "not measured". Look: the test folders, the CI
  configuration. Offer to run the test command; run it only after a yes.
- Documents updated with the code: changelog, migration notes, porting guide, API documentation. Look: the root and
  `docs/`.

## "Conventions" of `AGENTS.md`

- How errors are reported (exceptions, error codes, result types) and what a caller must do.
- Logging: the logger, levels, what must never be logged.
- Generated files that are never edited by hand, and the command that generates them.
- Anything else every change here must respect. Keep each to one line with a pointer.

## The workflow skill `<repo>-dev-workflow`

- Orientation: the main modules, classes or services and what each does; where to start reading for each kind of
  change. Look: the folder tree, build targets, entry points. Ask which parts are fragile, legacy or being replaced.
- Tests here: which program or suite covers what, how a new one is added, the cases this project always needs, what
  needs a live server, a device or a person.
- Additions to the worker brief: files not to touch, build variants to try, the baseline to compare with.
- Review items specific to this repository.
- Design rules with the repository's own names: classes or modules already too big, collaborators to extend
  instead, helpers to reuse, the lifecycle of stateful resources.
- The documents for the rows of the knowledge table: design notes folder, migration notes, the list of open checks.
- Building or running the tests needs more than ten lines: propose a `<repo>-build` or `<repo>-run-tests` skill.

## Designing what is not decided

When the architecture or the naming rules are missing and the maintainer wants them now. One question at a time,
each with a recommendation; the answers fill the skeletons of `documents.md`.
- Architecture: the components and the responsibility of each; who may depend on whom (and what must never depend
  on what); the data, where it is stored and its format; processes and threads; the external systems; how errors
  travel and are reported; configuration; what is deployed as one unit; what is expected to change soon.
- Naming: the common style of the language it follows, and only the differences; prefixes of public types; file
  and folder names; test names; the check command; what happens to existing code that does not follow (renamed
  only when touched, never in a mass rename, unless the maintainer decides otherwise).

## `.agents/adoption.md`

- `ci.check`: does CI run `sync_skills.py --check`? Without CI: `[-]` with the reason.
- `hook.documented`: does the README say how to enable the hook in a new clone?
- `tests.command`, `tests.coverage`, `licenses.folder`: from the answers above.
- `tools.agents`: which agent tools are used here, and does the row of each in the core rules table work (a
  subagent definition exists where the tool needs one)?
- `eval.prompts`: the three behaviour prompts (raise the language level; add a library; relax a core rule) must be
  run by the maintainer in a fresh session of each tool, not judged by you. Offer the prompts; record the date and
  the result they report.

## Profiles

- C++ and Qt: library or application (a library: its public headers are a contract and skill `semver-abi`
  applies); the prefix of its public types; the `qt-naming` integer level (relaxed by default); the oldest Qt
  version and compilers; the platforms built and those tested here; which dynamic checks run (skill
  `cpp-diagnostics`).
- PHP and Symfony: the PHP and Symfony versions; Doctrine migrations; queue workers and cron jobs; the style checker
  and static analysis commands; legacy code that must not be reformatted.
- Web: the Node version, JavaScript or TypeScript, the target browsers; the bundler; the test runner; what reaches
  the bundle through environment variables.
- Another language: ask what an agent gets wrong in it; when two repositories would write the same answers,
  propose a profile to `agent-skills`.

## Every run

Ask these at the end of phase 5, whatever the mode:
- What does a new colleague get wrong in their first month here?
- What must an agent never do here that these files do not say?
- Which part of the code would you not let a newcomer change alone, and why?
