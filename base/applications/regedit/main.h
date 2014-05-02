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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "resource.h"


#define STATUS_WINDOW   2001
#define TREE_WINDOW     2002
#define LIST_WINDOW     2003

#define	SPLIT_WIDTH	5
#define SPLIT_MIN  30

#define COUNT_OF(a) (sizeof(a)/sizeof(a[0]))

#define PM_MODIFYVALUE  0
#define PM_NEW          1
#define PM_TREECONTEXT  2
#define PM_HEXEDIT      3

#define MAX_NEW_KEY_LEN 128

extern HINSTANCE hInst;

/******************************************************************************/

enum OPTION_FLAGS
{
    OPTIONS_AUTO_REFRESH        = 0x01,
    OPTIONS_READ_ONLY_MODE      = 0x02,
    OPTIONS_CONFIRM_ON_DELETE   = 0x04,
    OPTIONS_SAVE_ON_EXIT        = 0x08,
    OPTIONS_DISPLAY_BINARY_DATA = 0x10,
    OPTIONS_VIEW_TREE_ONLY      = 0x20,
    OPTIONS_VIEW_DATA_ONLY      = 0x40,
};

typedef struct
{
    HWND    hWnd;
    HWND    hTreeWnd;
    HWND    hListWnd;
    HWND    hAddressBarWnd;
    HWND    hAddressBtnWnd;
    int     nFocusPanel;      /* 0: left  1: right */
    int     nSplitPos;
    WINDOWPLACEMENT pos;
    WCHAR   szPath[MAX_PATH];
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

extern WCHAR szTitle[];
extern WCHAR szFrameClass[];
extern WCHAR szChildClass[];

extern const WCHAR g_szGeneralRegKey[];

/* about.c */
extern void ShowAboutBox(HWND hWnd);

/* childwnd.c */
extern LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);
extern void ResizeWnd(int cx, int cy);
extern LPCWSTR get_root_key_name(HKEY hRootKey);

/* error.c */
extern int ErrorMessageBox(HWND hWnd, LPCWSTR lpTitle, DWORD dwErrorCode, ...);
extern int InfoMessageBox(HWND hWnd, UINT uType, LPCWSTR lpTitle, LPCWSTR lpMessage, ...);

/* find.c */
extern void FindDialog(HWND hWnd);
extern BOOL FindNext(HWND hWnd);

/* framewnd.c */
extern LRESULT CALLBACK FrameWndProc(HWND, UINT, WPARAM, LPARAM);
extern void SetupStatusBar(HWND hWnd, BOOL bResize);
extern void UpdateStatusBar(void);
extern BOOL CopyKeyName(HWND hWnd, HKEY hRootKey, LPCWSTR keyName);
extern BOOL ExportRegistryFile(HWND hWnd);

/* listview.c */
extern HWND CreateListView(HWND hwndParent, HMENU id);
extern BOOL RefreshListView(HWND hwndLV, HKEY hKey, LPCWSTR keyPath);
extern LPCWSTR GetValueName(HWND hwndLV, int iStartAt);
extern BOOL ListWndNotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL *Result);
extern BOOL IsDefaultValue(HWND hwndLV, int i);

/* regedit.c */
LPCWSTR getAppName(void);

/* treeview.c */
extern HWND CreateTreeView(HWND hwndParent, LPWSTR pHostName, HMENU id);
extern BOOL RefreshTreeView(HWND hWndTV);
extern BOOL RefreshTreeItem(HWND hwndTV, HTREEITEM hItem);
extern BOOL OnTreeExpanding(HWND hWnd, NMTREEVIEW* pnmtv);
extern LPCWSTR GetItemPath(HWND hwndTV, HTREEITEM hItem, HKEY* phRootKey);
extern BOOL DeleteNode(HWND hwndTV, HTREEITEM hItem);
extern HTREEITEM InsertNode(HWND hwndTV, HTREEITEM hItem, LPWSTR name);
extern HWND StartKeyRename(HWND hwndTV);
extern BOOL CreateNewKey(HWND hwndTV, HTREEITEM hItem);
extern BOOL SelectNode(HWND hwndTV, LPCWSTR keyPath);
extern void DestroyTreeView(HWND hwndTV);
extern void DestroyListView(HWND hwndLV);
extern void DestroyMainMenu(void);

/* edit.c */
extern BOOL ModifyValue(HWND hwnd, HKEY hKey, LPCWSTR valueName, BOOL EditBin);
extern BOOL DeleteKey(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath);
extern LONG RenameKey(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpNewName);
extern LONG RenameValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpDestValue, LPCWSTR lpSrcValue);
extern LONG QueryStringValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPWSTR pszBuffer, DWORD dwBufferLen);
extern BOOL GetKeyName(LPWSTR pszDest, size_t iDestLength, HKEY hRootKey, LPCWSTR lpSubKey);

/* security.c */
extern BOOL RegKeyEditPermissions(HWND hWndOwner, HKEY hKey, LPCWSTR lpMachine, LPCWSTR lpKeyName);

/* settings.c */
extern void LoadSettings(void);
extern void SaveSettings(void);

/* EOF */
