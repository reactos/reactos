/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/propsheet.c
 * PURPOSE:     Property dialog box message handler
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"


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
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USECALLBACK;// | PSH_MODELESS;
    psh.hwndParent = Info->hMainWnd;
    psh.hInstance = hInstance;
    psh.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SM_ICON));
    psh.pszCaption = Info->pCurrentService->lpDisplayName;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
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
        InitPropSheetPage(&psp[1], pServicePropSheet, IDD_DLG_DEPEND, DependenciesPageProc);

        Ret = (LONG)(PropertySheet(&psh) != -1);

        HeapFree(ProcessHeap,
                 0,
                 pServicePropSheet);
    }

    return Ret;
}

