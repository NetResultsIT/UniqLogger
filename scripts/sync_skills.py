#!/usr/bin/env python3
"""Keep the two copies of the agent skills identical.

The skills are edited in .agents/skills (the source). .claude/skills is a copy of it, kept as real files
because git symbolic links do not work on every platform.

    python3 <this script>            copy .agents/skills over .claude/skills
    python3 <this script> --check    exit 1 and list the differences, change nothing

With --check, when .agents/shared-skills.lock exists, it also reports shared skills (copied from agent-skills)
that were edited, removed or extended in this project, and a copy of this script that is not the shared one.
"""
import argparse
import filecmp
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

SOURCE = Path(".agents/skills")
COPY = Path(".claude/skills")
LOCK = Path(".agents/shared-skills.lock")


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


def main(argv=None, root=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="only report the differences (exit 1 if any)")
    args = parser.parse_args(argv)

    root = Path(root) if root is not None else repo_root()
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
        shared = check_lock(root)
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
