/*
 *  ReactOS regedit
 *
 *  framewnd.c
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

//#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>
    
#include "main.h"
#include "about.h"
#include "framewnd.h"
#include "treeview.h"
#include "listview.h"
#include <shellapi.h>

#include "regproc.h"

////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

static BOOL bInMenuLoop = FALSE;        // Tells us if we are in the menu loop

static HWND hChildWnd;

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
    MoveWindow(hChildWnd, prect->left, prect->top, prect->right, prect->bottom, TRUE);
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

static void OnExitMenuLoop(HWND hWnd)
{
    bInMenuLoop = FALSE;
    // Update the status bar pane sizes
	SetupStatusBar(hWnd, TRUE);
	UpdateStatusBar();
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
	HMENU hMenuView = GetSubMenu(hMenuFrame, ID_VIEW_MENU);

	CheckMenuItem(hMenuView, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);
	ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);
	resize_frame_client(hWnd);
}

static BOOL CheckCommDlgError(HWND hWnd)
{
        DWORD dwErrorCode = CommDlgExtendedError();
        switch (dwErrorCode) {
        case CDERR_DIALOGFAILURE:
            break;
        case CDERR_FINDRESFAILURE:
            break;
        case CDERR_NOHINSTANCE:
            break;
        case CDERR_INITIALIZATION:
            break;
        case CDERR_NOHOOK:
            break;
        case CDERR_LOCKRESFAILURE:
            break;
        case CDERR_NOTEMPLATE:
            break;
        case CDERR_LOADRESFAILURE:
            break;
        case CDERR_STRUCTSIZE:
            break;
        case CDERR_LOADSTRFAILURE:
            break;
        case FNERR_BUFFERTOOSMALL:
            break;
        case CDERR_MEMALLOCFAILURE:
            break;
        case FNERR_INVALIDFILENAME:
            break;
        case CDERR_MEMLOCKFAILURE:
            break;
        case FNERR_SUBCLASSFAILURE:
            break;
        default:
            break;
        }
	return TRUE;
}

UINT_PTR CALLBACK ImportRegistryFile_OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    OPENFILENAME* pOpenFileName;
    OFNOTIFY* pOfNotify;

    switch (uiMsg) {
    case WM_INITDIALOG:
        pOpenFileName = (OPENFILENAME*)lParam;
        break;
    case WM_NOTIFY:
        pOfNotify = (OFNOTIFY*)lParam;
        if (pOfNotify->hdr.code == CDN_INITDONE) {
        }
        break;
    default:
        break;
    }
    return 0L;
}

#define MAX_CUSTOM_FILTER_SIZE 50
TCHAR CustomFilterBuffer[MAX_CUSTOM_FILTER_SIZE];
TCHAR FileNameBuffer[_MAX_PATH];
TCHAR FileTitleBuffer[_MAX_PATH];

static BOOL InitOpenFileName(HWND hWnd, OPENFILENAME* pofn)
{
    memset(pofn, 0, sizeof(OPENFILENAME));
    pofn->lStructSize = sizeof(OPENFILENAME);
    pofn->hwndOwner = hWnd;
    pofn->hInstance = hInst;

    pofn->lpstrFilter = _T("Registration Files\0*.reg\0Win9x/NT4 Registration Files (REGEDIT4)\0*.reg\0All Files (*.*)\0*.*\0\0");
    pofn->lpstrCustomFilter = CustomFilterBuffer;
    pofn->nMaxCustFilter = MAX_CUSTOM_FILTER_SIZE;
    pofn->nFilterIndex = 0;
    pofn->lpstrFile = FileNameBuffer;
    pofn->nMaxFile = _MAX_PATH;
    pofn->lpstrFileTitle = FileTitleBuffer;
    pofn->nMaxFileTitle = _MAX_PATH;
//    pofn->lpstrInitialDir = _T("");
//    pofn->lpstrTitle = _T("Import Registry File");
//    pofn->Flags = OFN_ENABLETEMPLATE + OFN_EXPLORER + OFN_ENABLESIZING;
    pofn->Flags = OFN_HIDEREADONLY;
//    pofn->nFileOffset = ;
//    pofn->nFileExtension = ;
//    pofn->lpstrDefExt = _T("");
//    pofn->lCustData = ;
//    pofn->lpfnHook = ImportRegistryFile_OFNHookProc;
//    pofn->lpTemplateName = _T("ID_DLG_IMPORT_REGFILE");
//    pofn->lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG1);
//    pofn->FlagsEx = ;
	return TRUE;
}

static BOOL ImportRegistryFile(HWND hWnd)
{
    OPENFILENAME ofn;

    InitOpenFileName(hWnd, &ofn);
    ofn.lpstrTitle = _T("Import Registry File");
//    ofn.lCustData = ;
    if (GetOpenFileName(&ofn)) {
        if (!import_registry_file(ofn.lpstrFile)) {
            //printf("Can't open file \"%s\"\n", ofn.lpstrFile);
            return FALSE;
        }
/*
        get_file_name(&s, filename, MAX_PATH);
        if (!filename[0]) {
            printf("No file name is specified\n%s", usage);
            return FALSE;
            //exit(1);
        }
        while (filename[0]) {
            if (!import_registry_file(filename)) {
                perror("");
                printf("Can't open file \"%s\"\n", filename);
                return FALSE;
                //exit(1);
            }
            get_file_name(&s, filename, MAX_PATH);
        }
 */
    } else {
        CheckCommDlgError(hWnd);
    }
	return TRUE;
}


