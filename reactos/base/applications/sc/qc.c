/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/qc.c
 * PURPOSE:     Show the service configuration
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 *
 */

#include "sc.h"

BOOL QueryConfig(LPCTSTR ServiceName)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    DWORD cbBytesNeeded = 0;
    LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;
    LPWSTR lpPtr;
    INT nLen, i;

#ifdef SCDBG
    _tprintf(_T("service to show configuration - %s\n\n"), ServiceName);
#endif

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    hService = OpenService(hManager, ServiceName, SERVICE_QUERY_CONFIG);
    if (hService == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceConfig(hService,
                            NULL,
                            0,
                            &cbBytesNeeded))
    {
        if (cbBytesNeeded == 0)
        {
            bResult = FALSE;
            goto done;
        }
    }

    pServiceConfig = HeapAlloc(GetProcessHeap(), 0, cbBytesNeeded);
    if (pServiceConfig == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceConfig(hService,
                            pServiceConfig,
                            cbBytesNeeded,
                            &cbBytesNeeded))
    {
        bResult = FALSE;
        goto done;
    }

    _tprintf(_T("SERVICE_NAME: %s\n"), ServiceName);
    _tprintf(_T("        TYPE               : %-3lx "), pServiceConfig->dwServiceType);
    switch (pServiceConfig->dwServiceType)
    {
        case SERVICE_KERNEL_DRIVER:
            _tprintf(_T("KERNEL_DRIVER\n"));
            break;

        case SERVICE_FILE_SYSTEM_DRIVER:
            _tprintf(_T("FILE_SYSTEM_DRIVER\n"));
            break;

        case SERVICE_WIN32_OWN_PROCESS:
            _tprintf(_T("WIN32_OWN_PROCESS\n"));
            break;

        case SERVICE_WIN32_SHARE_PROCESS:
            _tprintf(_T("WIN32_SHARE_PROCESS\n"));
            break;

        case SERVICE_WIN32_OWN_PROCESS + SERVICE_INTERACTIVE_PROCESS:
            _tprintf(_T("WIN32_OWN_PROCESS (interactive)\n"));
            break;

        case SERVICE_WIN32_SHARE_PROCESS + SERVICE_INTERACTIVE_PROCESS:
            _tprintf(_T("WIN32_SHARE_PROCESS (interactive)\n"));
            break;

        default:
            _tprintf(_T("\n"));
            break;
    }

    _tprintf(_T("        START_TYPE         : %-3lx "), pServiceConfig->dwStartType);
    switch (pServiceConfig->dwStartType)
    {
        case SERVICE_BOOT_START:
            _tprintf(_T("BOOT_START\n"));
            break;

        case SERVICE_SYSTEM_START:
            _tprintf(_T("SYSTEM_START\n"));
            break;

        case SERVICE_AUTO_START:
            _tprintf(_T("AUTO_START\n"));
            break;

        case SERVICE_DEMAND_START:
            _tprintf(_T("DEMAND_START\n"));
            break;

        case SERVICE_DISABLED:
            _tprintf(_T("DISABLED\n"));
            break;

        default:
            _tprintf(_T("\n"));
            break;
    }

    _tprintf(_T("        ERROR_CONTROL      : %-3lx "), pServiceConfig->dwErrorControl);
    switch (pServiceConfig->dwErrorControl)
    {
        case SERVICE_ERROR_IGNORE:
            _tprintf(_T("IGNORE\n"));
            break;

        case SERVICE_ERROR_NORMAL:
            _tprintf(_T("NORMAL\n"));
            break;

        case SERVICE_ERROR_SEVERE:
            _tprintf(_T("SEVERE\n"));
            break;

        case SERVICE_ERROR_CRITICAL:
            _tprintf(_T("CRITICAL\n"));
            break;

        default:
            _tprintf(_T("\n"));
            break;
    }

    _tprintf(_T("        BINARY_PATH_NAME   : %s\n"), pServiceConfig->lpBinaryPathName);
    _tprintf(_T("        LOAD_ORDER_GROUP   : %s\n"), pServiceConfig->lpLoadOrderGroup);
    _tprintf(_T("        TAG                : %lu\n"), pServiceConfig->dwTagId);
    _tprintf(_T("        DISPLAY_NAME       : %s\n"), pServiceConfig->lpDisplayName);
    _tprintf(_T("        DEPENDENCIES       : "));
    lpPtr = pServiceConfig->lpDependencies;
    i = 0;
    while (*lpPtr != _T('\0'))
    {
       nLen = _tcslen(lpPtr);
       if (i != 0)
           _tprintf(_T("\n                           : "));
       _tprintf(_T("%s"), lpPtr);
       lpPtr = lpPtr + nLen + 1;
       i++;
    }
    _tprintf(_T("\n"));

    _tprintf(_T("        SERVICE_START_NAME : %s\n"), pServiceConfig->lpServiceStartName);

done:
    if (bResult == FALSE)
        ReportLastError();

    if (pServiceConfig != NULL)
        HeapFree(GetProcessHeap(), 0, pServiceConfig);

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}
