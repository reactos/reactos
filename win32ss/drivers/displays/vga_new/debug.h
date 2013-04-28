/*
 * PROJECT:         ReactOS VGA Display Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            boot/drivers/video/displays/vga/debug.h
 * PURPOSE:         Debug Support Header
 * PROGRAMMERS:     Copyright (c) 1992-1995 Microsoft Corporation
 */

#if DBG

VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

#define DISPDBG(arg) DebugPrint arg
#define RIP(x) { DebugPrint(0, x); EngDebugBreak();}

#else

#define DISPDBG(arg)
#define RIP(x)

#endif
