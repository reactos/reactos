/*
 *  ReactOS regedit
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
#include "treeview.h"
#include "listview.h"
#include <shellapi.h>


////////////////////////////////////////////////////////////////////////////////
// Globals and Variables:
//

static BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop

static HWND hChildWnd;

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
		SetupStatusBar(TRUE);
		GetClientRect(hStatusBar, &rt);
		prect->bottom -= rt.bottom;
	}
	MoveWindow(hChildWnd, prect->left-1,prect->top-1,prect->right+2,prect->bottom+1, TRUE);
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
    RECT  rc;
    int   nParts[3];
//    TCHAR text[260];

    bInMenuLoop = FALSE;
    // Update the status bar pane sizes
    GetClientRect(hWnd, &rc);
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = rc.right;
    SendMessage(hStatusBar, SB_SETPARTS, 3, (long)nParts);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)_T(""));
	SetupStatusBar(TRUE);
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

void SetupStatusBar(BOOL bResize)
{
    int nParts[4];
//		int parts[] = {300, 500};
//		SendMessage(Globals.hStatusBar, WM_SIZE, 0, 0);
//		SendMessage(Globals.hStatusBar, SB_SETPARTS, 2, (LPARAM)&parts);

    // Create the status bar panes
    nParts[0] = 150;
    nParts[1] = 220;
    nParts[2] = 100;
    nParts[3] = 100;
	if (bResize)
		SendMessage(hStatusBar, WM_SIZE, 0, 0);
    SendMessage(hStatusBar, SB_SETPARTS, 4, (long)nParts);
}

void UpdateStatusBar(void)
{
    TCHAR text[260];
	DWORD size;

//	size = sizeof(text)/sizeof(TCHAR);
//	GetUserName(text, &size);
//  SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)text);
	size = sizeof(text)/sizeof(TCHAR);
	GetComputerName(text, &size);
    SendMessage(hStatusBar, SB_SETTEXT, 3, (LPARAM)text);
}

static void toggle_child(HWND hWnd, UINT cmd, HWND hchild)
{
	BOOL vis = IsWindowVisible(hchild);
	HMENU hMenuView = GetSubMenu(hMenuFrame, ID_VIEW_MENU);

	CheckMenuItem(hMenuView, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);
	ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);
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
	switch (LOWORD(wParam)) {
    // Parse the menu selections:
    case ID_REGISTRY_PRINTERSETUP:
        //PRINTDLG pd;
        //PrintDlg(&pd);
        //PAGESETUPDLG psd;
        //PageSetupDlg(&psd);
        break;
    case ID_REGISTRY_OPENLOCAL:
/*
            {
            HWND hChildWnd;
//            hChildWnd = CreateWindow(szFrameClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CHILD,
//                                   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, NULL, hInst, NULL);
            hChildWnd = CreateWindow(szFrameClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CHILD,
                                     0, 0, 150, 170, hWnd, NULL, hInst, NULL);
            if (hChildWnd) {
                ShowWindow(hChildWnd, 1);
                UpdateWindow(hChildWnd);
            }
            }
 */
        break;
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        // TODO:
        break;
//	case ID_OPTIONS_TOOLBAR:
//		toggle_child(hWnd, LOWORD(wParam), hToolBar);
//      break;
	case ID_VIEW_STATUSBAR:
		toggle_child(hWnd, LOWORD(wParam), hStatusBar);
        break;
    case ID_HELP_HELPTOPICS:
//		WinHelp(hWnd, _T("regedit"), HELP_CONTENTS, 0);
		WinHelp(hWnd, _T("regedit"), HELP_FINDER, 0);
        break;
    case ID_HELP_ABOUT:
#ifdef WINSHELLAPI
        ShellAbout(hWnd, szTitle, _T(""), LoadIcon(hInst, (LPCTSTR)IDI_REGEDIT));
#else
        ShowAboutBox(hWnd);
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
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        {
//        HMENU hMenuWindow = GetSubMenu(hMenuFrame, GetMenuItemCount(hMenuFrame)-2);
        hChildWnd = CreateWindowEx(0, szChildClass, _T("regedit child window"),
//        hChildWnd = CreateWindowEx(0, (LPCTSTR)(int)hChildWndClass, _T("regedit child window"),
//                    WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE|WS_BORDER,
                    WS_CHILD|WS_VISIBLE|WS_BORDER,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    hWnd, (HMENU)0, hInst, NULL/*lpParam*/);
        }
        break;
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
   		    return DefWindowProc(hWnd, message, wParam, lParam);
        }
		break;
    case WM_SIZE:
        resize_frame_client(hWnd);
        break;
//        OnSize(wParam, lParam);
//        goto def;
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
		WinHelp(hWnd, _T("regedit"), HELP_QUIT, 0);
        PostQuitMessage(0);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
