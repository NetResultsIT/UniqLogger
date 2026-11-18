#!/usr/bin/env python3
"""Keep the two copies of the agent skills identical.

The skills are edited in .agents/skills (the source). .claude/skills is a copy of it, kept as real files
because git symbolic links do not work on every platform.

    python3 <this script>            copy .agents/skills over .claude/skills
    python3 <this script> --check    exit 1 and list the differences, change nothing
    python3 <this script> --fix      restore the core rules block of AGENTS.md (needs "core": true in the lock file)
    python3 <this script> --doctor [--write]   print the adoption checklist of this clone; --write updates the
                                     items a person decides in .agents/adoption.md

With --check, when .agents/shared-skills.lock exists, it also reports shared skills (copied from agent-skills)
that were edited, removed or extended in this project, and a copy of this script that is not the shared one.
When the lock file has "core": true, --check also reports a core rules block of AGENTS.md that is missing, damaged
or different from the shared text, and a missing "## Overrides of the core rules" section after it. --fix rewrites
only the lines between the two markers of the block (exit 1 and no change when the markers are not usable).
"""
import argparse
import filecmp
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

SOURCE = Path(".agents/skills")
COPY = Path(".claude/skills")
LOCK = Path(".agents/shared-skills.lock")
AGENTS = Path("AGENTS.md")
CORE_BEGIN = ("<!-- agent-skills:core begin: written by agent-skills, do not edit here "
              "(repair: sync_skills.py --fix) -->")
