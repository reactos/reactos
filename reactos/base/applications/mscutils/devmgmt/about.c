/*
 * PROJECT:     ReactOS Device Managment
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/devmgmt/about.c
 * PURPOSE:     About dialog box message handler
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

INT_PTR CALLBACK
AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND  hLicenseEditWnd;
    HICON hIcon = NULL;
    TCHAR strLicense[700];

    switch (message)
    {
    case WM_INITDIALOG:

        hIcon = (HICON)LoadImage(hInstance,
                                 MAKEINTRESOURCE(IDI_MAIN_ICON),
                                 IMAGE_ICON,
                                 16,
                                 16,
                                 0);

        SendMessage(hDlg,
                    WM_SETICON,
                    ICON_SMALL,
                    (LPARAM)hIcon);

        hLicenseEditWnd = GetDlgItem(hDlg,
                                     IDC_LICENSE_EDIT);

        LoadString(hInstance,
                   IDS_LICENSE,
                   strLicense,
                   sizeof(strLicense) / sizeof(TCHAR));

        SetWindowText(hLicenseEditWnd,
                      strLicense);

        return TRUE;

    case WM_COMMAND:

        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
        {
            DestroyIcon(hIcon);
            EndDialog(hDlg,
                      LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return FALSE;
}
