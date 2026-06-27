#pragma once
#include <windows.h>
#include <string>

class SubtitleOverlay {
public:
    void Init();
    void ShowWelcome();
    void UpdateText(const std::string& utf8Text);
    ~SubtitleOverlay();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Paint(HDC hdc);

    HWND  m_hwnd  = nullptr;
    HFONT m_hFont = nullptr;
    HFONT m_hFontShadow = nullptr;

    std::string m_currentText;   // UTF-8 subtitle to redraw on WM_PAINT
    bool        m_isError = false;
};