CORE_END = "<!-- agent-skills:core end -->"
OVERRIDES_HEADING = "## Overrides of the core rules"
CORE_TEXT = """\
## Core rules
The same in every repository that uses `agent-skills`. Each rule has an id. The project's own sections and skills
may add to a rule or make it stricter, never relax it: a rule is relaxed only by an entry in "Overrides of the core
rules" below, written by the maintainers, one line each:
`` - `<id>`: <what changes>. Reason: <why>. Agreed: <who>, <YYYY-MM-DD>. ``
An agent may draft such a line; the reason and the name of who agreed come from the user, never from the agent.

### Other people and agents work here too
- `shared-checkout`: do not run git commands that move a checkout someone else uses (`checkout`, `reset`, `clean`,
  `stash`) and do not overwrite the build output or dependency folders named in "Project facts". Work in a separate
  git worktree unless told to use the current one, or "Project facts" says worktrees cannot run side by side.
- `line-endings`: keep each file's line endings; new files are LF.
- `delete`: delete only what your own session created, by explicit path, never with a wildcard and never outside the
  project folder; ask before deleting anything else.
- `commit`: commit or push only when asked; never bypass a hook (`--no-verify`).
- `secrets`: never write a secret, a token or a password into a tracked file, a log, a report or a commit message.
- `language`: agent files (`AGENTS.md`, skills) are written in English; existing documents in other languages are
  pointed to as they are.

### How to work
- `truth`: when the user's request, design or plan has a flaw, say so, what it costs and what you would do instead;
  then do what the user decides. Do not agree to please.
- `no-guess`: say when you do not know or could not check. In the final report of a task, mark each statement as
  verified (say how), not verified, or assumed. Guess only when asked, and label the guess.
- `quality`: do not shorten the work to answer sooner (read the code involved, run the tests, check before you
  claim) unless the user asks for a quick answer. Done means: what was asked works, is tested and is reported.
  Do not widen the scope: propose the rest.
- `tests-first`: a feature or a change of behaviour comes with a test that fails without it; a bug fix starts with a
  test that reproduces the bug. Where coverage is measured, changed lines are covered and the total does not drop.
  Going without a test needs the user's explicit request, written in the plan and in the merge request description.
- `reuse`: before writing more than a few dozen lines that are not specific to this project, look in the project,
  in its dependencies and for an existing third-party component, and propose what you find (skill `third-party`).
- `design-docs`: the architecture document and the naming rules are named in "Project facts". A new module,
  service, process, protocol or data format gets a short design note before the code. When a document is missing,
  say so and propose to write it with the maintainer (skill `agentify`).
- `web-search`: web searches and page reading go to a subagent on the lightest model available (table below); it
  returns URLs and quoted passages. The lead judges them and reads itself the pages a decision rests on.

### Stop and ask first
You need the user's explicit consent before:
1. `gate.api`: changing a public API or another contract listed in "Project facts" (formats, protocols, database
   schema, base classes); name the consumers listed there that it affects; a breaking change follows the
   versioning named there, compatible way proposed first;
2. `gate.concurrency`: changing threads, processes, locks or their order, event loops or background workers;
3. `gate.security`: changing authentication, authorization, cryptography, the handling of secrets, or how personal
   data is stored, logged, exported or retained;
4. `gate.language-level`: raising the language level or the oldest compiler, runtime, framework or platform
   supported (the limits are in "Project facts");
5. `gate.build`: adding or removing files in the build files, or changing the dependency lists;
6. `gate.outside`: changing anything outside the repository: publishing, uploading or deleting packages, tags,
   deployments, CI settings, shared databases;
7. `gate.third-party`: adding a third-party component or a version whose license changed; do not commit it before
   the user acknowledges the license (skill `third-party`);
8. `gate.rules`: changing this block, the overrides, the hooks or the CI checks.
A request to make such a change is not yet the consent: first say which item it touches, what it costs and which
"Project facts" it conflicts with, then wait for the user to confirm. State what you want to change, why, which
files and which tests.

### Lead and worker
- `workflow`: a non trivial change (feature, bug fix, refactoring) follows skill `dev-workflow` and the project's
  workflow skill: the lead reads the code, designs the tests first and writes the plan; stops at the gate when the
  plan touches it; a worker on a lighter model implements the plan and runs the tests; the lead reviews the diff,
  reruns the tests and reports. A change of a few lines that the lead can verify alone needs no worker.
- `plan-challenge`: for a complex task (see `dev-workflow`) the lead proposes to have the plan challenged by a
  second high-capacity model, which gets the plan and the code, not the lead's reasoning.

| Tool | Worker, web search | Challenger |
|---|---|---|
| Claude Code | `Agent` tool with `model`: `sonnet` or `haiku` (web search: `haiku`) | ask the user to start another tool; else `Agent` tool, `opus` |
| Codex | a subagent whose model is set in `.codex/agents/<name>.toml`, when the project has one | ask the user to start another tool |
| Antigravity | a subagent in `.agents/agents/<name>.md` with `model: flash`, when the project has one | ask the user to start another tool |

When no lighter subagent is available, the session does the work itself and says so in the report.

### Where new knowledge goes
- `knowledge`: what you learn while working (a missing step, a trap, a wrong statement in these files) is written
  down in the same change, in one place: the table is in skill `dev-workflow`. Correct a wrong statement where it
  stands. A worker reports what it learned; the lead decides where it goes. Shared skills and this block are
  changed in the `agent-skills` repository, not here.
- `missing-skill`: when a skill named in these rules or in another skill is not in `.agents/skills`, say so before
  you go on, do not invent its procedure, and propose that a maintainer update the shared skills from
  `agent-skills` with `update_project.py` (the version installed here is in `.agents/shared-skills.lock`).
"""
OLD_CORE_TITLES = {"Other people and agents work here too", "Other people and agents", "Stop and ask first",
                   "Lead and worker", "Where new knowledge goes"}


def repo_root() -> Path:
    out = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True, check=True)
    return Path(out.stdout.strip())


def files_under(folder: Path) -> set:
    if not folder.is_dir():
        return set()
    return {p.relative_to(folder) for p in folder.rglob("*")
            if p.is_file() and p.name != ".DS_Store" and "__pycache__" not in p.parts}


def differences(source: Path, copy: Path) -> list:
    """One line per file that is missing, extra or different in the copy."""
    in_source, in_copy = files_under(source), files_under(copy)
    found = [f"missing in {COPY}: {p}" for p in sorted(in_source - in_copy)]
    found += [f"not in {SOURCE}: {p}" for p in sorted(in_copy - in_source)]
    found += [f"differs: {p}" for p in sorted(in_source & in_copy)
              if not filecmp.cmp(source / p, copy / p, shallow=False)]
    return found


def sha256_of(path: Path) -> str:
    """Hex sha256 of the file content with LF line ends (a checkout that converts them must not fail the check)."""
    return hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()


def read_lock(path: Path) -> dict:
    """The parsed lock file; raises ValueError when it is not usable."""
    try:
        lock = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ValueError(str(exc))
    if not isinstance(lock, dict) or not isinstance(lock.get("files"), dict):
        raise ValueError("no \"files\" object")
    return lock


