/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/advanced.c
 * PURPOSE:     Memory, start-up and profiles settings
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
                Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

static TCHAR BugLink[] = _T("http://www.reactos.org/bugzilla");

/* Property page dialog callback */
INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_PERFOR:
                {
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_VIRTMEM),
                              hwndDlg,
                              (DLGPROC)VirtMemDlgProc);
                }
                break;

                case IDC_USERPROFILE:
                {
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_USERPROFILE),
                              hwndDlg,
                              (DLGPROC)UserProfileDlgProc);
                }
                break;

                case IDC_STAREC:
                {
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_STARTUPRECOVERY),
                              hwndDlg,
                              (DLGPROC)StartRecDlgProc);
                }
                break;

                case IDC_ENVVAR:
                {
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_ENVIRONMENT_VARIABLES),
                              hwndDlg,
                              (DLGPROC)EnvironmentDlgProc);
                }
                break;

                case IDC_ERRORREPORT:
                {
                    ShellExecute(NULL,
                                 _T("open"),
                                 BugLink,
                                 NULL,
                                 NULL,
                                 SW_SHOWNORMAL);
                }
                break;

            }
        }
      break;

    }
    return FALSE;
}
