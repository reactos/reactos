/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/query.c
 * PURPOSE:     Query service information
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

ENUM_SERVICE_STATUS_PROCESS*
GetSelectedService(PMAIN_WND_INFO Info)
{
    LVITEM lvItem;

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = Info->SelectedItem;
    SendMessage(Info->hListView,
                LVM_GETITEM,
                0,
                (LPARAM)&lvItem);

    /* return pointer to selected service */
    return (ENUM_SERVICE_STATUS_PROCESS *)lvItem.lParam;
}

LPQUERY_SERVICE_CONFIG
GetServiceConfig(LPWSTR lpServiceName)
{
    LPQUERY_SERVICE_CONFIGW lpServiceConfig = NULL;
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    DWORD dwBytesNeeded;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS);
    if (hSCManager)
    {
        hService = OpenServiceW(hSCManager,
                                lpServiceName,
                                SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_QUERY_CONFIG);
        if (hService)
        {
            if (!QueryServiceConfigW(hService,
                                     NULL,
                                     0,
                                     &dwBytesNeeded))
            {
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {
                    lpServiceConfig = (LPQUERY_SERVICE_CONFIG)HeapAlloc(GetProcessHeap(),
                                                                        0,
                                                                        dwBytesNeeded);
                    if (lpServiceConfig)
                    {
                        if (!QueryServiceConfigW(hService,
                                                lpServiceConfig,
                                                dwBytesNeeded,
                                                &dwBytesNeeded))
                        {
                            HeapFree(GetProcessHeap(),
                                     0,
                                     lpServiceConfig);
                            lpServiceConfig = NULL;
                        }
                    }
                }
            }

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCManager);
    }

    return lpServiceConfig;
}

BOOL
SetServiceConfig(LPQUERY_SERVICE_CONFIG pServiceConfig,
                 LPWSTR lpServiceName,
                 LPWSTR lpPassword)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SC_LOCK scLock;
    BOOL bRet = FALSE;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_LOCK);
    if (hSCManager)
    {
        scLock = LockServiceDatabase(hSCManager);
        if (scLock)
        {
            hSc = OpenServiceW(hSCManager,
                               lpServiceName,
                               SERVICE_CHANGE_CONFIG);
            if (hSc)
            {
                if (ChangeServiceConfigW(hSc,
                                         pServiceConfig->dwServiceType,
                                         pServiceConfig->dwStartType,
                                         pServiceConfig->dwErrorControl,
                                         pServiceConfig->lpBinaryPathName,
                                         pServiceConfig->lpLoadOrderGroup,
                                         pServiceConfig->dwTagId ? &pServiceConfig->dwTagId : NULL,
                                         pServiceConfig->lpDependencies,
                                         pServiceConfig->lpServiceStartName,
                                         lpPassword,
                                         pServiceConfig->lpDisplayName))
                {
                    bRet = TRUE;
                }

                CloseServiceHandle(hSc);
            }

            UnlockServiceDatabase(scLock);
        }

        CloseServiceHandle(hSCManager);
    }

    if (!bRet)
        GetError();

    return bRet;
}

LPWSTR
GetServiceDescription(LPWSTR lpServiceName)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSc = NULL;
    SERVICE_DESCRIPTIONW *pServiceDescription = NULL;
    LPWSTR lpDescription = NULL;
    DWORD BytesNeeded = 0;
    DWORD dwSize;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        GetError();
        return NULL;
    }

    hSc = OpenServiceW(hSCManager,
                       lpServiceName,
                       SERVICE_QUERY_CONFIG);
    if (hSc)
    {
        if (!QueryServiceConfig2W(hSc,
                                  SERVICE_CONFIG_DESCRIPTION,
                                  NULL,
                                  0,
                                  &BytesNeeded))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                pServiceDescription = (SERVICE_DESCRIPTION *) HeapAlloc(ProcessHeap,
                                                                        0,
                                                                        BytesNeeded);
                if (pServiceDescription == NULL)
                    goto cleanup;

                if (QueryServiceConfig2W(hSc,
                                         SERVICE_CONFIG_DESCRIPTION,
                                         (LPBYTE)pServiceDescription,
                                         BytesNeeded,
                                         &BytesNeeded))
                {
                    if (pServiceDescription->lpDescription)
                    {
                        dwSize = wcslen(pServiceDescription->lpDescription) + 1;
                        lpDescription = HeapAlloc(ProcessHeap,
                                                  0,
                                                  dwSize * sizeof(WCHAR));
                        if (lpDescription)
                        {
                            StringCchCopyW(lpDescription,
                                           dwSize,
                                           pServiceDescription->lpDescription);
                        }
                    }
                }
            }
        }
    }

