/*
 * Regedit definitions
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

#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "resource.h"


#define STATUS_WINDOW   2001
#define TREE_WINDOW     2002
#define LIST_WINDOW     2003

#define MAX_LOADSTRING  100
#define	SPLIT_WIDTH		5
#define MAX_NAME_LEN    500


/******************************************************************************/

enum OPTION_FLAGS {
    OPTIONS_AUTO_REFRESH               = 0x01,
    OPTIONS_READ_ONLY_MODE             = 0x02,
    OPTIONS_CONFIRM_ON_DELETE          = 0x04,
    OPTIONS_SAVE_ON_EXIT          	   = 0x08,
    OPTIONS_DISPLAY_BINARY_DATA    	   = 0x10,
    OPTIONS_VIEW_TREE_ONLY       	   = 0x20,
    OPTIONS_VIEW_DATA_ONLY      	   = 0x40,
};

typedef struct {
	HWND	hWnd;
    HWND    hTreeWnd;
    HWND    hListWnd;
    int     nFocusPanel;      /* 0: left  1: right */
	int		nSplitPos;
	WINDOWPLACEMENT pos;
	TCHAR	szPath[MAX_PATH];
} ChildWnd;

/*******************************************************************************
 * Global Variables:
 */
extern HINSTANCE hInst;
extern HWND      hFrameWnd;
extern HMENU     hMenuFrame;
extern HWND      hStatusBar;
extern HFONT     hFont;
extern enum OPTION_FLAGS Options;

extern TCHAR szTitle[];
extern TCHAR szFrameClass[];
extern TCHAR szChildClass[];

/*******************************************************************************
 * Dynamically load all things that depend on user32.dll
 */
#include "winuser.h"
#include "wingdi.h"
#include "commctrl.h"
#include "commdlg.h"

#ifdef REGEDIT_DECLARE_FUNCTIONS
#define d(x) typeof(x) *p##x = NULL;
#else
#define d(x) extern typeof(x) *p##x;
#endif

d(BeginDeferWindowPos)
d(BeginPaint)
d(CallWindowProcA)
d(CheckMenuItem)
d(CloseClipboard)
d(CommDlgExtendedError)
d(CreateStatusWindowA)
d(CreateWindowExA)
d(DefWindowProcA)
d(DeferWindowPos)
d(DeleteDC)
d(DeleteObject)
d(DestroyMenu)
d(DestroyWindow)
d(DialogBoxParamA)
d(DispatchMessageA)
d(EmptyClipboard)
d(EndDeferWindowPos)
d(EndPaint)
d(EndDialog)
d(FillRect)
d(GetCapture)
d(GetClientRect)
d(GetCursorPos)
d(GetDC)
d(GetDlgItem)
d(GetMenu)
d(GetMessageA)
d(GetOpenFileNameA)
d(GetSaveFileNameA)
d(GetStockObject)
d(GetSubMenu)
d(GetSystemMetrics)
d(ImageList_Add)
d(ImageList_Create)
d(ImageList_GetImageCount)
d(InitCommonControls)
d(InvertRect)
d(IsWindowVisible)
d(LoadAcceleratorsA)
d(LoadBitmapA)
d(LoadCursorA)
d(LoadIconA)
d(LoadImageA)
d(LoadMenuA)
d(LoadStringA)
d(MessageBeep)
d(MoveWindow)
d(OpenClipboard)
d(PostQuitMessage)
d(PrintDlgA)
d(RegisterClassExA)
d(RegisterClipboardFormatA)
d(ReleaseCapture)
d(ReleaseDC)
d(ScreenToClient)
d(SendMessageA)
d(SetCapture)
d(SetCursor)
d(SetFocus)
d(SetWindowLongA)
d(SetWindowTextA)
d(ShowWindow)
d(TranslateAccelerator)
d(TranslateMessage)
d(UpdateWindow)
d(WinHelpA)
d(wsprintfA)

#undef d

