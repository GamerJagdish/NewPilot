#include "settings_ui.h"
#include "app_enumerator.h"
#include "action_runner.h"
#include "tray_icon.h"
#include "resource.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <vector>

#pragma comment(lib, "comctl32.lib")

// Enable modern Windows Visual Styles (Common Controls v6)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Control IDs
#define IDC_COMBO_ACTION_KIND 2001

#define IDC_LBL_APP_PICKER    2002
#define IDC_COMBO_APP_PICKER  2003

#define IDC_LBL_PATH          2004
#define IDC_EDIT_PATH         2005
#define IDC_BTN_BROWSE        2006
#define IDC_LBL_ARGS          2007
#define IDC_EDIT_ARGS         2008

#define IDC_LBL_PRESET        2009
#define IDC_COMBO_PRESET      2010

#define IDC_CHK_SHOW_TRAY     2011
#define IDC_CHK_AUTO_START    2012
#define IDC_BTN_TEST          2013
#define IDC_BTN_SAVE          2014
#define IDC_BTN_CANCEL        2015
#define IDC_BTN_ABOUT         2016
#define IDC_BTN_OPEN_WIN_SETTINGS 2017

static AppConfig g_config;
static std::vector<AppInfo> g_installed_apps;
static HFONT g_hFont = NULL;

struct PresetHotkey {
    const wchar_t* name;
    WORD vk;
    DWORD mods;
};

static PresetHotkey g_presets[] = {
    { L"Snipping Tool (Win + Shift + S)", 'S', MOD_WIN | MOD_SHIFT },
    { L"Task View (Win + Tab)", VK_TAB, MOD_WIN },
    { L"Mute / Unmute Audio", VK_VOLUME_MUTE, 0 },
    { L"Volume Up", VK_VOLUME_UP, 0 },
    { L"Volume Down", VK_VOLUME_DOWN, 0 },
    { L"Play / Pause Media", VK_MEDIA_PLAY_PAUSE, 0 },
    { L"Lock PC (Win + L)", 'L', MOD_WIN },
    { L"Next Track", VK_MEDIA_NEXT_TRACK, 0 },
    { L"Previous Track", VK_MEDIA_PREV_TRACK, 0 }
};

bool SettingsUI::focus_existing_window() {
    HWND hWnd = FindWindowW(L"NewPilotSettingsClass", L"NewPilot Settings");
    if (hWnd) {
        if (IsIconic(hWnd)) ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
        return true;
    }
    return false;
}

static void update_control_visibility(HWND hWnd) {
    int sel = (int)SendMessageW(GetDlgItem(hWnd, IDC_COMBO_ACTION_KIND), CB_GETCURSEL, 0, 0);

    // 0: Menu Key, 1: Store/System App, 2: Custom File/EXE, 3: Hotkey Preset
    ShowWindow(GetDlgItem(hWnd, IDC_LBL_APP_PICKER), (sel == 1) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, IDC_COMBO_APP_PICKER), (sel == 1) ? SW_SHOW : SW_HIDE);

    ShowWindow(GetDlgItem(hWnd, IDC_LBL_PATH), (sel == 2) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, IDC_EDIT_PATH), (sel == 2) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, IDC_BTN_BROWSE), (sel == 2) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, IDC_LBL_ARGS), (sel == 2) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, IDC_EDIT_ARGS), (sel == 2) ? SW_SHOW : SW_HIDE);

    ShowWindow(GetDlgItem(hWnd, IDC_LBL_PRESET), (sel == 3) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, IDC_COMBO_PRESET), (sel == 3) ? SW_SHOW : SW_HIDE);
}

