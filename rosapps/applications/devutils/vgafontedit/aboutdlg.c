/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/vgafontedit/aboutdlg.c
 * PURPOSE:     About dialog
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "precomp.h"

INT_PTR CALLBACK
AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
        case WM_COMMAND:
            if( LOWORD(wParam) == IDCANCEL )
            {
                EndDialog(hwnd, 0);
                return TRUE;
            }
            break;

        case WM_INITDIALOG:
            return TRUE;
    }

    return FALSE;
}
