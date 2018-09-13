//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       fopendlg.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "resource.h"
#include "fopendlg.h"

INT_PTR CALLBACK 
OpenFilesWarningProc(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hDlg,
                               IDC_DLGTYPEICON,
                               STM_SETICON,
                               (WPARAM)LoadIcon(NULL, IDI_WARNING),
                               0L);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
            }
            break;

        default:
            break;
    }
    return FALSE;
}


int
OpenFilesWarningDialog(
    HWND hwndParent
    )
{
    return (int)DialogBox(g_hInstance, 
                          MAKEINTRESOURCE(IDD_OPEN_FILES_WARNING), 
                          hwndParent, 
                          OpenFilesWarningProc);
}
