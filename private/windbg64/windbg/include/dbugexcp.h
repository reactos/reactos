/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dbugexcp.h

Abstract:

    Function prototypes exported from dbugexcp.c

Author:

    Kent Forschmiedt (kentf)  02-15-1994

Environment:

    Win32 - User

--*/

#ifndef _DBUGEXCP_H
#define _DBUGEXCP_H

void
InsertException(
    EXCEPTION_LIST **List,
    EXCEPTION_LIST *Item
    );

BOOL
GetDefaultExceptionList(
    VOID
    );

LOGERR
ParseException(
    LPSTR   String,
    UINT    Radix,
    BOOL *fException,
    BOOL *fEfd,
    BOOL *fName,
    BOOL *fCmd,
    BOOL *fCmd2,
    BOOL *fInvalid,
    DWORD   *pException,
    EXCEPTION_FILTER_DEFAULT *pEfd,
    LPSTR   *lpName,
    LPSTR   *lpCmd,
    LPSTR   *lpCmd2
    );

void
FormatException (
    EXCEPTION_FILTER_DEFAULT Efd,
    DWORD   Exception,
    LPSTR   lpName,
    LPSTR   lpCmd,
    LPSTR   lpCmd2,
    LPSTR   Separator,
    LPSTR   Buffer
    );

LOGERR
HandleSpecialExceptions(
    DWORD SpecialFunction,
    DWORD Action,
    LPSTR CommandTail
    );

#endif // _DBUGEXCP_H
