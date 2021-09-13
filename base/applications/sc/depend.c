/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/depend.c
 * PURPOSE:
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 *
 */

#include "sc.h"

BOOL EnumDepend(LPCTSTR ServiceName)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    DWORD BufferSize = 0;
    DWORD EntriesRead = 0;
    LPENUM_SERVICE_STATUS pBuffer = NULL;
    DWORD i;

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        _tprintf(_T("[SC] OpenSCManager FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    hService = OpenService(hManager, ServiceName, SERVICE_ENUMERATE_DEPENDENTS);
    if (hService == NULL)
    {
        _tprintf(_T("[SC] OpenService FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!EnumDependentServices(hService,
                               SERVICE_STATE_ALL,
                               NULL,
                               0,
                               &BufferSize,
                               &EntriesRead))
    {
        if (BufferSize == 0)
        {
            _tprintf(_T("[SC] EnumDependentServices FAILED %lu:\n\n"), GetLastError());
            bResult = FALSE;
            goto done;
        }
    }

    pBuffer = HeapAlloc(GetProcessHeap(), 0, BufferSize);
    if (pBuffer == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        _tprintf(_T("[SC] HeapAlloc FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!EnumDependentServices(hService,
                               SERVICE_STATE_ALL,
                               pBuffer,
                               BufferSize,
                               &BufferSize,
                               &EntriesRead))
    {
        _tprintf(_T("[SC] EnumDependentServices FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    _tprintf(_T("Enum: entriesRead = %lu\n"), EntriesRead);

    for (i = 0; i < EntriesRead; i++)
    {
        _tprintf(_T("\n"));
        _tprintf(_T("SERVICE_NAME: %s\n"), pBuffer[i].lpServiceName);
        _tprintf(_T("DISPLAY_NAME: %s\n"), pBuffer[i].lpDisplayName);
        PrintServiceStatus(&pBuffer[i].ServiceStatus);
    }

done:
    if (bResult == FALSE)
        ReportLastError();

    if (pBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, pBuffer);

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}
