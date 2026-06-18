# Domain docs

**Layout**: Single-context. One `CONTEXT.md` + `docs/adr/` at the repo root.

## Consumer rules

Skills that read domain artifacts (`improve-codebase-architecture`, `diagnosing-bugs`, `tdd`, `grill-with-docs`, `to-prd`) follow these rules:

1. Read `CONTEXT.md` at the repo root for domain language and project overview
2. Read `docs/adr/` for past architectural decisions relevant to the task
3. Write new ADRs to `docs/adr/` using the naming convention `ADR-<NNN>-<slug>.md`
4. Update `CONTEXT.md` when domain terms are refined or added
