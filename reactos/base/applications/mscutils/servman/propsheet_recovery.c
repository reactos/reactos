/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/propsheet_recovery.c
 * PURPOSE:     Recovery property page
 * COPYRIGHT:   Eric Kohl
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef struct _RECOVERYDATA
{
    ENUM_SERVICE_STATUS_PROCESS *pService;
    LPSERVICE_FAILURE_ACTIONS pServiceFailure;
    BOOL bChanged;
} RECOVERYDATA, *PRECOVERYDATA;

static
VOID
InitRecoveryPage(
    HWND hwndDlg)
{
    LPWSTR lpAction;
    INT id;

    for (id = IDS_NO_ACTION; id <= IDS_RESTART_COMPUTER; id++)
    {
        if (AllocAndLoadString(&lpAction,
                               hInstance,
                               id))
        {
            SendDlgItemMessageW(hwndDlg,
                                IDC_FIRST_FAILURE,
                                CB_ADDSTRING,
                                0,
                                (LPARAM)lpAction);

            SendDlgItemMessageW(hwndDlg,
                                IDC_SECOND_FAILURE,
                                CB_ADDSTRING,
                                0,
                                (LPARAM)lpAction);

            SendDlgItemMessageW(hwndDlg,
                                IDC_SUBSEQUENT_FAILURES,
                                CB_ADDSTRING,
                                0,
                                (LPARAM)lpAction);

            LocalFree(lpAction);
        }
    }

    SendDlgItemMessageW(hwndDlg,
                        IDC_FIRST_FAILURE,
                        CB_SETCURSEL,
                        0,
                        0);

    SendDlgItemMessageW(hwndDlg,
                        IDC_SECOND_FAILURE,
                        CB_SETCURSEL,
                        0,
                        0);

    SendDlgItemMessageW(hwndDlg,
                        IDC_SUBSEQUENT_FAILURES,
                        CB_SETCURSEL,
                        0,
                        0);

    SendDlgItemMessageW(hwndDlg,
                        IDC_RESET_TIME,
                        WM_SETTEXT,
                        0,
                        (LPARAM)L"0");

    SendDlgItemMessageW(hwndDlg,
                        IDC_RESTART_TIME,
                        WM_SETTEXT,
                        0,
                        (LPARAM)L"1");

    for (id = IDC_RESTART_TEXT1; id <= IDC_RESTART_OPTIONS; id++)
        EnableWindow(GetDlgItem(hwndDlg, id), FALSE);
}


static
BOOL
GetServiceFailure(
    PRECOVERYDATA pRecoveryData)
{
    LPSERVICE_FAILURE_ACTIONS pServiceFailure = NULL;
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bResult = TRUE;
    DWORD cbBytesNeeded = 0;

    hManager = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    hService = OpenService(hManager, pRecoveryData->pService->lpServiceName, SERVICE_QUERY_CONFIG);
    if (hService == NULL)
    {
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceConfig2(hService,
                             SERVICE_CONFIG_FAILURE_ACTIONS,
                             NULL,
                             0,
                             &cbBytesNeeded))
    {
        if (cbBytesNeeded == 0)
        {
            bResult = FALSE;
            goto done;
        }
    }

    pServiceFailure = HeapAlloc(GetProcessHeap(), 0, cbBytesNeeded);
    if (pServiceFailure == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        bResult = FALSE;
        goto done;
    }

    if (!QueryServiceConfig2(hService,
                             SERVICE_CONFIG_FAILURE_ACTIONS,
                             (LPBYTE)pServiceFailure,
                             cbBytesNeeded,
                             &cbBytesNeeded))
    {
        bResult = FALSE;
        goto done;
    }

    pRecoveryData->pServiceFailure = pServiceFailure;

done:
    if (bResult == FALSE && pServiceFailure != NULL)
        HeapFree(GetProcessHeap(), 0, pServiceFailure);

    if (hService)
        CloseServiceHandle(hService);

    if (hManager)
        CloseServiceHandle(hManager);

    return bResult;
}

