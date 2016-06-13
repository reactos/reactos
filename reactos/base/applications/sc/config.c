/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/config.c
 * PURPOSE:     Set the service configuration
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 *
 */

#include "sc.h"

BOOL SetConfig(LPCTSTR *ServiceArgs, INT ArgCount)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    SERVICE_CREATE_INFO ServiceInfo;

    if (!ParseCreateConfigArguments(ServiceArgs, ArgCount, TRUE, &ServiceInfo))
    {
        SetConfigUsage();
        return FALSE;
    }

#ifdef SCDBG
    _tprintf(_T("service name - %s\n"), ServiceInfo.lpServiceName);
    _tprintf(_T("display name - %s\n"), ServiceInfo.lpDisplayName);
    _tprintf(_T("service type - %lu\n"), ServiceInfo.dwServiceType);
    _tprintf(_T("start type - %lu\n"), ServiceInfo.dwStartType);
    _tprintf(_T("error control - %lu\n"), ServiceInfo.dwErrorControl);
    _tprintf(_T("Binary path - %s\n"), ServiceInfo.lpBinaryPathName);
    _tprintf(_T("load order group - %s\n"), ServiceInfo.lpLoadOrderGroup);
    _tprintf(_T("tag - %lu\n"), ServiceInfo.dwTagId);
    _tprintf(_T("dependencies - %s\n"), ServiceInfo.lpDependencies);
    _tprintf(_T("account start name - %s\n"), ServiceInfo.lpServiceStartName);
    _tprintf(_T("account password - %s\n"), ServiceInfo.lpPassword);
#endif

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        _tprintf(_T("[SC] OpenSCManager FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    hService = OpenService(hManager,
                           ServiceInfo.lpServiceName,
                           SERVICE_CHANGE_CONFIG);
    if (hService == NULL)
    {
        _tprintf(_T("[SC] OpenService FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!ChangeServiceConfig(hService,
                             ServiceInfo.dwServiceType,
                             ServiceInfo.dwStartType,
                             ServiceInfo.dwErrorControl,
                             ServiceInfo.lpBinaryPathName,
                             ServiceInfo.lpLoadOrderGroup,
                             ServiceInfo.bTagId ? &ServiceInfo.dwTagId : NULL,
                             ServiceInfo.lpDependencies,
                             ServiceInfo.lpServiceStartName,
                             ServiceInfo.lpPassword,
                             ServiceInfo.lpDisplayName))
    {
        _tprintf(_T("[SC] ChangeServiceConfig FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    _tprintf(_T("[SC] ChangeServiceConfig SUCCESS\n\n"));

done:
    if (bResult == FALSE)
        ReportLastError();

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}
