#include "SubtitleOverlay.h"
#include <string>

HINSTANCE SubtitleOverlay::s_hInst = nullptr;

// ============================================================
// UTF-8 → UTF-16 helper
// ============================================================
static std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &w[0], len);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

// ============================================================
// CreateOverlayWindow  —  called from overlay thread
// ============================================================
void SubtitleOverlay::CreateOverlayWindow() {
    HINSTANCE hInst = s_hInst ? s_hInst : GetModuleHandleW(nullptr);

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = L"AquaSubOverlayWnd";
    RegisterClassW(&wc);

    m_screenW   = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        L"AquaSubOverlayWnd", L"Aqua Sub",
        WS_POPUP | WS_VISIBLE,
        0, screenH - m_overlayH,
        m_screenW, m_overlayH,
        nullptr, nullptr, hInst, nullptr);

    if (!m_hwnd) return;

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Color key: black pixels → fully transparent
    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // Fonts
    m_hFont = CreateFontW(
        42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
}

// ============================================================
// OnPaint  —  standard WM_PAINT handler
// ============================================================
void SubtitleOverlay::OnPaint(HDC hdc) {
    RECT rc = {0, 0, m_screenW, m_overlayH};

    // Fill with black → transparent via color key
    FillRect(hdc, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    if (m_currentText.empty() || !m_hFont) return;

    std::wstring text = Utf8ToWide(m_currentText);
    if (text.empty()) return;

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, m_hFont);

    RECT textRect = {20, 20, m_screenW - 20, m_overlayH - 20};

    // Shadow
    RECT shadow = textRect;
    OffsetRect(&shadow, 2, 2);
    SetTextColor(hdc, RGB(0, 0, 0));
    DrawTextW(hdc, text.c_str(), -1, &shadow,
              DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOCLIP);

    // Main text
    SetTextColor(hdc, m_isError ? RGB(255, 80, 80) : RGB(255, 255, 0));
    DrawTextW(hdc, text.c_str(), -1, &textRect,
              DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOCLIP);
}

// ============================================================
// WndProc
// ============================================================
LRESULT CALLBACK SubtitleOverlay::WndProc(HWND hwnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<SubtitleOverlay*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (self) self->OnPaint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_AQUA_UPDATE_SUBTITLE: {
        auto* pText = reinterpret_cast<std::string*>(lParam);
        if (pText && self) {
            self->m_currentText = std::move(*pText);
            self->m_isError = (self->m_currentText.find("[ERR:") == 0);
            delete pText;
            RECT rc = {0, 0, self->m_screenW, self->m_overlayH};
            InvalidateRect(hwnd, &rc, FALSE);
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
// ThreadProc  —  overlay thread entry
// ============================================================
void SubtitleOverlay::ThreadProc() {
    CreateOverlayWindow();
    if (!m_hwnd) return;

    m_ready = true;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CleanupResources();
}

void SubtitleOverlay::CleanupResources() {
    if (m_hFont) { DeleteObject(m_hFont); m_hFont = nullptr; }
    m_hwnd = nullptr;
}

// ============================================================
// Public API
// ============================================================
void SubtitleOverlay::Init() {
    m_thread = std::thread(&SubtitleOverlay::ThreadProc, this);
    for (int i = 0; i < 200 && !m_ready; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void SubtitleOverlay::ShowWelcome() {
    UpdateText("Aqua's Divine Subtitle Plugin is ready!");
}

void SubtitleOverlay::UpdateText(const std::string& utf8Text) {
    if (!m_ready || !m_hwnd || utf8Text.empty()) return;
    auto* p = new std::string(utf8Text);
    if (!PostMessage(m_hwnd, WM_AQUA_UPDATE_SUBTITLE, 0, reinterpret_cast<LPARAM>(p)))
        delete p;
}

SubtitleOverlay::~SubtitleOverlay() {
    if (m_ready && m_hwnd)
        PostMessage(m_hwnd, WM_AQUA_SHUTDOWN, 0, 0);
    if (m_thread.joinable())
        m_thread.join();
}