def lock_differences(root: Path, lock: dict, script: Path) -> list:
    """One line per shared skill file that is missing, changed, unlisted, or a sync_skills.py that differs."""
    source = root / SOURCE
    found = []
    for name, digest in sorted(lock["files"].items()):
        path = source / name
        if not path.is_file():
            found.append(f"shared skill file missing: {name}")
        elif sha256_of(path) != digest:
            found.append(f"shared skill file changed locally: {name}")
    folders = {name.split("/")[0] for name in lock["files"]}
    for p in sorted(files_under(source)):
        name = p.as_posix()
        if p.parts[0] in folders and name not in lock["files"]:
            found.append(f"not in the lock file: {name}")
    if lock.get("sync_skills") and sha256_of(script) != lock["sync_skills"]:
        found.append("sync_skills.py differs from the shared version")
    return found


def check_lock(root: Path) -> list:
    """The lock file report lines (empty without a lock file)."""
    path = root / LOCK
    if not path.is_file():
        return []
    try:
        lock = read_lock(path)
    except ValueError as exc:
        return [f"cannot read {LOCK}: {exc}", SHARED_NOTE.format(version="unknown")]
    found = lock_differences(root, lock, Path(__file__))
    return found + [SHARED_NOTE.format(version=lock.get("version", "unknown"))] if found else []


SHARED_NOTE = ("shared skills come from agent-skills {version}: do not edit them here, "
               "a maintainer updates them with update_project.py")
UPGRADE_NOTE = ("a maintainer installs the missing skills with update_project.py from a checkout of agent-skills at "
                "its latest tag (installed here: {version})")
SKILL_RE = re.compile(r"skill `([a-z0-9-]+)`")


def _key(line: str) -> str:
    """A line as compared with a marker or heading: no trailing CR, no leading BOM."""
    return (line[:-1] if line.endswith("\r") else line).lstrip("\ufeff")


def _eol(text: str) -> str:
    return "\r\n" if "\r\n" in text else "\n"


def has_conflict_markers(text: str) -> bool:
    """True when a line is a merge conflict marker ("=======" alone is not: it is also a markdown heading underline)."""
    return any(line.startswith(("<<<<<<< ", ">>>>>>> ")) for line in text.split("\n"))


def has_core_markers(text: str) -> bool:
    """True when a line is the begin or the end marker of the core rules block."""
    return any(_key(line) in (CORE_BEGIN, CORE_END) for line in text.split("\n"))


def _structure(lines: list) -> tuple:
    """(problems, begin index, end index) of the markers; the indexes are None unless the markers are well formed."""
    if has_conflict_markers("\n".join(lines)):
        return ["merge conflict markers: resolve the conflict first"], None, None
    begins = [i for i, line in enumerate(lines) if _key(line) == CORE_BEGIN]
    ends = [i for i, line in enumerate(lines) if _key(line) == CORE_END]
    found = []
    for name, marks in (("begin", begins), ("end", ends)):
        if not marks:
            found.append(f"no core rules {name} marker")
        elif len(marks) > 1:
            found.append(f"more than one core rules {name} marker")
    if not found and ends[0] < begins[0]:
        found.append("the core rules end marker is before the begin marker")
    return found, (None if found else begins[0]), (None if found else ends[0])


def core_block_problems(text: str) -> list:
    """The problems of the core rules block in an AGENTS.md text (empty when it is current)."""
    lines = text.split("\n")
    found, begin, end = _structure(lines)
    if begin is None:
        return found
    if "".join(line + "\n" for line in lines[begin + 1:end]).replace("\r\n", "\n") != CORE_TEXT:
        found.append("the core rules block differs from the shared text")
    if not any(_key(line) == OVERRIDES_HEADING for line in lines[end + 1:]):
        found.append(f"no \"{OVERRIDES_HEADING}\" section after the core rules block")
    return found


def replace_core_block(text: str) -> str:
    """The text with the lines between the markers replaced by CORE_TEXT; ValueError when the markers are unusable."""
    lines = text.split("\n")
    found, begin, end = _structure(lines)
    if begin is None:
        raise ValueError(found[0])
    eol = _eol(text)
    inner = [line + ("\r" if eol == "\r\n" else "") for line in CORE_TEXT.split("\n")[:-1]]
    return "\n".join(lines[:begin + 1] + inner + lines[end:])


