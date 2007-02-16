/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/servman/progress.c
 * PURPOSE:     Progress dialog box message handler
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

BOOL CALLBACK ProgressDialogProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:

        break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    DestroyWindow(hDlg);
                break;

            }
        break;

        case WM_DESTROY:
            DestroyWindow(hDlg);
        break;

        default:
            return FALSE;
    }

    return TRUE;

}

