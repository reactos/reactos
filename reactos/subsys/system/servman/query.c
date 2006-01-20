/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        subsys/system/servman/query.c
 * PURPOSE:     Query service information
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

extern HINSTANCE hInstance;
extern HWND hListView;
extern HWND hStatus;

/* Stores the complete services array */
ENUM_SERVICE_STATUS_PROCESS *pServiceStatus = NULL;


/* Free service array */
VOID FreeMemory(VOID)
{
    HeapFree(GetProcessHeap(), 0, pServiceStatus);
}


/* Retrives the service description from the registry */
BOOL GetDescription(HKEY hKey, LPTSTR *retDescription)
{

    LPTSTR Description = NULL;
    DWORD dwValueSize = 0;
    LONG ret = RegQueryValueEx(hKey,
                               _T("Description"),
                               NULL,
                               NULL,
                               NULL,
                               &dwValueSize);
    if (ret != ERROR_SUCCESS && ret != ERROR_FILE_NOT_FOUND && ret != ERROR_INVALID_HANDLE)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    if (ret != ERROR_FILE_NOT_FOUND)
    {
        Description = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwValueSize);
        if (Description == NULL)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        if(RegQueryValueEx(hKey,
                           _T("Description"),
                           NULL,
                           NULL,
                           (LPBYTE)Description,
                           &dwValueSize))
        {
            HeapFree(GetProcessHeap(), 0, Description);
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    /* copy pointer over */
    *retDescription = Description;

    return TRUE;
}


/* get vendor of service binary */
BOOL GetExecutablePath(LPTSTR *ExePath)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hSc = NULL;
    LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    LVITEM item;
    DWORD BytesNeeded = 0;
    TCHAR FileName[MAX_PATH];


    item.mask = LVIF_PARAM;
    item.iItem = GetSelectedItem();
    SendMessage(hListView, LVM_GETITEM, 0, (LPARAM)&item);

    /* copy pointer to selected service */
    Service = (ENUM_SERVICE_STATUS_PROCESS *)item.lParam;

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        GetError(0);
        return FALSE;
    }

    /* get a handle to the service requested for starting */
    hSc = OpenService(hSCManager, Service->lpServiceName, SERVICE_QUERY_CONFIG);
    if (hSc == NULL)
    {
        GetError(0);
        goto cleanup;
    }


    if (!QueryServiceConfig(hSc, pServiceConfig, 0, &BytesNeeded))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pServiceConfig = (LPQUERY_SERVICE_CONFIG)
                HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
            if (pServiceConfig == NULL)
                goto cleanup;

            if (!QueryServiceConfig(hSc,
                                    pServiceConfig,
                                    BytesNeeded,
                                    &BytesNeeded))
            {
                HeapFree(GetProcessHeap(), 0, pServiceConfig);
                goto cleanup;
            }
        }
        else /* exit on failure */
        {
            goto cleanup;
        }
    }

    ZeroMemory(&FileName, MAX_PATH);
    if (_tcscspn(pServiceConfig->lpBinaryPathName, _T("\"")))
    {
        _tcsncpy(FileName, pServiceConfig->lpBinaryPathName,
            _tcscspn(pServiceConfig->lpBinaryPathName, _T(" ")) );
    }
    else
    {
        _tcscpy(FileName, pServiceConfig->lpBinaryPathName);
    }

    *ExePath = FileName;

    CloseServiceHandle(hSCManager);
    CloseServiceHandle(hSc);

    return TRUE;

cleanup:
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    if (hSc != NULL)
        CloseServiceHandle(hSc);
    return FALSE;
}




