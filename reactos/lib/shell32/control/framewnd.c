/*
 *  ReactOS shell32 - Control Panel Frame Window implementation
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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <stdlib.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include "control.h"
#include "framewnd.h"
#include "settings.h"
#include "listview.h"
//#include <shellapi.h>
#include "trace.h"


#ifdef __GNUC__
int WINAPI ShellAboutA(HWND,LPCSTR,LPCSTR,HICON);
int WINAPI ShellAboutW(HWND,LPCWSTR,LPCWSTR,HICON);
#else
int ShellAboutA(HWND,LPCSTR,LPCSTR,HICON);
int ShellAboutW(HWND,LPCWSTR,LPCWSTR,HICON);
#endif


////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

DWORD nListStyle;
DWORD nSortOrder;

static int nSelectedItem;
static HWND hListWnd;
static BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop

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

static void OnExitMenuLoop(HWND hwndLV, HWND hWnd)
{
    bInMenuLoop = FALSE;
    // Update the status bar pane sizes
	SetupStatusBar(hWnd, TRUE);
	UpdateStatusBar(hwndLV, -1);
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

void UpdateStatusBar(HWND hwndLV, int list_index)
{
    static int last_list_index;
    TCHAR buffer[MAX_CPL_INFO];
    LVITEM item;

    if (list_index == -1) list_index = last_list_index;
    last_list_index = list_index;
    //LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam; 
    //CPlEntry* pCPlEntry pCPLInfo = pnmv->lParam
    item.mask = LVIF_TEXT; 
    item.iItem = last_list_index; 
    item.pszText = buffer; 
    item.cchTextMax = MAX_CPL_INFO;
    item.iSubItem = 1; 
    if (ListView_GetItem(hwndLV, &item)) {
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)buffer);
    }
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

static void SetListSytle(DWORD view)
{
	HMENU hMenuView = GetSubMenu(hMenuFrame, ID_VIEW_MENU);
    DWORD dwListStyle = GetWindowLong(hListWnd, GWL_STYLE);
    dwListStyle &= ~(LVS_ICON | LVS_SMALLICON | LVS_LIST | LVS_REPORT);
    nListStyle = view;
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
    default:
        nListStyle = ID_VIEW_DETAILS;
    case ID_VIEW_DETAILS:
        dwListStyle |= LVS_REPORT;
        break;
    }
    SetWindowLong(hListWnd, GWL_STYLE, dwListStyle);
	CheckMenuItem(hMenuView, ID_VIEW_LARGE_ICONS, MF_BYCOMMAND);
	CheckMenuItem(hMenuView, ID_VIEW_SMALL_ICONS, MF_BYCOMMAND);
	CheckMenuItem(hMenuView, ID_VIEW_LIST,        MF_BYCOMMAND);
	CheckMenuItem(hMenuView, ID_VIEW_DETAILS,     MF_BYCOMMAND);
	CheckMenuItem(hMenuView, nListStyle, MF_BYCOMMAND|MF_CHECKED);
}

void SetViewArrangement(DWORD cmd)
{
    DWORD dwListStyle = GetWindowLong(hListWnd, GWL_STYLE);
    HMENU hMenuView = GetSubMenu(hMenuFrame, ID_VIEW_MENU);
    nSortOrder = cmd;
    dwListStyle &= ~(LVS_AUTOARRANGE);
	switch (cmd) {
    case ID_VIEW_ARRANGE_BY_NAME:
        SortListView(hListWnd, 0);
        break;
    case ID_VIEW_ARRANGE_BY_COMMENT:
        SortListView(hListWnd, 1);
        break;
    default:
        nSortOrder = ID_VIEW_ARRANGE_AUTO;
    case ID_VIEW_ARRANGE_AUTO:
        SortListView(hListWnd, -1);
        dwListStyle |= LVS_AUTOARRANGE;
        break;
    }
    SetWindowLong(hListWnd, GWL_STYLE, dwListStyle);
	CheckMenuItem(hMenuView, ID_VIEW_ARRANGE_BY_NAME, MF_BYCOMMAND);
	CheckMenuItem(hMenuView, ID_VIEW_ARRANGE_BY_COMMENT, MF_BYCOMMAND);
	CheckMenuItem(hMenuView, ID_VIEW_ARRANGE_AUTO, MF_BYCOMMAND);
	CheckMenuItem(hMenuView, nSortOrder, MF_BYCOMMAND|MF_CHECKED);
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: _CmdWndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes WM_COMMAND messages for the main frame window.
//
//

static void OnFileOpen(HWND hWnd)
{
    LVITEM item;

    item.mask = LVIF_PARAM; 
    item.iItem = nSelectedItem; 
    item.iSubItem = 0; 
    if (ListView_GetItem(hListWnd, &item)) {
        Control_LaunchApplet(hListWnd, (CPlEntry*)item.lParam);
    }
}

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
    // Parse the menu selections:
    case ID_FILE_OPEN:
        OnFileOpen(hWnd);
        break;
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
    case ID_VIEW_ARRANGE_BY_NAME:
    case ID_VIEW_ARRANGE_BY_COMMENT:
    case ID_VIEW_ARRANGE_AUTO:
        SetViewArrangement(LOWORD(wParam));
        break;
    case ID_VIEW_LINE_UP_ICONS:
        ListView_Arrange(hListWnd, LVA_DEFAULT );
        break;
    case ID_HELP_ABOUT:
#ifdef UNICODE
        ShellAboutW(hWnd, szTitle, L"", LoadIcon(hInst, (LPCTSTR)IDI_CONTROL));
#else
        ShellAboutA(hWnd, szTitle, "", LoadIcon(hInst, (LPCTSTR)IDI_CONTROL));
#endif
        break;
    default:
        return FALSE;
    }
	return TRUE;
}

extern CPlApplet* pListHead; // holds pointer to linked list of cpl modules CPlApplet*

static void OnLeftClick(HWND hWnd, NMITEMACTIVATE* nmitem)
{
    //HMENU hMenu = NULL;
    LVHITTESTINFO info;

    info.pt.x = nmitem->ptAction.x;
    info.pt.y = nmitem->ptAction.y;
    if (ListView_HitTest(hListWnd, &info) != -1) {
        LVITEM item;
        item.mask = LVIF_PARAM;
        item.iItem = info.iItem;
        if (ListView_GetItem(hListWnd, &item)) {
            //Control_LaunchApplet(hWnd, (CPlEntry*)item.lParam);
            //hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTROL_CONTEXT_APPLET));
        }
    } else {
        TCHAR buffer[MAX_CPL_INFO];
        int obj_count = ListView_GetItemCount(hListWnd);
        int cpl_count = 0;
        int cpl_locked_count = 0;
        CPlApplet* applet = pListHead;
        while (applet) {
            ++cpl_count;
            if (applet->hModule) ++cpl_locked_count;
            applet = applet->next;
        }


        TRACE(_T("OnLeftClick(0x%08X) - %u\n"), hWnd, obj_count);

        wsprintf(buffer, _T("%u applets in %u libraries (%u locked)"), obj_count, cpl_count, cpl_locked_count);
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)buffer);
    }
}

static void OnRightClick(HWND hWnd, NMITEMACTIVATE* nmitem)
{
    HMENU hMenu = NULL;
    LVHITTESTINFO info;
    info.pt.x = nmitem->ptAction.x;
    info.pt.y = nmitem->ptAction.y;
    if (ListView_HitTest(hListWnd, &info) != -1) {
        LVITEM item;
        item.mask = LVIF_PARAM;
        item.iItem = info.iItem;
        if (ListView_GetItem(hListWnd, &item)) {
            //Control_LaunchApplet(hWnd, (CPlEntry*)item.lParam);
            hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTROL_CONTEXT_APPLET));
        }
    }
    if (!hMenu) {
        hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTROL_CONTEXT));
    }
    if (hMenu) {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu) {
            POINT pt;
            GetCursorPos(&pt);
            TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        }
        DestroyMenu(hMenu);
    }
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
    RECT rect;

    switch (message) {
    case WM_CREATE:
        hListWnd = CreateListView(hWnd, LIST_WINDOW);
        SetListSytle(nListStyle);
        SetViewArrangement(nSortOrder);
        break;
    case WM_NOTIFY:
        {
        NMITEMACTIVATE* nmitem = (LPNMITEMACTIVATE)lParam;
        if (nmitem->hdr.idFrom == LIST_WINDOW) {
            switch (((LPNMHDR)lParam)->code) { 
            case LVN_ITEMCHANGED:
                nSelectedItem = ((LPNMLISTVIEW)lParam)->iItem;
                UpdateStatusBar(hListWnd, ((LPNMLISTVIEW)lParam)->iItem);
                break;
            //case NM_DBLCLK:
                //OnDblClick(hWnd, nmitem);
                //break;
            case NM_RETURN:
                OnFileOpen(hWnd);
                break;
            case NM_CLICK:
                OnLeftClick(hWnd, nmitem);
                break; 
            case NM_RCLICK:
                OnRightClick(hWnd, nmitem);
                break; 
            //default:
                //return FALSE;
            }
            if (ListViewNotifyProc(hListWnd, message, wParam, lParam))
                return TRUE;
        }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
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
        //if (nSortOrder == ID_VIEW_ARRANGE_AUTO) SetViewArrangement(nSortOrder);
        //if (nSortOrder == ID_VIEW_ARRANGE_AUTO) ListView_Update(hListWnd, 0);
        break;
    case WM_TIMER:
        break;
    case WM_ENTERMENULOOP:
        OnEnterMenuLoop(hWnd);
        break;
    case WM_EXITMENULOOP:
        OnExitMenuLoop(hListWnd, hWnd);
        break;
    case WM_MENUSELECT:
        OnMenuSelect(hWnd, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;
    case WM_DESTROY:
        GetWindowRect(hWnd, &rect);
        SaveSettings(&rect);
        DestroyListView(hListWnd);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
