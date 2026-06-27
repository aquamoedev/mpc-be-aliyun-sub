#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>

// Custom messages for cross-thread overlay updates
#define WM_AQUA_UPDATE_SUBTITLE  (WM_USER + 100)
#define WM_AQUA_SHUTDOWN         (WM_USER + 101)

class SubtitleOverlay {
public:
    void Init();
    void ShowWelcome();
    void UpdateText(const std::string& utf8Text);
    ~SubtitleOverlay();

private:
    void ThreadProc();
    void CreateOverlayWindow();
    void RedrawOverlay();
    void CleanupResources();
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND            m_hwnd       = nullptr;
    HFONT           m_hFont      = nullptr;
    HFONT           m_hFontShadow = nullptr;

    std::thread     m_thread;
    std::atomic<bool> m_ready{false};

    // Accessed only from overlay thread:
    std::string     m_currentText;
    bool            m_isError    = false;

    // Memory DC for UpdateLayeredWindow double-buffering
    HDC             m_hdcMem     = nullptr;
    HBITMAP         m_hbm        = nullptr;
    int             m_screenW    = 0;
    int             m_overlayH   = 200;   // bottom 200px strip
};