cleanup:
    if (pServiceDescription)
        HeapFree(ProcessHeap,
                 0,
                 pServiceDescription);
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    if (hSc != NULL)
        CloseServiceHandle(hSc);

    return lpDescription;
}

BOOL
SetServiceDescription(LPWSTR lpServiceName,
                      LPWSTR lpDescription)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SC_LOCK scLock;
    SERVICE_DESCRIPTION ServiceDescription;
    BOOL bRet = FALSE;

    hSCManager = OpenSCManagerW(NULL,
                                NULL,
                                SC_MANAGER_LOCK);
    if (hSCManager)
    {
        scLock = LockServiceDatabase(hSCManager);
        if (scLock)
        {
            hSc = OpenServiceW(hSCManager,
                               lpServiceName,
                               SERVICE_CHANGE_CONFIG);
            if (hSc)
            {
                ServiceDescription.lpDescription = lpDescription;

                if (ChangeServiceConfig2W(hSc,
                                          SERVICE_CONFIG_DESCRIPTION,
                                          &ServiceDescription))
                {
                    bRet = TRUE;
                }

                CloseServiceHandle(hSc);
            }

            UnlockServiceDatabase(scLock);
        }

        CloseServiceHandle(hSCManager);
    }

    if (!bRet)
        GetError();

    return bRet;
}

BOOL
GetServiceList(PMAIN_WND_INFO Info,
               DWORD *NumServices)
{
    SC_HANDLE ScHandle;
    BOOL bRet = FALSE;

    DWORD BytesNeeded = 0;
    DWORD ResumeHandle = 0;

    *NumServices = 0;

    if (Info->pAllServices != NULL)
    {
        HeapFree(ProcessHeap,
                     0,
                     Info->pAllServices);
        Info->pAllServices = NULL;
    }

    ScHandle = OpenSCManagerW(NULL,
                              NULL,
                              SC_MANAGER_ENUMERATE_SERVICE);
    if (ScHandle != NULL)
    {
        if (!EnumServicesStatusEx(ScHandle,
                                  SC_ENUM_PROCESS_INFO,
                                  SERVICE_WIN32,
                                  SERVICE_STATE_ALL,
                                  NULL,
                                  0,
                                  &BytesNeeded,
                                  NumServices,
                                  &ResumeHandle,
                                  0))
        {
            /* Call function again if required size was returned */
            if (GetLastError() == ERROR_MORE_DATA)
            {
                /* reserve memory for service info array */
                Info->pAllServices = (ENUM_SERVICE_STATUS_PROCESS *) HeapAlloc(ProcessHeap,
                                                                         0,
                                                                         BytesNeeded);
                if (Info->pAllServices)
                {
                    /* fill array with service info */
                    if (EnumServicesStatusEx(ScHandle,
                                             SC_ENUM_PROCESS_INFO,
                                             SERVICE_WIN32,
                                             SERVICE_STATE_ALL,
                                             (LPBYTE)Info->pAllServices,
                                             BytesNeeded,
                                             &BytesNeeded,
                                             NumServices,
                                             &ResumeHandle,
                                             0))
                    {
                        bRet = TRUE;
                    }
                }
            }
        }
    }

    if (ScHandle)
        CloseServiceHandle(ScHandle);

    if (!bRet && Info->pAllServices)
    {
        HeapFree(ProcessHeap,
                 0,
                 Info->pAllServices);
    }

    return bRet;
}

BOOL
UpdateServiceStatus(ENUM_SERVICE_STATUS_PROCESS* pService)
{
    SC_HANDLE hScm;
    BOOL bRet = FALSE;

    hScm = OpenSCManagerW(NULL,
                          NULL,
                          SC_MANAGER_ENUMERATE_SERVICE);
    if (hScm != NULL)
    {
        SC_HANDLE hService;

        hService = OpenServiceW(hScm,
                                pService->lpServiceName,
                                SERVICE_QUERY_STATUS);
        if (hService)
        {
            DWORD size;

            QueryServiceStatusEx(hService,
                                 SC_STATUS_PROCESS_INFO,
                                 (LPBYTE)&pService->ServiceStatusProcess,
                                 sizeof(SERVICE_STATUS_PROCESS),
                                 &size);

            CloseServiceHandle(hService);
            bRet = TRUE;
        }

        CloseServiceHandle(hScm);
    }

    return bRet;
}
