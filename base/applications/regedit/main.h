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

#define SPLIT_WIDTH    5
#define SPLIT_MIN     30

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(*A))

#define PM_MODIFYVALUE  0
#define PM_NEW          1
#define PM_TREECONTEXT  2
#define PM_ROOTITEM     3
#define PM_HEXEDIT      4

#define MAX_NEW_KEY_LEN  128
#define KEY_MAX_LEN      1024

#define REG_FORMAT_5     1
#define REG_FORMAT_4     2

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
    HICON   hArrowIcon;
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
extern const WCHAR* reg_class_namesW[];

/* about.c */
void ShowAboutBox(HWND hWnd);

/* childwnd.c */
LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);
void ResizeWnd(int cx, int cy);
LPCWSTR get_root_key_name(HKEY hRootKey);
VOID UpdateAddress(HTREEITEM hItem, HKEY hRootKey, LPCWSTR pszPath);

/* edit.c */
BOOL ModifyValue(HWND hwnd, HKEY hKey, LPCWSTR valueName, BOOL EditBin);
BOOL DeleteKey(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath);
LONG RenameKey(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpNewName);
LONG RenameValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpDestValue, LPCWSTR lpSrcValue);
LONG QueryStringValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPWSTR pszBuffer, DWORD dwBufferLen);
BOOL GetKeyName(LPWSTR pszDest, size_t iDestLength, HKEY hRootKey, LPCWSTR lpSubKey);

/* error.c */
int ErrorMessageBox(HWND hWnd, LPCWSTR lpTitle, DWORD dwErrorCode, ...);
int InfoMessageBox(HWND hWnd, UINT uType, LPCWSTR lpTitle, LPCWSTR lpMessage, ...);

/* find.c */
void FindDialog(HWND hWnd);
BOOL FindNext(HWND hWnd);
void FindNextMessageBox(HWND hWnd);

/* framewnd.c */
LRESULT CALLBACK FrameWndProc(HWND, UINT, WPARAM, LPARAM);
void SetupStatusBar(HWND hWnd, BOOL bResize);
void UpdateStatusBar(void);
BOOL CopyKeyName(HWND hWnd, HKEY hRootKey, LPCWSTR keyName);
BOOL ExportRegistryFile(HWND hWnd);

/* listview.c */
HWND CreateListView(HWND hwndParent, HMENU id, INT cx);
BOOL RefreshListView(HWND hwndLV, HKEY hKey, LPCWSTR keyPath);
WCHAR *GetValueName(HWND hwndLV, int iStartAt);
BOOL ListWndNotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL *Result);
BOOL TreeWndNotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL *Result);
BOOL IsDefaultValue(HWND hwndLV, int i);

/* regedit.c */
void WINAPIV output_message(unsigned int id, ...);
void WINAPIV error_exit(unsigned int id, ...);

/* regproc.c */
char *GetMultiByteString(const WCHAR *strW);
BOOL import_registry_file(FILE *reg_file);
void delete_registry_key(WCHAR *reg_key_name);
BOOL export_registry_key(WCHAR *file_name, WCHAR *path, DWORD format);

/* security.c */
BOOL RegKeyEditPermissions(HWND hWndOwner, HKEY hKey, LPCWSTR lpMachine, LPCWSTR lpKeyName);

/* settings.c */
void LoadSettings(void);
void SaveSettings(void);

/* treeview.c */
HWND CreateTreeView(HWND hwndParent, LPWSTR pHostName, HMENU id);
BOOL RefreshTreeView(HWND hWndTV);
BOOL RefreshTreeItem(HWND hwndTV, HTREEITEM hItem);
BOOL OnTreeExpanding(HWND hWnd, NMTREEVIEW* pnmtv);
LPCWSTR GetItemPath(HWND hwndTV, HTREEITEM hItem, HKEY* phRootKey);
BOOL DeleteNode(HWND hwndTV, HTREEITEM hItem);
HTREEITEM InsertNode(HWND hwndTV, HTREEITEM hItem, LPWSTR name);
HWND StartKeyRename(HWND hwndTV);
BOOL CreateNewKey(HWND hwndTV, HTREEITEM hItem);
BOOL SelectNode(HWND hwndTV, LPCWSTR keyPath);
void DestroyTreeView(HWND hwndTV);
void DestroyListView(HWND hwndLV);
void DestroyMainMenu(void);

/* EOF */
