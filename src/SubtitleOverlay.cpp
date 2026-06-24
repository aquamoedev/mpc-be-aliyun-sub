#include "SubtitleOverlay.h"

void SubtitleOverlay::Init() {
    HINSTANCE hInst = GetModuleHandle(nullptr);
    WNDCLASS wc = {};
    wc.lpfnWndProc   = DefWindowProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = "AquaSubOverlay";
    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        "AquaSubOverlay", "Subtitles", WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInst, nullptr
    );

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    hFont = CreateFont(
        36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei"
    );
}

void SubtitleOverlay::UpdateText(const std::string& text) {
    if (!hwnd || !hFont) return;
    HDC hdc = GetDC(hwnd);
    RECT rect;
    GetClientRect(hwnd, &rect);
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    SelectObject(hdc, hFont);
    SetTextColor(hdc, RGB(255, 255, 0));
    SetBkMode(hdc, TRANSPARENT);

    int y = rect.bottom - 150;
    TextOutA(hdc, 100, y, text.c_str(), static_cast<int>(text.length()));
    ReleaseDC(hwnd, hdc);
}
