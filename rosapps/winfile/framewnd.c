/*
 *  ReactOS winfile
 *
 *  framewnd.c
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
    
//#include <shellapi.h>
#include <windowsx.h>
#include <assert.h>
#define ASSERT assert

#include "main.h"
#include "about.h"
#include "framewnd.h"
#include "childwnd.h"
#include "utils.h"
#include "run.h"
#include "format.h"
#include "dialogs.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop
int  nOldWidth;                  // Holds the previous client area width
int  nOldHeight;                 // Holds the previous client area height


////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void resize_frame_rect(HWND hWnd, PRECT prect)
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
		SetupStatusBar(TRUE);
//		int parts[] = {300, 500};
//		SendMessage(Globals.hStatusBar, WM_SIZE, 0, 0);
//		SendMessage(Globals.hStatusBar, SB_SETPARTS, 2, (LPARAM)&parts);
		GetClientRect(Globals.hStatusBar, &rt);
		prect->bottom -= rt.bottom;
	}
	MoveWindow(Globals.hMDIClient, prect->left-1,prect->top-1,prect->right+2,prect->bottom+1, TRUE);
}

static void resize_frame(HWND hWnd, int cx, int cy)
{
	RECT rect = {0, 0, cx, cy};

	resize_frame_rect(hWnd, &rect);
}

void resize_frame_client(HWND hWnd)
{
	RECT rect;

	GetClientRect(hWnd, &rect);
	resize_frame_rect(hWnd, &rect);
}

////////////////////////////////////////////////////////////////////////////////
static HHOOK hcbthook;
static ChildWnd* newchild = NULL;

static LRESULT CALLBACK CBTProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HCBT_CREATEWND && newchild) {
        ChildWnd* pChildWnd = newchild;
        newchild = NULL;
        pChildWnd->hWnd = (HWND)wParam;
        SetWindowLong(pChildWnd->hWnd, GWL_USERDATA, (LPARAM)pChildWnd);
    }
    return CallNextHookEx(hcbthook, code, wParam, lParam);
}


/*
BOOL FindChildWindow(int cmd)
{
	TCHAR drv[_MAX_DRIVE];
	LPCTSTR root = Globals.drives;
	int i;
	for(i = cmd - ID_DRIVE_FIRST; i--; root++)
		while(*root)
			root++;
	if (activate_drive_window(root))
		return TRUE;
	_tsplitpath(root, drv, 0, 0, 0);
	if (!SetCurrentDirectory(drv)) {
		display_error(hWnd, GetLastError());
		//return TRUE;
	}
	return FALSE;
}
 */

static ChildWnd* alloc_child_window(LPCTSTR path)
{
	TCHAR drv[_MAX_DRIVE+1], dir[_MAX_DIR], name[_MAX_FNAME], ext[_MAX_EXT];
	ChildWnd* pChildWnd = (ChildWnd*)malloc(sizeof(ChildWnd));
	Root* root = &pChildWnd->root;
	Entry* entry;

	memset(pChildWnd, 0, sizeof(ChildWnd));
	pChildWnd->left.treePane = TRUE;
	pChildWnd->left.visible_cols = 0;
	pChildWnd->right.treePane = FALSE;
#ifndef _NO_EXTENSIONS
	pChildWnd->right.visible_cols = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_INDEX|COL_LINKS;
#else
	pChildWnd->right.visible_cols = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES;
#endif
	pChildWnd->pos.length = sizeof(WINDOWPLACEMENT);
	pChildWnd->pos.flags = 0;
	pChildWnd->pos.showCmd = SW_SHOWNORMAL;
	pChildWnd->pos.rcNormalPosition.left = CW_USEDEFAULT;
	pChildWnd->pos.rcNormalPosition.top = CW_USEDEFAULT;
	pChildWnd->pos.rcNormalPosition.right = CW_USEDEFAULT;
	pChildWnd->pos.rcNormalPosition.bottom = CW_USEDEFAULT;
	pChildWnd->nFocusPanel = 0;
	pChildWnd->nSplitPos = 200;
	pChildWnd->sortOrder = SORT_NAME;
	pChildWnd->header_wdths_ok = FALSE;
	lstrcpy(pChildWnd->szPath, path);
	_tsplitpath(path, drv, dir, name, ext);
#if !defined(_NO_EXTENSIONS) && defined(__linux__)
	if (*path == '/') {
		root->drive_type = GetDriveType(path);
		lstrcat(drv, _T("/"));
		lstrcpy(root->volname, _T("root fs"));
		root->fs_flags = 0;
		lstrcpy(root->fs, _T("unixfs"));
		lstrcpy(root->path, _T("/"));
		entry = read_tree_unix(root, path, pChildWnd->sortOrder);
    } else
#endif
	{
		root->drive_type = GetDriveType(path);
		lstrcat(drv, _T("\\"));
		GetVolumeInformation(drv, root->volname, _MAX_FNAME, 0, 0, &root->fs_flags, root->fs, _MAX_DIR);
		lstrcpy(root->path, drv);
		entry = read_tree_win(root, path, pChildWnd->sortOrder);
	}
//@@lstrcpy(root->entry.data.cFileName, drv);
	wsprintf(root->entry.data.cFileName, _T("%s - %s"), drv, root->fs);
	root->entry.data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	pChildWnd->left.root = &root->entry;
	set_curdir(pChildWnd, entry);
	return pChildWnd;
}

