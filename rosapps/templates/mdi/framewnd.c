/*
 *  ReactOS Application
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
    
#include "main.h"
#include "about.h"
#include "framewnd.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void resize_frame_rect(HWND hWnd, PRECT prect)
{
    RECT rt;

    if (IsWindowVisible(hToolBar)) {
        SendMessage(hToolBar, WM_SIZE, 0, 0);
        GetClientRect(hToolBar, &rt);
        prect->top = rt.bottom+3;
        prect->bottom -= rt.bottom+3;
    }
    if (IsWindowVisible(hStatusBar)) {
        int parts[] = {300, 500};

        SendMessage(hStatusBar, WM_SIZE, 0, 0);
        SendMessage(hStatusBar, SB_SETPARTS, 2, (LPARAM)&parts);
        GetClientRect(hStatusBar, &rt);
        prect->bottom -= rt.bottom;
    }
    MoveWindow(hMDIClient, prect->left-1,prect->top-1,prect->right+2,prect->bottom+1, TRUE);
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

LRESULT CALLBACK CBTProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HCBT_CREATEWND && newchild) {
        ChildWnd* pChildWnd = newchild;
        newchild = NULL;
        pChildWnd->hWnd = (HWND)wParam;
        SetWindowLong(pChildWnd->hWnd, GWL_USERDATA, (LPARAM)pChildWnd);
    }
    return CallNextHookEx(hcbthook, code, wParam, lParam);
}

#if 0
HWND create_child_window(ChildWnd* pChildWnd)
{
    MDICREATESTRUCT mcs = {
        szChildClass, (LPTSTR)pChildWnd->path, hInst,
        pChildWnd->pos.rcNormalPosition.left, pChildWnd->pos.rcNormalPosition.top,
        pChildWnd->pos.rcNormalPosition.right-pChildWnd->pos.rcNormalPosition.left,
        pChildWnd->pos.rcNormalPosition.bottom-pChildWnd->pos.rcNormalPosition.top,
        0/*style*/, 0/*lParam*/
    };
    hcbthook = SetWindowsHookEx(WH_CBT, CBTProc, 0, GetCurrentThreadId());
    newchild = pChildWnd;
    pChildWnd->hWnd = (HWND)SendMessage(hMDIClient, WM_MDICREATE, 0, (LPARAM)&mcs);
    if (!pChildWnd->hWnd)
        return 0;
    UnhookWindowsHookEx(hcbthook);
    return pChildWnd->hWnd;
}

#endif


void toggle_child(HWND hWnd, UINT cmd, HWND hchild)
{
    BOOL vis = IsWindowVisible(hchild);

    HMENU hMenuOptions = GetSubMenu(hMenuFrame, ID_OPTIONS_MENU);
    CheckMenuItem(hMenuOptions, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);
    ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);
    resize_frame_client(hWnd);
}


