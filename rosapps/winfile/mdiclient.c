/*
 *  ReactOS winfile
 *
 *  mdiclient.c
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
    
#include <shellapi.h>
//#include <winspool.h>
#include <windowsx.h>
#include <shellapi.h>
#include <ctype.h>
#include <assert.h>
#define ASSERT assert

#include "winfile.h"
#include "about.h"
#include "mdiclient.h"
#include "subframe.h"
#include "run.h"
#include "utils.h"
#include "treeview.h"
#include "listview.h"
#include "debug.h"
#include "draw.h"


#ifdef _NO_EXTENSIONS
#define	COLOR_SPLITBAR		WHITE_BRUSH
#else
#define	COLOR_SPLITBAR		LTGRAY_BRUSH
#endif


ChildWnd* alloc_child_window(LPCTSTR path)
{
	TCHAR drv[_MAX_DRIVE+1], dir[_MAX_DIR], name[_MAX_FNAME], ext[_MAX_EXT];
	ChildWnd* child = (ChildWnd*) malloc(sizeof(ChildWnd));
	Root* root = &child->root;
	Entry* entry;

	memset(child, 0, sizeof(ChildWnd));

	child->left.treePane = TRUE;
	child->left.visible_cols = 0;

	child->right.treePane = FALSE;
#ifndef _NO_EXTENSIONS
	child->right.visible_cols = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_INDEX|COL_LINKS;
#else
	child->right.visible_cols = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES;
#endif

	child->pos.length = sizeof(WINDOWPLACEMENT);
	child->pos.flags = 0;
	child->pos.showCmd = SW_SHOWNORMAL;
	child->pos.rcNormalPosition.left = CW_USEDEFAULT;
	child->pos.rcNormalPosition.top = CW_USEDEFAULT;
	child->pos.rcNormalPosition.right = CW_USEDEFAULT;
	child->pos.rcNormalPosition.bottom = CW_USEDEFAULT;

	child->focus_pane = 0;
	child->split_pos = 200;
	child->sortOrder = SORT_NAME;
	child->header_wdths_ok = FALSE;

	lstrcpy(child->path, path);

	_tsplitpath(path, drv, dir, name, ext);

#if !defined(_NO_EXTENSIONS) && defined(__linux__)
	if (*path == '/')
	{
		root->drive_type = GetDriveType(path);

		lstrcat(drv, _T("/"));
		lstrcpy(root->volname, _T("root fs"));
		root->fs_flags = 0;
		lstrcpy(root->fs, _T("unixfs"));

		lstrcpy(root->path, _T("/"));
		entry = read_tree_unix(root, path, child->sortOrder);
	}
	else
#endif
	{
		root->drive_type = GetDriveType(path);

		lstrcat(drv, _T("\\"));
		GetVolumeInformation(drv, root->volname, _MAX_FNAME, 0, 0, &root->fs_flags, root->fs, _MAX_DIR);

		lstrcpy(root->path, drv);
		entry = read_tree_win(root, path, child->sortOrder);
	}

//@@lstrcpy(root->entry.data.cFileName, drv);
	wsprintf(root->entry.data.cFileName, _T("%s - %s"), drv, root->fs);

	root->entry.data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

	child->left.root = &root->entry;

	set_curdir(child, entry);

	return child;
}


static HHOOK hcbthook;
static ChildWnd* newchild = NULL;

LRESULT CALLBACK CBTProc(int code, WPARAM wparam, LPARAM lparam)
{
	if (code==HCBT_CREATEWND && newchild) {
		ChildWnd* child = newchild;
		newchild = NULL;
		child->hwnd = (HWND) wparam;
		SetWindowLong(child->hwnd, GWL_USERDATA, (LPARAM)child);
	}
	return CallNextHookEx(hcbthook, code, wparam, lparam);
}

HWND create_child_window(ChildWnd* child)
{
	MDICREATESTRUCT mcs = {
//		WINEFILETREE, (LPTSTR)child->path, Globals.hInstance,
		szFrameClass, (LPTSTR)child->path, Globals.hInstance,
		child->pos.rcNormalPosition.left, child->pos.rcNormalPosition.top,
		child->pos.rcNormalPosition.right-child->pos.rcNormalPosition.left,
		child->pos.rcNormalPosition.bottom-child->pos.rcNormalPosition.top,
		0/*style*/, 0/*lParam*/
	};
	int idx;

	hcbthook = SetWindowsHookEx(WH_CBT, CBTProc, 0, GetCurrentThreadId());

	newchild = child;
	child->hwnd = (HWND) SendMessage(Globals.hMDIClient, WM_MDICREATE, 0, (LPARAM)&mcs);
	if (!child->hwnd)
		return 0;

	UnhookWindowsHookEx(hcbthook);

	idx = ListBox_FindItemData(child->left.hwnd, ListBox_GetCurSel(child->left.hwnd), child->left.cur);
	ListBox_SetCurSel(child->left.hwnd, idx);

	return child->hwnd;
}

