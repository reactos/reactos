/*
 *  ReactOS regedit
 *
 *  main.h
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


////////////////////////////////////////////////////////////////////////////////

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
    int     nFocusPanel;      // 0: left  1: right
	int		nSplitPos;
	WINDOWPLACEMENT pos;
	TCHAR	szPath[MAX_PATH];
} ChildWnd;

////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//
extern HINSTANCE hInst;
extern HWND      hFrameWnd;
extern HMENU     hMenuFrame;
extern HWND      hStatusBar;
extern HFONT     hFont;
extern enum OPTION_FLAGS Options;

extern TCHAR szTitle[];
extern TCHAR szFrameClass[];
extern TCHAR szChildClass[];

#if __MINGW32_MAJOR_VERSION < 1
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