def add_core_block(text: str) -> str:
    """The text of a file without markers with the block and an empty overrides section appended."""
    if has_conflict_markers(text):
        raise ValueError("merge conflict markers: resolve the conflict first")
    eol = _eol(text)
    if text and not text.endswith("\n"):
        text += eol
    if text and _key(text.split("\n")[-2]) != "":
        text += eol
    block = f"{CORE_BEGIN}\n{CORE_TEXT}{CORE_END}\n\n{OVERRIDES_HEADING}\nNone.\n"
    return text + block.replace("\n", eol)


def old_core_sections(text: str) -> list:
    """The headings outside the block of sections that the core rules now cover."""
    lines = text.split("\n")
    begin, end = _structure(lines)[1:]
    found = []
    for i, line in enumerate(lines):
        if begin is not None and begin <= i <= end:
            continue
        match = re.match(r"## +(?:\d+\. +)?(.+?)\s*$", line)
        if match and match.group(1) in OLD_CORE_TITLES:
            found.append(line.rstrip("\r"))
    return found


def read_agents(root: Path) -> str:
    """AGENTS.md as text with a BOM kept as a character; raises OSError or UnicodeDecodeError."""
    return (root / AGENTS).read_bytes().decode("utf-8")


def script_hint(root: Path) -> str:
    """This script's path relative to the root in POSIX form, or its file name when it is outside the root."""
    here = Path(__file__).resolve()
    try:
        return here.relative_to(root.resolve()).as_posix()
    except ValueError:
        return here.name


def core_skills(text: str = None) -> list:
    """The skills the core rules name: those in the block of an AGENTS.md text, or in CORE_TEXT."""
    if text is not None:
        lines = text.split("\n")
        begin, end = _structure(lines)[1:]
        text = "\n".join(lines[begin:end]) if begin is not None else ""
    return sorted(set(SKILL_RE.findall(CORE_TEXT if text is None else text)))


def missing_skills(root: Path, text: str) -> list:
    """The skills named in the core rules block of the AGENTS.md text that are not in .agents/skills."""
    return [name for name in core_skills(text) if not (root / SOURCE / name / "SKILL.md").is_file()]


def check_core(root: Path) -> list:
    """The AGENTS.md report lines (empty unless the lock file has "core": true)."""
    try:
        lock = read_lock(root / LOCK)
    except ValueError:
        return []
    if lock.get("core") is not True:
        return []
    if not (root / AGENTS).is_file():
        return [f"{AGENTS} not found: the lock file enables the core rules block"]
    try:
        text = read_agents(root)
    except (OSError, UnicodeDecodeError) as exc:
        return [f"{AGENTS}: cannot read: {exc}"]
    found = [f"{AGENTS}: {p}" for p in core_block_problems(text)]
    if found:
        found.append(f"{AGENTS}: repair the core rules block with python3 {script_hint(root)} --fix")
    missing = missing_skills(root, text)
    found += [f"skill `{name}` is named by the core rules but not in {SOURCE}" for name in missing]
    if missing:
        found.append(UPGRADE_NOTE.format(version=lock.get("version", "unknown")))
    return found


def fix_core(root: Path) -> int:
    """Restore the core rules block of AGENTS.md."""
    try:
        enabled = read_lock(root / LOCK).get("core") is True
    except ValueError:
        enabled = False
    if not enabled:
        print(f"the core rules block is not enabled: {LOCK} needs \"core\": true", file=sys.stderr)
        return 2
    if not (root / AGENTS).is_file():
        print(f"{AGENTS} not found in {root}")
        return 1
    try:
        text = read_agents(root)
        new = replace_core_block(text)
    except (OSError, UnicodeDecodeError, ValueError) as exc:
        print(f"{AGENTS}: {exc}")
        return 1
    if new != text:
        (root / AGENTS).write_bytes(new.encode("utf-8"))
    print(f"{AGENTS}: core rules block restored" if new != text else f"{AGENTS}: core rules block is up to date")
    return 0


