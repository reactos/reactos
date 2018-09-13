/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    listansi.cpp

Abstract:

    This module implements Unicode -> ANSI string thunking for the listview
    functions on Win95.

Author:

    Scott Field (sfield)    13-Mar-97

--*/

//
// make sure we bring in ANSI version of ListView macros + definitions
//

#undef UNICODE
#include <windows.h>
#include <shlobj.h>

#include "listu.h"

int
ListView_InsertItemAnsi(
    HWND hwnd,
    const LV_ITEM FAR *pitem
    )
{
    LV_ITEM NewItem;
    int iRet;

    LPWSTR pszText = (LPWSTR)(pitem->pszText);

    LPSTR pszTextAnsi;
    CHAR FastBuffer[MAX_PATH + 1];
    LPSTR SlowBuffer = NULL;

    DWORD cchText;

    //
    // convert the input Unicode strings to ANSI and then call the ANSI
    // version (for Win95).
    //


    cchText = lstrlenW(pszText);

    if(cchText == 0 || pszText == NULL) {
        pszTextAnsi = NULL;
    } else {

        DWORD cchRequired;

        //
        // convert supplied Unicode buffer to ANSI
        // try fast buffer first.
        //

        cchRequired = WideCharToMultiByte(
            CP_ACP,
            0,
            pszText,
            cchText + 1, // include NULL
            FastBuffer,
            0,
            NULL,
            NULL
            );

        if(cchRequired > sizeof(FastBuffer)) {
            SlowBuffer = (LPSTR)HeapAlloc(GetProcessHeap(), 0, cchRequired);
            if(SlowBuffer == NULL) return -1;
            pszTextAnsi = SlowBuffer;
        } else {
            pszTextAnsi = FastBuffer;
        }

        cchRequired = WideCharToMultiByte(
            CP_ACP,
            0,
            pszText,
            cchText + 1, // include NULL
            pszTextAnsi,
            cchRequired,
            NULL,
            NULL
            );
    }


    //
    // copy existing structure contents.
    //

    CopyMemory(&NewItem, pitem, sizeof(NewItem));

    NewItem.pszText = pszTextAnsi;

    // make call to ANSI version
    iRet = ListView_InsertItem(hwnd, &NewItem);

    if(SlowBuffer)
        HeapFree(GetProcessHeap(), 0, SlowBuffer);

    return iRet;
}

VOID
WINAPI
ListView_SetItemTextAnsi(
    HWND hwnd,
    int i,
    int iSubItem,
    LPWSTR pszText
    )
{
    LPSTR pszTextAnsi;
    CHAR FastBuffer[MAX_PATH + 1];
    LPSTR SlowBuffer = NULL;

    DWORD cchText;

    cchText = lstrlenW(pszText);

    if(cchText == 0 || pszText == NULL) {
        pszTextAnsi = NULL;
    } else {

        DWORD cchRequired;

        //
        // convert supplied Unicode buffer to ANSI
        // try fast buffer first.
        //

        cchRequired = WideCharToMultiByte(
            CP_ACP,
            0,
            pszText,
            cchText + 1, // include NULL
            FastBuffer,
            0,
            NULL,
            NULL
            );

        if(cchRequired > sizeof(FastBuffer)) {
            SlowBuffer = (LPSTR)HeapAlloc(GetProcessHeap(), 0, cchRequired);
            if(SlowBuffer == NULL) return;
            pszTextAnsi = SlowBuffer;
        } else {
            pszTextAnsi = FastBuffer;
        }

        cchRequired = WideCharToMultiByte(
            CP_ACP,
            0,
            pszText,
            cchText + 1, // include NULL
            pszTextAnsi,
            cchRequired,
            NULL,
            NULL
            );
    }


    //
    // convert the input Unicode strings to ANSI and then call the ANSI
    // version (for Win95).
    //


    ListView_SetItemText(hwnd, i, iSubItem, pszTextAnsi);

    if(SlowBuffer)
        HeapFree(GetProcessHeap(), 0, SlowBuffer);
}
