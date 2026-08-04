#include "tray_icon.h"
#include "config.h"
#include "action_runner.h"
#include "settings_ui.h"
#include "resource.h"
#include <shlobj.h>
#include <shellapi.h>
#include <appmodel.h>
#include <vector>

#define WM_TRAYICON (WM_USER + 100)
#define IDM_SETTINGS 1001
#define IDM_RUN_ACTION 1002
#define IDM_EXIT 1003
#define IDM_ABOUT 1004

static HWND g_hTrayWnd = NULL;
static NOTIFYICONDATAW g_nid = {};
static HANDLE g_hMutex = NULL;

bool TrayApplication::is_running_elsewhere() {
    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Local\\NewPilot.TrayMutex");
    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return true;
    }
    g_hMutex = hMutex;
    return false;
}

std::wstring TrayApplication::get_package_family_name() {
    UINT32 length = 0;
    LONG rc = GetPackageFamilyName(GetCurrentProcess(), &length, NULL);
    if (rc == ERROR_INSUFFICIENT_BUFFER && length > 0) {
        std::vector<wchar_t> buf(length);
        rc = GetPackageFamilyName(GetCurrentProcess(), &length, buf.data());
        if (rc == ERROR_SUCCESS) {
            return std::wstring(buf.data()); // Clean C-string constructor
        }
    }
    return L"";
}

bool TrayApplication::is_startup_enabled() {
    wchar_t startup_dir[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startup_dir))) {
        std::wstring link_path = std::wstring(startup_dir) + L"\\NewPilot.lnk";
        DWORD attribs = GetFileAttributesW(link_path.c_str());
        return (attribs != INVALID_FILE_ATTRIBUTES);
    }
    return false;
}

void TrayApplication::update_startup_shortcut(bool enable) {
    wchar_t startup_dir[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startup_dir))) return;

    std::wstring link_path = std::wstring(startup_dir) + L"\\NewPilot.lnk";

    if (!enable) {
        DeleteFileW(link_path.c_str());
        return;
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    IShellLinkW* psl = NULL;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl);
    if (SUCCEEDED(hr) && psl) {
        std::wstring pfn = get_package_family_name();
        if (!pfn.empty()) {
            psl->SetPath(L"explorer.exe");
            std::wstring args = L"shell:AppsFolder\\" + pfn + L"!Tray";
            psl->SetArguments(args.c_str());
        } else {
            wchar_t exe_path[MAX_PATH];
            GetModuleFileNameW(NULL, exe_path, MAX_PATH);
            psl->SetPath(exe_path);
            psl->SetArguments(L"--tray");
        }

        IPersistFile* ppf = NULL;
        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
            ppf->Save(link_path.c_str(), TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
}

void TrayApplication::ensure_tray_running() {
    HWND hExisting = FindWindowW(L"NewPilotTrayClass", NULL);
    if (hExisting != NULL) {
        return; // Already running
    }

    std::wstring pfn = get_package_family_name();
    if (!pfn.empty()) {
        std::wstring args = L"shell:AppsFolder\\" + pfn + L"!Tray";
        ShellExecuteW(NULL, L"open", L"explorer.exe", args.c_str(), NULL, SW_SHOWNORMAL);
    } else {
        wchar_t exe_path[MAX_PATH];
        GetModuleFileNameW(NULL, exe_path, MAX_PATH);
        ShellExecuteW(NULL, L"open", exe_path, L"--tray", NULL, SW_SHOWNORMAL);
    }
}

void TrayApplication::stop_tray_if_running() {
    HWND hExisting = FindWindowW(L"NewPilotTrayClass", NULL);
    if (hExisting != NULL) {
        SendMessageW(hExisting, WM_CLOSE, 0, 0);
    }
}

static LRESULT CALLBACK TrayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_TRAYICON:
            if (LOWORD(lParam) == WM_RBUTTONUP || lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, L"Settings...");
                AppendMenuW(hMenu, MF_STRING, IDM_RUN_ACTION, L"Run Action");
                AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"About / GitHub");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"Exit");

                SetForegroundWindow(hWnd);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
                DestroyMenu(hMenu);

                if (cmd == IDM_SETTINGS) {
                    SettingsUI::show(GetModuleHandle(NULL));
                } else if (cmd == IDM_RUN_ACTION) {
                    AppConfig config = AppConfig::load();
                    ActionRunner::run(config.tap_action);
                } else if (cmd == IDM_ABOUT) {
                    ShellExecuteW(NULL, L"open", L"https://github.com/GamerJagdish/newpilot", NULL, NULL, SW_SHOWNORMAL);
                } else if (cmd == IDM_EXIT) {
                    DestroyWindow(hWnd);
                }
            } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONDBLCLK) {
                SettingsUI::show(GetModuleHandle(NULL));
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            Shell_NotifyIconW(NIM_DELETE, &g_nid);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

int TrayApplication::run(HINSTANCE hInstance) {
    if (is_running_elsewhere()) return 0;

    WNDCLASSEXW wcex = { sizeof(wcex) };
    wcex.lpfnWndProc = TrayWndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"NewPilotTrayClass";
    RegisterClassExW(&wcex);

    g_hTrayWnd = CreateWindowExW(0, L"NewPilotTrayClass", L"NewPilot Tray Window", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!g_hTrayWnd) return 0;

    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hTrayWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wcscpy_s(g_nid.szTip, L"NewPilot Copilot Key Remapper");

    if (Shell_NotifyIconW(NIM_ADD, &g_nid)) {
        g_nid.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &g_nid);
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_hMutex) {
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }
    return (int)msg.wParam;
}
