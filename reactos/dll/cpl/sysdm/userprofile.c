/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/userprofile.c
 * PURPOSE:     Computer settings for networking
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

/* Property page dialog callback */
INT_PTR CALLBACK
UserProfileDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            MessageBox(hwndDlg, _T("Dialog not yet implemented!"), NULL, 0);
        }
        break;

        case WM_COMMAND:
        {
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                EndDialog(hwndDlg,
                          LOWORD(wParam));
                return TRUE;
            }
        }
        break;

        case WM_NOTIFY:
        {
            NMHDR *nmhdr = (NMHDR *)lParam;

            if (nmhdr->idFrom == IDC_USERACCOUNT_LINK && nmhdr->code == NM_CLICK)
            {
                ShellExecute(hwndDlg,
                             TEXT("open"),
                             TEXT("rundll32.exe"),
                             TEXT("shell32.dll, Control_RunDLL nusrmgr.cpl"),
                             NULL,
                             SW_SHOWNORMAL);
            }
            break;
        }
    }
    return FALSE;
}
