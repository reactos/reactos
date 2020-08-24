/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Child dialog proc
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "resource.h"
#include "precomp.h"

INT_PTR CALLBACK OptionsPageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
        return TRUE;
    
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CLEAN_PROGRAMS:
            ShellExecuteW(hwnd, NULL, L"control.exe", L"appwiz.cpl", NULL, SW_SHOW);
            break;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}
