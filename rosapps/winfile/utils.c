/*
 *  ReactOS winfile
 *
 *  utils.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include <windowsx.h>

#include "main.h"
#include "utils.h"
#include "sort.h"
#include "draw.h"

#define	FRM_CALC_CLIENT		0xBF83
#define	Frame_CalcFrameClient(hWnd, prt) ((BOOL)SNDMSG(hWnd, FRM_CALC_CLIENT, 0, (LPARAM)(PRECT)prt))


void display_error(HWND hWnd, DWORD error)
{
	PTSTR msg;

	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL))
		MessageBox(hWnd, msg, _T("Winefile"), MB_OK);
	else
		MessageBox(hWnd, _T("Error"), _T("Winefile"), MB_OK);
	LocalFree(msg);
}


static void read_directory_win(Entry* parent, LPCTSTR path)
{
	Entry* entry = (Entry*) malloc(sizeof(Entry));
	int level = parent->level + 1;
	Entry* last = 0;
	HANDLE hFind;
//#ifndef _NO_EXTENSIONS
	HANDLE hFile;
//#endif

	TCHAR buffer[MAX_PATH], *p;
	for(p=buffer; *path; )
		*p++ = *path++;

	lstrcpy(p, _T("\\*"));

    memset(entry, 0, sizeof(Entry));
	hFind = FindFirstFile(buffer, &entry->data);

	if (hFind != INVALID_HANDLE_VALUE) {
		parent->down = entry;

		do {
			entry->down = 0;
			entry->up = parent;
			entry->expanded = FALSE;
			entry->scanned = FALSE;
			entry->level = level;

//#ifdef _NO_EXTENSIONS
#if 0
			 // hide directory entry "."
			if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				LPCTSTR name = entry->data.cFileName;

				if (name[0]=='.' && name[1]=='\0')
					continue;
			}
#else
			entry->unix_dir = FALSE;
			entry->bhfi_valid = FALSE;

			lstrcpy(p+1, entry->data.cFileName);

			hFile = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
								0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);

			if (hFile != INVALID_HANDLE_VALUE) {
				if (GetFileInformationByHandle(hFile, &entry->bhfi))
					entry->bhfi_valid = TRUE;

				CloseHandle(hFile);
			}
#endif

			last = entry;

			entry = (Entry*) malloc(sizeof(Entry));
            memset(entry, 0, sizeof(Entry));

			if (last)
				last->next = entry;
		} while(FindNextFile(hFind, &entry->data));

		last->next = 0;

		FindClose(hFind);
	} else
		parent->down = 0;

	free(entry);

	parent->scanned = TRUE;
}


void read_directory(Entry* parent, LPCTSTR path, int sortOrder)
{
	TCHAR buffer[MAX_PATH];
	Entry* entry;
	LPCTSTR s;
	PTSTR d;

#if !defined(_NO_EXTENSIONS) && defined(__linux__)
	if (parent->unix_dir)
	{
		read_directory_unix(parent, path);

		if (Globals.prescan_node) {
			s = path;
			d = buffer;

			while(*s)
				*d++ = *s++;

			*d++ = _T('/');

			for(entry=parent->down; entry; entry=entry->next)
				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					lstrcpy(d, entry->data.cFileName);
					read_directory_unix(entry, buffer);
					SortDirectory(entry, sortOrder);
				}
		}
	}
	else
#endif
	{
		read_directory_win(parent, path);

		if (Globals.prescan_node) {
			s = path;
			d = buffer;

			while(*s)
				*d++ = *s++;

			*d++ = _T('\\');

			for(entry=parent->down; entry; entry=entry->next)
				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					lstrcpy(d, entry->data.cFileName);
					read_directory_win(entry, buffer);
					SortDirectory(entry, sortOrder);
				}
		}
	}

	SortDirectory(parent, sortOrder);
}

 // get full path of specified directory entry
void get_path(Entry* dir, PTSTR path)
{
	Entry* entry;
	int len = 0;
	int level = 0;

	for(entry=dir; entry; level++) {
		LPCTSTR name = entry->data.cFileName;
		LPCTSTR s = name;
		int l;

		for(l=0; *s && *s!=_T('/') && *s!=_T('\\'); s++)
			l++;

		if (entry->up) {
			memmove(path+l+1, path, len*sizeof(TCHAR));
			memcpy(path+1, name, l*sizeof(TCHAR));
			len += l+1;

#ifndef _NO_EXTENSIONS
			if (entry->unix_dir)
				path[0] = _T('/');
			else
#endif
				path[0] = _T('\\');

			entry = entry->up;
		} else {
			memmove(path+l, path, len*sizeof(TCHAR));
			memcpy(path, name, l*sizeof(TCHAR));
			len += l;
			break;
		}
	}

	if (!level) {
#ifndef _NO_EXTENSIONS
		if (entry->unix_dir)
			path[len++] = _T('/');
		else
#endif
			path[len++] = _T('\\');
	}

	path[len] = _T('\0');
}


#ifndef _NO_EXTENSIONS

void frame_get_clientspace(HWND hWnd, PRECT prect)
{
	RECT rt;

	if (!IsIconic(hWnd))
		GetClientRect(hWnd, prect);
	else {
		WINDOWPLACEMENT wp;
		GetWindowPlacement(hWnd, &wp);
		prect->left = prect->top = 0;
		prect->right = wp.rcNormalPosition.right-wp.rcNormalPosition.left-
						2*(GetSystemMetrics(SM_CXSIZEFRAME)+GetSystemMetrics(SM_CXEDGE));
		prect->bottom = wp.rcNormalPosition.bottom-wp.rcNormalPosition.top-
						2*(GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYEDGE))-
						GetSystemMetrics(SM_CYCAPTION)-GetSystemMetrics(SM_CYMENUSIZE);
	}   
	if (IsWindowVisible(Globals.hToolBar)) {
		GetClientRect(Globals.hToolBar, &rt);
		prect->top += rt.bottom+2;
	}
	if (IsWindowVisible(Globals.hDriveBar)) {
		GetClientRect(Globals.hDriveBar, &rt);
		prect->top += rt.bottom+2;
	}
	if (IsWindowVisible(Globals.hStatusBar)) {
		GetClientRect(Globals.hStatusBar, &rt);
		prect->bottom -= rt.bottom;
	}
}


#endif


int is_exe_file(LPCTSTR ext)
{
	const static LPCTSTR executable_extensions[] = {
		_T("COM"),
		_T("EXE"),
		_T("BAT"),
		_T("CMD"),
#ifndef _NO_EXTENSIONS
		_T("CMM"),
		_T("BTM"),
		_T("AWK"),
#endif
		0
	};
	TCHAR ext_buffer[_MAX_EXT];
	const LPCTSTR* p;
	LPCTSTR s;
	LPTSTR d;

	for (s = ext + 1, d = ext_buffer; (*d = tolower(*s)); s++)
		d++;
	for (p = executable_extensions; *p; p++)
		if (!_tcscmp(ext_buffer, *p))
			return 1;
	return 0;
}

int is_registered_type(LPCTSTR ext)
{
	//TODO

	return 1;
}

void set_curdir(ChildWnd* child, Entry* entry)
{
	TCHAR path[MAX_PATH];

	child->left.cur = entry;
	child->right.root = entry;
	child->right.cur = entry;

	if (!entry->scanned)
		scan_entry(child, entry);
	else {
//		ListBox_ResetContent(child->right.hWnd);
//		insert_entries(&child->right, entry->down, -1);

//        RefreshList(child->right.hWnd, entry);

//		calc_widths(&child->right, FALSE);
//#ifndef _NO_EXTENSIONS
//		set_header(&child->right);
//#endif
	}
        RefreshList(child->right.hWnd, entry->down);

	get_path(entry, path);
	lstrcpy(child->szPath, path);
	SetWindowText(child->hWnd, path);
	SetCurrentDirectory(path);
}


// calculate prefered width for all visible columns
BOOL calc_widths(Pane* pane, BOOL anyway)
{
	int col, x, cx, spc=3*Globals.spaceSize.cx;
	int entries = ListBox_GetCount(pane->hWnd);
	int orgWidths[COLUMNS];
	int orgPositions[COLUMNS+1];
	HFONT hfontOld;
	HDC hdc;
	int cnt;

	if (!anyway) {
		memcpy(orgWidths, pane->widths, sizeof(orgWidths));
		memcpy(orgPositions, pane->positions, sizeof(orgPositions));
	}

	for (col = 0; col < COLUMNS; col++)
		pane->widths[col] = 0;

	hdc = GetDC(pane->hWnd);
	hfontOld = SelectFont(hdc, Globals.hFont);

	for (cnt = 0; cnt < entries; cnt++) {
		Entry* entry = (Entry*) ListBox_GetItemData(pane->hWnd, cnt);

		DRAWITEMSTRUCT dis = {0/*CtlType*/, 0/*CtlID*/,
			0/*itemID*/, 0/*itemAction*/, 0/*itemState*/,
			pane->hWnd/*hwndItem*/, hdc};

		draw_item(pane, &dis, entry, COLUMNS);
	}
	SelectObject(hdc, hfontOld);
	ReleaseDC(pane->hWnd, hdc);

	x = 0;
	for ( col = 0; col < COLUMNS; col++) {
		pane->positions[col] = x;
		cx = pane->widths[col];
		if (cx) {
			cx += spc;
			if (cx < IMAGE_WIDTH)
				cx = IMAGE_WIDTH;
			pane->widths[col] = cx;
		}
		x += cx;
	}
	pane->positions[COLUMNS] = x;
	ListBox_SetHorizontalExtent(pane->hWnd, x);

	 // no change?
	if (!memcmp(orgWidths, pane->widths, sizeof(orgWidths)))
		return FALSE;

	 // don't move, if only collapsing an entry
	if (!anyway && pane->widths[0]<orgWidths[0] &&
		!memcmp(orgWidths+1, pane->widths+1, sizeof(orgWidths)-sizeof(int))) {
		pane->widths[0] = orgWidths[0];
		memcpy(pane->positions, orgPositions, sizeof(orgPositions));

		return FALSE;
	}
	InvalidateRect(pane->hWnd, 0, TRUE);
	return TRUE;
}


 // calculate one prefered column width

