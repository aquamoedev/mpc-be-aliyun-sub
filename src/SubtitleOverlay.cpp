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
    if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    return wide;
}

// ============================================================
// WndProc — runs on overlay thread
// ============================================================
LRESULT CALLBACK SubtitleOverlay::WndProc(HWND hwnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam) {
    SubtitleOverlay* self = reinterpret_cast<SubtitleOverlay*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_AQUA_UPDATE_SUBTITLE: {
        // Receive text from processing thread (allocated on heap)
        std::string* pText = reinterpret_cast<std::string*>(lParam);
        if (pText && self) {
            self->m_currentText = *pText;
            self->m_isError = (pText->find("[ERR:") == 0);
            delete pText;
            self->RedrawOverlay();
        } else {
            delete pText;
        }
        return 0;
    }
    case WM_AQUA_SHUTDOWN:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================
// CreateOverlayWindow — called from overlay thread
// ============================================================
void SubtitleOverlay::CreateOverlayWindow() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = L"AquaSubOverlay";
    RegisterClassW(&wc);

    m_screenW  = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        L"AquaSubOverlay", L"Aqua Subtitles",
        WS_POPUP,
        0, screenH - m_overlayH,   // bottom of screen
        m_screenW, m_overlayH,
        nullptr, nullptr, hInst, nullptr);

    if (!m_hwnd) return;

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Create memory DC and bitmap for UpdateLayeredWindow
    HDC hdcScreen = GetDC(NULL);
    m_hdcMem = CreateCompatibleDC(hdcScreen);
    m_hbm = CreateCompatibleBitmap(hdcScreen, m_screenW, m_overlayH);
    SelectObject(m_hdcMem, m_hbm);
    ReleaseDC(NULL, hdcScreen);

    // Fill with black (will be transparent via color key)
    RECT rc = {0, 0, m_screenW, m_overlayH};
    FillRect(m_hdcMem, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    // Create fonts
    m_hFont = CreateFontW(
        42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");

    m_hFontShadow = CreateFontW(
        42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");

    // Initial UpdateLayeredWindow to make window visible (all transparent)
    RedrawOverlay();

    ShowWindow(m_hwnd, SW_SHOW);
}

// ============================================================
// RedrawOverlay — draw text on memory DC, push via UpdateLayeredWindow
// ============================================================
void SubtitleOverlay::RedrawOverlay() {
    if (!m_hdcMem || !m_hbm || !m_hwnd) return;

    // Clear bitmap with black (transparent via color key)
    RECT rc = {0, 0, m_screenW, m_overlayH};
    FillRect(m_hdcMem, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    if (!m_currentText.empty() && m_hFont) {
        std::wstring wide = Utf8ToWide(m_currentText);
        if (!wide.empty()) {
            SetBkMode(m_hdcMem, TRANSPARENT);

            // Text area with padding
            RECT textRect = {20, 20, m_screenW - 20, m_overlayH - 20};

            // Shadow
            RECT shadowRect = textRect;
            OffsetRect(&shadowRect, 2, 2);
            SelectObject(m_hdcMem, m_hFontShadow);
            SetTextColor(m_hdcMem, RGB(0, 0, 0));
            DrawTextW(m_hdcMem, wide.c_str(), -1, &shadowRect,
                      DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_WORDBREAK);

            // Main text
            COLORREF color = m_isError ? RGB(255, 80, 80) : RGB(255, 255, 0);
            SelectObject(m_hdcMem, m_hFont);
            SetTextColor(m_hdcMem, color);
            DrawTextW(m_hdcMem, wide.c_str(), -1, &textRect,
                      DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_WORDBREAK);
        }
    }

    // Push to screen via UpdateLayeredWindow
    HDC hdcScreen = GetDC(NULL);
    POINT ptDst = {0, GetSystemMetrics(SM_CYSCREEN) - m_overlayH};
    SIZE sz = {m_screenW, m_overlayH};
    POINT ptSrc = {0, 0};
    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &sz, m_hdcMem, &ptSrc,
                        RGB(0, 0, 0), NULL, ULW_COLORKEY);
    ReleaseDC(NULL, hdcScreen);
}

// ============================================================
// ThreadProc — overlay thread entry point
// ============================================================
void SubtitleOverlay::ThreadProc() {
    CreateOverlayWindow();
    if (!m_hwnd) return;

    m_ready = true;  // Signal window is ready

    // Message pump
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CleanupResources();
}

// ============================================================
// CleanupResources — called when message pump exits
// ============================================================
void SubtitleOverlay::CleanupResources() {
    if (m_hFont)       { DeleteObject(m_hFont);       m_hFont = nullptr; }
    if (m_hFontShadow) { DeleteObject(m_hFontShadow); m_hFontShadow = nullptr; }
    if (m_hbm)         { DeleteObject(m_hbm);         m_hbm = nullptr; }
    if (m_hdcMem)      { DeleteDC(m_hdcMem);          m_hdcMem = nullptr; }
    m_hwnd = nullptr;
}

// ============================================================
// Init — create overlay thread, wait for window
// ============================================================
void SubtitleOverlay::Init() {
    m_thread = std::thread(&SubtitleOverlay::ThreadProc, this);
    // Wait for window to be created (timeout: 2 seconds)
    for (int i = 0; i < 200 && !m_ready; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ============================================================
// ShowWelcome — post welcome message
// ============================================================
void SubtitleOverlay::ShowWelcome() {
    UpdateText("Aqua's Divine Subtitle Plugin is ready!");
}

// ============================================================
// UpdateText — post text to overlay thread
// ============================================================
void SubtitleOverlay::UpdateText(const std::string& utf8Text) {
    if (!m_ready || !m_hwnd) return;
    if (utf8Text.empty()) return;

    // Allocate string on heap for cross-thread transfer
    std::string* pText = new std::string(utf8Text);
    if (!PostMessage(m_hwnd, WM_AQUA_UPDATE_SUBTITLE, 0, reinterpret_cast<LPARAM>(pText))) {
        delete pText;  // PostMessage failed, clean up
    }
}

// ============================================================
// Destructor — shutdown overlay thread
// ============================================================
SubtitleOverlay::~SubtitleOverlay() {
    if (m_ready && m_hwnd) {
        PostMessage(m_hwnd, WM_AQUA_SHUTDOWN, 0, 0);
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
}
