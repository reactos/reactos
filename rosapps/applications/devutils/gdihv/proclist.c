/*
 *	Gdi handle viewer
 *
 *	proclist.c
 *
 *	Copyright (C) 2007	Timo Kreuzer <timo <dot> kreuzer <at> reactos <dot> org>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gdihv.h"

VOID
ProcessList_Create(HWND hListCtrl)
{
	LVCOLUMN column;

	column.mask = LVCF_TEXT|LVCF_FMT|LVCF_WIDTH;
	column.fmt = LVCFMT_LEFT;

	column.pszText = L"Process";
	column.cx = 90;
	(void)ListView_InsertColumn(hListCtrl, 0, &column);

	column.pszText = L"ProcessID";
	column.cx = 90;
	(void)ListView_InsertColumn(hListCtrl, 1, &column);
	ProcessList_Update(hListCtrl);
}

VOID
ProcessList_Update(HWND hListCtrl)
{
	LV_ITEM item;
	DWORD ProcessIds[1024], BytesReturned;
	UINT cProcesses;
	HANDLE hProcess;
	WCHAR strText[MAX_PATH] = L"<unknown>";
	INT i;

	(void)ListView_DeleteAllItems(hListCtrl);
	memset(&item, 0, sizeof(LV_ITEM));
	item.mask = LVIF_TEXT|LVIF_PARAM;
	item.pszText = strText;

	/* Insert "kernel" */
	item.iItem = 0;
	item.lParam = 0;
	item.pszText = L"<Kernel>";
	(void)ListView_InsertItem(hListCtrl, &item);
	item.pszText = strText;
	wsprintf(strText, L"%#08x", 0);
	ListView_SetItemText(hListCtrl, 0, 1, strText);

	/* Insert "deleted" */
	item.iItem = 1;
	item.lParam = 1;
	item.pszText = L"<deleted>";
	(void)ListView_InsertItem(hListCtrl, &item);
	item.pszText = strText;
	wsprintf(strText, L"%#08x",1);
	ListView_SetItemText(hListCtrl, 1, 1, strText);

	if (!EnumProcesses(ProcessIds, sizeof(ProcessIds), &BytesReturned ))
	{
		return;
	}
	cProcesses = BytesReturned / sizeof(DWORD);
	if (cProcesses <= 1)
	{
		return;
	}
	for (i = 1; i < cProcesses; i++)
	{
		wsprintf(strText, L"<unknown>");
		item.lParam = ProcessIds[i];
		item.iItem = ListView_GetItemCount(hListCtrl);

		hProcess = 0;
		/* FIXME: HACK: ROS crashes when using OpenProcess with PROCESS_VM_READ */
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessIds[i]);
		if (hProcess)
		{
			GetModuleBaseName(hProcess, NULL, (LPWSTR)strText, MAX_PATH );
			CloseHandle(hProcess);
		}
		(void)ListView_InsertItem(hListCtrl, &item);

		wsprintf(strText, L"%#08x", ProcessIds[i]);
		ListView_SetItemText(hListCtrl, item.iItem, 1, strText);
	}
}
