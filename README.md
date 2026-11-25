# VNM TWEAKS

A Windows 11 game-performance app for people on modest PCs, built around one promise:

> **Every change is inspectable before it happens, recorded when it happens, and reversible afterwards. Nothing is claimed that was not measured.**

Nothing in VNM TWEAKS reads game memory, injects code, automates input, edits game files or touches network traffic to game servers. It only changes documented Windows settings, one at a time, through a single journaled path.

## Status: release 0.1

This is the first C++ release, and it is deliberately small. It exists to prove the whole pipeline works on a real machine:

| Piece | State |
|---|---|
| Qt 6 desktop shell with the module sidebar | Done |
| Change journal (JSONL, hash-chained) with per-item undo | Done |
| Restore-point gate (blocks writes, never blocks launch) | Done |
| `VNMHelper` - the only process that writes anything, allowlist-validated | Done |
| **Windows Performance -> Game Mode** - detect, preview, apply, undo | Done, the only real tweak |
| Unit tests + CI on Windows and Linux | Done |
| GPU, Network, Input, Games, Booster, Security Centre, Monitor, Cleanup, Presets, Aim Trainer | **Not built yet** - the pages say so and have no Apply button |

The previous Python prototype lives in `legacy/` for reference only. It is not maintained and not shipped.

## How to get a build

You do not need a C++ toolchain. Every push builds the app on GitHub Actions:

1. Open the **Actions** tab, pick the latest `build` run.
2. Download the **VNMTweaks-0.1.0-windows-x64** artifact.
3. Unzip it somewhere you can write to (not `C:\Program Files`) and run `VNMTweaks.exe`.

The binary is **unsigned** in 0.1, so SmartScreen will warn. Choose "More info" -> "Run anyway" *only* if you downloaded it from this repository's Actions page.

## How the safety model works

- **Detect before write.** The app reads the current value and shows it before anything changes.
- **Restore point gate.** Before the first change of a session, `VNMHelper` creates a Windows restore point. Windows allows only one per 24 hours, so an existing point from today satisfies the gate. If System Protection is off, the app offers to enable it. Read-only features always open; only writes are gated.
- **Journal.** Every change appends a line recording the target, the exact previous value and the new one. The lines are hash-chained, so an edited or truncated journal is detectable.
- **Undo.** Restoring uses the journal, not System Restore, so a single item can be undone months later.
- **Allowlist.** The helper compares every requested change against a compiled-in catalogue of legal targets and refuses anything else.

## Building it yourself (optional)

Requires CMake 3.24+, a C++20 compiler, and Qt 6.5+ if you want the GUI.

```bash
cmake -S . -B build -DVNM_BUILD_GUI=OFF   # core + tests only
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For the full app on Windows, install Qt 6.8 (MSVC 2022 64-bit) and configure without `-DVNM_BUILD_GUI=OFF`.

## Layout

```
src/core        pure C++20 logic: journal, gate, plan validation, the tweak itself
src/platform    the only place that touches Windows APIs
src/helper      VNMHelper.exe - apply, undo, restore points
src/app         Qt 6 Widgets front end
tests           unit tests (no third-party test framework)
docs            architecture, roadmap, decisions
legacy          the retired Python prototype
```

## Licence

GPL-3.0. See [LICENSE](LICENSE). Qt is used under the LGPLv3 and is dynamically linked.