# The adoption checklist. "auto" items are computed by --doctor in the clone where it runs and never written into a
# file; "manual" items are decided by a person and kept in .agents/adoption.md.
CHECKLIST = [
    ("files.agents-md", "auto", "AGENTS.md has the sections \"Project facts\", \"Conventions\" and \"Skills\""),
    ("files.claude-md", "auto", "CLAUDE.md imports AGENTS.md (a line \"@AGENTS.md\")"),
    ("files.no-override", "auto", "no AGENTS.override.md at the root (Codex would read it instead of AGENTS.md)"),
    ("core.block", "auto", "the core rules block is enabled, present and current"),
    ("core.overrides", "auto", "each override names a core rule once, with a reason, who agreed and a date"),
    ("core.old-sections", "auto", "no old section repeats what the core rules block covers"),
    ("skills.copy", "auto", ".claude/skills is the copy of .agents/skills; the shared skills are unchanged"),
    ("skills.workflow", "auto", "a <repo>-dev-workflow skill extends the shared skill dev-workflow"),
    ("skills.core", "auto", "the skills the core rules name are installed"),
    ("hook.file", "auto", "a pre-commit hook in the repository runs sync_skills.py --check"),
    ("hook.enabled", "auto", "the hook is enabled in this clone (core.hooksPath)"),
    ("facts.placeholders", "auto", "no <...> placeholder of the templates is left in AGENTS.md or the workflow skill"),
    ("facts.documents", "auto", "the documents named in \"Project facts\" exist, or are marked \"missing\""),
    ("facts.consumers", "auto", "\"Project facts\" names the consumers; \"none\" only as stated by a maintainer, "
                                "with the name and the date"),
    ("size.agents-md", "auto", "the project part of AGENTS.md (before the block) is at most 100 lines"),
    ("ci.check", "manual", "CI runs sync_skills.py --check on every merge request (the hook is opt-in per clone); "
                           "not applicable only without CI, with the reason"),
    ("hook.documented", "manual", "the README or AGENTS.md says how to enable the hook in a new clone"),
    ("facts.reviewed", "manual", "a maintainer checked every line of \"Project facts\" against the repository"),
    ("docs.architecture", "manual", "an architecture document exists, or writing one is planned (with a date)"),
    ("docs.naming", "manual", "naming rules exist (a document or a skill) with a check command, or are planned"),
    ("tests.command", "manual", "the test command of \"Project facts\" runs on a clean clone"),
    ("tests.coverage", "manual", "coverage is measured, or \"not measured\" is a recorded decision"),
    ("licenses.folder", "manual", "the license texts of all components are where \"Project facts\" says"),
    ("tools.agents", "manual", "the agent tools used here are known, and their row of the core rules table works "
                               "(a subagent definition exists where the tool needs one)"),
    ("eval.prompts", "manual", "the three behaviour prompts (language level, new component, relaxing a core rule) "
                               "were run in each tool used, with the date and the result"),
]
ADOPTION = Path(".agents/adoption.md")
MAX_OVERRIDES = 3
STATES = {"x": "done", " ": "open", "-": "n/a", ">": "deferred"}
DOC_LABELS = ("architecture", "naming", "documents", "license", "third-party")
OVERRIDE_RE = re.compile(r"- `([a-z][a-z.-]*)`: (.+?)\.\s+Reason: (.+?)\.\s+Agreed: (.+?), (\d{4}-\d{2}-\d{2})\.\s*$",
                         re.S)
ADOPTION_RE = re.compile(r"- \[(.)\] `([a-z][a-z.-]*)`(.*)$")
STATED_RE = re.compile(r"\(stated by [^,()]+, \d{4}-\d{2}-\d{2}\)")
NO_CONSUMERS = "none (stated by <name>, <YYYY-MM-DD>)"


def core_rule_ids() -> list:
    """The rule ids of CORE_TEXT, in order."""
    return re.findall(r"^\s*(?:- |\d+\. )`([a-z][a-z.-]*)`:", CORE_TEXT, re.M)


def _sections(lines: list) -> dict:
    """"## " heading -> its lines (up to the next "## " heading), outside the core rules block."""
    begin, end = _structure(lines)[1:]
    found, current = {}, None
    for i, line in enumerate(lines):
        if begin is not None and begin <= i <= end:
            current = None
            continue
        if _key(line).startswith("## "):
            current = _key(line)[3:].strip()
            found[current] = []
        elif current is not None:
            found[current].append(_key(line))
    return found


def _entries(lines: list) -> list:
    """The "- " list entries of some lines, each joined with its indented continuation lines."""
    entries = []
    for line in lines:
        if line.startswith("- "):
            entries.append(line)
        elif entries and line.startswith(" ") and line.strip():
            entries[-1] += "\n" + line.strip()
    return entries


