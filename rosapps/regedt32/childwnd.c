/*
 *  ReactOS regedt32
 *
 *  childwnd.c
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
#include "childwnd.h"

////////////////////////////////////////////////////////////////////////////////

static void draw_splitbar(HWND hWnd, int x)
{
	RECT rt;
	HDC hdc = GetDC(hWnd);

	GetClientRect(hWnd, &rt);
	rt.left = x - SPLIT_WIDTH/2;
	rt.right = x + SPLIT_WIDTH/2+1;
	InvertRect(hdc, &rt);
	ReleaseDC(hWnd, hdc);
}


static void OnPaint(HWND hWnd, ChildWnd* pChildWnd)
{
    PAINTSTRUCT ps;
    RECT rt;
    HDC hdc;

    GetClientRect(hWnd, &rt);
    hdc = BeginPaint(hWnd, &ps);
    FillRect(ps.hdc, &rt, GetStockObject(BLACK_BRUSH));
    EndPaint(hWnd, &ps);
}


static void OnSize(ChildWnd* pChildWnd, WPARAM wParam, LPARAM lParam)
{
    if (wParam != SIZE_MINIMIZED) {
		//resize_tree(pChildWnd, LOWORD(lParam), HIWORD(lParam));
    }
}

//
//  FUNCTION: ChildWndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the child windows.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int last_split;
    ChildWnd* pChildWnd = (ChildWnd*)GetWindowLong(hWnd, GWL_USERDATA);
    ASSERT(pChildWnd);

    switch (message) {
    case WM_PAINT:
        OnPaint(hWnd, pChildWnd);
        return 0;
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hWnd, &pt);
			if (pt.x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && pt.x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
				SetCursor(LoadCursor(0, IDC_SIZEWE));
				return TRUE;
			}
		}
		//goto def;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_LBUTTONDOWN: {
		RECT rt;
		int x = LOWORD(lParam);
		GetClientRect(hWnd, &rt);
		if (x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
			last_split = pChildWnd->nSplitPos;
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
			pChildWnd->nSplitPos = x;
			//resize_tree(pChildWnd, rt.right, rt.bottom);
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
				pChildWnd->nSplitPos = last_split;
#endif
				GetClientRect(hWnd, &rt);
				//resize_tree(pChildWnd, rt.right, rt.bottom);
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
				pChildWnd->nSplitPos = x;
				//resize_tree(pChildWnd, rt.right, rt.bottom);
				rt.left = x-SPLIT_WIDTH/2;
				rt.right = x+SPLIT_WIDTH/2+1;
				InvalidateRect(hWnd, &rt, FALSE);
				UpdateWindow(pChildWnd->hLeftWnd);
				UpdateWindow(hWnd);
				UpdateWindow(pChildWnd->hRightWnd);
			}
#endif
		}
		break;

#ifndef _NO_EXTENSIONS
	case WM_GETMINMAXINFO:
		DefMDIChildProc(hWnd, message, wParam, lParam);
		{LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
		lpmmi->ptMaxTrackSize.x <<= 1;//2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN
		lpmmi->ptMaxTrackSize.y <<= 1;//2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN
		break;}
#endif

	case WM_SETFOCUS:
		SetCurrentDirectory(pChildWnd->szPath);
		SetFocus(pChildWnd->nFocusPanel? pChildWnd->hRightWnd: pChildWnd->hLeftWnd);
		break;

    case WM_TIMER:
        break;

    case WM_COMMAND:
        // Parse the menu selections:
        switch (LOWORD(wParam)) {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefMDIChildProc(hWnd, message, wParam, lParam);
        }
        break;

	case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            OnSize(pChildWnd, wParam, lParam);
        }
        // fall through
	default:
        return DefMDIChildProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
