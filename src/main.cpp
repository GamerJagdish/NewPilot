#include <windows.h>
#include <appmodel.h>
#include <string>
#include <algorithm>

#include "config.h"
#include "action_runner.h"
#include "settings_ui.h"
#include "tray_icon.h"

enum class ExecutionMode {
    KeyPress,
    Settings,
    Tray
};

static std::wstring get_aumid() {
    UINT32 length = 0;
    LONG rc = GetApplicationUserModelId(GetCurrentProcess(), &length, NULL);
    if (rc == ERROR_INSUFFICIENT_BUFFER && length > 0) {
        std::wstring aumid(length, L'\0');
        rc = GetApplicationUserModelId(GetCurrentProcess(), &length, &aumid[0]);
        if (rc == ERROR_SUCCESS) {
            if (!aumid.empty() && aumid.back() == L'\0') aumid.pop_back();
            return aumid;
        }
    }
    return L"";
}

static bool match_arg(int argc, wchar_t* argv[], const wchar_t* opt1, const wchar_t* opt2) {
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], opt1) == 0 || _wcsicmp(argv[i], opt2) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_protocol_activation(int argc, wchar_t* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (wcsstr(argv[i], L"newpilot-key:") != NULL) {
            return true;
        }
    }
    return false;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    ExecutionMode mode = ExecutionMode::KeyPress;

    if (match_arg(argc, argv, L"--settings", L"/settings")) {
        mode = ExecutionMode::Settings;
    } else if (match_arg(argc, argv, L"--tray", L"/tray")) {
        mode = ExecutionMode::Tray;
    } else if (match_arg(argc, argv, L"--key", L"/key")) {
        mode = ExecutionMode::KeyPress;
    } else if (is_protocol_activation(argc, argv)) {
        mode = ExecutionMode::KeyPress;
    } else {
        std::wstring aumid = get_aumid();
        if (!aumid.empty()) {
            if (aumid.length() >= 9 && _wcsicmp(aumid.substr(aumid.length() - 9).c_str(), L"!Settings") == 0) {
                mode = ExecutionMode::Settings;
            } else if (aumid.length() >= 5 && _wcsicmp(aumid.substr(aumid.length() - 5).c_str(), L"!Tray") == 0) {
                mode = ExecutionMode::Tray;
            } else if (aumid.length() >= 9 && _wcsicmp(aumid.substr(aumid.length() - 9).c_str(), L"!NewPilot") == 0) {
                mode = ExecutionMode::KeyPress;
            }
        }
    }

    if (argv) LocalFree(argv);

    switch (mode) {
        case ExecutionMode::Settings:
            SettingsUI::show(hInstance);
            return 0;

        case ExecutionMode::Tray:
            return TrayApplication::run(hInstance);

        case ExecutionMode::KeyPress:
        default: {
            AppConfig config = AppConfig::load();
            if (!config.tap_action.is_configured()) {
                SettingsUI::show(hInstance);
                return 0;
            }
            ActionRunner::run(config.tap_action);
            return 0;
        }
    }
}