static HWND InitChildWindow(LPTSTR param)
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
//	pChildWnd = alloc_child_window(path);
//	if (!create_child_window(pChildWnd))
//		free(pChildWnd);
	pChildWnd = (ChildWnd*)malloc(sizeof(ChildWnd));
	if (pChildWnd != NULL) {
        MDICREATESTRUCT mcs = {
            szChildClass, path, hInst,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            0/*style*/, 0/*lParam*/
		};
		memset(pChildWnd, 0, sizeof(ChildWnd));
        lstrcpy(pChildWnd->szPath, path);
		pChildWnd->pos.length = sizeof(WINDOWPLACEMENT);
		pChildWnd->pos.flags = 0;
		pChildWnd->pos.showCmd = SW_SHOWNORMAL;
		pChildWnd->pos.rcNormalPosition.left = CW_USEDEFAULT;
		pChildWnd->pos.rcNormalPosition.top = CW_USEDEFAULT;
	    pChildWnd->pos.rcNormalPosition.right = CW_USEDEFAULT;
    	pChildWnd->pos.rcNormalPosition.bottom = CW_USEDEFAULT;
  	    pChildWnd->nFocusPanel = 0;
	    pChildWnd->nSplitPos = 200;
        hcbthook = SetWindowsHookEx(WH_CBT, CBTProc, 0, GetCurrentThreadId());
    	newchild = pChildWnd;
        pChildWnd->hWnd = (HWND)SendMessage(hMDIClient, WM_MDICREATE, 0, (LPARAM)&mcs);
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
    SendMessage(hStatusBar, SB_SETPARTS, 1, (long)&nParts);
    bInMenuLoop = TRUE;
    SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
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
    SendMessage(hStatusBar, SB_SETPARTS, 3, (long)nParts);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)_T(""));
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
    if (LoadString(hInst, nItemID, str, 100)) {
        // load appropriate string
        LPTSTR lpsz = str;
        // first newline terminates actual string
        lpsz = _tcschr(lpsz, '\n');
        if (lpsz != NULL)
            *lpsz = '\0';
    }
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)str);
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
    HWND hChildWnd;

    if (1) {

        switch (LOWORD(wParam)) {
        case ID_WINDOW_CLOSEALL:
            EnumChildWindows(hMDIClient, &CloseEnumProc, 0);
            break;
        case ID_WINDOW_CLOSE:
            hChildWnd = (HWND) SendMessage(hMDIClient, WM_MDIGETACTIVE, 0, 0);
            if (!SendMessage(hChildWnd, WM_QUERYENDSESSION, 0, 0))
                SendMessage(hMDIClient, WM_MDIDESTROY, (WPARAM)hChildWnd, 0);
            break;
        case ID_FILE_EXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        case ID_OPTIONS_TOOLBAR:
            toggle_child(hWnd, LOWORD(wParam), hToolBar);
            break;
        case ID_OPTIONS_STATUSBAR:
            toggle_child(hWnd, LOWORD(wParam), hStatusBar);
            break;

        case ID_FILE_OPEN:
        case ID_WINDOW_NEW_WINDOW:
            InitChildWindow("Child Window");
            return 0;
        case ID_WINDOW_CASCADE:
            SendMessage(hMDIClient, WM_MDICASCADE, 0, 0);
            break;
        case ID_WINDOW_TILE_HORZ:
            SendMessage(hMDIClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
            break;
        case ID_WINDOW_TILE_VERT:
            SendMessage(hMDIClient, WM_MDITILE, MDITILE_VERTICAL, 0);
            break;
        case ID_WINDOW_ARRANGE_ICONS:
            SendMessage(hMDIClient, WM_MDIICONARRANGE, 0, 0);
            break;
        case ID_HELP_ABOUT:
            ShowAboutBox(hWnd);
            break;
        default:
            hChildWnd = (HWND)SendMessage(hMDIClient, WM_MDIGETACTIVE, 0, 0);
            if (IsWindow(hChildWnd))
                SendMessage(hChildWnd, WM_COMMAND, wParam, lParam);
            else
			    return DefFrameProc(hWnd, hMDIClient, message, wParam, lParam);
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
            HMENU hMenuWindow = GetSubMenu(hMenuFrame, GetMenuItemCount(hMenuFrame)-2);
            CLIENTCREATESTRUCT ccs = { hMenuWindow, IDW_FIRST_CHILD };
#if 0
            hMDIClient = CreateWindow(_T("MDICLIENT"), NULL,
                    WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE,
                    0, 0, 0, 0,
                    hWnd, (HMENU)1, hInst, &ccs);
#else
            hMDIClient = CreateWindowEx(0, _T("MDICLIENT"), NULL,
//            hMDIClient = CreateWindowEx(0, (LPCTSTR)(int)hChildWndClass, NULL,
                    WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE|WS_BORDER,
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
            if (GetWindow(hMDIClient, GW_CHILD) != NULL)
                return 0;
        // else fall thru...
        default:
            return DefFrameProc(hWnd, hMDIClient, message, wParam, lParam);
        }
    return 0;
}


