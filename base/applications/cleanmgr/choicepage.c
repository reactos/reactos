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
    WCHAR StringText[MAX_PATH] = { 0 };
    WCHAR FullText[MAX_PATH] = { 0 };
    WCHAR TotalAmount[MAX_PATH] = { 0 };
    static WCHAR* ViewFolder = NULL;
    static HWND hList = 0;
    uint64_t TotalSize = sz.TempASize + sz.TempBSize + sz.RecycleBinSize + sz.ChkDskSize + sz.RappsSize;
    
    SHELLEXECUTEINFOW seI;
    ZeroMemory(&seI, sizeof(seI));
    seI.cbSize = sizeof seI;
    seI.lpVerb = L"open";
    seI.nShow = SW_SHOW;
    
    ViewFolder = wcv.RappsDir;

    switch (message)
    {
    case WM_INITDIALOG:
        SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
        hList = GetDlgItem(hwnd, IDC_CHOICE_LIST);
        InitListViewControl(hList);
        LoadStringW(GetModuleHandleW(NULL), IDS_CLEANUP, StringText, _countof(StringText));
        StringCchPrintfW(TotalAmount, _countof(TotalAmount), L"%.02lf %s", SetOptimalSize(TotalSize), SetOptimalUnit(TotalSize));
        StringCchPrintfW(FullText, _countof(FullText), StringText, TotalAmount, wcv.DriveLetter);
        SetDlgItemTextW(hwnd, IDC_STATIC_DLG, FullText);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_VIEW_FILES:
            if (!PathIsDirectoryW(ViewFolder))
            {
                ZeroMemory(&StringText, sizeof(StringText));
                LoadStringW(GetModuleHandleW(NULL), IDS_WARNING_FOLDER, StringText, _countof(StringText));
                MessageBoxW(hwnd, StringText, L"Warning", MB_OK | MB_ICONWARNING);
                break;
            }

            seI.lpFile = ViewFolder;
            if (!ShellExecuteExW(&seI))
            {
                MessageBoxW(NULL, L"ShellExecuteExW() failed!", L"Ok", MB_OK | MB_ICONSTOP);
                return FALSE;
            }
            break;
        }
        break;

    case WM_NOTIFY:
    {
        NMHDR* Header = (NMHDR*)lParam;
        NMLISTVIEW* NmList = (NMLISTVIEW*)lParam;
        LVITEMW lvI;
        ZeroMemory(&lvI, sizeof(lvI));
        static long long size = 0;

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
                    size = CheckedItem(NmList->iItem, hwnd, hList, size);
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
