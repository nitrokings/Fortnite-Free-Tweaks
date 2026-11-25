# AGENTS.md - how to continue VNM TWEAKS

Read this before changing anything. It exists because the project owner does not
write C++ and relies on an agent picking this up cold.

## Non-negotiable rules

1. **Never touch game processes, memory, files or input.** No `OpenProcess` on a
   game, no injection, no macros, no config writers for anti-cheat-protected
   games. Vanguard, Hyperion and EAC are kernel-level; a single suspect handle
   gets users banned. Everything the app does is a normal Windows setting.
2. **Never disable security features.** No Defender, firewall, VBS/HVCI, Secure
   Boot, TPM or Windows Update changes, and none of them may appear in a preset.
   Report their state; let the user act.
3. **No tweak without undo.** An item that cannot implement `detect()`,
   `preview()`, `apply()` and `undo()` is a *guide*: render it as text, with no
   Apply button. Do not ship fake buttons.
4. **No performance claim that was not measured on that machine.** If a delta is
   inside the noise floor, the UI says "no measurable difference".
5. **Do not put PII or secrets in the journal or logs.** The journal records
   registry paths and values only.

## Where things live

| Path | Purpose |
|---|---|
| `src/core/` | Pure C++20. No Qt, no Win32, no exceptions in the API. Everything here is unit tested. |
| `src/core/catalog.cpp` | **The allowlist.** Every actionable item must be declared here or `VNMHelper` will refuse it. |
| `src/core/journal.*` | Append-only JSONL journal, hash chain, undo support. |
| `src/core/gate.*` | Restore-point decision table. |
| `src/core/plan.*` | Plan file format + strict validation against the catalogue. |
| `src/platform/win/` | The only Windows-specific code. Non-Windows builds get stubs so CI still compiles. |
| `src/helper/` | `VNMHelper.exe`: `--apply`, `--undo`, `--ensure-restore-point`, `--enable-protection`, `--log-ack`. |
| `src/app/` | Qt 6 Widgets UI. No business logic, no registry access beyond reading state via core. |
| `tests/` | Hand-rolled test harness (`test_framework.h`), no third-party dependency. |

## Adding a tweak (the whole checklist)

1. Add a `TweakSpec` entry to `src/core/catalog.cpp` (id, target, value name,
   type, risk, revert).
2. Implement the item in `src/core/` against `IRegistry` + `Journal`, following
   `gamemode.cpp`: detect, preview, apply (with rollback when the journal write
   fails), undo (with an audit entry that sets `undoOf`).
3. Add unit tests using `FakeRegistry` and `TempDir`. Required cases: detect
   unset, detect set, apply, undo restoring an existing value, undo removing a
   value that did not exist, refusal of a tampered plan, rollback on journal
   failure.
4. Wire the card in `src/app/pages.cpp` (detect on show, apply via a plan file,
   undo via the journal id).
5. Update `docs/ROADMAP.md` status and the README table.

## Build and verify

```bash
# Linux: core + tests (fast, what CI runs first)
cmake -S . -B build -DVNM_BUILD_GUI=OFF && cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Windows: full app
cmake -S . -B build && cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

CI (`.github/workflows/build.yml`) runs both. A change is not done until the
Windows job is green and the artifact installs.

## Things that are deliberately unusual

- **The plan file is not JSON-generic**: `src/core/json_lite` only parses a flat
  object of string values. Keep it that way; it is a security boundary.
- **The journal is per user** (`%LOCALAPPDATA%\VNMTweaks\journal`) because
  release 0.1 only writes `HKCU`. Moving to machine-wide tweaks means moving the
  journal and its ACLs at the same time.
- **Restore points are best-effort.** Windows allows one per 24 hours and many
  machines have System Protection off. The gate treats "a point already exists
  today" as satisfied, and the journal is the real safety net.
