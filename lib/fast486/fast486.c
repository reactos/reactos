/*
 * Fast486 386/486 CPU Emulation Library
 * fast486.c
 *
 * Copyright (C) 2013 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* INCLUDES *******************************************************************/

#include <windef.h>

// #define NDEBUG
#include <debug.h>

#include <fast486.h>
#include "common.h"
#include "opcodes.h"

/* DEFINES ********************************************************************/

typedef enum
{
    FAST486_STEP_INTO,
    FAST486_STEP_OVER,
    FAST486_STEP_OUT,
    FAST486_CONTINUE
} FAST486_EXEC_CMD;

/* PRIVATE FUNCTIONS **********************************************************/

static
inline
VOID
NTAPI
Fast486ExecutionControl(PFAST486_STATE State, INT Command)
{
    UCHAR Opcode;
    INT ProcedureCallCount = 0;

    /* Main execution loop */
    do
    {
        /* Check if this is a new instruction */
        if (State->PrefixFlags == 0)
        {
            State->SavedInstPtr = State->InstPtr;

            /*
             * Check if there is an interrupt to execute, or a hardware interrupt signal
             * while interrupts are enabled.
             */
            if ((State->IntStatus == FAST486_INT_EXECUTE)
                || (State->Flags.If
                && (State->IntAckCallback != NULL)
                && (State->IntStatus == FAST486_INT_SIGNAL)))
            {
                FAST486_IDT_ENTRY IdtEntry;

                if (State->IntStatus == FAST486_INT_SIGNAL)
                {
                    /* Acknowledge the interrupt to get the number */
                    State->PendingIntNum = State->IntAckCallback(State);
                }

                /* Get the interrupt vector */
                if (Fast486GetIntVector(State, State->PendingIntNum, &IdtEntry))
                {
                    /* Perform the interrupt */
                    Fast486InterruptInternal(State,
                                             IdtEntry.Selector,
                                             MAKELONG(IdtEntry.Offset, IdtEntry.OffsetHigh),
                                             IdtEntry.Type);
                }

                /* Clear the interrupt status */
                State->IntStatus = FAST486_INT_NONE;
            }
        }

        /* Perform an instruction fetch */
        if (!Fast486FetchByte(State, &Opcode)) continue;

        // TODO: Check for CALL/RET to update ProcedureCallCount.

        if (Fast486OpcodeHandlers[Opcode] != NULL)
        {
            /* Call the opcode handler */
            Fast486OpcodeHandlers[Opcode](State, Opcode);
        }
        else
        {
            /* This is not a valid opcode */
            Fast486Exception(State, FAST486_EXCEPTION_UD);
        }

        if (Fast486OpcodeHandlers[Opcode] != Fast486OpcodePrefix)
        {
            /* A non-prefix opcode has been executed, reset the prefix flags */
            State->PrefixFlags = 0;
        }
        else
        {
            /* This is a prefix, go to the next instruction immediately */
            continue;
        }
    }
    while ((Command == FAST486_CONTINUE)
           || (Command == FAST486_STEP_OVER && ProcedureCallCount > 0)
           || (Command == FAST486_STEP_OUT && ProcedureCallCount >= 0)
           || (Fast486OpcodeHandlers[Opcode] == Fast486OpcodePrefix));
}

/* DEFAULT CALLBACKS **********************************************************/

static VOID
NTAPI
Fast486MemReadCallback(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
{
    UNREFERENCED_PARAMETER(State);

    RtlMoveMemory(Buffer, (PVOID)Address, Size);
}

static VOID
NTAPI
Fast486MemWriteCallback(PFAST486_STATE State, ULONG Address, PVOID Buffer, ULONG Size)
{
    UNREFERENCED_PARAMETER(State);

    RtlMoveMemory((PVOID)Address, Buffer, Size);
}

static VOID
NTAPI
Fast486IoReadCallback(PFAST486_STATE State, ULONG Port, PVOID Buffer, ULONG Size, UCHAR Width)
{
    UNREFERENCED_PARAMETER(State);
    UNREFERENCED_PARAMETER(Port);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Width);
}

static VOID
NTAPI
Fast486IoWriteCallback(PFAST486_STATE State, ULONG Port, PVOID Buffer, ULONG Size, UCHAR Width)
{
    UNREFERENCED_PARAMETER(State);
    UNREFERENCED_PARAMETER(Port);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Width);
}

static VOID
NTAPI
Fast486IdleCallback(PFAST486_STATE State)
{
    UNREFERENCED_PARAMETER(State);
}

static VOID
NTAPI
Fast486BopCallback(PFAST486_STATE State, UCHAR BopCode)
{
    UNREFERENCED_PARAMETER(State);
    UNREFERENCED_PARAMETER(BopCode);
}

