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

INT_PTR CALLBACK SagesetPageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HWND hList = 0;
	
	LVCOLUMNW lvC;

	switch (Message)
	{
	case WM_INITDIALOG:
		SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
		hList = GetDlgItem(hwnd, IDC_SAGERUN_LIST);
		SendMessageW(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
		
		ZeroMemory(&lvC, sizeof(lvC));
		lvC.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		lvC.cx = 158;
		lvC.cchTextMax = 256;
		lvC.fmt = LVCFMT_RIGHT;
		
		ListView_InsertColumn(hList, 0, &lvC);
		ListView_InsertColumn(hList, 1, &lvC);

		CreateImageLists(hList);

		AddItem(hList, L"Old ChkDsk Files", L"", 0);

		AddItem(hList, L"RAPPS Files", L"", 0);
		
		AddItem(hList, L"Temporary Files", L"", 0);
		
		AddItem(hList, L"Recycle Bin", L"", 2);

		ListView_SetCheckState(hList, 1, 1);
		ListView_SetItemState(hList, 1, LVIS_SELECTED, LVIS_SELECTED);
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
			if (Header && Header->idFrom == IDC_SAGERUN_LIST && Header->code == LVN_ITEMCHANGED)
			{
				if (NmList->uNewState & LVIS_SELECTED)
				{
					SelItem(hwnd, NmList->iItem);
				}

				else if (NmList->uNewState & LVIS_STATEIMAGEMASK)
				{
					SagesetCheckedItem(NmList->iItem, hwnd, hList);
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
