/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/propsheet_general.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2006-2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

static VOID
SetButtonStates(PSERVICEPROPSHEET dlgInfo,
                HWND hwndDlg)
{
    HWND hButton;
    LPQUERY_SERVICE_CONFIG lpServiceConfig;
    DWORD Flags, State;
    UINT i;

    Flags = dlgInfo->pService->ServiceStatusProcess.dwControlsAccepted;
    State = dlgInfo->pService->ServiceStatusProcess.dwCurrentState;

    for (i = IDC_START; i <= IDC_RESUME; i++)
    {
        hButton = GetDlgItem(hwndDlg, i);
        EnableWindow (hButton, FALSE);
    }

    lpServiceConfig = GetServiceConfig(dlgInfo->pService->lpServiceName);
    if (State == SERVICE_STOPPED &&
        lpServiceConfig && lpServiceConfig->dwStartType != SERVICE_DISABLED)
    {
        hButton = GetDlgItem(hwndDlg, IDC_START);
        EnableWindow (hButton, TRUE);
    }
    else if ( (Flags & SERVICE_ACCEPT_STOP) && (State == SERVICE_RUNNING) )
    {
        hButton = GetDlgItem(hwndDlg, IDC_STOP);
        EnableWindow (hButton, TRUE);
    }
    else if ( (Flags & SERVICE_ACCEPT_PAUSE_CONTINUE) && (State == SERVICE_RUNNING) )
    {
        hButton = GetDlgItem(hwndDlg, IDC_PAUSE);
        EnableWindow (hButton, TRUE);
    }

    if(lpServiceConfig)
        HeapFree(GetProcessHeap(), 0, lpServiceConfig);

    hButton = GetDlgItem(hwndDlg, IDC_START_PARAM);
    EnableWindow(hButton, (State == SERVICE_STOPPED));

    /* set the main toolbar */
    SetMenuAndButtonStates(dlgInfo->Info);
}

