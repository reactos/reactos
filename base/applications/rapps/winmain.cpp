/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/rapps/winmain.cpp
 * PURPOSE:     Main program
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev            (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas  (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov      (sanchaez@reactos.org)
 */
#include "rapps.h"

#include "unattended.h"

#include <atlcom.h>

HWND hMainWnd;
HINSTANCE hInst;
INT SelectedEnumType = ENUM_ALL_INSTALLED;
SETTINGS_INFO SettingsInfo;

class CRAppsModule : public CComModule
{
public:
};

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

CRAppsModule gModule;
CAtlWinModule gWinModule;

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

VOID FillDefaultSettings(PSETTINGS_INFO pSettingsInfo)
{
    ATL::CStringW szDownloadDir;
    ZeroMemory(pSettingsInfo, sizeof(SETTINGS_INFO));

    pSettingsInfo->bSaveWndPos = TRUE;
    pSettingsInfo->bUpdateAtStart = FALSE;
    pSettingsInfo->bLogEnabled = TRUE;

    if (FAILED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szDownloadDir.GetBuffer(MAX_PATH))))
    {
        szDownloadDir.ReleaseBuffer();
        if (!szDownloadDir.GetEnvironmentVariableW(L"SystemDrive"))
        {
            szDownloadDir = L"C:";
        }
    }
    else
    {
        szDownloadDir.ReleaseBuffer();
    }

    szDownloadDir += L"\\RAPPS Downloads";
    ATL::CStringW::CopyChars(pSettingsInfo->szDownloadDir,
                             _countof(pSettingsInfo->szDownloadDir),
                             szDownloadDir.GetString(),
                             szDownloadDir.GetLength() + 1);

    pSettingsInfo->bDelInstaller = FALSE;
    pSettingsInfo->Maximized = FALSE;
    pSettingsInfo->Left = CW_USEDEFAULT;
    pSettingsInfo->Top = CW_USEDEFAULT;
    pSettingsInfo->Width = 680;
    pSettingsInfo->Height = 450;
}

static BOOL LoadSettings()
{
    ATL::CRegKey RegKey;
    DWORD dwSize;
    BOOL bResult = FALSE;
    if (RegKey.Open(HKEY_CURRENT_USER, L"Software\\ReactOS\\rapps", KEY_READ) == ERROR_SUCCESS)
    {
        dwSize = sizeof(SettingsInfo);
        bResult = (RegKey.QueryBinaryValue(L"Settings", (PVOID) &SettingsInfo, &dwSize) == ERROR_SUCCESS);

        RegKey.Close();
    }

    return bResult;
}

VOID SaveSettings(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    ATL::CRegKey RegKey;

    if (SettingsInfo.bSaveWndPos)
    {
        wp.length = sizeof(wp);
        GetWindowPlacement(hwnd, &wp);

        SettingsInfo.Left = wp.rcNormalPosition.left;
        SettingsInfo.Top = wp.rcNormalPosition.top;
        SettingsInfo.Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
        SettingsInfo.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        SettingsInfo.Maximized = (wp.showCmd == SW_MAXIMIZE
                                  || (wp.showCmd == SW_SHOWMINIMIZED
                                      && (wp.flags & WPF_RESTORETOMAXIMIZED)));
    }

    if (RegKey.Create(HKEY_CURRENT_USER, L"Software\\ReactOS\\rapps", NULL,
                      REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, NULL) == ERROR_SUCCESS)
    {
        RegKey.SetBinaryValue(L"Settings", (const PVOID) &SettingsInfo, sizeof(SettingsInfo));
        RegKey.Close();
    }
}

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
{
    LPCWSTR szWindowClass = L"ROSAPPMGR";
    HANDLE hMutex;
    BOOL bIsFirstLaunch;

    InitializeAtlModule(hInstance, TRUE);

    if (GetUserDefaultUILanguage() == MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT))
    {
        SetProcessDefaultLayout(LAYOUT_RTL);
    }

    hInst = hInstance;

    hMutex = CreateMutexW(NULL, FALSE, szWindowClass);
    if ((!hMutex) || (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        /* If already started, it is found its window */
        HWND hWindow = FindWindowW(szWindowClass, NULL);

        /* Activate window */
        ShowWindow(hWindow, SW_SHOWNORMAL);
        SetForegroundWindow(hWindow);
        return 1;
    }
    bIsFirstLaunch = !LoadSettings();
    if (bIsFirstLaunch)
    {
        FillDefaultSettings(&SettingsInfo);
    }

    InitLogs();
    InitCommonControls();

    // skip window creation if there were some keys
    if (!UseCmdParameters(GetCommandLineW()))
    {
        if (SettingsInfo.bUpdateAtStart || bIsFirstLaunch)
            CAvailableApps::ForceUpdateAppsDB();

        ShowMainWindow(nShowCmd);
    }

    if (hMutex)
        CloseHandle(hMutex);

    InitializeAtlModule(hInstance, FALSE);

    return 0;
}