static void populate_dialog_data(HWND hWnd) {
    g_config = AppConfig::load();

    // Populate Action Kind Combo
    HWND hKindCombo = GetDlgItem(hWnd, IDC_COMBO_ACTION_KIND);
    SendMessageW(hKindCombo, CB_RESETCONTENT, 0, 0);
    SendMessageW(hKindCombo, CB_ADDSTRING, 0, (LPARAM)L"Context Menu Key (VK_APPS)");
    SendMessageW(hKindCombo, CB_ADDSTRING, 0, (LPARAM)L"Launch Store / System App");
    SendMessageW(hKindCombo, CB_ADDSTRING, 0, (LPARAM)L"Launch Custom File / Program / URL");
    SendMessageW(hKindCombo, CB_ADDSTRING, 0, (LPARAM)L"Quick Hotkey / System Action");

    int kind_idx = 0;
    if (g_config.tap_action.kind == ActionKind::ShellApp) kind_idx = 1;
    else if (g_config.tap_action.kind == ActionKind::File || g_config.tap_action.kind == ActionKind::Command) kind_idx = 2;
    else if (g_config.tap_action.kind == ActionKind::Hotkey) kind_idx = 3;
    SendMessageW(hKindCombo, CB_SETCURSEL, kind_idx, 0);

    // Populate Apps Combo
    HWND hAppCombo = GetDlgItem(hWnd, IDC_COMBO_APP_PICKER);
    SendMessageW(hAppCombo, CB_RESETCONTENT, 0, 0);
    g_installed_apps = AppEnumerator::get_installed_apps();
    int sel_app_idx = 0;
    for (size_t i = 0; i < g_installed_apps.size(); ++i) {
        SendMessageW(hAppCombo, CB_ADDSTRING, 0, (LPARAM)g_installed_apps[i].display_name.c_str());
        if (g_config.tap_action.kind == ActionKind::ShellApp && g_installed_apps[i].aumid == g_config.tap_action.aumid) {
            sel_app_idx = (int)i;
        }
    }
    if (!g_installed_apps.empty()) {
        SendMessageW(hAppCombo, CB_SETCURSEL, sel_app_idx, 0);
    }

    // Custom Path & Args
    SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_PATH), g_config.tap_action.path.c_str());
    SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_ARGS), g_config.tap_action.arguments.c_str());

    // Populate Hotkey Presets Combo
    HWND hPresetCombo = GetDlgItem(hWnd, IDC_COMBO_PRESET);
    SendMessageW(hPresetCombo, CB_RESETCONTENT, 0, 0);
    int sel_preset_idx = 0;
    for (size_t i = 0; i < sizeof(g_presets)/sizeof(g_presets[0]); ++i) {
        SendMessageW(hPresetCombo, CB_ADDSTRING, 0, (LPARAM)g_presets[i].name);
        if (g_config.tap_action.kind == ActionKind::Hotkey &&
            g_config.tap_action.virtual_key == g_presets[i].vk &&
            g_config.tap_action.modifiers == g_presets[i].mods) {
            sel_preset_idx = (int)i;
        }
    }
    SendMessageW(hPresetCombo, CB_SETCURSEL, sel_preset_idx, 0);

    // Tray options
    SendMessageW(GetDlgItem(hWnd, IDC_CHK_SHOW_TRAY), BM_SETCHECK, g_config.show_tray_icon ? BST_CHECKED : BST_UNCHECKED, 0);
    bool startup_active = TrayApplication::is_startup_enabled();
    SendMessageW(GetDlgItem(hWnd, IDC_CHK_AUTO_START), BM_SETCHECK, startup_active ? BST_CHECKED : BST_UNCHECKED, 0);

    update_control_visibility(hWnd);
}

static KeyAction build_action_from_ui(HWND hWnd) {
    KeyAction action;
    int kind_idx = (int)SendMessageW(GetDlgItem(hWnd, IDC_COMBO_ACTION_KIND), CB_GETCURSEL, 0, 0);

    if (kind_idx == 0) {
        action.kind = ActionKind::MenuKey;
    } else if (kind_idx == 1) {
        action.kind = ActionKind::ShellApp;
        int app_idx = (int)SendMessageW(GetDlgItem(hWnd, IDC_COMBO_APP_PICKER), CB_GETCURSEL, 0, 0);
        if (app_idx >= 0 && app_idx < (int)g_installed_apps.size()) {
            action.aumid = g_installed_apps[app_idx].aumid;
        }
    } else if (kind_idx == 2) {
        action.kind = ActionKind::File;
        wchar_t buf[MAX_PATH] = {};
        GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_PATH), buf, MAX_PATH);
        action.path = buf;
        GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_ARGS), buf, MAX_PATH);
        action.arguments = buf;
    } else if (kind_idx == 3) {
        action.kind = ActionKind::Hotkey;
        int preset_idx = (int)SendMessageW(GetDlgItem(hWnd, IDC_COMBO_PRESET), CB_GETCURSEL, 0, 0);
        if (preset_idx >= 0 && preset_idx < (int)(sizeof(g_presets)/sizeof(g_presets[0]))) {
            action.virtual_key = g_presets[preset_idx].vk;
            action.modifiers = g_presets[preset_idx].mods;
        }
    }
    return action;
}

