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
#include <tchar.h>
#include <commctrl.h>
#include <assert.h>
#define ASSERT assert

#include "main.h"


/*******************************************************************************
 * Local module support methods
 */

static void MakeFullRegPath(HWND hwndTV, HTREEITEM hItem, LPTSTR keyPath, int* pPathLen, int max)
{
    TVITEM item;
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    if (TreeView_GetItem(hwndTV, &item)) {
        if (item.hItem != TreeView_GetRoot(hwndTV)) {
            /* recurse */
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
	switch (LOWORD(wParam)) {
    /* Parse the menu selections: */
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        /* TODO */
        break;
    default:
        return FALSE;
    }
	return TRUE;
}

struct {
   char *Name;
   ULONG Id;
} Messages[] =
{
   {"WM_ACTIVATE", WM_ACTIVATE},
   {"WM_ACTIVATEAPP", WM_ACTIVATEAPP},
   {"WM_ASKCBFORMATNAME", WM_ASKCBFORMATNAME},
   {"WM_CANCELJOURNAL", WM_CANCELJOURNAL},
   {"WM_CANCELMODE", WM_CANCELMODE},
   {"WM_CAPTURECHANGED", 533},
   {"WM_CHANGECBCHAIN", 781},
   {"WM_CHAR", 258},
   {"WM_CHARTOITEM", 47},
   {"WM_CHILDACTIVATE", 34},
   {"WM_CLEAR", 771},
   {"WM_CLOSE", 16},
   {"WM_COMMAND", 273},
   {"WM_COMMNOTIFY", 68},
   {"WM_COMPACTING", 65},
   {"WM_COMPAREITEM", 57},
   {"WM_CONTEXTMENU", 123},
   {"WM_COPY", 769},
   {"WM_COPYDATA", 74},
   {"WM_CREATE", 1},
   {"WM_CTLCOLORBTN", 309},
   {"WM_CTLCOLORDLG", 310},
   {"WM_CTLCOLOREDIT", 307},
   {"WM_CTLCOLORLISTBOX", 308},
   {"WM_CTLCOLORMSGBOX", 306},
   {"WM_CTLCOLORSCROLLBAR", 311},
   {"WM_CTLCOLORSTATIC", 312},
   {"WM_CUT", 768},
   {"WM_DEADCHAR", 259},
   {"WM_DELETEITEM", 45},
   {"WM_DESTROY", 2},
   {"WM_DESTROYCLIPBOARD", 775},
   {"WM_DEVICECHANGE", 537},
   {"WM_DEVMODECHANGE", 27},
   {"WM_DISPLAYCHANGE", 126},
   {"WM_DRAWCLIPBOARD", 776},
   {"WM_DRAWITEM", 43},
   {"WM_DROPFILES", 563},
   {"WM_ENABLE", 10},
   {"WM_ENDSESSION", 22},
   {"WM_ENTERIDLE", 289},
   {"WM_ENTERMENULOOP", 529},
   {"WM_ENTERSIZEMOVE", 561},
   {"WM_ERASEBKGND", 20},
   {"WM_EXITMENULOOP", 530},
   {"WM_EXITSIZEMOVE", 562},
   {"WM_FONTCHANGE", 29},
   {"WM_GETDLGCODE", 135},
   {"WM_GETFONT", 49},
   {"WM_GETHOTKEY", 51},
   {"WM_GETICON", 127},
   {"WM_GETMINMAXINFO", 36},
   {"WM_GETTEXT", 13},
   {"WM_GETTEXTLENGTH", 14},
   {"WM_HANDHELDFIRST", 856},
   {"WM_HANDHELDLAST", 863},
   {"WM_HELP", 83},
   {"WM_HOTKEY", 786},
   {"WM_HSCROLL", 276},
   {"WM_HSCROLLCLIPBOARD", 782},
   {"WM_ICONERASEBKGND", 39},
   {"WM_INITDIALOG", 272},
   {"WM_INITMENU", 278},
   {"WM_INITMENUPOPUP", 279},
   {"WM_INPUTLANGCHANGE", 81},
   {"WM_INPUTLANGCHANGEREQUEST", 80},
   {"WM_KEYDOWN", 256},
   {"WM_KEYUP", 257},
   {"WM_KILLFOCUS", 8},
   {"WM_MDIACTIVATE", 546},
   {"WM_MDICASCADE", 551},
   {"WM_MDICREATE", 544},
   {"WM_MDIDESTROY", 545},
   {"WM_MDIGETACTIVE", 553},
   {"WM_MDIICONARRANGE", 552},
   {"WM_MDIMAXIMIZE", 549},
   {"WM_MDINEXT", 548},
   {"WM_MDIREFRESHMENU", 564},
   {"WM_MDIRESTORE", 547},
   {"WM_MDISETMENU", 560},
   {"WM_MDITILE", 550},
   {"WM_MEASUREITEM", 44},
   {"WM_MENURBUTTONUP", 290},
   {"WM_MENUCHAR", 288},
   {"WM_MENUSELECT", 287},
   {"WM_NEXTMENU", 531},
   {"WM_MOVE", 3},
   {"WM_MOVING", 534},
   {"WM_NCACTIVATE", 134},
   {"WM_NCCALCSIZE", 131},
   {"WM_NCCREATE", 129},
   {"WM_NCDESTROY", 130},
   {"WM_NCHITTEST", 132},
   {"WM_NCLBUTTONDBLCLK", 163},
   {"WM_NCLBUTTONDOWN", 161},
   {"WM_NCLBUTTONUP", 162},
   {"WM_NCMBUTTONDBLCLK", 169},
   {"WM_NCMBUTTONDOWN", 167},
   {"WM_NCMBUTTONUP", 168},
   {"WM_NCMOUSEMOVE", 160},
   {"WM_NCPAINT", 133},
   {"WM_NCRBUTTONDBLCLK", 166},
   {"WM_NCRBUTTONDOWN", 164},
   {"WM_NCRBUTTONUP", 165},
   {"WM_NEXTDLGCTL", 40},
   {"WM_NEXTMENU", 531},
   {"WM_NOTIFY", 78},
   {"WM_NOTIFYFORMAT", 85},
   {"WM_NULL", 0},
   {"WM_PAINT", 15},
   {"WM_PAINTCLIPBOARD", 777},
   {"WM_PAINTICON", 38},
   {"WM_PALETTECHANGED", 785},
   {"WM_PALETTEISCHANGING", 784},
   {"WM_PARENTNOTIFY", 528},
   {"WM_PASTE", 770},
   {"WM_PENWINFIRST", 896},
   {"WM_PENWINLAST", 911},
   {"WM_POWER", 72},
   {"WM_POWERBROADCAST", 536},
   {"WM_PRINT", 791},
   {"WM_PRINTCLIENT", 792},
   {"WM_QUERYDRAGICON", 55},
   {"WM_QUERYENDSESSION", 17},
   {"WM_QUERYNEWPALETTE", 783},
   {"WM_QUERYOPEN", 19},
   {"WM_QUEUESYNC", 35},
   {"WM_QUIT", 18},
   {"WM_RENDERALLFORMATS", 774},
   {"WM_RENDERFORMAT", 773},
   {"WM_SETCURSOR", 32},
   {"WM_SETFOCUS", 7},
   {"WM_SETFONT", 48},
   {"WM_SETHOTKEY", 50},
   {"WM_SETICON", 128},
   {"WM_SETREDRAW", 11},
   {"WM_SETTEXT", 12},
   {"WM_SETTINGCHANGE", 26},
   {"WM_SHOWWINDOW", 24},
   {"WM_SIZE", 5},
   {"WM_SIZECLIPBOARD", 779},
   {"WM_SIZING", 532},
   {"WM_SPOOLERSTATUS", 42},
   {"WM_STYLECHANGED", 125},
   {"WM_STYLECHANGING", 124},
   {"WM_SYSCHAR", 262},
   {"WM_SYSCOLORCHANGE", 21},
   {"WM_SYSCOMMAND", 274},
   {"WM_SYSDEADCHAR", 263},
   {"WM_SYSKEYDOWN", 260},
   {"WM_SYSKEYUP", 261},
   {"WM_TCARD", 82},
   {"WM_TIMECHANGE", 30},
   {"WM_TIMER", 275},
   {"WM_UNDO", 772},
   {"WM_USER", 1024},
   {"WM_USERCHANGED", 84},
   {"WM_VKEYTOITEM", 46},
   {"WM_VSCROLL", 277},
   {"WM_VSCROLLCLIPBOARD", 778},
   {"WM_WINDOWPOSCHANGED", 71},
   {"WM_WINDOWPOSCHANGING", 70},
   {"WM_WININICHANGE", 26},
   {"WM_KEYFIRST", 256},
   {"WM_KEYLAST", 264},
   {"WM_SYNCPAINT", 136},
   {"WM_MOUSEACTIVATE", 33},
   {"WM_MOUSEMOVE", 512},
   {"WM_LBUTTONDOWN", 513},
   {"WM_LBUTTONUP", 514},
   {"WM_LBUTTONDBLCLK", 515},
   {"WM_RBUTTONDOWN", 516},
   {"WM_RBUTTONUP", 517},
   {"WM_RBUTTONDBLCLK", 518},
   {"WM_MBUTTONDOWN", 519},
   {"WM_MBUTTONUP", 520},
   {"WM_MBUTTONDBLCLK", 521},
   {"WM_MOUSEWHEEL", 522},
   {"WM_MOUSEHOVER", 0x2A1},
   {"WM_MOUSELEAVE", 0x2A3}
};  
   
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
    static int last_split;
/*    ChildWnd* pChildWnd = (ChildWnd*)GetWindowLong(hWnd, GWL_USERDATA); */
    static ChildWnd* pChildWnd;

        {
           int i;
           char *name = NULL;

           for (i = sizeof(Messages) / sizeof(Messages[0]); i--; )
              if (message == Messages[i].Id)
              {
                 name = Messages[i].Name;
                 break;
              }

           if (name)
              DbgPrint("Window: %x Message: %s (%x) wParam: %x lParam: %x\n",
                 hWnd, name, message, wParam, lParam);
           else
              DbgPrint("Window: %x Message: %x wParam: %x lParam: %x\n",
                 hWnd, message, wParam, lParam);
        }

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
        /* fall through */
    default: def:
        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
