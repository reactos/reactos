/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/query.c
 * PURPOSE:     queries service info
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2018 Eric Kohl <eric.kohl@reactos.org>
 */

#include "sc.h"

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


static
DWORD
EnumServices(ENUM_SERVICE_STATUS_PROCESS **pServiceStatus,
             DWORD dwServiceType,
             DWORD dwServiceState,
             DWORD dwBufferSize,
             DWORD dwResumeIndex,
             LPCTSTR pszGroupName)
{
    SC_HANDLE hSCManager;
    DWORD BytesNeeded = 0;
    DWORD ResumeHandle = dwResumeIndex;
    DWORD NumServices = 0;
    BOOL Ret;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        ReportLastError();
        return 0;
    }

    if (dwBufferSize == 0)
    {
        Ret = EnumServicesStatusEx(hSCManager,
                                   SC_ENUM_PROCESS_INFO,
                                   dwServiceType,
                                   dwServiceState,
                                   (LPBYTE)*pServiceStatus,
                                   dwBufferSize,
                                   &BytesNeeded,
                                   &NumServices,
                                   &ResumeHandle,
                                   pszGroupName);
        if ((Ret == 0) && (GetLastError() != ERROR_MORE_DATA))
        {
            ReportLastError();
            return 0;
        }

        dwBufferSize = BytesNeeded;
    }

    *pServiceStatus = (ENUM_SERVICE_STATUS_PROCESS *)
                      HeapAlloc(GetProcessHeap(),
                                0,
                                dwBufferSize);
    if (*pServiceStatus != NULL)
    {
        if (EnumServicesStatusEx(hSCManager,
                                 SC_ENUM_PROCESS_INFO,
                                 dwServiceType,
                                 dwServiceState,
                                 (LPBYTE)*pServiceStatus,
                                 dwBufferSize,
                                 &BytesNeeded,
                                 &NumServices,
                                 &ResumeHandle,
                                 pszGroupName))
        {
            CloseServiceHandle(hSCManager);
            return NumServices;
        }
    }

    ReportLastError();
    if (*pServiceStatus)
        HeapFree(GetProcessHeap(), 0, *pServiceStatus);

    CloseServiceHandle(hSCManager);

    return NumServices;
}


static
BOOL
ParseQueryArguments(
    IN LPCTSTR *ServiceArgs,
    IN INT ArgCount,
    OUT PDWORD pdwServiceType,
    OUT PDWORD pdwServiceState,
    OUT PDWORD pdwBufferSize,
    OUT PDWORD pdwResumeIndex,
    OUT LPCTSTR *ppszGroupName,
    OUT LPCTSTR *ppszServiceName)
{
    INT TmpCount, TmpIndex;
    DWORD dwValue;

    TmpCount = ArgCount;
    TmpIndex = 0;
    while (TmpCount > 0)
    {
        if (!lstrcmpi(ServiceArgs[TmpIndex], _T("type=")))
        {
            TmpIndex++;
            TmpCount--;

            if (TmpCount > 0)
            {
                if (!lstrcmpi(ServiceArgs[TmpIndex], _T("service")))
                {
                    *pdwServiceType = SERVICE_WIN32;
                }
                else if (!lstrcmpi(ServiceArgs[TmpIndex], _T("driver")))
                {
                    *pdwServiceType = SERVICE_DRIVER;
                }
                else if (!lstrcmpi(ServiceArgs[TmpIndex], _T("all")))
                {
                    *pdwServiceType = SERVICE_TYPE_ALL;
                }
                else if (!lstrcmpi(ServiceArgs[TmpIndex], _T("interact")))
                {
                    *pdwServiceType |= SERVICE_INTERACTIVE_PROCESS;
                }
                else
                {
                    _tprintf(_T("ERROR following \"type=\"!\nMust be one of: all, driver, interact, service.\n"));
                    return FALSE;
                }

                TmpIndex++;
                TmpCount--;
            }
        }
        else if (!lstrcmpi(ServiceArgs[TmpIndex], _T("state=")))
        {
            TmpIndex++;
            TmpCount--;

            if (TmpCount > 0)
            {
                if (!lstrcmpi(ServiceArgs[TmpIndex], _T("active")))
                {
                    *pdwServiceState = SERVICE_ACTIVE;
                }
                else if (!lstrcmpi(ServiceArgs[TmpIndex], _T("inactive")))
                {
                    *pdwServiceState = SERVICE_INACTIVE;
                }
                else if (!lstrcmpi(ServiceArgs[TmpIndex], _T("all")))
                {
                    *pdwServiceState = SERVICE_STATE_ALL;
                }
                else
                {
                    _tprintf(_T("ERROR following \"state=\"!\nMust be one of: active, all, inactive.\n"));
                    return FALSE;
                }

                TmpIndex++;
                TmpCount--;
            }
        }
        else if (!lstrcmpi(ServiceArgs[TmpIndex], _T("bufsize=")))
        {
            TmpIndex++;
            TmpCount--;

            if (TmpCount > 0)
            {
                dwValue = _tcstoul(ServiceArgs[TmpIndex], NULL, 10);
                if (dwValue > 0)
                {
                    *pdwBufferSize = dwValue;
                }

                TmpIndex++;
                TmpCount--;
            }
        }
        else if (!lstrcmpi(ServiceArgs[TmpIndex], _T("ri=")))
        {
            TmpIndex++;
            TmpCount--;

            if (TmpCount >= 0)
            {
                dwValue = _tcstoul(ServiceArgs[TmpIndex], NULL, 10);
                if (dwValue > 0)
                {
                    *pdwResumeIndex = dwValue;
                }

                TmpIndex++;
                TmpCount--;
            }
        }
        else if (!lstrcmpi(ServiceArgs[TmpIndex], _T("group=")))
        {
            TmpIndex++;
            TmpCount--;

            if (TmpCount > 0)
            {
                *ppszGroupName = ServiceArgs[TmpIndex];

                TmpIndex++;
                TmpCount--;
            }
        }
        else
        {
            *ppszServiceName = ServiceArgs[TmpIndex];

            TmpIndex++;
            TmpCount--;
        }
    }

    return TRUE;
}


