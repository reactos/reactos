/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/general.c
 * PURPOSE:     Licence dialog box message handler
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"


INT_PTR CALLBACK
LicenceDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    HRSRC hResInfo;
    HGLOBAL hResMem;
    WCHAR *LicenseText;
    static HICON hIcon;

    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hIcon = LoadImage(hApplet,
                              MAKEINTRESOURCE(IDI_CPLSYSTEM),
                              IMAGE_ICON,
                              16,
                              16,
                              0);

            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hIcon);

            /* load license from resource */
            if(!(hResInfo = FindResource(hApplet,
                                         MAKEINTRESOURCE(RC_LICENSE),
                                         MAKEINTRESOURCE(RTDATA))) ||
               !(hResMem = LoadResource(hApplet, hResInfo)) ||
               !(LicenseText = LockResource(hResMem)))
            {
                ShowLastWin32Error(hDlg);
                break;
            }

            /* insert the license into the edit control (unicode!) */
            SetDlgItemText(hDlg,
                           IDC_LICENCEEDIT,
                           LicenseText);

            SendDlgItemMessage(hDlg,
                               IDC_LICENCEEDIT,
                               EM_SETSEL,
                               -1,
                               0);

            return TRUE;
        }

        case WM_COMMAND:
        {
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                DestroyIcon(hIcon);
                EndDialog(hDlg,
                          LOWORD(wParam));
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
