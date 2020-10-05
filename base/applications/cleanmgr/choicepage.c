/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         ChoicePage child dialog proc
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "precomp.h"

DIRSIZE DirectorySizes;
WCHAR DriveLetter[ARR_MAX_SIZE];
WCHAR RappsDir[MAX_PATH];

INT_PTR CALLBACK ChoicePageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hList = GetDlgItem(hwnd, IDC_CHOICE_LIST);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            WCHAR FullText[ARR_MAX_SIZE] = { 0 };
            WCHAR TotalAmount[ARR_MAX_SIZE] = { 0 };
            WCHAR TempText[ARR_MAX_SIZE] = { 0 };
            uint64_t TotalSize = DirectorySizes.TempSize + DirectorySizes.RecycleBinSize + DirectorySizes.ChkDskSize + DirectorySizes.RappsSize;

            SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            InitListViewControl(hList);
            LoadStringW(GetModuleHandleW(NULL), IDS_CLEANUP, TempText, _countof(TempText));
            StrFormatByteSizeW(TotalSize, TotalAmount, sizeof(TotalAmount));
            StringCchPrintfW(FullText, sizeof(FullText), TempText, TotalAmount, DriveLetter);
            SetDlgItemTextW(hwnd, IDC_STATIC_DLG, FullText);
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_VIEW_FILES:
                {
                    WCHAR LoadErrorString[ARR_MAX_SIZE] = { 0 };
                    SHELLEXECUTEINFOW seI;
                    ZeroMemory(&seI, sizeof(seI));
                    seI.cbSize = sizeof(seI);
                    seI.lpVerb = L"open";
                    seI.nShow = SW_SHOW;
                    seI.lpFile = RappsDir;
            
                    if (!PathIsDirectoryW(RappsDir))
                    {
                        LoadStringW(GetModuleHandleW(NULL), IDS_WARNING_FOLDER, LoadErrorString, sizeof(LoadErrorString));
                        MessageBoxW(hwnd, LoadErrorString, L"Warning", MB_OK | MB_ICONWARNING);
                        break;
                    }

                    if (!ShellExecuteExW(&seI))
                    {
                        DPRINT("ShellExecuteExW(): Failed to perform the operation!\n");
                        return FALSE;
                    }
                    break;
                }
            }
            break;

        case WM_NOTIFY:
        {
            HWND hParentDialog = GetParent(hwnd);
            HWND hButton = GetDlgItem(hParentDialog, IDOK);
            NMHDR* header = (NMHDR*)lParam;
            NMLISTVIEW* NmList = (NMLISTVIEW*)lParam;
            LVITEMW lvI;
            ZeroMemory(&lvI, sizeof(lvI));

            if (lParam)
            {
                if (header && header->idFrom == IDC_CHOICE_LIST && header->code == LVN_ITEMCHANGED)
                {
                    if ((NmList->uNewState ^ NmList->uOldState) & LVIS_SELECTED)
                    {
                        SelItem(hwnd, NmList->iItem);
                    }
                    else if ((NmList->uNewState ^ NmList->uOldState) & LVIS_STATEIMAGEMASK)
                    {
                        DirectorySizes.CountSize = CheckedItem(NmList->iItem, hwnd, hList, DirectorySizes.CountSize);
                    }
                }
            }

            if (DirectorySizes.CountSize == 0)
            {
                EnableWindow(hButton, FALSE);
                break;
            }
            EnableWindow(hButton, TRUE);
            break;
        }

        default:
            return FALSE;
    }
    return TRUE;
}
