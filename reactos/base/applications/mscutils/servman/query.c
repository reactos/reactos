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
                              SERVICE_QUERY_CONFIG);
            if (hSc)
            {
                if (ChangeServiceConfig(hSc,
                                        pServiceConfig->dwServiceType,
                                        pServiceConfig->dwStartType,
                                        pServiceConfig->dwErrorControl,
                                        pServiceConfig->lpBinaryPathName,
                                        pServiceConfig->lpLoadOrderGroup,
                                        &pServiceConfig->dwTagId,
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



static BOOL
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


BOOL
RefreshServiceList(PMAIN_WND_INFO Info)
{
    ENUM_SERVICE_STATUS_PROCESS *pService;
    LPTSTR lpDescription;
    LVITEM lvItem;
    TCHAR szNumServices[32];
    TCHAR szStatus[64];
    DWORD NumServices;
    DWORD Index;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");

    (void)ListView_DeleteAllItems(Info->hListView);

    if (GetServiceList(Info, &NumServices))
    {
        TCHAR buf[300];     /* buffer to hold key path */
        INT NumListedServ = 0; /* how many services were listed */

        for (Index = 0; Index < NumServices; Index++)
        {
            HKEY hKey = NULL;
            LPTSTR lpLogOnAs = NULL;
            DWORD StartUp = 0;
            DWORD dwValueSize;

            /* copy the service info over */
            pService = &Info->pAllServices[Index];

             /* open the registry key for the service */
            _sntprintf(buf,
                       300,
                       Path,
                       pService->lpServiceName);
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             buf,
                             0,
                             KEY_READ,
                             &hKey) != ERROR_SUCCESS)
            {
                HeapFree(ProcessHeap,
                         0,
                         pService);
                continue;
            }

            /* set the display name */
            ZeroMemory(&lvItem, sizeof(LVITEM));
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.pszText = pService->lpDisplayName;

            /* Add the service pointer */
            lvItem.lParam = (LPARAM)pService;

            /* add it to the listview */
            lvItem.iItem = ListView_InsertItem(Info->hListView, &lvItem);

            /* set the description */
            if ((lpDescription = GetServiceDescription(pService->lpServiceName)))
            {
                lvItem.pszText = lpDescription;
                lvItem.iSubItem = 1;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);

                HeapFree(ProcessHeap,
                         0,
                         lpDescription);
            }

            /* set the status */
            if (pService->ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
            {
                LoadString(hInstance,
                           IDS_SERVICES_STARTED,
                           szStatus,
                           sizeof(szStatus) / sizeof(TCHAR));
                lvItem.pszText = szStatus;
                lvItem.iSubItem = 2;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);
            }

            /* set the startup type */
            dwValueSize = sizeof(DWORD);
            if (RegQueryValueEx(hKey,
                                _T("Start"),
                                NULL,
                                NULL,
                                (LPBYTE)&StartUp,
                                &dwValueSize) == ERROR_SUCCESS)
            {
                switch (StartUp)
                {
                    case 2:
                        LoadStringW(hInstance,
                                    IDS_SERVICES_AUTO,
                                    szStatus,
                                    sizeof(szStatus) / sizeof(TCHAR));
                         break;
                    case 3:
                        LoadStringW(hInstance,
                                    IDS_SERVICES_MAN,
                                    szStatus,
                                    sizeof(szStatus) / sizeof(TCHAR));
                        break;

                    case 4:
                        LoadStringW(hInstance,
                                    IDS_SERVICES_DIS,
                                    szStatus,
                                    sizeof(szStatus) / sizeof(TCHAR));
                        break;
                    default:
                        szStatus[0] = 0;
                        break;
                }

                lvItem.pszText = szStatus;
                lvItem.iSubItem = 3;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);
            }

            /* set Log On As */
            dwValueSize = 0;
            if (RegQueryValueEx(hKey,
                                _T("ObjectName"),
                                NULL,
                                NULL,
                                NULL,
                                &dwValueSize) == ERROR_SUCCESS)
            {
                lpLogOnAs = HeapAlloc(ProcessHeap,
                                     0,
                                     dwValueSize);
                if (lpLogOnAs != NULL)
                {
                    if(RegQueryValueEx(hKey,
                                       _T("ObjectName"),
                                       NULL,
                                       NULL,
                                       (LPBYTE)lpLogOnAs,
                                       &dwValueSize) == ERROR_SUCCESS)
                    {
                        lvItem.pszText = lpLogOnAs;
                        lvItem.iSubItem = 4;
                        SendMessage(Info->hListView,
                                    LVM_SETITEMTEXT,
                                    lvItem.iItem,
                                    (LPARAM)&lvItem);
                    }

                    HeapFree(ProcessHeap,
                             0,
                             lpLogOnAs);
                }

                RegCloseKey(hKey);
            }
        }

        /* set the number of listed services in the status bar */
        NumListedServ = ListView_GetItemCount(Info->hListView);
        LoadString(hInstance,
                   IDS_NUM_SERVICES,
                   szNumServices,
                   sizeof(szNumServices) / sizeof(TCHAR));

        _sntprintf(buf,
                   300,
                   szNumServices,
                   NumListedServ);

        SendMessage(Info->hStatus,
                    SB_SETTEXT,
                    0,
                    (LPARAM)buf);
    }

    /* turn redraw flag on. It's turned off initially via the LBS_NOREDRAW flag */
    SendMessage (Info->hListView,
                 WM_SETREDRAW,
                 TRUE,
                 0);

    return TRUE;
}
