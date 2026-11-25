"""NTRO TWEAKS FREE - safe, reversible Fortnite and Windows optimization helper.

This application intentionally avoids cheats, driver injection, unsafe registry/network
hacks, forced Fortnite file edits, macros, and hidden changes.  Most Fortnite/GPU/network
items are transparent guide toggles that prepare a preview and tell the user exactly what
to do in the official settings UI.  The few system actions are reversible Windows settings
or safe cleanup actions, and they require confirmation before running.
"""

from __future__ import annotations

import ctypes
import ctypes.wintypes
import os
import platform
import shutil
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from tkinter import BOTH, END, LEFT, RIGHT, Canvas, Frame, StringVar, Text, Tk, Toplevel, messagebox, ttk
from tkinter import filedialog

APP_NAME = "NTRO TWEAKS FREE"
RESTORE_PROMPT = "Do you want to create a Windows restore point before applying tweaks?"
SAFETY_PROMISE = (
    "Safe and transparent only: no cheats, no hidden registry hacks, no netsh/network "
    "stack modifications, no driver injection, no macros, and no gameplay automation."
)

ACCENT = "#2563eb"
BG = "#f6f7fb"
PANEL = "#ffffff"
TEXT = "#172033"
MUTED = "#667085"
BORDER = "#d9e2f1"


@dataclass
class Tweak:
    title: str
    description: str
    kind: str = "toggle"
    options: tuple[str, ...] = ("Off", "On")
    default: str = "Off"
    action: str = "Guide only"
    safety: str = "Reversible from the same setting."
    command: list[str] | None = None
    requires_restore: bool = False
    value: StringVar | None = field(default=None, repr=False)


class WindowsMetrics:
    """Small stdlib-only CPU/RAM reader so the monitor works without dependencies."""

    class MEMORYSTATUSEX(ctypes.Structure):
        _fields_ = [
            ("dwLength", ctypes.c_ulong),
            ("dwMemoryLoad", ctypes.c_ulong),
            ("ullTotalPhys", ctypes.c_ulonglong),
            ("ullAvailPhys", ctypes.c_ulonglong),
            ("ullTotalPageFile", ctypes.c_ulonglong),
            ("ullAvailPageFile", ctypes.c_ulonglong),
            ("ullTotalVirtual", ctypes.c_ulonglong),
            ("ullAvailVirtual", ctypes.c_ulonglong),
            ("ullAvailExtendedVirtual", ctypes.c_ulonglong),
        ]

    def __init__(self) -> None:
        self._last_idle = self._last_kernel = self._last_user = None

    @staticmethod
    def _filetime_to_int(filetime: ctypes.wintypes.FILETIME) -> int:
        return (filetime.dwHighDateTime << 32) | filetime.dwLowDateTime

    def cpu_percent(self) -> float:
        if platform.system() != "Windows":
            try:
                load = os.getloadavg()[0]
                cpus = os.cpu_count() or 1
                return max(0.0, min(100.0, (load / cpus) * 100.0))
            except OSError:
                return 0.0
        idle = ctypes.wintypes.FILETIME()
        kernel = ctypes.wintypes.FILETIME()
        user = ctypes.wintypes.FILETIME()
        if not ctypes.windll.kernel32.GetSystemTimes(ctypes.byref(idle), ctypes.byref(kernel), ctypes.byref(user)):
            return 0.0
        idle_i = self._filetime_to_int(idle)
        kernel_i = self._filetime_to_int(kernel)
        user_i = self._filetime_to_int(user)
        if self._last_idle is None:
            self._last_idle, self._last_kernel, self._last_user = idle_i, kernel_i, user_i
            return 0.0
        idle_delta = idle_i - self._last_idle
        total_delta = (kernel_i - self._last_kernel) + (user_i - self._last_user)
        self._last_idle, self._last_kernel, self._last_user = idle_i, kernel_i, user_i
        if total_delta <= 0:
            return 0.0
        return max(0.0, min(100.0, 100.0 * (1.0 - idle_delta / total_delta)))

    def ram_percent(self) -> tuple[float, str]:
        if platform.system() != "Windows":
            try:
                pages = os.sysconf("SC_PHYS_PAGES")
                avail = os.sysconf("SC_AVPHYS_PAGES")
                size = os.sysconf("SC_PAGE_SIZE")
                total = pages * size
                free = avail * size
                used_pct = 100.0 * (1.0 - free / total)
                return used_pct, f"{(total-free)/1024**3:.1f} / {total/1024**3:.1f} GB"
            except Exception:
                return 0.0, "Unavailable"
        mem = self.MEMORYSTATUSEX()
        mem.dwLength = ctypes.sizeof(self.MEMORYSTATUSEX)
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(mem)):
            used = mem.ullTotalPhys - mem.ullAvailPhys
            return float(mem.dwMemoryLoad), f"{used/1024**3:.1f} / {mem.ullTotalPhys/1024**3:.1f} GB"
        return 0.0, "Unavailable"