static LRESULT CALLBACK SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_STANDARD_CLASSES | ICC_LINK_CLASS };
            InitCommonControlsEx(&icex);

            // Create clean Segoe UI font
            g_hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            // Action Kind Header
            HWND hLblKind = CreateWindowW(L"STATIC", L"When Copilot key is pressed:", WS_CHILD | WS_VISIBLE, 20, 18, 440, 18, hWnd, NULL, NULL, NULL);
            SendMessageW(hLblKind, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hKindCombo = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 20, 40, 440, 150, hWnd, (HMENU)IDC_COMBO_ACTION_KIND, NULL, NULL);
            SendMessageW(hKindCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            // App Picker Controls (Option 1)
            HWND hLblApp = CreateWindowW(L"STATIC", L"Select Application:", WS_CHILD, 20, 75, 440, 18, hWnd, (HMENU)IDC_LBL_APP_PICKER, NULL, NULL);
            SendMessageW(hLblApp, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hAppCombo = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 20, 96, 440, 250, hWnd, (HMENU)IDC_COMBO_APP_PICKER, NULL, NULL);
            SendMessageW(hAppCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            // File/Program/URL Path Controls (Option 2)
            HWND hLblPath = CreateWindowW(L"STATIC", L"Target File, Program, or URL Path:", WS_CHILD, 20, 75, 440, 18, hWnd, (HMENU)IDC_LBL_PATH, NULL, NULL);
            SendMessageW(hLblPath, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            // Edit height set to 23px for perfect vertical text centering in Windows 11
            HWND hEditPath = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 20, 96, 340, 23, hWnd, (HMENU)IDC_EDIT_PATH, NULL, NULL);
            SendMessageW(hEditPath, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hBtnBrowse = CreateWindowW(L"BUTTON", L"Browse...", WS_CHILD | BS_PUSHBUTTON, 370, 96, 90, 23, hWnd, (HMENU)IDC_BTN_BROWSE, NULL, NULL);
            SendMessageW(hBtnBrowse, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hLblArgs = CreateWindowW(L"STATIC", L"Command-Line Arguments (Optional):", WS_CHILD, 20, 127, 440, 18, hWnd, (HMENU)IDC_LBL_ARGS, NULL, NULL);
            SendMessageW(hLblArgs, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hEditArgs = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 20, 148, 440, 23, hWnd, (HMENU)IDC_EDIT_ARGS, NULL, NULL);
            SendMessageW(hEditArgs, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            // Hotkey Preset Controls (Option 3)
            HWND hLblPreset = CreateWindowW(L"STATIC", L"Select Preset Action:", WS_CHILD, 20, 75, 440, 18, hWnd, (HMENU)IDC_LBL_PRESET, NULL, NULL);
            SendMessageW(hLblPreset, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hPresetCombo = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 20, 96, 440, 200, hWnd, (HMENU)IDC_COMBO_PRESET, NULL, NULL);
            SendMessageW(hPresetCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            // Button to open Windows 11 Keyboard Settings directly
            HWND hBtnOpenWinSettings = CreateWindowW(L"BUTTON", L"Open Windows Keyboard Settings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 180, 440, 30, hWnd, (HMENU)IDC_BTN_OPEN_WIN_SETTINGS, NULL, NULL);
            SendMessageW(hBtnOpenWinSettings, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            // Group Box for System Tray & Startup Options
            HWND hGroupTray = CreateWindowW(L"BUTTON", L"Tray and Startup Options", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 220, 440, 85, hWnd, NULL, NULL, NULL);
            SendMessageW(hGroupTray, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hChkTray = CreateWindowW(L"BUTTON", L"Show notification area (tray) icon", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 35, 242, 400, 20, hWnd, (HMENU)IDC_CHK_SHOW_TRAY, NULL, NULL);
            SendMessageW(hChkTray, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hChkStart = CreateWindowW(L"BUTTON", L"Start tray icon automatically when I sign in", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 35, 268, 400, 20, hWnd, (HMENU)IDC_CHK_AUTO_START, NULL, NULL);
            SendMessageW(hChkStart, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            // Action Buttons
            HWND hBtnTest = CreateWindowW(L"BUTTON", L"Test Action", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 318, 90, 30, hWnd, (HMENU)IDC_BTN_TEST, NULL, NULL);
            SendMessageW(hBtnTest, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hBtnAbout = CreateWindowW(L"BUTTON", L"About", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 118, 318, 80, 30, hWnd, (HMENU)IDC_BTN_ABOUT, NULL, NULL);
            SendMessageW(hBtnAbout, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hBtnSave = CreateWindowW(L"BUTTON", L"Save and Close", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 230, 318, 120, 30, hWnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);
            SendMessageW(hBtnSave, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 360, 318, 100, 30, hWnd, (HMENU)IDC_BTN_CANCEL, NULL, NULL);
            SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            populate_dialog_data(hWnd);
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);

            if (id == IDC_COMBO_ACTION_KIND && code == CBN_SELCHANGE) {
                update_control_visibility(hWnd);
            } else if (id == IDC_BTN_BROWSE) {
                wchar_t file_buf[MAX_PATH] = {};
                OPENFILENAMEW ofn = { sizeof(ofn) };
                ofn.hwndOwner = hWnd;
                ofn.lpstrFile = file_buf;
                ofn.nMaxFile = MAX_PATH;
                ofn.lpstrFilter = L"Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                if (GetOpenFileNameW(&ofn)) {
                    SetWindowTextW(GetDlgItem(hWnd, IDC_EDIT_PATH), file_buf);
                }
            } else if (id == IDC_BTN_TEST) {
                KeyAction test_act = build_action_from_ui(hWnd);
                ActionRunner::run(test_act);
            } else if (id == IDC_BTN_ABOUT) {
                ShellExecuteW(NULL, L"open", L"https://github.com/GamerJagdish/newpilot", NULL, NULL, SW_SHOWNORMAL);
            } else if (id == IDC_BTN_OPEN_WIN_SETTINGS) {
                ShellExecuteW(NULL, L"open", L"ms-settings:personalization-textinput-copilot-hardwarekey", NULL, NULL, SW_SHOWNORMAL);
            } else if (id == IDC_BTN_SAVE) {
                g_config.tap_action = build_action_from_ui(hWnd);
                g_config.show_tray_icon = (SendMessageW(GetDlgItem(hWnd, IDC_CHK_SHOW_TRAY), BM_GETCHECK, 0, 0) == BST_CHECKED);
                g_config.auto_start_tray = (SendMessageW(GetDlgItem(hWnd, IDC_CHK_AUTO_START), BM_GETCHECK, 0, 0) == BST_CHECKED);
                
                g_config.save();
                TrayApplication::update_startup_shortcut(g_config.auto_start_tray);

                if (g_config.show_tray_icon) {
                    TrayApplication::ensure_tray_running();
                } else {
                    TrayApplication::stop_tray_if_running();
                }

                DestroyWindow(hWnd);
            } else if (id == IDC_BTN_CANCEL) {
                DestroyWindow(hWnd);
            }
            break;
        }

        case WM_DESTROY:
            if (g_hFont) DeleteObject(g_hFont);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void SettingsUI::show(HINSTANCE hInstance) {
    if (focus_existing_window()) return;

    WNDCLASSEXW wcex = { sizeof(wcex) };
    wcex.lpfnWndProc = SettingsWndProc;
    wcex.hInstance = hInstance;
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = L"NewPilotSettingsClass";
    wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wcex.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    RegisterClassExW(&wcex);

    int width = 495;
    int height = 400;
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int x = (screen_w - width) / 2;
    int y = (screen_h - height) / 2;

    HWND hWnd = CreateWindowExW(0, L"NewPilotSettingsClass", L"NewPilot Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, width, height, NULL, NULL, hInstance, NULL);

    if (!hWnd) return;

    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
