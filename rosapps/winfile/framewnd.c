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
//#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
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
#include "worker.h"
#include "utils.h"
#include "shell.h"
#include "network.h"
#include "dialogs.h"


////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop
int  nOldWidth;                  // Holds the previous client area width
int  nOldHeight;                 // Holds the previous client area height

static HHOOK hcbthook;
static ChildWnd* newchild = NULL;


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
//		new_top = --prect->top + rt.bottom+3;
		new_top = --prect->top + rt.bottom+0;
		MoveWindow(Globals.hDriveBar, 0, prect->top, rt.right, new_top, TRUE);
		prect->top = new_top;
//		prect->bottom -= rt.bottom+2;
		prect->bottom -= rt.bottom-1;
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
	pChildWnd->nSplitPos = 300;
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
//            CW_USEDEFAULT, CW_USEDEFAULT,
//            CW_USEDEFAULT, CW_USEDEFAULT,
            20, 20, 200, 200, 
            WS_MAXIMIZE, 0
//            0/*style*/, 0/*lParam*/
		};
        hcbthook = SetWindowsHookEx(WH_CBT, CBTProc, 0, GetCurrentThreadId());
    	newchild = pChildWnd;
        pChildWnd->hWnd = (HWND)SendMessage(Globals.hMDIClient, WM_MDICREATE, 0, (LPARAM)&mcs);
        UnhookWindowsHookEx(hcbthook);
        if (pChildWnd->hWnd != NULL) {
            return pChildWnd->hWnd;
        } else {
            free(pChildWnd);
        	newchild = pChildWnd = NULL;
        }
	}
    return 0;
}

static BOOL CALLBACK CloseEnumProc(HWND hWnd, LPARAM lParam)
{
    if (!GetWindow(hWnd, GW_OWNER)) {
        SendMessage(GetParent(hWnd), WM_MDIRESTORE, (WPARAM)hWnd, 0);
        if (SendMessage(hWnd, WM_QUERYENDSESSION, 0, 0)) {
            SendMessage(GetParent(hWnd), WM_MDIDESTROY, (WPARAM)hWnd, 0);
        }
    }
    return 1;
}

static void OnEnterMenuLoop(HWND hWnd)
{
    int nParts;

    // Update the status bar pane sizes
    nParts = -1;
    SendMessage(Globals.hStatusBar, SB_SETPARTS, 1, (long)&nParts);
    bInMenuLoop = TRUE;
    SendMessage(Globals.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
}

static void OnExitMenuLoop(HWND hWnd)
{
/*
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
 */
    SendMessage(Globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)_T(""));
    bInMenuLoop = FALSE;
    SetupStatusBar(TRUE);
	UpdateStatusBar();
}

