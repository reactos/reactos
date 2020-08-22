/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions to parse command-line flags and process them
 * COPYRIGHT:   Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 */
#include "rapps.h"

#include "unattended.h"

#include "winmain.h"

#include <setupapi.h>

#include <conutils.h>

BOOL MatchCmdOption(LPWSTR argvOption, LPCWSTR szOptToMacth)
{
    WCHAR FirstCharList[] = { L'-', L'/' };

    for (UINT i = 0; i < _countof(FirstCharList); i++)
    {
        if (argvOption[0] == FirstCharList[i])
        {
            if (StrCmpIW(argvOption + 1, szOptToMacth) == 0)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }
    return FALSE;
}

BOOL HandleInstallCommand(LPWSTR szCommand, int argcLeft, LPWSTR * argvLeft)
{
    if (argcLeft == 0)
    {
        ConResMsgPrintf(StdOut, NULL, IDS_CMD_NEED_PACKAGE_NAME, szCommand);
        ConPrintf(StdOut, (LPWSTR)L"\n");
        return FALSE;
    }
    FreeConsole();

    ATL::CSimpleArray<ATL::CStringW> PkgNameList;

    for (int i = 0; i < argcLeft; i++)
    {
        PkgNameList.Add(argvLeft[i]);
    }

    CAvailableApps apps;
    apps.UpdateAppsDB();
    apps.Enum(ENUM_ALL_AVAILABLE, NULL, NULL);

    ATL::CSimpleArray<CAvailableApplicationInfo> arrAppInfo = apps.FindAppsByPkgNameList(PkgNameList);
    if (arrAppInfo.GetSize() > 0)
    {
        DownloadListOfApplications(arrAppInfo, TRUE);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL HandleSetupCommand(LPWSTR szCommand, int argcLeft, LPWSTR * argvLeft)
{
    if (argcLeft != 1)
    {
        ConResMsgPrintf(StdOut, NULL, IDS_CMD_NEED_FILE_NAME, szCommand);
        ConPrintf(StdOut, (LPWSTR)L"\n");
        return FALSE;
    }

    ATL::CSimpleArray<ATL::CStringW> PkgNameList;
    HINF InfHandle = SetupOpenInfFileW(argvLeft[0], NULL, INF_STYLE_WIN4, NULL);
    if (InfHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    INFCONTEXT Context;
    if (SetupFindFirstLineW(InfHandle, L"RAPPS", L"Install", &Context))
    {
        WCHAR szPkgName[MAX_PATH];
        do
        {
            if (SetupGetStringFieldW(&Context, 1, szPkgName, _countof(szPkgName), NULL))
            {
                PkgNameList.Add(szPkgName);
            }
        } while (SetupFindNextLine(&Context, &Context));
    }
    SetupCloseInfFile(InfHandle);

    CAvailableApps apps;
    apps.UpdateAppsDB();
    apps.Enum(ENUM_ALL_AVAILABLE, NULL, NULL);

    ATL::CSimpleArray<CAvailableApplicationInfo> arrAppInfo = apps.FindAppsByPkgNameList(PkgNameList);
    if (arrAppInfo.GetSize() > 0)
    {
        DownloadListOfApplications(arrAppInfo, TRUE);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL HandleHelpCommand(LPWSTR szCommand, int argcLeft, LPWSTR * argvLeft)
{
    if (argcLeft != 0)
    {
        return FALSE;
    }

    ConPrintf(StdOut, (LPWSTR)L"\n");
    ConResPuts(StdOut, IDS_APPTITLE);
    ConPrintf(StdOut, (LPWSTR)L"\n\n");

    ConResPuts(StdOut, IDS_CMD_USAGE);
    ConPrintf(StdOut, (LPWSTR)L"%ls\n", UsageString);
    return TRUE;
}

BOOL ParseCmdAndExecute(LPWSTR lpCmdLine, BOOL bIsFirstLaunch, int nCmdShow)
{
    INT argc;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);

    if (!argv)
    {
        return FALSE;
    }

    if (argc == 1) // RAPPS is launched without options
    {
        // Close the console, and open MainWindow
        FreeConsole();


        // Check for if rapps MainWindow is already launched in another process
        HANDLE hMutex;

        hMutex = CreateMutexW(NULL, FALSE, szWindowClass);
        if ((!hMutex) || (GetLastError() == ERROR_ALREADY_EXISTS))
        {
            /* If already started, it is found its window */
            HWND hWindow = FindWindowW(szWindowClass, NULL);

            /* Activate window */
            ShowWindow(hWindow, SW_SHOWNORMAL);
            SetForegroundWindow(hWindow);
            return FALSE;
        }

        if (SettingsInfo.bUpdateAtStart || bIsFirstLaunch)
            CAvailableApps::ForceUpdateAppsDB();

        MainWindowLoop(nCmdShow);

        if (hMutex)
            CloseHandle(hMutex);
    }
    else if (MatchCmdOption(argv[1], CMD_KEY_INSTALL))
    {
        return HandleInstallCommand(argv[1], argc - 2, argv + 2);
    }
    else if (MatchCmdOption(argv[1], CMD_KEY_SETUP))
    {
        return HandleSetupCommand(argv[1], argc - 2, argv + 2);
    }
    else if (MatchCmdOption(argv[1], CMD_KEY_HELP))
    {
        return HandleHelpCommand(argv[1], argc - 2, argv + 2);
    }
    else
    {
        // unrecognized/invalid options
        ConResPuts(StdOut, IDS_CMD_INVALID_OPTION);
        ConPrintf(StdOut, (LPWSTR)L"\n");
        return FALSE;
    }


    return TRUE;
}
