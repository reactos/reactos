/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig.c
 * PURPOSE:     msconfig main dialog
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */
#include <precomp.h>

HINSTANCE hInst = 0;

HWND hMainWnd;                   /* Main Window */
HWND hTabWnd;                    /* Tab Control Window */

void MsConfig_OnTabWndSelChange(void);

BOOL OnCreate(HWND hWnd)
{
    TCHAR   szTemp[256];
    TCITEM  item;

    hTabWnd = GetDlgItem(hWnd, IDC_TAB);
    hGeneralPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_GENERAL_PAGE), hWnd, (DLGPROC) GeneralPageWndProc);
    hSystemPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SYSTEM_PAGE), hWnd, (DLGPROC) SystemPageWndProc);
    hFreeLdrPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_FREELDR_PAGE), hWnd, (DLGPROC) FreeLdrPageWndProc);
    hServicesPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SERVICES_PAGE), hWnd, (DLGPROC) ServicesPageWndProc);
    hStartupPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_STARTUP_PAGE), hWnd, (DLGPROC) StartupPageWndProc);
    hToolsPage = CreateDialog(hInst, MAKEINTRESOURCE(IDD_TOOLS_PAGE), hWnd, (DLGPROC) ToolsPageWndProc);

    LoadString(hInst, IDS_MSCONFIG, szTemp, 256);
    SetWindowText(hWnd, szTemp);

    // Insert Tab Pages
    LoadString(hInst, IDS_TAB_GENERAL, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 0, &item);

    LoadString(hInst, IDS_TAB_SYSTEM, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 1, &item);

    LoadString(hInst, IDS_TAB_FREELDR, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 2, &item);

    LoadString(hInst, IDS_TAB_SERVICES, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 3, &item);

    LoadString(hInst, IDS_TAB_STARTUP, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 4, &item);

    LoadString(hInst, IDS_TAB_TOOLS, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 5, &item);

    MsConfig_OnTabWndSelChange();

    return TRUE;
}


void MsConfig_OnTabWndSelChange(void)
{
    switch (TabCtrl_GetCurSel(hTabWnd)) {
    case 0: //General
        ShowWindow(hGeneralPage, SW_SHOW);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hToolsPage, SW_HIDE);
        BringWindowToTop(hGeneralPage);
        break;
    case 1: //SYSTEM.INI
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_SHOW);
        ShowWindow(hToolsPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_HIDE);
        BringWindowToTop(hSystemPage);
		break;
    case 2: //Freeldr
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_SHOW);
        ShowWindow(hServicesPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hToolsPage, SW_HIDE);
        BringWindowToTop(hFreeLdrPage);
        break;
    case 3: //Services
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_SHOW);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hToolsPage, SW_HIDE);
        BringWindowToTop(hServicesPage);
        break;
    case 4: //startup
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_SHOW);
        ShowWindow(hToolsPage, SW_HIDE);
        BringWindowToTop(hStartupPage);
        break;
    case 5: //Tools
        ShowWindow(hGeneralPage, SW_HIDE);
        ShowWindow(hSystemPage, SW_HIDE);
        ShowWindow(hFreeLdrPage, SW_HIDE);
        ShowWindow(hServicesPage, SW_HIDE);
        ShowWindow(hStartupPage, SW_HIDE);
        ShowWindow(hToolsPage, SW_SHOW);
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
	static 			HICON hIcon;

    switch (message)
    {
        case WM_INITDIALOG:
            hMainWnd = hDlg;
            hIcon = LoadImage(GetModuleHandle(NULL),
                              MAKEINTRESOURCE(IDI_APPICON),
                              IMAGE_ICON,
                              16,
                              16,
                              0);
            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hIcon);
            return OnCreate(hDlg);

        case WM_COMMAND:

            if (LOWORD(wParam) == IDOK) 
            {
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
            DestroyWindow(hGeneralPage);
            DestroyWindow(hServicesPage);
            DestroyWindow(hStartupPage);
            DestroyWindow(hFreeLdrPage);
            DestroyWindow(hSystemPage);
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

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&InitControls);

    hInst = hInstance;
 
    DialogBox(hInst, (LPCTSTR)IDD_MSCONFIG_DIALOG, NULL, (DLGPROC) MsConfigWndProc);
  
    return 0;
}

/* EOF */
