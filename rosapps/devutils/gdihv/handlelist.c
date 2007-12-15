/*
 *	Gdi handle viewer
 *
 *	handlelist.c
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
HandleList_Create(HWND hListCtrl)
{
	LVCOLUMN column;

	column.mask = LVCF_TEXT|LVCF_FMT|LVCF_WIDTH;
	column.fmt = LVCFMT_LEFT;

	column.pszText = L"Number";
	column.cx = 50;
	(void)ListView_InsertColumn(hListCtrl, 0, &column);

	column.pszText = L"Index";
	column.cx = 45;
	(void)ListView_InsertColumn(hListCtrl, 1, &column);

	column.pszText = L"Handle";
	column.cx = 90;
	(void)ListView_InsertColumn(hListCtrl, 2, &column);

	column.pszText = L"Type";
	column.cx = 80;
	(void)ListView_InsertColumn(hListCtrl, 3, &column);

	column.pszText = L"Process";
	column.cx = 80;
	(void)ListView_InsertColumn(hListCtrl, 4, &column);

	column.pszText = L"KernelData";
	column.cx = 80;
	(void)ListView_InsertColumn(hListCtrl, 5, &column);

	column.pszText = L"UserData";
	column.cx = 80;
	(void)ListView_InsertColumn(hListCtrl, 6, &column);

	column.pszText = L"Type";
	column.cx = 80;
	(void)ListView_InsertColumn(hListCtrl, 7, &column);

	HandleList_Update(hListCtrl, 0);
}

VOID
HandleList_Update(HWND hHandleListCtrl, HANDLE ProcessId)
{
	INT i, index;
	HANDLE handle;
	PGDI_TABLE_ENTRY pEntry;
	LVITEM item;
	TCHAR strText[80];
	TCHAR* str2;

	(void)ListView_DeleteAllItems(hHandleListCtrl);
	item.mask = LVIF_TEXT|LVIF_PARAM;
	item.pszText = strText;
	item.cchTextMax = 80;
	for (i = 0; i<= GDI_HANDLE_COUNT; i++)
	{
		pEntry = &GdiHandleTable[i];
		if ( ((ProcessId != (HANDLE)1) && ((pEntry->Type & GDI_HANDLE_BASETYPE_MASK) != 0)) ||
		     ((ProcessId == (HANDLE)1) && ((pEntry->Type & GDI_HANDLE_BASETYPE_MASK) == 0)) )
		{
			if (ProcessId == (HANDLE)1 ||
			    ((LONG)ProcessId & 0xfffc) == ((ULONG)pEntry->ProcessId & 0xfffc))
			{
				handle = GDI_HANDLE_CREATE(i, pEntry->Type);
				index = ListView_GetItemCount(hHandleListCtrl);
				item.iItem = index;
				item.iSubItem = 0;
				item.lParam = (LPARAM)handle;

				wsprintf(strText, L"%d", index);
				(void)ListView_InsertItem(hHandleListCtrl, &item);

				wsprintf(strText, L"%d", i);
				ListView_SetItemText(hHandleListCtrl, index, 1, strText);

				wsprintf(strText, L"%#08x", handle);
				ListView_SetItemText(hHandleListCtrl, index, 2, strText);

				str2 = GetTypeName(handle);
				ListView_SetItemText(hHandleListCtrl, index, 3, str2);

				wsprintf(strText, L"%#08x", (UINT)pEntry->ProcessId);
				ListView_SetItemText(hHandleListCtrl, index, 4, strText);

				wsprintf(strText, L"%#08x", (UINT)pEntry->KernelData);
				ListView_SetItemText(hHandleListCtrl, index, 5, strText);

				wsprintf(strText, L"%#08x", (UINT)pEntry->UserData);
				ListView_SetItemText(hHandleListCtrl, index, 6, strText);

				wsprintf(strText, L"%#08x", (UINT)pEntry->Type);
				ListView_SetItemText(hHandleListCtrl, index, 7, strText);
			}
		}
	}
}

TCHAR*
GetTypeName(HANDLE handle)
{
	TCHAR* strText;
	UINT Type = GDI_HANDLE_GET_TYPE(handle);

	switch (Type)
	{
		case GDI_OBJECT_TYPE_DC:
			strText = L"DC";
			break;
		case GDI_OBJECT_TYPE_REGION:
			strText = L"Region";
			break;
		case GDI_OBJECT_TYPE_BITMAP:
			strText = L"Bitmap";
			break;
		case GDI_OBJECT_TYPE_PALETTE:
			strText = L"Palette";
			break;
		case GDI_OBJECT_TYPE_FONT:
			strText = L"Font";
			break;
		case GDI_OBJECT_TYPE_BRUSH:
			strText = L"Brush";
			break;
		case GDI_OBJECT_TYPE_EMF:
			strText = L"EMF";
			break;
		case GDI_OBJECT_TYPE_PEN:
			strText = L"Pen";
			break;
		case GDI_OBJECT_TYPE_EXTPEN:
			strText = L"ExtPen";
			break;
		case GDI_OBJECT_TYPE_COLORSPACE:
			strText = L"ColSpace";
			break;
		case GDI_OBJECT_TYPE_METADC:
			strText = L"MetaDC";
			break;
		case GDI_OBJECT_TYPE_METAFILE:
			strText = L"Metafile";
			break;
		case GDI_OBJECT_TYPE_ENHMETAFILE:
			strText = L"EMF";
			break;
		case GDI_OBJECT_TYPE_ENHMETADC:
			strText = L"EMDC";
			break;
		case GDI_OBJECT_TYPE_MEMDC:
			strText = L"MemDC";
			break;
		case GDI_OBJECT_TYPE_DCE:
			strText = L"DCE";
			break;
		case GDI_OBJECT_TYPE_PFE:
			strText = L"PFE";
			break;
		case GDI_OBJECT_TYPE_DONTCARE:
			strText = L"anything";
			break;
		case GDI_OBJECT_TYPE_SILENT:
		default:
			strText = L"unknown";
			break;
	}
	return strText;
}