class NTROTweaksApp:
    def __init__(self, root: Tk) -> None:
        self.root = root
        self.root.title(APP_NAME)
        self.root.geometry("1180x760")
        self.root.minsize(980, 640)
        self.root.configure(bg=BG)
        self.metrics = WindowsMetrics()
        self.current_section = "Fortnite Tweaks"
        self.tweaks_by_section = self._build_tweaks()
        self.monitor_points: list[float] = []
        self._build_styles()
        self._build_shell()
        self.show_section(self.current_section)
        self._tick_monitor()

    def _build_styles(self) -> None:
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("TFrame", background=BG)
        style.configure("Card.TFrame", background=PANEL, relief="flat")
        style.configure("Sidebar.TFrame", background="#111827")
        style.configure("TLabel", background=BG, foreground=TEXT, font=("Segoe UI", 10))
        style.configure("Title.TLabel", background=BG, foreground=TEXT, font=("Segoe UI Semibold", 20))
        style.configure("Subtitle.TLabel", background=BG, foreground=MUTED, font=("Segoe UI", 10))
        style.configure("CardTitle.TLabel", background=PANEL, foreground=TEXT, font=("Segoe UI Semibold", 11))
        style.configure("CardText.TLabel", background=PANEL, foreground=MUTED, font=("Segoe UI", 9), wraplength=560)
        style.configure("Primary.TButton", background=ACCENT, foreground="white", borderwidth=0, padding=(14, 8), font=("Segoe UI Semibold", 10))
        style.map("Primary.TButton", background=[("active", "#1d4ed8")])
        style.configure("TButton", padding=(12, 7), font=("Segoe UI", 10))
        style.configure("TCheckbutton", background=PANEL, foreground=TEXT, font=("Segoe UI", 10))
        style.configure("TCombobox", padding=5)
        style.configure("Horizontal.TProgressbar", background=ACCENT, troughcolor="#e5e7eb")

    def _build_shell(self) -> None:
        self.sidebar = ttk.Frame(self.root, style="Sidebar.TFrame", width=235)
        self.sidebar.pack(side=LEFT, fill="y")
        self.sidebar.pack_propagate(False)
        brand = ttk.Label(self.sidebar, text="NTRO\nTWEAKS FREE", background="#111827", foreground="white", font=("Segoe UI Semibold", 18), justify="left")
        brand.pack(anchor="w", padx=22, pady=(24, 12))
        safety = ttk.Label(self.sidebar, text="Safe • reversible • transparent", background="#111827", foreground="#bfdbfe", font=("Segoe UI", 9))
        safety.pack(anchor="w", padx=22, pady=(0, 18))
        self.nav_buttons: dict[str, ttk.Button] = {}
        for name in self.tweaks_by_section:
            btn = ttk.Button(self.sidebar, text=name, command=lambda n=name: self.show_section(n))
            btn.pack(fill="x", padx=14, pady=3)
            self.nav_buttons[name] = btn
        footer = ttk.Label(self.sidebar, text="No cheats. No macros.\nNo unsafe network hacks.", background="#111827", foreground="#9ca3af", font=("Segoe UI", 9), justify="left")
        footer.pack(side="bottom", anchor="w", padx=22, pady=22)
        self.content = ttk.Frame(self.root, style="TFrame")
        self.content.pack(side=RIGHT, expand=True, fill=BOTH)

    def _build_tweaks(self) -> dict[str, list[Tweak]]:
        return {
            "Fortnite Tweaks": [
                Tweak("Performance Mode", "Use Fortnite's official Rendering Mode: Performance to reduce CPU/GPU load on lower-end PCs.", action="Open Fortnite Video settings and set Rendering Mode to Performance."),
                Tweak("Fullscreen mode", "Prefer Fullscreen or Windowed Fullscreen for stable frame pacing depending on your display behavior.", options=("Windowed Fullscreen", "Fullscreen"), default="Fullscreen"),
                Tweak("Disable shadows", "Turning shadows off is one of the safest ways to improve FPS while keeping gameplay legitimate.", default="On"),
                Tweak("Graphics preset", "Choose a safe graphics baseline. Competitive keeps visibility while reducing expensive effects.", kind="select", options=("Low", "Medium", "Competitive"), default="Competitive"),
                Tweak("Motion blur", "Motion blur adds visual smoothing but can reduce clarity and add perceived input delay.", default="Off"),
                Tweak("View distance", "Lower view distance can improve FPS; Far keeps more world visibility at a moderate cost.", kind="select", options=("Low", "Medium", "Far"), default="Medium"),
            ],
            "Windows Performance": [
                Tweak("Game Mode", "Windows Game Mode prioritizes game processes and reduces background interruptions.", action="Opens Windows Game Mode settings.", command=["start", "ms-settings:gaming-gamemode"], requires_restore=True),
                Tweak("Startup apps review", "Review and disable launch-at-login apps you do not need while gaming.", action="Opens Windows Startup Apps settings.", command=["start", "ms-settings:startupapps"], requires_restore=True),
                Tweak("Background apps limiter", "Use Windows app permissions and Focus Assist to reduce background activity before playing.", action="Opens Installed Apps settings for user-controlled review.", command=["start", "ms-settings:appsfeatures"], requires_restore=True),
                Tweak("Power plan", "High Performance can reduce clock ramp-up delays; Balanced is better for laptops and daily use.", kind="select", options=("Balanced", "High Performance"), default="Balanced", action="Opens Power Options; no hidden powercfg changes are made.", command=["control", "powercfg.cpl"], requires_restore=True),
                Tweak("Visual effects", "Best performance reduces animations and shadows in Windows for a lighter desktop.", kind="select", options=("Best appearance", "Best performance"), default="Best performance", action="Opens Performance Options UI.", command=["SystemPropertiesPerformance.exe"], requires_restore=True),
            ],
            "GPU Settings Guide": [
                Tweak("NVIDIA Low Latency Mode", "Use the NVIDIA Control Panel to choose Auto, On, or Off. On can reduce render queue latency.", kind="select", options=("Auto", "On", "Off"), default="On"),
                Tweak("V-Sync", "Turning V-Sync off usually lowers latency; use G-SYNC/FreeSync settings if available.", default="Off"),
                Tweak("Power management mode", "Performance can reduce GPU downclocking during matches; Balanced saves power.", kind="select", options=("Performance", "Balanced"), default="Performance"),
                Tweak("Texture filtering quality", "Performance favors FPS; Quality favors image quality.", kind="select", options=("Performance", "Quality"), default="Performance"),
            ],
            "Network Optimization": [
                Tweak("DNS provider", "Choose a reputable DNS provider in Windows adapter settings. This does not modify ping to game servers directly.", kind="select", options=("Automatic", "Cloudflare (1.1.1.1)", "Google (8.8.8.8)"), default="Automatic", action="Guide only; no netsh or network stack commands."),
                Tweak("Ethernet vs Wi-Fi", "Ethernet is usually lower jitter. If using Wi-Fi, prefer 5/6 GHz, strong signal, and minimal interference."),
                Tweak("Packet loss checklist", "Read-only checklist: test another cable, restart router, close downloads, check ISP status, test Epic status."),
                Tweak("Ping stability checklist", "Read-only checklist: use nearest matchmaking region, avoid VPNs, pause cloud sync, and test at different times."),
            ],
            "Input Responsiveness": [
                Tweak("Enhance pointer precision", "Disable mouse acceleration for consistent aim muscle memory. Opens official Mouse settings.", action="Opens Mouse Properties.", command=["control", "main.cpl"], requires_restore=True),
                Tweak("DPI guidance", "Pick a mouse DPI range and tune in-game sensitivity. This app does not automate aiming.", kind="select", options=("400", "800", "1200", "1600"), default="800"),
                Tweak("Polling rate info", "125 Hz = 8 ms, 500 Hz = 2 ms, 1000 Hz = 1 ms. Configure only in your mouse software.", kind="select", options=("125 Hz", "500 Hz", "1000 Hz"), default="1000 Hz"),
                Tweak("USB power saving", "Prevent Windows from powering down USB devices if your mouse disconnects or stutters.", action="Opens Device Manager/Power Options guide; no hidden edits.", command=["control", "powercfg.cpl"], requires_restore=True),
                Tweak("Fullscreen optimization", "Per-app Compatibility setting that can help or hurt depending on the game and system.", action="Guide only per Fortnite shortcut properties."),
            ],
            "Performance Monitor": [],
            "System Cleanup": [
                Tweak("Temporary files cleaner", "Safely remove user temp files that are not currently locked by Windows.", action="Deletes files from your user TEMP folder after confirmation.", requires_restore=True),
                Tweak("Storage analyzer", "Choose a folder to summarize large files so you can decide what to remove.", action="Read-only scan."),
                Tweak("Fortnite cache cleanup guide", "Shows safe Epic/Fortnite cache locations to review; does not delete game files automatically."),
                Tweak("Browser cache cleanup", "Opens your browser settings instructions; browser data is only removed by you."),
            ],
            "Presets": [],
        }

    def clear_content(self) -> None:
        for child in self.content.winfo_children():
            child.destroy()

    def show_section(self, name: str) -> None:
        self.current_section = name
        self.clear_content()
        header = ttk.Frame(self.content, style="TFrame")
        header.pack(fill="x", padx=28, pady=(24, 8))
        ttk.Label(header, text=name, style="Title.TLabel").pack(anchor="w")
        ttk.Label(header, text=SAFETY_PROMISE, style="Subtitle.TLabel").pack(anchor="w", pady=(4, 0))
        if name == "Performance Monitor":
            self._render_monitor()
        elif name == "Presets":
            self._render_presets()
        else:
            self._render_tweak_cards(name)

    def _render_tweak_cards(self, section: str) -> None:
        actions = ttk.Frame(self.content, style="TFrame")
        actions.pack(fill="x", padx=28, pady=(0, 8))
        if section == "Fortnite Tweaks":
            ttk.Button(actions, text="Apply recommended competitive preset", style="Primary.TButton", command=self.apply_fortnite_competitive).pack(side=LEFT, padx=(0, 8))
            ttk.Button(actions, text="Reset Fortnite settings (guide)", command=self.reset_fortnite_guide).pack(side=LEFT)
        if section == "GPU Settings Guide":
            ttk.Button(actions, text="Apply recommended competitive GPU preset", style="Primary.TButton", command=self.apply_gpu_competitive).pack(side=LEFT, padx=(0, 8))
            ttk.Button(actions, text="Open NVIDIA Control Panel", command=lambda: self.safe_run(["nvcplui.exe"], restore=False)).pack(side=LEFT)
        if section == "System Cleanup":
            ttk.Button(actions, text="Run selected cleanup action", style="Primary.TButton", command=self.run_cleanup_action).pack(side=LEFT, padx=(0, 8))
        body = ttk.Frame(self.content, style="TFrame")
        body.pack(expand=True, fill=BOTH, padx=28, pady=8)
        for tweak in self.tweaks_by_section[section]:
            self._card(body, tweak)

    def _card(self, parent: Frame, tweak: Tweak) -> None:
        card = ttk.Frame(parent, style="Card.TFrame", padding=16)
        card.pack(fill="x", pady=7)
        left = ttk.Frame(card, style="Card.TFrame")
        left.pack(side=LEFT, expand=True, fill="x")
        ttk.Label(left, text=tweak.title, style="CardTitle.TLabel").pack(anchor="w")
        ttk.Label(left, text=tweak.description, style="CardText.TLabel").pack(anchor="w", pady=(4, 2))
        ttk.Label(left, text=f"Action: {tweak.action}  •  Safety: {tweak.safety}", style="CardText.TLabel").pack(anchor="w")
        if tweak.value is None:
            tweak.value = StringVar(value=tweak.default)
        controls = ttk.Frame(card, style="Card.TFrame")
        controls.pack(side=RIGHT, padx=(18, 0))
        if tweak.kind == "select":
            box = ttk.Combobox(controls, values=tweak.options, textvariable=tweak.value, state="readonly", width=22)
            box.pack(pady=(0, 8))
        else:
            ttk.Checkbutton(controls, textvariable=tweak.value, variable=tweak.value, onvalue="On", offvalue="Off").pack(pady=(0, 8))
        ttk.Button(controls, text="Apply", command=lambda t=tweak: self.apply_tweak(t)).pack(fill="x")

    def _confirm(self, title: str, detail: str, system_level: bool) -> bool:
        if system_level:
            answer = messagebox.askyesnocancel("Create restore point", RESTORE_PROMPT + "\n\nYes is recommended.")
            if answer is None:
                return False
            if answer:
                self.create_restore_point()
            else:
                if not messagebox.askokcancel("Continue without restore point", "You chose not to create a restore point. Continue only if you understand how to reverse this setting."):
                    return False
        return messagebox.askokcancel(title, detail + "\n\nThis app will only perform the transparent action shown above.")

    def create_restore_point(self) -> None:
        if platform.system() != "Windows":
            messagebox.showwarning("Restore point unavailable", "Windows restore points can only be created on Windows. Continuing in guide/demo mode.")
            return
        cmd = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", "Checkpoint-Computer -Description 'NTRO Tweaks Free restore point' -RestorePointType 'MODIFY_SETTINGS'"]
        try:
            subprocess.run(cmd, check=True, capture_output=True, text=True)
            messagebox.showinfo("Restore point", "Windows restore point creation was requested successfully.")
        except subprocess.CalledProcessError as exc:
            messagebox.showwarning("Restore point warning", f"Windows could not create the restore point. You may need Administrator rights or System Protection enabled.\n\n{exc.stderr or exc.stdout}")

    def apply_tweak(self, tweak: Tweak) -> None:
        detail = f"{tweak.title}: {tweak.value.get() if tweak.value else tweak.default}\n\n{tweak.description}\n\n{tweak.action}"
        if tweak.title == "Temporary files cleaner":
            self.clean_temp_files()
            return
        if tweak.title == "Storage analyzer":
            self.storage_analyzer()
            return
        if tweak.title == "Browser cache cleanup":
            self.show_browser_cache_guide()
            return
        if not self._confirm(f"Apply {tweak.title}?", detail, tweak.requires_restore):
            return
        if tweak.command:
            self.safe_run(tweak.command, restore=False)
        else:
            self.show_guide(tweak.title, detail)

    def safe_run(self, command: list[str], restore: bool = False) -> None:
        if restore and not self._confirm("Run safe Windows action?", "The selected action opens an official Windows settings tool.", True):
            return
        if platform.system() != "Windows":
            messagebox.showinfo("Windows action", f"This action runs on Windows:\n{' '.join(command)}")
            return
        try:
            if command[0] == "start":
                os.startfile(command[1])  # type: ignore[attr-defined]
            else:
                subprocess.Popen(command)
        except Exception as exc:
            messagebox.showerror("Action failed", str(exc))

    def show_guide(self, title: str, body: str) -> None:
        win = Toplevel(self.root)
        win.title(title)
        win.geometry("620x420")
        text = Text(win, wrap="word", font=("Segoe UI", 10), padx=14, pady=14)
        text.pack(expand=True, fill=BOTH)
        text.insert(END, body + "\n\nRecommended path: use the official Fortnite, Windows, GPU control panel, or browser settings UI. No hidden changes were made.")
        text.configure(state="disabled")

    def apply_fortnite_competitive(self) -> None:
        preview = (
            "Competitive Fortnite preset preview:\n"
            "• Performance Mode: On\n• Fullscreen: Fullscreen\n• Shadows: Off\n"
            "• Graphics preset: Competitive\n• Motion blur: Off\n• View distance: Medium\n\n"
            "This is instruction-based only. It does not edit Fortnite config files."
        )
        if messagebox.askokcancel("Preview competitive preset", preview):
            for t in self.tweaks_by_section["Fortnite Tweaks"]:
                values = {"Performance Mode": "On", "Fullscreen mode": "Fullscreen", "Disable shadows": "On", "Graphics preset": "Competitive", "Motion blur": "Off", "View distance": "Medium"}
                if t.value is None:
                    t.value = StringVar(value=t.default)
                t.value.set(values.get(t.title, t.default))
            self.show_guide("Competitive Fortnite preset", preview)

    def reset_fortnite_guide(self) -> None:
        self.show_guide("Reset Fortnite settings", "Open Fortnite → Settings → Video and manually restore defaults or use Epic Games Launcher → Library → Fortnite → Manage → Verify. This app does not force-edit or delete Fortnite configuration files.")

    def apply_gpu_competitive(self) -> None:
        preview = "Competitive GPU preset preview:\n• Low Latency Mode: On\n• V-Sync: Off\n• Power management: Performance\n• Texture filtering: Performance\n\nApply these manually in NVIDIA/AMD/Intel control panels."
        if messagebox.askokcancel("Preview GPU preset", preview):
            for t in self.tweaks_by_section["GPU Settings Guide"]:
                values = {"NVIDIA Low Latency Mode": "On", "V-Sync": "Off", "Power management mode": "Performance", "Texture filtering quality": "Performance"}
                if t.value is None:
                    t.value = StringVar(value=t.default)
                t.value.set(values.get(t.title, t.default))
            self.show_guide("Competitive GPU preset", preview)

    def _render_monitor(self) -> None:
        cards = ttk.Frame(self.content, style="TFrame")
        cards.pack(fill="x", padx=28, pady=12)
        self.cpu_label = self.metric_card(cards, "CPU usage", "0%")
        self.gpu_label = self.metric_card(cards, "GPU usage", "Unavailable without vendor tools")
        self.ram_label = self.metric_card(cards, "RAM usage", "0%")
        self.temp_label = self.metric_card(cards, "Temperature", "Unavailable if sensors are not exposed")
        fps_card = ttk.Frame(self.content, style="Card.TFrame", padding=16)
        fps_card.pack(fill="x", padx=28, pady=8)
        ttk.Label(fps_card, text="FPS / frame time helper", style="CardTitle.TLabel").pack(anchor="w")
        ttk.Label(fps_card, text="Use Fortnite's built-in FPS counter or trusted overlays. Enter FPS here to visualize frame time without injecting overlays into the game.", style="CardText.TLabel").pack(anchor="w", pady=4)
        self.fps_var = StringVar(value="144")
        row = ttk.Frame(fps_card, style="Card.TFrame")
        row.pack(anchor="w", pady=6)
        ttk.Combobox(row, values=("60", "120", "144", "165", "240", "360"), textvariable=self.fps_var, width=8).pack(side=LEFT)
        ttk.Button(row, text="Add frame-time point", command=self.add_frame_point).pack(side=LEFT, padx=8)
        self.graph = Canvas(self.content, height=210, bg="white", highlightthickness=1, highlightbackground=BORDER)
        self.graph.pack(fill="x", padx=28, pady=12)
        self._draw_graph()

    def metric_card(self, parent: Frame, title: str, value: str) -> ttk.Label:
        card = ttk.Frame(parent, style="Card.TFrame", padding=14)
        card.pack(side=LEFT, expand=True, fill="x", padx=(0, 10))
        ttk.Label(card, text=title, style="CardText.TLabel").pack(anchor="w")
        label = ttk.Label(card, text=value, background=PANEL, foreground=TEXT, font=("Segoe UI Semibold", 16))
        label.pack(anchor="w", pady=(6, 0))
        return label

    def _tick_monitor(self) -> None:
        if hasattr(self, "cpu_label"):
            cpu = self.metrics.cpu_percent()
            ram, ram_text = self.metrics.ram_percent()
            self.cpu_label.configure(text=f"{cpu:.0f}%")
            self.ram_label.configure(text=f"{ram:.0f}% ({ram_text})")
        self.root.after(1500, self._tick_monitor)

    def add_frame_point(self) -> None:
        try:
            fps = float(self.fps_var.get())
            if fps <= 0:
                raise ValueError
            self.monitor_points.append(1000.0 / fps)
            self.monitor_points = self.monitor_points[-40:]
            self._draw_graph()
        except ValueError:
            messagebox.showerror("Invalid FPS", "Enter a positive FPS value.")

    def _draw_graph(self) -> None:
        if not hasattr(self, "graph"):
            return
        self.graph.delete("all")
        width = max(self.graph.winfo_width(), 760)
        height = 210
        self.graph.create_text(14, 16, anchor="w", text="Frame time graph (lower is smoother)", fill=TEXT, font=("Segoe UI", 10, "bold"))
        if not self.monitor_points:
            self.graph.create_text(width / 2, height / 2, text="Add FPS points to plot frame time", fill=MUTED, font=("Segoe UI", 11))
            return
        max_ms = max(max(self.monitor_points), 20)
        step = (width - 60) / max(1, len(self.monitor_points) - 1)
        points = []
        for i, ms in enumerate(self.monitor_points):
            x = 30 + i * step
            y = height - 24 - (ms / max_ms) * (height - 60)
            points.extend((x, y))
        self.graph.create_line(points, fill=ACCENT, width=2, smooth=True)
        for x, y in zip(points[::2], points[1::2]):
            self.graph.create_oval(x - 3, y - 3, x + 3, y + 3, fill=ACCENT, outline="")

    def _render_presets(self) -> None:
        body = ttk.Frame(self.content, style="TFrame")
        body.pack(expand=True, fill=BOTH, padx=28, pady=12)
        presets = {
            "🥔 Potato Mode": "Lowest safe visuals, Performance Mode, shadows off, motion blur off, view distance low, Best Performance visual effects.",
            "⚖ Balanced Mode": "Balanced Windows power, medium Fortnite view distance, stable visuals, background app review.",
            "🏆 Competitive Mode": "Performance-focused Fortnite/GPU suggestions, Game Mode review, mouse acceleration guidance, cleanup review.",
        }
        for title, desc in presets.items():
            card = ttk.Frame(body, style="Card.TFrame", padding=18)
            card.pack(fill="x", pady=8)
            ttk.Label(card, text=title, style="CardTitle.TLabel").pack(anchor="w")
            ttk.Label(card, text=desc, style="CardText.TLabel").pack(anchor="w", pady=5)
            ttk.Button(card, text="Preview and apply", style="Primary.TButton", command=lambda t=title, d=desc: self.apply_preset(t, d)).pack(anchor="e")

    def apply_preset(self, title: str, desc: str) -> None:
        preview = f"{title}\n\n{desc}\n\nOnly safe toggles and official settings guidance will be applied. System-level items still ask about a restore point."
        if not messagebox.askokcancel("Preset preview", preview):
            return
        if "Competitive" in title:
            self.apply_fortnite_competitive()
            self.apply_gpu_competitive()
        else:
            self.show_guide(title, preview)

    def clean_temp_files(self) -> None:
        if not self._confirm("Run cleanup?", "Temporary files cleaner removes unlocked files from your user TEMP directory only.", True):
            return
        temp_dir = Path(tempfile.gettempdir())
        removed = 0
        skipped = 0
        for item in temp_dir.iterdir():
            try:
                if item.is_dir():
                    shutil.rmtree(item)
                else:
                    item.unlink()
                removed += 1
            except Exception:
                skipped += 1
        messagebox.showinfo("Cleanup complete", f"Scanned: {temp_dir}\nRemoved unlocked items: {removed}\nSkipped locked/protected items: {skipped}")

    def storage_analyzer(self) -> None:
        folder = filedialog.askdirectory(title="Choose a folder to analyze")
        if not folder:
            return
        root_path = Path(folder)
        totals: list[tuple[int, Path]] = []
        for path in root_path.rglob("*"):
            try:
                if path.is_file():
                    totals.append((path.stat().st_size, path))
            except Exception:
                continue
        largest = sorted(totals, reverse=True)[:15]
        lines = [f"Read-only storage analyzer for: {root_path}", "", "Largest files:"]
        for size, path in largest:
            lines.append(f"{size / 1024 / 1024:8.1f} MB  {path}")
        lines.append("\nNo files were deleted. Use File Explorer if you decide to remove anything.")
        self.show_guide("Storage analyzer results", "\n".join(lines))

    def show_browser_cache_guide(self) -> None:
        self.show_guide("Browser cache cleanup", "Open your browser settings and search for cache or browsing data. Recommended: clear cached images/files only, keep passwords and cookies unless you intentionally want to sign out.")

    def run_cleanup_action(self) -> None:
        self.clean_temp_files()


def main() -> None:
    root = Tk()
    app = NTROTweaksApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