BOOL
RefreshServiceList(VOID)
{
    LVITEM item;
    TCHAR szNumServices[32];
    TCHAR szStatus[64];
    DWORD NumServices = 0;
    DWORD Index;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");

    ListView_DeleteAllItems(hListView);

    NumServices = GetServiceList();

    if (NumServices)
    {
        HICON hiconItem;    /* icon for list-view items */
        HIMAGELIST hSmall;  /* image list for other views */
        TCHAR buf[300];     /* buffer to hold key path */
        INT NumListedServ = 0; /* how many services were listed */

        /* Create the icon image lists */
        hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), ILC_MASK | ILC_COLOR16, 1, 1);

        /* Add an icon to each image list */
        hiconItem = LoadImage(hInstance, MAKEINTRESOURCE(IDI_SM_ICON),
                              IMAGE_ICON, 16, 16, 0);
        ImageList_AddIcon(hSmall, hiconItem);

        /* assign the image to the list view */
        ListView_SetImageList(hListView, hSmall, LVSIL_SMALL);

        for (Index = 0; Index < NumServices; Index++)
        {
            HKEY hKey = NULL;
            LPTSTR Description = NULL;
            LPTSTR LogOnAs = NULL;
            DWORD StartUp = 0;
            DWORD dwValueSize;

             /* open the registry key for the service */
            _sntprintf(buf, 300, Path,
                      pServiceStatus[Index].lpServiceName);

            RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         buf,
                         0,
                         KEY_READ,
                         &hKey);


            /* set the display name */

            ZeroMemory(&item, sizeof(LVITEM));
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.pszText = pServiceStatus[Index].lpDisplayName;

            /* Set a pointer for each service so we can query it later.
             * Not all services are added to the list, so we can't query
             * the item number as they become out of sync with the array */
            item.lParam = (LPARAM)&pServiceStatus[Index];

            item.iItem = ListView_GetItemCount(hListView);
            item.iItem = ListView_InsertItem(hListView, &item);




            /* set the description */

            if (GetDescription(hKey, &Description))
            {
                item.pszText = Description;
                item.iSubItem = 1;
                SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);

                HeapFree(GetProcessHeap(), 0, Description);
            }


            /* set the status */

            if (pServiceStatus[Index].ServiceStatusProcess.dwCurrentState
                    == SERVICE_RUNNING)
            {
                LoadString(hInstance, IDS_SERVICES_STARTED, szStatus,
                    sizeof(szStatus) / sizeof(TCHAR));
                item.pszText = szStatus;
                item.iSubItem = 2;
                SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
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
                LoadString(hInstance, IDS_SERVICES_AUTO, szStatus,
                    sizeof(szStatus) / sizeof(TCHAR));
                item.pszText = szStatus;
                item.iSubItem = 3;
                SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
            }
            else if (StartUp == 0x03)
            {
                LoadString(hInstance, IDS_SERVICES_MAN, szStatus,
                    sizeof(szStatus) / sizeof(TCHAR));
                item.pszText = szStatus;
                item.iSubItem = 3;
                SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
            }
            else if (StartUp == 0x04)
            {
                LoadString(hInstance, IDS_SERVICES_DIS, szStatus,
                    sizeof(szStatus) / sizeof(TCHAR));
                item.pszText = szStatus;
                item.iSubItem = 3;
                SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
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

            LogOnAs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwValueSize);
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
                HeapFree(GetProcessHeap(), 0, LogOnAs);
                RegCloseKey(hKey);
                continue;
            }

            item.pszText = LogOnAs;
            item.iSubItem = 4;
            SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);

            HeapFree(GetProcessHeap(), 0, LogOnAs);

            RegCloseKey(hKey);

        }

        NumListedServ = ListView_GetItemCount(hListView);

        /* set the number of listed services in the status bar */
        LoadString(hInstance, IDS_NUM_SERVICES, szNumServices,
            sizeof(szNumServices) / sizeof(TCHAR));
        _sntprintf(buf, 300, szNumServices, NumListedServ);
        SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)buf);
    }

    /* turn redraw flag on. It's turned off initially via the LBS_NOREDRAW flag */
    SendMessage (hListView, WM_SETREDRAW, TRUE, 0) ;

    return TRUE;
}




