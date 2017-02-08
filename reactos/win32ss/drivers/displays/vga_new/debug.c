/*
 * PROJECT:         ReactOS VGA Display Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            win32ss/drivers/displays/vga_new/debug.c
 * PURPOSE:         Debug Support
 * PROGRAMMERS:     Copyright (c) 1992-1995 Microsoft Corporation
 */

#include "driver.h"

#if DBG

ULONG DebugLevel = 0xFFFFFFFF;

/*****************************************************************************
 *
 *   Routine Description:
 *
 *      This function is variable-argument, level-sensitive debug print
 *      routine.
 *      If the specified debug level for the print statement is lower or equal
 *      to the current debug level, the message will be printed.
 *
 *   Arguments:
 *
 *      DebugPrintLevel - Specifies at which debugging level the string should
 *          be printed
 *
 *      DebugMessage - Variable argument ascii c string
 *
 *   Return Value:
 *
 *      None.
 *
 ***************************************************************************/

VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

{

    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel)
    {
        EngDebugPrint(STANDARD_DEBUG_PREFIX, DebugMessage, ap);
    }

    va_end(ap);

}

#endif
