#include <precomp.h>

HWND hServicesPage;
HWND hServicesListCtrl;
HWND hServicesDialog;

INT_PTR CALLBACK
ServicesPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN   column;
	TCHAR       szTemp[256];
	DWORD dwStyle;

    switch (message) {
    case WM_INITDIALOG:

        hServicesListCtrl = GetDlgItem(hDlg, IDC_TOOLS_LIST);
        hServicesDialog = hDlg;

        dwStyle = SendMessage(hServicesListCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
        dwStyle = dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
        SendMessage(hServicesListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);

	    SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

        // Initialize the application page's controls
        column.mask = LVCF_TEXT | LVCF_WIDTH;

        LoadString(hInst, IDS_SERVICES_COLUMN_SERVICE, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 150;
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
        ListView_InsertColumn(hServicesListCtrl, 1, &column);

		return TRUE;
	}

  return 0;
}