BOOL
Query(LPCTSTR *ServiceArgs,
      DWORD ArgCount,
      BOOL bExtended)
{
    LPENUM_SERVICE_STATUS_PROCESS pServiceStatus = NULL;
    DWORD NumServices = 0;
    DWORD dwServiceType = SERVICE_WIN32;
    DWORD dwServiceState = SERVICE_ACTIVE;
    DWORD dwBufferSize = 0;
    DWORD dwResumeIndex = 0;
    LPCTSTR pszGroupName = NULL;
    LPCTSTR pszServiceName = NULL;
    DWORD i;

#ifdef SCDBG
    LPCTSTR *TmpArgs = ServiceArgs;
    INT TmpCnt = ArgCount;

    _tprintf(_T("Arguments:\n"));
    while (TmpCnt)
    {
        _tprintf(_T("  %s\n"), *TmpArgs);
        TmpArgs++;
        TmpCnt--;
    }
    _tprintf(_T("\n"));
#endif /* SCDBG */

    /* Parse arguments */
    if (!ParseQueryArguments(ServiceArgs,
                             ArgCount,
                             &dwServiceType,
                             &dwServiceState,
                             &dwBufferSize,
                             &dwResumeIndex,
                             &pszGroupName,
                             &pszServiceName))
        return FALSE;

#ifdef SCDBG
    _tprintf(_T("Service type: %lx\n"), dwServiceType);
    _tprintf(_T("Service state: %lx\n"), dwServiceState);
    _tprintf(_T("Buffer size: %lu\n"), dwBufferSize);
    _tprintf(_T("Resume index: %lu\n"), dwResumeIndex);
    _tprintf(_T("Group name: %s\n"), pszGroupName);
    _tprintf(_T("Service name: %s\n"), pszServiceName);
#endif

    if (pszServiceName)
    {
        /* Print only the requested service */

        LPSERVICE_STATUS_PROCESS pStatus;

        pStatus = QueryService(pszServiceName);
        if (pStatus)
        {
            PrintService(pszServiceName,
                         NULL,
                         pStatus,
                         bExtended);

            HeapFree(GetProcessHeap(), 0, pStatus);
        }
    }
    else
    {
        /* Print all matching services */

        NumServices = EnumServices(&pServiceStatus,
                                   dwServiceType,
                                   dwServiceState,
                                   dwBufferSize,
                                   dwResumeIndex,
                                   pszGroupName);
        if (NumServices == 0)
            return FALSE;

        for (i = 0; i < NumServices; i++)
        {
            PrintService(pServiceStatus[i].lpServiceName,
                         pServiceStatus[i].lpDisplayName,
                         &pServiceStatus[i].ServiceStatusProcess,
                         bExtended);
        }

#ifdef SCDBG
        _tprintf(_T("number : %lu\n"), NumServices);
#endif

        if (pServiceStatus)
            HeapFree(GetProcessHeap(), 0, pServiceStatus);
    }

    return TRUE;
}
