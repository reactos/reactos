/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            soft386.c
 * PURPOSE:         Functions meant for use by the host.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

// #define WIN32_NO_STATUS
// #define _INC_WINDOWS
#include <windef.h>

// #define NDEBUG
#include <debug.h>

#include <soft386.h>
#include "common.h"
#include "opcodes.h"

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
inline
VOID
NTAPI
Soft386ExecutionControl(PSOFT386_STATE State, INT Command)
{
    UCHAR Opcode;
    INT ProcedureCallCount = 0;

    /* Main execution loop */
    do
    {
        /* Perform an instruction fetch */
        if (!Soft386FetchByte(State, &Opcode)) continue;

        // TODO: Check for CALL/RET to update ProcedureCallCount.

        if (Soft386OpcodeHandlers[Opcode] != NULL)
        {
            /* Call the opcode handler */
            Soft386OpcodeHandlers[Opcode](State, Opcode);
        }
        else
        {
            /* This is not a valid opcode */
            Soft386Exception(State, SOFT386_EXCEPTION_UD);
        }

        if (Soft386OpcodeHandlers[Opcode] != Soft386OpcodePrefix)
        {
            /* A non-prefix opcode has been executed, reset the prefix flags */
            State->PrefixFlags = 0;
        }

        /* Increment the time stamp counter */
        State->TimeStampCounter++;
    }
    while ((Command == SOFT386_CONTINUE)
           || (Command == SOFT386_STEP_OVER && ProcedureCallCount > 0)
           || (Command == SOFT386_STEP_OUT && ProcedureCallCount >= 0));
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
    DPRINT1("\nCPU currently executing in %s mode at %04X:%08X\n"
            "Time Stamp Counter = %016X\n",
            (State->ControlRegisters[0] & SOFT386_CR0_PE) ? "protected" : "real",
            State->SegmentRegs[SOFT386_REG_CS].Selector,
            State->InstPtr.Long,
            State->TimeStampCounter);
    DPRINT1("\nGeneral purpose registers:\n"
            "EAX = %08X\tECX = %08X\tEDX = %08X\tEBX = %08X\n"
            "ESP = %08X\tEBP = %08X\tESI = %08X\tEDI = %08X\n",
            State->GeneralRegs[SOFT386_REG_EAX].Long,
            State->GeneralRegs[SOFT386_REG_ECX].Long,
            State->GeneralRegs[SOFT386_REG_EDX].Long,
            State->GeneralRegs[SOFT386_REG_EBX].Long,
            State->GeneralRegs[SOFT386_REG_ESP].Long,
            State->GeneralRegs[SOFT386_REG_EBP].Long,
            State->GeneralRegs[SOFT386_REG_ESI].Long,
            State->GeneralRegs[SOFT386_REG_EDI].Long);
    DPRINT1("\nSegment registers:\n"
            "ES = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "CS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "SS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "DS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "FS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "GS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n",
            State->SegmentRegs[SOFT386_REG_ES].Selector,
            State->SegmentRegs[SOFT386_REG_ES].Base,
            State->SegmentRegs[SOFT386_REG_ES].Limit,
            State->SegmentRegs[SOFT386_REG_ES].Dpl,
            State->SegmentRegs[SOFT386_REG_CS].Selector,
            State->SegmentRegs[SOFT386_REG_CS].Base,
            State->SegmentRegs[SOFT386_REG_CS].Limit,
            State->SegmentRegs[SOFT386_REG_CS].Dpl,
            State->SegmentRegs[SOFT386_REG_SS].Selector,
            State->SegmentRegs[SOFT386_REG_SS].Base,
            State->SegmentRegs[SOFT386_REG_SS].Limit,
            State->SegmentRegs[SOFT386_REG_SS].Dpl,
            State->SegmentRegs[SOFT386_REG_DS].Selector,
            State->SegmentRegs[SOFT386_REG_DS].Base,
            State->SegmentRegs[SOFT386_REG_DS].Limit,
            State->SegmentRegs[SOFT386_REG_DS].Dpl,
            State->SegmentRegs[SOFT386_REG_FS].Selector,
            State->SegmentRegs[SOFT386_REG_FS].Base,
            State->SegmentRegs[SOFT386_REG_FS].Limit,
            State->SegmentRegs[SOFT386_REG_FS].Dpl,
            State->SegmentRegs[SOFT386_REG_GS].Selector,
            State->SegmentRegs[SOFT386_REG_GS].Base,
            State->SegmentRegs[SOFT386_REG_GS].Limit,
            State->SegmentRegs[SOFT386_REG_GS].Dpl);
    DPRINT1("\nFlags: %08X (%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s) Iopl: %u\n",
            State->Flags.Long,
            State->Flags.Cf ? "CF" : "cf",
            State->Flags.Pf ? "PF" : "pf",
            State->Flags.Af ? "AF" : "af",
            State->Flags.Zf ? "ZF" : "zf",
            State->Flags.Sf ? "SF" : "sf",
            State->Flags.Tf ? "TF" : "tf",
            State->Flags.If ? "IF" : "if",
            State->Flags.Df ? "DF" : "df",
            State->Flags.Of ? "OF" : "of",
            State->Flags.Nt ? "NT" : "nt",
            State->Flags.Rf ? "RF" : "rf",
            State->Flags.Vm ? "VM" : "vm",
            State->Flags.Ac ? "AC" : "ac",
            State->Flags.Vif ? "VIF" : "vif",
            State->Flags.Vip ? "VIP" : "vip",
            State->Flags.Iopl);
    DPRINT1("\nControl Registers:\n"
            "CR0 = %08X\tCR1 = %08X\tCR2 = %08X\tCR3 = %08X\n"
            "CR4 = %08X\tCR5 = %08X\tCR6 = %08X\tCR7 = %08X\n",
            State->ControlRegisters[SOFT386_REG_CR0],
            State->ControlRegisters[SOFT386_REG_CR1],
            State->ControlRegisters[SOFT386_REG_CR2],
            State->ControlRegisters[SOFT386_REG_CR3],
            State->ControlRegisters[SOFT386_REG_CR4],
            State->ControlRegisters[SOFT386_REG_CR5],
            State->ControlRegisters[SOFT386_REG_CR6],
            State->ControlRegisters[SOFT386_REG_CR7]);
    DPRINT1("\nDebug Registers:\n"
            "DR0 = %08X\tDR1 = %08X\tDR2 = %08X\tDR3 = %08X\n"
            "DR4 = %08X\tDR5 = %08X\tDR6 = %08X\tDR7 = %08X\n",
            State->DebugRegisters[SOFT386_REG_DR0],
            State->DebugRegisters[SOFT386_REG_DR1],
            State->DebugRegisters[SOFT386_REG_DR2],
            State->DebugRegisters[SOFT386_REG_DR3],
            State->DebugRegisters[SOFT386_REG_DR4],
            State->DebugRegisters[SOFT386_REG_DR5],
            State->DebugRegisters[SOFT386_REG_DR6],
            State->DebugRegisters[SOFT386_REG_DR7]);
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
    SOFT386_IDLE_PROC IdleCallback = State->IdleCallback;

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
    State->ControlRegisters[SOFT386_REG_CR0] |= SOFT386_CR0_ET;
    
    /* Restore the callbacks */
    State->MemReadCallback = MemReadCallback;
    State->MemWriteCallback = MemWriteCallback;
    State->IoReadCallback = IoReadCallback;
    State->IoWriteCallback = IoWriteCallback;
    State->IdleCallback = IdleCallback;
}

VOID
NTAPI
Soft386Interrupt(PSOFT386_STATE State, UCHAR Number)
{
    // TODO: NOT IMPLEMENTED!!!
    UNIMPLEMENTED;
}

VOID
NTAPI
Soft386ExecuteAt(PSOFT386_STATE State, USHORT Segment, ULONG Offset)
{
    /* Load the new CS */
    if (!Soft386LoadSegment(State, SOFT386_REG_CS, Segment))
    {
        /* An exception occurred, let the handler execute instead */
        return;
    }

    /* Set the new IP */
    State->InstPtr.Long = Offset;
}

/* EOF */
