/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/generalpage.c
 * PURPOSE:     General page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include <precomp.h>

HWND hGeneralPage;
HWND hGeneralDialog;

INT_PTR CALLBACK
GeneralPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (message) {
    case WM_INITDIALOG:
        hGeneralDialog = hDlg;
        SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
        return TRUE;
    }
    return 0;
}