static VOID
SetServiceStatusText(PSERVICEPROPSHEET dlgInfo,
                     HWND hwndDlg)
{
    LPWSTR lpStatus;
    UINT id;

    if (dlgInfo->pService->ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
    {
        id = IDS_SERVICES_STARTED;
    }
    else
    {
        id = IDS_SERVICES_STOPPED;
    }

    if (AllocAndLoadString(&lpStatus,
                           hInstance,
                           id))
    {
        SendDlgItemMessageW(hwndDlg,
                            IDC_SERV_STATUS,
                            WM_SETTEXT,
                            0,
                            (LPARAM)lpStatus);
        LocalFree(lpStatus);
    }
}

/*
 * Fills the 'startup type' combo box with possible
 * values and sets it to value of the selected item
 */
static VOID
SetStartupType(LPWSTR lpServiceName,
               HWND hwndDlg)
{
    HWND hList;
    LPQUERY_SERVICE_CONFIG pServiceConfig;
    LPWSTR lpBuf;
    DWORD StartUp = 0;
    UINT i;

    hList = GetDlgItem(hwndDlg, IDC_START_TYPE);

    for (i = IDS_SERVICES_AUTO; i <= IDS_SERVICES_DIS; i++)
    {
        if (AllocAndLoadString(&lpBuf,
                               hInstance,
                               i))
        {
            SendMessageW(hList,
                         CB_ADDSTRING,
                         0,
                         (LPARAM)lpBuf);
            LocalFree(lpBuf);
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

        SendMessageW(hList,
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
InitGeneralPage(PSERVICEPROPSHEET dlgInfo,
                HWND hwndDlg)
{
    LPQUERY_SERVICE_CONFIG pServiceConfig;
    LPWSTR lpDescription;

    /* set the service name */
    SendDlgItemMessageW(hwndDlg,
                        IDC_SERV_NAME,
                        WM_SETTEXT,
                        0,
                        (LPARAM)dlgInfo->pService->lpServiceName);

    /* set the display name */
    SendDlgItemMessageW(hwndDlg,
                        IDC_DISP_NAME,
                        WM_SETTEXT,
                        0,
                        (LPARAM)dlgInfo->pService->lpDisplayName);

    /* set the description */
    if ((lpDescription = GetServiceDescription(dlgInfo->pService->lpServiceName)))
    {
        SendDlgItemMessageW(hwndDlg,
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
        SendDlgItemMessageW(hwndDlg,
                            IDC_EXEPATH,
                            WM_SETTEXT,
                            0,
                            (LPARAM)pServiceConfig->lpBinaryPathName);
        HeapFree(ProcessHeap,
                         0,
                         pServiceConfig);
    }


    /* set startup type */
    SetStartupType(dlgInfo->pService->lpServiceName, hwndDlg);

    SetServiceStatusText(dlgInfo,
                         hwndDlg);

    if (dlgInfo->Info->bIsUserAnAdmin)
    {
        HWND hEdit = GetDlgItem(hwndDlg,
                                IDC_EDIT);
        EnableWindow(hEdit,
                     TRUE);
    }
}

VOID
SaveDlgInfo(PSERVICEPROPSHEET dlgInfo,
            HWND hwndDlg)
{
    LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;
    HWND hList;
    DWORD StartUp;

    pServiceConfig = HeapAlloc(ProcessHeap,
                               HEAP_ZERO_MEMORY,
                               sizeof(*pServiceConfig));
    if (pServiceConfig)
    {
        pServiceConfig->dwServiceType = SERVICE_NO_CHANGE;
        pServiceConfig->dwErrorControl = SERVICE_NO_CHANGE;

        hList = GetDlgItem(hwndDlg, IDC_START_TYPE);
        StartUp = SendMessage(hList,
                              CB_GETCURSEL,
                              0,
                              0);
        switch (StartUp)
        {
            case 0: pServiceConfig->dwStartType = SERVICE_AUTO_START; break;
            case 1: pServiceConfig->dwStartType = SERVICE_DEMAND_START; break;
            case 2: pServiceConfig->dwStartType = SERVICE_DISABLED; break;
        }

        if (SetServiceConfig(pServiceConfig,
                             dlgInfo->pService->lpServiceName,
                             NULL))
        {
            ChangeListViewText(dlgInfo->Info,
                               dlgInfo->pService,
                               LVSTARTUP);
        }

        HeapFree(ProcessHeap,
                 0,
                 pServiceConfig);
    }
}

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
                InitGeneralPage(dlgInfo, hwndDlg);
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
                {
                    WCHAR szStartParams[256];
                    LPWSTR lpStartParams = NULL;

                    if (GetDlgItemText(hwndDlg, IDC_START_PARAM, szStartParams, 256) > 0)
                        lpStartParams = szStartParams;

                    RunActionWithProgress(hwndDlg,
                                          dlgInfo->pService->lpServiceName,
                                          dlgInfo->pService->lpDisplayName,
                                          ACTION_START,
                                          lpStartParams);

                    UpdateServiceStatus(dlgInfo->pService);
                    ChangeListViewText(dlgInfo->Info, dlgInfo->pService, LVSTATUS);
                    SetButtonStates(dlgInfo, hwndDlg);
                    SetServiceStatusText(dlgInfo, hwndDlg);
                    break;
                }

                case IDC_STOP:
                    RunActionWithProgress(hwndDlg,
                                          dlgInfo->pService->lpServiceName,
                                          dlgInfo->pService->lpDisplayName,
                                          ACTION_STOP,
                                          NULL);

                    UpdateServiceStatus(dlgInfo->pService);
                    ChangeListViewText(dlgInfo->Info, dlgInfo->pService, LVSTATUS);
                    SetButtonStates(dlgInfo, hwndDlg);
                    SetServiceStatusText(dlgInfo, hwndDlg);
                    break;

                case IDC_PAUSE:
                    RunActionWithProgress(hwndDlg,
                                          dlgInfo->pService->lpServiceName,
                                          dlgInfo->pService->lpDisplayName,
                                          ACTION_PAUSE,
                                          NULL);

                    UpdateServiceStatus(dlgInfo->pService);
                    ChangeListViewText(dlgInfo->Info, dlgInfo->pService, LVSTATUS);
                    SetButtonStates(dlgInfo, hwndDlg);
                    SetServiceStatusText(dlgInfo, hwndDlg);
                    break;

                case IDC_RESUME:
                    RunActionWithProgress(hwndDlg,
                                          dlgInfo->pService->lpServiceName,
                                          dlgInfo->pService->lpDisplayName,
                                          ACTION_RESUME,
                                          NULL);

                    UpdateServiceStatus(dlgInfo->pService);
                    ChangeListViewText(dlgInfo->Info, dlgInfo->pService, LVSTATUS);
                    SetButtonStates(dlgInfo, hwndDlg);
                    SetServiceStatusText(dlgInfo, hwndDlg);
                    break;

                case IDC_EDIT:
                {
                    HWND hName, hDesc, hExePath;

                    hName = GetDlgItem(hwndDlg, IDC_DISP_NAME);
                    hDesc = GetDlgItem(hwndDlg, IDC_DESCRIPTION);
                    hExePath = GetDlgItem(hwndDlg, IDC_EXEPATH);

                    SendMessage(hName, EM_SETREADONLY, FALSE, 0);
                    SendMessage(hDesc, EM_SETREADONLY, FALSE, 0);
                    SendMessage(hExePath, EM_SETREADONLY, FALSE, 0);
                    break;
                }

                case IDC_START_PARAM:
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR lpnm = (LPNMHDR)lParam;

                switch (lpnm->code)
                {
                    case PSN_APPLY:
                        SaveDlgInfo(dlgInfo, hwndDlg);
                        SetButtonStates(dlgInfo, hwndDlg);
                    break;
                }
            }
        break;
    }

    return FALSE;
}
