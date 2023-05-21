/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions to parse command-line flags and process them
 * COPYRIGHT:   Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 *              Copyright 2020 He Yang (1160386205@qq.com)
 */

#include "gui.h"
#include "unattended.h"
#include <setupapi.h>
#include <conutils.h>

static BOOL
MatchCmdOption(LPWSTR argvOption, LPCWSTR szOptToMacth)
{
    WCHAR FirstCharList[] = {L'-', L'/'};

    for (UINT i = 0; i < _countof(FirstCharList); i++)
    {
        if (argvOption[0] == FirstCharList[i])
        {
            return StrCmpIW(argvOption + 1, szOptToMacth) == 0;
        }
    }
    return FALSE;
}

static void
InitRappsConsole()
{
    // First, try to attach to our parent's console
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        // Did we already have a console?
        if (GetLastError() != ERROR_ACCESS_DENIED)
        {
            // No, try to open a new one
            AllocConsole();
        }
    }
    ConInitStdStreams(); // Initialize the Console Standard Streams
}

static BOOL
HandleInstallCommand(CAppDB *db, LPWSTR szCommand, int argcLeft, LPWSTR *argvLeft)
{
    if (argcLeft < 1)
    {
        InitRappsConsole();
        ConResMsgPrintf(StdOut, NULL, IDS_CMD_NEED_PACKAGE_NAME, szCommand);
        return FALSE;
    }

    CAtlList<CAppInfo *> Applications;
    for (int i = 0; i < argcLeft; i++)
    {
        LPCWSTR PackageName = argvLeft[i];
        CAppInfo *AppInfo = db->FindByPackageName(PackageName);
        if (AppInfo)
        {
            Applications.AddTail(AppInfo);
        }
    }

    return DownloadListOfApplications(Applications, TRUE);
}

static BOOL
HandleSetupCommand(CAppDB *db, LPWSTR szCommand, int argcLeft, LPWSTR *argvLeft)
{
    if (argcLeft != 1)
    {
        InitRappsConsole();
        ConResMsgPrintf(StdOut, NULL, IDS_CMD_NEED_FILE_NAME, szCommand);
        return FALSE;
    }

    CAtlList<CAppInfo *> Applications;
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
                CAppInfo *AppInfo = db->FindByPackageName(szPkgName);
                if (AppInfo)
                {
                    Applications.AddTail(AppInfo);
                }
            }
        } while (SetupFindNextLine(&Context, &Context));
    }
    SetupCloseInfFile(InfHandle);

    return DownloadListOfApplications(Applications, TRUE);
}

static BOOL
HandleFindCommand(CAppDB *db, LPWSTR szCommand, int argcLeft, LPWSTR *argvLeft)
{
    if (argcLeft < 1)
    {
        ConResMsgPrintf(StdOut, NULL, IDS_CMD_NEED_PARAMS, szCommand);
        return FALSE;
    }

    CAtlList<CAppInfo *> List;
    db->GetApps(List, ENUM_ALL_AVAILABLE);

    for (int i = 0; i < argcLeft; i++)
    {
        LPCWSTR lpszSearch = argvLeft[i];
        ConResMsgPrintf(StdOut, NULL, IDS_CMD_FIND_RESULT_FOR, lpszSearch);

        POSITION CurrentListPosition = List.GetHeadPosition();
        while (CurrentListPosition)
        {
            CAppInfo *Info = List.GetNext(CurrentListPosition);

            if (SearchPatternMatch(Info->szDisplayName, lpszSearch) || SearchPatternMatch(Info->szComments, lpszSearch))
            {
                ConPrintf(StdOut, L"%s (%s)\n", Info->szDisplayName.GetString(), Info->szIdentifier.GetString());
            }
        }

        ConPrintf(StdOut, L"\n");
    }

    return TRUE;
}

