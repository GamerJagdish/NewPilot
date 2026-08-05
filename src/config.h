#pragma once
#include <windows.h>
#include <string>

enum class ActionKind {
    MenuKey = 0,
    ShellApp = 1,
    File = 2,
    Hotkey = 3,
    Command = 4
};

struct KeyAction {
    ActionKind kind = ActionKind::MenuKey;
    std::wstring aumid;         // For ShellApp
    std::wstring path;          // For File or Command
    std::wstring arguments;     // For File or Command
    std::wstring working_dir;   // For File or Command
    WORD virtual_key = 0;       // For Hotkey
    DWORD modifiers = 0;        // For Hotkey (MOD_CONTROL, MOD_SHIFT, MOD_ALT, MOD_WIN)

    bool is_configured() const {
        switch (kind) {
            case ActionKind::MenuKey:
                return true;
            case ActionKind::ShellApp:
                return !aumid.empty();
            case ActionKind::File:
            case ActionKind::Command:
                return !path.empty();
            case ActionKind::Hotkey:
                return virtual_key != 0;
            default:
                return false;
        }
    }
};

struct AppConfig {
    KeyAction tap_action;
    bool show_tray_icon = false;
    bool auto_start_tray = false;

    static std::wstring get_config_dir();
    static std::wstring get_config_path();
    static bool has_config_file();
    static AppConfig load();
    bool save() const;
};
