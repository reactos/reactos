/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/appearance.c
 * PURPOSE:         Appearance property page
 * 
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include "desk.h"


static VOID
OnInitDialog(HWND hwndDlg)
{
    TCHAR szBuffer[256];
    UINT i;

    for (i = IDS_ITEM_FIRST; i < IDS_ITEM_LAST; i++)
    {
        LoadString(hApplet, i, szBuffer, 256);
        SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_UI_ITEM, CB_ADDSTRING, 0, (LPARAM)szBuffer);
    }

    SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_UI_ITEM, CB_SETCURSEL, 2, 0);
}


INT_PTR CALLBACK
AppearancePageProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitDialog(hwndDlg);
            break;

        case WM_COMMAND:
            break;

        case WM_USER:
            SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_UI_ITEM, CB_SETCURSEL, lParam, 0);
            break;
    }

    return FALSE;
}

