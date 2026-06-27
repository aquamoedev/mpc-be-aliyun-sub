#pragma once
#include <windows.h>

// Legacy modal entry (kept for direct filter-graph use)
void ShowConfigDialog(HINSTANCE hInst, HWND hParent);

// Modeless child-dialog API for IPropertyPage hosting
HWND CreateConfigPage(HINSTANCE hInst, HWND hParent);
void DestroyConfigPage(HWND hPage);
