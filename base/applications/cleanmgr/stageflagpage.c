/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         StageFlagPage child dialog function
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "precomp.h"

INT_PTR CALLBACK SetStageFlagPageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hList = GetDlgItem(hwnd, IDC_SAGESET_LIST);

    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            InitStageFlagListViewControl(hList);
            return TRUE;

        case WM_NOTIFY:
        {
            BOOL IsItemChecked = FALSE;
            HWND hParentDialog = GetParent(hwnd);
            HWND hButton = GetDlgItem(hParentDialog, IDOK);
            NMHDR* header = (NMHDR*)lParam;
            NMLISTVIEW* NmList = (NMLISTVIEW*)lParam;
            int NumOfItems = ListView_GetItemCount(hList);
            LVITEMW lvI;
            ZeroMemory(&lvI, sizeof(lvI));

            if (lParam)
            {
                if (header && header->idFrom == IDC_SAGESET_LIST && header->code == LVN_ITEMCHANGED)
                {
                    if ((NmList->uNewState ^ NmList->uOldState) & LVIS_SELECTED)
                    {
                        SelItem(hwnd, NmList->iItem);
                    }
                    else if ((NmList->uNewState ^ NmList->uOldState) & LVIS_STATEIMAGEMASK)
                    {
                        StageFlagCheckedItem(NmList->iItem, hwnd, GetDlgItem(hwnd, IDC_SAGESET_LIST));
                    }
                }
            }
            for (int i = 0; i < NumOfItems; i++)
            {
                if (ListView_GetCheckState(hList, i))
                {
                    IsItemChecked = TRUE;
                    break;
                }
            }
            EnableWindow(hButton, IsItemChecked);
            break;
        }

        default:
            return FALSE;
    }
    return TRUE;
}
