/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions to parse command-line flags and process them
 * COPYRIGHT:   Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 *              Copyright 2020 He Yang (1160386205@qq.com)
 */

#include "gui.h"
#include "unattended.h"
#include "configparser.h"
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

static CAppInfo *
SearchForAppWithDisplayName(CAppDB &db, AppsCategories Type, LPCWSTR Name)
{
    CAtlList<CAppInfo *> List;
    db.GetApps(List, Type);
    for (POSITION it = List.GetHeadPosition(); it;)
    {
        CAppInfo *Info = List.GetNext(it);
        if (SearchPatternMatch(Info->szDisplayName, Name))
        {
            return Info;
        }
    }
    return NULL;
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
HandleUninstallCommand(CAppDB &db, UINT argcLeft, LPWSTR *argvLeft)
{
    UINT argi = 0, silent = FALSE, byregkeyname = FALSE;
    for (; argcLeft; ++argi, --argcLeft)
    {
        if (!StrCmpIW(argvLeft[argi], L"/S"))
            ++silent;
        else if (!StrCmpIW(argvLeft[argi], L"/K"))
            ++byregkeyname;
        else
            break;
    }
    if (argcLeft != 1)
        return FALSE;

    CStringW buf;
    LPCWSTR name = argvLeft[argi];
    BOOL retval = FALSE;
    CAppInfo *pInfo = NULL, *pDelete = NULL;

    if (!byregkeyname)
    {
        for (UINT i = ENUM_INSTALLED_MIN; !pInfo && i <= ENUM_INSTALLED_MAX; ++i)
        {
            pInfo = SearchForAppWithDisplayName(db, AppsCategories(i), name);
        }

        if (!pInfo)
        {
            CAvailableApplicationInfo *p = db.FindAvailableByPackageName(name);
            if (p)
            {
                CConfigParser *cp = p->GetConfigParser();
                if (cp && cp->GetString(DB_REGNAME, buf) && !buf.IsEmpty())
                {
                    name = buf.GetString();
                    byregkeyname = TRUE;
                }
            }
        }
    }

    if (byregkeyname)
    {
        // Force a specific key type if requested (<M|U>[32|64]<\\KeyName>)
        if (name[0])
        {
            REGSAM wow = 0;
            UINT i = 1;
            if (name[i] == '3' && name[i + 1])
                wow = KEY_WOW64_32KEY, i += 2;
            else if (name[i] == '6' && name[i + 1])
                wow = KEY_WOW64_64KEY, i += 2;

            if (name[i++] == '\\')
            {
                pInfo = CAppDB::CreateInstalledAppInstance(name + i, name[0] == 'U', wow);
            }
        }

        if (!pInfo)
        {
            pInfo = CAppDB::CreateInstalledAppByRegistryKey(name);
        }
        pDelete = pInfo;
    }

    if (pInfo)
    {
        retval = pInfo->UninstallApplication(silent ? UCF_SILENT : UCF_NONE);
    }
    delete pDelete;
    return retval;
}

static BOOL
HandleGenerateInstallerCommand(CAppDB &db, UINT argcLeft, LPWSTR *argvLeft)
{
    if (argcLeft != 2)
        return FALSE;

    CAvailableApplicationInfo *pAI = db.FindAvailableByPackageName(argvLeft[0]);
    if (!pAI)
        return FALSE;

    return ExtractAndRunGeneratedInstaller(*pAI, argvLeft[1]);
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
    if (!argv)
        return FALSE;

    CStringW Directory;
    GetStorageDirectory(Directory);
    CAppDB db(Directory);

    BOOL bAppwizMode = (argc > 1 && MatchCmdOption(argv[1], CMD_KEY_APPWIZ));
    if (!bAppwizMode)
    {
        if (SettingsInfo.bUpdateAtStart || bIsFirstLaunch)
            db.RemoveCached();

        db.UpdateAvailable();
    }

    db.UpdateInstalled();

    if (argc == 1 || bAppwizMode) // RAPPS is launched without options or APPWIZ mode is requested
    {
        // Check whether the RAPPS MainWindow is already launched in another process
        CStringW szWindowText(MAKEINTRESOURCEW(bAppwizMode ? IDS_APPWIZ_TITLE : IDS_APPTITLE));
        LPCWSTR pszMutex = bAppwizMode ? L"RAPPWIZ" : szWindowClass;

        HANDLE hMutex = CreateMutexW(NULL, FALSE, pszMutex);
        if ((!hMutex) || (GetLastError() == ERROR_ALREADY_EXISTS))
        {
            /* If already started, find its window */
            HWND hWindow;
            for (int wait = 2500, inter = 250; wait > 0; wait -= inter)
            {
                if ((hWindow = FindWindowW(szWindowClass, szWindowText)) != NULL)
                    break;
                Sleep(inter);
            }

            if (hWindow)
            {
                /* Activate the window in the other instance */
                ShowWindow(hWindow, SW_SHOW);
                SwitchToThisWindow(hWindow, TRUE);
                if (bAppwizMode)
                    PostMessage(hWindow, WM_COMMAND, ID_ACTIVATE_APPWIZ, 0);

                if (hMutex)
                    CloseHandle(hMutex);

                return FALSE;
            }
        }
        szWindowText.Empty();

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
    else if (MatchCmdOption(argv[1], CMD_KEY_UNINSTALL))
    {
        return HandleUninstallCommand(db, argc - 2, argv + 2);
    }
    else if (MatchCmdOption(argv[1], CMD_KEY_GENINST))
    {
        return HandleGenerateInstallerCommand(db, argc - 2, argv + 2);
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