static BOOL ExportRegistryFile(HWND hWnd)
{
    OPENFILENAME ofn;
    TCHAR ExportKeyPath[_MAX_PATH];

    ExportKeyPath[0] = _T('\0');
    InitOpenFileName(hWnd, &ofn);
    ofn.lpstrTitle = _T("Export Registry File");
//    ofn.lCustData = ;
    ofn.Flags = OFN_ENABLETEMPLATE + OFN_EXPLORER;
    ofn.lpfnHook = ImportRegistryFile_OFNHookProc;
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG1);
    if (GetSaveFileName(&ofn)) {
        BOOL result;
        result = export_registry_key(ofn.lpstrFile, ExportKeyPath);
        //result = export_registry_key(ofn.lpstrFile, NULL);
        //if (!export_registry_key(ofn.lpstrFile, NULL)) {
        if (!result) {
            //printf("Can't open file \"%s\"\n", ofn.lpstrFile);
            return FALSE;
        }
/*
        TCHAR filename[MAX_PATH];
        filename[0] = '\0';
        get_file_name(&s, filename, MAX_PATH);
        if (!filename[0]) {
            printf("No file name is specified\n%s", usage);
            return FALSE;
            //exit(1);
        }
        if (s[0]) {
            TCHAR reg_key_name[KEY_MAX_LEN];
            get_file_name(&s, reg_key_name, KEY_MAX_LEN);
            export_registry_key(filename, reg_key_name);
        } else {
            export_registry_key(filename, NULL);
        }
 */
    } else {
        CheckCommDlgError(hWnd);
    }
	return TRUE;
}

BOOL PrintRegistryHive(HWND hWnd, LPTSTR path)
{
#if 1
    PRINTDLG pd;

    ZeroMemory(&pd, sizeof(PRINTDLG));
    pd.lStructSize = sizeof(PRINTDLG);
    pd.hwndOwner   = hWnd;
    pd.hDevMode    = NULL;     // Don't forget to free or store hDevMode
    pd.hDevNames   = NULL;     // Don't forget to free or store hDevNames
    pd.Flags       = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC; 
    pd.nCopies     = 1;
    pd.nFromPage   = 0xFFFF; 
    pd.nToPage     = 0xFFFF; 
    pd.nMinPage    = 1; 
    pd.nMaxPage    = 0xFFFF; 
    if (PrintDlg(&pd) == TRUE) {
        // GDI calls to render output. 
        DeleteDC(pd.hDC); // Delete DC when done.
    }
#else    
    HRESULT hResult;
    PRINTDLGEX pd;

    hResult = PrintDlgEx(&pd);
    if (hResult == S_OK) {
        switch (pd.dwResultAction) {
        case PD_RESULT_APPLY:
            //The user clicked the Apply button and later clicked the Cancel button. This indicates that the user wants to apply the changes made in the property sheet, but does not yet want to print. The PRINTDLGEX structure contains the information specified by the user at the time the Apply button was clicked. 
            break;
        case PD_RESULT_CANCEL:
            //The user clicked the Cancel button. The information in the PRINTDLGEX structure is unchanged. 
            break;
        case PD_RESULT_PRINT:
            //The user clicked the Print button. The PRINTDLGEX structure contains the information specified by the user. 
            break;
        default:
            break;
        }
    } else {
        switch (hResult) {
        case E_OUTOFMEMORY:
            //Insufficient memory. 
            break;
        case E_INVALIDARG:
            // One or more arguments are invalid. 
            break;
        case E_POINTER:
            //Invalid pointer. 
            break;
        case E_HANDLE:
            //Invalid handle. 
            break;
        case E_FAIL:
            //Unspecified error. 
            break;
        default:
            break;
        }
        return FALSE;
    }
#endif
    return TRUE;
}

