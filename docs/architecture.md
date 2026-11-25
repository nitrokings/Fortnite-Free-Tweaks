# Architecture

## Shape

```
+--------------------- VNMTweaks.exe (Qt 6 Widgets, unelevated) ---------------------+
|  pages/         detect state, show diffs, call the helper                          |
|  vnm_core       catalog, journal, gate, plan validation, tweak implementations     |
+------------------------------------+-----------------------------------------------+
                                     | plan file + UAC (runas), result JSON
                                     v
+--------------------- VNMHelper.exe (console, elevated on demand) ------------------+
|  allowlist check -> IRegistry write -> journal append -> result JSON               |
+------------------------------------+-----------------------------------------------+
                                     v
                        HKEY_CURRENT_USER / System Restore
```

Three rules produce this shape:

1. The UI is never privileged. It cannot write registry values even if it wanted to.
2. The privileged binary is small and validates everything it is asked to do.
3. All logic that can be tested lives in `vnm_core`, which has no Qt and no Win32.

## The tweak contract

```cpp
DetectResult detect() const;                    // On / Off / Unknown, always with a reason
ChangePlan   preview(bool enable) const;        // exact target + before + after, never mutates
bool         apply(const ChangePlan&, error);   // write, then journal; roll back if journaling fails
bool         undo(const JournalEntry&, error);  // restore the recorded value, append an audit entry
```

`Unknown` is a first-class state: if a Windows build moves a value or the
machine refuses a read, the item reports `Unknown` instead of guessing.

## Journal

- One JSON object per line, one file per month, hash-chained with FNV-1a 64.
- The chain catches edits and truncation. It is tamper *evidence*, not
  cryptography - the threat model is a corrupted file or a dishonest support
  claim, not a targeted attacker with write access.
- Undo writes a second entry with `undoOf` pointing at the entry it reversed, so
  the history always shows both directions.
- `apply()` rolls the registry back if the journal write fails: a machine must
  never be changed without a record.

## Restore-point gate

| Situation | Decision |
|---|---|
| A point exists for today (Windows allows one per 24 h) | `Allow` |
| System Protection disabled | `EnableProtection` (offer, then retry) |
| Not elevated | `Elevate` |
| Otherwise | `CreatePoint` |
| Everything fails | Typed acknowledgement, logged verbatim; read-only mode still works |

Launching the app, scanning, the monitor and the aim trainer are never gated -
only writes are.

## Deliberate deviations from the planning document

| Planned | Built | Why |
|---|---|---|
| Catch2 v3 | 60-line harness in `tests/test_framework.h` | No dependency can break the build for an owner who cannot debug a toolchain. |
| nlohmann/json | `src/core/json_lite` (flat string objects only) | The subset is a security boundary; a full parser would accept shapes the helper must never receive. |
| Journal in `%ProgramData%` | `%LOCALAPPDATA%\VNMTweaks\journal` | 0.1 writes only `HKCU`, so the journal is per user and needs no ACL juggling. Machine-wide tweaks will move it. |
| Plans as arbitrary JSON | Fixed `version/itemId/target/valueName/valueType/before/after/session/restorePoint` | The helper rejects anything else, including unknown versions. |
| spdlog | Not yet | The journal already records what changed; structured app logging arrives with the diagnostics panel in 0.2. |

## Non-Windows builds

`src/platform/win/*.cpp` compiles to stubs on other hosts so the Linux CI job can
build and test `vnm_core` and the helper. The GUI also builds on Linux (Qt is
cross-platform) which is how the app is compile-checked before reaching Windows.
