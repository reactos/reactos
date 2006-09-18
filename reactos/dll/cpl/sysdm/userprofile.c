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
            HWND hLink = GetDlgItem(hwndDlg, IDC_USERACCOUNT_LINK);

            TextToLink(hLink, 
                       _T("rundll32.exe"),
                       _T("shell32.dll, Control_RunDLL nusrmgr.cpl"));

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
  }
  return FALSE;
}
