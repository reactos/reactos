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
//#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
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
// Global and Local Variables:
//

enum OPTION_FLAGS Options;
BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop

static HHOOK hcbthook;
static ChildWnd* newchild = NULL;


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
	MoveWindow(hMDIClient, prect->left,prect->top,prect->right,prect->bottom, TRUE);
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

////////////////////////////////////////////////////////////////////////////////

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

static ChildWnd* alloc_child_window(LPCTSTR szKeyName, HKEY hKey)
{
	ChildWnd* pChildWnd = (ChildWnd*)malloc(sizeof(ChildWnd));

	memset(pChildWnd, 0, sizeof(ChildWnd));
	pChildWnd->pos.length = sizeof(WINDOWPLACEMENT);
	pChildWnd->pos.flags = 0;
	pChildWnd->pos.showCmd = SW_SHOWNORMAL;
	pChildWnd->pos.rcNormalPosition.left = CW_USEDEFAULT;
	pChildWnd->pos.rcNormalPosition.top = CW_USEDEFAULT;
	pChildWnd->pos.rcNormalPosition.right = CW_USEDEFAULT;
	pChildWnd->pos.rcNormalPosition.bottom = CW_USEDEFAULT;
	pChildWnd->nFocusPanel = 0;
	pChildWnd->nSplitPos = 300;
//	pChildWnd->visible_cols = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES;
//	pChildWnd->sortOrder = SORT_NAME;
//	pChildWnd->header_wdths_ok = FALSE;
	lstrcpy(pChildWnd->szKeyName, szKeyName); // MAX_PATH
	pChildWnd->hKey = hKey;
	return pChildWnd;
}

static HWND CreateChildWindow(HWND hWnd, LPCTSTR szKeyName, HKEY hKey, int unused)
{
	ChildWnd* pChildWnd = alloc_child_window(szKeyName, hKey);
	if (pChildWnd != NULL) {
        MDICREATESTRUCT mcs = { szChildClass, szKeyName, hInst,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            0/*style*/, (LPARAM)hKey/*lParam*/};
        hcbthook = SetWindowsHookEx(WH_CBT, CBTProc, 0, GetCurrentThreadId());
    	newchild = pChildWnd;
        pChildWnd->hWnd = (HWND)SendMessage(hMDIClient, WM_MDICREATE, 0, (LPARAM)&mcs);
        UnhookWindowsHookEx(hcbthook);
        if (pChildWnd->hWnd != NULL) {
            return pChildWnd->hWnd;
        } else {
            free(pChildWnd);
        	newchild = pChildWnd = NULL;
        }
	}
    return 0;
}

