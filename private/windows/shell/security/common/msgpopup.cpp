//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msgpopup.cpp
//
//  This file contains MessageBox helper functions.
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop


/*******************************************************************

    NAME:       MsgPopup

    SYNOPSIS:   Displays a message to the user

    ENTRY:      hwnd        - Owner window handle
                pszMsgFmt   - Main message text
                pszTitle    - MessageBox title
                uType       - MessageBox flags
                hInstance   - Module to load strings from.  Only required if
                              pszMsgFmt or pszTitle is a string resource ID.
                Optional format insert parameters.

    EXIT:

    RETURNS:    MessageBox result

    NOTES:      Either of the string parameters may be string resource ID's.

    HISTORY:
        JeffreyS    11-Jun-1997     Created

********************************************************************/

int
WINAPIV
MsgPopup(HWND hwnd,
         LPCTSTR pszMsgFmt,
         LPCTSTR pszTitle,
         UINT uType,
         HINSTANCE hInstance,
         ...)
{
    int nResult;
    LPTSTR szMsg = NULL;
    LPTSTR szTitle = NULL;
    DWORD dwFormatResult;
    va_list args;

    if (pszMsgFmt == NULL)
        return -1;

    //
    // Insert arguments into the format string
    //
    va_start(args, hInstance);
    if (IS_INTRESOURCE(pszMsgFmt))
        dwFormatResult = vFormatStringID(&szMsg, hInstance, (UINT)((ULONG_PTR)pszMsgFmt), &args);
    else
        dwFormatResult = vFormatString(&szMsg, pszMsgFmt, &args);
    va_end(args);

    if (!dwFormatResult)
        return -1;

    //
    // Load the caption if necessary
    //
    if (pszTitle && IS_INTRESOURCE(pszTitle))
    {
        if (LoadStringAlloc(&szTitle, hInstance, (UINT)((ULONG_PTR)pszTitle)))
            pszTitle = szTitle;
        else
            pszTitle = NULL;
    }

    //
    // Display message box
    //
    nResult = MessageBox(hwnd, szMsg, pszTitle, uType);

    LocalFreeString(&szMsg);
    LocalFreeString(&szTitle);

    return nResult;
}


/*******************************************************************

    NAME:       SysMsgPopup

    SYNOPSIS:   Displays a message to the user using a system error
                message as an insert.

    ENTRY:      hwnd        - Owner window handle
                pszMsg      - Main message text
                pszTitle    - MessageBox title
                uType       - MessageBox flags
                hInstance   - Module to load strings from.  Only required if
                              pszMsg or pszTitle is a string resource ID.
                dwErrorID   - System defined error code (Insert 1)
                pszInsert2  - Optional string to be inserted into pszMsg

    EXIT:

    RETURNS:    MessageBox result

    NOTES:      Any of the string parameters may be string resource ID's.

    HISTORY:
        JeffreyS    11-Jun-1997     Created

********************************************************************/

int
WINAPI
SysMsgPopup(HWND hwnd,
            LPCTSTR pszMsg,
            LPCTSTR pszTitle,
            UINT uType,
            HINSTANCE hInstance,
            DWORD dwErrorID,
            LPCTSTR pszInsert2)
{
    int nResult;
    LPTSTR szInsert2 = NULL;
    LPTSTR szErrorText = NULL;

    //
    // Load the 2nd insert string if necessary
    //
    if (pszInsert2 && IS_INTRESOURCE(pszInsert2))
    {
        if (LoadStringAlloc(&szInsert2, hInstance, (UINT)((ULONG_PTR)pszInsert2)))
            pszInsert2 = szInsert2;
        else
            pszInsert2 = NULL;
    }

    //
    // Get the error message string
    //
    if (dwErrorID)
    {
        GetSystemErrorText(&szErrorText, dwErrorID);
    }

    nResult = MsgPopup(hwnd, pszMsg, pszTitle, uType, hInstance, szErrorText, pszInsert2);

    LocalFreeString(&szInsert2);
    LocalFreeString(&szErrorText);

    return nResult;
}