BOOL CopyKeyName(HWND hWnd, LPTSTR keyName)
{
    BOOL result;
    
    result = OpenClipboard(hWnd);
    if (result) {
        result = EmptyClipboard();
        if (result) {

            //HANDLE hClipData;
            //hClipData = SetClipboardData(UINT uFormat, HANDLE hMem);

        } else {
            // error emptying clipboard
            DWORD dwError = GetLastError();
        }
        if (!CloseClipboard()) {
            // error closing clipboard
            DWORD dwError = GetLastError();
        }
    } else {
        // error opening clipboard
        DWORD dwError = GetLastError();
    }
    return result;
}

BOOL RefreshView(HWND hWnd)
{
    // TODO:
    MessageBeep(-1);
    MessageBeep(MB_ICONASTERISK);
    MessageBeep(MB_ICONEXCLAMATION);
    MessageBeep(MB_ICONHAND);
    MessageBeep(MB_ICONQUESTION);
    MessageBeep(MB_OK);
    return TRUE;
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
    case ID_REGISTRY_IMPORTREGISTRYFILE:
        ImportRegistryFile(hWnd);
        break;
    case ID_REGISTRY_EXPORTREGISTRYFILE:
        ExportRegistryFile(hWnd);
        break;
    case ID_REGISTRY_CONNECTNETWORKREGISTRY:
        break;
    case ID_REGISTRY_DISCONNECTNETWORKREGISTRY:
        break;
    case ID_REGISTRY_PRINT:
        PrintRegistryHive(hWnd, _T(""));
        break;
    case ID_EDIT_COPYKEYNAME:
        CopyKeyName(hWnd, _T(""));
        break;
    case ID_REGISTRY_PRINTERSETUP:
        //PRINTDLG pd;
        //PrintDlg(&pd);
        //PAGESETUPDLG psd;
        //PageSetupDlg(&psd);
        break;
    case ID_REGISTRY_OPENLOCAL:
        break;
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        RefreshView(hWnd);
        break;
//	case ID_OPTIONS_TOOLBAR:
//		toggle_child(hWnd, LOWORD(wParam), hToolBar);
//      break;
	case ID_VIEW_STATUSBAR:
		toggle_child(hWnd, LOWORD(wParam), hStatusBar);
        break;
    case ID_HELP_HELPTOPICS:
//		WinHelp(hWnd, _T("regedit"), HELP_CONTENTS, 0);
		WinHelp(hWnd, _T("regedit"), HELP_FINDER, 0);
        break;
    case ID_HELP_ABOUT:
#ifdef WINSHELLAPI
//        ShellAbout(hWnd, szTitle, _T(""), LoadIcon(hInst, (LPCTSTR)IDI_REGEDIT));
#else
        ShowAboutBox(hWnd);
#endif
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
    static ChildWnd* pChildWnd = NULL;

    switch (message) {
    case WM_CREATE:
        {
        pChildWnd = HeapAlloc(GetProcessHeap(), 0, sizeof(ChildWnd));
        _tcsncpy(pChildWnd->szPath, _T("My Computer"), MAX_PATH);
        hChildWnd = CreateWindowEx(0, szChildClass, _T("regedit child window"),
//                    WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE|WS_BORDER,
                    WS_CHILD|WS_VISIBLE | WS_EX_CLIENTEDGE,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    hWnd, (HMENU)0, hInst, pChildWnd);
        }
        break;
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
   		    return DefWindowProc(hWnd, message, wParam, lParam);
        }
		break;
    case WM_SIZE:
        resize_frame_client(hWnd);
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
    case WM_DESTROY:
        if (pChildWnd) {
            HeapFree(GetProcessHeap(), 0, pChildWnd);
            pChildWnd = NULL;
        }
		WinHelp(hWnd, _T("regedit"), HELP_QUIT, 0);
        PostQuitMessage(0);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