static void OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    TCHAR str[100];

    if (hSysMenu == NULL) return;

    _tcscpy(str, _T(""));
    if (nFlags & MF_POPUP) {
        switch (nItemID) {
        case ID_FILE_MENU:
            //EnableMenuItem(hSysMenu, uIDEnableItem,  MF_BYCOMMAND|MF_ENABLED);
            break;
        case ID_DISK_MENU:
//            EnableMenuItem(hSysMenu, ID_DISK_COPY_DISK,  MF_BYCOMMAND|MF_GRAYED);
            EnableMenuItem(hSysMenu, ID_DISK_COPY_DISK,  MF_BYCOMMAND|MF_ENABLED);
            break;
        case ID_TREE_MENU:
        case ID_VIEW_MENU:
        case ID_OPTIONS_MENU:
        case ID_SECURITY_MENU:
        case ID_WINDOW_MENU:
        case ID_HELP_MENU:
            break;
        }
//        if (hSysMenu != GetMenu(hWnd)) {
//            if (nItemID == 2) nItemID = 5;
//        }
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

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
        if (activate_drive_window(root)) {
            return TRUE;
        }
		_tsplitpath(root, drv, 0, 0, 0);
		if (!SetCurrentDirectory(drv)) {
			display_error(hWnd, GetLastError());
            return TRUE;
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

        case ID_DISK_COPY_DISK:
            CopyDisk(hWnd);
			break;
        case ID_DISK_LABEL_DISK:
            LabelDisk(hWnd);
			break;
        case ID_DISK_FORMAT_DISK:
            FormatDisk(hWnd);
#if 0
//			SHFormatDrive(hWnd, 0 /* A: */, SHFMT_ID_DEFAULT, 0);
			{
				UINT OldMode = SetErrorMode(0); // Get the current Error Mode settings.
				SetErrorMode(OldMode & ~SEM_FAILCRITICALERRORS); // Force O/S to handle
				// Call SHFormatDrive here.
				SHFormatDrive(hWnd, 0 /* A: */, SHFMT_ID_DEFAULT, 0);
				SetErrorMode(OldMode); // Put it back the way it was. 			
			}
#endif
			break;
        case ID_DISK_CONNECT_NETWORK_DRIVE:
            MapNetworkDrives(hWnd, TRUE);
			break;
        case ID_DISK_DISCONNECT_NETWORK_DRIVE:
            MapNetworkDrives(hWnd, FALSE);
			break;
        case ID_DISK_SHARE_AS:
            ModifySharing(hWnd, TRUE);
			break;
        case ID_DISK_STOP_SHARING:
            ModifySharing(hWnd, FALSE);
			break;
        case ID_DISK_SELECT_DRIVE:
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
            SendMessage(Globals.hToolBar, TB_CUSTOMIZE, 0, 0);
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
            if (Globals.Options & OPTIONS_OPEN_NEW_WINDOW_ON_CONNECT) {
                Globals.Options &= ~OPTIONS_OPEN_NEW_WINDOW_ON_CONNECT;
                CheckMenuItem(Globals.hMenuOptions, cmd, MF_BYCOMMAND);
            } else {
                Globals.Options |= OPTIONS_OPEN_NEW_WINDOW_ON_CONNECT;
                CheckMenuItem(Globals.hMenuOptions, cmd, MF_BYCOMMAND | MF_CHECKED);
            }
            break;
		case ID_OPTIONS_MINIMISE_ON_USE:
            if (Globals.Options & ID_OPTIONS_MINIMISE_ON_USE) {
                Globals.Options &= ~ID_OPTIONS_MINIMISE_ON_USE;
                CheckMenuItem(Globals.hMenuOptions, cmd, MF_BYCOMMAND);
            } else {
                Globals.Options |= ID_OPTIONS_MINIMISE_ON_USE;
                CheckMenuItem(Globals.hMenuOptions, cmd, MF_BYCOMMAND | MF_CHECKED);
            }
            break;
        case ID_OPTIONS_SAVE_ON_EXIT:
            if (Globals.Options & OPTIONS_SAVE_ON_EXIT) {
                Globals.Options &= ~OPTIONS_SAVE_ON_EXIT;
                CheckMenuItem(Globals.hMenuOptions, cmd, MF_BYCOMMAND);
            } else {
                Globals.Options |= OPTIONS_SAVE_ON_EXIT;
                CheckMenuItem(Globals.hMenuOptions, cmd, MF_BYCOMMAND | MF_CHECKED);
            }
            break;
		case ID_WINDOW_NEW_WINDOW:
            CreateChildWindow(-1);
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
        case ID_WINDOW_REFRESH:
            // TODO:
            break;
		case ID_HELP_CONTENTS:
			WinHelp(hWnd, _T("winfile"), HELP_CONTENTS, 0);
            break;
		case ID_HELP_SEARCH_HELP:
			WinHelp(hWnd, _T("winfile"), HELP_FINDER, 0);
            break;
		case ID_HELP_HOW_TO_USE_HELP:
			WinHelp(hWnd, _T("winfile"), HELP_HELPONHELP, 0);
            break;
        case ID_HELP_ABOUT:
#ifdef WINSHELLAPI
            ShellAbout(hWnd, szTitle, "", LoadIcon(Globals.hInstance, (LPCTSTR)IDI_WINFILE));
#else
            ShowAboutBox(hWnd);
#endif
            break;
		default:
/*
			if ((cmd<IDW_FIRST_CHILD || cmd>=IDW_FIRST_CHILD+0x100) &&
				(cmd<SC_SIZE || cmd>SC_RESTORE)) {
				MessageBox(hWnd, _T("Not yet implemented"), _T("Winefile"), MB_OK);
			}
			return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
 */
/*
            hChildWnd = (HWND)SendMessage(Globals.hMDIClient, WM_MDIGETACTIVE, 0, 0);
            if (IsWindow(hChildWnd))
                SendMessage(hChildWnd, WM_COMMAND, wParam, lParam);
            else
			    return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
 */
            return FALSE;
		}
	}
	return TRUE;
}


