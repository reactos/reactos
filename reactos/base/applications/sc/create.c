/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/create.c
 * PURPOSE:     Create a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Roel Messiant <roelmessiant@gmail.com>
 *
 */

#include "sc.h"

BOOL Create(LPCTSTR *ServiceArgs, INT ArgCount)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bRet = TRUE;

    INT i;
    INT Length;
    LPTSTR lpBuffer = NULL;
    SERVICE_CREATE_INFO ServiceInfo;

    if (!ParseCreateConfigArguments(ServiceArgs, ArgCount, FALSE, &ServiceInfo))
    {
        CreateUsage();
        return FALSE;
    }

    if (!ServiceInfo.dwServiceType)
        ServiceInfo.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    if (!ServiceInfo.dwStartType)
        ServiceInfo.dwStartType = SERVICE_DEMAND_START;

    if (!ServiceInfo.dwErrorControl)
        ServiceInfo.dwErrorControl = SERVICE_ERROR_NORMAL;

    if (ServiceInfo.lpDependencies)
    {
        Length = lstrlen(ServiceInfo.lpDependencies);

        lpBuffer = HeapAlloc(GetProcessHeap(),
                             0,
                            (Length + 2) * sizeof(TCHAR));

        for (i = 0; i < Length; i++)
            if (ServiceInfo.lpDependencies[i] == _T('/'))
                lpBuffer[i] = 0;
            else
                lpBuffer[i] = ServiceInfo.lpDependencies[i];

        lpBuffer[Length] = 0;
        lpBuffer[Length + 1] = 0;

        ServiceInfo.lpDependencies = lpBuffer;
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

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager == NULL)
    {
        _tprintf(_T("[SC] OpenSCManager FAILED %lu:\n\n"), GetLastError());
        bRet = FALSE;
        goto done;
    }

    hService = CreateService(hSCManager,
                             ServiceInfo.lpServiceName,
                             ServiceInfo.lpDisplayName,
                             SERVICE_ALL_ACCESS,
                             ServiceInfo.dwServiceType,
                             ServiceInfo.dwStartType,
                             ServiceInfo.dwErrorControl,
                             ServiceInfo.lpBinaryPathName,
                             ServiceInfo.lpLoadOrderGroup,
                             ServiceInfo.bTagId ? &ServiceInfo.dwTagId : NULL,
                             ServiceInfo.lpDependencies,
                             ServiceInfo.lpServiceStartName,
                             ServiceInfo.lpPassword);
    if (hService == NULL)
    {
        _tprintf(_T("[SC] CreateService FAILED %lu:\n\n"), GetLastError());
        bRet = FALSE;
        goto done;
    }

    _tprintf(_T("[SC] CreateService SUCCESS\n\n"));

done:
    if (bRet == FALSE)
        ReportLastError();

    if (hService)
        CloseServiceHandle(hService);

    if (hSCManager)
        CloseServiceHandle(hSCManager);

    if (lpBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, lpBuffer);

    return bRet;
}