void calc_single_width(Pane* pane, int col)
{
	HFONT hfontOld;
	int x, cx;
	int entries = ListBox_GetCount(pane->hWnd);
	int cnt;
	HDC hdc;

	pane->widths[col] = 0;
	hdc = GetDC(pane->hWnd);
	hfontOld = SelectFont(hdc, Globals.hFont);
	for (cnt = 0; cnt < entries; cnt++) {
		Entry* entry = (Entry*) ListBox_GetItemData(pane->hWnd, cnt);
		DRAWITEMSTRUCT dis = {0, 0, 0, 0, 0, pane->hWnd, hdc};
		draw_item(pane, &dis, entry, col);
	}
	SelectObject(hdc, hfontOld);
	ReleaseDC(pane->hWnd, hdc);
	cx = pane->widths[col];
	if (cx) {
		cx += 3*Globals.spaceSize.cx;
		if (cx < IMAGE_WIDTH)
			cx = IMAGE_WIDTH;
	}
	pane->widths[col] = cx;
	x = pane->positions[col] + cx;
	for(; col<COLUMNS; ) {
		pane->positions[++col] = x;
		x += pane->widths[col];
	}
	ListBox_SetHorizontalExtent(pane->hWnd, x);
}


#ifndef _NO_EXTENSIONS

