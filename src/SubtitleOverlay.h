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
    HWND  m_hwnd  = nullptr;
    HFONT m_hFont = nullptr;
    HFONT m_hFontShadow = nullptr;
};