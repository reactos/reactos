/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/description.c
 * PURPOSE:     Query/Set the service description
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 *
 */

#include "sc.h"

BOOL QueryDescription(LPCTSTR ServiceName)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    DWORD cbBytesNeeded = 0;
    LPSERVICE_DESCRIPTION pServiceDescription = NULL;

#ifdef SCDBG
    _tprintf(_T("service to show description - %s\n\n"), ServiceName);
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

    hService = OpenService(hManager, ServiceName, SERVICE_QUERY_CONFIG);
    if (hService == NULL)
    {
        _tprintf(_T("[SC] OpenService FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceConfig2(hService,
                             SERVICE_CONFIG_DESCRIPTION,
                             NULL,
                             0,
                             &cbBytesNeeded))
    {
        if (cbBytesNeeded == 0)
        {
            _tprintf(_T("[SC] QueryServiceConfig2 FAILED %lu:\n\n"), GetLastError());
            bResult = FALSE;
            goto done;
        }
    }

    pServiceDescription = HeapAlloc(GetProcessHeap(), 0, cbBytesNeeded);
    if (pServiceDescription == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        _tprintf(_T("[SC] HeapAlloc FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceConfig2(hService,
                             SERVICE_CONFIG_DESCRIPTION,
                             (LPBYTE)pServiceDescription,
                             cbBytesNeeded,
                             &cbBytesNeeded))
    {
        _tprintf(_T("[SC] QueryServiceConfig2 FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    _tprintf(_T("[SC] QueryServiceConfig2 SUCCESS\n\n"));

    _tprintf(_T("SERVICE_NAME: %s\n"), ServiceName);
    _tprintf(_T("        DESCRIPTION        : %s\n"),
             (pServiceDescription->lpDescription) ? pServiceDescription->lpDescription : _T(""));

done:
    if (bResult == FALSE)
        ReportLastError();

    if (pServiceDescription != NULL)
        HeapFree(GetProcessHeap(), 0, pServiceDescription);

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}


BOOL SetDescription(LPCTSTR ServiceName, LPCTSTR Description)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    SERVICE_DESCRIPTION ServiceDescription;

#ifdef SCDBG
    _tprintf(_T("service to set description - %s\n\n"), ServiceName);
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

    hService = OpenService(hManager, ServiceName, SERVICE_CHANGE_CONFIG);
    if (hService == NULL)
    {
        _tprintf(_T("[SC] OpenService FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    ServiceDescription.lpDescription = (LPTSTR)Description;

    if (!ChangeServiceConfig2(hService,
                              SERVICE_CONFIG_DESCRIPTION,
                              (LPBYTE)&ServiceDescription))
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
