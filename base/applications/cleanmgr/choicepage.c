/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Child dialog proc
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "resource.h"
#include "precomp.h"

INT_PTR CALLBACK ChoicePageDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        WCHAR FullText[ARR_MAX_SIZE] = { 0 };
        WCHAR TotalAmount[ARR_MAX_SIZE] = { 0 };
        WCHAR TempText[ARR_MAX_SIZE] = { 0 };
        uint64_t TotalSize = sz.TempASize + sz.TempBSize + sz.RecycleBinSize + sz.ChkDskSize + sz.RappsSize;

        SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
        InitListViewControl(GetDlgItem(hwnd, IDC_CHOICE_LIST));
        LoadStringW(GetModuleHandleW(NULL), IDS_CLEANUP, TempText, _countof(TempText));
        StrFormatByteSizeW(TotalSize, TotalAmount, _countof(TotalAmount));
        StringCchPrintfW(FullText, _countof(FullText), TempText, TotalAmount, wcv.DriveLetter);
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
            seI.lpFile = wcv.RappsDir;
            
            if (!PathIsDirectoryW(wcv.RappsDir))
            {;
                LoadStringW(GetModuleHandleW(NULL), IDS_WARNING_FOLDER, LoadErrorString, _countof(LoadErrorString));
                MessageBoxW(hwnd, LoadErrorString, L"Warning", MB_OK | MB_ICONWARNING);
                break;
            }

            if (!ShellExecuteExW(&seI))
            {
                MessageBoxW(NULL, L"ShellExecuteExW() failed!", L"Ok", MB_OK | MB_ICONSTOP);
                return FALSE;
            }
            break;
        }
        }
        break;

    case WM_NOTIFY:
    {
        NMHDR* Header = (NMHDR*)lParam;
        NMLISTVIEW* NmList = (NMLISTVIEW*)lParam;
        LVITEMW lvI;
        ZeroMemory(&lvI, sizeof(lvI));

        if (lParam)
        {
            /* FIX ME: A bug in comctl32 causes selected item to send LVIS_STATEIMAGEMASK */
            if (Header && Header->idFrom == IDC_CHOICE_LIST && Header->code == LVN_ITEMCHANGED)
            {
                if (NmList->uNewState & LVIS_SELECTED)
                {
                    SelItem(hwnd, NmList->iItem);
                }

                else if (NmList->uNewState & LVIS_STATEIMAGEMASK)
                {
                    sz.CountSize = CheckedItem(NmList->iItem, hwnd, GetDlgItem(hwnd, IDC_CHOICE_LIST), sz.CountSize);
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
