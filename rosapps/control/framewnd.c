/*
 *  ReactOS control
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

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include "main.h"
//#include "about.h"
#include "framewnd.h"
#include "listview.h"
#include <shellapi.h>


////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

static BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop

static HWND hListWnd;
static DWORD dwListStyle;

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void resize_frame_rect(HWND hWnd, PRECT prect)
{
	RECT rt;
/*
	if (IsWindowVisible(hToolBar)) {
		SendMessage(hToolBar, WM_SIZE, 0, 0);
		GetClientRect(hToolBar, &rt);
		prect->top = rt.bottom+3;
		prect->bottom -= rt.bottom+3;
	}
 */
	if (IsWindowVisible(hStatusBar)) {
		SetupStatusBar(hWnd, TRUE);
		GetClientRect(hStatusBar, &rt);
		prect->bottom -= rt.bottom;
	}
    MoveWindow(hListWnd, prect->left, prect->top, prect->right, prect->bottom, TRUE);
}

void resize_frame_client(HWND hWnd)
{
	RECT rect;

	GetClientRect(hWnd, &rect);
	resize_frame_rect(hWnd, &rect);
}

////////////////////////////////////////////////////////////////////////////////

static void OnEnterMenuLoop(HWND hWnd)
{
    int nParts;

    // Update the status bar pane sizes
    nParts = -1;
    SendMessage(hStatusBar, SB_SETPARTS, 1, (long)&nParts);
    bInMenuLoop = TRUE;
    SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
}

static void OnExitMenuLoop(HWND hWnd)
{
    bInMenuLoop = FALSE;
    // Update the status bar pane sizes
	SetupStatusBar(hWnd, TRUE);
	UpdateStatusBar();
}

static void OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    TCHAR str[100];

    _tcscpy(str, _T(""));
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

void SetupStatusBar(HWND hWnd, BOOL bResize)
{
    RECT  rc;
    int nParts;
    GetClientRect(hWnd, &rc);
    nParts = rc.right;
//    nParts = -1;
	if (bResize)
		SendMessage(hStatusBar, WM_SIZE, 0, 0);
	SendMessage(hStatusBar, SB_SETPARTS, 1, (LPARAM)&nParts);
}

void UpdateStatusBar(void)
{
    TCHAR text[260];
	DWORD size;

	size = sizeof(text)/sizeof(TCHAR);
	GetComputerName(text, &size);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)text);
}

static void toggle_child(HWND hWnd, UINT cmd, HWND hchild)
{
	BOOL vis = IsWindowVisible(hchild);
	HMENU hMenuView = GetSubMenu(hMenuFrame, ID_VIEW_MENU);

	CheckMenuItem(hMenuView, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);
	ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);
	resize_frame_client(hWnd);
}

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    RECT rt;
    HDC hdc;

    GetClientRect(hWnd, &rt);
    hdc = BeginPaint(hWnd, &ps);
    FillRect(ps.hdc, &rt, GetStockObject(LTGRAY_BRUSH));
    EndPaint(hWnd, &ps);
}

static void SetListSytle(WORD view)
{

	HMENU hMenuView = GetSubMenu(hMenuFrame, ID_VIEW_MENU);
    dwListStyle = GetWindowLong(hListWnd, GWL_STYLE);
    dwListStyle &= ~(LVS_ICON | LVS_SMALLICON | LVS_LIST | LVS_REPORT);
	switch (view) {
    case ID_VIEW_LARGE_ICONS:
        dwListStyle |= LVS_ICON;
        break;
    case ID_VIEW_SMALL_ICONS:
        dwListStyle |= LVS_SMALLICON;
        break;
    case ID_VIEW_LIST:
        dwListStyle |= LVS_LIST;
        break;
    case ID_VIEW_DETAILS:
    default:
        dwListStyle |= LVS_REPORT;
        break;
    }
    SetWindowLong(hListWnd, GWL_STYLE, dwListStyle);
	CheckMenuItem(hMenuView, ID_VIEW_LARGE_ICONS, MF_BYCOMMAND);
	CheckMenuItem(hMenuView, ID_VIEW_SMALL_ICONS, MF_BYCOMMAND);
	CheckMenuItem(hMenuView, ID_VIEW_LIST,        MF_BYCOMMAND);
	CheckMenuItem(hMenuView, ID_VIEW_DETAILS,     MF_BYCOMMAND);
	CheckMenuItem(hMenuView, view, MF_BYCOMMAND|MF_CHECKED);
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
	switch (LOWORD(wParam)) {
    // Parse the menu selections:
    case ID_FILE_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        RefreshListView(hListWnd, NULL);
        break;
    case ID_VIEW_LARGE_ICONS:
    case ID_VIEW_SMALL_ICONS:
    case ID_VIEW_LIST:
    case ID_VIEW_DETAILS:
        SetListSytle(LOWORD(wParam));
        break;
	case ID_VIEW_STATUSBAR:
		toggle_child(hWnd, LOWORD(wParam), hStatusBar);
        break;
    case ID_HELP_ABOUT:
#ifdef WINSHELLAPI
        ShellAbout(hWnd, szTitle, _T(""), LoadIcon(hInst, (LPCTSTR)IDI_CONTROL));
#else
//        ShowAboutBox(hWnd);
#endif
        break;
    default:
        return FALSE;
    }
	return TRUE;
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
    static ChildWnd* pChildWnd = NULL;

    switch (message) {
    case WM_CREATE:
//        pChildWnd = (ChildWnd*)((LPCREATESTRUCT)lParam)->lpCreateParams;
//        ASSERT(pChildWnd);
//        pChildWnd->nSplitPos = 250;
//        pChildWnd->hTreeWnd = CreateTreeView(hWnd, pChildWnd->szPath, TREE_WINDOW);
//        pChildWnd->hListWnd = CreateListView(hWnd, LIST_WINDOW/*, pChildWnd->szPath*/);

        if (dwListStyle == 0) dwListStyle |= LVS_REPORT;
        hListWnd = CreateListView(hWnd, LIST_WINDOW, dwListStyle/*, pChildWnd->szPath*/);
        break;
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
   		    return DefWindowProc(hWnd, message, wParam, lParam);
        }
		break;
    case WM_PAINT:
        OnPaint(hWnd);
        return 0;
    case WM_SIZE:
        resize_frame_client(hWnd);
        break;
    case WM_TIMER:
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
        if (pChildWnd) {
            free(pChildWnd);
            pChildWnd = NULL;
        }
		WinHelp(hWnd, _T("control"), HELP_QUIT, 0);
        PostQuitMessage(0);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
