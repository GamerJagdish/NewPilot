#pragma once
#include <windows.h>
#include "config.h"

class SettingsUI {
public:
    static void show(HINSTANCE hInstance);
    static bool focus_existing_window();
};