#define BeginDeferWindowPos pBeginDeferWindowPos
#define BeginPaint pBeginPaint
#define CallWindowProcA pCallWindowProcA
#define CheckMenuItem pCheckMenuItem
#define CloseClipboard pCloseClipboard
#define CommDlgExtendedError pCommDlgExtendedError
#define CreateStatusWindowA pCreateStatusWindowA
#define CreateWindowExA pCreateWindowExA
#define DefWindowProcA pDefWindowProcA
#define DeferWindowPos pDeferWindowPos
#define DeleteDC pDeleteDC
#define DeleteObject pDeleteObject
#define DestroyMenu pDestroyMenu
#define DestroyWindow pDestroyWindow
#define DialogBoxParamA pDialogBoxParamA
#define DispatchMessageA pDispatchMessageA
#define EmptyClipboard pEmptyClipboard
#define EndDeferWindowPos pEndDeferWindowPos
#define EndDialog pEndDialog
#define EndPaint pEndPaint
#define FillRect pFillRect
#define GetCapture pGetCapture
#define GetClientRect pGetClientRect
#define GetCursorPos pGetCursorPos
#define GetDC pGetDC
#define GetDlgItem pGetDlgItem
#define GetMenu pGetMenu
#define GetMessageA pGetMessageA
#define GetOpenFileNameA pGetOpenFileNameA
#define GetSaveFileNameA pGetSaveFileNameA
#define GetStockObject pGetStockObject
#define GetSubMenu pGetSubMenu
#define GetSystemMetrics pGetSystemMetrics
#define ImageList_Add pImageList_Add
#define ImageList_Create pImageList_Create
#define ImageList_GetImageCount pImageList_GetImageCount
#define InitCommonControls pInitCommonControls
#define InvertRect pInvertRect
#define IsWindowVisible pIsWindowVisible
#define LoadAcceleratorsA pLoadAcceleratorsA
#define LoadBitmapA pLoadBitmapA
#define LoadCursorA pLoadCursorA
#define LoadIconA pLoadIconA
#define LoadImageA pLoadImageA
#define LoadMenuA pLoadMenuA
#define LoadStringA pLoadStringA
#define MessageBeep pMessageBeep
#define MoveWindow pMoveWindow
#define OpenClipboard pOpenClipboard
#define PostQuitMessage pPostQuitMessage
#define PrintDlgA pPrintDlgA
#define RegisterClassExA pRegisterClassExA
#define RegisterClipboardFormatA pRegisterClipboardFormatA
#define ReleaseCapture pReleaseCapture
#define ReleaseDC pReleaseDC
#define ScreenToClient pScreenToClient
#define SendMessageA pSendMessageA
#define SetCapture pSetCapture
#define SetCursor pSetCursor
#define SetFocus pSetFocus
#define SetWindowLongA pSetWindowLongA
#define SetWindowTextA pSetWindowTextA
#define ShowWindow pShowWindow
#undef TranslateAccelerator
#define TranslateAccelerator pTranslateAccelerator
#define TranslateMessage pTranslateMessage
#define UpdateWindow pUpdateWindow
#define WinHelpA pWinHelpA
#define wsprintfA pwsprintfA

#ifdef __cplusplus
};
#endif

/* about.c */
extern void ShowAboutBox(HWND hWnd);

/* childwnd.c */
extern LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);

/* framewnd.c */
extern LRESULT CALLBACK FrameWndProc(HWND, UINT, WPARAM, LPARAM);
extern void SetupStatusBar(HWND hWnd, BOOL bResize);
extern void UpdateStatusBar(void);

/* listview.c */
extern HWND CreateListView(HWND hwndParent, int id);
extern BOOL RefreshListView(HWND hwndTV, HKEY hKey, LPTSTR keyPath);

/* treeview.c */
extern HWND CreateTreeView(HWND hwndParent, LPTSTR pHostName, int id);
extern BOOL OnTreeExpanding(HWND hWnd, NMTREEVIEW* pnmtv);
extern HKEY FindRegRoot(HWND hwndTV, HTREEITEM hItem, LPTSTR keyPath, int* pPathLen, int max);

#endif /* __MAIN_H__ */
