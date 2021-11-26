/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/name.c
 * PURPOSE:
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 *
 */

#include "sc.h"

BOOL GetDisplayName(LPCTSTR ServiceName)
{
    SC_HANDLE hManager = NULL;
    BOOL bResult = TRUE;
    DWORD BufferSize = 0;
    LPTSTR pBuffer = NULL;

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        _tprintf(_T("[SC] OpenSCManager FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!GetServiceDisplayName(hManager,
                               ServiceName,
                               NULL,
                               &BufferSize))
    {
        if (BufferSize == 0)
        {
            _tprintf(_T("[SC] GetServiceDisplayName FAILED %lu:\n\n"), GetLastError());
            bResult = FALSE;
            goto done;
        }
    }

    pBuffer = HeapAlloc(GetProcessHeap(), 0, (BufferSize + 1) * sizeof(TCHAR));
    if (pBuffer == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        _tprintf(_T("[SC] HeapAlloc FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    BufferSize++;
    if (!GetServiceDisplayName(hManager,
                               ServiceName,
                               pBuffer,
                               &BufferSize))
    {
        _tprintf(_T("[SC] GetServiceDisplayName FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    _tprintf(_T("[SC] GetServiceDisplayName SUCCESS  Name = %s\n"), pBuffer);

done:
    if (bResult == FALSE)
        ReportLastError();

    if (pBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, pBuffer);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}

BOOL GetKeyName(LPCTSTR ServiceName)
{
    SC_HANDLE hManager = NULL;
    BOOL bResult = TRUE;
    DWORD BufferSize = 0;
    LPTSTR pBuffer = NULL;

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        _tprintf(_T("[SC] OpenSCManager FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    if (!GetServiceKeyName(hManager,
                           ServiceName,
                           NULL,
                           &BufferSize))
    {
        if (BufferSize == 0)
        {
            _tprintf(_T("[SC] GetServiceKeyName FAILED %lu:\n\n"), GetLastError());
            bResult = FALSE;
            goto done;
        }
    }

    pBuffer = HeapAlloc(GetProcessHeap(), 0, (BufferSize + 1) * sizeof(TCHAR));
    if (pBuffer == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        _tprintf(_T("[SC] HeapAlloc FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    BufferSize++;
    if (!GetServiceKeyName(hManager,
                           ServiceName,
                           pBuffer,
                           &BufferSize))
    {
        _tprintf(_T("[SC] GetServiceKeyName FAILED %lu:\n\n"), GetLastError());
        bResult = FALSE;
        goto done;
    }

    _tprintf(_T("[SC] GetServiceKeyName SUCCESS  Name = %s\n"), pBuffer);

done:
    if (bResult == FALSE)
        ReportLastError();

    if (pBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, pBuffer);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}
