/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps_new/aboutdlg.cpp
 * PURPOSE:         About Dialog
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

static
INT_PTR CALLBACK
AboutDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hDlg, LOWORD(wParam));
                    break;
            }
        }
        break;
    }

    return FALSE;
}

VOID
ShowAboutDialog(VOID)
{
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_ABOUT_DIALOG),
              hMainWnd,
              AboutDlgProc);
}
