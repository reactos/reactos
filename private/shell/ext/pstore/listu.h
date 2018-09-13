/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    listu.h

Abstract:

    This module implements Unicode -> ANSI string thunking for the listview
    functions on Win95, and Unicode -> Unicode passthru on WinNT.

Author:

    Scott Field (sfield)    13-Mar-97

--*/

#ifndef __LISTU_H__
#define __LISTU_H__

#ifdef __cplusplus
extern "C" {
#endif



//
// Unicode to ANSI conversion stubs.
//

int
ListView_InsertItemU(
    HWND hwnd,
    const LV_ITEM FAR *pitem
    );

VOID
WINAPI
ListView_SetItemTextU(
    HWND hwnd,
    int i,
    int iSubItem,
    LPWSTR pszText
    );


//
// ANSI function prototypes
//

int
ListView_InsertItemAnsi(
    HWND hwnd,
    const LV_ITEM FAR *pitem
    );

VOID
WINAPI
ListView_SetItemTextAnsi(
    HWND hwnd,
    int i,
    int iSubItem,
    LPWSTR pszText
    );

//
// Unicode function prototypes
//

int
ListView_InsertItemUnicode(
    HWND hwnd,
    const LV_ITEM FAR *pitem
    );

VOID
WINAPI
ListView_SetItemTextUnicode(
    HWND hwnd,
    int i,
    int iSubItem,
    LPWSTR pszText
    );

#ifdef __cplusplus
}
#endif

#endif  // __LISTU_H__