static BOOL
HandleInfoCommand(CAppDB *db, LPWSTR szCommand, int argcLeft, LPWSTR *argvLeft)
{
    if (argcLeft < 1)
    {
        ConResMsgPrintf(StdOut, NULL, IDS_CMD_NEED_PARAMS, szCommand);
        return FALSE;
    }

    for (int i = 0; i < argcLeft; i++)
    {
        LPCWSTR PackageName = argvLeft[i];
        CAppInfo *AppInfo = db->FindByPackageName(PackageName);
        if (!AppInfo)
        {
            ConResMsgPrintf(StdOut, NULL, IDS_CMD_PACKAGE_NOT_FOUND, PackageName);
        }
        else
        {
            ConResMsgPrintf(StdOut, NULL, IDS_CMD_PACKAGE_INFO, PackageName);

            ConPuts(StdOut, AppInfo->szDisplayName);

            if (!AppInfo->szDisplayVersion.IsEmpty())
            {
                ConResPrintf(StdOut, IDS_AINFO_VERSION);
                ConPuts(StdOut, AppInfo->szDisplayVersion);
            }

            CStringW License, Size, UrlSite, UrlDownload;
            AppInfo->GetDisplayInfo(License, Size, UrlSite, UrlDownload);

            if (!License.IsEmpty())
            {
                ConResPrintf(StdOut, IDS_AINFO_LICENSE);
                ConPuts(StdOut, License);
            }

            if (!Size.IsEmpty())
            {
                ConResPrintf(StdOut, IDS_AINFO_SIZE);
                ConPuts(StdOut, Size);
            }

            if (!UrlSite.IsEmpty())
            {
                ConResPrintf(StdOut, IDS_AINFO_URLSITE);
                ConPuts(StdOut, UrlSite);
            }

            if (AppInfo->szComments)
            {
                ConResPrintf(StdOut, IDS_AINFO_DESCRIPTION);
                ConPuts(StdOut, AppInfo->szComments);
            }

            if (!UrlDownload.IsEmpty())
            {
                ConResPrintf(StdOut, IDS_AINFO_URLDOWNLOAD);
                ConPuts(StdOut, UrlDownload);
            }
            ConPuts(StdOut, L"\n");
        }
        ConPuts(StdOut, L"\n");
    }
    return TRUE;
}

static VOID
PrintHelpCommand()
{
    ConPrintf(StdOut, L"\n");
    ConResPuts(StdOut, IDS_APPTITLE);
    ConPrintf(StdOut, L"\n\n");

    ConResPuts(StdOut, IDS_CMD_USAGE);
    ConPrintf(StdOut, L"%ls\n", UsageString);
}

BOOL
ParseCmdAndExecute(LPWSTR lpCmdLine, BOOL bIsFirstLaunch, int nCmdShow)
{
    INT argc;
    LPWSTR *argv = CommandLineToArgvW(lpCmdLine, &argc);
    BOOL bAppwizMode = FALSE;

    if (!argv)
    {
        return FALSE;
    }

    CStringW Directory;
    GetStorageDirectory(Directory);
    CAppDB db(Directory);

    if (argc > 1 && MatchCmdOption(argv[1], CMD_KEY_APPWIZ))
    {
        bAppwizMode = TRUE;
    }

    if (SettingsInfo.bUpdateAtStart || bIsFirstLaunch)
    {
        db.RemoveCached();
    }
    db.UpdateAvailable();
    db.UpdateInstalled();

    if (argc == 1 || bAppwizMode) // RAPPS is launched without options or APPWIZ mode is requested
    {
        // Check whether the RAPPS MainWindow is already launched in another process
        HANDLE hMutex;

        hMutex = CreateMutexW(NULL, FALSE, szWindowClass);
        if ((!hMutex) || (GetLastError() == ERROR_ALREADY_EXISTS))
        {
            /* If already started, find its window */
            HWND hWindow = FindWindowW(szWindowClass, NULL);

            /* Activate window */
            ShowWindow(hWindow, SW_SHOWNORMAL);
            SetForegroundWindow(hWindow);
            if (bAppwizMode)
                PostMessage(hWindow, WM_COMMAND, ID_ACTIVATE_APPWIZ, 0);
            return FALSE;
        }

        CMainWindow wnd(&db, bAppwizMode);
        MainWindowLoop(&wnd, nCmdShow);

        if (hMutex)
            CloseHandle(hMutex);

        return TRUE;
    }

    if (MatchCmdOption(argv[1], CMD_KEY_INSTALL))
    {
        return HandleInstallCommand(&db, argv[1], argc - 2, argv + 2);
    }
    else if (MatchCmdOption(argv[1], CMD_KEY_SETUP))
    {
        return HandleSetupCommand(&db, argv[1], argc - 2, argv + 2);
    }

    InitRappsConsole();

    if (MatchCmdOption(argv[1], CMD_KEY_FIND))
    {
        return HandleFindCommand(&db, argv[1], argc - 2, argv + 2);
    }
    else if (MatchCmdOption(argv[1], CMD_KEY_INFO))
    {
        return HandleInfoCommand(&db, argv[1], argc - 2, argv + 2);
    }
    else if (MatchCmdOption(argv[1], CMD_KEY_HELP) || MatchCmdOption(argv[1], CMD_KEY_HELP_ALT))
    {
        PrintHelpCommand();
        return TRUE;
    }
    else
    {
        // unrecognized/invalid options
        ConResPuts(StdOut, IDS_CMD_INVALID_OPTION);
        PrintHelpCommand();
        return FALSE;
    }
}
