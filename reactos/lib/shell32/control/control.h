/*
 *  ReactOS shell32 - Control Panel
 *
 *  control.h
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

#ifndef __CONTROL_H__
#define __CONTROL_H__

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

#define MAX_CPL_NAME 128
#define MAX_CPL_INFO 128

////////////////////////////////////////////////////////////////////////////////
/*
typedef struct tagNEWCPLINFO { 
    DWORD dwSize; 
    DWORD dwFlags; 
    DWORD dwHelpContext; 
    LONG  lData; 
    HICON hIcon; 
    TCHAR  szName[32]; 
    TCHAR  szInfo[64]; 
    TCHAR  szHelpFile[128]; 
} NEWCPLINFO; 
    
 */
typedef struct tagCPLAppletINFO {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwHelpContext;
    LONG  lData;
    HICON hIcon;
    TCHAR  szName[32];
    TCHAR  szInfo[192];
//    TCHAR  szHelpFile[128];
} CPLAppletINFO; 
    

typedef struct CPlApplet {
    struct CPlApplet* next;     /* linked list */
    HWND              hWnd;
    unsigned          count;    /* number of subprograms */
    HMODULE           hModule;  /* module of loaded applet */
    TCHAR             filename[MAX_PATH];
    APPLET_PROC       proc;     /* entry point address */
    NEWCPLINFO        info[1];  /* array of count information. dwSize field is 0 if entry is invalid */
} CPlApplet;

typedef struct CPlEntry {
    CPlApplet* pCPlApplet;      /* which cpl module we are associated with (contained in) */
    HWND       hWnd;            /* handle to existing window if we are already launched */
    unsigned   nSubProg;        /* which sub-program we are within the CPlApplet */
    unsigned   nIconIndex;      /*  */
    long       lData;
//    union {
//        NEWCPLINFO    NewCplInfo;
//        CPLAppletINFO AppletInfo;
//    } info;
} CPlEntry;

typedef struct CPanel {
    CPlApplet* first;           /* linked list */
    HWND       hWnd;
    unsigned   status;
    CPlApplet* clkApplet;
    unsigned   clkSP;
} CPanel;

#ifndef CPL_STARTWPARMSW
#undef CPL_STARTWPARMS
#define CPL_STARTWPARMSW 10
#ifdef UNICODE
#define CPL_STARTWPARMS CPL_STARTWPARMSW
#else
#define CPL_STARTWPARMS CPL_STARTWPARMSA
#endif
#endif


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
extern TCHAR szWindowClass[];

void Control_DoLaunch(CPlApplet** pListHead, HWND hWnd, LPCTSTR cmd);
CPlApplet* Control_LoadApplet(HWND hWnd, LPCTSTR cmd, CPlApplet** pListHead);
CPlApplet* Control_UnloadApplet(CPlApplet* applet);


#ifdef __GNUC__
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
#define HDITEM HD_ITEM
#define LPNMLISTVIEW LPNM_LISTVIEW
#define NMLISTVIEW NM_LISTVIEW
#define HDN_ENDDRAG TBN_ENDDRAG
#define LVSICF_NOSCROLL LVS_NOSCROLL
#define HDM_GETORDERARRAY	(HDM_FIRST+19)   // TODO: FIX ME
#endif


#ifdef __cplusplus
};
#endif

#endif // __CONTROL_H__
