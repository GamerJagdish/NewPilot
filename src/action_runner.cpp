#include "action_runner.h"
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shobjidl.h>


static std::wstring expand_env(const std::wstring& input) {
    if (input.empty()) return L"";
    DWORD size = ExpandEnvironmentStringsW(input.c_str(), NULL, 0);
    if (size == 0) return input;
    std::wstring result(size, L'\0');
    ExpandEnvironmentStringsW(input.c_str(), &result[0], size);
    if (!result.empty() && result.back() == L'\0') {
        result.pop_back();
    }
    return result;
}

void ActionRunner::release_stuck_modifiers() {
    Sleep(50); // Allow physical hardware Copilot key (Win+Shift+F23) to settle

    INPUT inputs[8] = {};
    int count = 0;

    WORD mods[] = { VK_LWIN, VK_RWIN, VK_CONTROL, VK_SHIFT, VK_MENU };
    for (WORD vk : mods) {
        if ((GetAsyncKeyState(vk) & 0x8000) != 0) {
            inputs[count].type = INPUT_KEYBOARD;
            inputs[count].ki.wVk = vk;
            inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
            count++;
        }
    }

    if (count > 0) {
        SendInput(count, inputs, sizeof(INPUT));
        Sleep(20);
    }
}

bool ActionRunner::send_menu_key() {
    release_stuck_modifiers();
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_APPS;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_APPS;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    return SendInput(2, inputs, sizeof(INPUT)) == 2;
}

bool ActionRunner::send_hotkey(WORD virtual_key, DWORD modifiers) {
    release_stuck_modifiers();

    INPUT inputs[10] = {};
    int count = 0;

    if (modifiers & MOD_CONTROL) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_CONTROL;
        count++;
    }
    if (modifiers & MOD_SHIFT) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_SHIFT;
        count++;
    }
    if (modifiers & MOD_ALT) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_MENU;
        count++;
    }
    if (modifiers & MOD_WIN) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_LWIN;
        count++;
    }

    inputs[count].type = INPUT_KEYBOARD;
    inputs[count].ki.wVk = virtual_key;
    count++;

    inputs[count].type = INPUT_KEYBOARD;
    inputs[count].ki.wVk = virtual_key;
    inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
    count++;

    if (modifiers & MOD_WIN) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_LWIN;
        inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
        count++;
    }
    if (modifiers & MOD_ALT) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_MENU;
        inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
        count++;
    }
    if (modifiers & MOD_SHIFT) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_SHIFT;
        inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
        count++;
    }
    if (modifiers & MOD_CONTROL) {
        inputs[count].type = INPUT_KEYBOARD;
        inputs[count].ki.wVk = VK_CONTROL;
        inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
        count++;
    }

    return SendInput(count, inputs, sizeof(INPUT)) == (UINT)count;
}

bool ActionRunner::launch_shell_app(const std::wstring& aumid) {
    if (aumid.empty()) return false;

    release_stuck_modifiers();

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    IApplicationActivationManager* pAppAct = NULL;
    hr = CoCreateInstance(CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER, IID_IApplicationActivationManager, (void**)&pAppAct);
    if (SUCCEEDED(hr) && pAppAct) {
        DWORD pid = 0;
        hr = pAppAct->ActivateApplication(aumid.c_str(), L"", AO_NONE, &pid);
        pAppAct->Release();
        if (SUCCEEDED(hr)) {
            CoUninitialize();
            return true;
        }
    }
    CoUninitialize();

    // Fallback via shell:AppsFolder
    std::wstring shell_path = L"shell:AppsFolder\\" + aumid;
    HINSTANCE hInst = ShellExecuteW(NULL, L"open", shell_path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return ((INT_PTR)hInst > 32);
}

bool ActionRunner::launch_file(const KeyAction& action) {
    if (action.path.empty()) return false;

    release_stuck_modifiers();

    std::wstring expanded_path = expand_env(action.path);
    std::wstring expanded_args = expand_env(action.arguments);
    std::wstring expanded_dir = expand_env(action.working_dir);

    if (expanded_dir.empty()) {
        size_t last_slash = expanded_path.find_last_of(L"\\/");
        if (last_slash != std::wstring::npos) {
            expanded_dir = expanded_path.substr(0, last_slash);
        }
    }

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_DEFAULT;
    sei.hwnd = NULL;
    sei.lpVerb = L"open";
    sei.lpFile = expanded_path.c_str();
    sei.lpParameters = expanded_args.empty() ? NULL : expanded_args.c_str();
    sei.lpDirectory = expanded_dir.empty() ? NULL : expanded_dir.c_str();
    sei.nShow = SW_SHOWNORMAL;

    return ShellExecuteExW(&sei) == TRUE;
}

bool ActionRunner::run(const KeyAction& action) {
    if (!action.is_configured()) return false;

    switch (action.kind) {
        case ActionKind::MenuKey:
            return send_menu_key();

        case ActionKind::ShellApp:
            return launch_shell_app(action.aumid);

        case ActionKind::File:
        case ActionKind::Command:
            return launch_file(action);

        case ActionKind::Hotkey:
            return send_hotkey(action.virtual_key, action.modifiers);

        default:
            return false;
    }
}
