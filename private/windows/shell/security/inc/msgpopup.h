//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msgpopup.h
//
//  Definitions and prototypes for MessageBox helper functions.
//
//--------------------------------------------------------------------------

#ifndef _MSGPOPUP_H_
#define _MSGPOPUP_H_

#ifdef __cplusplus
extern "C" {
#endif

int
WINAPIV
MsgPopup(HWND hwnd,                 // Owner window
         LPCTSTR pszMsg,            // May be resource ID
         LPCTSTR pszTitle,          // May be resource ID or NULL
         UINT uType,                // MessageBox flags
         HINSTANCE hInstance,       // Resource strings loaded from here
         ...);                      // Arguments to insert into pszMsg
int
WINAPI
SysMsgPopup(HWND hwnd,
            LPCTSTR pszMsg,
            LPCTSTR pszTitle,
            UINT uType,
            HINSTANCE hInstance,
            DWORD dwErrorID,
            LPCTSTR pszInsert2 = NULL);

#ifdef __cplusplus
}
#endif

#endif  // _MSGPOPUP_H_
