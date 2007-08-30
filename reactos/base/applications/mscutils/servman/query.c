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
GetServiceConfig(LPTSTR lpServiceName)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSc = NULL;
    LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;
    DWORD BytesNeeded = 0;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        GetError();
        return NULL;
    }

    hSc = OpenService(hSCManager,
                      lpServiceName,
                      SERVICE_QUERY_CONFIG);
    if (hSc == NULL)
    {
        GetError();
        goto cleanup;
    }

    if (!QueryServiceConfig(hSc,
                            pServiceConfig,
                            0,
                            &BytesNeeded))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pServiceConfig = (LPQUERY_SERVICE_CONFIG) HeapAlloc(ProcessHeap,
                                                                0,
                                                                BytesNeeded);
            if (pServiceConfig == NULL)
                goto cleanup;

            if (!QueryServiceConfig(hSc,
                                    pServiceConfig,
                                    BytesNeeded,
                                    &BytesNeeded))
            {
                HeapFree(ProcessHeap,
                         0,
                         pServiceConfig);

                pServiceConfig = NULL;
            }
        }
    }

cleanup:
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    if (hSc != NULL)
        CloseServiceHandle(hSc);

    return pServiceConfig;
}


BOOL
SetServiceConfig(LPQUERY_SERVICE_CONFIG pServiceConfig,
                 LPTSTR lpServiceName,
                 LPTSTR lpPassword)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SC_LOCK scLock;
    BOOL bRet = FALSE;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_LOCK);
    if (hSCManager)
    {
        scLock = LockServiceDatabase(hSCManager);
        if (scLock)
        {
            hSc = OpenService(hSCManager,
                              lpServiceName,
                              SERVICE_CHANGE_CONFIG);
            if (hSc)
            {
                if (ChangeServiceConfig(hSc,
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
            }

            UnlockServiceDatabase(scLock);
        }
    }

    if (!bRet)
        GetError();

    return bRet;
}


LPTSTR
GetServiceDescription(LPTSTR lpServiceName)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSc = NULL;
    SERVICE_DESCRIPTION *pServiceDescription = NULL;
    LPTSTR lpDescription = NULL;
    DWORD BytesNeeded = 0;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        GetError();
        return NULL;
    }

    hSc = OpenService(hSCManager,
                      lpServiceName,
                      SERVICE_QUERY_CONFIG);
    if (hSc)
    {
        if (!QueryServiceConfig2(hSc,
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

                if (QueryServiceConfig2(hSc,
                                        SERVICE_CONFIG_DESCRIPTION,
                                        (LPBYTE)pServiceDescription,
                                        BytesNeeded,
                                        &BytesNeeded))
                {
                    if (pServiceDescription->lpDescription)
                    {
                        lpDescription = HeapAlloc(ProcessHeap,
                                                  0,
                                                  (_tcslen(pServiceDescription->lpDescription) + 1) * sizeof(TCHAR));
                        if (lpDescription)
                            _tcscpy(lpDescription,
                                    pServiceDescription->lpDescription);
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
SetServiceDescription(LPTSTR lpServiceName,
                      LPTSTR lpDescription)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SC_LOCK scLock;
    SERVICE_DESCRIPTION ServiceDescription;
    BOOL bRet = FALSE;

    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_LOCK);
    if (hSCManager)
    {
        scLock = LockServiceDatabase(hSCManager);
        if (scLock)
        {
            hSc = OpenService(hSCManager,
                              lpServiceName,
                              SERVICE_CHANGE_CONFIG);
            if (hSc)
            {
                ServiceDescription.lpDescription = lpDescription;

                if (ChangeServiceConfig2(hSc,
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

    ScHandle = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_ENUMERATE_SERVICE);
    if (ScHandle != INVALID_HANDLE_VALUE)
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

    if (!bRet)
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

    hScm = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_ENUMERATE_SERVICE);
    if (hScm != INVALID_HANDLE_VALUE)
    {
        SC_HANDLE hService;

        hService = OpenService(hScm,
                               pService->lpServiceName,
                               SERVICE_QUERY_STATUS);
        if (hService)
        {
            DWORD size;

            QueryServiceStatusEx(hService,
                                 SC_STATUS_PROCESS_INFO,
                                 (LPBYTE)&pService->ServiceStatusProcess,
                                 sizeof(*pService),
                                 &size);

            bRet = TRUE;
        }
    }

    return bRet;
}
