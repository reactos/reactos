/*
 *  ReactOS regedit
 *
 *  childwnd.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include <assert.h>
#define ASSERT assert

#include "main.h"
#include "framewnd.h"
#include "childwnd.h"
#include "treeview.h"
#include "listview.h"


////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void MakeFullRegPath(HWND hwndTV, HTREEITEM hItem, LPTSTR keyPath, int* pPathLen, int max)
{
    TVITEM item;
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    if (TreeView_GetItem(hwndTV, &item)) {
        if (item.hItem != TreeView_GetRoot(hwndTV)) {
            // recurse
            MakeFullRegPath(hwndTV, TreeView_GetParent(hwndTV, hItem), keyPath, pPathLen, max);
            keyPath[*pPathLen] = _T('\\');
            ++(*pPathLen);
        }
        item.mask = TVIF_TEXT;
        item.hItem = hItem;
        item.pszText = &keyPath[*pPathLen];
        item.cchTextMax = max - *pPathLen;
        if (TreeView_GetItem(hwndTV, &item)) {
            *pPathLen += _tcslen(item.pszText);
        }
    }
}

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

static void ResizeWnd(ChildWnd* pChildWnd, int cx, int cy)
{
	HDWP hdwp = BeginDeferWindowPos(2);
	RECT rt = {0, 0, cx, cy};

	cx = pChildWnd->nSplitPos + SPLIT_WIDTH/2;
    DeferWindowPos(hdwp, pChildWnd->hTreeWnd, 0, rt.left, rt.top, pChildWnd->nSplitPos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
    DeferWindowPos(hdwp, pChildWnd->hListWnd, 0, rt.left+cx  , rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
	EndDeferWindowPos(hdwp);
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
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        // TODO:
        break;
    default:
        return FALSE;
    }
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
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
//    ChildWnd* pChildWnd = (ChildWnd*)GetWindowLong(hWnd, GWL_USERDATA);
    static ChildWnd* pChildWnd;

    switch (message) {
    case WM_CREATE:
        pChildWnd = (ChildWnd*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        ASSERT(pChildWnd);
        pChildWnd->nSplitPos = 250;
        pChildWnd->hTreeWnd = CreateTreeView(hWnd, pChildWnd->szPath, TREE_WINDOW);
        pChildWnd->hListWnd = CreateListView(hWnd, LIST_WINDOW/*, pChildWnd->szPath*/);
        break;
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
            goto def;
        }
		break;
    case WM_PAINT:
        OnPaint(hWnd);
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
		goto def;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_LBUTTONDOWN: {
		RECT rt;
		int x = LOWORD(lParam);
		GetClientRect(hWnd, &rt);
		if (x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
			last_split = pChildWnd->nSplitPos;
			draw_splitbar(hWnd, last_split);
			SetCapture(hWnd);
		}
		break;}

	case WM_LBUTTONUP:
		if (GetCapture() == hWnd) {
			RECT rt;
			int x = LOWORD(lParam);
			draw_splitbar(hWnd, last_split);
			last_split = -1;
			GetClientRect(hWnd, &rt);
			pChildWnd->nSplitPos = x;
			ResizeWnd(pChildWnd, rt.right, rt.bottom);
			ReleaseCapture();
		}
		break;

	case WM_CAPTURECHANGED:
		if (GetCapture()==hWnd && last_split>=0)
			draw_splitbar(hWnd, last_split);
		break;

    case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			if (GetCapture() == hWnd) {
				RECT rt;
				draw_splitbar(hWnd, last_split);
				GetClientRect(hWnd, &rt);
                ResizeWnd(pChildWnd, rt.right, rt.bottom);
				last_split = -1;
				ReleaseCapture();
				SetCursor(LoadCursor(0, IDC_ARROW));
			}
		break;

	case WM_MOUSEMOVE:
		if (GetCapture() == hWnd) {
			RECT rt;
			int x = LOWORD(lParam);
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
		}
		break;

	case WM_SETFOCUS:
        if (pChildWnd != NULL) {
		    SetFocus(pChildWnd->nFocusPanel? pChildWnd->hListWnd: pChildWnd->hTreeWnd);
        }
		break;

    case WM_TIMER:
        break;

	case WM_NOTIFY:
        if ((int)wParam == TREE_WINDOW) {
            switch (((LPNMHDR)lParam)->code) { 
            case TVN_ITEMEXPANDING: 
                return !OnTreeExpanding(pChildWnd->hTreeWnd, (NMTREEVIEW*)lParam);
            case TVN_SELCHANGED:
                {
                    HKEY hKey;
                    TCHAR keyPath[1000];
                    int keyPathLen = 0;
                    keyPath[0] = _T('\0');
                    hKey = FindRegRoot(pChildWnd->hTreeWnd, ((NMTREEVIEW*)lParam)->itemNew.hItem, keyPath, &keyPathLen, sizeof(keyPath)/sizeof(TCHAR));
                    RefreshListView(pChildWnd->hListWnd, hKey, keyPath);

                    keyPathLen = 0;
                    keyPath[0] = _T('\0');
                    MakeFullRegPath(pChildWnd->hTreeWnd, ((NMTREEVIEW*)lParam)->itemNew.hItem, keyPath, &keyPathLen, sizeof(keyPath)/sizeof(TCHAR));
                    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)keyPath);
                }
                break;
            default:
                goto def;
            }
        } else
        if ((int)wParam == LIST_WINDOW) {
            if (!SendMessage(pChildWnd->hListWnd, message, wParam, lParam)) {
                goto def;
            }
        }
        break;

	case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && pChildWnd != NULL) {
	    	ResizeWnd(pChildWnd, LOWORD(lParam), HIWORD(lParam));
        }
        // fall through
    default: def:
        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