static UCHAR
NTAPI
Fast486IntAckCallback(PFAST486_STATE State)
{
    UNREFERENCED_PARAMETER(State);

    /* Return something... */
    return 0;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
Fast486Initialize(PFAST486_STATE         State,
                  FAST486_MEM_READ_PROC  MemReadCallback,
                  FAST486_MEM_WRITE_PROC MemWriteCallback,
                  FAST486_IO_READ_PROC   IoReadCallback,
                  FAST486_IO_WRITE_PROC  IoWriteCallback,
                  FAST486_IDLE_PROC      IdleCallback,
                  FAST486_BOP_PROC       BopCallback,
                  FAST486_INT_ACK_PROC   IntAckCallback)
{
    /* Set the callbacks (or use default ones if some are NULL) */
    State->MemReadCallback  = (MemReadCallback  ? MemReadCallback  : Fast486MemReadCallback );
    State->MemWriteCallback = (MemWriteCallback ? MemWriteCallback : Fast486MemWriteCallback);
    State->IoReadCallback   = (IoReadCallback   ? IoReadCallback   : Fast486IoReadCallback  );
    State->IoWriteCallback  = (IoWriteCallback  ? IoWriteCallback  : Fast486IoWriteCallback );
    State->IdleCallback     = (IdleCallback     ? IdleCallback     : Fast486IdleCallback    );
    State->BopCallback      = (BopCallback      ? BopCallback      : Fast486BopCallback     );
    State->IntAckCallback   = (IntAckCallback   ? IntAckCallback   : Fast486IntAckCallback  );

    /* Reset the CPU */
    Fast486Reset(State);
}

VOID
NTAPI
Fast486Reset(PFAST486_STATE State)
{
    FAST486_SEG_REGS i;

    FAST486_MEM_READ_PROC  MemReadCallback  = State->MemReadCallback;
    FAST486_MEM_WRITE_PROC MemWriteCallback = State->MemWriteCallback;
    FAST486_IO_READ_PROC   IoReadCallback   = State->IoReadCallback;
    FAST486_IO_WRITE_PROC  IoWriteCallback  = State->IoWriteCallback;
    FAST486_IDLE_PROC      IdleCallback     = State->IdleCallback;
    FAST486_BOP_PROC       BopCallback      = State->BopCallback;
    FAST486_INT_ACK_PROC   IntAckCallback   = State->IntAckCallback;

    /* Clear the entire structure */
    RtlZeroMemory(State, sizeof(*State));

    /* Initialize the registers */
    State->Flags.AlwaysSet = 1;
    State->InstPtr.LowWord = 0xFFF0;

    /* Initialize segments */
    for (i = 0; i < FAST486_NUM_SEG_REGS; i++)
    {
        State->SegmentRegs[i].Selector = 0;
        State->SegmentRegs[i].Base = 0;
        State->SegmentRegs[i].Limit = 0xFFFF;
        State->SegmentRegs[i].Present = TRUE;
        State->SegmentRegs[i].ReadWrite = TRUE;
        State->SegmentRegs[i].Executable = FALSE;
        State->SegmentRegs[i].DirConf = FALSE;
        State->SegmentRegs[i].SystemType = 1; // Segment descriptor
        State->SegmentRegs[i].Dpl = 0;
        State->SegmentRegs[i].Size = FALSE; // 16-bit
    }

    /* Initialize the code segment */
    State->SegmentRegs[FAST486_REG_CS].Executable = TRUE;
    State->SegmentRegs[FAST486_REG_CS].Selector = 0xF000;
    State->SegmentRegs[FAST486_REG_CS].Base = 0xFFFF0000;

    /* Initialize the IDT */
    State->Idtr.Size = 0x3FF;
    State->Idtr.Address = 0;

    /* Initialize CR0 */
    State->ControlRegisters[FAST486_REG_CR0] |= FAST486_CR0_ET;

    /* Restore the callbacks */
    State->MemReadCallback  = MemReadCallback;
    State->MemWriteCallback = MemWriteCallback;
    State->IoReadCallback   = IoReadCallback;
    State->IoWriteCallback  = IoWriteCallback;
    State->IdleCallback     = IdleCallback;
    State->BopCallback      = BopCallback;
    State->IntAckCallback   = IntAckCallback;
}

VOID
NTAPI
Fast486DumpState(PFAST486_STATE State)
{
    DPRINT1("\nCPU currently executing in %s mode at %04X:%08X\n",
            (State->ControlRegisters[0] & FAST486_CR0_PE) ? "protected" : "real",
            State->SegmentRegs[FAST486_REG_CS].Selector,
            State->InstPtr.Long);
    DPRINT1("\nGeneral purpose registers:\n"
            "EAX = %08X\tECX = %08X\tEDX = %08X\tEBX = %08X\n"
            "ESP = %08X\tEBP = %08X\tESI = %08X\tEDI = %08X\n",
            State->GeneralRegs[FAST486_REG_EAX].Long,
            State->GeneralRegs[FAST486_REG_ECX].Long,
            State->GeneralRegs[FAST486_REG_EDX].Long,
            State->GeneralRegs[FAST486_REG_EBX].Long,
            State->GeneralRegs[FAST486_REG_ESP].Long,
            State->GeneralRegs[FAST486_REG_EBP].Long,
            State->GeneralRegs[FAST486_REG_ESI].Long,
            State->GeneralRegs[FAST486_REG_EDI].Long);
    DPRINT1("\nSegment registers:\n"
            "ES = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "CS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "SS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "DS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "FS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n"
            "GS = %04X (Base: %08X, Limit: %08X, Dpl: %u)\n",
            State->SegmentRegs[FAST486_REG_ES].Selector,
            State->SegmentRegs[FAST486_REG_ES].Base,
            State->SegmentRegs[FAST486_REG_ES].Limit,
            State->SegmentRegs[FAST486_REG_ES].Dpl,
            State->SegmentRegs[FAST486_REG_CS].Selector,
            State->SegmentRegs[FAST486_REG_CS].Base,
            State->SegmentRegs[FAST486_REG_CS].Limit,
            State->SegmentRegs[FAST486_REG_CS].Dpl,
            State->SegmentRegs[FAST486_REG_SS].Selector,
            State->SegmentRegs[FAST486_REG_SS].Base,
            State->SegmentRegs[FAST486_REG_SS].Limit,
            State->SegmentRegs[FAST486_REG_SS].Dpl,
            State->SegmentRegs[FAST486_REG_DS].Selector,
            State->SegmentRegs[FAST486_REG_DS].Base,
            State->SegmentRegs[FAST486_REG_DS].Limit,
            State->SegmentRegs[FAST486_REG_DS].Dpl,
            State->SegmentRegs[FAST486_REG_FS].Selector,
            State->SegmentRegs[FAST486_REG_FS].Base,
            State->SegmentRegs[FAST486_REG_FS].Limit,
            State->SegmentRegs[FAST486_REG_FS].Dpl,
            State->SegmentRegs[FAST486_REG_GS].Selector,
            State->SegmentRegs[FAST486_REG_GS].Base,
            State->SegmentRegs[FAST486_REG_GS].Limit,
            State->SegmentRegs[FAST486_REG_GS].Dpl);
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
            "CR0 = %08X\tCR2 = %08X\tCR3 = %08X\n",
            State->ControlRegisters[FAST486_REG_CR0],
            State->ControlRegisters[FAST486_REG_CR2],
            State->ControlRegisters[FAST486_REG_CR3]);
    DPRINT1("\nDebug Registers:\n"
            "DR0 = %08X\tDR1 = %08X\tDR2 = %08X\n"
            "DR3 = %08X\tDR4 = %08X\tDR5 = %08X\n",
            State->DebugRegisters[FAST486_REG_DR0],
            State->DebugRegisters[FAST486_REG_DR1],
            State->DebugRegisters[FAST486_REG_DR2],
            State->DebugRegisters[FAST486_REG_DR3],
            State->DebugRegisters[FAST486_REG_DR4],
            State->DebugRegisters[FAST486_REG_DR5]);
}

