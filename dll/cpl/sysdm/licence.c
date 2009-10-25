/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/general.c
 * PURPOSE:     Licence dialog box message handler
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

typedef struct _LIC_CONTEXT
{
    HICON hIcon;
} LIC_CONTEXT, *PLIC_CONTEXT;


static BOOL
OnInitDialog(HWND hDlg, PLIC_CONTEXT pLicInfo)
{
    HRSRC hResInfo;
    HGLOBAL hResMem;
    WCHAR *LicenseText;

    pLicInfo->hIcon = LoadImage(hApplet,
                                MAKEINTRESOURCE(IDI_CPLSYSTEM),
                                IMAGE_ICON,
                                16,
                                16,
                                0);

    SendMessage(hDlg,
                WM_SETICON,
                ICON_SMALL,
                (LPARAM)pLicInfo->hIcon);

    /* load license from resource */
    if (!(hResInfo = FindResource(hApplet,
                                  MAKEINTRESOURCE(RC_LICENSE),
                                  MAKEINTRESOURCE(RTDATA))) ||
        !(hResMem = LoadResource(hApplet, hResInfo)) ||
        !(LicenseText = LockResource(hResMem)))
    {
        ShowLastWin32Error(hDlg);
        return FALSE;
    }

    /* insert the license into the edit control (unicode!) */
    SetDlgItemText(hDlg,
                   IDC_LICENCEEDIT,
                   LicenseText);

    PostMessage(GetDlgItem(hDlg, IDC_LICENCEEDIT),
                EM_SETSEL,
                -1,
                0);

    return TRUE;
}


INT_PTR CALLBACK
LicenceDlgProc(HWND hDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PLIC_CONTEXT pLicInfo;

    UNREFERENCED_PARAMETER(lParam);

    pLicInfo = (PLIC_CONTEXT)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pLicInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LIC_CONTEXT));
            if (pLicInfo == NULL)
            {
                EndDialog(hDlg, 0);
                return FALSE;
            }
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pLicInfo);
            return OnInitDialog(hDlg, pLicInfo);

        case WM_DESTROY:
            if (pLicInfo)
            {
                DestroyIcon(pLicInfo->hIcon);
                HeapFree(GetProcessHeap(), 0, pLicInfo);
            }
            break;

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
