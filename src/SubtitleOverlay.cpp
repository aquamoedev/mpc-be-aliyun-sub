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
    wide.resize(len - 1); // drop null terminator
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

    // Black → transparent; only non-black pixels (our text) will be visible
    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(m_hwnd, SW_SHOW);

    // Main font: yellow, bold, 42pt
    m_hFont = CreateFontW(
        42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");

    // Shadow font: dark outline, slightly offset, same size
    m_hFontShadow = CreateFontW(
        42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
}

// ============================================================
// UpdateText  –  draw translated subtitle on the overlay
// ============================================================
void SubtitleOverlay::UpdateText(const std::string& utf8Text) {
    if (!m_hwnd || !m_hFont) return;
    if (utf8Text.empty()) return;

    std::wstring wide = Utf8ToWide(utf8Text);
    if (wide.empty()) return;

    HDC hdc = GetDC(m_hwnd);
    RECT rect;
    GetClientRect(m_hwnd, &rect);

    // Fill entire window with black (transparent via color key)
    HBRUSH hBlack = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    FillRect(hdc, &rect, hBlack);

    // Subtitle area: near the bottom of the screen, centered
    RECT textRect = rect;
    textRect.top    = rect.bottom - 200;
    textRect.bottom = rect.bottom - 40;

    SetBkMode(hdc, TRANSPARENT);

    // Draw shadow (dark outline) slightly offset
    RECT shadowRect = textRect;
    OffsetRect(&shadowRect, 2, 2);
    SelectObject(hdc, m_hFontShadow);
    SetTextColor(hdc, RGB(0, 0, 0));
    DrawTextW(hdc, wide.c_str(), -1, &shadowRect,
              DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE);

    // Draw main text (yellow)
    SelectObject(hdc, m_hFont);
    SetTextColor(hdc, RGB(255, 255, 0));
    DrawTextW(hdc, wide.c_str(), -1, &textRect,
              DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE);

    ReleaseDC(m_hwnd, hdc);
}

// ============================================================
// Destructor
// ============================================================
SubtitleOverlay::~SubtitleOverlay() {
    if (m_hFont)        DeleteObject(m_hFont);
    if (m_hFontShadow)  DeleteObject(m_hFontShadow);
    if (m_hwnd)         DestroyWindow(m_hwnd);
}