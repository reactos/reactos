/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Child dialog proc
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */

#include "resource.h"
#include "precomp.h"

DIRSIZE sz;
WCHAR_VAR wcv;
BOOL_VAR bv;

INT_PTR CALLBACK ChoicePageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    WCHAR TempList[MAX_PATH] = { 0 };
    WCHAR StringText[MAX_PATH] = { 0 };
    WCHAR FullText[MAX_PATH] = { 0 };
    WCHAR totalAmount[MAX_PATH] = { 0 };
    static WCHAR* ViewFolder = NULL;
    uint64_t TempSize = sz.TempASize + sz.TempBSize;
    uint64_t RecycleSize = sz.RecycleBinSize;
    uint64_t ChkDskSize = sz.ChkDskSize;
    uint64_t RappsSize = sz.RappsSize;
    static HWND hList = 0;
    
    LVCOLUMNW lvC;
    
    SHELLEXECUTEINFOW seI;
    
    ViewFolder = wcv.RappsDir;

    switch (Message)
    {
    case WM_INITDIALOG:
        SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
        hList = GetDlgItem(hwnd, IDC_CHOICE_LIST);
        
        if(hList == NULL)
        {
            MessageBoxW(NULL, L"GetDlgItem() failed!", L"Error", MB_OK | MB_ICONERROR);
        }
        
        SendMessageW(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
        
        ZeroMemory(&lvC, sizeof(lvC));
        lvC.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        lvC.cx = 158;
        lvC.cchTextMax = 256;
        lvC.fmt = LVCFMT_RIGHT;
        
        ListView_InsertColumn(hList, 0, &lvC);
        ListView_InsertColumn(hList, 1, &lvC);

        if (!CreateImageLists(hList))
        {
            MessageBoxW(NULL, L"CreateImageLists() failed!", L"Error", MB_OK | MB_ICONERROR);
        }

        StringCchPrintfW(TempList, _countof(TempList), L"%.02lf %s", SetOptimalSize(ChkDskSize), SetOptimalUnit(ChkDskSize));
        AddItem(hList, L"Old ChkDsk Files", TempList, 0);
        ZeroMemory(&TempList, sizeof(TempList));

        StringCchPrintfW(TempList, _countof(TempList), L"%.02lf %s", SetOptimalSize(RappsSize), SetOptimalUnit(RappsSize));
        AddItem(hList, L"RAPPS Files", TempList, 0);
        ZeroMemory(&TempList, sizeof(TempList));
        
        StringCchPrintfW(TempList, _countof(TempList), L"%.02lf %s", SetOptimalSize(RecycleSize), SetOptimalUnit(RecycleSize));
        AddItem(hList, L"Recycle Bin", TempList, 2);
        ZeroMemory(&TempList, sizeof(TempList));
        
        if (bv.SysDrive == TRUE)
        {
            StringCchPrintfW(TempList, _countof(TempList), L"%.02lf %s", SetOptimalSize(TempSize), SetOptimalUnit(TempSize));
            AddItem(hList, L"Temporary Files", TempList, 0);
        }

        LoadStringW(GetModuleHandleW(NULL), IDS_CLEANUP, StringText, _countof(StringText));
        StringCchPrintfW(totalAmount, _countof(totalAmount), L"%.02lf %s", SetOptimalSize(TempSize + RecycleSize + ChkDskSize + RappsSize), SetOptimalUnit(TempSize + RecycleSize + ChkDskSize + RappsSize));
        StringCchPrintfW(FullText, _countof(FullText), StringText, totalAmount, wcv.DriveLetter);

        ZeroMemory(&seI, sizeof(seI));
        seI.cbSize = sizeof seI;
        seI.lpVerb = L"open";
        seI.nShow = SW_SHOW;

        SetDlgItemTextW(hwnd, IDC_STATIC_DLG, FullText);
        ListView_SetCheckState(hList, 1, 1);
        ListView_SetItemState(hList, 1, LVIS_SELECTED, LVIS_SELECTED);
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
