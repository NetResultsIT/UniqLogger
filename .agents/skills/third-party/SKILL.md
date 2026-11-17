---
name: third-party
description: Third-party components (library, package, tool, copied source, font, data set, agent skill) - look for an existing one before writing code, what to present to the user, the license procedure in five steps that ends with the user's explicit acknowledgement, pinning, updating and removing. Use when a task could be solved by existing code, before adding or updating any dependency, and when reviewing a change that adds one.
---

# Third-party components

A shared skill: the same text in every project. The project says in `AGENTS.md`, under "Project facts", where its
license texts and its dependency lists are, the command that reports the licenses of all dependencies
(transitive ones included) when the package manager has one, and whether a list of approved licenses exists.

## Reuse before writing

Before you write code that is not specific to the project (a file format, a protocol, an algorithm, a widget, a
retry policy, a command line parser), look for it, in this order:

1. the project itself, and the standard library and framework it already uses;
2. the components the project already depends on;
3. an existing third-party component.

When step 3 finds candidates, present them to the user; do not choose alone. For each candidate:

- name, exact version, license (SPDX id), where it comes from (URL);
- who maintains it, the date of the last release, open security advisories;
- what it brings with it: its own dependencies, size, the language level and platforms it needs, compared with the
  project's;
- how it would enter the project (package manager, prebuilt package, copied source);
- what writing it here would cost instead, and your recommendation.

Read these facts from the component's own repository or package page, with the URL. Do not state a license, a date
or a maintenance status from memory: say "not verified" when you could not read it.

Writing it here is the right answer when the need is a few dozen lines, when the component brings far more than is
needed, or when no candidate fits the license, the platforms or the language level. Say which.

Adding a component is a gate item in `AGENTS.md`: the user's explicit consent comes first, with the procedure below.

## License procedure

It applies to: a new dependency, code copied into the repository (files, snippets, reference implementations), a
tool or data file shipped with the product or its tests, and a new version of a component whose license changed.

1. Find the license of the exact version used, from the upstream source (not a summary site), with its SPDX id and
   the copyright lines. Check the files too: a repository can carry more than one license.
2. Add the full text where the project keeps license texts, in its format (one file per component named
   `<name>-<SPDX id>.txt` when the project has no format yet). A package manager brings transitive dependencies:
   run the project's license report before and after, and treat each new license in it like a direct one.
3. Compare it with the licenses already in the project and tell the user what you found: the same as one already
   used, or different. Flag what needs attention: copyleft (GPL, AGPL, the linking terms of LGPL), patent or
   field-of-use clauses, non-commercial terms, attribution duties (notices the product must ship), export
   restrictions, terms of service attached to a hosted service the component calls.
4. Stop. The user must explicitly acknowledge that the license fits the organization's policy: checked with the
   person responsible, or the same license as a component already approved. You may say that it is similar to one
   already used; that is never the approval. Do not commit the component before the acknowledgement.
5. Record the acknowledgement in the commit message and in the merge request description: component, version,
   SPDX id, "acknowledged by the user on <date>: <checked with ... | same as <component or license>>".

## Pinning, updating, removing

- With a package manager, the committed lock file pins the exact versions; keep the range style of the manifest
  the project already uses. Without one, pin a release number, a tag or a commit id, never a branch or "latest".
- Copied source stays unmodified, with a note next to it: origin URL, version or commit id, date, license. A local
  change to it is a separate commit, listed in that note.
- An update repeats step 1: licenses change between versions. Read the change log for breaking changes and run the
  project's tests before proposing it.
- When a component is removed, remove its license text and its entries in the dependency lists in the same change.

## What to report

The candidates considered and why each was kept or dropped, the facts you verified with their URLs, the facts you
could not verify, the license comparison, and the acknowledgement you are waiting for.
