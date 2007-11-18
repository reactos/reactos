/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/hardprof.c
 * PURPOSE:     Modify hardware profiles
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

/* Property page dialog callback */
static INT_PTR CALLBACK
RenameProfDlgProc(HWND hwndDlg,
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
            MessageBox(hwndDlg, _T("Dialog not yet implemented!"), NULL, 0);
            break;

        case WM_COMMAND:
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                EndDialog(hwndDlg,
                          LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}


/* Property page dialog callback */
INT_PTR CALLBACK
HardProfDlgProc(HWND hwndDlg,
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
		{
		    SendMessage(GetDlgItem(hwndDlg, IDC_HRDPROFUP),
			            BM_SETIMAGE,(WPARAM)IMAGE_ICON,
						(LPARAM)(HANDLE)LoadIcon(hApplet, MAKEINTRESOURCE(IDI_UP)));
		    SendMessage(GetDlgItem(hwndDlg, IDC_HRDPROFDWN),
			            BM_SETIMAGE,(WPARAM)IMAGE_ICON,
						(LPARAM)(HANDLE)LoadIcon(hApplet, MAKEINTRESOURCE(IDI_DOWN)));
            MessageBox(hwndDlg, _T("Dialog not yet implemented!"), NULL, 0);
		}
        break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_HRDPROFRENAME:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_RENAMEPROFILE),
                              hwndDlg,
                              (DLGPROC)RenameProfDlgProc);
                    break;

                case IDOK:
                case IDCANCEL:
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;
            }
            break;
    }

    return FALSE;
}
