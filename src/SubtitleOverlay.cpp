#include "SubtitleOverlay.h"
#include <string>

// ============================================================
// UTF-8 → UTF-16 helper
// ============================================================
static std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring wide(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], len);
    wide.resize(len - 1);
    return wide;
}

// ============================================================
// Init  –  create a fullscreen, topmost, transparent overlay
// ============================================================
void SubtitleOverlay::Init() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = DefWindowProcW;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"AquaSubOverlay";
    RegisterClassW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        L"AquaSubOverlay", L"Aqua Subtitles",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInst, nullptr);

    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(m_hwnd, SW_SHOW);

    m_hFont = CreateFontW(
        42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");

    m_hFontShadow = CreateFontW(
        42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");

    m_hFontSmall = CreateFontW(
        28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
}

// ============================================================
// UpdateText  –  draw translated subtitle or error on the overlay
// ============================================================
void SubtitleOverlay::UpdateText(const std::string& utf8Text) {
    if (!m_hwnd || !m_hFont) return;
    if (utf8Text.empty()) return;

    std::wstring wide = Utf8ToWide(utf8Text);
    if (wide.empty()) return;

    bool isError = (utf8Text.find("[ERR:") == 0);

    HDC hdc = GetDC(m_hwnd);
    RECT rect;
    GetClientRect(m_hwnd, &rect);

    HBRUSH hBlack = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    FillRect(hdc, &rect, hBlack);

    SetBkMode(hdc, TRANSPARENT);

    if (isError) {
        // Error text: red, smaller, top-right area
        RECT errRect = rect;
        errRect.left   = rect.right  - 700;
        errRect.top    = 30;
        errRect.bottom = 200;

        SelectObject(hdc, m_hFontSmall);
        SetTextColor(hdc, RGB(255, 80, 80));
        DrawTextW(hdc, wide.c_str(), -1, &errRect,
                  DT_RIGHT | DT_TOP | DT_NOCLIP | DT_WORDBREAK);
    } else {
        // Normal subtitle: yellow, bottom-center, with shadow
        RECT textRect = rect;
        textRect.top    = rect.bottom - 200;
        textRect.bottom = rect.bottom - 40;

        // Shadow
        RECT shadowRect = textRect;
        OffsetRect(&shadowRect, 2, 2);
        SelectObject(hdc, m_hFontShadow);
        SetTextColor(hdc, RGB(0, 0, 0));
        DrawTextW(hdc, wide.c_str(), -1, &shadowRect,
                  DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE);

        // Main text
        SelectObject(hdc, m_hFont);
        SetTextColor(hdc, RGB(255, 255, 0));
        DrawTextW(hdc, wide.c_str(), -1, &textRect,
                  DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE);
    }

    ReleaseDC(m_hwnd, hdc);
}

// ============================================================
// Destructor
// ============================================================
SubtitleOverlay::~SubtitleOverlay() {
    if (m_hFont)        DeleteObject(m_hFont);
    if (m_hFontShadow)  DeleteObject(m_hFontShadow);
    if (m_hFontSmall)   DeleteObject(m_hFontSmall);
    if (m_hwnd)         DestroyWindow(m_hwnd);
}