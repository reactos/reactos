/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/rapps/aboutdlg.cpp
 * PURPOSE:     About Dialog
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev      (dmitry@reactos.org)
 *              Copyright 2017 Alexander Shaposhikov (sanchaez@reactos.org)
 */
#include "rapps.h"

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
