/*
 *  ReactOS winfile
 *
 *  winfile.c
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

#include "winfile.h"
#include "about.h"
#include "run.h"
#include "treeview.h"
#include "listview.h"
#include "mdiclient.h"
#include "subframe.h"
#include "format.h"



////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

SORT_ORDER SortOrder = SORT_NAME;

HWND hTreeWnd;                   // Tree Control Window
HWND hListWnd;                   // List Control Window
HWND hSplitWnd;                  // Splitter Bar Control Window

int  nOldWidth;                  // Holds the previous client area width
int  nOldHeight;                 // Holds the previous client area height

BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop


////////////////////////////////////////////////////////////////////////////////

// OnSize()
// This function handles all the sizing events for the application
// It re-sizes every window, and child window that needs re-sizing
void OnSize(UINT nType, int cx, int cy)
{
    if (nType == SIZE_MINIMIZED)
        return;
#if 1
	resize_frame_client(Globals.hMainWnd);
#else
    int     nParts[3];
    int     nXDifference;
    int     nYDifference;
    RECT    rc;

    nXDifference = cx - nOldWidth;
    nYDifference = cy - nOldHeight;
    nOldWidth = cx;
    nOldHeight = cy;

    // Update the status bar size
    GetWindowRect(Globals.hStatusBar, &rc);
    SendMessage(Globals.hStatusBar, WM_SIZE, nType, MAKELPARAM(cx, cy + (rc.bottom - rc.top)));

    // Update the status bar pane sizes
    nParts[0] = bInMenuLoop ? -1 : 100;
    nParts[1] = 210;
    nParts[2] = cx;
    SendMessage(Globals.hStatusBar, SB_SETPARTS, bInMenuLoop ? 1 : 3, (long)nParts);

    GetWindowRect(Globals.hStatusBar, &rc);

//    MoveWindow(hTreeWnd,0,0,cx/2,cy-(rc.bottom - rc.top),TRUE);
//    MoveWindow(hListWnd,cx/2,0,cx,cy-(rc.bottom - rc.top),TRUE);

//    MoveWindow(Globals.hMDIClient,0,0,cx/2,cy-(rc.bottom - rc.top),TRUE);
    ShowWindow(Globals.hMDIClient, SW_SHOW);

//    ShowWindow(hTreeWnd, SW_HIDE);
//    ShowWindow(hListWnd, SW_HIDE);
#endif
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
//    TCHAR text[260];

    bInMenuLoop = FALSE;
    // Update the status bar pane sizes
    GetClientRect(hWnd, &rc);
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = rc.right;
    SendMessage(Globals.hStatusBar, SB_SETPARTS, 3, (long)nParts);
    SendMessage(Globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)_T(""));
//    wsprintf(text, _T("CPU Usage: %3d%%"), PerfDataGetProcessorUsage());
//    SendMessage(Globals.hStatusBar, SB_SETTEXT, 1, (LPARAM)text);
//    wsprintf(text, _T("Processes: %d"), PerfDataGetProcessCount());
//    SendMessage(Globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)text);
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

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwndClient;
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;
    //TCHAR szHello[MAX_LOADSTRING];
    //LoadString(Globals.hInstance, IDS_HELLO, szHello, MAX_LOADSTRING);

    switch (message) {
    case WM_COMMAND:
        wmId    = LOWORD(wParam); 
        wmEvent = HIWORD(wParam); 
		
		hwndClient = (HWND)SendMessage(Globals.hMDIClient, WM_MDIGETACTIVE, 0, 0);
		if (hwndClient) 
		    if (SendMessage(hwndClient, WM_DISPATCH_COMMAND, wParam, lParam))
			    break;

        // Parse the menu selections:
        switch (wmId) {

        case ID_FILE_RUN:
            OnFileRun();
            break;

			/*
			FormatDisk
			*/

		case ID_VIEW_SORT_BY_NAME:
			SortOrder = SORT_NAME;
			break;
		case ID_VIEW_SORT_BY_TYPE:
			SortOrder = SORT_EXT;
			break;
		case ID_VIEW_SORT_BY_SIZE:
			SortOrder = SORT_SIZE;
			break;
		case ID_VIEW_SORT_BY_DATE:
			SortOrder = SORT_DATE;
			break;

		case ID_OPTIONS_TOOLBAR:
			toggle_child(hWnd, wmId, Globals.hToolBar);
			break;
		case ID_OPTIONS_DRIVEBAR:
			toggle_child(hWnd, wmId, Globals.hDriveBar);
			break;
		case ID_OPTIONS_STATUSBAR:
			toggle_child(hWnd, wmId, Globals.hStatusBar);
			break;
#if 0
        case ID_REGISTRY_OPENLOCAL:
            {
            HWND hChildWnd;
//            hChildWnd = CreateWindow(szChildClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CHILD,
//                                   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, NULL, Globals.hInstance, NULL);
            hChildWnd = CreateWindow(szChildClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CHILD,
                                     0, 0, 150, 170, hWnd, NULL, Globals.hInstance, NULL);
            if (hChildWnd) {
                ShowWindow(hChildWnd, 1);
                UpdateWindow(hChildWnd);
            }
            }
            break;
#endif
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

        case ID_HELP_ABOUT:
#if 1
            ShowAboutBox(hWnd);
#else
            {
            HICON hIcon = LoadIcon(Globals.hInstance, (LPCTSTR)IDI_WINFILE);
            ShellAbout(hWnd, szTitle, "", hIcon);
            //if (hIcon) DestroyIcon(hIcon); // NOT REQUIRED
            }
#endif
            break;
        case ID_FILE_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
#if 0
            if (SendMessage(Globals.hMDIClient, message, wParam, lParam) != 0) {
                //return DefWindowProc(hWnd, message, wParam, lParam);
                return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
            }
#else
            //return DefWindowProc(hWnd, message, wParam, lParam);
			return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
#endif
            break;
        }
        break;
    case WM_SIZE:
        // Handle the window sizing in it's own function
        OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        GetClientRect(hWnd, &rc);
        //DrawText(hdc, szHello, strlen(szHello), &rt, DT_CENTER);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
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
    default:
        //return DefWindowProc(hWnd, message, wParam, lParam);
		return DefFrameProc(hWnd, Globals.hMDIClient, message, wParam, lParam);
   }
   return 0;
}
