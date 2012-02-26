#pragma once
#include "Devices.h"

class CDeviceView : public CDevices
{
    HWND m_hMainWnd;
    HWND m_hTreeView;
    HWND m_hPropertyDialog;
    HWND m_hShortcutMenu;

public:
    CDeviceView(HWND hMainWnd);
    ~CDeviceView(void);

    BOOL Initialize();
    BOOL Uninitialize();

    VOID Size(INT x, INT y, INT cx, INT cy);
    VOID Refresh();
    VOID DisplayPropertySheet();
    VOID SetFocus();
};

