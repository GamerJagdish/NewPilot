# NewPilot

> **Lightweight Copilot Key Remapper for Windows 11**  
> Native C++ (Win32) alternative to heavy .NET and Electron key remappers — **~140 KB installed size**, **< 5ms response time**, **< 2 MB RAM usage**.

<a href="https://apps.microsoft.com/detail/9NGW4QTD1SWF?referrer=appbadge&mode=full" target="_blank" rel="noopener noreferrer">
	<img src="https://get.microsoft.com/images/en-us%20dark.svg" width="200"/>
</a>

---

## Download & Installation

### Option 1: Official Microsoft Store (Recommended)

| Open in Microsoft Store | Direct .exe Installer Download |
| :---: | :---: |
| <a href="https://apps.microsoft.com/detail/9NGW4QTD1SWF?referrer=appbadge&mode=full" target="_blank" rel="noopener noreferrer"><img src="https://get.microsoft.com/images/en-us%20dark.svg" width="160" alt="Open in Store"/></a> | <a href="https://get.microsoft.com/installer/download/9NGW4QTD1SWF?referrer=appbadge" target="_self"><img src="https://img.shields.io/badge/Download-.exe-0078D4?style=for-the-badge&logo=windows" width="160" alt="Direct .exe Download"/></a> |

### Option 2: 1-Click PowerShell Web Installer
Open **PowerShell as Administrator** and execute:

```powershell
irm https://raw.githubusercontent.com/GamerJagdish/NewPilot/main/install.ps1 | iex
```

*(This command trusts the security certificate, installs the MSIX package, and opens the Settings configuration dialog).*

### Enabling NewPilot in Windows 11
1. Open **Windows 11 Settings**.
2. Go to **Bluetooth & devices** → **Keyboard**.
3. Under **Shortcuts and hotkeys**, locate **Customize Copilot key on keyboard**.
4. Select **Custom** → choose **NewPilot**.

---

## Why NewPilot?

Modern laptops and keyboards feature a dedicated **Windows 11 Copilot key** (or `Win+C`). While Windows Settings allows customizing this key, the picker is empty by default unless an application is packaged as an MSIX and declares the `com.microsoft.windows.copilotkeyprovider` extension.

Existing solutions rely on heavy runtimes (.NET SDK, Electron, WPF), introducing large binary footprints, startup latency, and persistent background memory usage. **NewPilot** is written in **pure native C++ (C++20 & Win32 API)**, offering instant keypress execution with zero background overhead.

### Benchmark Comparison

| Metric | Heavy .NET / Electron Alternatives | **NewPilot (Native C++)** | Improvement |
| :--- | :--- | :--- | :--- |
| **Installed App Size** | ~15 MB – 60 MB+ | **~140 KB** | **> 99% Smaller** |
| **MSIX Package Size** | ~15 MB – 50 MB | **~54 - 60 KB** | **> 99.5% Smaller** |
| **Keypress Latency** | ~100ms – 300ms (.NET CLR init) | **< 5ms** (Instantaneous) | **~30x Faster** |
| **Tray Memory Usage** | 30 MB – 80 MB RAM | **< 2 MB RAM** | **> 93% RAM Reduction** |
| **Dependencies** | .NET 10 SDK / Runtime | **Zero Runtimes** (Pure Win32 C++) | Native Windows OS API |

---

## Features

- **Context Menu Key (`VK_APPS`)**: Remaps the Copilot key to function as a right-click / context menu key.
- **Store & System App Launcher**: Launches any installed UWP or Windows Store application (Windows Terminal, Notepad, Calculator, VS Code, Spotify) via a simple app picker.
- **Custom Executable, File, or URL**: Opens any program, document, folder, or website with optional command-line arguments and working directory.
- **System Action Shortcuts**:
  - Snipping Tool (`Win + Shift + S`)
  - Task View (`Win + Tab`)
  - Mute / Unmute Audio (`VK_VOLUME_MUTE`)
  - Volume Up / Volume Down
  - Media Controls (Play / Pause / Track Skip)
  - Lock PC (`Win + L`)
- **Native GUI**: Built with Windows 11 Visual Styles (Common Controls v6) and anti-aliased Segoe UI typography.
- **Optional System Tray Icon**: Low-footprint background daemon with sign-in auto-start support.

---

## Project Structure

```
NewPilot/
├── CMakeLists.txt              # CMake build configuration
├── build.ps1                   # MSVC build, packaging & signing script
├── install.ps1                 # Automated certificate trust & sideload installer
├── packaging/
│   ├── AppxManifest.xml        # MSIX package manifest with copilotkeyprovider extension
│   └── resources/              # Icon assets & app.ico
├── scripts/
│   ├── generate_assets.ps1     # Generates tile PNG logos
│   └── generate_ico.ps1        # Generates app.ico resource
└── src/
    ├── main.cpp                # App entry point & AUMID launch router
    ├── action_runner.cpp       # Native C++ action execution engine
    ├── app_enumerator.cpp      # Shell API UWP/Store app enumerator
    ├── config.cpp              # Fast JSON config loader (%LOCALAPPDATA%\NewPilot)
    ├── settings_ui.cpp         # Native Win32 settings dialog GUI
    ├── tray_icon.cpp           # Low-resource system tray daemon
    ├── resource.h              # Resource identifiers
    └── resources.rc            # Embedded Win32 icon resource script
```

---

## License

Distributed under the [MIT License](LICENSE).
