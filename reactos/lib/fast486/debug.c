/*
 * Fast486 386/486 CPU Emulation Library
 * fast486dbg.c
 *
 * Copyright (C) 2015 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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
#include "fpu.h"

/* DEFINES ********************************************************************/

typedef enum
{
    FAST486_STEP_INTO,
    FAST486_STEP_OVER,
    FAST486_STEP_OUT,
    FAST486_CONTINUE
} FAST486_EXEC_CMD;

/* PRIVATE FUNCTIONS **********************************************************/

FORCEINLINE
VOID
FASTCALL
Fast486ExecutionControl(PFAST486_STATE State, FAST486_EXEC_CMD Command)
{
    UCHAR Opcode;
    FAST486_OPCODE_HANDLER_PROC CurrentHandler;
    INT ProcedureCallCount = 0;
    BOOLEAN Trap;

    /* Main execution loop */
    do
    {
        Trap = State->Flags.Tf;

        if (!State->Halted)
        {
NextInst:
            /* Check if this is a new instruction */
            if (State->PrefixFlags == 0)
            {
                State->SavedInstPtr = State->InstPtr;
                State->SavedStackPtr = State->GeneralRegs[FAST486_REG_ESP];
            }

            /* Perform an instruction fetch */
            if (!Fast486FetchByte(State, &Opcode))
            {
                /* Exception occurred */
                State->PrefixFlags = 0;
                continue;
            }

            // TODO: Check for CALL/RET to update ProcedureCallCount.

            /* Call the opcode handler */
            CurrentHandler = Fast486OpcodeHandlers[Opcode];
            CurrentHandler(State, Opcode);

            /* If this is a prefix, go to the next instruction immediately */
            if (CurrentHandler == Fast486OpcodePrefix) goto NextInst;

            /* A non-prefix opcode has been executed, reset the prefix flags */
            State->PrefixFlags = 0;
        }

        /*
         * Check if there is an interrupt to execute, or a hardware interrupt signal
         * while interrupts are enabled.
         */
        if (State->DoNotInterrupt)
        {
            /* Clear the interrupt delay flag */
            State->DoNotInterrupt = FALSE;
        }
        else if (Trap && !State->Halted)
        {
            /* Perform the interrupt */
            Fast486PerformInterrupt(State, FAST486_EXCEPTION_DB);
        }
        else if (State->Flags.If && State->IntSignaled)
        {
            /* No longer halted */
            State->Halted = FALSE;

            /* Acknowledge the interrupt and perform it */
            Fast486PerformInterrupt(State, State->IntAckCallback(State));

            /* Clear the interrupt status */
            State->IntSignaled = FALSE;
        }
    }
    while ((Command == FAST486_CONTINUE) ||
           (Command == FAST486_STEP_OVER && ProcedureCallCount > 0) ||
           (Command == FAST486_STEP_OUT && ProcedureCallCount >= 0));
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
Fast486DumpState(PFAST486_STATE State)
{
    DbgPrint("\nFast486DumpState -->\n");
    DbgPrint("\nCPU currently executing in %s mode at %04X:%08X\n",
            (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE) ? "protected" : "real",
             State->SegmentRegs[FAST486_REG_CS].Selector,
             State->InstPtr.Long);
    DbgPrint("\nGeneral purpose registers:\n"
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
    DbgPrint("\nSegment registers:\n"
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
    DbgPrint("\nFlags: %08X (%s %s %s %s %s %s %s %s %s %s %s %s %s) Iopl: %u\n",
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
             State->Flags.Iopl);
    DbgPrint("\nControl Registers:\n"
             "CR0 = %08X\tCR2 = %08X\tCR3 = %08X\n",
             State->ControlRegisters[FAST486_REG_CR0],
             State->ControlRegisters[FAST486_REG_CR2],
             State->ControlRegisters[FAST486_REG_CR3]);
    DbgPrint("\nDebug Registers:\n"
             "DR0 = %08X\tDR1 = %08X\tDR2 = %08X\n"
             "DR3 = %08X\tDR4 = %08X\tDR5 = %08X\n",
             State->DebugRegisters[FAST486_REG_DR0],
             State->DebugRegisters[FAST486_REG_DR1],
             State->DebugRegisters[FAST486_REG_DR2],
             State->DebugRegisters[FAST486_REG_DR3],
             State->DebugRegisters[FAST486_REG_DR4],
             State->DebugRegisters[FAST486_REG_DR5]);

#ifndef FAST486_NO_FPU
    DbgPrint("\nFPU Registers:\n"
             "ST0 = %04X%016llX\tST1 = %04X%016llX\n"
             "ST2 = %04X%016llX\tST3 = %04X%016llX\n"
             "ST4 = %04X%016llX\tST5 = %04X%016llX\n"
             "ST6 = %04X%016llX\tST7 = %04X%016llX\n"
             "Status: %04X\tControl: %04X\tTag: %04X\n",
             FPU_ST(0).Exponent | ((USHORT)FPU_ST(0).Sign << 15),
             FPU_ST(0).Mantissa,
             FPU_ST(1).Exponent | ((USHORT)FPU_ST(1).Sign << 15),
             FPU_ST(1).Mantissa,
             FPU_ST(2).Exponent | ((USHORT)FPU_ST(2).Sign << 15),
             FPU_ST(2).Mantissa,
             FPU_ST(3).Exponent | ((USHORT)FPU_ST(3).Sign << 15),
             FPU_ST(3).Mantissa,
             FPU_ST(4).Exponent | ((USHORT)FPU_ST(4).Sign << 15),
             FPU_ST(4).Mantissa,
             FPU_ST(5).Exponent | ((USHORT)FPU_ST(5).Sign << 15),
             FPU_ST(5).Mantissa,
             FPU_ST(6).Exponent | ((USHORT)FPU_ST(6).Sign << 15),
             FPU_ST(6).Mantissa,
             FPU_ST(7).Exponent | ((USHORT)FPU_ST(7).Sign << 15),
             FPU_ST(7).Mantissa,
             State->FpuStatus,
             State->FpuControl,
             State->FpuTag);
#endif

    DbgPrint("\n<-- Fast486DumpState\n\n");
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

/* EOF */
