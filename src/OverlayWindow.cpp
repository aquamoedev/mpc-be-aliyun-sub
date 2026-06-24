#include <windows.h>
#include <string>

class SubtitleOverlay {
    HWND hwnd;
    HFONT hFont;

public:
    void Init() {
        hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT, 
                               "STATIC", "Subtitles", WS_POPUP, 
                               0, 0, 1920, 1080, NULL, NULL, NULL, NULL);
        
        SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_COLORKEY);
        ShowWindow(hwnd, SW_SHOW);
        hFont = CreateFont(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                          DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");
    }

    void UpdateText(const std::string& text) {
        HDC hdc = GetDC(hwnd);
        SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(255, 255, 0)); 
        SetBkMode(hdc, TRANSPARENT);
        TextOutA(hdc, 500, 900, text.c_str(), text.length());
        ReleaseDC(hwnd, hdc);
    }
};
