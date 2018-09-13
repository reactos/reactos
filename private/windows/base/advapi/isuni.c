/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    isuni.c

Abstract:

    Covering routine for RtlIsUnicode string, since this is declare a BOOL
    API and Rtl is BOOLEAN

Author:

    Steve Wood (stevewo) 16-Dec-1993

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

BOOL
WINAPI
IsTextUnicode(
    LPVOID lpv,
    int iSize,
    LPINT lpiResult
    )
{
    if (RtlIsTextUnicode( lpv, iSize, lpiResult )) {
        return TRUE;
        }
    else {
        return FALSE;
        }
}
