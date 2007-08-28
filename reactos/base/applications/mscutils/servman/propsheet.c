/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/propsheet.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

typedef struct _SERVICEPROPSHEET
{
    PMAIN_WND_INFO Info;
    ENUM_SERVICE_STATUS_PROCESS *pService;
} SERVICEPROPSHEET, *PSERVICEPROPSHEET;


static VOID
SetButtonStates(PSERVICEPROPSHEET dlgInfo,
                HWND hwndDlg)
{
    HWND hButton;
    DWORD Flags, State;

    Flags = dlgInfo->pService->ServiceStatusProcess.dwControlsAccepted;
    State = dlgInfo->pService->ServiceStatusProcess.dwCurrentState;

    if (State == SERVICE_STOPPED)
    {
        hButton = GetDlgItem(hwndDlg, IDC_START);
        EnableWindow (hButton, TRUE);
    }

    if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
    {
        hButton = GetDlgItem(hwndDlg, IDC_STOP);
        EnableWindow (hButton, TRUE);
    }

    if ( (Flags & SERVICE_ACCEPT_PAUSE_CONTINUE) && (State == SERVICE_RUNNING) )
    {
        hButton = GetDlgItem(hwndDlg, IDC_PAUSE);
        EnableWindow (hButton, TRUE);
    }

    if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
    {
        hButton = GetDlgItem(hwndDlg, IDC_PAUSE);
        EnableWindow (hButton, TRUE);
    }
}

/*
 * Fills the 'startup type' combo box with possible
 * values and sets it to value of the selected item
 */
static VOID
SetStartupType(LPTSTR lpServiceName,
               HWND hwndDlg)
{
    HWND hList;
    LPQUERY_SERVICE_CONFIG pServiceConfig;
    LPTSTR lpBuf;
    DWORD StartUp = 0;
    UINT i;

    hList = GetDlgItem(hwndDlg, IDC_START_TYPE);

    for (i = IDS_SERVICES_AUTO; i <= IDS_SERVICES_DIS; i++)
    {
        if (AllocAndLoadString(&lpBuf,
                               hInstance,
                               i))
        {
            SendMessage(hList,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)lpBuf);
            HeapFree(ProcessHeap,
                     0,
                     lpBuf);
        }
    }

    pServiceConfig = GetServiceConfig(lpServiceName);

    if (pServiceConfig)
    {
        switch (pServiceConfig->dwStartType)
        {
            case SERVICE_AUTO_START:   StartUp = 0; break;
            case SERVICE_DEMAND_START: StartUp = 1; break;
            case SERVICE_DISABLED:     StartUp = 2; break;
        }

        SendMessage(hList,
                    CB_SETCURSEL,
                    StartUp,
                    0);

        HeapFree(ProcessHeap,
                 0,
                 pServiceConfig);
    }
}


/*
 * Populates the General Properties dialog with
 * the relevant service information
 */
