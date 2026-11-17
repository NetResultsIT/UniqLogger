---
name: qt-naming
description: Naming conventions for Qt C++ code - types, files, members (m_camelCase, m_pX pointers, m_rX references), locals, methods, signals and slots, parameters (i_/o_/io_ in internal code, never in the public API), integer types at two levels (relaxed or strict, chosen by the project), test programs and functions, spelling, rename commits. Use before writing or reviewing names in Qt C++, adding a test program, or renaming.
---

# Qt C++ naming conventions

A shared skill. The project's naming document (named in `AGENTS.md`, "Project facts") adds its own rules on top:
the prefix of its public types, its named exceptions, the integer level, its check script. It may add rules or
make one stricter; it never relaxes one without an entry in "Overrides of the core rules".

Old code is not renamed for the sake of it: the rules apply to new code, to code you rewrite anyway, and to rename
passes planned on purpose. Third-party code copied into the repository keeps its own names.

## Types and files
- Classes, structs, enums, typedefs: `PascalCase`. Acronyms in new names are written as words: `RtpSession`,
  `SipAccount`, `HttpClient`, not `RTPSession`.
- Public types carry the project's prefix when it has one; internal types have none.
- A file is named after its main class (`WaveFileWriter.h` / `.cpp`). A header that groups small helper types of one
  subject is fine. Public headers keep their names: renaming one breaks every consumer's `#include`.
- Keep each file's line endings.

## Data members

| What | Form | Examples |
|---|---|---|
| POD (`bool`, integers, floating point, `char`, enums, `std::atomic` of those) | `m_camelCase` | `m_initDone`, `m_retryCount` |
| non-POD object held by value | `m_camelCase` or `m_PascalCase` | `m_loggers`, `m_Settings` |
| pointer (raw or smart) | `m_pX` (expected); `m_xPtr` tolerated | `m_pLogger`, `m_parentPtr` |
| reference | `m_rX` | `m_rConfig` |

- `m_PascalCase` only for non-POD types: `int m_FileError` should be `m_fileError`.
- A member that holds a pointer starts with `m_p` (or ends with `Ptr`), and nothing else does.
- No other type letters in new code (`m_bDone`, `m_lBytes`, `m_ulUsec`): the type is in the declaration.
- Static members `s_camelCase`; constants (`static constexpr`, `const` at namespace scope) `kCamelCase`, or
  `ALL_CAPS` where the surrounding file uses it.
- Classes derived from a third-party base keep that library's member style where the base requires it.

## Locals, methods, signals and slots
- Locals: `camelCase`; `PascalCase` only for non-POD objects. A `p` prefix or `Ptr` suffix for pointers is
  recommended.
- Methods: `camelCase`. No new methods starting with an uppercase letter or written in `snake_case`.
- New signals start with `sig` (`sigIncomingCall`), or name the subject and a past participle of the state it
  reached (`volumeChanged`, `configurationModified`, `usersListUpdated`, `videoEnabled`), or both
  (`sigVolumeChanged`). Not a verb in another form or a noun alone (`writersToDelete`, `startThread`, `progress`).
  Existing public signals keep their names: renaming one is a public API change.
- Slots that react to something start with `on` (`onFinished`).
- Connections use the typed form only (skill `qt-cpp`, "Connections"). A name used by `QMetaObject::invokeMethod`
  as a string breaks silently when renamed: search for the string before renaming a slot.

## Parameters and the public API
- The public API is what a user of the library can call: the public members and signals of the exported classes in
  the installed headers, and what the project's wrappers expose. Private and protected members of those classes, and
  public members of internal classes, are internal code.
- Internal code: the direction prefixes are encouraged, `i_` input, `o_` output, `io_` both
  (`int read(char *o_buffer, int i_maxBytes)`). Not `in_`, `out_` or a bare `_`.
- Public API: **no** direction prefixes. The direction and the unit go in the documentation:
  `\param[out] errCode`, `\param timeout in milliseconds`.

## Integer types
The project chooses the level in "Project facts" (`Naming rules: skill qt-naming, integers strict`); without a
choice, **relaxed** applies.

**Relaxed**
- Never `long` / `unsigned long`: they are 32-bit on Windows and on 32-bit Android, 64-bit on macOS, Linux, iOS and
  arm64. Use `qint32` / `quint32` or `qint64` / `quint64` from the range the value needs (microseconds since an
  epoch and byte counters need 64 bits).
- A Qt fixed-width type where the width matters: protocol fields, file formats, counters, everything in the public
  API. Plain `int` is accepted elsewhere.
- `qsizetype` for sizes and indexes of Qt containers in internal code; never in a public API that is wrapped for
  32-bit targets (it is 32-bit there).
- Do not mix `<cstdint>` and Qt types for the same value: on Linux `int64_t` is `long` and `qint64` is `long long`,
  two different types (pointers do not convert, overloads and registered meta types differ).

**Strict** (everything of relaxed, plus)
- Qt fixed-width types in all new and rewritten code: `short`, `long long`, `unsigned long long` and Qt's `uint`,
  `ulong`, `ushort` are replaced by the Qt type of the same width. Plain `int` stays accepted internally.
- No `<cstdint>` types in the project's own code, except where an external API requires them (overrides and
  callbacks of third-party libraries, Qt overrides with their own signature) and in public headers that must
  build without Qt, which the project names.
- The public API uses `qint32` for every `int`, and `quint32`, `quint64` for the unsigned ones, in parameters, return
  values, signal arguments, public struct fields and typedefs.

## Tests and tools
- One folder per test program, `Test<Subject>`, and the program has the folder's name (`TARGET = TestSubject`):
  logs and crash reports then say which program they belong to.
- Test classes are `PascalCase`, named after the folder.
- New test functions: `test_When_<condition>_Then_<expected>` for behaviour, `test_<unit>_<case>` for unit
  checks. Existing test functions keep their names: skills, documents and run logs refer to them.
- Data rows: `QTest::newRow("<short description>")`, unique within the test.

## Spelling
Identifiers are English and spelled correctly (`retransmission`, `management`, `length`, `receiver`, `payload`). A
misspelled internal identifier is fixed in its own commit; a misspelled public one is a public API change
(`gate.api`).

## Renaming existing names
- Not in passing in a functional change: renames are their own commits, one subject each, built with every build
  variant the project has, and listed in `.git-blame-ignore-revs`
  (`git config blame.ignoreRevsFile .git-blame-ignore-revs`).
- A renamed public name (method, type, enum value, header) is a public API change: `gate.api` and skill `semver-abi`.
  Renaming only a public parameter is source compatible, but its documentation changes with it.
- Before a rename pass, check the open branches and worktrees: a rename in a file another branch changes makes its
  rebase conflict.
- After a rename that touches the public API, rebuild the project's own tools and testers.

## Checking
Run the project's naming check (its command is in "Project facts") on the files you changed before you hand the
change over: the change must not add findings. Without a check script, review the names of the diff against this
skill.
