# NTRO TWEAKS FREE

A clean Windows desktop application focused on safe, reversible Fortnite and system optimization guidance.

## Safety promise

NTRO TWEAKS FREE only includes legitimate performance settings, transparent system tools, and user-controlled toggles. It does **not** include cheats, macros, aim assist, gameplay automation, driver injection, hidden registry hacks, unsafe network stack changes, or forced Fortnite configuration edits.

Before any system-level action, the app asks:

> Do you want to create a Windows restore point before applying tweaks?

Choosing **Yes** runs Windows restore point creation through `Checkpoint-Computer`. Choosing **No** continues only after a warning.

## Sections

- **Fortnite Tweaks**: Performance Mode, fullscreen guidance, shadows, low/medium/competitive presets, motion blur, view distance, competitive preset preview, and reset guide.
- **Windows Performance**: Game Mode, startup apps review, background app review, power plan selector, and visual effects mode.
- **GPU Settings Guide**: NVIDIA Low Latency Mode, V-Sync, power management, texture filtering, and a competitive GPU preset preview.
- **Network Optimization**: DNS selector guidance, Ethernet/Wi-Fi tips, packet loss checklist, and ping stability checklist. No `netsh` or network stack modifications are used.
- **Input Responsiveness**: Enhance pointer precision guidance, DPI range, polling rate info, USB power saving guidance, and fullscreen optimization guide. No macros or automation.
- **Performance Monitor**: Lightweight CPU/RAM monitor, GPU/temperature availability notes, and a non-intrusive FPS-to-frame-time graph helper.
- **System Cleanup**: User temp cleaner, storage analyzer concept, Fortnite cache cleanup guide, and browser cache instructions.
- **Presets**: Potato Mode, Balanced Mode, and Competitive Mode, each previewed before applying.

## Run from source

```bat
py -3 src\ntro_tweaks_free.py
```

Or double-click:

```bat
tools\run_ntro_tweaks_free.bat
```

The app uses only the Python standard library.

## Build a Windows EXE

This repository includes Python source that can be packaged on Windows with PyInstaller:

```bat
py -3 -m pip install pyinstaller
py -3 -m PyInstaller --onefile --windowed --name "NTRO TWEAKS FREE" src\ntro_tweaks_free.py
```

The resulting executable will be created under `dist\NTRO TWEAKS FREE.exe`.

> Note: the current Linux container does not have PyInstaller or a Windows cross-compiler installed, so the EXE cannot be rebuilt here. Existing legacy `FortniteOptimizer.exe` artifacts are left untouched.