VOID
NTAPI
Fast486Continue(PFAST486_STATE State)
{
    /* Call the internal function */
    Fast486ExecutionControl(State, FAST486_CONTINUE);
}

VOID
NTAPI
Fast486StepInto(PFAST486_STATE State)
{
    /* Call the internal function */
    Fast486ExecutionControl(State, FAST486_STEP_INTO);
}

VOID
NTAPI
Fast486StepOver(PFAST486_STATE State)
{
    /* Call the internal function */
    Fast486ExecutionControl(State, FAST486_STEP_OVER);
}

VOID
NTAPI
Fast486StepOut(PFAST486_STATE State)
{
    /* Call the internal function */
    Fast486ExecutionControl(State, FAST486_STEP_OUT);
}

VOID
NTAPI
Fast486Interrupt(PFAST486_STATE State, UCHAR Number)
{
    /* Set the interrupt status and the number */
    State->IntStatus = FAST486_INT_EXECUTE;
    State->PendingIntNum = Number;
}

VOID
NTAPI
Fast486InterruptSignal(PFAST486_STATE State)
{
    /* Set the interrupt status */
    State->IntStatus = FAST486_INT_SIGNAL;
}

VOID
NTAPI
Fast486ExecuteAt(PFAST486_STATE State, USHORT Segment, ULONG Offset)
{
    /* Load the new CS */
    if (!Fast486LoadSegment(State, FAST486_REG_CS, Segment))
    {
        /* An exception occurred, let the handler execute instead */
        return;
    }

    /* Set the new IP */
    State->InstPtr.Long = Offset;
}

VOID
NTAPI
Fast486SetStack(PFAST486_STATE State, USHORT Segment, ULONG Offset)
{
    /* Load the new SS */
    if (!Fast486LoadSegment(State, FAST486_REG_SS, Segment))
    {
        /* An exception occurred, let the handler execute instead */
        return;
    }

    /* Set the new SP */
    State->GeneralRegs[FAST486_REG_ESP].Long = Offset;
}

VOID
NTAPI
Fast486SetSegment(PFAST486_STATE State,
                  FAST486_SEG_REGS Segment,
                  USHORT Selector)
{
    /* Call the internal function */
    Fast486LoadSegment(State, Segment, Selector);
}

/* EOF */
