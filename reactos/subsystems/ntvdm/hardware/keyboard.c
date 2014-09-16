/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            keyboard.c
 * PURPOSE:         Keyboard emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "keyboard.h"
#include "ps2.h"

static BYTE PS2Port = 0;

/* PUBLIC FUNCTIONS ***********************************************************/

VOID KeyboardEventHandler(PKEY_EVENT_RECORD KeyEvent)
{
    WORD i;
    BYTE ScanCode = (BYTE)KeyEvent->wVirtualScanCode;

    /* If this is a key release, set the highest bit in the scan code */
    if (!KeyEvent->bKeyDown) ScanCode |= 0x80;

    /* Push the scan code into the PS/2 queue */
    for (i = 0; i < KeyEvent->wRepeatCount; i++)
    {
        PS2QueuePush(PS2Port, ScanCode);
    }

    // PicInterruptRequest(1);
}

VOID KeyboardCommand(BYTE Command)
{
    UNIMPLEMENTED;
}

BOOLEAN KeyboardInit(BYTE PS2Connector)
{
    PS2Port = PS2Connector;
    return TRUE;
}
