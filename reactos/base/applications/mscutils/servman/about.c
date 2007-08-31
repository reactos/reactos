/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/about.c
 * PURPOSE:     About dialog box message handler
 * COPYRIGHT:   Copyright 2005-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

BOOL CALLBACK
AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND  hLicenseEditWnd;
    HICON hIcon = NULL;
    TCHAR strLicense[700];

    switch (message)
    {
    case WM_INITDIALOG:

        hIcon = (HICON)LoadImage(hInstance,
                                 MAKEINTRESOURCE(IDI_SM_ICON),
                                 IMAGE_ICON,
                                 16,
                                 16,
                                 0);
        if (hIcon)
        {
            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hIcon);
            DestroyIcon(hIcon);
        }

        hLicenseEditWnd = GetDlgItem(hDlg,
                                     IDC_LICENSE_EDIT);
        if (hLicenseEditWnd)
        {
            LoadString(hInstance,
                       IDS_LICENSE,
                       strLicense,
                       sizeof(strLicense) / sizeof(TCHAR));

            SetWindowText(hLicenseEditWnd,
                          strLicense);
        }

        return TRUE;

    case WM_COMMAND:

        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
        {
            EndDialog(hDlg,
                      LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return FALSE;
}