// free all memory associated with a child window
static void free_child_window(ChildWnd* child)
{
	free_entries(&child->root.entry);
	free(child);
}

void toggle_child(HWND hwnd, UINT cmd, HWND hchild)
{
	BOOL vis = IsWindowVisible(hchild);

	CheckMenuItem(Globals.hMenuOptions, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);
	ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);
#ifndef _NO_EXTENSIONS
	if (g_fullscreen.mode)
		fullscreen_move(hwnd);
#endif
	resize_frame_client(hwnd);
}

BOOL activate_drive_window(LPCTSTR path)
{
	TCHAR drv1[_MAX_DRIVE], drv2[_MAX_DRIVE];
	HWND child_wnd;

	_tsplitpath(path, drv1, 0, 0, 0);

	 // search for a already open window for the same drive
	for (child_wnd = GetNextWindow(Globals.hMDIClient,GW_CHILD); 
	     child_wnd; 
		 child_wnd = GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		ChildWnd* child = (ChildWnd*) GetWindowLong(child_wnd, GWL_USERDATA);
		if (child) {
			_tsplitpath(child->root.path, drv2, 0, 0, 0);
			if (!lstrcmpi(drv2, drv1)) {
				SendMessage(Globals.hMDIClient, WM_MDIACTIVATE, (WPARAM)child_wnd, 0);
				if (IsMinimized(child_wnd))
					ShowWindow(child_wnd, SW_SHOWNORMAL);
				return TRUE;
			}
		}
	}
	return FALSE;
}

static void InitChildWindow(ChildWnd* child)
{
	create_tree_window(child->hwnd, &child->left, IDW_TREE_LEFT, IDW_HEADER_LEFT, child->path);
	create_list_window(child->hwnd, &child->right, IDW_TREE_RIGHT, IDW_HEADER_RIGHT);
}


#ifndef _NO_EXTENSIONS

void set_header(Pane* pane)
{
	HD_ITEM item;
	int scroll_pos = GetScrollPos(pane->hwnd, SB_HORZ);
	int i=0, x=0;

	item.mask = HDI_WIDTH;
	item.cxy = 0;

	for(; x+pane->widths[i]<scroll_pos && i<COLUMNS; i++) {
		x += pane->widths[i];
		Header_SetItem(pane->hwndHeader, i, &item);
	}

	if (i < COLUMNS) {
		x += pane->widths[i];
		item.cxy = x - scroll_pos;
		Header_SetItem(pane->hwndHeader, i++, &item);

		for(; i<COLUMNS; i++) {
			item.cxy = pane->widths[i];
			x += pane->widths[i];
			Header_SetItem(pane->hwndHeader, i, &item);
		}
	}
}

