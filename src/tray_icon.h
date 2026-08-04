#pragma once
#include <windows.h>
#include <string>

class TrayApplication {
public:
    static bool is_running_elsewhere();
    static int run(HINSTANCE hInstance);
    static void update_startup_shortcut(bool enable);
    static bool is_startup_enabled();
    static std::wstring get_package_family_name();
    static void ensure_tray_running();
    static void stop_tray_if_running();
};
