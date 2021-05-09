/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main program
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev            (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas  (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov      (sanchaez@reactos.org)
 */
#include "rapps.h"
#include "unattended.h"
#include "winmain.h"
#include <atlcom.h>
#include <gdiplus.h>
#include <conutils.h>

LPCWSTR szWindowClass = L"ROSAPPMGR";

HWND hMainWnd;
HINSTANCE hInst;
SETTINGS_INFO SettingsInfo;

class CRAppsModule : public CComModule
{
public:
};

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

CRAppsModule gModule;
CAtlWinModule gWinModule;

Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR           gdiplusToken;


static VOID InitializeAtlModule(HINSTANCE hInstance, BOOL bInitialize)
{
    if (bInitialize)
    {
        gModule.Init(ObjectMap, hInstance, NULL);
    }
    else
    {
        gModule.Term();
    }
}

VOID InitializeGDIPlus(BOOL bInitialize)
{
    if (bInitialize)
    {
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    else
    {
        Gdiplus::GdiplusShutdown(gdiplusToken);
    }
}

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
{
    BOOL bIsFirstLaunch;
    
    InitializeAtlModule(hInstance, TRUE);
    InitializeGDIPlus(TRUE);

    if (GetUserDefaultUILanguage() == MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT))
    {
        SetProcessDefaultLayout(LAYOUT_RTL);
    }

    hInst = hInstance;

    bIsFirstLaunch = !LoadSettings(&SettingsInfo);
    if (bIsFirstLaunch)
    {
        FillDefaultSettings(&SettingsInfo);
    }

    InitLogs();
    InitCommonControls();

    // parse cmd-line and perform the corresponding operation
    BOOL bSuccess = ParseCmdAndExecute(GetCommandLineW(), bIsFirstLaunch, SW_SHOWNORMAL);
    
    InitializeGDIPlus(FALSE);
    InitializeAtlModule(GetModuleHandle(NULL), FALSE);

    return bSuccess ? 0 : 1;
}
