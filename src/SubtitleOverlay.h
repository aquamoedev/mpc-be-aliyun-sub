#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>

#define WM_AQUA_UPDATE_SUBTITLE  (WM_USER + 100)
#define WM_AQUA_SHUTDOWN         (WM_USER + 101)

class SubtitleOverlay {
public:
    static void SetDllInstance(HINSTANCE hInst) { s_hInst = hInst; }
    static void Log(const char* fmt, ...);

    void Init();
    void ShowWelcome();
    void UpdateText(const std::string& utf8Text);
    ~SubtitleOverlay();

    bool IsReady() const { return m_ready; }

private:
    void ThreadProc();
    void CreateOverlayWindow();
    void OnPaint(HDC hdc);
    void CleanupResources();

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    static HINSTANCE s_hInst;

    HWND              m_hwnd    = nullptr;
    HFONT             m_hFont   = nullptr;
    std::thread       m_thread;
    std::atomic<bool> m_ready{false};

    // Overlay thread only:
    std::string m_currentText;
    bool        m_isError  = false;
    int         m_screenW  = 0;
    int         m_overlayH = 200;
};
