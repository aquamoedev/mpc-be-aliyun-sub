#include "PluginConfig.h"
#include <windows.h>
#include <string>

// ============================================================
// Registry key: HKCU\Software\AquaSubFilter
// ============================================================

static const wchar_t* REG_PATH = L"Software\\AquaSubFilter";

static std::string ReadRegStr(HKEY hKey, const wchar_t* name, const std::string& def) {
    wchar_t buf[2048] = {};
    DWORD size = sizeof(buf);
    DWORD type = REG_SZ;
    if (RegQueryValueExW(hKey, name, nullptr, &type,
                         reinterpret_cast<BYTE*>(buf), &size) == ERROR_SUCCESS &&
        type == REG_SZ) {
        int len = WideCharToMultiByte(CP_UTF8, 0, buf, -1, nullptr, 0, nullptr, nullptr);
        if (len > 0) {
            std::string s(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, buf, -1, &s[0], len, nullptr, nullptr);
            return s;
        }
    }
    return def;
}

static void WriteRegStr(HKEY hKey, const wchar_t* name, const std::string& val) {
    int len = MultiByteToWideChar(CP_UTF8, 0, val.c_str(), -1, nullptr, 0);
    if (len <= 0) return;
    std::wstring wide(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, val.c_str(), -1, &wide[0], len);
    RegSetValueExW(hKey, name, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(wide.c_str()),
                   static_cast<DWORD>(wide.size() * sizeof(wchar_t)));
}

static int ReadRegInt(HKEY hKey, const wchar_t* name, int def) {
    DWORD val = 0, size = sizeof(val), type = REG_DWORD;
    if (RegQueryValueExW(hKey, name, nullptr, &type,
                         reinterpret_cast<BYTE*>(&val), &size) == ERROR_SUCCESS &&
        type == REG_DWORD) {
        return static_cast<int>(val);
    }
    return def;
}

static void WriteRegInt(HKEY hKey, const wchar_t* name, int val) {
    DWORD dw = static_cast<DWORD>(val);
    RegSetValueExW(hKey, name, 0, REG_DWORD,
                   reinterpret_cast<const BYTE*>(&dw), sizeof(dw));
}

// ============================================================
// Constructor — auto-load from registry
// ============================================================

PluginConfig::PluginConfig() {
    LoadFromRegistry();
}

// ============================================================
// LoadFromRegistry
// ============================================================

void PluginConfig::LoadFromRegistry() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return; // no saved config yet — use defaults
    }

    sttApiBase   = ReadRegStr(hKey, L"SttApiBase", sttApiBase);
    sttApiKey    = ReadRegStr(hKey, L"SttApiKey", sttApiKey);
    sttModel     = ReadRegStr(hKey, L"SttModel", sttModel);

    translateApiBase = ReadRegStr(hKey, L"TranslateApiBase", translateApiBase);
    translateApiKey  = ReadRegStr(hKey, L"TranslateApiKey", translateApiKey);
    translateModel   = ReadRegStr(hKey, L"TranslateModel", translateModel);
    translateSystemPrompt = ReadRegStr(hKey, L"TranslateSystemPrompt", translateSystemPrompt);

    targetLang = ReadRegStr(hKey, L"TargetLang", targetLang);
    offsetMs   = ReadRegInt(hKey, L"OffsetMs", offsetMs);
    isEnabled  = ReadRegInt(hKey, L"IsEnabled", isEnabled ? 1 : 0) != 0;

    RegCloseKey(hKey);
}

// ============================================================
// SaveToRegistry
// ============================================================

void PluginConfig::SaveToRegistry() {
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                        &hKey, nullptr) != ERROR_SUCCESS) {
        return;
    }

    WriteRegStr(hKey, L"SttApiBase", sttApiBase);
    WriteRegStr(hKey, L"SttApiKey", sttApiKey);
    WriteRegStr(hKey, L"SttModel", sttModel);

    WriteRegStr(hKey, L"TranslateApiBase", translateApiBase);
    WriteRegStr(hKey, L"TranslateApiKey", translateApiKey);
    WriteRegStr(hKey, L"TranslateModel", translateModel);
    WriteRegStr(hKey, L"TranslateSystemPrompt", translateSystemPrompt);

    WriteRegStr(hKey, L"TargetLang", targetLang);
    WriteRegInt(hKey, L"OffsetMs", offsetMs);
    WriteRegInt(hKey, L"IsEnabled", isEnabled ? 1 : 0);

    RegCloseKey(hKey);
}