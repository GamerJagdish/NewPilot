#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct AppInfo {
    std::wstring display_name;
    std::wstring aumid;
};

class AppEnumerator {
public:
    static std::vector<AppInfo> get_installed_apps();
};