static struct FullScreenParameters {
	BOOL	mode;
	RECT	orgPos;
	BOOL	wasZoomed;
} g_fullscreen = {
	FALSE	// mode
};

BOOL toggle_fullscreen(HWND hWnd)
{
	RECT rt;

	if ((g_fullscreen.mode=!g_fullscreen.mode)) {
		GetWindowRect(hWnd, &g_fullscreen.orgPos);
		g_fullscreen.wasZoomed = IsZoomed(hWnd);

		Frame_CalcFrameClient(hWnd, &rt);
		ClientToScreen(hWnd, (LPPOINT)&rt.left);
		ClientToScreen(hWnd, (LPPOINT)&rt.right);

		rt.left = g_fullscreen.orgPos.left-rt.left;
		rt.top = g_fullscreen.orgPos.top-rt.top;
		rt.right = GetSystemMetrics(SM_CXSCREEN)+g_fullscreen.orgPos.right-rt.right;
		rt.bottom = GetSystemMetrics(SM_CYSCREEN)+g_fullscreen.orgPos.bottom-rt.bottom;

		MoveWindow(hWnd, rt.left, rt.top, rt.right-rt.left, rt.bottom-rt.top, TRUE);
	} else {
		MoveWindow(hWnd, g_fullscreen.orgPos.left, g_fullscreen.orgPos.top,
							g_fullscreen.orgPos.right-g_fullscreen.orgPos.left,
							g_fullscreen.orgPos.bottom-g_fullscreen.orgPos.top, TRUE);

		if (g_fullscreen.wasZoomed)
			ShowWindow(hWnd, WS_MAXIMIZE);
	}

	return g_fullscreen.mode;
}

void fullscreen_move(HWND hWnd)
{
	RECT rt, pos;
	GetWindowRect(hWnd, &pos);

	Frame_CalcFrameClient(hWnd, &rt);
	ClientToScreen(hWnd, (LPPOINT)&rt.left);
	ClientToScreen(hWnd, (LPPOINT)&rt.right);

	rt.left = pos.left-rt.left;
	rt.top = pos.top-rt.top;
	rt.right = GetSystemMetrics(SM_CXSCREEN)+pos.right-rt.right;
	rt.bottom = GetSystemMetrics(SM_CYSCREEN)+pos.bottom-rt.bottom;

	MoveWindow(hWnd, rt.left, rt.top, rt.right-rt.left, rt.bottom-rt.top, TRUE);
}

#endif
