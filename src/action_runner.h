#pragma once
#include "config.h"

class ActionRunner {
public:
    static bool run(const KeyAction& action);
    static void release_stuck_modifiers();
    static bool launch_shell_app(const std::wstring& aumid);
    static bool launch_file(const KeyAction& action);
    static bool send_menu_key();
    static bool send_hotkey(WORD virtual_key, DWORD modifiers);
};
