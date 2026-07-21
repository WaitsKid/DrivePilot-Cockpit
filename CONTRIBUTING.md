# Contributing

This is primarily a personal portfolio project, but disciplined changes are welcome.

## Workflow

1. Create an Issue describing the change;
2. Create a branch such as `feature/rag-manual` or `fix/dms-reconnect`;
3. Keep one concern per commit;
4. Run `python scripts/pre_push_check.py`;
5. Run relevant pytest suites;
6. Confirm Qt builds from a clean directory;
7. Open a Pull Request with screenshots or logs when UI behavior changes.

## Code Boundaries

- QML: layout, visual state, animation and user interaction;
- C++: Qt integration, network, data, persistence and client-side validation;
- Python DMS: training, inference and fatigue state;
- Python Agent: model calls, sessions and tool orchestration;
- no C++ reverse lookup/manipulation of QML objects;
- no real secret in source code, examples or test fixtures.

## Commit Style

Use Conventional Commit-style prefixes:

```text
feat:
fix:
docs:
refactor:
test:
build:
chore:
```
