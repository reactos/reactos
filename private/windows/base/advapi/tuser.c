/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tuser.c

Abstract:

    This module tests windows GetUserName API.

Author:

    Dave Snipp (DaveSn) 27-May-92

Revision History:

--*/

#include <windows.h>
#include <stdio.h>

CHAR BufferA[256];
WCHAR BufferW[256];

DWORD cbBufA = 256, cbBufW = 256;

int
main (void)
{
    if (GetUserNameW(BufferW, &cbBufW))
        printf("UniCode UserName : %ws\nNo of Characters = %d\n", BufferW, cbBufW);
    else
        printf("UniCode Failed : 0x%0x\n", GetLastError());

    if (GetUserNameA(BufferA, &cbBufA))
        printf("Ansi UserName : %s\nNo of Characters = %d\n", BufferA, cbBufA);
    else
        printf("Ansi Failed : 0x%0x\n", GetLastError());

    cbBufW=0;

    if (!GetUserNameW(BufferW, &cbBufW)) {
        printf("GetUserNameW requires %d size buffer\n", cbBufW);
    } else
        printf("GetUserNameW should not have succeeded\n");

    cbBufA=0;

    if (!GetUserNameA(BufferA, &cbBufA)) {
        printf("GetUserNameA requires %d size buffer\n", cbBufA);
    } else
        printf("GetUserNameA should not have succeeded\n");

    return(0);
}
