/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Child dialog proc
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "precomp.h"

INT_PTR CALLBACK SagesetPageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
        InitSagesetListViewControl(GetDlgItem(hwnd, IDC_SAGESET_LIST));
        return TRUE;

    case WM_NOTIFY:
    {
        NMHDR* Header = (NMHDR*)lParam;
        NMLISTVIEW* NmList = (NMLISTVIEW*)lParam;
        LVITEMW lvI;
        ZeroMemory(&lvI, sizeof(lvI));

        if (lParam)
        {
            /* FIX ME: A bug in comctl32 causes selected item to send LVIS_STATEIMAGEMASK */
            if (Header && Header->idFrom == IDC_SAGESET_LIST && Header->code == LVN_ITEMCHANGED)
            {
                if ((NmList->uNewState ^ NmList->uOldState) & LVIS_SELECTED)
                {
                    SelItem(hwnd, NmList->iItem);
                }

                else if ((NmList->uNewState ^ NmList->uOldState) & LVIS_STATEIMAGEMASK)
                {
                    SagesetCheckedItem(NmList->iItem, hwnd, GetDlgItem(hwnd, IDC_SAGESET_LIST));
                }
            }
        }
        break;
    }

    default:
        return FALSE;
    }
    return TRUE;
}