static LRESULT pane_notify(Pane* pane, NMHDR* pnmh)
{
	switch(pnmh->code) {
	case HDN_TRACK:
	case HDN_ENDTRACK:
		{
		HD_NOTIFY* phdn = (HD_NOTIFY*) pnmh;
		int idx = phdn->iItem;
		int dx = phdn->pitem->cxy - pane->widths[idx];
		int i;

		RECT clnt;
		GetClientRect(pane->hwnd, &clnt);
		 // move immediate to simulate HDS_FULLDRAG (for now [04/2000] not realy needed with WINELIB)
		Header_SetItem(pane->hwndHeader, idx, phdn->pitem);
		pane->widths[idx] += dx;
		for (i = idx; ++i <= COLUMNS; )
			pane->positions[i] += dx;
		{
		int scroll_pos = GetScrollPos(pane->hwnd, SB_HORZ);
		RECT rt_scr = {pane->positions[idx+1]-scroll_pos, 0, clnt.right, clnt.bottom};
		RECT rt_clip = {pane->positions[idx]-scroll_pos, 0, clnt.right, clnt.bottom};
		if (rt_scr.left < 0) rt_scr.left = 0;
		if (rt_clip.left < 0) rt_clip.left = 0;
		ScrollWindowEx(pane->hwnd, dx, 0, &rt_scr, &rt_clip, 0, 0, SW_INVALIDATE);
		rt_clip.right = pane->positions[idx+1];
		RedrawWindow(pane->hwnd, &rt_clip, 0, RDW_INVALIDATE|RDW_UPDATENOW);
		if (pnmh->code == HDN_ENDTRACK) {
			ListBox_SetHorizontalExtent(pane->hwnd, pane->positions[COLUMNS]);
			if (GetScrollPos(pane->hwnd, SB_HORZ) != scroll_pos)
				set_header(pane);
		}
		}
		}
		return FALSE;
	case HDN_DIVIDERDBLCLICK:
		{
		HD_NOTIFY* phdn = (HD_NOTIFY*) pnmh;
		HD_ITEM item;
		calc_single_width(pane, phdn->iItem);
		item.mask = HDI_WIDTH;
		item.cxy = pane->widths[phdn->iItem];
		Header_SetItem(pane->hwndHeader, phdn->iItem, &item);
		InvalidateRect(pane->hwnd, 0, TRUE);
		break;
		}
	}
	return 0;
}

#endif


static BOOL pane_command(Pane* pane, UINT cmd)
{
	switch(cmd) {
	case ID_VIEW_NAME:
		if (pane->visible_cols) {
			pane->visible_cols = 0;
			calc_widths(pane, TRUE);
#ifndef _NO_EXTENSIONS
			set_header(pane);
#endif
			InvalidateRect(pane->hwnd, 0, TRUE);
			CheckMenuItem(Globals.hMenuView, ID_VIEW_NAME, MF_BYCOMMAND|MF_CHECKED);
//			CheckMenuItem(Globals.hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND);
//			CheckMenuItem(Globals.hMenuView, ID_VIEW_SELECTED_ATTRIBUTES, MF_BYCOMMAND);
		}
		break;
#if 0
	case ID_VIEW_ALL_ATTRIBUTES:
		if (pane->visible_cols != COL_ALL) {
			pane->visible_cols = COL_ALL;
			calc_widths(pane, TRUE);
#ifndef _NO_EXTENSIONS
			set_header(pane);
#endif
			InvalidateRect(pane->hwnd, 0, TRUE);
			CheckMenuItem(Globals.hMenuView, ID_VIEW_NAME, MF_BYCOMMAND);
//			CheckMenuItem(Globals.hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND|MF_CHECKED);
//			CheckMenuItem(Globals.hMenuView, ID_VIEW_SELECTED_ATTRIBUTES, MF_BYCOMMAND);
		}
		break;
#ifndef _NO_EXTENSIONS
	case ID_PREFERED_SIZES: {
		calc_widths(pane, TRUE);
		set_header(pane);
		InvalidateRect(pane->hwnd, 0, TRUE);
		break;}
#endif
#endif
		// TODO: more command ids...
	default:
		return FALSE;
	}
	return TRUE;
}


LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	static int last_split;

	ChildWnd* child = (ChildWnd*) GetWindowLong(hwnd, GWL_USERDATA);
	ASSERT(child);

	switch(nmsg) {
		case WM_DRAWITEM: {
			LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lparam;
			Entry* entry = (Entry*) dis->itemData;

			if (dis->CtlID == IDW_TREE_LEFT)
				draw_item(&child->left, dis, entry, -1);
			else
				draw_item(&child->right, dis, entry, -1);

			return TRUE;}

		case WM_CREATE:
			InitChildWindow(child);
			break;

		case WM_NCDESTROY:
			free_child_window(child);
			SetWindowLong(hwnd, GWL_USERDATA, 0);
			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HBRUSH lastBrush;
			RECT rt;
			GetClientRect(hwnd, &rt);
			BeginPaint(hwnd, &ps);
			rt.left = child->split_pos-SPLIT_WIDTH/2;
			rt.right = child->split_pos+SPLIT_WIDTH/2+1;
			lastBrush = SelectBrush(ps.hdc, (HBRUSH)GetStockObject(COLOR_SPLITBAR));
			Rectangle(ps.hdc, rt.left, rt.top-1, rt.right, rt.bottom+1);
			SelectObject(ps.hdc, lastBrush);
#ifdef _NO_EXTENSIONS
			rt.top = rt.bottom - GetSystemMetrics(SM_CYHSCROLL);
			FillRect(ps.hdc, &rt, GetStockObject(BLACK_BRUSH));
#endif
			EndPaint(hwnd, &ps);
			break;}

		case WM_SETCURSOR:
			if (LOWORD(lparam) == HTCLIENT) {
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);

				if (pt.x>=child->split_pos-SPLIT_WIDTH/2 && pt.x<child->split_pos+SPLIT_WIDTH/2+1) {
					SetCursor(LoadCursor(0, IDC_SIZEWE));
					return TRUE;
				}
			}
			goto def;

		case WM_LBUTTONDOWN: {
			RECT rt;
			int x = LOWORD(lparam);

			GetClientRect(hwnd, &rt);

			if (x>=child->split_pos-SPLIT_WIDTH/2 && x<child->split_pos+SPLIT_WIDTH/2+1) {
				last_split = child->split_pos;
#ifdef _NO_EXTENSIONS
				draw_splitbar(hwnd, last_split);
#endif
				SetCapture(hwnd);
			}

			break;}

		case WM_LBUTTONUP:
			if (GetCapture() == hwnd) {
#ifdef _NO_EXTENSIONS
				RECT rt;
				int x = LOWORD(lparam);
				draw_splitbar(hwnd, last_split);
				last_split = -1;
				GetClientRect(hwnd, &rt);
				child->split_pos = x;
				resize_tree(child, rt.right, rt.bottom);
#endif
				ReleaseCapture();
			}
			break;

#ifdef _NO_EXTENSIONS
		case WM_CAPTURECHANGED:
			if (GetCapture()==hwnd && last_split>=0)
				draw_splitbar(hwnd, last_split);
			break;
