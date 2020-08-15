/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL - See COPYING in the top level directory
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
	WCHAR tempList[MAX_PATH] = { 0 };
	WCHAR stringText[MAX_PATH] = { 0 };
	WCHAR fullText[MAX_PATH] = { 0 };
	WCHAR totalAmount[MAX_PATH] = { 0 };
	static WCHAR* viewFolder = NULL;
	uint64_t tempSize = sz.tempASize + sz.tempBSize;
	uint64_t recycleSize = sz.recyclebinSize;
	uint64_t downloadSize = sz.downloadedSize;
	uint64_t rappsSize = sz.rappsSize;
	static HWND hList = 0;
	
	LVCOLUMNW lvC;
	ZeroMemory(&lvC, sizeof(lvC));

	lvC.mask = LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
	lvC.cx = 158;
	lvC.cchTextMax = 256;
	lvC.fmt = LVCFMT_RIGHT;
	
	SHELLEXECUTEINFOW seI;
	ZeroMemory(&seI, sizeof(seI));
	seI.cbSize = sizeof seI;
	seI.lpVerb = L"open";
	seI.nShow = SW_SHOW;
	
	viewFolder = wcv.downloadDir;

	switch (Message)
	{
	case WM_INITDIALOG:
		SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
		hList = GetDlgItem(hwnd, IDC_CHOICE_LIST);
		SendMessageW(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
		
		ListView_InsertColumn(hList, 0, &lvC);
		ListView_InsertColumn(hList, 1, &lvC);

		createImageLists(hList);

		StringCchPrintfW(tempList, _countof(tempList), L"%.02lf %s", setOptimalSize(downloadSize), setOptimalUnit(downloadSize));
		addItem(hList, L"Downloaded Files", tempList, 1);
		memset(tempList, 0, sizeof tempList);

		StringCchPrintfW(tempList, _countof(tempList), L"%.02lf %s", setOptimalSize(rappsSize), setOptimalUnit(rappsSize));
		addItem(hList, L"Downloaded RAPPS Files", tempList, 0);
		memset(tempList, 0, sizeof tempList);
		
		if (bv.sysDrive == TRUE)
		{
			StringCchPrintfW(tempList, _countof(tempList), L"%.02lf %s", setOptimalSize(tempSize), setOptimalUnit(tempSize));
			addItem(hList, L"Temporary Files", tempList, 0);
		}
		
		StringCchPrintfW(tempList, _countof(tempList), L"%.02lf %s", setOptimalSize(recycleSize), setOptimalUnit(recycleSize));
		addItem(hList, L"Recycle Bin", tempList, 2);
		memset(tempList, 0, sizeof tempList);

		LoadStringW(GetModuleHandleW(NULL), IDS_CLEANUP, stringText, _countof(stringText));
		StringCchPrintfW(totalAmount, _countof(totalAmount), L"%.02lf %s", setOptimalSize(tempSize + recycleSize + downloadSize + rappsSize), setOptimalUnit(tempSize + recycleSize + downloadSize + rappsSize));
		StringCchPrintfW(fullText, _countof(fullText), stringText, totalAmount, wcv.driveLetter);

		SetDlgItemTextW(hwnd, IDC_STATIC_DLG, fullText);
		ListView_SetCheckState(hList, 1, 1);
		ListView_SetItemState(hList, 1, LVIS_SELECTED, LVIS_SELECTED);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_VIEW_FILES:
			if(!PathIsDirectoryW(viewFolder))
			{
				MessageBoxW(hwnd, L"Folder doesn't exist!", L"Warning", MB_OK | MB_ICONWARNING);
				break;
			}

			seI.lpFile = viewFolder;
			if (!ShellExecuteExW(&seI))
			{
				wchar_t err[256] = { 0 };
				FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
				MessageBoxW(NULL, err, L"Ok", MB_OK);
			}
			break;
		}
		break;

	case WM_NOTIFY:
	{
		NMHDR* header = (NMHDR*)lParam;
		NMLISTVIEW* nmlist = (NMLISTVIEW*)lParam;
		LVITEMW lvI;
		ZeroMemory(&lvI, sizeof(lvI));
		static long long size = 0;

		if (lParam)
		{
			/* FIX ME: A bug in comctl32 causes selected item to send LVIS_STATEIMAGEMASK */
			if (header && header->idFrom == IDC_CHOICE_LIST && header->code == LVN_ITEMCHANGED)
			{
				if (nmlist->uNewState & LVIS_SELECTED)
				{
					selItem(nmlist->iItem, hwnd);
				}

				else if (nmlist->uNewState & LVIS_STATEIMAGEMASK)
				{
					size = checkedItem(nmlist->iItem, hwnd, hList, size);
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
