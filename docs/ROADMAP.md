# Roadmap

Each release is usable on its own. Status is updated as work lands.

| Release | Contents | Status |
|---|---|---|
| **0.1** | Repo cleanup, CMake + Qt shell, journal + undo, restore-point gate, `VNMHelper`, Game Mode end to end, CI artifact | **Shipped in code; awaiting first green Windows build** |
| 0.2 | Rest of Windows Performance (power plan, visual effects, per-app fullscreen optimisation, mouse acceleration, HAGS), system info, diagnostics export | Planned |
| 0.3 | Measurement: PresentMon capture, A/B runner, report card with a real noise floor; GPU and Network modules | Planned |
| 0.4 | Game Booster: background consumer audit, suspend/resume, priority, dGPU preference, measured before/after | Planned |
| 0.5 | Security Centre: posture audit, autorun inventory, signature checks, VirusTotal lookups, Defender offline scan trigger | Planned |
| 0.6 | Per-game profiles (`games.json`), anti-cheat warnings, sensitivity converter | Planned |
| 0.7 | Aim trainer (Qt Quick 3D, separate window), SQLite progress, charts | Planned |
| 0.8 | Advanced mode: service audit, scheduled-task audit, timer experiments, `bcdedit` read-only | Planned |
| 0.9 | Hardened cleanup, storage analyser, reviewable presets, first-run wizard | Planned |
| 1.0 | Code signing, installer/uninstaller, docs site, opt-in crash reporting, update check | Planned |

## Deliberate ordering

- The safety spine (journal, gate, helper allowlist) comes before any second
  tweak, because every later item depends on it.
- Measurement (0.3) comes before the Booster (0.4), so the Booster can never ship
  a claim it cannot back up.
- The aim trainer (0.7) is last among the features: it is the most fun and the
  least load-bearing.