DWORD
GetServiceList(VOID)
{
    SC_HANDLE ScHandle;

    DWORD BytesNeeded = 0;
    DWORD ResumeHandle = 0;
    DWORD NumServices = 0;

    ScHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (ScHandle != INVALID_HANDLE_VALUE)
    {
        if (EnumServicesStatusEx(ScHandle,
                                 SC_ENUM_PROCESS_INFO,
                                 SERVICE_WIN32,
                                 SERVICE_STATE_ALL,
                                 (LPBYTE)pServiceStatus,
                                 0, &BytesNeeded,
                                 &NumServices,
                                 &ResumeHandle,
                                 0) == 0)
        {
            /* Call function again if required size was returned */
            if (GetLastError() == ERROR_MORE_DATA)
            {
                /* reserve memory for service info array */
                pServiceStatus = (ENUM_SERVICE_STATUS_PROCESS *)
                        HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                if (pServiceStatus == NULL)
			        return FALSE;

                /* fill array with service info */
                if (EnumServicesStatusEx(ScHandle,
                                         SC_ENUM_PROCESS_INFO,
                                         SERVICE_WIN32,
                                         SERVICE_STATE_ALL,
                                         (LPBYTE)pServiceStatus,
                                         BytesNeeded,
                                         &BytesNeeded,
                                         &NumServices,
                                         &ResumeHandle,
                                         0) == 0)
                {
                    HeapFree(GetProcessHeap(), 0, pServiceStatus);
                    return FALSE;
                }
            }
            else /* exit on failure */
            {
                return FALSE;
            }
        }
    }

    CloseServiceHandle(ScHandle);

    return NumServices;
}











