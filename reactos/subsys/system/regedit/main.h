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

#include "resource.h"


#define STATUS_WINDOW   2001
#define TREE_WINDOW     2002
#define LIST_WINDOW     2003

#define	SPLIT_WIDTH	5
#define SPLIT_MIN  30

#define COUNT_OF(a) (sizeof(a)/sizeof(a[0]))

#define PM_MODIFYVALUE  0
#define PM_NEW          1

extern HINSTANCE hInst;

/******************************************************************************/

enum OPTION_FLAGS {
    OPTIONS_AUTO_REFRESH            	   = 0x01,
    OPTIONS_READ_ONLY_MODE          	   = 0x02,
    OPTIONS_CONFIRM_ON_DELETE       	   = 0x04,
    OPTIONS_SAVE_ON_EXIT         	   = 0x08,
    OPTIONS_DISPLAY_BINARY_DATA    	   = 0x10,
    OPTIONS_VIEW_TREE_ONLY       	   = 0x20,
    OPTIONS_VIEW_DATA_ONLY      	   = 0x40,
};

typedef struct {
    HWND    hWnd;
    HWND    hTreeWnd;
    HWND    hListWnd;
    int     nFocusPanel;      /* 0: left  1: right */
    int	    nSplitPos;
    WINDOWPLACEMENT pos;
    TCHAR   szPath[MAX_PATH];
} ChildWnd;
extern ChildWnd* g_pChildWnd;

/*******************************************************************************
 * Global Variables:
 */
extern HINSTANCE hInst;
extern HWND      hFrameWnd;
extern HMENU     hMenuFrame;
extern HWND      hStatusBar;
extern HMENU     hPopupMenus;
extern HFONT     hFont;
extern enum OPTION_FLAGS Options;

extern TCHAR szTitle[];
extern TCHAR szFrameClass[];
extern TCHAR szChildClass[];

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
extern BOOL RefreshListView(HWND hwndLV, HKEY hKey, LPCTSTR keyPath);
extern LPCTSTR GetValueName(HWND hwndLV, int iStartAt);
extern BOOL ListWndNotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL *Result);
extern BOOL IsDefaultValue(HWND hwndLV, int i);

/* treeview.c */
extern HWND CreateTreeView(HWND hwndParent, LPTSTR pHostName, int id);
extern BOOL OnTreeExpanding(HWND hWnd, NMTREEVIEW* pnmtv);
extern LPCTSTR GetItemPath(HWND hwndTV, HTREEITEM hItem, HKEY* phRootKey);

/* edit.c */
extern BOOL ModifyValue(HWND hwnd, HKEY hKey, LPCTSTR valueName, BOOL EditBin);

#endif /* __MAIN_H__ */