#endif

		case WM_KEYDOWN:
			if (wparam == VK_ESCAPE)
				if (GetCapture() == hwnd) {
					RECT rt;
#ifdef _NO_EXTENSIONS
					draw_splitbar(hwnd, last_split);
#else
					child->split_pos = last_split;
#endif
					GetClientRect(hwnd, &rt);
					resize_tree(child, rt.right, rt.bottom);
					last_split = -1;
					ReleaseCapture();
					SetCursor(LoadCursor(0, IDC_ARROW));
				}
			break;

		case WM_MOUSEMOVE:
			if (GetCapture() == hwnd) {
				RECT rt;
				int x = LOWORD(lparam);

#ifdef _NO_EXTENSIONS
				HDC hdc = GetDC(hwnd);
				GetClientRect(hwnd, &rt);

				rt.left = last_split-SPLIT_WIDTH/2;
				rt.right = last_split+SPLIT_WIDTH/2+1;
				InvertRect(hdc, &rt);

				last_split = x;
				rt.left = x-SPLIT_WIDTH/2;
				rt.right = x+SPLIT_WIDTH/2+1;
				InvertRect(hdc, &rt);

				ReleaseDC(hwnd, hdc);
#else
				GetClientRect(hwnd, &rt);

				if (x>=0 && x<rt.right) {
					child->split_pos = x;
					resize_tree(child, rt.right, rt.bottom);
					rt.left = x-SPLIT_WIDTH/2;
					rt.right = x+SPLIT_WIDTH/2+1;
					InvalidateRect(hwnd, &rt, FALSE);
					UpdateWindow(child->left.hwnd);
					UpdateWindow(hwnd);
					UpdateWindow(child->right.hwnd);
				}
#endif
			}
			break;

#ifndef _NO_EXTENSIONS
		case WM_GETMINMAXINFO:
			DefMDIChildProc(hwnd, nmsg, wparam, lparam);

			{LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;

			lpmmi->ptMaxTrackSize.x <<= 1;//2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN
			lpmmi->ptMaxTrackSize.y <<= 1;//2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN
			break;}
#endif

		case WM_SETFOCUS:
			SetCurrentDirectory(child->path);
			SetFocus(child->focus_pane? child->right.hwnd: child->left.hwnd);
			break;

		case WM_DISPATCH_COMMAND: {
			Pane* pane = GetFocus()==child->left.hwnd? &child->left: &child->right;

			switch(LOWORD(wparam)) {
				case ID_WINDOW_NEW_WINDOW: {
					ChildWnd* new_child = alloc_child_window(child->path);

					if (!create_child_window(new_child))
						free(new_child);

					break;}
#if 0
				case ID_REFRESH:
					scan_entry(child, pane->cur);
					break;
				case ID_ACTIVATE:
					activate_entry(child, pane);
					break;
#endif
				case ID_WINDOW_CASCADE:
					SendMessage(Globals.hMDIClient, WM_MDICASCADE, 0, 0);
					break;
				case ID_WINDOW_TILE_HORZ:
					SendMessage(Globals.hMDIClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
					break;
				case ID_WINDOW_TILE_VERT:
					SendMessage(Globals.hMDIClient, WM_MDITILE, MDITILE_VERTICAL, 0);
					break;
				case ID_WINDOW_ARRANGE_ICONS:
					SendMessage(Globals.hMDIClient, WM_MDIICONARRANGE, 0, 0);
					break;
				default:
					return pane_command(pane, LOWORD(wparam));
			}

			return TRUE;}

		case WM_COMMAND: {
			Pane* pane = GetFocus()==child->left.hwnd? &child->left: &child->right;

			switch(HIWORD(wparam)) {
				case LBN_SELCHANGE: {
					int idx = ListBox_GetCurSel(pane->hwnd);
					Entry* entry = (Entry*) ListBox_GetItemData(pane->hwnd, idx);

					if (pane == &child->left)
						set_curdir(child, entry);
					else
						pane->cur = entry;
					break;}

				case LBN_DBLCLK:
					activate_entry(child, pane);
					break;
			}
			break;}

#ifndef _NO_EXTENSIONS
		case WM_NOTIFY: {
			NMHDR* pnmh = (NMHDR*) lparam;
			return pane_notify(pnmh->idFrom==IDW_HEADER_LEFT? &child->left: &child->right, pnmh);}
#endif
		case WM_SIZE:
			if (wparam != SIZE_MINIMIZED)
				resize_tree(child, LOWORD(lparam), HIWORD(lparam));
			 // fall through
		default: def:
			return DefMDIChildProc(hwnd, nmsg, wparam, lparam);
	}
	return 0;
}

/*
RegenerateUserEnvironment
 */