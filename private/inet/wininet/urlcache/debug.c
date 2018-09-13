/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Contains debug functions.

Author:

    Madan Appiah (madana) 15-Nov-1994

Environment:

    User Mode - Win32

Revision History:

--*/


#if DBG


#include <windows.h>
#include <winnt.h>

#include <stdlib.h>
#include <stdio.h>
#include <debug.h>



VOID
InternetDebugPrintValist(
    IN LPSTR Format,
    va_list list
    );

extern BOOL UrlcacheDebugEnabled;

VOID
TcpsvcsDbgPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;

    if (!UrlcacheDebugEnabled) {

        return;

    }

    va_start(arglist, Format);

    InternetDebugPrintValist(Format, arglist);

    va_end(arglist);
}

#endif // DBG
