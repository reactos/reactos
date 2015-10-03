/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/cpu/bop.c
 * PURPOSE:         BIOS Operation Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE VARIABLES **********************************************************/

/*
 * This is the list of registered BOP handlers.
 */
static EMULATOR_BOP_PROC BopProc[EMULATOR_MAX_BOP_NUM] = { NULL };

/* PUBLIC FUNCTIONS ***********************************************************/

VOID RegisterBop(BYTE BopCode, EMULATOR_BOP_PROC BopHandler)
{
    BopProc[BopCode] = BopHandler;
}

VOID FASTCALL EmulatorBiosOperation(PFAST486_STATE State, UCHAR BopCode)
{
    WORD StackSegment, StackPointer;
    LPWORD Stack;

    /* Get the SS:SP */
    StackSegment = State->SegmentRegs[FAST486_REG_SS].Selector;
    StackPointer = State->GeneralRegs[FAST486_REG_ESP].LowWord;

    /* Get the stack */
    Stack = (LPWORD)SEG_OFF_TO_PTR(StackSegment, StackPointer);

    /* Call the BOP handler */
    if (BopProc[BopCode] != NULL)
        BopProc[BopCode](Stack);
    else
        DPRINT1("Invalid BOP code: 0x%02X\n", BopCode);
}

/* EOF */
