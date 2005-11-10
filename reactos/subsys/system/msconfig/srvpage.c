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
        LoadString(hInst, IDS_SERVICES_COLUMN_VENDOR, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 200;
        ListView_InsertColumn(hServicesListCtrl, 1, &column);

        column.mask = LVCF_TEXT | LVCF_WIDTH;
        LoadString(hInst, IDS_SERVICES_COLUMN_STATUS, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 70;
        ListView_InsertColumn(hServicesListCtrl, 2, &column);

        GetServices();
		return TRUE;
	}

  return 0;
}

void
GetServices ( void )
{
    HKEY hKey, hSubKey;
    DWORD dwSubKeys, dwKeyLength;
    DWORD dwType, dwDataLength;
    size_t Index;
    TCHAR lpKeyName[MAX_KEY_LENGTH];
    TCHAR lpSubKey[MAX_KEY_LENGTH];
    TCHAR DisplayName[MAX_VALUE_NAME];
    TCHAR ObjectName[MAX_VALUE_NAME];
    TCHAR lpServicesKey[MAX_KEY_LENGTH] = _T("SYSTEM\\CurrentControlSet\\Services");
    LV_ITEM item;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpServicesKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            for (Index = 0; Index < dwSubKeys; Index++) 
            { 
                dwKeyLength = MAX_KEY_LENGTH;
                if (RegEnumKeyEx(hKey, Index, lpKeyName, &dwKeyLength, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                {
                    _tcscpy(lpSubKey, lpServicesKey);
                    _tcscat(lpSubKey, _T("\\"));
                    _tcscat(lpSubKey, lpKeyName);
                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpSubKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
                    {
                        dwDataLength = MAX_VALUE_NAME;
                        if (RegQueryValueEx(hSubKey, _T("ObjectName"), NULL, &dwType, (LPBYTE)ObjectName, &dwDataLength) == ERROR_SUCCESS)
                        {
                            dwDataLength = MAX_VALUE_NAME;
                            if (RegQueryValueEx(hSubKey, _T("DisplayName"), NULL, &dwType, (LPBYTE)DisplayName, &dwDataLength) == ERROR_SUCCESS)
                            {
                                memset(&item, 0, sizeof(LV_ITEM));
                                item.mask = LVIF_TEXT;
                                item.iImage = 0;
                                item.pszText = DisplayName;
                                item.iItem = ListView_GetItemCount(hServicesListCtrl);
                                item.lParam = 0;
                                ListView_InsertItem(hServicesListCtrl, &item);
                            }
                        }
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }
}
