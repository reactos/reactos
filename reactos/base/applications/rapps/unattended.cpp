#include "unattended.h"
#include "defines.h"
#include "available.h"
#include "dialogs.h"

#include "setupapi.h"

BOOL CmdParser(LPWSTR lpCmdLine)
{
    INT argc;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
    ATL::CString szName;

    if (!argv || argc < 2)
    {
        return FALSE;
    }

    // Setup key - single app expected
    // TODO: add multiple apps
    // TODO: use DB filenames as names because they're shorter

    // app setup
    ATL::CSimpleArray<ATL::CStringW> arrNames;
    if (!StrCmpW(argv[0], CMD_KEY_INSTALL))
    {
        for (int i = 1; i < argc; ++i)
        {
            arrNames.Add(argv[i]);
        }       
    } 
    else 
    if (!StrCmpW(argv[0], CMD_KEY_SETUP))
    {
        //TODO: inf file loading
        HINF InfHandle = SetupOpenInfFileW(argv[1], NULL, INF_STYLE_WIN4, NULL);
        if (InfHandle == INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }

        INFCONTEXT Context;
        if (!SetupFindFirstLineW(InfHandle, L"RAPPS", L"Install", &Context))
        {
            return FALSE;
        }

        WCHAR szName[MAX_PATH];
        do
        {
            if (SetupGetStringFieldW(&Context, 1, szName, MAX_PATH, NULL))
            {
                arrNames.Add(szName);
            }
        } 
        while (SetupFindNextLine(&Context, &Context));
        SetupCloseInfFile(InfHandle);
    }

    CAvailableApps apps;
    apps.EnumAvailableApplications(ENUM_ALL_AVAILABLE, NULL);
    ATL::CSimpleArray<PAPPLICATION_INFO> arrAppInfo = apps.FindInfoList(arrNames);
    if (arrAppInfo.GetSize() > 0)
    {
        CDownloadManager::DownloadListOfApplications(arrAppInfo, TRUE);
        return TRUE;
    }
    
    return FALSE;
}

