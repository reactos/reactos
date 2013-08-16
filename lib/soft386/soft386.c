/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            soft386.c
 * PURPOSE:         Functions meant for use by the host.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "soft386.h"

/* DEFINES ********************************************************************/

typedef enum
{
    SOFT386_STEP_INTO,
    SOFT386_STEP_OVER,
    SOFT386_STEP_OUT,
    SOFT386_CONTINUE
} SOFT386_EXEC_CMD;

/* PRIVATE FUNCTIONS **********************************************************/

static
VOID
NTAPI
Soft386ExecutionControl(PSOFT386_STATE State, INT Command)
{
    // TODO: NOT IMPLEMENTED!!!
    UNIMPLEMENTED;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
Soft386Continue(PSOFT386_STATE State)
{
    /* Call the internal function */
    Soft386ExecutionControl(State, SOFT386_CONTINUE);
}

VOID
NTAPI
Soft386StepInto(PSOFT386_STATE State)
{
    /* Call the internal function */
    Soft386ExecutionControl(State, SOFT386_STEP_INTO);
}

VOID
NTAPI
Soft386StepOver(PSOFT386_STATE State)
{
    /* Call the internal function */
    Soft386ExecutionControl(State, SOFT386_STEP_OVER);
}

VOID
NTAPI
Soft386StepOut(PSOFT386_STATE State)
{
    /* Call the internal function */
    Soft386ExecutionControl(State, SOFT386_STEP_OUT);
}

VOID
NTAPI
Soft386DumpState(PSOFT386_STATE State)
{
    // TODO: NOT IMPLEMENTED!!!
    UNIMPLEMENTED;
}

VOID
NTAPI
Soft386Reset(PSOFT386_STATE State)
{
    INT i;
    SOFT386_MEM_READ_PROC MemReadCallback = State->MemReadCallback;
    SOFT386_MEM_WRITE_PROC MemWriteCallback = State->MemWriteCallback;
    SOFT386_IO_READ_PROC IoReadCallback = State->IoReadCallback;
    SOFT386_IO_WRITE_PROC IoWriteCallback = State->IoWriteCallback;

    /* Clear the entire structure */
    RtlZeroMemory(State, sizeof(*State));

    /* Initialize the registers */
    State->Flags.AlwaysSet = 1;
    State->InstPtr.LowWord = 0xFFF0;

    /* Initialize segments */
    for (i = 0; i < SOFT386_NUM_SEG_REGS; i++)
    {
        /* Set the selector, base and limit, other values don't apply in real mode */
        State->SegmentRegs[i].Selector = 0;
        State->SegmentRegs[i].Base = 0;
        State->SegmentRegs[i].Limit = 0xFFFF;
    }

    /* Initialize the code segment */
    State->SegmentRegs[SOFT386_REG_CS].Selector = 0xF000;
    State->SegmentRegs[SOFT386_REG_CS].Base = 0xFFFF0000;

    /* Initialize the IDT */
    State->Idtr.Size = 0x3FF;
    State->Idtr.Address = 0;

    /* Initialize CR0 */
    State->ControlRegisters[0] |= SOFT386_CR0_ET;
    
    /* Restore the callbacks */
    State->MemReadCallback = MemReadCallback;
    State->MemWriteCallback = MemWriteCallback;
    State->IoReadCallback = IoReadCallback;
    State->IoWriteCallback = IoWriteCallback;
}
