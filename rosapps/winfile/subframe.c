/*
 *  ReactOS winfile
 *
 *  subframe.c
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
    
//#include <windowsx.h>
//#include <ctype.h>
#include <assert.h>
#define ASSERT assert

#include "winfile.h"
#include "mdiclient.h"
#include "subframe.h"
#include "utils.h"
//#include "treeview.h"
//#include "listview.h"
//#include "debug.h"


static void resize_frame_rect(HWND hwnd, PRECT prect)
{
	int new_top;
	RECT rt;

	if (IsWindowVisible(Globals.hToolBar)) {
		SendMessage(Globals.hToolBar, WM_SIZE, 0, 0);
		GetClientRect(Globals.hToolBar, &rt);
		prect->top = rt.bottom+3;
		prect->bottom -= rt.bottom+3;
	}

	if (IsWindowVisible(Globals.hDriveBar)) {
		SendMessage(Globals.hDriveBar, WM_SIZE, 0, 0);
		GetClientRect(Globals.hDriveBar, &rt);
		new_top = --prect->top + rt.bottom+3;
		MoveWindow(Globals.hDriveBar, 0, prect->top, rt.right, new_top, TRUE);
		prect->top = new_top;
		prect->bottom -= rt.bottom+2;
	}

	if (IsWindowVisible(Globals.hStatusBar)) {
		int parts[] = {300, 500};

		SendMessage(Globals.hStatusBar, WM_SIZE, 0, 0);
		SendMessage(Globals.hStatusBar, SB_SETPARTS, 2, (LPARAM)&parts);
		GetClientRect(Globals.hStatusBar, &rt);
		prect->bottom -= rt.bottom;
	}

	MoveWindow(Globals.hMDIClient, prect->left-1,prect->top-1,prect->right+2,prect->bottom+1, TRUE);
}

static void resize_frame(HWND hwnd, int cx, int cy)
{
	RECT rect = {0, 0, cx, cy};

	resize_frame_rect(hwnd, &rect);
}

void resize_frame_client(HWND hwnd)
{
	RECT rect;

	GetClientRect(hwnd, &rect);
	resize_frame_rect(hwnd, &rect);
}


LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	if (1) {
		switch (nmsg) {
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_COMMAND:
			{
			UINT cmd = LOWORD(wparam);
			HWND hwndClient = (HWND) SendMessage(Globals.hMDIClient, WM_MDIGETACTIVE, 0, 0);

			if (SendMessage(hwndClient, WM_DISPATCH_COMMAND, wparam, lparam))
				break;

			if (cmd>=ID_DRIVE_FIRST && cmd<=ID_DRIVE_FIRST+0xFF) {
				TCHAR drv[_MAX_DRIVE], path[MAX_PATH];
				ChildWnd* child;
				LPCTSTR root = Globals.drives;
				int i;
				for(i = cmd - ID_DRIVE_FIRST; i--; root++)
					while(*root)
						root++;
				if (activate_drive_window(root))
					return 0;
				_tsplitpath(root, drv, 0, 0, 0);
				if (!SetCurrentDirectory(drv)) {
					display_error(hwnd, GetLastError());
					return 0;
				}
				GetCurrentDirectory(MAX_PATH, path); //@@ letztes Verzeichnis pro Laufwerk speichern
				child = alloc_child_window(path);
				if (!create_child_window(child))
					free(child);
			} else {
				switch (cmd) {
				case ID_FILE_EXIT:
					PostQuitMessage(0);
					break;
				case ID_WINDOW_NEW_WINDOW:
					{
					TCHAR path[MAX_PATH];
					ChildWnd* child;
					GetCurrentDirectory(MAX_PATH, path);
					child = alloc_child_window(path);
					if (!create_child_window(child))
						free(child);
					}
					break;
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
				case ID_OPTIONS_TOOLBAR:
					toggle_child(hwnd, cmd, Globals.hToolBar);
					break;
				case ID_OPTIONS_DRIVEBAR:
					toggle_child(hwnd, cmd, Globals.hDriveBar);
					break;
				case ID_OPTIONS_STATUSBAR:
					toggle_child(hwnd, cmd, Globals.hStatusBar);
					break;
#if 0
				case ID_EXECUTE:
					{
					struct ExecuteDialog dlg = {{0}};
					if (DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_EXECUTE), hwnd, ExecuteDialogWndProg, (LPARAM)&dlg) == IDOK)
						ShellExecute(hwnd, _T("open")/*operation*/, dlg.cmd/*file*/, NULL/*parameters*/, NULL/*dir*/, dlg.cmdshow);
					}
					break;
				case ID_HELP:
					WinHelp(hwnd, _T("winfile"), HELP_INDEX, 0);
					break;
#endif
#ifndef _NO_EXTENSIONS
				case ID_VIEW_FULLSCREEN:
					CheckMenuItem(Globals.hMenuOptions, cmd, toggle_fullscreen(hwnd)?MF_CHECKED:0);
					break;
#ifdef __linux__
				case ID_DRIVE_UNIX_FS:
					{
					TCHAR path[MAX_PATH];
					ChildWnd* child;
					if (activate_drive_window(_T("/")))
						break;
					getcwd(path, MAX_PATH);
					child = alloc_child_window(path);
					if (!create_child_window(child))
						free(child);
					}
					break;
#endif
#endif
					//TODO: There are even more menu items!
				default:
					/*@@if (wParam >= PM_FIRST_LANGUAGE && wParam <= PM_LAST_LANGUAGE)
						STRING_SelectLanguageByNumber(wParam - PM_FIRST_LANGUAGE);
					else */
					if ((cmd<IDW_FIRST_CHILD || cmd>=IDW_FIRST_CHILD+0x100) &&
						(cmd<SC_SIZE || cmd>SC_RESTORE)) {
						MessageBox(hwnd, _T("Not yet implemented"), _T("Winefile"), MB_OK);
					}
					return DefFrameProc(hwnd, Globals.hMDIClient, nmsg, wparam, lparam);
				}
			}
            }
			break;
		case WM_SIZE:
			resize_frame(hwnd, LOWORD(lparam), HIWORD(lparam));
			break;	// do not pass message to DefFrameProc
#ifndef _NO_EXTENSIONS
		case WM_GETMINMAXINFO:
			{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;
			lpmmi->ptMaxTrackSize.x <<= 1;//2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN
			lpmmi->ptMaxTrackSize.y <<= 1;//2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN
			}
			break;
		case FRM_CALC_CLIENT:
			frame_get_clientspace(hwnd, (PRECT)lparam);
			return TRUE;
#endif
		default:
			return DefFrameProc(hwnd, Globals.hMDIClient, nmsg, wparam, lparam);
		}
	}
	return 0;
}


