# NewPilot 🚀

> **Ultra-Lightweight & Instant Copilot Key Remapper for Windows 11**  
> Native C++ (Win32) alternative to heavy .NET/Electron key remappers — **~140 KB installed size**, **< 5ms response time**, **< 2 MB RAM usage**.

<a href="https://apps.microsoft.com/detail/9NGW4QTD1SWF?referrer=appbadge&mode=full" target="_blank" rel="noopener noreferrer">
	<img src="https://get.microsoft.com/images/en-us%20dark.svg" width="200"/>
</a>

---

## 🛍️ Download & Installation

### Option 1: Official Microsoft Store (Recommended)

| Open in Microsoft Store | Direct .exe Installer Download |
| :---: | :---: |
| <a href="https://apps.microsoft.com/detail/9NGW4QTD1SWF?referrer=appbadge&mode=full" target="_blank" rel="noopener noreferrer"><img src="https://get.microsoft.com/images/en-us%20dark.svg" width="160" alt="Open in Store"/></a> | <a href="https://get.microsoft.com/installer/download/9NGW4QTD1SWF?referrer=appbadge" target="_self"><img src="https://img.shields.io/badge/Download-.exe-0078D4?style=for-the-badge&logo=windows" width="160" alt="Direct .exe Download"/></a> |

### Option 2: 1-Click PowerShell Web Installer
Open **PowerShell as Administrator** and paste this 1-line command:

```powershell
irm https://raw.githubusercontent.com/GamerJagdish/NewPilot/main/install.ps1 | iex
```

*(The 1-line installer automatically trusts the security certificate, cleans up old installations, installs the MSIX package, and opens the NewPilot Settings GUI).*

### Enabling NewPilot in Windows 11
1. Open **Windows 11 Settings**.
2. Go to **Bluetooth & devices** → **Keyboard**.
3. Under **Shortcuts and hotkeys**, locate **Customize Copilot key on keyboard**.
4. Select **Custom** → choose **NewPilot**.

---

## ⚡ Why NewPilot?

Modern laptops and keyboards feature a dedicated **Windows 11 Copilot key** (or `Win+C`). While Windows Settings allows customizing this key, the picker is nearly always empty unless an app is packaged as an MSIX and declares the `com.microsoft.windows.copilotkeyprovider` extension.

Other solutions rely on heavy runtimes (.NET 10, Electron, WPF, WinForms), introducing large binary sizes, startup latency, and memory bloat. **NewPilot** is written in **pure native C++ (C++20 & Win32 API)**, delivering near-zero resource consumption and instant keypress execution.

### Performance Benchmark Comparison

| Metric | Heavy .NET / Electron Alternatives | **NewPilot (Native C++)** | Improvement |
| :--- | :--- | :--- | :--- |
| **Installed App Size** | ~15 MB – 60 MB+ | **~140 KB** | **> 99% Smaller** |
| **MSIX Package Size** | ~15 MB – 50 MB | **~54 - 60 KB** | **> 99.5% Smaller** |
| **Keypress Latency** | ~100ms – 300ms (.NET CLR init) | **< 5ms** (Instantaneous) | **~30x Faster** |
| **Tray Memory Usage** | 30 MB – 80 MB RAM | **< 2 MB RAM** | **> 93% RAM Reduction** |
| **Dependencies** | .NET 10 SDK / Runtime | **Zero Runtimes** (Pure Win32 C++) | Native Windows OS API |

---

## ✨ Features

- **Context Menu Key (`VK_APPS`)**: Remap Copilot key to act as a right-click / context menu key.
- **Store & System App Launcher**: Launch any installed UWP or Windows Store app (Windows Terminal, Notepad, Calculator, VS Code, Spotify, etc.) via dropdown picker.
- **Custom Program / File / URL**: Open any executable, document, folder, or web link with custom command-line arguments and working directory.
- **Quick System Hotkeys**: Instant system actions:
  - Snipping Tool (`Win + Shift + S`)
  - Task View (`Win + Tab`)
  - Mute / Unmute Audio (`VK_VOLUME_MUTE`)
  - Volume Up / Volume Down
  - Play / Pause / Next / Prev Track
  - Lock PC (`Win + L`)
- **Modern Native GUI**: Built with Windows 11 Visual Styles (Common Controls v6) and anti-aliased Segoe UI typography.
- **Optional System Tray Icon**: Minimal background daemon with sign-in auto-start shortcut support.


---

## 📂 Project Structure

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
    ├── app_enumerator.cpp     # Shell API UWP/Store app enumerator
    ├── config.cpp              # Fast JSON config loader (%LOCALAPPDATA%\NewPilot)
    ├── settings_ui.cpp         # Native Win32 settings dialog GUI
    ├── tray_icon.cpp           # Low-resource system tray daemon
    ├── resource.h              # Resource identifiers
    └── resources.rc            # Embedded Win32 icon resource script
```

---

## 📄 License

Distributed under the [MIT License](LICENSE).
