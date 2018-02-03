/*
* PROJECT:      ReactOS Applications Manager
* LICENSE:      GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* FILE:         base/applications/rapps/unattended.cpp
* PURPOSE:      Functions to parse command-line flags and process them
* COPYRIGHT:    Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
*/
#include "rapps.h"

#include "unattended.h"

#include <setupapi.h>

#define MIN_ARGS 2

BOOL UseCmdParameters(LPWSTR lpCmdLine)
{
    INT argc;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);

    if (!argv || argc < MIN_ARGS)
    {
        return FALSE;
    }

    // TODO: use DB filenames as names because they're shorter
    ATL::CSimpleArray<ATL::CStringW> arrNames;
    if (!StrCmpW(argv[0], CMD_KEY_INSTALL))
    {
        for (INT i = 1; i < argc; ++i)
        {
            arrNames.Add(argv[i]);
        }       
    } 
    else 
    if (!StrCmpW(argv[0], CMD_KEY_SETUP))
    {
        HINF InfHandle = SetupOpenInfFileW(argv[1], NULL, INF_STYLE_WIN4, NULL);
        if (InfHandle == INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }

        INFCONTEXT Context;
        if (SetupFindFirstLineW(InfHandle, L"RAPPS", L"Install", &Context))
        {
            WCHAR szName[MAX_PATH];
            do
            {
                if (SetupGetStringFieldW(&Context, 1, szName, _countof(szName), NULL))
                {
                    arrNames.Add(szName);
                }
            } while (SetupFindNextLine(&Context, &Context));
        }
        SetupCloseInfFile(InfHandle);
    }
    else
    {
        return FALSE;
    }

    CAvailableApps apps;
    apps.UpdateAppsDB();
    apps.Enum(ENUM_ALL_AVAILABLE, NULL);

    ATL::CSimpleArray<CAvailableApplicationInfo> arrAppInfo = apps.FindInfoList(arrNames);
    if (arrAppInfo.GetSize() > 0)
    {
        CDownloadManager::DownloadListOfApplications(arrAppInfo, TRUE);
        return TRUE;
    }
    
    return FALSE;
}
