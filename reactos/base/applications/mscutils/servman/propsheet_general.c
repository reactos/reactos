/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/propsheet_general.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2006-2009 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"


typedef struct _PAGEDATA
{
    PSERVICEPROPSHEET dlgInfo;
    BOOL bDisplayNameChanged;
    BOOL bDescriptionChanged;
    BOOL bBinaryPathChanged;
    BOOL bStartTypeChanged;
} PAGEDATA, *PPAGEDATA;


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

    hButton = GetDlgItem(hwndDlg, IDC_START_PARAM);
    EnableWindow(hButton, (State == SERVICE_STOPPED && lpServiceConfig && lpServiceConfig->dwStartType != SERVICE_DISABLED));

    if (lpServiceConfig)
        HeapFree(GetProcessHeap(), 0, lpServiceConfig);

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
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), TRUE);
    }
}

VOID
SaveDlgInfo(PPAGEDATA pPageData,
            HWND hwndDlg)
{
    LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;
    PWSTR pDisplayName = NULL;
    PWSTR pDescription;
    INT nLength;
    DWORD StartUp;

    pServiceConfig = HeapAlloc(ProcessHeap,
                               HEAP_ZERO_MEMORY,
                               sizeof(*pServiceConfig));
    if (pServiceConfig)
    {
        pServiceConfig->dwServiceType = SERVICE_NO_CHANGE;
        pServiceConfig->dwErrorControl = SERVICE_NO_CHANGE;
        pServiceConfig->dwStartType = SERVICE_NO_CHANGE;

        if (pPageData->bStartTypeChanged)
        {
            StartUp = SendDlgItemMessageW(hwndDlg, IDC_START_TYPE, CB_GETCURSEL, 0, 0);
            switch (StartUp)
            {
                case 0:
                    pServiceConfig->dwStartType = SERVICE_AUTO_START;
                    break;

                case 1:
                    pServiceConfig->dwStartType = SERVICE_DEMAND_START;
                    break;
                case 2:
                    pServiceConfig->dwStartType = SERVICE_DISABLED;
                    break;
            }
        }

        if (pPageData->bBinaryPathChanged)
        {
            nLength = SendDlgItemMessageW(hwndDlg, IDC_EXEPATH, WM_GETTEXTLENGTH, 0, 0);
            pServiceConfig->lpBinaryPathName = HeapAlloc(ProcessHeap,
                                                         HEAP_ZERO_MEMORY,
                                                         (nLength + 1) * sizeof(WCHAR));
            if (pServiceConfig->lpBinaryPathName != NULL)
                SendDlgItemMessageW(hwndDlg, IDC_EXEPATH, WM_GETTEXT, nLength + 1, (LPARAM)pServiceConfig->lpBinaryPathName);
        }

        if (pPageData->bDisplayNameChanged)
        {
            nLength = SendDlgItemMessageW(hwndDlg, IDC_DISP_NAME, WM_GETTEXTLENGTH, 0, 0);
            pDisplayName = HeapAlloc(ProcessHeap,
                                     HEAP_ZERO_MEMORY,
                                     (nLength + 1) * sizeof(WCHAR));
            if (pDisplayName != NULL)
            {
                SendDlgItemMessageW(hwndDlg, IDC_DISP_NAME, WM_GETTEXT, nLength + 1, (LPARAM)pDisplayName);

                if (pPageData->dlgInfo->pService->lpDisplayName)
                    HeapFree(ProcessHeap, 0, pPageData->dlgInfo->pService->lpDisplayName);

                pPageData->dlgInfo->pService->lpDisplayName = pDisplayName;
                pServiceConfig->lpDisplayName = pDisplayName;
            }
        }

        if (SetServiceConfig(pServiceConfig,
                             pPageData->dlgInfo->pService->lpServiceName,
                             NULL))
        {
            if (pPageData->bDisplayNameChanged)
                ChangeListViewText(pPageData->dlgInfo->Info,
                                   pPageData->dlgInfo->pService,
                                   LVNAME);

            if (pPageData->bStartTypeChanged)
                ChangeListViewText(pPageData->dlgInfo->Info,
                                   pPageData->dlgInfo->pService,
                                   LVSTARTUP);
        }

        if (pServiceConfig->lpBinaryPathName != NULL)
            HeapFree(ProcessHeap, 0, pServiceConfig->lpBinaryPathName);

        HeapFree(ProcessHeap, 0, pServiceConfig);
    }

    if (pPageData->bDescriptionChanged)
    {
        nLength = SendDlgItemMessageW(hwndDlg, IDC_DESCRIPTION, WM_GETTEXTLENGTH, 0, 0);
        pDescription = HeapAlloc(ProcessHeap, HEAP_ZERO_MEMORY, (nLength + 1) * sizeof(WCHAR));
        if (pDescription != NULL)
        {
            SendDlgItemMessageW(hwndDlg, IDC_DESCRIPTION, WM_GETTEXT, nLength + 1, (LPARAM)pDescription);

            if (SetServiceDescription(pPageData->dlgInfo->pService->lpServiceName,
                                      pDescription))
            {
                ChangeListViewText(pPageData->dlgInfo->Info,
                                   pPageData->dlgInfo->pService,
                                   LVDESC);
            }

            HeapFree(ProcessHeap, 0, pDescription);
        }
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
    PPAGEDATA pPageData;

    /* Get the window context */
    pPageData = (PPAGEDATA)GetWindowLongPtr(hwndDlg,
                                            GWLP_USERDATA);
    if (pPageData == NULL && uMsg != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pPageData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PAGEDATA));
            if (pPageData != NULL)
            {
                SetWindowLongPtr(hwndDlg,
                                 GWLP_USERDATA,
                                 (LONG_PTR)pPageData);

                pPageData->dlgInfo = (PSERVICEPROPSHEET)(((LPPROPSHEETPAGE)lParam)->lParam);
                if (pPageData->dlgInfo != NULL)
                {
                    InitGeneralPage(pPageData->dlgInfo, hwndDlg);
                    SetButtonStates(pPageData->dlgInfo, hwndDlg);
                }
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pPageData);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_START_TYPE:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        pPageData->bStartTypeChanged = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_DISP_NAME:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        pPageData->bDisplayNameChanged = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_DESCRIPTION:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        pPageData->bDescriptionChanged = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_EXEPATH:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        pPageData->bBinaryPathChanged = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_START:
                {
                    WCHAR szStartParams[256];
                    LPWSTR lpStartParams = NULL;

                    if (GetDlgItemText(hwndDlg, IDC_START_PARAM, szStartParams, 256) > 0)
                        lpStartParams = szStartParams;

                    EnableWindow(GetDlgItem(hwndDlg, IDC_START_PARAM), FALSE);

                    RunActionWithProgress(hwndDlg,
                                          pPageData->dlgInfo->pService->lpServiceName,
                                          pPageData->dlgInfo->pService->lpDisplayName,
                                          ACTION_START,
                                          lpStartParams);

                    UpdateServiceStatus(pPageData->dlgInfo->pService);
                    ChangeListViewText(pPageData->dlgInfo->Info, pPageData->dlgInfo->pService, LVSTATUS);
                    SetButtonStates(pPageData->dlgInfo, hwndDlg);
                    SetServiceStatusText(pPageData->dlgInfo, hwndDlg);
                    break;
                }

                case IDC_STOP:
                    RunActionWithProgress(hwndDlg,
                                          pPageData->dlgInfo->pService->lpServiceName,
                                          pPageData->dlgInfo->pService->lpDisplayName,
                                          ACTION_STOP,
                                          NULL);

                    UpdateServiceStatus(pPageData->dlgInfo->pService);
                    ChangeListViewText(pPageData->dlgInfo->Info, pPageData->dlgInfo->pService, LVSTATUS);
                    SetButtonStates(pPageData->dlgInfo, hwndDlg);
                    SetServiceStatusText(pPageData->dlgInfo, hwndDlg);
                    break;

                case IDC_PAUSE:
                    RunActionWithProgress(hwndDlg,
                                          pPageData->dlgInfo->pService->lpServiceName,
                                          pPageData->dlgInfo->pService->lpDisplayName,
                                          ACTION_PAUSE,
                                          NULL);

                    UpdateServiceStatus(pPageData->dlgInfo->pService);
                    ChangeListViewText(pPageData->dlgInfo->Info, pPageData->dlgInfo->pService, LVSTATUS);
                    SetButtonStates(pPageData->dlgInfo, hwndDlg);
                    SetServiceStatusText(pPageData->dlgInfo, hwndDlg);
                    break;

                case IDC_RESUME:
                    RunActionWithProgress(hwndDlg,
                                          pPageData->dlgInfo->pService->lpServiceName,
                                          pPageData->dlgInfo->pService->lpDisplayName,
                                          ACTION_RESUME,
                                          NULL);

                    UpdateServiceStatus(pPageData->dlgInfo->pService);
                    ChangeListViewText(pPageData->dlgInfo->Info, pPageData->dlgInfo->pService, LVSTATUS);
                    SetButtonStates(pPageData->dlgInfo, hwndDlg);
                    SetServiceStatusText(pPageData->dlgInfo, hwndDlg);
                    break;

                case IDC_EDIT:
                    SendDlgItemMessage(hwndDlg, IDC_DISP_NAME, EM_SETREADONLY, FALSE, 0);
                    SendDlgItemMessage(hwndDlg, IDC_DESCRIPTION, EM_SETREADONLY, FALSE, 0);
                    SendDlgItemMessage(hwndDlg, IDC_EXEPATH, EM_SETREADONLY, FALSE, 0);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                    if (pPageData->bDisplayNameChanged ||
                        pPageData->bDescriptionChanged ||
                        pPageData->bBinaryPathChanged ||
                        pPageData->bStartTypeChanged)
                    {
                        SaveDlgInfo(pPageData, hwndDlg);
                        SetButtonStates(pPageData->dlgInfo, hwndDlg);
                        pPageData->bDisplayNameChanged = FALSE;
                        pPageData->bDescriptionChanged = FALSE;
                        pPageData->bBinaryPathChanged = FALSE;
                        pPageData->bStartTypeChanged = FALSE;
                    }
                    break;
            }
            break;
    }

    return FALSE;
}
