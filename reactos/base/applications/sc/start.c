/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/sc/start.c
 * PURPOSE:     Start a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "sc.h"

BOOL Start(LPCTSTR ServiceName, LPCTSTR *ServiceArgs, INT ArgCount)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSc = NULL;
    LPSERVICE_STATUS_PROCESS pServiceInfo = NULL;
    DWORD BufSiz = 0;
    DWORD BytesNeeded = 0;
    DWORD Ret;

#ifdef SCDBG
{
    LPCTSTR *TmpArgs = ServiceArgs;
    INT TmpCnt = ArgCount;
    _tprintf(_T("service to control - %s\n"), ServiceName);
    _tprintf(_T("Arguments:\n"));
    while (TmpCnt)
    {
        _tprintf(_T("  %s\n"), *TmpArgs);
        TmpArgs++;
        TmpCnt--;
    }
    _tprintf(_T("\n"));
}
#endif /* SCDBG */

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CONNECT);
    if (hSCManager == NULL)
        goto fail;

    hSc = OpenService(hSCManager,
                      ServiceName,
                      SERVICE_START | SERVICE_QUERY_STATUS);

    if (hSc == NULL)
    {
        _tprintf(_T("openService failed\n"));
        goto fail;
    }

    if (! StartService(hSc,
                       ArgCount,
                       ServiceArgs))
    {
		_tprintf(_T("[SC] StartService FAILED %lu:\n\n"), GetLastError());
        goto fail;
    }

    Ret = QueryServiceStatusEx(hSc,
                               SC_STATUS_PROCESS_INFO,
                               NULL,
                               BufSiz,
                               &BytesNeeded);
    if ((Ret != 0) || (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) //FIXME: check this
        goto fail;


    pServiceInfo = (LPSERVICE_STATUS_PROCESS)HeapAlloc(GetProcessHeap(),
                                                       0,
                                                       BytesNeeded);
    if (pServiceInfo == NULL)
        goto fail;

    if (!QueryServiceStatusEx(hSc,
                              SC_STATUS_PROCESS_INFO,
                              (LPBYTE)pServiceInfo,
                              BytesNeeded,
                              &BytesNeeded))
    {
        goto fail;
    }

    PrintServiceEx(ServiceName,
                   pServiceInfo);

    HeapFree(GetProcessHeap(), 0, pServiceInfo);
    CloseServiceHandle(hSc);
    CloseServiceHandle(hSCManager);

    return TRUE;

fail:
    ReportLastError();
    if (pServiceInfo) HeapFree(GetProcessHeap(), 0, pServiceInfo);
    if (hSc) CloseServiceHandle(hSc);
    if (hSCManager) CloseServiceHandle(hSCManager);
    return FALSE;

}
