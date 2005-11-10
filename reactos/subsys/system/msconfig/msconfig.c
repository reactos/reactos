#include <precomp.h>

HINSTANCE hInst = 0;

HWND hMainWnd;                   /* Main Window */
HWND hTabWnd;                    /* Tab Control Window */


BOOL OnCreate(HWND hWnd)
{
	TCHAR   szTemp[256];
	TCITEM  item;

	hTabWnd = GetDlgItem(hWnd, IDC_TAB);
    hToolsPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_TOOLS_PAGE), hWnd, ToolsPageWndProc);
    hServicesPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SERVICES_PAGE), hWnd, ServicesPageWndProc);
    hStartupPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_STARTUP_PAGE), hWnd, StartupPageWndProc);

	// Insert Tab Pages
	LoadString(hInst, IDS_TAB_GENERAL, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 0, &item);

	LoadString(hInst, IDS_TAB_SYSTEM, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 1, &item);

	LoadString(hInst, IDS_TAB_FREELDR, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 2, &item);

	LoadString(hInst, IDS_TAB_SERVICES, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 3, &item);

	LoadString(hInst, IDS_TAB_STARTUP, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 4, &item);

	LoadString(hInst, IDS_TAB_TOOLS, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    TabCtrl_InsertItem(hTabWnd, 5, &item);

	return TRUE;
}


void MsConfig_OnTabWndSelChange(void)
{
    switch (TabCtrl_GetCurSel(hTabWnd)) {
    case 0: //General
        ShowWindow(hToolsPage, SW_HIDE);
		ShowWindow(hStartupPage, SW_HIDE);
		//ShowWindow(hFreeLdrPage, SW_HIDE);
		ShowWindow(hServicesPage, SW_HIDE);
        //BringWindowToTop(hFreeLdrPage);
		break;
    case 1: //SYSTEM.INI
        ShowWindow(hToolsPage, SW_HIDE);
		ShowWindow(hStartupPage, SW_HIDE);
		//ShowWindow(hFreeLdrPage, SW_SHOW);
		ShowWindow(hServicesPage, SW_HIDE);
        //BringWindowToTop(hFreeLdrPage);
		break;
    case 2: //Freeldr
        ShowWindow(hToolsPage, SW_HIDE);
		ShowWindow(hStartupPage, SW_HIDE);
		//ShowWindow(hFreeLdrPage, SW_SHOW);
		ShowWindow(hServicesPage, SW_HIDE);
        //BringWindowToTop(hFreeLdrPage);
		break;
    case 3: //Services
        ShowWindow(hToolsPage, SW_HIDE);
		ShowWindow(hStartupPage, SW_HIDE);
		//ShowWindow(hFreeLdrPage, SW_HIDE);
		ShowWindow(hServicesPage, SW_SHOW);
        BringWindowToTop(hServicesPage);
		break;
    case 4: //startup
        ShowWindow(hToolsPage, SW_HIDE);
		ShowWindow(hStartupPage, SW_SHOW);
		//ShowWindow(hFreeLdrPage, SW_HIDE);
		ShowWindow(hServicesPage, SW_HIDE);
        BringWindowToTop(hStartupPage);
		break;
	case 5: //Tools
        ShowWindow(hToolsPage, SW_SHOW);
		ShowWindow(hStartupPage, SW_HIDE);
		//ShowWindow(hFreeLdrPage, SW_HIDE);
		ShowWindow(hServicesPage, SW_HIDE);
        BringWindowToTop(hToolsPage);
		break;
	}
}


/* Message handler for dialog box. */
INT_PTR CALLBACK
MsConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int             idctrl;
    LPNMHDR         pnmh;

    switch (message) {
    case WM_INITDIALOG:
        hMainWnd = hDlg;
        return OnCreate(hDlg);

	case WM_COMMAND:

		if (LOWORD(wParam) == IDOK) {
			//MsConfig_OnSaveChanges();
		}

        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
		}
		break;

    case WM_NOTIFY:
        idctrl = (int)wParam;
        pnmh = (LPNMHDR)lParam;
        if ((pnmh->hwndFrom == hTabWnd) &&
            (pnmh->idFrom == IDC_TAB) &&
            (pnmh->code == TCN_SELCHANGE))
        {
            MsConfig_OnTabWndSelChange();
        }
        break;

    case WM_DESTROY:
		DestroyWindow(hToolsPage);
		DestroyWindow(hServicesPage);
		DestroyWindow(hStartupPage);
        return DefWindowProc(hDlg, message, wParam, lParam);

    }

    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

    INITCOMMONCONTROLSEX InitControls;

    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&InitControls);

    hInst = hInstance;
 
    DialogBox(hInst, (LPCTSTR)IDD_MSCONFIG_DIALOG, NULL, MsConfigWndProc);
  
    return 0;
}

/* EOF */
