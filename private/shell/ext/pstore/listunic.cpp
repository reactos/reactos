/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    listunic.cpp

Abstract:

    This module implements Unicode -> ANSI string thunking for the listview
    functions on Win95, and Unicode -> Unicode passthru on WinNT.

Author:

    Scott Field (sfield)    13-Mar-97

--*/

//
// make sure we bring in Unicode version of ListView macros + definitions
//

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <shlobj.h>

#include "listu.h"
#include "unicode.h"

int
ListView_InsertItemU(
    HWND hwnd,
    const LV_ITEM FAR *pitem
    )
{
    if(FIsWinNT())
        return ListView_InsertItemUnicode(hwnd, pitem);

    return ListView_InsertItemAnsi(hwnd, pitem);
}

VOID
WINAPI
ListView_SetItemTextU(
    HWND hwnd,
    int i,
    int iSubItem,
    LPWSTR pszText
    )
{
    if(FIsWinNT())
        ListView_SetItemTextUnicode(hwnd, i, iSubItem, pszText);
    else
        ListView_SetItemTextAnsi(hwnd, i, iSubItem, pszText);
}



int
ListView_InsertItemUnicode(
    HWND hwnd,
    const LV_ITEM FAR *pitem
    )
{

    //
    // the strings we were passed are already Unicode, just pass control
    // directly to the Unicode version.
    //

    return ListView_InsertItem(hwnd, pitem);
}

VOID
WINAPI
ListView_SetItemTextUnicode(
    HWND hwnd,
    int i,
    int iSubItem,
    LPWSTR pszText
    )
{
    //
    // the strings we were passed are already Unicode, just pass control
    // directly to the Unicode version.
    //

    ListView_SetItemText(hwnd, i, iSubItem, pszText);
}