static
VOID
ShowFailureActions(
    HWND hwndDlg,
    PRECOVERYDATA pRecoveryData)
{
    WCHAR szBuffer[256];
    PWSTR startPtr, endPtr;
    INT i, index, id, length;

    for (i = 0; i < min(pRecoveryData->pServiceFailure->cActions, 3); i++)
    {
        index = -1;

        switch (pRecoveryData->pServiceFailure->lpsaActions[i].Type)
        {
            case SC_ACTION_NONE:
                index = 0;
                break;

            case SC_ACTION_RESTART:
                index = 1;

                wsprintf(szBuffer, L"%lu", pRecoveryData->pServiceFailure->lpsaActions[i].Delay / 60000);
                SendDlgItemMessageW(hwndDlg,
                                    IDC_RESTART_TIME,
                                    WM_SETTEXT,
                                    0,
                                    (LPARAM)szBuffer);

                for (id = IDC_RESTART_TEXT1; id <= IDC_RESTART_TEXT2; id++)
                     EnableWindow(GetDlgItem(hwndDlg, id), TRUE);
                break;

            case SC_ACTION_REBOOT:
                index = 3;

                EnableWindow(GetDlgItem(hwndDlg, IDC_RESTART_OPTIONS), TRUE);
                break;

            case SC_ACTION_RUN_COMMAND:
                index = 2;

                for (id = IDC_RUN_GROUPBOX; id <= IDC_ADD_FAILCOUNT; id++)
                    EnableWindow(GetDlgItem(hwndDlg, id), TRUE);
                break;
        }

        if (index != -1)
        {
            SendDlgItemMessageW(hwndDlg,
                                IDC_FIRST_FAILURE + i,
                                CB_SETCURSEL,
                                index,
                                0);
        }
    }

    wsprintf(szBuffer, L"%lu", pRecoveryData->pServiceFailure->dwResetPeriod / 86400);
    SendDlgItemMessageW(hwndDlg,
                        IDC_RESET_TIME,
                        WM_SETTEXT,
                        0,
                        (LPARAM)szBuffer);

    if (pRecoveryData->pServiceFailure->lpCommand != NULL)
    {
        ZeroMemory(szBuffer, sizeof(szBuffer));

        startPtr = pRecoveryData->pServiceFailure->lpCommand;
        if (*startPtr == L'\"')
            startPtr++;

        endPtr = wcschr(startPtr, L'\"');
        if (endPtr != NULL)
        {
            length = (INT)((LONG_PTR)endPtr - (LONG_PTR)startPtr);
            CopyMemory(szBuffer, startPtr, length);
        }
        else
        {
            wcscpy(szBuffer, startPtr);
        }

        SendDlgItemMessageW(hwndDlg,
                            IDC_PROGRAM,
                            WM_SETTEXT,
                            0,
                            (LPARAM)szBuffer);

        ZeroMemory(szBuffer, sizeof(szBuffer));

        if (endPtr != NULL)
        {
            startPtr = endPtr + 1;
            while (iswspace(*startPtr))
                startPtr++;

            endPtr = wcsstr(pRecoveryData->pServiceFailure->lpCommand, L"/fail=%1%");
            if (endPtr != NULL)
            {
                while (iswspace(*(endPtr - 1)))
                    endPtr--;

                length = (INT)((LONG_PTR)endPtr - (LONG_PTR)startPtr);
                CopyMemory(szBuffer, startPtr, length);
            }
            else
            {
                wcscpy(szBuffer, startPtr);
            }

            SendDlgItemMessageW(hwndDlg,
                                IDC_PARAMETERS,
                                WM_SETTEXT,
                                0,
                                (LPARAM)szBuffer);

            endPtr = wcsstr(pRecoveryData->pServiceFailure->lpCommand, L"/fail=%1%");
            if (endPtr != NULL)
            {
                SendDlgItemMessageW(hwndDlg,
                                    IDC_ADD_FAILCOUNT,
                                    BM_SETCHECK,
                                    BST_CHECKED,
                                    0);
            }
        }
    }
}


static
VOID
UpdateFailureActions(
    HWND hwndDlg,
    PRECOVERYDATA pRecoveryData)
{
    INT id, index;
    BOOL bRestartService = FALSE;
    BOOL bRunProgram = FALSE;
    BOOL bRebootComputer = FALSE;

    for (id = IDC_FIRST_FAILURE; id <= IDC_SUBSEQUENT_FAILURES; id++)
    {
        index = SendDlgItemMessageW(hwndDlg,
                                    id,
                                    CB_GETCURSEL,
                                    0,
                                    0);
        switch (index)
        {
            case 1: /* Restart Service */
                bRestartService = TRUE;
                break;

            case 2: /* Run Program */
                bRunProgram = TRUE;
                break;

            case 3: /* Reboot Computer */
                bRebootComputer = TRUE;
                break;
        }
    }

    for (id = IDC_RESTART_TEXT1; id <= IDC_RESTART_TEXT2; id++)
         EnableWindow(GetDlgItem(hwndDlg, id), bRestartService);

    for (id = IDC_RUN_GROUPBOX; id <= IDC_ADD_FAILCOUNT; id++)
         EnableWindow(GetDlgItem(hwndDlg, id), bRunProgram);

    EnableWindow(GetDlgItem(hwndDlg, IDC_RESTART_OPTIONS), bRebootComputer);
}


