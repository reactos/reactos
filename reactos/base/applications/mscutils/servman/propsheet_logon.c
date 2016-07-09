/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/propsheet_logon.c
 * PURPOSE:     Logon property page
 * COPYRIGHT:   Eric Kohl
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef struct _LOGONDATA
{
    ENUM_SERVICE_STATUS_PROCESS *pService;
    LPQUERY_SERVICE_CONFIG pServiceConfig;
    WCHAR szAccountName[64];
    WCHAR szPassword1[64];
    WCHAR szPassword2[64];
    BOOL bInitial;
    BOOL bAccountNameChanged;
} LOGONDATA, *PLOGONDATA;

static
VOID
SetControlStates(
    HWND hwndDlg,
    PLOGONDATA pLogonData,
    BOOL bLocalSystem)
{
    BOOL y = bLocalSystem ? FALSE : TRUE;

    EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON_INTERACTIVE), bLocalSystem);
    EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON_ACCOUNTNAME), y);
    EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON_SEARCH), y);
    EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON_PW1TEXT), y);
    EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON_PASSWORD1), y);
    EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON_PW2TEXT), y);
    EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON_PASSWORD2), y);

    if (bLocalSystem == TRUE && pLogonData->bInitial == FALSE)
    {
        GetDlgItemText(hwndDlg, IDC_LOGON_ACCOUNTNAME, pLogonData->szAccountName, 64);
        GetDlgItemText(hwndDlg, IDC_LOGON_PASSWORD1, pLogonData->szPassword1, 64);
        GetDlgItemText(hwndDlg, IDC_LOGON_PASSWORD2, pLogonData->szPassword2, 64);
    }

    SetDlgItemText(hwndDlg, IDC_LOGON_ACCOUNTNAME, bLocalSystem ? L"" : pLogonData->szAccountName);
    SetDlgItemText(hwndDlg, IDC_LOGON_PASSWORD1, bLocalSystem ? L"" : pLogonData->szPassword1);
    SetDlgItemText(hwndDlg, IDC_LOGON_PASSWORD2, bLocalSystem ? L"" : pLogonData->szPassword2);

    pLogonData->bInitial = FALSE;
}


/*
 * Logon Property dialog callback.
 * Controls messages to the Logon dialog
 */
INT_PTR
CALLBACK
LogonPageProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PLOGONDATA pLogonData;

    /* Get the window context */
    pLogonData = (PLOGONDATA)GetWindowLongPtr(hwndDlg,
                                              GWLP_USERDATA);
    if (pLogonData == NULL && uMsg != WM_INITDIALOG)
    {
        return FALSE;
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pLogonData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LOGONDATA));
            if (pLogonData != NULL)
            {
                SetWindowLongPtr(hwndDlg,
                                 GWLP_USERDATA,
                                 (LONG_PTR)pLogonData);

                pLogonData->bInitial = TRUE;
                pLogonData->pService = ((PSERVICEPROPSHEET)(((LPPROPSHEETPAGE)lParam)->lParam))->pService;

                pLogonData->pServiceConfig = GetServiceConfig(pLogonData->pService->lpServiceName);
                if (pLogonData->pServiceConfig != NULL)
                {
                    if (pLogonData->pServiceConfig->lpServiceStartName == NULL ||
                        _wcsicmp(pLogonData->pServiceConfig->lpServiceStartName, L"LocalSystem") == 0)
                    {
                        PostMessageW(GetDlgItem(hwndDlg, IDC_LOGON_SYSTEMACCOUNT), BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                        PostMessageW(hwndDlg, WM_COMMAND, IDC_LOGON_SYSTEMACCOUNT, 0);
                    }
                    else
                    {
                        wcscpy(pLogonData->szAccountName, pLogonData->pServiceConfig->lpServiceStartName);
                        PostMessageW(GetDlgItem(hwndDlg, IDC_LOGON_THISACCOUNT), BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                        PostMessageW(hwndDlg, WM_COMMAND, IDC_LOGON_THISACCOUNT, 0);
                    }
                }
            }

            EnableWindow(GetDlgItem(hwndDlg, IDC_LOGON_HWPROFILE), FALSE);
            break;

        case WM_DESTROY:
            if (pLogonData->pServiceConfig)
                HeapFree(GetProcessHeap(), 0, pLogonData->pServiceConfig);

            HeapFree(GetProcessHeap(), 0, pLogonData);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_LOGON_SYSTEMACCOUNT:
                    SetControlStates(hwndDlg, pLogonData, TRUE);
                    break;

                case IDC_LOGON_THISACCOUNT:
                    SetControlStates(hwndDlg, pLogonData, FALSE);
                    break;

                case IDC_LOGON_ACCOUNTNAME:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        pLogonData->bAccountNameChanged = TRUE;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;


            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                    break;
            }
            break;
    }

    return FALSE;
}
