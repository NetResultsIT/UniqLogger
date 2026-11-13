---
name: semver-abi
description: Semantic versioning and binary compatibility rules of this library - how to classify a change as patch, minor or major, what breaks the ABI of a C++/Qt library even when the source still compiles, how to change an API without breaking it (new API, the old one deprecated and forwarding), when the version number is raised, and how to check the exported symbols. Use when a change touches a public header, an exported class, an enum, a JSON format or an error code, when adding a feature, and before a release.
---

# Semantic versioning and ABI

This skill is shared by several Qt libraries. The file in a project is a copy from the `agent-skills` repository,
pinned in `.agents/shared-skills.lock`: do not edit it in the project. Where the version is defined, which
document records a deprecation and which major is still unreleased are in the project's `AGENTS.md` and workflow
skill.

## The rule

The version is `MAJOR.MINOR.PATCH`. Applications built against a version keep working, **without being rebuilt**,
with every later version of the same major: the binary interface (ABI) is kept at all costs within a major.

| The change | Version | Needs |
|---|---|---|
| A bug fix; an internal change with no new public symbol and no change of documented behaviour | PATCH + 1 | nothing more |
| A new feature; anything new that a user can call or read (method, signal, class, enumerator, JSON key, option) | MINOR + 1, PATCH = 0 | ABI kept (see "Adding without breaking") |
| A public API changed or removed; an ABI break of any kind | MAJOR + 1 | the user's explicit acknowledgement of the new major, after the compatible alternative was proposed |

When you think a change needs a major: stop, and propose the compatible way first (next section). Only if the user
then says, in so many words, that the major is to be raised, make the breaking change. "Change this method" is not
that acknowledgement.

The rule binds from the first release of a major. While a major has never been released (the project's `AGENTS.md`
says which), its API may still change with the user's consent and without a further major; the change is still
recorded in the migration notes.

## Changing an API without breaking it

1. Add the new API beside the old one (new name, or new overload when the types cannot be confused): MINOR + 1.
2. Keep the old one, same signature and same symbol, and make it call the new one, so there is one implementation.
   If the old behaviour cannot be kept exactly, keep it as close as possible and document the difference.
3. Mark the old one deprecated: in the Doxygen (`\deprecated` with the version and the replacement) and, for
   C++ callers, with the project's deprecation macro or `[[deprecated("use X")]]` when the project uses it. A
   deprecation warning must not break a build that treats warnings as errors without the user knowing: say so.
4. Record it where the project records API changes (migration or porting notes): old, new, since which version.
5. The old API is removed only in the next major, and its removal is listed there.

A behaviour change of an existing method (other default, other error code, other unit, other threading) is an API
change even when the signature is the same: prefer a new method or an option that defaults to the old behaviour.

## What is public

Whatever a user can call, read or link: the installed headers and exported classes; the generated bindings
(Qt, Objective-C, Java, Python) and their names; signals and their argument types; enumerators **and their numeric
values** (applications store them, other libraries cast them); error codes; the keys, value types and units of
the JSON the library produces or accepts; file formats and file names the library writes for others to read;
defaults. Internal headers that are not installed, and members that are private and not inline, are not.

## What breaks the ABI (the source may still compile)

For an exported class or a public struct:
- adding, removing or reordering a **virtual** method, or changing its signature; adding the first virtual;
- adding, removing, reordering or retyping a **data member** (the size and the offsets change) unless the class
  keeps its data behind a private implementation pointer; changing its base classes;
- changing a method's signature in any way: parameter types, `const`, return type (the return type alone still
  changes behaviour), a default argument (the old value is compiled into the callers);
- removing or renaming an exported function, method or class; removing the export macro; making a method inline
  that was not, or the reverse; changing the body of an **inline** function or a template in a public header
  (callers keep the old body);
- changing a `typedef`, the underlying type of an enum, a `#define` or a constant in a public header (compiled
  into the callers).

For enums: changing the value of an enumerator, inserting one before the last (the following ones move), removing
one. For Qt: changing a signal or slot signature, removing a `Q_PROPERTY` or `Q_INVOKABLE`, changing a registered
meta type name.

## Adding without breaking (minor)

- New **non-virtual** methods on an existing class; new classes; new free functions; new signals.
- New enumerators **at the end**, with an explicit value; say in the notes that old applications will receive a
  value they do not know.
- New data: only behind the private implementation pointer. A struct with public fields that users create or copy
  cannot grow: add a new struct or new accessors instead.
- New optional behaviour: a new method or option that defaults to the old behaviour, not a new default argument
  on an existing method (add an overload instead; check that the generated bindings can tell the two apart).
- New JSON keys: allowed (users must ignore keys they do not know; say so in the notes); never change the type or
  the meaning of an existing key.
- A new virtual method is never an addition: add a non-virtual one, or a new interface class.

## Raising the version number

- The change that needs a higher number raises it, in the same merge request: a feature raises MINOR (and resets
  PATCH) unless the development branch is already at a higher MINOR than the last release; a fix needs nothing when
  the development branch is already past the last release.
- After a release the development branch goes to PATCH + 1 of what was released.
- The version is written in one place (the project's workflow skill says where); other copies (Doxygen project
  number, resource files) follow it in the same change.
- Never lower a version, never reuse a released number, never change what a tag points to.

## Checking

- `abi_check.py` (next to this file) compares the exported symbols of two builds of the library:
  `python3 <this folder>/abi_check.py <old library> <new library>`. A symbol that disappeared or changed is an ABI
  break (exit 1); new symbols are listed as additions (the change is at least a minor). It reads the symbol table
  with `nm`, so it works for macOS and Linux shared libraries built the same way (same platform, same
  configuration); it cannot see a changed class layout, a changed enum value or a changed inline function: those
  are found by reading the diff of the public headers with the lists above.
- Before a release: run it against the library of the last release of the same major, and read
  `git diff <last release tag> -- <public headers>` with the lists above.
- In a review: for every changed public header, say which row of the table the change is, and check that the
  version and the notes agree with it.

## What to report

The classification of the change (patch, minor or major) and why; for a minor, how the ABI was kept; for a
proposed major, the compatible alternative you offer instead; the output of `abi_check.py` when a library of the
last release was available, or that it was not run.
