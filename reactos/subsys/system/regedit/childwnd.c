/*
 * Regedit child window
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
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

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>

#include "main.h"

ChildWnd* g_pChildWnd;
HBITMAP SizingPattern = 0;
HBRUSH  SizingBrush = 0;

/*******************************************************************************
 * Local module support methods
 */

static LPCTSTR get_root_key_name(HKEY hRootKey)
{
    if (hRootKey == HKEY_CLASSES_ROOT) return _T("HKEY_CLASSES_ROOT");
    if (hRootKey == HKEY_CURRENT_USER) return _T("HKEY_CURRENT_USER");
    if (hRootKey == HKEY_LOCAL_MACHINE) return _T("HKEY_LOCAL_MACHINE");
    if (hRootKey == HKEY_USERS) return _T("HKEY_USERS");
    if (hRootKey == HKEY_CURRENT_CONFIG) return _T("HKEY_CURRENT_CONFIG");
    return _T("UKNOWN HKEY, PLEASE REPORT");
}

static void draw_splitbar(HWND hWnd, int x)
{
    RECT rt;
    HGDIOBJ OldObj;
    HDC hdc = GetDC(hWnd);
    
    if(!SizingPattern)
    {
      const DWORD Pattern[4] = {0x5555AAAA, 0x5555AAAA, 0x5555AAAA, 0x5555AAAA};
      SizingPattern = CreateBitmap(8, 8, 1, 1, Pattern);
    }
    if(!SizingBrush)
    {
      SizingBrush = CreatePatternBrush(SizingPattern);
    }
    GetClientRect(hWnd, &rt);
    rt.left = x - SPLIT_WIDTH/2;
    rt.right = x + SPLIT_WIDTH/2+1;
    OldObj = SelectObject(hdc, SizingBrush);
    PatBlt(hdc, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
    SelectObject(hdc, OldObj);
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
    FillRect(ps.hdc, &rt, GetSysColorBrush(COLOR_BTNFACE));
    EndPaint(hWnd, &ps);
}

/*******************************************************************************
 *
 *  FUNCTION: _CmdWndProc(HWND, unsigned, WORD, LONG)
 *
 *  PURPOSE:  Processes WM_COMMAND messages for the main frame window.
 *
 */

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ChildWnd* pChildWnd = g_pChildWnd;
    switch (LOWORD(wParam)) {
        /* Parse the menu selections: */
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        /* TODO */
        break;
    case ID_SWITCH_PANELS:
        pChildWnd->nFocusPanel = !pChildWnd->nFocusPanel;
        SetFocus(pChildWnd->nFocusPanel? pChildWnd->hListWnd: pChildWnd->hTreeWnd);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/*******************************************************************************
 *
 *  FUNCTION: ChildWndProc(HWND, unsigned, WORD, LONG)
 *
 *  PURPOSE:  Processes messages for the child windows.
 *
 *  WM_COMMAND  - process the application menu
 *  WM_PAINT    - Paint the main window
 *  WM_DESTROY  - post a quit message and return
 *
 */
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static short last_split;
    BOOL Result;
    ChildWnd* pChildWnd = g_pChildWnd;

    switch (message) {
    case WM_CREATE:
    {
        TCHAR buffer[MAX_PATH];
        /* load "My Computer" string */
        LoadString(hInst, IDS_MY_COMPUTER, buffer, sizeof(buffer)/sizeof(TCHAR));
        
	g_pChildWnd = pChildWnd = HeapAlloc(GetProcessHeap(), 0, sizeof(ChildWnd));
        if (!pChildWnd) return 0;
        _tcsncpy(pChildWnd->szPath, buffer, MAX_PATH);
        pChildWnd->nSplitPos = 250;
        pChildWnd->hWnd = hWnd;
        pChildWnd->hTreeWnd = CreateTreeView(hWnd, pChildWnd->szPath, TREE_WINDOW);
        pChildWnd->hListWnd = CreateListView(hWnd, LIST_WINDOW/*, pChildWnd->szPath*/);
        SetFocus(pChildWnd->hTreeWnd);
        break;
    }
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
        HeapFree(GetProcessHeap(), 0, pChildWnd);
        pChildWnd = NULL;
        PostQuitMessage(0);
        break;
    case WM_LBUTTONDOWN: {
            RECT rt;
            POINTS pt;
            pt = MAKEPOINTS(lParam);
            GetClientRect(hWnd, &rt);
            if (pt.x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && pt.x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
                last_split = pChildWnd->nSplitPos;
                draw_splitbar(hWnd, last_split);
                SetCapture(hWnd);
            }
            break;
        }

    case WM_LBUTTONUP:
        if (GetCapture() == hWnd) {
            RECT rt;
            POINTS pt;
            pt = MAKEPOINTS(lParam);
            GetClientRect(hWnd, &rt);
            pt.x = min(max(pt.x, SPLIT_MIN), rt.right - SPLIT_MIN);
            draw_splitbar(hWnd, last_split);
            last_split = -1;
            pChildWnd->nSplitPos = pt.x;
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
            HDC hdc;
            RECT rt;
            HGDIOBJ OldObj;
            POINTS pt;
            if(!SizingPattern)
            {
              const DWORD Pattern[4] = {0x5555AAAA, 0x5555AAAA, 0x5555AAAA, 0x5555AAAA};
              SizingPattern = CreateBitmap(8, 8, 1, 1, Pattern);
            }
            if(!SizingBrush)
            {
              SizingBrush = CreatePatternBrush(SizingPattern);
            }
            
            pt = MAKEPOINTS(lParam);
            GetClientRect(hWnd, &rt);
            pt.x = min(max(pt.x, SPLIT_MIN), rt.right - SPLIT_MIN);
            if(last_split != pt.x)
            {
              rt.left = last_split-SPLIT_WIDTH/2;
              rt.right = last_split+SPLIT_WIDTH/2+1;
              hdc = GetDC(hWnd);
              OldObj = SelectObject(hdc, SizingBrush);
              PatBlt(hdc, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
              last_split = pt.x;
              rt.left = pt.x-SPLIT_WIDTH/2;
              rt.right = pt.x+SPLIT_WIDTH/2+1;
              PatBlt(hdc, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
              SelectObject(hdc, OldObj);
              ReleaseDC(hWnd, hdc);
            }
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
            case TVN_SELCHANGED: {
                    LPCTSTR keyPath, rootName;
		    LPTSTR fullPath;
                    HKEY hRootKey;

		    keyPath = GetItemPath(pChildWnd->hTreeWnd, ((NMTREEVIEW*)lParam)->itemNew.hItem, &hRootKey);
		    if (keyPath) {
		        RefreshListView(pChildWnd->hListWnd, hRootKey, keyPath);
			rootName = get_root_key_name(hRootKey);
			fullPath = HeapAlloc(GetProcessHeap(), 0, (_tcslen(rootName) + 1 + _tcslen(keyPath) + 1) * sizeof(TCHAR));
			if (fullPath) {
			    _stprintf(fullPath, _T("%s\\%s"), rootName, keyPath);
			    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)fullPath);
			    HeapFree(GetProcessHeap(), 0, fullPath);
			}
		    }
                }
                break;
	    case NM_SETFOCUS:
		pChildWnd->nFocusPanel = 0;
		break;
            default:
                return 0;
            }
        } else
        {
            if ((int)wParam == LIST_WINDOW)
            {
		switch (((LPNMHDR)lParam)->code) {
		  case NM_SETFOCUS:
		  	pChildWnd->nFocusPanel = 1;
		  	break;
		  default:
                	if(!ListWndNotifyProc(pChildWnd->hListWnd, wParam, lParam, &Result))
                	{
                  		goto def;
                	}
                	break;
        	}
            }
        }
        break;
    
    case WM_CONTEXTMENU:
    {
      POINTS pt;
      if((HWND)wParam == pChildWnd->hListWnd)
      {
        int i, cnt;
        BOOL IsDefault;
        pt = MAKEPOINTS(lParam);
        cnt = ListView_GetSelectedCount(pChildWnd->hListWnd);
        i = ListView_GetNextItem(pChildWnd->hListWnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
        if(i == -1)
        {
          TrackPopupMenu(GetSubMenu(hPopupMenus, PM_NEW), TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
        }
        else
        {
          HMENU mnu = GetSubMenu(hPopupMenus, PM_MODIFYVALUE);
          SetMenuDefaultItem(mnu, ID_EDIT_MODIFY, MF_BYCOMMAND);
          IsDefault = IsDefaultValue(pChildWnd->hListWnd, i);
          if(cnt == 1)
            EnableMenuItem(mnu, ID_EDIT_RENAME, MF_BYCOMMAND | (IsDefault ? MF_DISABLED | MF_GRAYED : MF_ENABLED));
          else
            EnableMenuItem(mnu, ID_EDIT_RENAME, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
          EnableMenuItem(mnu, ID_EDIT_MODIFY, MF_BYCOMMAND | (cnt == 1 ? MF_ENABLED : MF_DISABLED | MF_GRAYED));
          EnableMenuItem(mnu, ID_EDIT_MODIFY_BIN, MF_BYCOMMAND | (cnt == 1 ? MF_ENABLED : MF_DISABLED | MF_GRAYED));
          
          TrackPopupMenu(mnu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
        }
      }
      break;
    }
    
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && pChildWnd != NULL) {
            ResizeWnd(pChildWnd, LOWORD(lParam), HIWORD(lParam));
        }
        /* fall through */
default: def:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
