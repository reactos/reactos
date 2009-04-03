/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/sc/query.c
 * PURPOSE:     queries service info
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */
/*
 * TODO:
 * Allow calling of 2 options e.g.:
 *    type= driver state= inactive
 */

#include "sc.h"

LPTSTR QueryOpts[] = {
    _T("type="),
    _T("state="),
    _T("bufsize="),
    _T("ri="),
    _T("group="),
};


LPSERVICE_STATUS_PROCESS
QueryService(LPCTSTR ServiceName)
{
    SC_HANDLE hSCManager = NULL;
    LPSERVICE_STATUS_PROCESS pServiceInfo = NULL;
    SC_HANDLE hSc = NULL;
    DWORD BufSiz = 0;
    DWORD BytesNeeded = 0;
    DWORD Ret;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_CONNECT);
    if (hSCManager == NULL)
    {
        ReportLastError();
        return NULL;
    }

    hSc = OpenService(hSCManager,
                      ServiceName,
                      SERVICE_QUERY_STATUS);
    if (hSc == NULL)
        goto fail;

    Ret = QueryServiceStatusEx(hSc,
                               SC_STATUS_PROCESS_INFO,
                               NULL,
                               BufSiz,
                               &BytesNeeded);
    if ((Ret != 0) || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
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

    CloseServiceHandle(hSc);
    CloseServiceHandle(hSCManager);
    return pServiceInfo;

fail:
    ReportLastError();
    if (pServiceInfo) HeapFree(GetProcessHeap(), 0, pServiceInfo);
    if (hSc) CloseServiceHandle(hSc);
    if (hSCManager) CloseServiceHandle(hSCManager);
    return NULL;
}


static BOOL
EnumServices(ENUM_SERVICE_STATUS_PROCESS **pServiceStatus,
             DWORD ServiceType,
             DWORD ServiceState)
{
    SC_HANDLE hSCManager;
    DWORD BufSize = 0;
    DWORD BytesNeeded = 0;
    DWORD ResumeHandle = 0;
    DWORD NumServices = 0;
    DWORD Ret;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        ReportLastError();
        return FALSE;
    }

    Ret = EnumServicesStatusEx(hSCManager,
                               SC_ENUM_PROCESS_INFO,
                               ServiceType,
                               ServiceState,
                               (LPBYTE)*pServiceStatus,
                               BufSize,
                               &BytesNeeded,
                               &NumServices,
                               &ResumeHandle,
                               0);

    if ((Ret == 0) && (GetLastError() == ERROR_MORE_DATA))
    {
        *pServiceStatus = (ENUM_SERVICE_STATUS_PROCESS *)
                          HeapAlloc(GetProcessHeap(),
                                    0,
                                    BytesNeeded);
        if (*pServiceStatus != NULL)
        {
            if (EnumServicesStatusEx(hSCManager,
                                     SC_ENUM_PROCESS_INFO,
                                     ServiceType,
                                     ServiceState,
                                     (LPBYTE)*pServiceStatus,
                                     BytesNeeded,
                                     &BytesNeeded,
                                     &NumServices,
                                     &ResumeHandle,
                                     0))
            {
                return NumServices;
            }
        }
    }

    ReportLastError();
    if (pServiceStatus)
        HeapFree(GetProcessHeap(), 0, *pServiceStatus);

    return NumServices;
}


BOOL
Query(LPCTSTR *ServiceArgs,
      DWORD ArgCount,
      BOOL bExtended)
{
    LPENUM_SERVICE_STATUS_PROCESS pServiceStatus = NULL;
    DWORD NumServices = 0;
    //DWORD ServiceType;
    //DWORD ServiceState;
    BOOL bServiceName = TRUE;
    DWORD OptSize, i;

    LPCTSTR *TmpArgs;
    INT TmpCnt;

#ifdef SCDBG
    TmpArgs = ServiceArgs;
    TmpCnt = ArgCount;
    _tprintf(_T("Arguments:\n"));
    while (TmpCnt)
    {
        _tprintf(_T("  %s\n"), *TmpArgs);
        TmpArgs++;
        TmpCnt--;
    }
    _tprintf(_T("\n"));
#endif /* SCDBG */

    /* display all running services and drivers */
    if (ArgCount == 0)
    {
        NumServices = EnumServices(&pServiceStatus,
                                   SERVICE_WIN32,
                                   SERVICE_ACTIVE);

        if (NumServices != 0)
        {
            for (i=0; i < NumServices; i++)
            {
                PrintService(pServiceStatus[i].lpServiceName,
                             &pServiceStatus[i].ServiceStatusProcess,
                             bExtended);
            }

            _tprintf(_T("number : %lu\n"), NumServices);

            if (pServiceStatus)
                HeapFree(GetProcessHeap(), 0, pServiceStatus);

            return TRUE;
        }

        return FALSE;
    }

    TmpArgs = ServiceArgs;
    TmpCnt = ArgCount;
    OptSize = sizeof(QueryOpts) / sizeof(QueryOpts[0]);
    while (TmpCnt--)
    {
        for (i=0; i < OptSize; i++)
        {
            if (!lstrcmpi(*TmpArgs, QueryOpts[i]))
            {
                bServiceName = FALSE;
            }
        }
        TmpArgs++;
    }


    /* FIXME: parse options */


    /* print only the service requested */
    if (bServiceName)
    {
        LPSERVICE_STATUS_PROCESS pStatus;
        LPCTSTR ServiceName = *ServiceArgs;

        pStatus = QueryService(ServiceName);
        if (pStatus)
        {
            PrintService(ServiceName,
                         pStatus,
                         bExtended);
        }
    }

    if (pServiceStatus)
        HeapFree(GetProcessHeap(), 0, pServiceStatus);

    return TRUE;
}
