/*
 * PROJECT:     ReactOS Character Map
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/charmap/about.c
 * PURPOSE:     about dialog
 * COPYRIGHT:   Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */


#include <precomp.h>


static INT_PTR CALLBACK
AboutDialogProc(HWND hDlg,
                UINT message,
                WPARAM wParam,
                LPARAM lParam)
{
    static HICON hIcon = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            HWND  hLicenseEditWnd;
            WCHAR strLicense[700];

            hIcon = LoadImageW(hInstance,
                               MAKEINTRESOURCEW(IDI_ICON),
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
            }

            hLicenseEditWnd = GetDlgItem(hDlg,
                                         IDC_LICENSE_EDIT);

            if (LoadStringW(hInstance,
                            IDS_LICENSE,
                            strLicense,
                            sizeof(strLicense) / sizeof(WCHAR)))
            {
                SetWindowTextW(hLicenseEditWnd,
                               strLicense);
            }

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

            break;
        }
    }

    return FALSE;
}


VOID
ShowAboutDlg(HWND hWndParent)
{
    DialogBoxW(hInstance,
               MAKEINTRESOURCEW(IDD_ABOUTBOX),
               hWndParent,
               AboutDialogProc);
}