void CreateClientChildren(HWND hWnd)
{
    CreateChildWindow(hWnd, _T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT, 1);
    CreateChildWindow(hWnd, _T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER, 1);
    CreateChildWindow(hWnd, _T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE, 1);
    CreateChildWindow(hWnd, _T("HKEY_USERS"), HKEY_USERS, 1);
    CreateChildWindow(hWnd, _T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG, 1);
    PostMessage(hMDIClient, WM_MDICASCADE, 0, 0);
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
/*
UINT_PTR CALLBACK CFHookProc(
  HWND hdlg,      // handle to dialog box
  UINT uiMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
);

typedef UINT_PTR (CALLBACK *LPCFHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct { 
  DWORD        lStructSize; 
  HWND         hwndOwner; 
  HDC          hDC; 
  LPLOGFONT    lpLogFont; 
  INT          iPointSize; 
  DWORD        Flags; 
  COLORREF     rgbColors; 
  LPARAM       lCustData; 
  LPCFHOOKPROC lpfnHook; 
  LPCTSTR      lpTemplateName; 
  HINSTANCE    hInstance; 
  LPTSTR       lpszStyle; 
  WORD         nFontType; 
  WORD         ___MISSING_ALIGNMENT__; 
  INT          nSizeMin; 
  INT          nSizeMax; 
} CHOOSEFONT, *LPCHOOSEFONT; 
 */
static void CmdOptionsFont(HWND hWnd)
{
//    LOGFONT LogFont;
    CHOOSEFONT cf = { sizeof(CHOOSEFONT), hWnd, NULL,
//        &LogFont,   // lpLogFont
        NULL,       // lpLogFont
        0,          // iPointSize
//        CF_INITTOLOGFONTSTRUCT, // Flags
        CF_SCREENFONTS,          // Flags
        0,          // rgbColors; 
        0L,         // lCustData; 
        NULL,       // lpfnHook; 
        NULL,       // lpTemplateName; 
        hInst,      // hInstance; 
        NULL,       // lpszStyle; 
        0,          // nFontType; 
        0,          // ___MISSING_ALIGNMENT__; 
        0,          // nSizeMin; 
        0           // nSizeMax
    };

    if (ChooseFont(&cf)) {


    } else {
        TCHAR* errStr = NULL;
        DWORD error = CommDlgExtendedError();
        switch (error) {
        case CDERR_DIALOGFAILURE: errStr = _T("The dialog box could not be created. The common dialog box function's call to the DialogBox function failed. For example, this error occurs if the common dialog box call specifies an invalid window handle."); break;
        case CDERR_FINDRESFAILURE: errStr = _T("The common dialog box function failed to find a specified resource."); break;
        case CDERR_INITIALIZATION: errStr = _T("The common dialog box function failed during initialization. This error often occurs when sufficient memory is not available."); break;
        case CDERR_LOADRESFAILURE: errStr = _T("The common dialog box function failed to load a specified resource."); break;
        case CDERR_LOADSTRFAILURE: errStr = _T("The common dialog box function failed to load a specified string."); break;
        case CDERR_LOCKRESFAILURE: errStr = _T("The common dialog box function failed to lock a specified resource."); break;
        case CDERR_MEMALLOCFAILURE: errStr = _T("The common dialog box function was unable to allocate memory for internal structures."); break;
        case CDERR_MEMLOCKFAILURE: errStr = _T("The common dialog box function was unable to lock the memory associated with a handle."); break;
        case CDERR_NOHINSTANCE: errStr = _T("The ENABLETEMPLATE flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a corresponding instance handle."); break;
        case CDERR_NOHOOK: errStr = _T("The ENABLEHOOK flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a pointer to a corresponding hook procedure."); break;
        case CDERR_NOTEMPLATE: errStr = _T("The ENABLETEMPLATE flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a corresponding template."); break;
        case CDERR_REGISTERMSGFAIL: errStr = _T("The RegisterWindowMessage function returned an error code when it was called by the common dialog box function."); break;
        case CDERR_STRUCTSIZE: errStr = _T("The lStructSize member of the initialization structure for the corresponding common dialog box is invalid."); break;
        case CFERR_MAXLESSTHANMIN:
            break;
        case CFERR_NOFONTS:
            break;
        }
        if (errStr) {
            MessageBox(hWnd, errStr, szTitle, MB_ICONERROR | MB_OK);
        }
    }
}


static void CmdRegistryPrint(HWND hWnd, int cmd)
{
    PRINTDLG pd = { sizeof(PRINTDLG), hWnd,
        0, // hDevMode; 
        0, // hDevNames; 
        NULL, // hDC; 
        0L, // Flags; 
        0, // nFromPage; 
        0, // nToPage; 
        0, // nMinPage; 
        0, // nMaxPage; 
        0, // nCopies; 
        NULL, // hInstance; 
        0, // lCustData; 
        NULL, // lpfnPrintHook; 
        NULL, // lpfnSetupHook; 
        NULL, // lpPrintTemplateName; 
        NULL, // lpSetupTemplateName; 
        0, // hPrintTemplate; 
        0 // hSetupTemplate; 
    };

    switch (cmd) {
    case ID_REGISTRY_PRINTSUBTREE:
        PrintDlg(&pd);
        break;
    case ID_REGISTRY_PRINTERSETUP:
        PrintDlg(&pd);
        break;
    }
    //PAGESETUPDLG psd;
    //PageSetupDlg(&psd);
}
/*
typedef struct tagOFN { 
  DWORD         lStructSize; 
  HWND          hwndOwner; 
  HINSTANCE     hInstance; 
  LPCTSTR       lpstrFilter; 
  LPTSTR        lpstrCustomFilter; 
  DWORD         nMaxCustFilter; 
  DWORD         nFilterIndex; 
  LPTSTR        lpstrFile; 
  DWORD         nMaxFile; 
  LPTSTR        lpstrFileTitle; 
  DWORD         nMaxFileTitle; 
  LPCTSTR       lpstrInitialDir; 
  LPCTSTR       lpstrTitle; 
  DWORD         Flags; 
  WORD          nFileOffset; 
  WORD          nFileExtension; 
  LPCTSTR       lpstrDefExt; 
  LPARAM        lCustData; 
  LPOFNHOOKPROC lpfnHook; 
  LPCTSTR       lpTemplateName; 
#if (_WIN32_WINNT >= 0x0500)
  void *        pvReserved;
  DWORD         dwReserved;
  DWORD         FlagsEx;
#endif // (_WIN32_WINNT >= 0x0500)
} OPENFILENAME, *LPOPENFILENAME; 
 */
    //GetOpenFileName(...);
    //GetSaveFileName(...);
static void CmdRegistrySaveSubTreeAs(HWND hWnd)
{
    OPENFILENAME ofn;// = {    };

    memset(&ofn, 0, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME); 
    ofn.hwndOwner = hWnd; 
    if (GetSaveFileName(&ofn)) {
    } else {
    }
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

void UpdateStatusBar(void)
{
    TCHAR text[260];
	DWORD size;

	size = sizeof(text)/sizeof(TCHAR);
	GetComputerName(text, &size);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)text);
}

