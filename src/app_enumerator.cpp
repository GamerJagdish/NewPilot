#include "app_enumerator.h"
#include <shlobj.h>
#include <propkey.h>
#include <algorithm>

std::vector<AppInfo> AppEnumerator::get_installed_apps() {
    std::vector<AppInfo> apps;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    IShellFolder* pDesktopFolder = NULL;
    hr = SHGetDesktopFolder(&pDesktopFolder);
    if (FAILED(hr) || !pDesktopFolder) {
        CoUninitialize();
        return apps;
    }

    PIDLIST_ABSOLUTE pidlAppsFolder = NULL;
    hr = pDesktopFolder->ParseDisplayName(NULL, NULL, const_cast<LPWSTR>(L"shell:AppsFolder"), NULL, &pidlAppsFolder, NULL);
    pDesktopFolder->Release();

    if (FAILED(hr) || !pidlAppsFolder) {
        CoUninitialize();
        return apps;
    }

    IShellItem2* pAppsFolderItem = NULL;
    hr = SHCreateItemFromIDList(pidlAppsFolder, IID_PPV_ARGS(&pAppsFolderItem));
    CoTaskMemFree(pidlAppsFolder);

    if (FAILED(hr) || !pAppsFolderItem) {
        CoUninitialize();
        return apps;
    }

    IEnumShellItems* pEnum = NULL;
    hr = pAppsFolderItem->BindToHandler(NULL, BHID_EnumItems, IID_PPV_ARGS(&pEnum));
    pAppsFolderItem->Release();

    if (FAILED(hr) || !pEnum) {
        CoUninitialize();
        return apps;
    }

    IShellItem* pItem = NULL;
    ULONG fetched = 0;
    while (pEnum->Next(1, &pItem, &fetched) == S_OK && fetched == 1) {
        IShellItem2* pItem2 = NULL;
        if (SUCCEEDED(pItem->QueryInterface(IID_PPV_ARGS(&pItem2)))) {
            LPWSTR pszName = NULL;
            LPWSTR pszAppId = NULL;

            pItem2->GetString(PKEY_ItemNameDisplay, &pszName);
            pItem2->GetString(PKEY_AppUserModel_ID, &pszAppId);

            if (pszName && pszAppId && wcslen(pszName) > 0 && wcslen(pszAppId) > 0) {
                AppInfo app;
                app.display_name = pszName;
                app.aumid = pszAppId;
                apps.push_back(app);
            }

            if (pszName) CoTaskMemFree(pszName);
            if (pszAppId) CoTaskMemFree(pszAppId);
            pItem2->Release();
        }
        pItem->Release();
    }
    pEnum->Release();
    CoUninitialize();

    std::sort(apps.begin(), apps.end(), [](const AppInfo& a, const AppInfo& b) {
        return _wcsicmp(a.display_name.c_str(), b.display_name.c_str()) < 0;
    });

    return apps;
}
