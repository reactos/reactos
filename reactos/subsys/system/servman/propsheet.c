/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        subsys/system/servman/propsheet.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

extern ENUM_SERVICE_STATUS_PROCESS *pServiceStatus;
extern HINSTANCE hInstance;
extern HWND hListView;
extern HWND hMainWnd;


typedef struct _PROP_DLG_INFO
{
    LPTSTR lpServiceName;
    LPTSTR lpDisplayName;
    LPTSTR lpDescription;
    LPTSTR lpPathToExe;
    TCHAR  szStartupType;
    TCHAR  szServiceStatus[25];
    LPTSTR lpStartParams;
} PROP_DLG_INFO, *PPROP_DLG_INFO;


/*
 * Fills the 'startup type' combo box with possible
 * values and sets it to value of the selected item
 */
VOID SetStartupType(HKEY hKey, HWND hwndDlg)
{
    HWND hList;
    TCHAR buf[25];
    DWORD dwValueSize = 0;
    DWORD StartUp = 0;

    hList = GetDlgItem(hwndDlg, IDC_START_TYPE);

    LoadString(hInstance, IDS_SERVICES_AUTO, buf, sizeof(buf) / sizeof(TCHAR));
    SendMessage(hList, CB_ADDSTRING, 0, (LPARAM)buf);
    LoadString(hInstance, IDS_SERVICES_MAN, buf, sizeof(buf) / sizeof(TCHAR));
    SendMessage(hList, CB_ADDSTRING, 0, (LPARAM)buf);
    LoadString(hInstance, IDS_SERVICES_DIS, buf, sizeof(buf) / sizeof(TCHAR));
    SendMessage(hList, CB_ADDSTRING, 0, (LPARAM)buf);

    dwValueSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey,
                        _T("Start"),
                        NULL,
                        NULL,
                        (LPBYTE)&StartUp,
                        &dwValueSize))
    {
        RegCloseKey(hKey);
        return;
    }

    if (StartUp == 0x02)
        SendMessage(hList, CB_SETCURSEL, 0, 0);
    else if (StartUp == 0x03)
        SendMessage(hList, CB_SETCURSEL, 1, 0);
    else if (StartUp == 0x04)
        SendMessage(hList, CB_SETCURSEL, 2, 0);

}


/*
 * Populates the General Properties dialog with
 * the relevant service information
 */
VOID GetDlgInfo(HWND hwndDlg)
{
    HKEY hKey;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    LVITEM item;
    PROP_DLG_INFO DlgInfo;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");
    TCHAR buf[300];

    item.mask = LVIF_PARAM;
    item.iItem = GetSelectedItem();
    SendMessage(hListView, LVM_GETITEM, 0, (LPARAM)&item);

    /* copy pointer to selected service */
    Service = (ENUM_SERVICE_STATUS_PROCESS *)item.lParam;

    /* open the registry key for the service */
    _sntprintf(buf, sizeof(buf) / sizeof(TCHAR), Path, Service->lpServiceName);
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 buf,
                 0,
                 KEY_READ,
                 &hKey);

    /* set the service name */
    DlgInfo.lpServiceName = Service->lpServiceName;
    SendDlgItemMessage(hwndDlg, IDC_SERV_NAME, WM_SETTEXT, 0, (
        LPARAM)DlgInfo.lpServiceName);


    /* set the display name */
    DlgInfo.lpDisplayName = Service->lpDisplayName;
    SendDlgItemMessage(hwndDlg, IDC_DISP_NAME, WM_SETTEXT, 0,
        (LPARAM)DlgInfo.lpDisplayName);


    /* set the description */
    if (GetDescription(hKey, &DlgInfo.lpDescription))
        SendDlgItemMessage(hwndDlg, IDC_DESCRIPTION, WM_SETTEXT, 0,
            (LPARAM)DlgInfo.lpDescription);


    /* FIXME: needs implementing. Use code base at bottom of query.c */
    /* set the executable path */
    if (GetExecutablePath(&DlgInfo.lpPathToExe))
        SendDlgItemMessage(hwndDlg, IDC_EXEPATH, WM_SETTEXT, 0, (LPARAM)DlgInfo.lpPathToExe);


    /* set startup type */
    SetStartupType(hKey, hwndDlg);



    /* set service status */
    if (Service->ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
    {
        LoadString(hInstance, IDS_SERVICES_STARTED, DlgInfo.szServiceStatus,
            sizeof(DlgInfo.szServiceStatus) / sizeof(TCHAR));
        SendDlgItemMessageW(hwndDlg, IDC_SERV_STATUS, WM_SETTEXT, 0, (LPARAM)DlgInfo.szServiceStatus);
    }
    else
    {
        LoadString(hInstance, IDS_SERVICES_STOPPED, DlgInfo.szServiceStatus,
            sizeof(DlgInfo.szServiceStatus) / sizeof(TCHAR));
        SendDlgItemMessageW(hwndDlg, IDC_SERV_STATUS, WM_SETTEXT, 0, (LPARAM)DlgInfo.szServiceStatus);
    }




}


#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

/*
 * General Property dialog callback.
 * Controls messages to the General dialog
 */
/* FIXME: this may be better as a modeless dialog */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg,
                UINT uMsg,
		        WPARAM wParam,
		        LPARAM lParam)
{

    switch (uMsg)
    {
        case WM_INITDIALOG:
            GetDlgInfo(hwndDlg);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_START:
                    SendMessage(hMainWnd, WM_COMMAND, ID_START, 0);
                    break;

                case IDC_STOP:
                    SendMessage(hMainWnd, WM_COMMAND, ID_STOP, 0);
                    break;

                case IDC_PAUSE:
                    SendMessage(hMainWnd, WM_COMMAND, ID_PAUSE, 0);
                    break;

                case IDC_RESUME:
                    SendMessage(hMainWnd, WM_COMMAND, ID_RESUME, 0);
                    break;
            }
            break;

        case WM_DESTROY:
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                switch (lpnm->code)

                default:
                    break;
            }
            break;
    }

    return FALSE;
}



/*
 * Dependancies Property dialog callback.
 * Controls messages to the Dependancies dialog
 */
/* FIXME: this may be better as a modeless dialog */
INT_PTR CALLBACK
DependanciesPageProc(HWND hwndDlg,
                UINT uMsg,
		        WPARAM wParam,
		        LPARAM lParam)
{

    switch (uMsg)
    {
        case WM_INITDIALOG:

            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_START:
                    break;

                case IDC_STOP:

                    break;
            }
            break;

        case WM_DESTROY:
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                switch (lpnm->code)

                default:
                    break;
            }
            break;
    }

    return FALSE;
}


static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hInstance;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
}


LONG APIENTRY
OpenPropSheet(HWND hwnd)
{
  PROPSHEETHEADER psh;
  PROPSHEETPAGE psp[2];
  TCHAR Caption[256];

  LoadString(hInstance, IDS_PROP_SHEET, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hInstance;
  psh.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SM_ICON));
  psh.pszCaption = Caption;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.ppsp = psp;

  InitPropSheetPage(&psp[0], IDD_DLG_GENERAL, GeneralPageProc);
  //InitPropSheetPage(&psp[1], IDD_DLG_GENERAL, LogonPageProc);
  //InitPropSheetPage(&psp[2], IDD_DLG_GENERAL, RecoveryPageProc);
  InitPropSheetPage(&psp[1], IDD_DLG_DEPEND, DependanciesPageProc);


  return (LONG)(PropertySheet(&psh) != -1);
}

