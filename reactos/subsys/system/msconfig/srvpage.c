#include <precomp.h>

HWND hServicesPage;
HWND hServicesListCtrl;
HWND hServicesDialog;

void GetServices ( void );

INT_PTR CALLBACK
ServicesPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN   column;
	TCHAR       szTemp[256];
	DWORD dwStyle;

    switch (message) {
    case WM_INITDIALOG:

        hServicesListCtrl = GetDlgItem(hDlg, IDC_SERVICES_LIST);
        hServicesDialog = hDlg;

        dwStyle = SendMessage(hServicesListCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
        dwStyle = dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
        SendMessage(hServicesListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);

	    SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

        // Initialize the application page's controls
        column.mask = LVCF_TEXT | LVCF_WIDTH;

        LoadString(hInst, IDS_SERVICES_COLUMN_SERVICE, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 200;
        ListView_InsertColumn(hServicesListCtrl, 0, &column);

        column.mask = LVCF_TEXT | LVCF_WIDTH;
        LoadString(hInst, IDS_SERVICES_COLUMN_REQ, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 70;
        ListView_InsertColumn(hServicesListCtrl, 1, &column);

        column.mask = LVCF_TEXT | LVCF_WIDTH;
        LoadString(hInst, IDS_SERVICES_COLUMN_VENDOR, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 200;
        ListView_InsertColumn(hServicesListCtrl, 2, &column);

        column.mask = LVCF_TEXT | LVCF_WIDTH;
        LoadString(hInst, IDS_SERVICES_COLUMN_STATUS, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 70;
        ListView_InsertColumn(hServicesListCtrl, 3, &column);

        GetServices();
		return TRUE;
	}

  return 0;
}

void
GetServices ( void )
{
    LV_ITEM item;
    SC_HANDLE ScHandle;
    DWORD BufSize = 0;
    DWORD BytesNeeded = 0;
    DWORD ResumeHandle = 0;
    DWORD NumServices = 0; 
    size_t Index;
    TCHAR szStatus[128];

    ENUM_SERVICE_STATUS_PROCESS *pServiceStatus = NULL;

    ScHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (ScHandle != INVALID_HANDLE_VALUE)
    {
        if (EnumServicesStatusEx(ScHandle, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, (LPBYTE)pServiceStatus, BufSize, &BytesNeeded, &NumServices, &ResumeHandle, 0) == 0)
        {
            /* Call function again if required size was returned */
            if (GetLastError() == ERROR_MORE_DATA)
            {
                /* reserve memory for service info array */
                pServiceStatus = (ENUM_SERVICE_STATUS_PROCESS *) HeapAlloc(GetProcessHeap(), 0, BytesNeeded);
                if (pServiceStatus == NULL) 
			        return;

                /* fill array with service info */
                if (EnumServicesStatusEx(ScHandle, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, (LPBYTE)pServiceStatus, BytesNeeded, &BytesNeeded, &NumServices, &ResumeHandle, 0) == 0)
                {
                    HeapFree(GetProcessHeap(), 0, pServiceStatus);
                    return;
                }
            }
            else /* exit on failure */
            {
                return;
            }
        }

        if (NumServices)
        {
            for (Index = 0; Index < NumServices; Index++)
            {
                memset(&item, 0, sizeof(LV_ITEM));
                item.mask = LVIF_TEXT;
                item.iImage = 0;
                item.pszText = pServiceStatus[Index].lpDisplayName;
                item.iItem = ListView_GetItemCount(hServicesListCtrl);
                item.lParam = 0;
                item.iItem = ListView_InsertItem(hServicesListCtrl, &item);

                /* FIXME
                if (QueryServiceConfig2(ScHandle, SERVICE_CONFIG_FAILURE_ACTIONS, ) == 0)
                {
                    if (GetLastError() == ERROR_MORE_DATA)
                    {

                    }
                }
                LoadString(hInst, ( CONDITION  ? IDS_YES : IDS_NO), szStatus, 128);
                item.pszText = szStatus;
                item.iSubItem = 1;
                SendMessage(hServicesListCtrl, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);
                */

                LoadString(hInst, ((pServiceStatus[Index].ServiceStatusProcess.dwCurrentState == SERVICE_STOPPED) ? IDS_SERVICES_STATUS_STOPPED : IDS_SERVICES_STATUS_RUNNING), szStatus, 128);
                item.pszText = szStatus;
                item.iSubItem = 3;
                SendMessage(hServicesListCtrl, LVM_SETITEMTEXT, item.iItem, (LPARAM) &item);

            }
        }

        CloseServiceHandle(ScHandle);
    }        


}
