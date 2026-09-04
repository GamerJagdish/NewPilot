#include <windows.h>
#include <wincrypt.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include "resource.h"

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")

static bool ExtractResource(int resId, std::vector<BYTE>& outBuffer) {
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(resId), MAKEINTRESOURCEW((ULONG_PTR)RT_RCDATA));
    if (!hRes) return false;
    HGLOBAL hGlobal = LoadResource(NULL, hRes);
    if (!hGlobal) return false;
    DWORD size = SizeofResource(NULL, hRes);
    void* ptr = LockResource(hGlobal);
    if (!ptr || size == 0) return false;
    outBuffer.resize(size);
    memcpy(outBuffer.data(), ptr, size);
    return true;
}

static bool WriteBufferToFile(const std::wstring& filePath, const std::vector<BYTE>& buffer) {
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    BOOL ok = WriteFile(hFile, buffer.data(), static_cast<DWORD>(buffer.size()), &written, NULL);
    CloseHandle(hFile);
    return ok && (written == buffer.size());
}

static void ImportCertificate(const std::vector<BYTE>& certData) {
    if (certData.empty()) return;

    auto addToStore = [&](DWORD storeLocation, const wchar_t* storeName) {
        HCERTSTORE hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            0,
            NULL,
            storeLocation,
            storeName
        );
        if (hStore) {
            CertAddEncodedCertificateToStore(
                hStore,
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                certData.data(),
                static_cast<DWORD>(certData.size()),
                CERT_STORE_ADD_REPLACE_EXISTING,
                NULL
            );
            CertCloseStore(hStore, 0);
        }
    };

    addToStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, L"TrustedPeople");
    addToStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, L"Root");
    addToStore(CERT_SYSTEM_STORE_CURRENT_USER, L"TrustedPeople");
    addToStore(CERT_SYSTEM_STORE_CURRENT_USER, L"Root");
}

static void TerminateRunningNewPilot() {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"NewPilot.exe") == 0) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    Sleep(200);
}

static std::wstring Base64EncodeUtf16(const std::wstring& text) {
    const BYTE* bytes = reinterpret_cast<const BYTE*>(text.data());
    DWORD byteCount = static_cast<DWORD>(text.size() * sizeof(wchar_t));
    DWORD charsNeeded = 0;
    if (!CryptBinaryToStringW(bytes, byteCount, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &charsNeeded) || charsNeeded == 0) {
        return L"";
    }
    std::vector<wchar_t> buf(charsNeeded + 1, L'\0');
    if (!CryptBinaryToStringW(bytes, byteCount, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buf.data(), &charsNeeded)) {
        return L"";
    }
    return std::wstring(buf.data());
}

static bool RunHiddenPowerShellScript(const std::wstring& script, DWORD timeoutMs = 90000) {
    std::wstring encoded = Base64EncodeUtf16(script);
    if (encoded.empty()) return false;

    std::wstring cmd = L"powershell.exe -NoProfile -NonInteractive -WindowStyle Hidden -ExecutionPolicy Bypass -EncodedCommand " + encoded;

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(L'\0');

    if (!CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return false;
    }

    DWORD waitRes = WaitForSingleObject(pi.hProcess, timeoutMs);
    DWORD exitCode = 1;
    if (waitRes == WAIT_OBJECT_0) {
        GetExitCodeProcess(pi.hProcess, &exitCode);
    } else {
        TerminateProcess(pi.hProcess, 1);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exitCode == 0);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    bool silent = false;
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], L"/s") == 0 ||
            _wcsicmp(argv[i], L"/silent") == 0 ||
            _wcsicmp(argv[i], L"-s") == 0 ||
            _wcsicmp(argv[i], L"--silent") == 0) {
            silent = true;
            break;
        }
    }
    if (argv) LocalFree(argv);

    // 1. Extract embedded assets
    std::vector<BYTE> certData;
    std::vector<BYTE> msixData;

    if (!ExtractResource(IDR_DEV_CERT, certData) || !ExtractResource(IDR_MSIX_PACKAGE, msixData)) {
        if (!silent) {
            MessageBoxW(NULL, L"Failed to load installation resources from installer.", L"NewPilot Setup Error", MB_ICONERROR | MB_OK);
        }
        return 1;
    }

    // 2. Trust developer certificate in system certificate stores
    ImportCertificate(certData);

    // 3. Prepare temporary extraction directory
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring stagingDir = std::wstring(tempPath) + L"NewPilotSetup_" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(stagingDir.c_str(), NULL);

    std::wstring msixFile = stagingDir + L"\\NewPilot.msix";
    if (!WriteBufferToFile(msixFile, msixData)) {
        RemoveDirectoryW(stagingDir.c_str());
        if (!silent) {
            MessageBoxW(NULL, L"Failed to extract installer files to temporary directory.", L"NewPilot Setup Error", MB_ICONERROR | MB_OK);
        }
        return 1;
    }

    // 4. Terminate any running instances
    TerminateRunningNewPilot();

    // 5. Build and execute installation script
    std::wstring installScript =
        L"$ErrorActionPreference = 'Stop'\n"
        L"Stop-Process -Name NewPilot -Force -ErrorAction SilentlyContinue\n"
        L"$existing = Get-AppxPackage -Name 'NewPilot'\n"
        L"if ($existing) {\n"
        L"    Remove-AppxPackage -Package $existing.PackageFullName -ErrorAction SilentlyContinue\n"
        L"}\n"
        L"Add-AppxPackage -Path '" + msixFile + L"' -ForceApplicationShutdown\n"
        L"$pkg = Get-AppxPackage -Name 'NewPilot'\n"
        L"if ($pkg) {\n"
        L"    $aumid = 'shell:AppsFolder\\' + $pkg.PackageFamilyName + '!Settings'\n"
        L"    Start-Process explorer.exe -ArgumentList $aumid -ErrorAction SilentlyContinue\n"
        L"}\n";

    bool installOk = RunHiddenPowerShellScript(installScript);

    // 6. Cleanup temporary files
    DeleteFileW(msixFile.c_str());
    RemoveDirectoryW(stagingDir.c_str());

    if (!installOk) {
        if (!silent) {
            MessageBoxW(
                NULL,
                L"Installation of NewPilot MSIX package failed.\n\n"
                L"Please verify that your system is running Windows 11 and that sideloading / package installation is enabled.",
                L"NewPilot Setup Error",
                MB_ICONERROR | MB_OK
            );
        }
    }

    return 0;
}