/*
    //WORD wCodePage;
    //WORD wLangID;
    //SC_HANDLE hService;
    //DWORD dwHandle, dwLen;
    //UINT BufLen;
    //TCHAR* lpData;
    //TCHAR* lpBuffer;
    //TCHAR szStrFileInfo[80];
    //TCHAR FileName[MAX_PATH];
    //LPVOID pvData;

    //LPSERVICE_FAILURE_ACTIONS pServiceFailureActions = NULL;
    //LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;

               BytesNeeded = 0;
                hService = OpenService(ScHandle,
                                       pServiceStatus[Index].lpServiceName,
                                       SC_MANAGER_CONNECT);
                if (hService != INVALID_HANDLE_VALUE)
                {
                    / * check if service is required by the system* /
                    if (!QueryServiceConfig2(hService,
                                             SERVICE_CONFIG_FAILURE_ACTIONS,
                                             (LPBYTE)pServiceFailureActions,
                                             0,
                                             &BytesNeeded))
                    {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            pServiceFailureActions = (LPSERVICE_FAILURE_ACTIONS)
                                HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                            if (pServiceFailureActions == NULL)
			                    return FALSE;

                            if (!QueryServiceConfig2(hService,
                                                     SERVICE_CONFIG_FAILURE_ACTIONS,
                                                     (LPBYTE)pServiceFailureActions,
                                                     BytesNeeded,
                                                     &BytesNeeded))
                            {
                                HeapFree(GetProcessHeap(), 0, pServiceFailureActions);
                                return FALSE;
                            }
                        }
                        else / * exit on failure * /
                        {
                            return FALSE;
                        }
                    }
                    if (pServiceFailureActions->cActions)
                    {
                        if (pServiceFailureActions->lpsaActions[0].Type == SC_ACTION_REBOOT)
                        {
                            LoadString(hInstance, IDS_SERVICES_YES, szStatus, 128);
                            item.pszText = szStatus;
                            item.iSubItem = 1;
                            SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                        }
                    }

					if (pServiceFailureActions != NULL)
					{
						HeapFree(GetProcessHeap(), 0, pServiceFailureActions);
						pServiceFailureActions = NULL;
					}

                    / * get vendor of service binary * /
                    BytesNeeded = 0;
                    if (!QueryServiceConfig(hService, pServiceConfig, 0, &BytesNeeded))
                    {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            pServiceConfig = (LPQUERY_SERVICE_CONFIG)
                                HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                            if (pServiceConfig == NULL)
			                    return FALSE;

                            if (!QueryServiceConfig(hService,
                                                    pServiceConfig,
                                                    BytesNeeded,
                                                    &BytesNeeded))
                            {
                                HeapFree(GetProcessHeap(), 0, pServiceConfig);
                                return FALSE;
                            }
                        }
                        else / * exit on failure * /
                        {
                            return FALSE;
                        }
                    }

                    memset(&FileName, 0, MAX_PATH);
                    if (_tcscspn(pServiceConfig->lpBinaryPathName, _T("\"")))
                    {
                        _tcsncpy(FileName, pServiceConfig->lpBinaryPathName,
                            _tcscspn(pServiceConfig->lpBinaryPathName, _T(" ")) );
                    }
                    else
                    {
                        _tcscpy(FileName, pServiceConfig->lpBinaryPathName);
                    }

					HeapFree(GetProcessHeap(), 0, pServiceConfig);
					pServiceConfig = NULL;

					dwLen = GetFileVersionInfoSize(FileName, &dwHandle);
                    if (dwLen)
                    {
                        lpData = (TCHAR*) HeapAlloc(GetProcessHeap(), 0, dwLen);
                        if (lpData == NULL)
			                return FALSE;

                        if (!GetFileVersionInfo (FileName, dwHandle, dwLen, lpData)) {
		                    HeapFree(GetProcessHeap(), 0, lpData);
		                    return FALSE;
	                    }

                        if (VerQueryValue(lpData, _T("\\VarFileInfo\\Translation"), &pvData, (PUINT) &BufLen))
                        {
                            wCodePage = LOWORD(*(DWORD*) pvData);
                            wLangID = HIWORD(*(DWORD*) pvData);
                            wsprintf(szStrFileInfo, _T("StringFileInfo\\%04X%04X\\CompanyName"), wCodePage, wLangID);
                        }

                        if (VerQueryValue (lpData, szStrFileInfo, (LPVOID) &lpBuffer, (PUINT) &BufLen)) {
                            item.pszText = lpBuffer;
                            item.iSubItem = 2;
                            SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                        }
						HeapFree(GetProcessHeap(), 0, lpData);
                    }
                    else
                    {
                        LoadString(hInstance, IDS_SERVICES_UNKNOWN, szStatus, 128);
                        item.pszText = szStatus;
                        item.iSubItem = 2;
                        SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                    }
                    CloseServiceHandle(hService);
                }







    HeapFree(GetProcessHeap(), 0, pServiceConfig);
    pServiceConfig = NULL;

    dwLen = GetFileVersionInfoSize(FileName, &dwHandle);
    if (dwLen)
    {
        lpData = (TCHAR*) HeapAlloc(GetProcessHeap(), 0, dwLen);
        if (lpData == NULL)
            return FALSE;

        if (!GetFileVersionInfo (FileName, dwHandle, dwLen, lpData)) {
            HeapFree(GetProcessHeap(), 0, lpData);
            return FALSE;
        }

        if (VerQueryValue(lpData, _T("\\VarFileInfo\\Translation"),
                          &pvData, (PUINT) &BufLen))
        {
            wCodePage = LOWORD(*(DWORD*) pvData);
            wLangID = HIWORD(*(DWORD*) pvData);
            wsprintf(szStrFileInfo, _T("StringFileInfo\\%04X%04X\\CompanyName"),
                     wCodePage, wLangID);
        }

        if (VerQueryValue (lpData, szStrFileInfo, (LPVOID) &lpBuffer, (PUINT) &BufLen))
        {
            item.pszText = lpBuffer;
            item.iSubItem = 2;
            SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
        }
        HeapFree(GetProcessHeap(), 0, lpData);
    }
    else
    {
        LoadString(hInstance, IDS_SERVICES_UNKNOWN, szStatus, 128);
        item.pszText = szStatus;
        item.iSubItem = 2;
        SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
    }


*/

