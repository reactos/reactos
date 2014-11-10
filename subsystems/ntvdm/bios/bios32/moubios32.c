/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            moubios32.c
 * PURPOSE:         VDM Mouse 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"

#include "moubios32.h"
#include "bios32p.h"

#include "io.h"
#include "hardware/mouse.h"

/* PRIVATE VARIABLES **********************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

// Mouse IRQ 12
static VOID WINAPI BiosMouseIrq(LPWORD Stack)
{
    PicIRQComplete(Stack);
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN MouseBios32Initialize(VOID)
{
    /* Set up the HW vector interrupts */
    EnableHwIRQ(12, BiosMouseIrq);

    return TRUE;
}

VOID MouseBios32Cleanup(VOID)
{
}
