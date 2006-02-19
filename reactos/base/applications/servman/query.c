/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/query.c
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



ENUM_SERVICE_STATUS_PROCESS*
GetSelectedService(VOID)
{
    ENUM_SERVICE_STATUS_PROCESS *pSelectedService = NULL;
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = GetSelectedItem();
    SendMessage(hListView, LVM_GETITEM, 0, (LPARAM)&item);

    /* copy pointer to selected service */
    pSelectedService = (ENUM_SERVICE_STATUS_PROCESS *)item.lParam;

    return pSelectedService;
}


/* Sets the service description in the registry */
BOOL SetDescription(LPTSTR ServiceName, LPTSTR Description)
{
    HKEY hKey;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");
    TCHAR buf[300];
    TCHAR szBuf[MAX_PATH];
    LONG val;


   /* open the registry key for the service */
    _sntprintf(buf, sizeof(buf) / sizeof(TCHAR), Path, ServiceName);
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 buf,
                 0,
                 KEY_WRITE,
                 &hKey);


   if ((val = RegSetValueEx(hKey,
                     _T("Description"),
                     0,
                     REG_SZ,
                     (LPBYTE)Description,
                     (DWORD)lstrlen(szBuf)+1)) != ERROR_SUCCESS)
   {
       GetError(val);
       return FALSE;
   }


    RegCloseKey(hKey);
    return TRUE;
}



/* Retrives the service description from the registry */
BOOL GetDescription(LPTSTR lpServiceName, LPTSTR *retDescription)
{
    HKEY hKey;
    LPTSTR Description = NULL;
    DWORD dwValueSize = 0;
    LONG ret;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");
    TCHAR buf[300];

    /* open the registry key for the service */
    _sntprintf(buf, sizeof(buf) / sizeof(TCHAR), Path, lpServiceName);
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 buf,
                 0,
                 KEY_READ,
                 &hKey);

    ret = RegQueryValueEx(hKey,
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
    DWORD BytesNeeded = 0;

    /* copy pointer to selected service */
    Service = GetSelectedService();

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

    *ExePath = pServiceConfig->lpBinaryPathName;

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


VOID InitListViewImage(VOID)
{
    HICON hSmIconItem, hLgIconItem;    /* icon for list-view items */
    HIMAGELIST hSmall, hLarge;  /* image list for other views */


    /* Create the icon image lists */
    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
    GetSystemMetrics(SM_CYSMICON), ILC_MASK | ILC_COLOR32, 1, 1);

    hLarge = ImageList_Create(GetSystemMetrics(SM_CXICON),
    GetSystemMetrics(SM_CYICON), ILC_MASK | ILC_COLOR32, 1, 1);

    /* Add an icon to each image list */
    hSmIconItem = LoadImage(hInstance, MAKEINTRESOURCE(IDI_SM_ICON),
                          IMAGE_ICON, 16, 16, 0);
    ImageList_AddIcon(hSmall, hSmIconItem);

    hLgIconItem = LoadImage(hInstance, MAKEINTRESOURCE(IDI_SM_ICON),
                          IMAGE_ICON, 32, 32, 0);
    ImageList_AddIcon(hLarge, hLgIconItem);

    /* assign the image to the list view */
    ListView_SetImageList(hListView, hSmall, LVSIL_SMALL);
    ListView_SetImageList(hListView, hLarge, LVSIL_NORMAL);

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

    InitListViewImage();

    NumServices = GetServiceList();

    if (NumServices)
    {
        TCHAR buf[300];     /* buffer to hold key path */
        INT NumListedServ = 0; /* how many services were listed */

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

            if (GetDescription(pServiceStatus[Index].lpServiceName, &Description))
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
