#include "config.h"
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <vector>

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

std::wstring AppConfig::get_config_dir() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::wstring dir = std::wstring(path) + L"\\NewPilot";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir;
    }
    return L"";
}

std::wstring AppConfig::get_config_path() {
    std::wstring dir = get_config_dir();
    if (dir.empty()) return L"config.json";
    return dir + L"\\config.json";
}

// Simple JSON helper functions
static std::wstring escape_json(const std::wstring& s) {
    std::wstring out;
    out.reserve(s.length() + 8);
    for (wchar_t c : s) {
        if (c == L'\\') out += L"\\\\";
        else if (c == L'"') out += L"\\\"";
        else if (c == L'\n') out += L"\\n";
        else if (c == L'\r') out += L"\\r";
        else if (c == L'\t') out += L"\\t";
        else out += c;
    }
    return out;
}

static std::wstring extract_json_string(const std::wstring& json, const std::wstring& key) {
    std::wstring search = L"\"" + key + L"\"";
    size_t pos = json.find(search);
    if (pos == std::wstring::npos) return L"";

    pos = json.find(L':', pos + search.length());
    if (pos == std::wstring::npos) return L"";

    size_t start_quote = json.find(L'"', pos + 1);
    if (start_quote == std::wstring::npos) return L"";

    std::wstring val;
    bool escaped = false;
    for (size_t i = start_quote + 1; i < json.length(); ++i) {
        wchar_t c = json[i];
        if (escaped) {
            if (c == L'n') val += L'\n';
            else if (c == L'r') val += L'\r';
            else if (c == L't') val += L'\t';
            else val += c;
            escaped = false;
        } else if (c == L'\\') {
            escaped = true;
        } else if (c == L'"') {
            break;
        } else {
            val += c;
        }
    }
    return val;
}

static int extract_json_int(const std::wstring& json, const std::wstring& key, int default_val) {
    std::wstring search = L"\"" + key + L"\"";
    size_t pos = json.find(search);
    if (pos == std::wstring::npos) return default_val;

    pos = json.find(L':', pos + search.length());
    if (pos == std::wstring::npos) return default_val;

    size_t start = json.find_first_of(L"0123456789-", pos + 1);
    if (start == std::wstring::npos) return default_val;

    try {
        return std::stoi(json.substr(start));
    } catch (...) {
        return default_val;
    }
}

static bool extract_json_bool(const std::wstring& json, const std::wstring& key, bool default_val) {
    std::wstring search = L"\"" + key + L"\"";
    size_t pos = json.find(search);
    if (pos == std::wstring::npos) return default_val;

    pos = json.find(L':', pos + search.length());
    if (pos == std::wstring::npos) return default_val;

    size_t true_pos = json.find(L"true", pos + 1);
    size_t false_pos = json.find(L"false", pos + 1);
    size_t end_comma = json.find_first_of(L",}\n", pos + 1);

    if (true_pos != std::wstring::npos && (end_comma == std::wstring::npos || true_pos < end_comma)) {
        return true;
    }
    if (false_pos != std::wstring::npos && (end_comma == std::wstring::npos || false_pos < end_comma)) {
        return false;
    }
    return default_val;
}

AppConfig AppConfig::load() {
    AppConfig config;
    std::wstring path = get_config_path();

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return config;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return config;
    }

    std::vector<char> buffer(fileSize + 1, 0);
    DWORD bytesRead = 0;
    ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    // Convert UTF-8 buffer to std::wstring
    int wlen = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), bytesRead, NULL, 0);
    if (wlen <= 0) return config;

    std::wstring json(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, buffer.data(), bytesRead, &json[0], wlen);

    config.tap_action.kind = static_cast<ActionKind>(extract_json_int(json, L"kind", 0));
    config.tap_action.aumid = extract_json_string(json, L"aumid");
    config.tap_action.path = extract_json_string(json, L"path");
    config.tap_action.arguments = extract_json_string(json, L"arguments");
    config.tap_action.working_dir = extract_json_string(json, L"working_dir");
    config.tap_action.virtual_key = static_cast<WORD>(extract_json_int(json, L"virtual_key", 0));
    config.tap_action.modifiers = static_cast<DWORD>(extract_json_int(json, L"modifiers", 0));

    config.show_tray_icon = extract_json_bool(json, L"show_tray_icon", false);
    config.auto_start_tray = extract_json_bool(json, L"auto_start_tray", false);

    return config;
}

bool AppConfig::save() const {
    std::wstring path = get_config_path();

    std::wstringstream ss;
    ss << L"{\n";
    ss << L"  \"kind\": " << static_cast<int>(tap_action.kind) << L",\n";
    ss << L"  \"aumid\": \"" << escape_json(tap_action.aumid) << L"\",\n";
    ss << L"  \"path\": \"" << escape_json(tap_action.path) << L"\",\n";
    ss << L"  \"arguments\": \"" << escape_json(tap_action.arguments) << L"\",\n";
    ss << L"  \"working_dir\": \"" << escape_json(tap_action.working_dir) << L"\",\n";
    ss << L"  \"virtual_key\": " << tap_action.virtual_key << L",\n";
    ss << L"  \"modifiers\": " << tap_action.modifiers << L",\n";
    ss << L"  \"show_tray_icon\": " << (show_tray_icon ? L"true" : L"false") << L",\n";
    ss << L"  \"auto_start_tray\": " << (auto_start_tray ? L"true" : L"false") << L"\n";
    ss << L"}\n";

    std::wstring json = ss.str();

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, json.c_str(), -1, NULL, 0, NULL, NULL);
    if (utf8_len <= 0) return false;

    std::vector<char> utf8_buf(utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, json.c_str(), -1, utf8_buf.data(), utf8_len, NULL, NULL);

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD bytesWritten = 0;
    // Don't write trailing null byte
    WriteFile(hFile, utf8_buf.data(), utf8_len - 1, &bytesWritten, NULL);
    CloseHandle(hFile);

    return true;
}
