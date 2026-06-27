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
// DrawText helper — shadow + colored text, bottom-center
// ============================================================
static void DrawSubtitle(HDC hdc, RECT& rect, const std::wstring& text,
                         HFONT hFont, HFONT hFontShadow, COLORREF color) {
    RECT textRect = rect;
    textRect.top    = rect.bottom - 160;
    textRect.bottom = rect.bottom - 30;

    // Shadow
    RECT shadowRect = textRect;
    OffsetRect(&shadowRect, 2, 2);
    SelectObject(hdc, hFontShadow);
    SetTextColor(hdc, RGB(0, 0, 0));
    DrawTextW(hdc, text.c_str(), -1, &shadowRect,
              DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_WORDBREAK);

    // Main text
    SelectObject(hdc, hFont);
    SetTextColor(hdc, color);
    DrawTextW(hdc, text.c_str(), -1, &textRect,
              DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_WORDBREAK);
}

// ============================================================
// WndProc — handles WM_PAINT to persist subtitle text
// ============================================================
LRESULT CALLBACK SubtitleOverlay::WndProc(HWND hwnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam) {
    // Retrieve 'this' from GWLP_USERDATA
    SubtitleOverlay* self = reinterpret_cast<SubtitleOverlay*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_PAINT: {
        if (self) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            self->Paint(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        break;
    }
    case WM_ERASEBKGND:
        // Prevent flicker — we fill in WM_PAINT
        return 1;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================
// Paint — called from WM_PAINT handler
// ============================================================
void SubtitleOverlay::Paint(HDC hdc) {
    RECT rect;
    GetClientRect(m_hwnd, &rect);

    // Fill with black (transparent via color key)
    HBRUSH hBlack = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    FillRect(hdc, &rect, hBlack);

    if (m_currentText.empty() || !m_hFont) return;

    std::wstring wide = Utf8ToWide(m_currentText);
    if (wide.empty()) return;

    SetBkMode(hdc, TRANSPARENT);
    COLORREF color = m_isError ? RGB(255, 80, 80) : RGB(255, 255, 0);
    DrawSubtitle(hdc, rect, wide, m_hFont, m_hFontShadow, color);
}

// ============================================================
// Init — create a fullscreen, topmost, transparent overlay
// ============================================================
void SubtitleOverlay::Init() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
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

    // Store 'this' for WndProc access
    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

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
}

// ============================================================
// ShowWelcome — display welcome message on startup
// ============================================================
void SubtitleOverlay::ShowWelcome() {
    if (!m_hwnd || !m_hFont) return;
    UpdateText("Aqua's Divine Subtitle Plugin is ready!");
}

// ============================================================
// UpdateText — store subtitle and trigger repaint
// ============================================================
void SubtitleOverlay::UpdateText(const std::string& utf8Text) {
    if (!m_hwnd || !m_hFont) return;
    if (utf8Text.empty()) return;

    m_currentText = utf8Text;
    m_isError = (utf8Text.find("[ERR:") == 0);

    // Trigger WM_PAINT
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

// ============================================================
// Destructor
// ============================================================
SubtitleOverlay::~SubtitleOverlay() {
    if (m_hFont)        DeleteObject(m_hFont);
    if (m_hFontShadow)  DeleteObject(m_hFontShadow);
    if (m_hwnd)         DestroyWindow(m_hwnd);
}
