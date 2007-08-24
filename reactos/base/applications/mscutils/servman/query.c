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



/* get vendor of service binary */
LPTSTR
GetExecutablePath(PMAIN_WND_INFO Info)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSc = NULL;
    LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;
    DWORD BytesNeeded = 0;
    LPTSTR lpExePath = NULL;

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        GetError();
        return FALSE;
    }

    /* get a handle to the service requested for starting */
    hSc = OpenService(hSCManager,
                      Info->CurrentService->lpServiceName,
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

            if (QueryServiceConfig(hSc,
                                   pServiceConfig,
                                   BytesNeeded,
                                   &BytesNeeded))
            {
                lpExePath = HeapAlloc(ProcessHeap,
                                      0,
                                      (_tcslen(pServiceConfig->lpBinaryPathName) +1 ) * sizeof(TCHAR));

                _tcscpy(lpExePath, pServiceConfig->lpBinaryPathName);
            }
        }
    }

cleanup:
    if (pServiceConfig)
        HeapFree(ProcessHeap,
                 0,
                 pServiceConfig);
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    if (hSc != NULL)
        CloseServiceHandle(hSc);

    return lpExePath;
}


BOOL
RefreshServiceList(PMAIN_WND_INFO Info)
{
    LVITEM lvItem;
    TCHAR szNumServices[32];
    TCHAR szStatus[64];
    DWORD NumServices = 0;
    DWORD Index;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");

    (void)ListView_DeleteAllItems(Info->hListView);

    NumServices = GetServiceList(Info);

    if (NumServices)
    {
        TCHAR buf[300];     /* buffer to hold key path */
        INT NumListedServ = 0; /* how many services were listed */

        for (Index = 0; Index < NumServices; Index++)
        {
            HKEY hKey = NULL;
            LPTSTR lpDescription = NULL;
            LPTSTR LogOnAs = NULL;
            DWORD StartUp = 0;
            DWORD dwValueSize;

             /* open the registry key for the service */
            _sntprintf(buf,
                       300,
                       Path,
                       Info->pServiceStatus[Index].lpServiceName);

            RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         buf,
                         0,
                         KEY_READ,
                         &hKey);


            /* set the display name */
            ZeroMemory(&lvItem, sizeof(LVITEM));
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.pszText = Info->pServiceStatus[Index].lpDisplayName;

            /* Set a pointer for each service so we can query it later.
             * Not all services are added to the list, so we can't query
             * the item number as they become out of sync with the array */
            lvItem.lParam = (LPARAM)&Info->pServiceStatus[Index];

            lvItem.iItem = ListView_GetItemCount(Info->hListView);
            lvItem.iItem = ListView_InsertItem(Info->hListView, &lvItem);

            /* set the description */
            if ((lpDescription = GetDescription(Info->pServiceStatus[Index].lpServiceName)))
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
            if (Info->pServiceStatus[Index].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
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
                                &dwValueSize))
            {
                RegCloseKey(hKey);
                continue;
            }

            if (StartUp == 0x02)
            {
                LoadString(hInstance,
                           IDS_SERVICES_AUTO,
                           szStatus,
                           sizeof(szStatus) / sizeof(TCHAR));
                lvItem.pszText = szStatus;
                lvItem.iSubItem = 3;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);
            }
            else if (StartUp == 0x03)
            {
                LoadString(hInstance,
                           IDS_SERVICES_MAN,
                           szStatus,
                           sizeof(szStatus) / sizeof(TCHAR));
                lvItem.pszText = szStatus;
                lvItem.iSubItem = 3;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);
            }
            else if (StartUp == 0x04)
            {
                LoadString(hInstance,
                           IDS_SERVICES_DIS,
                           szStatus,
                           sizeof(szStatus) / sizeof(TCHAR));
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
                                &dwValueSize))
            {
                RegCloseKey(hKey);
                continue;
            }

            LogOnAs = HeapAlloc(ProcessHeap,
                                HEAP_ZERO_MEMORY,
                                dwValueSize);
            if (LogOnAs == NULL)
            {
                RegCloseKey(hKey);
                return FALSE;
            }
            if(RegQueryValueEx(hKey,
                               _T("ObjectName"),
                               NULL,
                               NULL,
                               (LPBYTE)LogOnAs,
                               &dwValueSize))
            {
                HeapFree(ProcessHeap,
                         0,
                         LogOnAs);
                RegCloseKey(hKey);
                continue;
            }

            lvItem.pszText = LogOnAs;
            lvItem.iSubItem = 4;
            SendMessage(Info->hListView,
                        LVM_SETITEMTEXT,
                        lvItem.iItem,
                        (LPARAM)&lvItem);

            HeapFree(ProcessHeap,
                     0,
                     LogOnAs);

            RegCloseKey(hKey);

        }

        NumListedServ = ListView_GetItemCount(Info->hListView);

        /* set the number of listed services in the status bar */
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
                 0) ;

    return TRUE;
}




DWORD
GetServiceList(PMAIN_WND_INFO Info)
{
    SC_HANDLE ScHandle;
    BOOL bGotServices = FALSE;

    DWORD BytesNeeded = 0;
    DWORD ResumeHandle = 0;
    DWORD NumServices = 0;

    ScHandle = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_ENUMERATE_SERVICE);
    if (ScHandle != INVALID_HANDLE_VALUE)
    {
        if (!EnumServicesStatusEx(ScHandle,
                                  SC_ENUM_PROCESS_INFO,
                                  SERVICE_WIN32,
                                  SERVICE_STATE_ALL,
                                  (LPBYTE)Info->pServiceStatus,
                                  0,
                                  &BytesNeeded,
                                  &NumServices,
                                  &ResumeHandle,
                                  0))
        {
            /* Call function again if required size was returned */
            if (GetLastError() == ERROR_MORE_DATA)
            {
                /* reserve memory for service info array */
                Info->pServiceStatus = (ENUM_SERVICE_STATUS_PROCESS *)
                                          HeapAlloc(ProcessHeap,
                                                    0,
                                                    BytesNeeded);
                if (Info->pServiceStatus == NULL)
                    return FALSE;

                /* fill array with service info */
                if (EnumServicesStatusEx(ScHandle,
                                         SC_ENUM_PROCESS_INFO,
                                         SERVICE_WIN32,
                                         SERVICE_STATE_ALL,
                                         (LPBYTE)Info->pServiceStatus,
                                         BytesNeeded,
                                         &BytesNeeded,
                                         &NumServices,
                                         &ResumeHandle,
                                         0))
                {
                    bGotServices = TRUE;
                }
            }
        }
    }

    if (ScHandle)
        CloseServiceHandle(ScHandle);

    if (!bGotServices)
    {
        HeapFree(ProcessHeap,
                 0,
                 Info->pServiceStatus);
    }

    return NumServices;
}