static VOID
GetDlgInfo(PSERVICEPROPSHEET dlgInfo,
           HWND hwndDlg)
{
    LPQUERY_SERVICE_CONFIG pServiceConfig;
    LPTSTR lpDescription;

    /* set the service name */
    SendDlgItemMessage(hwndDlg,
                       IDC_SERV_NAME,
                       WM_SETTEXT,
                       0,
                       (LPARAM)dlgInfo->pService->lpServiceName);

    /* set the display name */
    SendDlgItemMessage(hwndDlg,
                       IDC_DISP_NAME,
                       WM_SETTEXT,
                       0,
                       (LPARAM)dlgInfo->pService->lpDisplayName);

    /* set the description */
    if ((lpDescription = GetServiceDescription(dlgInfo->pService->lpServiceName)))
    {
        SendDlgItemMessage(hwndDlg,
                           IDC_DESCRIPTION,
                           WM_SETTEXT,
                           0,
                           (LPARAM)lpDescription);

        HeapFree(ProcessHeap,
                 0,
                 lpDescription);
    }

    pServiceConfig = GetServiceConfig(dlgInfo->pService->lpServiceName);
    if (pServiceConfig)
    {
        SendDlgItemMessage(hwndDlg,
                           IDC_EXEPATH,
                           WM_SETTEXT,
                           0,
                           (LPARAM)pServiceConfig->lpBinaryPathName);
    }


    /* set startup type */
    SetStartupType(dlgInfo->pService->lpServiceName, hwndDlg);

    /* set service status */
    if (dlgInfo->pService->ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
    {
        TCHAR szServiceStatus[32];

        LoadString(hInstance,
                   IDS_SERVICES_STARTED,
                   szServiceStatus,
                   _tcslen(szServiceStatus) + 1);

        SendDlgItemMessage(hwndDlg,
                           IDC_SERV_STATUS,
                           WM_SETTEXT,
                           0,
                           (LPARAM)szServiceStatus);
    }
    else
    {
        TCHAR szServiceStatus[32];

        LoadString(hInstance,
                   IDS_SERVICES_STOPPED,
                   szServiceStatus,
                   _tcslen(szServiceStatus) + 1);

        SendDlgItemMessage(hwndDlg,
                           IDC_SERV_STATUS,
                           WM_SETTEXT,
                           0,
                           (LPARAM)szServiceStatus);
    }
}


/*
 * General Property dialog callback.
 * Controls messages to the General dialog
 */
static INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    PSERVICEPROPSHEET dlgInfo;

    /* Get the window context */
    dlgInfo = (PSERVICEPROPSHEET)GetWindowLongPtr(hwndDlg,
                                                  GWLP_USERDATA);
    if (dlgInfo == NULL && uMsg != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            dlgInfo = (PSERVICEPROPSHEET)(((LPPROPSHEETPAGE)lParam)->lParam);
            if (dlgInfo != NULL)
            {
                SetWindowLongPtr(hwndDlg,
                                 GWLP_USERDATA,
                                 (LONG_PTR)dlgInfo);
                GetDlgInfo(dlgInfo, hwndDlg);
                SetButtonStates(dlgInfo, hwndDlg);
            }
        }
        break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_START_TYPE:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;

                case IDC_START:
                    if (DoStart(dlgInfo->Info))
                    {
                        //UpdateServiceStatus(dlgInfo->pService);
                        //ChangeListViewText(dlgInfo, LVSTATUS);
                        //SetMenuAndButtonStates(Info);
                    }
                break;

                case IDC_STOP:
                    //SendMessage(Info->hMainWnd, WM_COMMAND, ID_STOP, 0);
                break;

                case IDC_PAUSE:
                    //SendMessage(Info->hMainWnd, WM_COMMAND, ID_PAUSE, 0);
                break;

                case IDC_RESUME:
                    //SendMessage(Info->hMainWnd, WM_COMMAND, ID_RESUME, 0);
                break;

                case IDC_START_PARAM:
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
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
                    case PSN_APPLY:
                        MessageBox(NULL, _T("apply"), NULL, 0);
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
static INT_PTR CALLBACK
DependanciesPageProc(HWND hwndDlg,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    PMAIN_WND_INFO Info;

    /* Get the window context */
    Info = (PMAIN_WND_INFO)GetWindowLongPtr(hwndDlg,
                                            GWLP_USERDATA);

    if (Info == NULL && uMsg != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            Info = (PMAIN_WND_INFO)(((LPPROPSHEETPAGE)lParam)->lParam);
            if (Info != NULL)
            {
                SetWindowLongPtr(hwndDlg,
                                 GWLP_USERDATA,
                                 (LONG_PTR)Info);
            }
        }
        break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {

            }
            break;

        case WM_DESTROY:
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                switch (lpnm->code)
                {

                }

            }
            break;
    }

    return FALSE;
}


static INT CALLBACK
AddEditButton(HWND hwnd, UINT message, LPARAM lParam)
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
InitPropSheetPage(PROPSHEETPAGE *psp,
                  PSERVICEPROPSHEET dlgInfo,
                  WORD idDlg,
                  DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hInstance;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
  psp->lParam = (LPARAM)dlgInfo;
}


LONG APIENTRY
OpenPropSheet(PMAIN_WND_INFO Info)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[2];
    PSERVICEPROPSHEET pServicePropSheet;
    LONG Ret = 0;

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USECALLBACK;
    psh.hwndParent = Info->hMainWnd;
    psh.hInstance = hInstance;
    psh.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SM_ICON));
    psh.pszCaption = Info->pCurrentService->lpDisplayName;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.pfnCallback = AddEditButton;
    psh.ppsp = psp;


    pServicePropSheet = HeapAlloc(ProcessHeap,
                                  0,
                                  sizeof(*pServicePropSheet));
    if (pServicePropSheet)
    {
        /* save current service, as it could change while the dialog is open */
        pServicePropSheet->pService = Info->pCurrentService;
        pServicePropSheet->Info = Info;

        InitPropSheetPage(&psp[0], pServicePropSheet, IDD_DLG_GENERAL, GeneralPageProc);
        //InitPropSheetPage(&psp[1], Info, IDD_DLG_GENERAL, LogonPageProc);
        //InitPropSheetPage(&psp[2], Info, IDD_DLG_GENERAL, RecoveryPageProc);
        InitPropSheetPage(&psp[1], pServicePropSheet, IDD_DLG_DEPEND, DependanciesPageProc);

        Ret = (LONG)(PropertySheet(&psh) != -1);

        HeapFree(ProcessHeap,
                 0,
                 pServicePropSheet);
    }

    return Ret;
}

