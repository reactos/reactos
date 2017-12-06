/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/propsheet.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2006-2017 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

unsigned int __stdcall PropSheetThread(void* Param);

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

VOID
OpenPropSheet(PMAIN_WND_INFO Info)
{
    PSERVICEPROPSHEET pServicePropSheet;
    HANDLE hThread;

    pServicePropSheet = HeapAlloc(ProcessHeap,
                                  0,
                                  sizeof(*pServicePropSheet));
    if (!pServicePropSheet) return;

    /* Set the current service in this calling thread to avoid
     * it being updated before the thread is up */
    pServicePropSheet->pService = Info->pCurrentService;
    pServicePropSheet->Info = Info;

    hThread = (HANDLE)_beginthreadex(NULL, 0, &PropSheetThread, pServicePropSheet, 0, NULL);
    if (hThread)
    {
        CloseHandle(hThread);
    }
}


unsigned int __stdcall PropSheetThread(void* Param)
{
    PSERVICEPROPSHEET pServicePropSheet;
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[4];
    HWND hDlg = NULL;
    MSG Msg;

    pServicePropSheet = (PSERVICEPROPSHEET)Param;

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_MODELESS;
    psh.hwndParent = pServicePropSheet->Info->hMainWnd;
    psh.hInstance = hInstance;
    psh.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SM_ICON));
    psh.pszCaption = pServicePropSheet->Info->pCurrentService->lpDisplayName;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    /* Initialize the tabs */
    InitPropSheetPage(&psp[0], pServicePropSheet, IDD_DLG_GENERAL, GeneralPageProc);
    InitPropSheetPage(&psp[1], pServicePropSheet, IDD_LOGON, LogonPageProc);
    InitPropSheetPage(&psp[2], pServicePropSheet, IDD_RECOVERY, RecoveryPageProc);
    InitPropSheetPage(&psp[3], pServicePropSheet, IDD_DLG_DEPEND, DependenciesPageProc);

    hDlg = (HWND)PropertySheetW(&psh);
    if (hDlg)
    {
        /* Pump the message queue */
        while (GetMessageW(&Msg, NULL, 0, 0))
        {
            if (!PropSheet_GetCurrentPageHwnd(hDlg))
            {
                /* The user hit the ok / cancel button, pull it down */
                EnableWindow(pServicePropSheet->Info->hMainWnd, TRUE);
                DestroyWindow(hDlg);
            }

            if (!PropSheet_IsDialogMessage(hDlg, &Msg))
            {
                TranslateMessage(&Msg);
                DispatchMessageW(&Msg);
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, pServicePropSheet);

    return (hDlg != NULL);
}