def check_overrides(sections: dict) -> tuple:
    """(status, detail, ids) of the overrides section."""
    lines = sections.get(OVERRIDES_HEADING[3:])
    if lines is None:
        return "fail", f"no \"{OVERRIDES_HEADING}\" section", []
    known, ids, problems = set(core_rule_ids()), [], []
    for entry in _entries(lines):
        match = OVERRIDE_RE.match(entry.replace("\n", " "))
        if not match:
            problems.append(f"not in the form `<id>`: <change>. Reason: <why>. Agreed: <who>, <YYYY-MM-DD>.: "
                            f"{entry.splitlines()[0]}")
            continue
        ident = match.group(1)
        if ident not in known:
            problems.append(f"no core rule `{ident}`")
        elif ident in ids:
            problems.append(f"`{ident}` overridden twice")
        ids.append(ident)
    if problems:
        return "fail", "; ".join(problems), ids
    if len(ids) > MAX_OVERRIDES:
        return "warn", (f"{len(ids)} overrides ({', '.join(ids)}), more than {MAX_OVERRIDES}: worth a talk with the "
                        f"agent-skills maintainers"), ids
    return "ok", (", ".join(ids) if ids else "none"), ids


def find_placeholders(text: str) -> list:
    """The <...> placeholders of a markdown text, outside code spans, comments and the core rules block."""
    lines = text.split("\n")
    begin, end = _structure(lines)[1:]
    if begin is not None:
        lines = lines[:begin] + lines[end + 1:]
    found = []
    for line in lines:
        line = re.sub(r"`[^`]*`", "", line)
        found += re.findall(r"<[A-Za-z][^<>\n]*>", line)
    return [p for p in found if not p.startswith(("<http", "<br"))]


def missing_documents(root: Path, sections: dict) -> list:
    """The paths in backticks of the document lines of "Project facts" that do not exist."""
    missing = []
    for entry in _entries(sections.get("Project facts", [])):
        label = entry[2:].split(":", 1)[0].strip().lower()
        if not label.startswith(DOC_LABELS):
            continue
        for token in re.findall(r"`([^`\s]+)`", entry):
            if "/" in token and not re.search(r"[*<>$?]", token) and not token.startswith("http"):
                if not (root / token.rstrip("/")).exists():
                    missing.append(token)
    return missing


def check_consumers(sections: dict) -> tuple:
    """(ok, detail) of the Consumers line of "Project facts": present, filled in, and "none" only when a maintainer
    stated it."""
    for entry in _entries(sections.get("Project facts", [])):
        label, _, value = entry[2:].partition(":")
        if label.strip().lower() != "consumers":
            continue
        value = " ".join(value.split())
        if not value or find_placeholders(value):
            return False, f"not filled in: the projects, applications and libraries using it, or {NO_CONSUMERS}"
        if value.lower().startswith("none") and not STATED_RE.search(value):
            return False, f"\"none\" needs who acknowledged it: {NO_CONSUMERS}"
        return True, ""
    return False, f"no \"- Consumers:\" line in \"Project facts\": who uses this repository, or {NO_CONSUMERS}"


def hooks_folder(root: Path) -> tuple:
    """(core.hooksPath of this clone or None, whether root is a git checkout, the folder where the hook is looked
    for: core.hooksPath when set, else .githooks)."""
    try:
        out = subprocess.run(["git", "config", "core.hooksPath"], cwd=str(root), capture_output=True, text=True)
        inside = subprocess.run(["git", "rev-parse", "--is-inside-work-tree"], cwd=str(root), capture_output=True,
                                text=True)
    except OSError:
        return None, False, root / ".githooks"
    if inside.returncode != 0:
        return None, False, root / ".githooks"
    path = out.stdout.strip() if out.returncode == 0 else ""
    return (path or None), True, (root / path if path else root / ".githooks")


def hook_runs_check(folder: Path) -> bool:
    """True when the folder has a pre-commit hook and one of its files runs sync_skills.py."""
    if not (folder / "pre-commit").is_file():
        return False
    for path in folder.iterdir():
        try:
            if path.is_file() and "sync_skills.py" in path.read_text(encoding="utf-8", errors="replace"):
                return True
        except OSError:
            continue
    return False


