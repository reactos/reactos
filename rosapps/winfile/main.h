/*
 *  ReactOS winfile
 *
 *  main.h
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
/*
 * Based on Winefile, Copyright 2000 martin Fuchs <martin-fuchs@gmx.net>
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"
#include "entries.h"


////////////////////////////////////////////////////////////////////////////////

#define MAX_LOADSTRING 100
#define	BUFFER_LEN	1024

#define	WM_DISPATCH_COMMAND	0xBF80

////////////////////////////////////////////////////////////////////////////////
 // range for drive bar command ids: 0x9000..0x90FF
#define	ID_DRIVE_FIRST					0x9001


#define _NO_EXTENSIONS

enum IMAGE {
	IMG_NONE=-1,	IMG_FILE=0,			IMG_DOCUMENT,	IMG_EXECUTABLE,
	IMG_FOLDER,		IMG_OPEN_FOLDER,	IMG_FOLDER_PLUS,IMG_OPEN_PLUS,	IMG_OPEN_MINUS,
	IMG_FOLDER_UP,	IMG_FOLDER_CUR
};

#define	IMAGE_WIDTH			16
#define	IMAGE_HEIGHT		13
#define	SPLIT_WIDTH			3

#define IDW_STATUSBAR		0x100
#define IDW_TOOLBAR			0x101
#define IDW_DRIVEBAR		0x102
#define IDW_DRIVEBOX        0x103

#define	IDW_FIRST_CHILD		0xC000	//0x200

#define IDW_TREE_LEFT		3
#define IDW_TREE_RIGHT		6
#define IDW_HEADER_LEFT		2
#define IDW_HEADER_RIGHT	5

#define IDW_STATUS_WINDOW   7

////////////////////////////////////////////////////////////////////////////////
void _wsplitpath(const WCHAR* path, WCHAR* drv, WCHAR* dir, WCHAR* name, WCHAR* ext);
void _splitpath(const CHAR* path, CHAR* drv, CHAR* dir, CHAR* name, CHAR* ext);

#ifdef _UNICODE
#define _tsplitpath _wsplitpath
#else
#define _tsplitpath _splitpath
#endif

////////////////////////////////////////////////////////////////////////////////

enum OPTION_FLAGS {
    OPTIONS_OPEN_NEW_WINDOW_ON_CONNECT = 0x01,
    OPTIONS_MINIMISE_ON_USE            = 0x02,
    OPTIONS_SAVE_ON_EXIT          	   = 0x04,
};

////////////////////////////////////////////////////////////////////////////////

enum COLUMN_FLAGS {
	COL_SIZE		= 0x01,
	COL_DATE		= 0x02,
	COL_TIME		= 0x04,
	COL_ATTRIBUTES	= 0x08,
	COL_DOSNAMES	= 0x10,
#ifdef _NO_EXTENSIONS
	COL_ALL = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_DOSNAMES
#else
	COL_INDEX		= 0x20,
	COL_LINKS		= 0x40,
	COL_ALL = COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_DOSNAMES|COL_INDEX|COL_LINKS
#endif
};

#define USE_GLOBAL_STRUCT
#ifdef USE_GLOBAL_STRUCT
typedef struct
{
  HINSTANCE hInstance;
  HACCEL	hAccel;
  HWND		hMainWnd;
  HMENU		hMenuFrame;
  HMENU		hWindowsMenu;
  HMENU		hLanguageMenu;
  HMENU		hMenuView;
  HMENU		hMenuOptions;
  HWND		hMDIClient;
  HWND		hStatusBar;
  HWND		hToolBar;
  HWND		hDriveBar;
  HFONT		hFont;

  TCHAR		num_sep;
  SIZE		spaceSize;
  HIMAGELIST himl;

  TCHAR		drives[BUFFER_LEN];
  BOOL		prescan_node;	//TODO

  LPCSTR	lpszLanguage;
  UINT		wStringTableOffset;
  enum OPTION_FLAGS Options;

} WINFILE_GLOBALS;

extern WINFILE_GLOBALS Globals;

#define STRINGID(id) (Globals.wStringTableOffset + 0x##id)

#else

extern HINSTANCE hInstance;
extern HACCEL	hAccel;
extern HWND		hMainWnd;
extern HMENU	hMenuFrame;
extern HMENU	hWindowsMenu;
extern HMENU	hLanguageMenu;
extern HMENU	hMenuView;
extern HMENU	hMenuOptions;
extern HWND		hMDIClient;
extern HWND		hStatusBar;
extern HWND		hToolBar;
extern HWND		hDriveBar;
extern HFONT	hFont;

extern TCHAR	num_sep;
extern SIZE		spaceSize;
extern HIMAGELIST himl;

extern TCHAR	drives[BUFFER_LEN];
extern BOOL		prescan_node;	//TODO

extern LPCSTR	lpszLanguage;
extern UINT		wStringTableOffset;

#define Globals.

#endif

extern UINT OemCodePage;
extern UINT AnsiCodePage;
extern LCID UserDefaultLCID;

extern HINSTANCE hInst;
extern TCHAR szTitle[];
extern TCHAR szFrameClass[];
extern TCHAR szChildClass[];


void SetupStatusBar(BOOL bResize);
void UpdateStatusBar(void);

#ifndef _MSC_VER
typedef struct tagNMITEMACTIVATE{
    NMHDR   hdr;
    int     iItem;
    int     iSubItem;
    UINT    uNewState;
    UINT    uOldState;
    UINT    uChanged;
    POINT   ptAction;
    LPARAM  lParam;
    UINT    uKeyFlags;
} NMITEMACTIVATE, FAR *LPNMITEMACTIVATE;
#endif

#ifdef __cplusplus
};
#endif

#endif // __MAIN_H__