HWND CreateChildWindow(int drv_id)
{
	//TCHAR drv[_MAX_DRIVE];
	TCHAR path[MAX_PATH];
	ChildWnd* pChildWnd = NULL;
/*
	LPCTSTR root = Globals.drives;
	int i;
	for(i = cmd - ID_DRIVE_FIRST; i--; root++)
		while(*root)
			root++;
	if (activate_drive_window(root))
		return 0;
	_tsplitpath(root, drv, 0, 0, 0);
	if (!SetCurrentDirectory(drv)) {
		display_error(hWnd, GetLastError());
		return 0;
	}
 */
	GetCurrentDirectory(MAX_PATH, path);
//	pChildWnd = (ChildWnd*)malloc(sizeof(ChildWnd));
	pChildWnd = alloc_child_window(path);
//	if (!create_child_window(pChildWnd))
//		free(pChildWnd);

	if (pChildWnd != NULL) {
        MDICREATESTRUCT mcs = {
            szChildClass, path, hInst,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            0/*style*/, 0/*lParam*/
		};
        hcbthook = SetWindowsHookEx(WH_CBT, CBTProc, 0, GetCurrentThreadId());
    	newchild = pChildWnd;
        pChildWnd->hWnd = (HWND)SendMessage(Globals.hMDIClient, WM_MDICREATE, 0, (LPARAM)&mcs);
        UnhookWindowsHookEx(hcbthook);
        if (pChildWnd->hWnd == NULL) {
            free(pChildWnd);
        	newchild = pChildWnd = NULL;
        }
        return pChildWnd->hWnd;
	}
    return 0;
}

BOOL CALLBACK CloseEnumProc(HWND hWnd, LPARAM lParam)
{
    if (!GetWindow(hWnd, GW_OWNER)) {
        SendMessage(GetParent(hWnd), WM_MDIRESTORE, (WPARAM)hWnd, 0);
        if (SendMessage(hWnd, WM_QUERYENDSESSION, 0, 0)) {
            SendMessage(GetParent(hWnd), WM_MDIDESTROY, (WPARAM)hWnd, 0);
        }
    }
    return 1;
}

void OnEnterMenuLoop(HWND hWnd)
{
    int nParts;

    // Update the status bar pane sizes
    nParts = -1;
    SendMessage(Globals.hStatusBar, SB_SETPARTS, 1, (long)&nParts);
    bInMenuLoop = TRUE;
    SendMessage(Globals.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
}

void OnExitMenuLoop(HWND hWnd)
{
    RECT  rc;
    int   nParts[3];

    bInMenuLoop = FALSE;
    // Update the status bar pane sizes
    GetClientRect(hWnd, &rc);
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = rc.right;
    SendMessage(Globals.hStatusBar, SB_SETPARTS, 3, (long)nParts);
    SendMessage(Globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)_T(""));
	SetupStatusBar(TRUE);
	UpdateStatusBar();
}

void OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    TCHAR str[100];

    strcpy(str, TEXT(""));
    if (nFlags & MF_POPUP) {
        if (hSysMenu != GetMenu(hWnd)) {
            if (nItemID == 2) nItemID = 5;
        }
    }
    if (LoadString(Globals.hInstance, nItemID, str, 100)) {
        // load appropriate string
        LPTSTR lpsz = str;
        // first newline terminates actual string
        lpsz = _tcschr(lpsz, '\n');
        if (lpsz != NULL)
            *lpsz = '\0';
    }
    SendMessage(Globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)str);
}


