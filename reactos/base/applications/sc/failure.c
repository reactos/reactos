/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/failure.c
 * PURPOSE:     Set the service failure actions
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 */

#include "sc.h"

BOOL
SetFailure(
    LPCTSTR *ServiceArgs,
    INT ArgCount)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    SERVICE_FAILURE_ACTIONS FailureActions;
    LPCTSTR lpServiceName = NULL;

    if (!ParseFailureArguments(ServiceArgs, ArgCount, &lpServiceName, &FailureActions))
    {
        SetFailureUsage();
        return FALSE;
    }

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
                           lpServiceName,
                           SERVICE_CHANGE_CONFIG);
    if (hService == NULL)
    {
        _tprintf(_T("[SC] OpenService FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!ChangeServiceConfig2(hService,
                              SERVICE_CONFIG_FAILURE_ACTIONS,
                              (LPBYTE)&FailureActions))
    {
        _tprintf(_T("[SC] ChangeServiceConfig2 FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    _tprintf(_T("[SC] ChangeServiceConfig2 SUCCESS\n\n"));

done:
    if (bResult == FALSE)
        ReportLastError();

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}