def read_adoption(root: Path) -> dict:
    """id -> (state, note) from .agents/adoption.md (empty when there is none)."""
    path = root / ADOPTION
    if not path.is_file():
        return {}
    found = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = ADOPTION_RE.match(line.strip())
        if match and match.group(1) in STATES:
            rest = match.group(3)
            note = rest.split("Note:", 1)[1].strip() if "Note:" in rest else ""
            found[match.group(2)] = (STATES[match.group(1)], note)
    return found


def adoption_text(states: dict) -> str:
    """The content of .agents/adoption.md: every manual item with its state and note."""
    marks = {v: k for k, v in STATES.items()}
    lines = ["# Adoption checklist", "",
             "The items a person decides; `sync_skills.py --doctor` checks the others in each clone. Mark `[x]` done,",
             "`[-]` not applicable, `[>]` deferred, `[ ]` open; write the reason or the date after \"Note:\".", ""]
    for ident, kind, text in CHECKLIST:
        if kind == "manual":
            state, note = states.get(ident, ("open", ""))
            lines.append(f"- [{marks[state]}] `{ident}` {text}." + (f" Note: {note}" if note else ""))
    return "\n".join(lines) + "\n"


def doctor_report(root: Path, script: Path = None) -> dict:
    """{"version", "items": [{"id", "status", "detail", "note"}], "overrides": [ids], "unknown": [ids]}."""
    script = Path(__file__) if script is None else script
    try:
        lock = read_lock(root / LOCK)
    except ValueError:
        lock = None
    try:
        text = read_agents(root) if (root / AGENTS).is_file() else ""
    except (OSError, UnicodeDecodeError):
        text = ""
    lines = text.split("\n")
    sections = _sections(lines)
    result = {}

    def mark(ident, ok, detail="", status=None):
        result[ident] = (status or ("ok" if ok else "fail"), detail)

    wanted = ["Project facts", "Conventions", "Skills"]
    lacking = [s for s in wanted if s not in sections]
    mark("files.agents-md", bool(text) and not lacking,
        "AGENTS.md not found" if not text else ("missing: " + ", ".join(lacking) if lacking else ""))
    claude = root / "CLAUDE.md"
    has_import = claude.is_file() and "@AGENTS.md" in claude.read_text(encoding="utf-8", errors="replace").split()
    mark("files.claude-md", has_import, "" if has_import else "add a line \"@AGENTS.md\" to CLAUDE.md")
    override = (root / "AGENTS.override.md").exists()
    mark("files.no-override", not override, "remove AGENTS.override.md" if override else "")
    if not (lock and lock.get("core") is True):
        mark("core.block", False, "not enabled: python3 tools/update_project.py <project> --core (from agent-skills)")
    else:
        problems = core_block_problems(text) if text else ["AGENTS.md not found"]
        mark("core.block", not problems, "; ".join(problems) + (f" (repair: {script_hint(root)} --fix)"
                                                                 if problems else ""))
    status, detail, overrides = check_overrides(sections)
    mark("core.overrides", None, detail, status=status)
    old = old_core_sections(text)
    mark("core.old-sections", not old, ", ".join(old))
    copy = differences(root / SOURCE, root / COPY) if (root / SOURCE).is_dir() else [f"{SOURCE} not found"]
    shared = lock_differences(root, lock, script) if lock else []
    mark("skills.copy", not copy and not shared, "; ".join(copy + shared))
    workflows = sorted((root / SOURCE).glob("*-dev-workflow/SKILL.md")) if (root / SOURCE).is_dir() else []
    extends = [p for p in workflows if "Extends the shared skill `dev-workflow`" in p.read_text(encoding="utf-8")]
    mark("skills.workflow", extends, "" if extends else ("no <repo>-dev-workflow skill" if not workflows else
                                                         "the workflow skill does not say it extends dev-workflow"))
    missing = missing_skills(root, text) if lock and lock.get("core") is True else []
    version = lock.get("version", "unknown") if lock else "unknown"
    mark("skills.core", not missing,
         "not installed: " + ", ".join(missing) + "; " + UPGRADE_NOTE.format(version=version) if missing else "")
    configured, is_git, folder = hooks_folder(root)
    has_hook = folder.is_dir() and hook_runs_check(folder)
    where = os.path.relpath(str(folder), str(root)).replace(os.sep, "/")
    mark("hook.file", has_hook, "" if has_hook else f"no pre-commit hook in {where} "
                                                   f"that runs sync_skills.py --check")
    hint = f"git config core.hooksPath {where}"
    if not is_git:
        mark("hook.enabled", None, "not a git checkout, cannot tell", status="warn")
    elif configured and has_hook:
        mark("hook.enabled", True, configured)
    else:
        mark("hook.enabled", None, f"not enabled in this clone: {hint}", status="warn")
    places = find_placeholders(text)
    for path in workflows:
        places += find_placeholders(path.read_text(encoding="utf-8"))
    short = [p if len(p) <= 30 else p[:27] + "...>" for p in places[:3]]
    mark("facts.placeholders", not places, f"{len(places)} left, the first: {', '.join(short)}" if places else "")
    missing = missing_documents(root, sections)
    mark("facts.documents", not missing, "not found: " + ", ".join(missing) if missing else "")
    mark("facts.consumers", *check_consumers(sections))
    begin = _structure(lines)[1]
    size = begin if begin is not None else len(lines)
    mark("size.agents-md", size <= 100, f"{size} lines")

    manual = read_adoption(root)
    items = []
    for ident, kind, text_ in CHECKLIST:
        if kind == "auto":
            status, detail = result[ident]
            items.append({"id": ident, "status": status, "detail": detail or text_, "note": ""})
        else:
            state, note = manual.get(ident, ("open", ""))
            items.append({"id": ident, "status": state, "detail": text_, "note": note})
    known = {item[0] for item in CHECKLIST}
    return {"version": lock.get("version", "unknown") if lock else None, "items": items, "overrides": overrides,
            "unknown": sorted(set(manual) - known)}


