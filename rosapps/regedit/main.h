/*
 *  ReactOS regedit
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

#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"
#include "regproc.h"


typedef struct _Entry {
	struct _Entry*	next;
	struct _Entry*	down;
	struct _Entry*	up;
	BOOL	expanded;
	BOOL	scanned;
	int		level;
    BOOL    bKey;       // key or value?
    HKEY    hKey;
//    BOOL    bRoot;
    HTREEITEM hTreeItem;
} Entry;


typedef struct {
	Entry	entry;
	TCHAR	path[MAX_PATH];
//	DWORD	_flags;
} Root;

typedef struct {
	HWND	hWnd;
    HWND    hTreeWnd;
    HWND    hListWnd;
    int     nFocusPanel;      // 0: left  1: right
	WINDOWPLACEMENT pos;
	int		nSplitPos;
	Root	root;
} ChildWnd;


#define STATUS_WINDOW   2001
#define TREE_WINDOW     2002
#define LIST_WINDOW     2003
#define SPLIT_WINDOW    2004

#define MAX_LOADSTRING  100
#define	SPLIT_WIDTH		3

////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//
extern HINSTANCE hInst;
extern HWND hFrameWnd;
extern HWND hStatusBar;
extern HMENU hMenuFrame;

extern TCHAR szTitle[];
extern TCHAR szFrameClass[];
extern TCHAR szChildClass[];

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
