#include "ConfigDialog.h"
#include "PluginConfig.h"
#include <windows.h>
#include <string>

static PluginConfig& cfg = PluginConfig::Instance();

static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
    case WM_INITDIALOG:
        SetDlgItemTextA(hDlg, 101, cfg.aliyunApiKey.c_str());
        SetDlgItemTextA(hDlg, 102, cfg.aliyunSecret.c_str());
        SetDlgItemInt(hDlg, 103, cfg.offsetMs, TRUE);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            char buf[256];
            GetDlgItemTextA(hDlg, 101, buf, sizeof(buf));
            cfg.aliyunApiKey = buf;
            GetDlgItemTextA(hDlg, 102, buf, sizeof(buf));
            cfg.aliyunSecret = buf;
            cfg.offsetMs = GetDlgItemInt(hDlg, 103, nullptr, TRUE);
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowConfigDialog(HINSTANCE hInst, HWND hParent) {
    DialogBox(hInst, MAKEINTRESOURCE(100), hParent, DlgProc);
}
