#include "servman.h"

extern HINSTANCE hInstance;
extern HWND hListView;
extern HWND hStatus;

/* Stores the service array */
ENUM_SERVICE_STATUS_PROCESS *pServiceStatus = NULL;


/* free service array */
VOID FreeMemory(VOID)
{
    HeapFree(GetProcessHeap(), 0, pServiceStatus);
}


BOOL
RefreshServiceList(VOID)
{
    LV_ITEM item;
    TCHAR szNumServices[32];
    TCHAR szStatus[128];
    DWORD NumServices = 0;
    DWORD Index;

    NumServices = GetServiceList();

    if (NumServices)
    {
        HICON hiconItem;     /* icon for list-view items */
        HIMAGELIST hSmall;   /* image list for other views */
        TCHAR buf[40];

        /* Create the icon image lists */
        hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON), ILC_MASK, 1, 1);

        /* Add an icon to each image list */
        hiconItem = LoadImage(hInstance, MAKEINTRESOURCE(IDI_SM_ICON), IMAGE_ICON, 16, 16, 0);
        ImageList_AddIcon(hSmall, hiconItem);

        ListView_SetImageList(hListView, hSmall, LVSIL_SMALL);

        /* set the number of services in the status bar */
        LoadString(hInstance, IDS_SERVICES_NUM_SERVICES, szNumServices, 32);
        _stprintf(buf, szNumServices, NumServices);
        SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)buf);

        for (Index = 0; Index < NumServices; Index++)
        {
            HKEY hKey = NULL;
            TCHAR Description[5000];
            DWORD Size = 5000;

            /* set the display name */

            ZeroMemory(&item, sizeof(LV_ITEM));
            item.mask = LVIF_TEXT;
            //item.iImage = 0;
            item.pszText = pServiceStatus[Index].lpDisplayName;
            item.iItem = ListView_GetItemCount(hListView);
            item.lParam = 0;
            item.iItem = ListView_InsertItem(hListView, &item);


            /* set the description */

            _stprintf(buf, _T("System\\CurrentControlSet\\Services\\%s"),
                      pServiceStatus[Index].lpServiceName);

            if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             buf,
                             0,
                             KEY_READ,
                             &hKey) != ERROR_SUCCESS)
            {
                GetError();
                return FALSE;
            }

            RegQueryValueEx(hKey,
                            _T("Description"),
                            NULL,
                            NULL,
                            (LPBYTE)Description,
                            &Size);

            item.pszText = Description;
            item.iSubItem = 1;
            SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);


            /* set the status */

            if (pServiceStatus[Index].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
            {
                LoadString(hInstance, IDS_SERVICES_STATUS_RUNNING, szStatus, 128);
                item.pszText = szStatus;
                item.iSubItem = 2;
                SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
            }
            else
            {
                item.pszText = '\0';
                item.iSubItem = 2;
                SendMessage(hListView, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
            }
         }
    }

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
                pServiceStatus = (ENUM_SERVICE_STATUS_PROCESS *) HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
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
*/
