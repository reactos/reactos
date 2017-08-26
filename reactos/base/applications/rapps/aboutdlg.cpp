/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/aboutdlg.cpp
 * PURPOSE:         About Dialog
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Alexander Shaposhikov (chaez.san@gmail.com)
 */
#include "defines.h"

static INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_COMMAND && LOWORD(wParam) == IDOK)
    {
        return EndDialog(hDlg, LOWORD(wParam));
    }

    return FALSE;
}

VOID ShowAboutDialog()
{
    DialogBoxW(hInst,
               MAKEINTRESOURCEW(IDD_ABOUT_DIALOG),
               hMainWnd,
               AboutDlgProc);
}
