#include "SubtitleOverlay.h"
#include <string>
#include <cstdarg>
#include <cstdio>
#include <ctime>

HINSTANCE SubtitleOverlay::s_hInst = nullptr;

// ============================================================
// Debug logging to %TEMP%\AquaSub.log
// ============================================================
void SubtitleOverlay::Log(const char* fmt, ...) {
    wchar_t tmpPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpPath);
    std::wstring path = std::wstring(tmpPath) + L"AquaSub.log";

    FILE* f = _wfopen(path.c_str(), L"a");
    if (!f) return;

    time_t now = time(nullptr);
    struct tm t;
    localtime_s(&t, &now);
    fprintf(f, "[%02d:%02d:%02d] ", t.tm_hour, t.tm_min, t.tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);

    fprintf(f, "\n");
    fflush(f);
    fclose(f);
}

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
    Log("CreateOverlayWindow: hInst=%p", (void*)hInst);

    // Register window class (ignore error if already registered)
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = L"AquaSubOverlayWnd";
    ATOM atom = RegisterClassW(&wc);
    if (!atom) {
        DWORD err = GetLastError();
        Log("RegisterClassW failed: err=%lu (1410=already exists is OK)", err);
    }

    m_screenW   = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    Log("Screen: %dx%d, overlay: %dx%d at y=%d",
        m_screenW, screenH, m_screenW, m_overlayH, screenH - m_overlayH);

    // BUG FIX: Removed WS_EX_TRANSPARENT — it interferes with
    // LWA_COLORKEY layered window rendering.
    // LWA_COLORKEY already makes transparent pixels click-through.
    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,  // no WS_EX_TRANSPARENT!
        L"AquaSubOverlayWnd", L"Aqua Sub",
        WS_POPUP | WS_VISIBLE,
        0, screenH - m_overlayH,
        m_screenW, m_overlayH,
        nullptr, nullptr, hInst, nullptr);

    if (!m_hwnd) {
        DWORD err = GetLastError();
        Log("CreateWindowExW FAILED: err=%lu", err);
        return;
    }
    Log("CreateWindowExW OK: hwnd=%p", (void*)m_hwnd);

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Color key: black pixels → fully transparent
    BOOL layered = SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    Log("SetLayeredWindowAttributes: ret=%d, err=%lu", layered, GetLastError());

    // Fonts
    m_hFont = CreateFontW(
        42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
    Log("CreateFontW: hFont=%p", (void*)m_hFont);

    // Force initial paint so the window is composited
    UpdateWindow(m_hwnd);
    Log("Initial UpdateWindow done");
}

// ============================================================
// OnPaint  —  standard WM_PAINT handler
// ============================================================
void SubtitleOverlay::OnPaint(HDC hdc) {
    RECT rc = {0, 0, m_screenW, m_overlayH};

    // Fill with black → transparent via color key
    FillRect(hdc, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    if (m_currentText.empty() || !m_hFont) {
        Log("OnPaint: no text (empty=%d, font=%p)", m_currentText.empty(), (void*)m_hFont);
        return;
    }

    std::wstring text = Utf8ToWide(m_currentText);
    if (text.empty()) return;

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, m_hFont);

    RECT textRect = {20, 20, m_screenW - 20, m_overlayH - 20};

    // Shadow (also black, will be transparent via color key — that's fine,
    // it creates a subtle depth effect against the video behind)
    RECT shadow = textRect;
    OffsetRect(&shadow, 2, 2);
    SetTextColor(hdc, RGB(0, 0, 0));
    DrawTextW(hdc, text.c_str(), -1, &shadow,
              DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOCLIP);

    // Main text — bright yellow (or red for errors)
    SetTextColor(hdc, m_isError ? RGB(255, 80, 80) : RGB(255, 255, 0));
    int drawn = DrawTextW(hdc, text.c_str(), -1, &textRect,
              DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOCLIP);
    Log("OnPaint: drew %d chars, text=%.60s", drawn, m_currentText.c_str());
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
            // BUG FIX: Force immediate WM_PAINT after InvalidateRect
            RECT rc = {0, 0, self->m_screenW, self->m_overlayH};
            InvalidateRect(hwnd, &rc, FALSE);
            UpdateWindow(hwnd);  // ← force paint NOW
            Log("UpdateSubtitle: text set, UpdateWindow called");
        } else {
            delete pText;
        }
        return 0;
    }
    case WM_AQUA_SHUTDOWN:
        Log("WM_AQUA_SHUTDOWN received");
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        Log("WM_DESTROY");
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================
// ThreadProc  —  overlay thread entry
// ============================================================
void SubtitleOverlay::ThreadProc() {
    Log("Overlay thread started (tid=%lu)", (unsigned long)GetCurrentThreadId());
    CreateOverlayWindow();
    if (!m_hwnd) {
        Log("Overlay thread exiting: window creation failed");
        return;
    }

    m_ready = true;
    Log("Overlay ready, entering message loop");

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CleanupResources();
    Log("Overlay thread exiting cleanly");
}

void SubtitleOverlay::CleanupResources() {
    if (m_hFont) { DeleteObject(m_hFont); m_hFont = nullptr; }
    m_hwnd = nullptr;
}

// ============================================================
// Public API
// ============================================================
void SubtitleOverlay::Init() {
    Log("SubtitleOverlay::Init() called");
    m_thread = std::thread(&SubtitleOverlay::ThreadProc, this);
    for (int i = 0; i < 200 && !m_ready; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    Log("Init done: ready=%d, hwnd=%p", m_ready.load(), (void*)m_hwnd);
}

void SubtitleOverlay::ShowWelcome() {
    Log("ShowWelcome called: ready=%d, hwnd=%p", m_ready.load(), (void*)m_hwnd);
    UpdateText("Aqua's Divine Subtitle Plugin is ready!");
}

void SubtitleOverlay::UpdateText(const std::string& utf8Text) {
    if (!m_ready || !m_hwnd || utf8Text.empty()) {
        Log("UpdateText SKIPPED: ready=%d, hwnd=%p, empty=%d",
            m_ready.load(), (void*)m_hwnd, utf8Text.empty());
        return;
    }
    auto* p = new std::string(utf8Text);
    BOOL posted = PostMessage(m_hwnd, WM_AQUA_UPDATE_SUBTITLE, 0, reinterpret_cast<LPARAM>(p));
    Log("UpdateText: PostMessage ret=%d, err=%lu, text=%.60s",
        posted, posted ? 0 : GetLastError(), utf8Text.c_str());
    if (!posted)
        delete p;
}

SubtitleOverlay::~SubtitleOverlay() {
    Log("~SubtitleOverlay: ready=%d, hwnd=%p", m_ready.load(), (void*)m_hwnd);
    if (m_ready && m_hwnd)
        PostMessage(m_hwnd, WM_AQUA_SHUTDOWN, 0, 0);
    if (m_thread.joinable())
        m_thread.join();
    Log("~SubtitleOverlay: done");
}
