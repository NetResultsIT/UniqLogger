#!/usr/bin/env python3
"""Compare the exported symbols of two builds of a library, to find an ABI break.

    python3 abi_check.py <old library> <new library> [--demangle] [--ignore REGEX ...] [--quiet]

Works on a macOS .dylib, a Linux .so and a static .a, read with nm. A symbol that is in the old library and not
in the new one was removed or changed (C++ names are mangled: a changed signature is one removed and one added
symbol, they are not paired). New symbols are additions. A universal macOS binary is compared per architecture;
an architecture that disappeared is a break.

Exit code: 0 no symbol removed, 1 at least one removed (or an architecture removed), 2 usage or tool error.

Weak symbols (nm types W, V, w, v) are left out: inline functions and templates are emitted in every library
that uses them, so they come and go without a change of the interface. Nothing else is left out unless a
--ignore regular expression matches the (mangled) name.

Limits: it cannot see a changed class layout, a changed enum value, a changed inline function, or a changed
return type on platforms whose mangling omits it. Both files must be built the same way (platform,
configuration, compiler).
"""
import argparse
import re
import shutil
import subprocess
import sys

WEAK = set("WVwv")
UNDEFINED = set("Uu")
ARCH_LINE = re.compile(r"^.*\(for architecture ([^)]+)\):\s*$")
SYMBOL_LINE = re.compile(r"^([0-9a-fA-F]*)\s+([A-Za-z?-])\s+(\S.*)$")


class ToolError(Exception):
    """A problem that ends the run with exit code 2."""


def nm_command(path: str, platform: str = None) -> list:
    """The nm command line that lists the exported, defined symbols of the file."""
    platform = platform or sys.platform
    if platform == "darwin":
        return ["nm", "-gU", path]
    if path.endswith(".a"):
        return ["nm", "-g", "--defined-only", path]
    return ["nm", "-D", "--defined-only", path]


def run_nm(path: str) -> str:
    """The text printed by nm for the file; raises ToolError when it cannot be read."""
    if shutil.which("nm") is None:
        raise ToolError("nm not found")
    try:
        done = subprocess.run(nm_command(path), stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              universal_newlines=True)
    except OSError as exc:
        raise ToolError(f"cannot run nm: {exc}")
    if done.returncode != 0:
        raise ToolError(f"nm failed on {path}: {done.stderr.strip() or 'exit ' + str(done.returncode)}")
    return done.stdout


def parse_nm(text: str, ignore: list = ()) -> dict:
    """Map architecture name ('' when nm printed no architecture) to the set of symbol names it defines."""
    patterns = [re.compile(r) for r in ignore]
    result = {}
    arch = ""
    for line in text.splitlines():
        header = ARCH_LINE.match(line)
        if header:
            arch = header.group(1)
            result.setdefault(arch, set())
            continue
        match = SYMBOL_LINE.match(line.rstrip())
        if not match:
            continue
        kind, name = match.group(2), match.group(3)
        if kind in WEAK or kind in UNDEFINED or any(p.search(name) for p in patterns):
            continue
        result.setdefault(arch, set()).add(name)
    return result or {"": set()}


def compare(old: dict, new: dict) -> dict:
    """Map architecture to (removed, added) sorted lists; an architecture only in one side is a key of 'archs'."""
    both = sorted(set(old) & set(new))
    return {
        "symbols": {a: (sorted(old[a] - new[a]), sorted(new[a] - old[a])) for a in both},
        "archs_removed": sorted(set(old) - set(new)),
        "archs_added": sorted(set(new) - set(old)),
    }


def demangle(names: list) -> list:
    """The names through c++filt (one process); the names unchanged, with a warning, when that is not possible."""
    if not names:
        return names
    if shutil.which("c++filt") is None:
        print("warning: c++filt not found, printing the mangled names", file=sys.stderr)
        return names
    done = subprocess.run(["c++filt"], input="\n".join(names) + "\n", stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, universal_newlines=True)
    lines = done.stdout.splitlines()
    if done.returncode != 0 or len(lines) != len(names):
        print("warning: c++filt failed, printing the mangled names", file=sys.stderr)
        return names
    return lines


def summary(removed: int, added: int) -> str:
    """The one line verdict."""
    if removed:
        return f"ABI break: {removed} symbol(s) removed or changed"
    if added:
        return f"ABI kept: {added} symbol(s) added"
    return "ABI kept: no change in the exported symbols"


def report(result: dict, show_names: bool, use_demangle: bool) -> (list, int):
    """The output lines and the number of removals (an architecture removed counts as one)."""
    lines = []
    removed_total = added_total = 0
    symbols = result["symbols"]
    for arch in result["archs_removed"]:
        lines.append(f"architecture removed: {arch}")
        removed_total += 1
    for arch in result["archs_added"]:
        lines.append(f"architecture added: {arch}")
    for arch, (removed, added) in symbols.items():
        removed_total += len(removed)
        added_total += len(added)
        if use_demangle:
            shown = demangle(removed + added)
            removed, added = shown[:len(removed)], shown[len(removed):]
        if len(symbols) > 1 and (removed or added):
            lines.append(f"architecture {arch}:")
        lines += [f"removed: {n}" for n in removed] + [f"added: {n}" for n in added]
    if not show_names:
        lines = []
    lines.append(summary(removed_total, added_total))
    return lines, removed_total


def main(argv=None) -> int:
    """Run the tool; returns the exit code."""
    parser = argparse.ArgumentParser(description="Compare the exported symbols of two builds of a library.")
    parser.add_argument("old", help="the library of the last release")
    parser.add_argument("new", help="the library to check")
    parser.add_argument("--demangle", action="store_true", help="print C++ names through c++filt")
    parser.add_argument("--ignore", action="append", default=[], metavar="REGEX",
                        help="leave out the symbols whose name matches (repeatable)")
    parser.add_argument("--quiet", action="store_true", help="print only the summary line")
    try:
        args = parser.parse_args(argv)
    except SystemExit as exc:
        return 2 if exc.code else 0
    try:
        for pattern in args.ignore:
            re.compile(pattern)
        old = parse_nm(run_nm(args.old), args.ignore)
        new = parse_nm(run_nm(args.new), args.ignore)
    except re.error as exc:
        print(f"error: bad --ignore expression: {exc}", file=sys.stderr)
        return 2
    except ToolError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    lines, removed = report(compare(old, new), not args.quiet, args.demangle)
    print("\n".join(lines))
    return 1 if removed else 0


if __name__ == "__main__":
    sys.exit(main())
