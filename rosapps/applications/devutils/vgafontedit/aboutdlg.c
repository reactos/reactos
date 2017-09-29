/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     About dialog
 * COPYRIGHT:   Copyright 2008 Colin Finck (colin@reactos.org)
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
