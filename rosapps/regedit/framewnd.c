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
//#include <winspool.h>


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

HWND hTreeWnd;                   // Tree Control Window
HWND hListWnd;                   // List Control Window
HWND hSplitWnd;                  // Splitter Bar Control Window
int  nOldWidth;                  // Holds the previous client area width
int  nOldHeight;                 // Holds the previous client area height
BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void resize_frame_rect(HWND hWnd, PRECT prect)
{
//	int new_top;
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
		GetClientRect(hStatusBar, &rt);
		prect->bottom -= rt.bottom;
	}
//	MoveWindow(hWnd, prect->left-1,prect->top-1,prect->right+2,prect->bottom+1, TRUE);
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

static void draw_splitbar(HWND hWnd, int x)
{
	RECT rt;
	HDC hdc = GetDC(hWnd);

	GetClientRect(hWnd, &rt);
	if (IsWindowVisible(hStatusBar)) {
    	RECT rect;
		GetClientRect(hStatusBar, &rect);
		rt.bottom -= rect.bottom;
	}
	rt.left = x - SPLIT_WIDTH/2;
	rt.right = x + SPLIT_WIDTH/2+1;
	InvertRect(hdc, &rt);
	ReleaseDC(hWnd, hdc);
}

#define _NO_EXTENSIONS

static int nSplitPos = 250;

static void ResizeWnd(int cx, int cy)
{
	HDWP hdwp = BeginDeferWindowPos(2);
	RECT rt = {0, 0, cx, cy};

	cx = nSplitPos + SPLIT_WIDTH/2;
	if (IsWindowVisible(hStatusBar)) {
    	RECT rect;
		GetClientRect(hStatusBar, &rect);
		rt.bottom -= rect.bottom;
	}
    DeferWindowPos(hdwp, hTreeWnd, 0, rt.left, rt.top, nSplitPos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
	DeferWindowPos(hdwp, hListWnd, 0, rt.left+cx+1, rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
	EndDeferWindowPos(hdwp);
}

static void OnSize(WPARAM wParam, LPARAM lParam)
{
    if (wParam != SIZE_MINIMIZED) {
		ResizeWnd(LOWORD(lParam), HIWORD(lParam));
    }
}

/*
// OnSize()
// This function handles all the sizing events for the application
// It re-sizes every window, and child window that needs re-sizing
static void OnSize(UINT nType, int cx, int cy)
{
    int     nParts[3];
    int     nXDifference;
    int     nYDifference;
    RECT    rc;

    if (nType == SIZE_MINIMIZED)
        return;
    nXDifference = cx - nOldWidth;
    nYDifference = cy - nOldHeight;
    nOldWidth = cx;
    nOldHeight = cy;

    // Update the status bar size
    GetWindowRect(hStatusBar, &rc);
    SendMessage(hStatusBar, WM_SIZE, nType, MAKELPARAM(cx, cy + (rc.bottom - rc.top)));

    // Update the status bar pane sizes
    nParts[0] = bInMenuLoop ? -1 : 100;
    nParts[1] = 210;
    nParts[2] = cx;
    SendMessage(hStatusBar, SB_SETPARTS, bInMenuLoop ? 1 : 3, (long)nParts);
    GetWindowRect(hStatusBar, &rc);
    MoveWindow(hTreeWnd,0,0,cx/2,cy-(rc.bottom - rc.top),TRUE);
    MoveWindow(hListWnd,cx/2,0,cx,cy-(rc.bottom - rc.top),TRUE);

}
 */
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
//    TCHAR text[260];

    bInMenuLoop = FALSE;
    // Update the status bar pane sizes
    GetClientRect(hWnd, &rc);
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = rc.right;
    SendMessage(hStatusBar, SB_SETPARTS, 3, (long)nParts);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)_T(""));
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

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    wmId    = LOWORD(wParam); 
    wmEvent = HIWORD(wParam); 

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
		case ID_HELP_HELPTOPICS:
//			WinHelp(hWnd, _T("regedit"), HELP_CONTENTS, 0);
			WinHelp(hWnd, _T("regedit"), HELP_FINDER, 0);
            break;
        case ID_HELP_ABOUT:
#ifdef WINSHELLAPI
            ShellAbout(hWnd, szTitle, "", LoadIcon(hInst, (LPCTSTR)IDI_REGEDIT));
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
    static int last_split;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
    case WM_CREATE:
        //HWND CreateListView(HWND hwndParent/*, Pane* pane*/, int id, LPTSTR lpszPathName);
        hTreeWnd = CreateTreeView(hWnd, 1000, "c:\\foobar.txt");
        hListWnd = CreateListView(hWnd, 1001, "");
        break;
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
   		    return DefWindowProc(hWnd, message, wParam, lParam);
        }
		break;
    case WM_SIZE:
//        resize_frame_client(hWnd);
//        OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
//        break;
        OnSize(wParam, lParam);
		goto def;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        //RECT rt;
        //GetClientRect(hWnd, &rt);
        //DrawText(hdc, szHello, strlen(szHello), &rt, DT_CENTER);
        EndPaint(hWnd, &ps);
        break;
        break;
    case WM_TIMER:
        break;

		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT) {
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hWnd, &pt);
				if (pt.x>=nSplitPos-SPLIT_WIDTH/2 && pt.x<nSplitPos+SPLIT_WIDTH/2+1) {
					SetCursor(LoadCursor(0, IDC_SIZEWE));
					return TRUE;
				}
			}
			goto def;

		case WM_LBUTTONDOWN: {
			RECT rt;
			int x = LOWORD(lParam);

			GetClientRect(hWnd, &rt);
			if (x>=nSplitPos-SPLIT_WIDTH/2 && x<nSplitPos+SPLIT_WIDTH/2+1) {
				last_split = nSplitPos;
#ifdef _NO_EXTENSIONS
				draw_splitbar(hWnd, last_split);
#endif
				SetCapture(hWnd);
			}
			break;}

		case WM_LBUTTONUP:
			if (GetCapture() == hWnd) {
#ifdef _NO_EXTENSIONS
				RECT rt;
				int x = LOWORD(lParam);
				draw_splitbar(hWnd, last_split);
				last_split = -1;
				GetClientRect(hWnd, &rt);
				nSplitPos = x;
				ResizeWnd(rt.right, rt.bottom);
#endif
				ReleaseCapture();
			}
			break;

#ifdef _NO_EXTENSIONS
		case WM_CAPTURECHANGED:
			if (GetCapture()==hWnd && last_split>=0)
				draw_splitbar(hWnd, last_split);
			break;
#endif

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
				if (GetCapture() == hWnd) {
					RECT rt;
#ifdef _NO_EXTENSIONS
					draw_splitbar(hWnd, last_split);
#else
					nSplitPos = last_split;
#endif
					GetClientRect(hWnd, &rt);
					ResizeWnd(rt.right, rt.bottom);
					last_split = -1;
					ReleaseCapture();
					SetCursor(LoadCursor(0, IDC_ARROW));
				}
			break;

		case WM_MOUSEMOVE:
			if (GetCapture() == hWnd) {
				RECT rt;
				int x = LOWORD(lParam);

#ifdef _NO_EXTENSIONS
				HDC hdc = GetDC(hWnd);
				GetClientRect(hWnd, &rt);
				rt.left = last_split-SPLIT_WIDTH/2;
				rt.right = last_split+SPLIT_WIDTH/2+1;
				InvertRect(hdc, &rt);
				last_split = x;
				rt.left = x-SPLIT_WIDTH/2;
				rt.right = x+SPLIT_WIDTH/2+1;
				InvertRect(hdc, &rt);
				ReleaseDC(hWnd, hdc);
#else
				GetClientRect(hWnd, &rt);
				if (x>=0 && x<rt.right) {
					nSplitPos = x;
					ResizeWnd(rt.right, rt.bottom);
					rt.left = x-SPLIT_WIDTH/2;
					rt.right = x+SPLIT_WIDTH/2+1;
					InvalidateRect(hWnd, &rt, FALSE);
					UpdateWindow(hTreeWnd);
					UpdateWindow(hWnd);
					UpdateWindow(hListWnd);
				}
#endif
			}
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
    default: def:
        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