static void toggle_child(HWND hWnd, UINT cmd, HWND hchild)
{
	BOOL vis = IsWindowVisible(hchild);

	CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);
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
    case ID_REGISTRY_OPENLOCAL:
        CreateClientChildren(hWnd);
        break;
    case ID_REGISTRY_CLOSE:
        SendMessage(hWnd, WM_COMMAND, ID_WINDOW_CLOSEALL, 0);
//        SendMessage(hWnd, WM_CLOSE, 0, 0);
        break;
    case ID_REGISTRY_LOADHIVE:
    case ID_REGISTRY_UNLOADHIVE:
    case ID_REGISTRY_RESTORE:
    case ID_REGISTRY_SAVEKEY:
    case ID_REGISTRY_SELECTCOMPUTER:
        break;
    case ID_REGISTRY_PRINTSUBTREE:
    case ID_REGISTRY_PRINTERSETUP:
        CmdRegistryPrint(hWnd, LOWORD(wParam));
        break;
    case ID_REGISTRY_SAVESUBTREEAS:
        CmdRegistrySaveSubTreeAs(hWnd);
        break;
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_OPTIONS_FONT:
        CmdOptionsFont(hWnd);
        break;

    case ID_VIEW_STATUSBAR:
		toggle_child(hWnd, LOWORD(wParam), hStatusBar);
        break;

    case ID_VIEW_DISPLAYBINARYDATA:
        if (Options & OPTIONS_DISPLAY_BINARY_DATA) {
            Options &= ~OPTIONS_DISPLAY_BINARY_DATA;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), LOWORD(wParam), MF_BYCOMMAND);
        } else {
            Options |= OPTIONS_DISPLAY_BINARY_DATA;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
        }
        break;
////
    case ID_VIEW_TREEANDDATA:
        Options &= ~(OPTIONS_VIEW_TREE_ONLY|OPTIONS_VIEW_DATA_ONLY);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_TREEONLY, MF_BYCOMMAND);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_DATAONLY, MF_BYCOMMAND);
        break;
    case ID_VIEW_TREEONLY:
        Options &= ~OPTIONS_VIEW_DATA_ONLY;
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_TREEANDDATA, MF_BYCOMMAND);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_DATAONLY, MF_BYCOMMAND);
        break;
    case ID_VIEW_DATAONLY:
        Options &= ~OPTIONS_VIEW_TREE_ONLY;
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_TREEANDDATA, MF_BYCOMMAND);
        CheckMenuItem(GetSubMenu(hMenuFrame, ID_VIEW_MENU), ID_VIEW_TREEONLY, MF_BYCOMMAND);
        break;
