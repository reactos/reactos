/*
 *  ReactOS regedt32
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
    
#include <shellapi.h>

#include "main.h"
#include "framewnd.h"


////////////////////////////////////////////////////////////////////////////////
// Globals and Variables:
//

BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop

static HHOOK hcbthook;
static ChildWnd* newchild = NULL;

////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

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

static HWND InitChildWindow(LPTSTR param)
{
	//TCHAR drv[_MAX_DRIVE];
	TCHAR path[MAX_PATH];
	ChildWnd* pChildWnd = NULL;
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

static LRESULT _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hChildWnd;
    switch (LOWORD(wParam)) {
    case ID_WINDOW_CLOSEALL:
        EnumChildWindows(hMDIClient, &CloseEnumProc, 0);
        break;
    case ID_WINDOW_CLOSE:
        hChildWnd = (HWND) SendMessage(hMDIClient, WM_MDIGETACTIVE, 0, 0);
        if (!SendMessage(hChildWnd, WM_QUERYENDSESSION, 0, 0))
            SendMessage(hMDIClient, WM_MDIDESTROY, (WPARAM)hChildWnd, 0);
        break;
//        case ID_FILE_EXIT:
//            SendMessage(hWnd, WM_CLOSE, 0, 0);
//            break;
//        case IDM_EXIT:
//            DestroyWindow(hWnd);
//            break;
//        case ID_FILE_OPEN:
    case ID_REGISTRY_PRINTERSETUP:
        //PRINTDLG pd;
        //PrintDlg(&pd);
        //PAGESETUPDLG psd;
        //PageSetupDlg(&psd);
        break;
    case ID_REGISTRY_OPENLOCAL:
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
//        ShowAboutBox(hWnd);
        {
        HICON hIcon = LoadIcon(hInst, (LPCTSTR)IDI_REGEDT32);
        ShellAbout(hWnd, szTitle, "FrameWndProc", hIcon);
        //if (hIcon) DestroyIcon(hIcon); // NOT REQUIRED
        }
        break;
    default:
        hChildWnd = (HWND)SendMessage(hMDIClient, WM_MDIGETACTIVE, 0, 0);
        if (IsWindow(hChildWnd))
            SendMessage(hChildWnd, WM_COMMAND, wParam, lParam);
        else
            return DefFrameProc(hWnd, hMDIClient, message, wParam, lParam);
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
        hMDIClient = CreateWindowEx(0, _T("MDICLIENT"), NULL,
                    WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE|WS_BORDER,
                    0, 0, 0, 0,
                    hWnd, (HMENU)0, hInst, &ccs);
        }
	    break;
   	case WM_COMMAND:
    	return _CmdWndProc(hWnd, message, wParam, lParam);
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


