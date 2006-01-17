/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        subsys/system/servman/propsheet.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2005 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

extern ENUM_SERVICE_STATUS_PROCESS *pServiceStatus;
extern HINSTANCE hInstance;
extern HWND hListView;
extern INT SelectedItem;


typedef struct _PROP_DLG_INFO
{
    LPTSTR lpServiceName;
    LPTSTR lpDisplayName;
    LPTSTR lpDescription;
    LPTSTR lpPathToExe;
    DWORD  dwStartupType;
    DWORD  dwServiceStatus;
    LPTSTR lpStartParams;
} PROP_DLG_INFO, *PPROP_DLG_INFO;




VOID GetDlgInfo(HWND hwndDlg)
{
    HKEY hKey;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    LVITEM item;
    PROP_DLG_INFO DlgInfo;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");
    TCHAR buf[300];

    item.mask = LVIF_PARAM;
    item.iItem = SelectedItem;
    SendMessage(hListView, LVM_GETITEM, 0, (LPARAM)&item);

    /* copy pointer to selected service */
    Service = (ENUM_SERVICE_STATUS_PROCESS *)item.lParam;

    /* open the registry key for the service */
    _sntprintf(buf, 300, Path, Service->lpServiceName);
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 buf,
                 0,
                 KEY_READ,
                 &hKey);

    /* set the service name */
    DlgInfo.lpServiceName = Service->lpServiceName;
    SendDlgItemMessageW(hwndDlg, IDC_SERV_NAME, WM_SETTEXT, 0, (LPARAM)DlgInfo.lpServiceName);

    /* set the display name */
    DlgInfo.lpDisplayName = Service->lpDisplayName;
    SendDlgItemMessageW(hwndDlg, IDC_DISP_NAME, WM_SETTEXT, 0, (LPARAM)DlgInfo.lpDisplayName);

    /* set the description */
    if (GetDescription(hKey, &DlgInfo.lpDescription))
        SendDlgItemMessageW(hwndDlg, IDC_DESCRIPTION, WM_SETTEXT, 0, (LPARAM)DlgInfo.lpDescription);

    /* set the executable path */
    if (GetExecutablePath(&DlgInfo.lpPathToExe))
        SendDlgItemMessageW(hwndDlg, IDC_EXEPATH, WM_SETTEXT, 0, (LPARAM)DlgInfo.lpPathToExe);




}


#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif
/* Property page dialog callback */
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
