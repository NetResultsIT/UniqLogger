# Agentify: skeletons of the documents

Use these when the maintainer chose to write a document now. Describe what is; mark what is planned as "planned"
and what is not decided as "not decided". Every statement must be checkable in the code or come from the
maintainer. Keep a document short (about 200 lines); a section with nothing specific to say is left out.

## Architecture document (default `docs/architecture.md`)

```markdown
# <Project> architecture

## Purpose and context
What the project does and for whom. The consumers and the contract each uses (or a pointer to the consumers list).

## Components
| Component | Responsibility | Folder | Depends on |
|---|---|---|---|

## Dependency rules
Who may use whom; what must never depend on what; where the boundaries to external systems are.

## Data
What is stored where, in which format; the schema and how it changes (migrations); what is personal data.

## Processes, threads and concurrency
What runs in parallel, how the parts communicate, the lock order.

## External systems
Each system, what is exchanged, who owns it.

## Errors, logging, configuration
How errors travel and are reported; what is logged; where the configuration comes from.

## Decisions
One line per design note, with its link and date.

## Not decided
The open questions, each with who decides.
```

## Naming rules (default `docs/naming.md`; not needed when a profile skill such as `qt-naming` covers them)

```markdown
# <Project> naming rules

Follows <the common style of the language, by name and version>. Only the differences and additions are listed.

| What | Rule | Example |
|---|---|---|

Files and folders: <rule>. Tests: <rule>.
Check: `<command>`; it runs on the changed files only.
Existing code that does not follow: renamed only when the change touches it, never in a mass rename.
```

## Design note (default `docs/design/<YYYY-MM-DD>-<topic>.md`)

```markdown
# <Topic>

Date: <YYYY-MM-DD>. Decided by: <name>.

## Problem
## Options considered
## Choice and why
## What changes for the users of the code and for the consumers
## Tests
```