def print_report(root: Path, report: dict) -> int:
    """Print a doctor report; 1 when a computed item fails."""
    print(f"{root}: agent-skills {report['version'] or 'not installed'}")
    for item in report["items"]:
        note = f" (note: {item['note']})" if item["note"] else ""
        print(f"  {item['status']:<8} {item['id']:<19} {item['detail']}{note}")
    for ident in report["unknown"]:
        print(f"  {'warn':<8} {ident:<19} in {ADOPTION} but not in the checklist any more")
    if not (root / ADOPTION).is_file():
        print(f"  {ADOPTION} not found: write it with --doctor --write")
    return 1 if any(item["status"] == "fail" for item in report["items"]) else 0


def doctor(root: Path, write: bool) -> int:
    """Print the adoption checklist of this clone; with write, also rewrite .agents/adoption.md."""
    if write:
        text = adoption_text(read_adoption(root))
        path = root / ADOPTION
        if not path.is_file() or path.read_text(encoding="utf-8") != text:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(text.encode("utf-8"))
            print(f"{ADOPTION} written")
    return print_report(root, doctor_report(root))


def main(argv=None, root=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="only report the differences (exit 1 if any)")
    mode.add_argument("--fix", action="store_true", help="restore the core rules block of AGENTS.md")
    mode.add_argument("--doctor", action="store_true", help="print the adoption checklist (exit 1 if an item fails)")
    parser.add_argument("--write", action="store_true", help="with --doctor: write or update .agents/adoption.md")
    args = parser.parse_args(argv)
    if args.write and not args.doctor:
        parser.error("--write needs --doctor")

    root = Path(root) if root is not None else repo_root()
    if args.fix:
        return fix_core(root)
    if args.doctor:
        return doctor(root, args.write)
    source, copy = root / SOURCE, root / COPY
    if not source.is_dir():
        print(f"{SOURCE} not found in {root}", file=sys.stderr)
        return 2

    found = differences(source, copy)
    if args.check:
        for line in found:
            print(line)
        if found:
            print(f"{COPY} is not a copy of {SOURCE}: edit the skills in {SOURCE} and run this script without --check")
        shared = check_lock(root) + check_core(root)
        for line in shared:
            print(line)
        return 1 if found or shared else 0

    if found:
        if copy.exists():
            shutil.rmtree(copy)
        shutil.copytree(source, copy, ignore=shutil.ignore_patterns(".DS_Store", "__pycache__"))
    print(f"{COPY}: {len(found)} file(s) updated" if found else f"{COPY} is up to date")
    return 0


if __name__ == "__main__":
    sys.exit(main())
