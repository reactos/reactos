/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main program
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas  (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 */
#include "rapps.h"
#include "unattended.h"
#include "winmain.h"
#include <atlcom.h>
#include <gdiplus.h>
#include <conutils.h>

LPCWSTR szWindowClass = L"ROSAPPMGR2";
LONG g_Busy = 0;

HWND hMainWnd;
HINSTANCE hInst;
SETTINGS_INFO SettingsInfo;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

CComModule gModule;
CAtlWinModule gWinModule;

INT WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gModule.Init(ObjectMap, hInstance, NULL);
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    if (GetUserDefaultUILanguage() == MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT))
    {
        SetProcessDefaultLayout(LAYOUT_RTL);
    }

    hInst = hInstance;
    BOOL bIsFirstLaunch = !LoadSettings(&SettingsInfo);

    InitLogs();
    InitCommonControls();
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL); // Give UI higher priority than background threads

    // parse cmd-line and perform the corresponding operation
    BOOL bSuccess = ParseCmdAndExecute(GetCommandLineW(), bIsFirstLaunch, SW_SHOWNORMAL);

    Gdiplus::GdiplusShutdown(gdiplusToken);
    gModule.Term();

    return bSuccess ? 0 : 1;
}
