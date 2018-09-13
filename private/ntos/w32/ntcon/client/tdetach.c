/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    tdetach.c

Abstract:

    Test program for detached processes

Author:

    Therese Stowell (thereses) July-15-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop

CHAR String[11] = "detachedpr";

BOOL
CallConsoleApi(
    IN WORD y
    )
{
    CHAR_INFO Buffer[10];
    COORD BufferSize;
    COORD BufferCoord;
    SMALL_RECT WriteRegion;
    int i;
    BOOL Success;

    BufferSize.X = 10;
    BufferSize.Y = 1;
    BufferCoord.X = 0;
    BufferCoord.Y = 0;
    WriteRegion.Left = 0;
    WriteRegion.Top = y;
    WriteRegion.Right = 14;
    WriteRegion.Bottom = y;
    for (i=0;i<10;i++) {
        Buffer[i].Char.AsciiChar = String[i];
        Buffer[i].Attributes = y;
    }
    Success = WriteConsoleOutput(GetStdHandle(STD_OUTPUT_HANDLE),
                                 Buffer,
                                 BufferSize,
                                 BufferCoord,
                                 &WriteRegion
                                );
    return Success;
}

DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    DbgPrint("entering tdetach\n");
    if (CallConsoleApi(5))
        DbgPrint("TDETACH: CallConsoleApi succeeded\n");
}
