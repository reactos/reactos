/*
 *  ReactOS Application
 *
 *  panelwnd.c
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
    
#include <assert.h>
#define ASSERT assert

#include "main.h"
#include "panelwnd.h"

////////////////////////////////////////////////////////////////////////////////

static void OnPaint(HWND hWnd, ChildWnd* pChildWnd)
{
    PAINTSTRUCT ps;
    RECT rt;
    GetClientRect(hWnd, &rt);
    BeginPaint(hWnd, &ps);

//    lastBrush = SelectObject(ps.hdc, (HBRUSH)GetStockObject(WHITE_BRUSH));
//    Rectangle(ps.hdc, rt.left, rt.top-1, rt.right, rt.bottom+1);
//    SelectObject(ps.hdc, lastBrush);
//    rt.top = rt.bottom - GetSystemMetrics(SM_CYHSCROLL);
    FillRect(ps.hdc, &rt, GetStockObject(BLACK_BRUSH));
/*
    rt.left = pChildWnd->nSplitPos-SPLIT_WIDTH/2;
    rt.right = pChildWnd->nSplitPos+SPLIT_WIDTH/2+1;
    lastBrush = SelectBrush(ps.hdc, (HBRUSH)GetStockObject(COLOR_SPLITBAR));
    Rectangle(ps.hdc, rt.left, rt.top-1, rt.right, rt.bottom+1);
    SelectObject(ps.hdc, lastBrush);
#ifdef _NO_EXTENSIONS
    rt.top = rt.bottom - GetSystemMetrics(SM_CYHSCROLL);
    FillRect(ps.hdc, &rt, GetStockObject(BLACK_BRUSH));
#endif
 */
    EndPaint(hWnd, &ps);
}


//
//  FUNCTION: ChildWndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the pChildWnd windows.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK PanelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
//  Pane* pane;
//  ChildWnd* pChildWnd = (ChildWnd*)GetWindowLong(hWnd, GWL_USERDATA);
//  ChildWnd* new_child;
//  ASSERT(pChildWnd);

    if (1) {
        switch(message) {
        case WM_CREATE:
            break;
        case WM_PAINT:
            OnPaint(hWnd, 0/*pChildWnd*/);
            break;
/*
        case WM_COMMAND:
            pane = GetFocus()==pChildWnd->left.hWnd? &pChildWnd->left: &pChildWnd->right;
            switch(LOWORD(wParam)) {
            case ID_WINDOW_NEW_WINDOW:
                break;
            default:
                return pane_command(pane, LOWORD(wParam));
            }
            break;
 */
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    return 0;
}