static TBBUTTON tbButtonNew[] = {
	{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
	{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
};
        
static LRESULT MsgNotify(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam)
{
    LPNMHDR     lpnmhdr;

//LPNMHDR                lpnmhdr;
static int             nResetCount;
static LPTBBUTTON      lpSaveButtons;
//LPARAM                 lParam;

    
    lpnmhdr = (LPNMHDR)lparam;
/*
    // The following code allows the toolbar to be customized. 
    // If you return FALSE the Customize Toolbar dialog flashes
    // and goes away.
  
    if (lpnmhdr->code == TBN_QUERYINSERT || lpnmhdr->code == TBN_QUERYDELETE) {
        return TRUE;
    }
        
    if (lpnmhdr->code == TBN_GETBUTTONINFO) {
        LPTBNOTIFY lpTbNotify = (LPTBNOTIFY)lparam;
        TCHAR szBuffer[20];

//        int tbButtonNew[20] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };
		TBBUTTON tbButtonNew[] = {
			{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
			{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
			{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
			{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
			{0, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		};
        
		// 20 = total number of buttons.
		// tbButton and tbButtonNew send information about
		// the other 12 buttons in tbButtonNew.
        if (lpTbNotify->iItem < 5) {
            lpTbNotify->tbButton = tbButtonNew[lpTbNotify->iItem];
//            LoadString(hInst, 4000+lpTbNotify->iItem, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
            LoadString(hInst, lpTbNotify->iItem, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
            lstrcpy (lpTbNotify->pszText, szBuffer);
            lpTbNotify->cchText = sizeof(szBuffer)/sizeof(TCHAR);
            return TRUE;
        } else {
            return 0;
        }
    }
 */
    switch (lpnmhdr->code) {
    case TBN_QUERYINSERT:
    case TBN_QUERYDELETE:
        return TRUE;

    case TBN_GETBUTTONINFO:
        {
        LPTBNOTIFY lpTbNotify = (LPTBNOTIFY)lparam;
        TCHAR szBuffer[20];
/*
typedef struct _TBBUTTON {
    int iBitmap; 
    int idCommand; 
    BYTE fsState; 
    BYTE fsStyle; 
    DWORD dwData; 
    INT_PTR iString; 
} TBBUTTON, NEAR* PTBBUTTON, FAR* LPTBBUTTON; 
 */
		// 20 = total number of buttons.
		// tbButton and tbButtonNew send information about
		// the other 12 buttons in tbButtonNew.
        if (lpTbNotify->iItem < 12) {
            lpTbNotify->tbButton = tbButtonNew[lpTbNotify->iItem];
            LoadString(hInst, lpTbNotify->iItem + 32769, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
            lstrcpy(lpTbNotify->pszText, szBuffer);
            lpTbNotify->cchText = sizeof(szBuffer)/sizeof(TCHAR);
            return TRUE;
        } else {
            return 0;
        }
        }
        break;

    case TBN_BEGINADJUST: // Start customizing the toolbar.
        {
	    LPTBNOTIFY  lpTB = (LPTBNOTIFY)lparam;
        int i;
	   
        // Allocate memory to store the button information.
        nResetCount = SendMessage(lpTB->hdr.hwndFrom, TB_BUTTONCOUNT, 0, 0);
        lpSaveButtons = (LPTBBUTTON)GlobalAlloc(GPTR, sizeof(TBBUTTON) * nResetCount);
      
        // Save the current configuration so if the user presses
        // reset, the original toolbar can be restored.
        for (i = 0; i < nResetCount; i++) {
            SendMessage(lpTB->hdr.hwndFrom, TB_GETBUTTON, i, (LPARAM)(lpSaveButtons + i));
        }
        }
        return TRUE;
   
    case TBN_RESET:
        {
        LPTBNOTIFY  lpTB = (LPTBNOTIFY)lparam;
        int         nCount, i;
	
        // Remove all of the existing buttons starting with the last and working down.
        nCount = SendMessage(lpTB->hdr.hwndFrom, TB_BUTTONCOUNT, 0, 0);
        for (i = nCount - 1; i >= 0; i--) {
            SendMessage(lpTB->hdr.hwndFrom, TB_DELETEBUTTON, i, 0);
        }
      
        // Restore the buttons that were saved.
        SendMessage(lpTB->hdr.hwndFrom, TB_ADDBUTTONS, (WPARAM)nResetCount, (LPARAM)lpSaveButtons);
        }
        return TRUE;
   
    case TBN_ENDADJUST:
        // Free the memory allocated during TBN_BEGINADJUST
        GlobalFree((HGLOBAL)lpSaveButtons);
        return TRUE;
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
        Globals.hMDIClient = CreateWindowEx(0, _T("MDICLIENT"), NULL,
                WS_EX_MDICHILD|WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE,
                0, 0, 0, 0,
                hWnd, (HMENU)0, hInst, &ccs);
        }
        CheckShellAvailable();
        CreateNetworkMonitorThread(hWnd);
        CreateMonitorThread(hWnd);
        CreateChildWindow(-1);
        SetTimer(hWnd, 1, 5000, NULL);
		break;

    case WM_NOTIFY:

        if (MsgNotify(hWnd, message, wParam, lParam)) return TRUE;
//        return MsgNotify(hWnd, message, wParam, lParam);
        switch (((LPNMHDR)lParam)->code) { 
#ifdef _MSC_VER
        case TTN_GETDISPINFO: 
            { 
            LPTOOLTIPTEXT lpttt; 
            lpttt = (LPTOOLTIPTEXT)lParam; 
            lpttt->hinst = hInst; 
            // load appropriate string
            lpttt->lpszText = MAKEINTRESOURCE(lpttt->hdr.idFrom); 
            } 
            break; 
#endif
        default:
            break;
        }
        break;
   
	case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
//            if (LOWORD(wParam) > ID_CMD_FIRST && LOWORD(wParam) < ID_CMD_LAST) {
                HWND hChildWnd = (HWND)SendMessage(Globals.hMDIClient, WM_MDIGETACTIVE, 0, 0);
                if (IsWindow(hChildWnd))
                    if (SendMessage(hChildWnd, WM_DISPATCH_COMMAND, wParam, lParam))
                        break;
//            }
   		    return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
        }
		break;

	case WM_TIMER:
        SignalMonitorEvent();
        SignalNetworkMonitorEvent();
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
		WinHelp(hWnd, _T("winfile"), HELP_QUIT, 0);
        KillTimer(hWnd, 1);
        DestryMonitorThread();
        DestryNetworkMonitorThread();
		PostQuitMessage(0);
		break;
    case WM_QUERYENDSESSION:
    case WM_CLOSE:
        SendMessage(hWnd, WM_COMMAND, ID_WINDOW_CLOSEALL, 0);
        if (GetWindow(Globals.hMDIClient, GW_CHILD) != NULL)
            return 0;
        // else fall thru...
    default: //def:
        return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
	}
	return 0;
}