////
    case ID_OPTIONS_AUTOREFRESH:
        if (Options & OPTIONS_AUTO_REFRESH) {
            Options &= ~OPTIONS_AUTO_REFRESH;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), LOWORD(wParam), MF_BYCOMMAND);
        } else {
            Options |= OPTIONS_AUTO_REFRESH;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
        }
        break;
    case ID_OPTIONS_READONLYMODE:
        if (Options & OPTIONS_READ_ONLY_MODE) {
            Options &= ~OPTIONS_READ_ONLY_MODE;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), LOWORD(wParam), MF_BYCOMMAND);
        } else {
            Options |= OPTIONS_READ_ONLY_MODE;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
        }
        break;
    case ID_OPTIONS_CONFIRMONDELETE:
        if (Options & OPTIONS_CONFIRM_ON_DELETE) {
            Options &= ~OPTIONS_CONFIRM_ON_DELETE;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), LOWORD(wParam), MF_BYCOMMAND);
        } else {
            Options |= OPTIONS_CONFIRM_ON_DELETE;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
        }
        break;
    case ID_OPTIONS_SAVESETTINGSONEXIT:
        if (Options & OPTIONS_SAVE_ON_EXIT) {
            Options &= ~OPTIONS_SAVE_ON_EXIT;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), LOWORD(wParam), MF_BYCOMMAND);
        } else {
            Options |= OPTIONS_SAVE_ON_EXIT;
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
        }
        break;

    case ID_WINDOW_CASCADE:
        SendMessage(hMDIClient, WM_MDICASCADE, 0, 0);
        break;
    case ID_WINDOW_TILE_HORZ:
        SendMessage(hMDIClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
        break;
    case ID_WINDOW_TILE_VERT:
        SendMessage(hMDIClient, WM_MDITILE, MDITILE_VERTICAL, 0);
        break;
    case ID_WINDOW_ARRANGEICONS:
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
                WS_EX_MDICHILD|WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE,
                0, 0, 0, 0,
                hWnd, (HMENU)0, hInst, &ccs);
        }
        if (Options & OPTIONS_AUTO_REFRESH) {
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), ID_OPTIONS_AUTOREFRESH, MF_BYCOMMAND | MF_CHECKED);
        }
        if (Options & OPTIONS_READ_ONLY_MODE) {
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), ID_OPTIONS_READONLYMODE, MF_BYCOMMAND | MF_CHECKED);
        }
        if (Options & OPTIONS_CONFIRM_ON_DELETE) {
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), ID_OPTIONS_CONFIRMONDELETE, MF_BYCOMMAND | MF_CHECKED);
        }
        if (Options & OPTIONS_SAVE_ON_EXIT) {
            CheckMenuItem(GetSubMenu(hMenuFrame, ID_OPTIONS_MENU), ID_OPTIONS_SAVESETTINGSONEXIT, MF_BYCOMMAND | MF_CHECKED);
        }
        CreateClientChildren(hWnd);
	    break;
   	case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
//            HWND hChildWnd = (HWND)SendMessage(hMDIClient, WM_MDIGETACTIVE, 0, 0);
//            if (IsWindow(hChildWnd))
//                if (SendMessage(hChildWnd, WM_DISPATCH_COMMAND, wParam, lParam))
//                    break;
   		    return DefFrameProc(hWnd, hMDIClient, message, wParam, lParam);
        }
	    break;
	case WM_SIZE:
        resize_frame_client(hWnd);
		break;
    case WM_DESTROY:
		WinHelp(hWnd, _T("regedt32"), HELP_QUIT, 0);
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


