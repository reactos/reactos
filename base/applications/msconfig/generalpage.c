/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/generalpage.c
 * PURPOSE:     General page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include "precomp.h"

HWND hGeneralPage;
HWND hGeneralDialog;

VOID
EnableCheckboxControls(HWND hDlg, BOOL bEnable)
{
    EnableWindow(GetDlgItem(hDlg, IDC_CBX_SYSTEM_INI), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_CBX_SYSTEM_SERVICE), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_CBX_STARTUP_ITEM), bEnable);
}


INT_PTR CALLBACK
GeneralPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        hGeneralDialog = hDlg;
        SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
        /* FIXME */
        SendDlgItemMessage(hDlg, IDC_CBX_NORMAL_START, BM_SETCHECK, BST_CHECKED, 0);
        EnableCheckboxControls(hDlg, FALSE);
        return TRUE;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
            case IDC_CBX_NORMAL_START:
            case IDC_CBX_DIAGNOSTIC_START:
                EnableCheckboxControls(hDlg, FALSE);
                break;
            case IDC_CBX_SELECTIVE_STARTUP:
                EnableCheckboxControls(hDlg, TRUE);
                break;
            default:
                break;
        }
    }
    return 0;
}