static BOOL activate_drive_window(LPCTSTR path)
{
	TCHAR drv1[_MAX_DRIVE], drv2[_MAX_DRIVE];
	HWND child_wnd;

	_tsplitpath(path, drv1, 0, 0, 0);

	 // search for a already open window for the same drive
	for (child_wnd = GetNextWindow(Globals.hMDIClient,GW_CHILD); 
	     child_wnd; 
		 child_wnd = GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		ChildWnd* pChildWnd = (ChildWnd*) GetWindowLong(child_wnd, GWL_USERDATA);
		if (pChildWnd) {
			_tsplitpath(pChildWnd->root.path, drv2, 0, 0, 0);
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

static void toggle_child(HWND hWnd, UINT cmd, HWND hchild)
{
	BOOL vis = IsWindowVisible(hchild);

	CheckMenuItem(Globals.hMenuOptions, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);
	ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);
#ifndef _NO_EXTENSIONS
	if (g_fullscreen.mode)
		fullscreen_move(hWnd);
#endif
	resize_frame_client(hWnd);
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: _CmdWndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes WM_COMMAND messages for the main frame window.
//
//

LRESULT _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT cmd = LOWORD(wParam);
	HWND hChildWnd;
//	HWND hwndClient = (HWND)SendMessage(Globals.hMDIClient, WM_MDIGETACTIVE, 0, 0);
//	if (hwndClient) 
//	    if (SendMessage(hwndClient, WM_DISPATCH_COMMAND, wParam, lParam))
//			return 0;

	if (cmd >= ID_DRIVE_FIRST && cmd <= (ID_DRIVE_FIRST + 0xFF)) {
		TCHAR drv[_MAX_DRIVE];
		//TCHAR path[MAX_PATH];
		//ChildWnd* pChildWnd;
		LPCTSTR root = Globals.drives;
		int i;
		for (i = cmd - ID_DRIVE_FIRST; i--; root++)
			while (*root)
				root++;
		if (activate_drive_window(root))
			return 0;
		_tsplitpath(root, drv, 0, 0, 0);
		if (!SetCurrentDirectory(drv)) {
			display_error(hWnd, GetLastError());
			return 0;
		}
		//GetCurrentDirectory(MAX_PATH, path); //@@ letztes Verzeichnis pro Laufwerk speichern
        //CreateChildWindow(path);
        CreateChildWindow(cmd - ID_DRIVE_FIRST);

//		pChildWnd = alloc_child_window(path);
//		if (!create_child_window(pChildWnd))
//			free(pChildWnd);
	} else {
		switch (cmd) {
        case ID_WINDOW_CLOSEALL:
            EnumChildWindows(Globals.hMDIClient, &CloseEnumProc, 0);
            break;
        case ID_WINDOW_CLOSE:
            hChildWnd = (HWND) SendMessage(Globals.hMDIClient, WM_MDIGETACTIVE, 0, 0);
            if (!SendMessage(hChildWnd, WM_QUERYENDSESSION, 0, 0))
                SendMessage(Globals.hMDIClient, WM_MDIDESTROY, (WPARAM)hChildWnd, 0);
            break;
        case ID_FILE_EXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        case ID_FILE_RUN:
            OnFileRun();
            break;
		case ID_DISK_FORMAT_DISK:
//			SHFormatDrive(hWnd, 0 /* A: */, SHFMT_ID_DEFAULT, 0);
			{
				UINT OldMode = SetErrorMode(0); // Get the current Error Mode settings.
				SetErrorMode(OldMode & ~SEM_FAILCRITICALERRORS); // Force O/S to handle
				// Call SHFormatDrive here.
				SHFormatDrive(hWnd, 0 /* A: */, SHFMT_ID_DEFAULT, 0);
				SetErrorMode(OldMode); // Put it back the way it was. 			
			}
			break;
        case ID_VIEW_BY_FILE_TYPE:
			{
			struct ExecuteDialog dlg = {{0}};
            if (DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_DIALOG_VIEW_TYPE), hWnd, ViewFileTypeWndProc, (LPARAM)&dlg) == IDOK) {
            }
			}
			break;
        case ID_OPTIONS_CONFIRMATION:
			{
			struct ExecuteDialog dlg = {{0}};
            if (DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS_CONFIRMATON), hWnd, OptionsConfirmationWndProc, (LPARAM)&dlg) == IDOK) {
            }
			}
            break;
		case ID_OPTIONS_FONT:
            break;
		case ID_OPTIONS_CUSTOMISE_TOOLBAR:
            break;
		case ID_OPTIONS_TOOLBAR:
			toggle_child(hWnd, cmd, Globals.hToolBar);
            break;
		case ID_OPTIONS_DRIVEBAR:
			toggle_child(hWnd, cmd, Globals.hDriveBar);
            break;
		case ID_OPTIONS_STATUSBAR:
			toggle_child(hWnd, cmd, Globals.hStatusBar);
            break;
		case ID_OPTIONS_OPEN_NEW_WINDOW_ON_CONNECT:
            break;
		case ID_OPTIONS_MINIMISE_ON_USE:
            break;
        case ID_OPTIONS_SAVE_ON_EXIT:
            break;

		case ID_WINDOW_NEW_WINDOW:
            CreateChildWindow(-1);
//			{
//			ChildWnd* pChildWnd = alloc_child_window(path);
//			if (!create_child_window(pChildWnd))
//				free(pChildWnd);
//			}
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
		case ID_HELP_CONTENTS:
			WinHelp(hWnd, _T("winfile"), HELP_INDEX, 0);
            break;
		case ID_HELP_SEARCH_HELP:
            break;
		case ID_HELP_HOW_TO_USE_HELP:
#ifdef WINSHELLAPI
            ShellAbout(hWnd, szTitle, "", LoadIcon(Globals.hInstance, (LPCTSTR)IDI_WINFILE));
#endif
            break;
        case ID_HELP_ABOUT:
            ShowAboutBox(hWnd);
            break;
		default:
/*
			if ((cmd<IDW_FIRST_CHILD || cmd>=IDW_FIRST_CHILD+0x100) &&
				(cmd<SC_SIZE || cmd>SC_RESTORE)) {
				MessageBox(hWnd, _T("Not yet implemented"), _T("Winefile"), MB_OK);
			}
			return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
 */
            hChildWnd = (HWND)SendMessage(Globals.hMDIClient, WM_MDIGETACTIVE, 0, 0);
            if (IsWindow(hChildWnd))
                SendMessage(hChildWnd, WM_COMMAND, wParam, lParam);
            else
			    return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
		}
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: FrameWndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main frame window.
//
//  WM_COMMAND  - process the application menu
//  WM_DESTROY  - post a quit message and return
//
//

LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
    case WM_CREATE:
        {
        HMENU hMenuWindow = GetSubMenu(Globals.hMenuFrame, GetMenuItemCount(Globals.hMenuFrame)-2);
        CLIENTCREATESTRUCT ccs = { hMenuWindow, IDW_FIRST_CHILD };
#if 0
        hMDIClient = CreateWindow(_T("MDICLIENT"), NULL,
                    WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE,
                    0, 0, 0, 0,
                    hWnd, (HMENU)1, hInst, &ccs);
#else
        Globals.hMDIClient = CreateWindowEx(0, _T("MDICLIENT"), NULL,
                //WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE|WS_BORDER,
                WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE,
                0, 0, 0, 0,
                hWnd, (HMENU)0, hInst, &ccs);
#endif
        }
		break;
	case WM_COMMAND:
		return _CmdWndProc(hWnd, message, wParam, lParam);
		break;
	case WM_SIZE:
        resize_frame_client(hWnd);
		break;
    case WM_ENTERMENULOOP:
        OnEnterMenuLoop(hWnd);
		break;
    case WM_EXITMENULOOP:
        OnExitMenuLoop(hWnd);
		break;
    case WM_MENUSELECT:
        OnMenuSelect(hWnd, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
    case WM_QUERYENDSESSION:
    case WM_CLOSE:
        SendMessage(hWnd, WM_COMMAND, ID_WINDOW_CLOSEALL, 0);
        if (GetWindow(Globals.hMDIClient, GW_CHILD) != NULL)
            return 0;
        // else fall thru...
    default:
        return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
	}
	return 0;
}
