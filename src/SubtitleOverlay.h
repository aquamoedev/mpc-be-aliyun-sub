#pragma once
#include <windows.h>
#include <string>

class SubtitleOverlay {
public:
    void Init();
    void UpdateText(const std::string& text);

private:
    HWND hwnd   = nullptr;
    HFONT hFont = nullptr;
};
