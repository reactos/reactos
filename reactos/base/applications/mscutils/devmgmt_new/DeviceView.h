#pragma once
#include "Devices.h"

class CDeviceView : public CDevices
{
    HWND m_hPropertyDialog;
    HWND m_hShortcutMenu;

public:
    CDeviceView(void);
    ~CDeviceView(void);

    BOOL Initialize();
    BOOL Uninitialize();

    VOID Refresh();
    VOID DisplayPropertySheet();
    VOID SetFocus();
};

