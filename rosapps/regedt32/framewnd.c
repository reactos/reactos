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
void CmdOptionsFont(HWND hWnd)
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


void CmdRegistryPrint(HWND hWnd, int cmd)
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
void CmdRegistrySaveSubTreeAs(HWND hWnd)
{
    OPENFILENAME ofn;// = {    };

    memset(&ofn, 0, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME); 
    ofn.hwndOwner = hWnd; 
    if (GetSaveFileName(&ofn)) {
    } else {
    }
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
    case ID_REGISTRY_PRINTSUBTREE:
    case ID_REGISTRY_PRINTERSETUP:
        CmdRegistryPrint(hWnd, LOWORD(wParam));
        break;
    case ID_REGISTRY_SAVESUBTREEAS:
        CmdRegistrySaveSubTreeAs(hWnd);
        break;
    case ID_OPTIONS_FONT:
        CmdOptionsFont(hWnd);
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


