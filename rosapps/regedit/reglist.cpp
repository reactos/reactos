/*
 *  ReactOS regedit
 *
 *  reglist.cpp
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
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include "regedit.h"
#include "reglist.h"


// Global Variables:
extern HINSTANCE hInst;
extern HWND hMainWnd;


HWND CreateListView(HWND hwndParent, LPSTR lpszFileName) 
{ 
    RECT rcClient;  // dimensions of client area 
    HWND hwndLV;    // handle to list view control 
 
    // Get the dimensions of the parent window's client area, and create the list view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndLV = CreateWindowEx(0, WC_LISTVIEW, "List View", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES, 
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)LIST_WINDOW, hInst, NULL); 
 
    // Initialize the image list, and add items to the control. 
/*
    if (!InitListViewImageLists(hwndLV) || 
            !InitListViewItems(hwndLV, lpszFileName)) { 
        DestroyWindow(hwndLV); 
        return FALSE; 
    } 
 */
    return hwndLV;
} 

