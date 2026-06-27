#include "ConfigDialog.h"
#include "PluginConfig.h"
#include "SubtitleOverlay.h"
#include "resource.h"
#include <windows.h>
#include <string>

namespace {

static constexpr int MAX_PATH_BUF = 2048;

static void SetDlgItemText8(HWND hDlg, int id, const std::string& s) {
    SetDlgItemTextA(hDlg, id, s.c_str());
}

static std::string GetDlgItemText8(HWND hDlg, int id) {
    std::string buf(MAX_PATH_BUF, '\0');
    int len = GetDlgItemTextA(hDlg, id, &buf[0], MAX_PATH_BUF);
    if (len <= 0) return "";
    buf.resize(len);
    return buf;
}

static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    auto& cfg = PluginConfig::Instance();

    switch (msg) {
    case WM_INITDIALOG: {
        SetDlgItemText8(hDlg, IDC_STT_API_BASE, cfg.sttApiBase);
        SetDlgItemText8(hDlg, IDC_STT_API_KEY,  cfg.sttApiKey);
        SetDlgItemText8(hDlg, IDC_STT_MODEL,    cfg.sttModel);
        SetDlgItemText8(hDlg, IDC_TRANSLATE_API_BASE, cfg.translateApiBase);
        SetDlgItemText8(hDlg, IDC_TRANSLATE_API_KEY,  cfg.translateApiKey);
        SetDlgItemText8(hDlg, IDC_TRANSLATE_MODEL,    cfg.translateModel);
        SetDlgItemText8(hDlg, IDC_TRANSLATE_PROMPT,   cfg.translateSystemPrompt);
        SetDlgItemText8(hDlg, IDC_TARGET_LANG, cfg.targetLang);
        SetDlgItemInt(hDlg,  IDC_OFFSET_MS,    cfg.offsetMs, TRUE);
        CheckDlgButton(hDlg, IDC_ENABLED,      cfg.isEnabled ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    }
    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        if (id == IDOK) {
            cfg.sttApiBase = GetDlgItemText8(hDlg, IDC_STT_API_BASE);
            cfg.sttApiKey  = GetDlgItemText8(hDlg, IDC_STT_API_KEY);
            cfg.sttModel   = GetDlgItemText8(hDlg, IDC_STT_MODEL);
            cfg.translateApiBase = GetDlgItemText8(hDlg, IDC_TRANSLATE_API_BASE);
            cfg.translateApiKey  = GetDlgItemText8(hDlg, IDC_TRANSLATE_API_KEY);
            cfg.translateModel   = GetDlgItemText8(hDlg, IDC_TRANSLATE_MODEL);
            cfg.translateSystemPrompt = GetDlgItemText8(hDlg, IDC_TRANSLATE_PROMPT);
            cfg.targetLang = GetDlgItemText8(hDlg, IDC_TARGET_LANG);
            cfg.offsetMs   = GetDlgItemInt(hDlg, IDC_OFFSET_MS, nullptr, TRUE);
            cfg.isEnabled  = (IsDlgButtonChecked(hDlg, IDC_ENABLED) == BST_CHECKED);
            cfg.SaveToRegistry();
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (id == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

} // namespace

void ShowConfigDialog(HINSTANCE hInst, HWND hParent) {
    SubtitleOverlay::Log("ShowConfigDialog: hInst=%p, hParent=%p", (void*)hInst, (void*)hParent);
    INT_PTR result = DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hParent, DlgProc);
    SubtitleOverlay::Log("ShowConfigDialog: DialogBox returned %lld, err=%lu", (long long)result, GetLastError());
}
