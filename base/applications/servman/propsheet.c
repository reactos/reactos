/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/propsheet.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "servman.h"

HWND hwndGenDlg;
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



VOID SetButtonStates()
{
    HWND hButton;
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    DWORD Flags, State;

    /* get pointer to selected service */
    Service = GetSelectedService();

    Flags = Service->ServiceStatusProcess.dwControlsAccepted;
    State = Service->ServiceStatusProcess.dwCurrentState;

    if (State == SERVICE_STOPPED)
    {
        hButton = GetDlgItem(hwndGenDlg, IDC_START);
        EnableWindow (hButton, TRUE);
    }

    if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
    {
        hButton = GetDlgItem(hwndGenDlg, IDC_STOP);
        EnableWindow (hButton, TRUE);
    }

    if ( (Flags & SERVICE_ACCEPT_PAUSE_CONTINUE) && (State == SERVICE_RUNNING) )
    {
        hButton = GetDlgItem(hwndGenDlg, IDC_PAUSE);
        EnableWindow (hButton, TRUE);
    }

    if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
    {
        hButton = GetDlgItem(hwndGenDlg, IDC_PAUSE);
        EnableWindow (hButton, TRUE);
    }
}

/*
 * Fills the 'startup type' combo box with possible
 * values and sets it to value of the selected item
 */
VOID SetStartupType(LPTSTR lpServiceName)
{
    HWND hList;
    HKEY hKey;
    TCHAR buf[25];
    DWORD dwValueSize = 0;
    DWORD StartUp = 0;
    LPCTSTR Path = _T("System\\CurrentControlSet\\Services\\%s");
    TCHAR KeyBuf[300];

    /* open the registry key for the service */
    _sntprintf(KeyBuf, sizeof(KeyBuf) / sizeof(TCHAR), Path, lpServiceName);
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 KeyBuf,
                 0,
                 KEY_READ,
                 &hKey);

    hList = GetDlgItem(hwndGenDlg, IDC_START_TYPE);

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
VOID GetDlgInfo()
{
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;
    PROP_DLG_INFO DlgInfo;
    

    /* get pointer to selected service */
    Service = GetSelectedService();


    /* set the service name */
    DlgInfo.lpServiceName = Service->lpServiceName;
    SendDlgItemMessage(hwndGenDlg, IDC_SERV_NAME, WM_SETTEXT, 0, (
        LPARAM)DlgInfo.lpServiceName);


    /* set the display name */
    DlgInfo.lpDisplayName = Service->lpDisplayName;
    SendDlgItemMessage(hwndGenDlg, IDC_DISP_NAME, WM_SETTEXT, 0,
        (LPARAM)DlgInfo.lpDisplayName);


    /* set the description */
    if (GetDescription(Service->lpServiceName, &DlgInfo.lpDescription))
        SendDlgItemMessage(hwndGenDlg, IDC_DESCRIPTION, WM_SETTEXT, 0,
            (LPARAM)DlgInfo.lpDescription);


    /* set the executable path */
    if (GetExecutablePath(&DlgInfo.lpPathToExe))
        SendDlgItemMessage(hwndGenDlg, IDC_EXEPATH, WM_SETTEXT, 0, (LPARAM)DlgInfo.lpPathToExe);


    /* set startup type */
    SetStartupType(Service->lpServiceName);


    /* set service status */
    if (Service->ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
    {
        LoadString(hInstance, IDS_SERVICES_STARTED, DlgInfo.szServiceStatus,
            sizeof(DlgInfo.szServiceStatus) / sizeof(TCHAR));
        SendDlgItemMessageW(hwndGenDlg, IDC_SERV_STATUS, WM_SETTEXT, 0, (LPARAM)DlgInfo.szServiceStatus);
    }
    else
    {
        LoadString(hInstance, IDS_SERVICES_STOPPED, DlgInfo.szServiceStatus,
            sizeof(DlgInfo.szServiceStatus) / sizeof(TCHAR));
        SendDlgItemMessageW(hwndGenDlg, IDC_SERV_STATUS, WM_SETTEXT, 0, (LPARAM)DlgInfo.szServiceStatus);
    }

}



#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

/*
 * General Property dialog callback.
 * Controls messages to the General dialog
 */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg,
                UINT uMsg,
		        WPARAM wParam,
		        LPARAM lParam)
{
    hwndGenDlg = hwndDlg;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            GetDlgInfo();
            SetButtonStates();
        break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_START_TYPE:
                    /* Enable the 'Apply' button */
                    //PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;

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

                case IDC_START_PARAM:
                    /* Enable the 'Apply' button */
                    //PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;

            }
            break;

        case WM_DESTROY:
        break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                switch (lpnm->code)
                {
                    case MCN_SELECT:
                        /* Enable the 'Apply' button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                    default:
                    break;
                }
            }
        break;
    }

    return FALSE;
}



/*
 * Dependancies Property dialog callback.
 * Controls messages to the Dependancies dialog
 */
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


INT CALLBACK AddEditButton(HWND hwnd, UINT message, LPARAM lParam)
{
    HWND hEditButton;

    switch (message)
    {
        case PSCB_PRECREATE:
            /*hEditButton = CreateWindowEx(0,
                                         WC_BUTTON,
                                         NULL,
                                         WS_CHILD | WS_VISIBLE,
                                         20, 300, 30, 15,
                                         hwnd,
                                         NULL,
                                         hInstance,
                                         NULL);
            if (hEditButton == NULL)
                GetError(0);*/

            hEditButton = GetDlgItem(hwnd, PSBTN_OK);
            DestroyWindow(hEditButton);
            //SetWindowText(hEditButton, _T("test"));

            return TRUE;
    }
    return TRUE;
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
    ENUM_SERVICE_STATUS_PROCESS *Service = NULL;

    Service = GetSelectedService();

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USECALLBACK;
    psh.hwndParent = hwnd;
    psh.hInstance = hInstance;
    psh.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SM_ICON));
    psh.pszCaption = Service->lpDisplayName;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.pfnCallback = AddEditButton;
    psh.ppsp = psp;


    InitPropSheetPage(&psp[0], IDD_DLG_GENERAL, GeneralPageProc);
    //InitPropSheetPage(&psp[1], IDD_DLG_GENERAL, LogonPageProc);
    //InitPropSheetPage(&psp[2], IDD_DLG_GENERAL, RecoveryPageProc);
    InitPropSheetPage(&psp[1], IDD_DLG_DEPEND, DependanciesPageProc);

    return (LONG)(PropertySheet(&psh) != -1);
}