static
VOID
BrowseFile(
    HWND hwndDlg)
{
    WCHAR szFile[MAX_PATH] = {'\0'};
    PWCHAR pszFilter = L"Executable Files (*.exe;*.com;*.cmd;*.bat)\0*.exe;*.com;*.cmd;*.bat\0";
    OPENFILENAME ofn;

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ENABLESIZING;
    ofn.hwndOwner = hwndDlg;
    ofn.lpstrFilter = pszFilter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;

    if (GetOpenFileName(&ofn))
    {
        SendDlgItemMessageW(hwndDlg,
                            IDC_PROGRAM,
                            WM_SETTEXT,
                            0,
                            (LPARAM)szFile);
    }
}


static
VOID
SetFailureActions(
    HWND hwndDlg)
{
    SERVICE_FAILURE_ACTIONS FailureActions;
    BOOL bRestartService = FALSE;
    BOOL bRunProgram = FALSE;
    BOOL bRebootComputer = FALSE;
    INT id, index;

    ZeroMemory(&FailureActions, sizeof(FailureActions));

    /* Count the number of valid failure actions */
    for (id = IDC_FIRST_FAILURE; id <= IDC_SUBSEQUENT_FAILURES; id++)
    {
        index = SendDlgItemMessageW(hwndDlg,
                                    id,
                                    CB_GETCURSEL,
                                    0,
                                    0);
        switch (index)
        {
            case 1: /* Restart Service */
                bRestartService = TRUE;
                FailureActions.cActions++;
                break;

            case 2: /* Run Program */
                bRunProgram = TRUE;
                FailureActions.cActions++;
                break;

            case 3: /* Reboot Computer */
                bRebootComputer = TRUE;
                FailureActions.cActions++;
                break;
        }
    }

    if (bRestartService)
    {
        // IDC_RESTART_TIME
    }

    if (bRunProgram)
    {
        // IDC_RESTART_TIME
    }

    if (bRebootComputer)
    {
        // IDC_RESTART_TIME
    }


#if 0
typedef struct _SERVICE_FAILURE_ACTIONS {
  DWORD     dwResetPeriod;
  LPTSTR    lpRebootMsg;
  LPTSTR    lpCommand;
  DWORD     cActions;
  SC_ACTION *lpsaActions;
} SERVICE_FAILURE_ACTIONS, *LPSERVICE_FAILURE_ACTIONS;
#endif
}


INT_PTR
CALLBACK
RecoveryPageProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PRECOVERYDATA pRecoveryData;

    /* Get the window context */
    pRecoveryData = (PRECOVERYDATA)GetWindowLongPtr(hwndDlg,
                                                    GWLP_USERDATA);
    if (pRecoveryData == NULL && uMsg != WM_INITDIALOG)
        return FALSE;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pRecoveryData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RECOVERYDATA));
            if (pRecoveryData != NULL)
            {
                SetWindowLongPtr(hwndDlg,
                                 GWLP_USERDATA,
                                 (LONG_PTR)pRecoveryData);

                pRecoveryData->pService = ((PSERVICEPROPSHEET)(((LPPROPSHEETPAGE)lParam)->lParam))->pService;

                InitRecoveryPage(hwndDlg);

                if (GetServiceFailure(pRecoveryData))
                {
                    ShowFailureActions(hwndDlg, pRecoveryData);
                }
            }
            break;

        case WM_DESTROY:
            if (pRecoveryData != NULL)
            {
                if (pRecoveryData->pServiceFailure != NULL)
                    HeapFree(GetProcessHeap(), 0, pRecoveryData->pServiceFailure);

                HeapFree(GetProcessHeap(), 0, pRecoveryData);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_FIRST_FAILURE:
                case IDC_SECOND_FAILURE:
                case IDC_SUBSEQUENT_FAILURES:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        UpdateFailureActions(hwndDlg, pRecoveryData);
                        pRecoveryData->bChanged = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_RESET_TIME:
                case IDC_RESTART_TIME:
                case IDC_PROGRAM:
                case IDC_PARAMETERS:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        pRecoveryData->bChanged = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_ADD_FAILCOUNT:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        pRecoveryData->bChanged = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_BROWSE_PROGRAM:
                    BrowseFile(hwndDlg);
                    break;

                case IDC_RESTART_OPTIONS:
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                    if (pRecoveryData->bChanged)
                    {
                        SetFailureActions(hwndDlg);
                        pRecoveryData->bChanged = FALSE;
                    }
                    break;
            }
            break;
    }

    return FALSE;
}
