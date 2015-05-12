/*
 * Fast486 386/486 CPU Emulation Library
 * extraops.c
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
#include "opcodes.h"
#include "common.h"
#include "opgroups.h"
#include "extraops.h"

/* PUBLIC VARIABLES ***********************************************************/

FAST486_OPCODE_HANDLER_PROC
Fast486ExtendedHandlers[FAST486_NUM_OPCODE_HANDLERS] =
{
    Fast486ExtOpcodeGroup0F00,          /* 0x00 - 0x01 */
    Fast486ExtOpcodeGroup0F01,
    Fast486ExtOpcodeLar,                /* 0x02 */
    Fast486ExtOpcodeLsl,                /* 0x03 */
    Fast486ExtOpcodeInvalid,            /* 0x04 - 0x05 */   // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeClts,               /* 0x06 */
    Fast486ExtOpcodeInvalid,            /* 0x07 */          // Invalid
    Fast486ExtOpcodeUnimplemented, // TODO: OPCODE 0x08 NOT IMPLEMENTED
    Fast486ExtOpcodeUnimplemented, // TODO: OPCODE 0x09 NOT IMPLEMENTED
    Fast486ExtOpcodeInvalid,            /* 0x0A */          // Invalid
    Fast486ExtOpcode0F0B,               /* 0x0B */          // Reserved (UD2)
    Fast486ExtOpcodeInvalid,            /* 0x0C - 0x1F */   // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeStoreControlReg,    /* 0x20 */
    Fast486ExtOpcodeStoreDebugReg,      /* 0x21 */
    Fast486ExtOpcodeLoadControlReg,     /* 0x22 */
    Fast486ExtOpcodeLoadDebugReg,       /* 0x23 */
    Fast486ExtOpcodeUnimplemented, // TODO: OPCODE 0x24 NOT IMPLEMENTED
    Fast486ExtOpcodeInvalid,            /* 0x25 */          // Invalid
    Fast486ExtOpcodeUnimplemented, // TODO: OPCODE 0x26 NOT IMPLEMENTED
    Fast486ExtOpcodeInvalid,            /* 0x27 - 0x7F */   // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeConditionalJmp,     /* 0x80 - 0x8F */
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalJmp,
    Fast486ExtOpcodeConditionalSet,     /* 0x90 - 0x9F */
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodeConditionalSet,
    Fast486ExtOpcodePushFs,             /* 0xA0 */
    Fast486ExtOpcodePopFs,              /* 0xA1 */
    Fast486ExtOpcodeInvalid,            /* 0xA2 */          // Invalid
    Fast486ExtOpcodeBitTest,            /* 0xA3 */
    Fast486ExtOpcodeShld,               /* 0xA4 - 0xA5 */
    Fast486ExtOpcodeShld,
    Fast486ExtOpcodeInvalid,            /* 0xA6 - 0xA7 */   // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodePushGs,             /* 0xA8 - 0xA9 */
    Fast486ExtOpcodePopGs,
    Fast486ExtOpcodeInvalid,            /* 0xAA */          // Invalid
    Fast486ExtOpcodeBts,                /* 0xAB */
    Fast486ExtOpcodeShrd,               /* 0xAC - 0xAD */
    Fast486ExtOpcodeShrd,
    Fast486ExtOpcodeInvalid,            /* 0xAE */          // Invalid
    Fast486ExtOpcodeImul,               /* 0xAF */
    Fast486ExtOpcodeCmpXchgByte,        /* 0xB0 */
    Fast486ExtOpcodeCmpXchg,            /* 0xB1 */
    Fast486ExtOpcodeLss,                /* 0xB2 */
    Fast486ExtOpcodeBtr,                /* 0xB3 */
    Fast486ExtOpcodeLfsLgs,             /* 0xB4 - 0xB5 */
    Fast486ExtOpcodeLfsLgs,
    Fast486ExtOpcodeMovzxByte,          /* 0xB6 - 0xB7 */
    Fast486ExtOpcodeMovzxWord,
    Fast486ExtOpcodeInvalid,            /* 0xB8 */          // Invalid
    Fast486ExtOpcodeGroup0FB9,          /* 0xB9 */
    Fast486ExtOpcodeGroup0FBA,          /* 0xBA */
    Fast486ExtOpcodeBtc,                /* 0xBB */
    Fast486ExtOpcodeBsf,                /* 0xBC */
    Fast486ExtOpcodeBsr,                /* 0xBD */
    Fast486ExtOpcodeMovsxByte,          /* 0xBE - 0xBF */
    Fast486ExtOpcodeMovsxWord,
    Fast486ExtOpcodeXaddByte,           /* 0xC0 - 0xC1 */
    Fast486ExtOpcodeXadd,
    Fast486ExtOpcodeInvalid,            /* 0xC2 - 0xC7 */   // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeBswap,              /* 0xC8 - 0xCF */
    Fast486ExtOpcodeBswap,
    Fast486ExtOpcodeBswap,
    Fast486ExtOpcodeBswap,
    Fast486ExtOpcodeBswap,
    Fast486ExtOpcodeBswap,
    Fast486ExtOpcodeBswap,
    Fast486ExtOpcodeBswap,
    Fast486ExtOpcodeInvalid,            /* 0xD0 - 0xFF */   // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
    Fast486ExtOpcodeInvalid,                                // Invalid
};

/* PUBLIC FUNCTIONS ***********************************************************/

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeInvalid)
{
    DPRINT1("FAST486 -- Extended opcode 0x%02X is INVALID!\n", Opcode);
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return;
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeUnimplemented)
{
    DPRINT1("FAST486 -- Extended opcode 0x%02X is UNIMPLEMENTED\n", Opcode);
    // Fast486Exception(State, FAST486_EXCEPTION_UD);
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcode0F0B)
{
    /* Reserved opcode (UD2) */
    Fast486Exception(State, FAST486_EXCEPTION_UD);
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLar)
{
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN Valid;
    USHORT Selector;
    FAST486_GDT_ENTRY GdtEntry;
    DWORD AccessRights;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    if (!(State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
        || State->Flags.Vm)
    {
        /* Not recognized */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    NO_LOCK_PREFIX();
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Value;

        /* Read the value */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        Selector = LOWORD(Value);
    }
    else
    {
        /* Read the value */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Selector))
        {
            /* Exception occurred */
            return;
        }
    }

    if (!Fast486ReadDescriptorEntry(State, Selector, &Valid, &GdtEntry))
    {
        /* Exception occurred */
        return;
    }

    if (!Valid)
    {
        State->Flags.Zf = FALSE;
        return;
    }

    /* Privilege check */
    if (((GET_SEGMENT_RPL(Selector) > GdtEntry.Dpl))
        || (Fast486GetCurrentPrivLevel(State) > GdtEntry.Dpl))
    {
        State->Flags.Zf = FALSE;
        return;
    }

    /* Set ZF */
    State->Flags.Zf = TRUE;

    /* Get the access rights */
    AccessRights = ((PDWORD)&GdtEntry)[1] & 0x00F0FF00;

    /* Return the access rights */
    if (OperandSize) Fast486WriteModrmDwordOperands(State, &ModRegRm, TRUE, AccessRights);
    else Fast486WriteModrmWordOperands(State, &ModRegRm, TRUE, LOWORD(AccessRights));
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLsl)
{
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN Valid;
    USHORT Selector;
    ULONG Limit;
    FAST486_GDT_ENTRY GdtEntry;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    if (!(State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
        || State->Flags.Vm)
    {
        /* Not recognized */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    NO_LOCK_PREFIX();
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Value;

        /* Read the value */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        Selector = LOWORD(Value);
    }
    else
    {
        /* Read the value */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Selector))
        {
            /* Exception occurred */
            return;
        }
    }

    if (!Fast486ReadDescriptorEntry(State, Selector, &Valid, &GdtEntry))
    {
        /* Exception occurred */
        return;
    }

    if (!Valid)
    {
        State->Flags.Zf = FALSE;
        return;
    }

    /* Privilege check */
    if (((GET_SEGMENT_RPL(Selector) > GdtEntry.Dpl))
        || (Fast486GetCurrentPrivLevel(State) > GdtEntry.Dpl))
    {
        State->Flags.Zf = FALSE;
        return;
    }

    /* Calculate the limit */
    Limit = GdtEntry.Limit | (GdtEntry.LimitHigh << 16);

    if (GdtEntry.Granularity)
    {
        Limit <<= 12;
        Limit |= 0x00000FFF;
    }

    /* Set ZF */
    State->Flags.Zf = TRUE;

    /* Return the limit */
    if (OperandSize) Fast486WriteModrmDwordOperands(State, &ModRegRm, TRUE, Limit);
    else Fast486WriteModrmWordOperands(State, &ModRegRm, TRUE, LOWORD(Limit));
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeClts)
{
    NO_LOCK_PREFIX();

    /* The current privilege level must be zero */
    if (Fast486GetCurrentPrivLevel(State) != 0)
    {
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return;
    }

    /* Clear the task switch bit */
    State->ControlRegisters[FAST486_REG_CR0] &= ~FAST486_CR0_TS;
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeStoreControlReg)
{
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    NO_LOCK_PREFIX();
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* The current privilege level must be zero */
    if (Fast486GetCurrentPrivLevel(State) != 0)
    {
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return;
    }

    if ((ModRegRm.Register == 1) || (ModRegRm.Register > 3))
    {
        /* CR1, CR4, CR5, CR6 and CR7 don't exist */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    if (ModRegRm.Register != 0)
    {
        /* CR2 and CR3 and are stored in array indexes 1 and 2 */
        ModRegRm.Register--;
    }

    /* Store the value of the control register */
    State->GeneralRegs[ModRegRm.SecondRegister].Long = State->ControlRegisters[ModRegRm.Register];
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeStoreDebugReg)
{
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    NO_LOCK_PREFIX();
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* The current privilege level must be zero */
    if (Fast486GetCurrentPrivLevel(State) != 0)
    {
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return;
    }

    if ((ModRegRm.Register == 6) || (ModRegRm.Register == 7))
    {
        /* DR6 and DR7 are aliases to DR4 and DR5 */
        ModRegRm.Register -= 2;
    }

    if (State->DebugRegisters[FAST486_REG_DR5] & FAST486_DR5_GD)
    {
        /* Disallow access to debug registers */
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return;
    }

    /* Store the value of the debug register */
    State->GeneralRegs[ModRegRm.SecondRegister].Long = State->DebugRegisters[ModRegRm.Register];
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLoadControlReg)
{
    ULONG Value;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    NO_LOCK_PREFIX();
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* The current privilege level must be zero */
    if (Fast486GetCurrentPrivLevel(State) != 0)
    {
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return;
    }

    if ((ModRegRm.Register == 1) || (ModRegRm.Register > 3))
    {
        /* CR1, CR4, CR5, CR6 and CR7 don't exist */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    if (ModRegRm.Register != 0)
    {
        /* CR2 and CR3 and are stored in array indexes 1 and 2 */
        ModRegRm.Register--;
    }

    /* Get the value */
    Value = State->GeneralRegs[ModRegRm.SecondRegister].Long;

    if (ModRegRm.Register == (INT)FAST486_REG_CR0)
    {
        /* CR0 checks */

        if (((Value & (FAST486_CR0_PG | FAST486_CR0_PE)) == FAST486_CR0_PG)
            || ((Value & (FAST486_CR0_CD | FAST486_CR0_NW)) == FAST486_CR0_NW))
        {
            /* Invalid value */
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return;
        }
    }

#ifndef FAST486_NO_PREFETCH
    /* Changing CR0 or CR3 can interfere with prefetching (because of paging) */
    State->PrefetchValid = FALSE;
#endif

    if (State->Tlb && (ModRegRm.Register == (INT)FAST486_REG_CR3))
    {
        /* Flush the TLB */
        RtlFillMemory(State->Tlb, NUM_TLB_ENTRIES * sizeof(ULONG), 0xFF);
    }

    /* Load a value to the control register */
    State->ControlRegisters[ModRegRm.Register] = Value;
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLoadDebugReg)
{
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    NO_LOCK_PREFIX();
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* The current privilege level must be zero */
    if (Fast486GetCurrentPrivLevel(State) != 0)
    {
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return;
    }

    if ((ModRegRm.Register == 6) || (ModRegRm.Register == 7))
    {
        /* DR6 and DR7 are aliases to DR4 and DR5 */
        ModRegRm.Register -= 2;
    }

    if (State->DebugRegisters[FAST486_REG_DR5] & FAST486_DR5_GD)
    {
        /* Disallow access to debug registers */
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return;
    }

    /* Load a value to the debug register */
    State->DebugRegisters[ModRegRm.Register] = State->GeneralRegs[ModRegRm.SecondRegister].Long;

    if (ModRegRm.Register == (INT)FAST486_REG_DR4)
    {
        /* The reserved bits are 1 */
        State->DebugRegisters[ModRegRm.Register] |= FAST486_DR4_RESERVED;
    }
    else if (ModRegRm.Register == (INT)FAST486_REG_DR5)
    {
        /* The reserved bits are 0 */
        State->DebugRegisters[ModRegRm.Register] &= ~FAST486_DR5_RESERVED;
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodePushFs)
{
    /* Call the internal API */
    Fast486StackPush(State, State->SegmentRegs[FAST486_REG_FS].Selector);
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodePopFs)
{
    ULONG NewSelector;

    if (!Fast486StackPop(State, &NewSelector))
    {
        /* Exception occurred */
        return;
    }

    /* Call the internal API */
    Fast486LoadSegment(State, FAST486_REG_FS, LOWORD(NewSelector));
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBitTest)
{
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    UINT DataSize;
    ULONG BitNumber;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the number of bits */
    if (OperandSize) DataSize = 32;
    else DataSize = 16;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Get the bit number */
    BitNumber = OperandSize ? State->GeneralRegs[ModRegRm.Register].Long
                            : (ULONG)State->GeneralRegs[ModRegRm.Register].LowWord;

    if (ModRegRm.Memory)
    {
        /*
         * For memory operands, add the bit offset divided by
         * the data size to the address
         */
        ModRegRm.MemoryAddress += (BitNumber / DataSize) * (DataSize / 8);
    }

    /* Normalize the bit number */
    BitNumber %= DataSize;

    if (OperandSize)
    {
        ULONG Value;

        /* Read the value */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Set CF to the bit value */
        State->Flags.Cf = (Value >> BitNumber) & 1;
    }
    else
    {
        USHORT Value;

        /* Read the value */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Set CF to the bit value */
        State->Flags.Cf = (Value >> BitNumber) & 1;
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeShld)
{
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    UCHAR Count;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFE) == 0xA4);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (Opcode == 0xA4)
    {
        /* Fetch the count */
        if (!Fast486FetchByte(State, &Count))
        {
            /* Exception occurred */
            return;
        }
    }
    else
    {
        /* The count is in CL */
        Count = State->GeneralRegs[FAST486_REG_ECX].LowByte;
    }

    /* Normalize the count */
    Count &= 0x1F;

    /* Do nothing if the count is zero */
    if (Count == 0) return;

    if (OperandSize)
    {
        ULONG Source, Destination, Result;

        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, &Source, &Destination))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Result = (Destination << Count) | (Source >> (32 - Count));

        /* Update flags */
        State->Flags.Cf = (Destination >> (32 - Count)) & 1;
        if (Count == 1) State->Flags.Of = (Result & SIGN_FLAG_LONG)
                                          != (Destination & SIGN_FLAG_LONG);
        State->Flags.Zf = (Result == 0);
        State->Flags.Sf = ((Result & SIGN_FLAG_LONG) != 0);
        State->Flags.Pf = Fast486CalculateParity(Result);

        /* Write back the result */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Result);
    }
    else
    {
        USHORT Source, Destination, Result;
        ULONG DoubleSource;

        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, &Source, &Destination))
        {
            /* Exception occurred */
            return;
        }

        DoubleSource = Source | (Source << 16);

        /* Calculate the result */
        Result = (Destination << Count) | (DoubleSource >> (32 - Count));

        /* Update flags */
        if (Count <= 16) State->Flags.Cf = (Destination >> (16 - Count)) & 1;
        else  State->Flags.Cf = (Source >> (32 - Count)) & 1;

        if (Count == 1) State->Flags.Of = (Result & SIGN_FLAG_WORD)
                                          != (Destination & SIGN_FLAG_WORD);
        State->Flags.Zf = (Result == 0);
        State->Flags.Sf = ((Result & SIGN_FLAG_WORD) != 0);
        State->Flags.Pf = Fast486CalculateParity(Result);

        /* Write back the result */
        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Result);
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodePushGs)
{
    /* Call the internal API */
    Fast486StackPush(State, State->SegmentRegs[FAST486_REG_GS].Selector);
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodePopGs)
{
    ULONG NewSelector;

    if (!Fast486StackPop(State, &NewSelector))
    {
        /* Exception occurred */
        return;
    }

    /* Call the internal API */
    Fast486LoadSegment(State, FAST486_REG_GS, LOWORD(NewSelector));
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBts)
{
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    UINT DataSize;
    ULONG BitNumber;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the number of bits */
    if (OperandSize) DataSize = 32;
    else DataSize = 16;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Get the bit number */
    BitNumber = OperandSize ? State->GeneralRegs[ModRegRm.Register].Long
                            : (ULONG)State->GeneralRegs[ModRegRm.Register].LowWord;

    if (ModRegRm.Memory)
    {
        /*
         * For memory operands, add the bit offset divided by
         * the data size to the address
         */
        ModRegRm.MemoryAddress += (BitNumber / DataSize) * (DataSize / 8);
    }

    /* Normalize the bit number */
    BitNumber %= DataSize;

    if (OperandSize)
    {
        ULONG Value;

        /* Read the value */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Set CF to the bit value */
        State->Flags.Cf = (Value >> BitNumber) & 1;

        /* Set the bit */
        Value |= 1 << BitNumber;

        /* Write back the result */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    }
    else
    {
        USHORT Value;

        /* Read the value */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Set CF to the bit value */
        State->Flags.Cf = (Value >> BitNumber) & 1;

        /* Set the bit */
        Value |= 1 << BitNumber;

        /* Write back the result */
        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeShrd)
{
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    UCHAR Count;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFE) == 0xAC);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (Opcode == 0xAC)
    {
        /* Fetch the count */
        if (!Fast486FetchByte(State, &Count))
        {
            /* Exception occurred */
            return;
        }
    }
    else
    {
        /* The count is in CL */
        Count = State->GeneralRegs[FAST486_REG_ECX].LowByte;
    }

    /* Normalize the count */
    Count &= 0x1F;

    /* Do nothing if the count is zero */
    if (Count == 0) return;

    if (OperandSize)
    {
        ULONG Source, Destination, Result;

        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, &Source, &Destination))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Result = (Destination >> Count) | (Source << (32 - Count));

        /* Update flags */
        State->Flags.Cf = (Destination >> (Count - 1)) & 1;
        if (Count == 1) State->Flags.Of = (Result & SIGN_FLAG_LONG)
                                          != (Destination & SIGN_FLAG_LONG);
        State->Flags.Zf = (Result == 0);
        State->Flags.Sf = ((Result & SIGN_FLAG_LONG) != 0);
        State->Flags.Pf = Fast486CalculateParity(Result);

        /* Write back the result */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Result);
    }
    else
    {
        USHORT Source, Destination, Result;

        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, &Source, &Destination))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Result = (Destination >> Count) | (Source << (16 - Count));

        if (Count >= 16) Result |= (ULONG)(Source | (Source << 16)) >> (Count - 16);

        /* Update flags */
        if (Count <= 16) State->Flags.Cf = (Destination >> (Count - 1)) & 1;
        else State->Flags.Cf = (Source >> (Count - 17)) & 1;

        if (Count == 1) State->Flags.Of = (Result & SIGN_FLAG_WORD)
                                          != (Destination & SIGN_FLAG_WORD);
        State->Flags.Zf = (Result == 0);
        State->Flags.Sf = ((Result & SIGN_FLAG_WORD) != 0);
        State->Flags.Pf = Fast486CalculateParity(Result);

        /* Write back the result */
        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Result);
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeImul)
{
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        LONG Source, Destination;
        LONGLONG Result;

        /* Read the operands */
        if (!Fast486ReadModrmDwordOperands(State,
                                           &ModRegRm,
                                           (PULONG)&Destination,
                                           (PULONG)&Source))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Result = (LONGLONG)Source * (LONGLONG)Destination;

        /* Update the flags */
        State->Flags.Cf = State->Flags.Of = ((Result < -2147483648LL) || (Result > 2147483647LL));

        /* Write back the result */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, TRUE, (ULONG)((LONG)Result));
    }
    else
    {
        SHORT Source, Destination;
        LONG Result;

        /* Read the operands */
        if (!Fast486ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          (PUSHORT)&Destination,
                                          (PUSHORT)&Source))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Result = (LONG)Source * (LONG)Destination;

        /* Update the flags */
        State->Flags.Cf = State->Flags.Of = ((Result < -32768) || (Result > 32767));

        /* Write back the result */
        Fast486WriteModrmWordOperands(State, &ModRegRm, TRUE, (USHORT)((SHORT)Result));
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeCmpXchgByte)
{
    FAST486_MOD_REG_RM ModRegRm;
    UCHAR Accumulator = State->GeneralRegs[FAST486_REG_EAX].LowByte;
    UCHAR Source, Destination, Result;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Read the operands */
    if (!Fast486ReadModrmByteOperands(State, &ModRegRm, &Source, &Destination))
    {
        /* Exception occurred */
        return;
    }

    /* Compare AL with the destination */
    Result = Accumulator - Destination;

    /* Update the flags */
    State->Flags.Cf = (Accumulator < Destination);
    State->Flags.Of = ((Accumulator & SIGN_FLAG_BYTE) != (Destination & SIGN_FLAG_BYTE))
                      && ((Accumulator & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = (Accumulator & 0x0F) < (Destination & 0x0F);
    State->Flags.Zf = (Result == 0);
    State->Flags.Sf = ((Result & SIGN_FLAG_BYTE) != 0);
    State->Flags.Pf = Fast486CalculateParity(Result);

    if (State->Flags.Zf)
    {
        /* Load the source operand into the destination */
        Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Source);
    }
    else
    {
        /* Load the destination into AL */
        State->GeneralRegs[FAST486_REG_EAX].LowByte = Destination;
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeCmpXchg)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Source, Destination, Result;
        ULONG Accumulator = State->GeneralRegs[FAST486_REG_EAX].Long;

        /* Read the operands */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, &Source, &Destination))
        {
            /* Exception occurred */
            return;
        }

        /* Compare EAX with the destination */
        Result = Accumulator - Destination;

        /* Update the flags */
        State->Flags.Cf = (Accumulator < Destination);
        State->Flags.Of = ((Accumulator & SIGN_FLAG_LONG) != (Destination & SIGN_FLAG_LONG))
                          && ((Accumulator & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = (Accumulator & 0x0F) < (Destination & 0x0F);
        State->Flags.Zf = (Result == 0);
        State->Flags.Sf = ((Result & SIGN_FLAG_LONG) != 0);
        State->Flags.Pf = Fast486CalculateParity(Result);

        if (State->Flags.Zf)
        {
            /* Load the source operand into the destination */
            Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Source);
        }
        else
        {
            /* Load the destination into EAX */
            State->GeneralRegs[FAST486_REG_EAX].Long = Destination;
        }
    }
    else
    {
        USHORT Source, Destination, Result;
        USHORT Accumulator = State->GeneralRegs[FAST486_REG_EAX].LowWord;

        /* Read the operands */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, &Source, &Destination))
        {
            /* Exception occurred */
            return;
        }

        /* Compare AX with the destination */
        Result = Accumulator - Destination;

        /* Update the flags */
        State->Flags.Cf = (Accumulator < Destination);
        State->Flags.Of = ((Accumulator & SIGN_FLAG_WORD) != (Destination & SIGN_FLAG_WORD))
                          && ((Accumulator & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = (Accumulator & 0x0F) < (Destination & 0x0F);
        State->Flags.Zf = (Result == 0);
        State->Flags.Sf = ((Result & SIGN_FLAG_WORD) != 0);
        State->Flags.Pf = Fast486CalculateParity(Result);

        if (State->Flags.Zf)
        {
            /* Load the source operand into the destination */
            Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Source);
        }
        else
        {
            /* Load the destination into AX */
            State->GeneralRegs[FAST486_REG_EAX].LowWord = Destination;
        }
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLss)
{
    UCHAR FarPointer[6];
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xB2);

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (!ModRegRm.Memory)
    {
        /* Invalid */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    if (!Fast486ReadMemory(State,
                           (State->PrefixFlags & FAST486_PREFIX_SEG)
                           ? State->SegmentOverride : FAST486_REG_DS,
                           ModRegRm.MemoryAddress,
                           FALSE,
                           FarPointer,
                           OperandSize ? 6 : 4))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Offset = *((PULONG)FarPointer);
        USHORT Segment = *((PUSHORT)&FarPointer[sizeof(ULONG)]);

        /* Set the register to the offset */
        State->GeneralRegs[ModRegRm.Register].Long = Offset;

        /* Load the segment */
        Fast486LoadSegment(State, FAST486_REG_SS, Segment);
    }
    else
    {
        USHORT Offset = *((PUSHORT)FarPointer);
        USHORT Segment = *((PUSHORT)&FarPointer[sizeof(USHORT)]);

        /* Set the register to the offset */
        State->GeneralRegs[ModRegRm.Register].LowWord = Offset;

        /* Load the segment */
        Fast486LoadSegment(State, FAST486_REG_SS, Segment);
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBtr)
{
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    UINT DataSize;
    ULONG BitNumber;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the number of bits */
    if (OperandSize) DataSize = 32;
    else DataSize = 16;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Get the bit number */
    BitNumber = OperandSize ? State->GeneralRegs[ModRegRm.Register].Long
                            : (ULONG)State->GeneralRegs[ModRegRm.Register].LowWord;

    if (ModRegRm.Memory)
    {
        /*
         * For memory operands, add the bit offset divided by
         * the data size to the address
         */
        ModRegRm.MemoryAddress += (BitNumber / DataSize) * (DataSize / 8);
    }

    /* Normalize the bit number */
    BitNumber %= DataSize;

    if (OperandSize)
    {
        ULONG Value;

        /* Read the value */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Set CF to the bit value */
        State->Flags.Cf = (Value >> BitNumber) & 1;

        /* Clear the bit */
        Value &= ~(1 << BitNumber);

        /* Write back the result */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    }
    else
    {
        USHORT Value;

        /* Read the value */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Set CF to the bit value */
        State->Flags.Cf = (Value >> BitNumber) & 1;

        /* Clear the bit */
        Value &= ~(1 << BitNumber);

        /* Write back the result */
        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLfsLgs)
{
    UCHAR FarPointer[6];
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFE) == 0xB4);

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (!ModRegRm.Memory)
    {
        /* Invalid */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    if (!Fast486ReadMemory(State,
                           (State->PrefixFlags & FAST486_PREFIX_SEG)
                           ? State->SegmentOverride : FAST486_REG_DS,
                           ModRegRm.MemoryAddress,
                           FALSE,
                           FarPointer,
                           OperandSize ? 6 : 4))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Offset = *((PULONG)FarPointer);
        USHORT Segment = *((PUSHORT)&FarPointer[sizeof(ULONG)]);

        /* Set the register to the offset */
        State->GeneralRegs[ModRegRm.Register].Long = Offset;

        /* Load the segment */
        Fast486LoadSegment(State,
                           (Opcode == 0xB4)
                           ? FAST486_REG_FS : FAST486_REG_GS,
                           Segment);
    }
    else
    {
        USHORT Offset = *((PUSHORT)FarPointer);
        USHORT Segment = *((PUSHORT)&FarPointer[sizeof(USHORT)]);

        /* Set the register to the offset */
        State->GeneralRegs[ModRegRm.Register].LowWord = Offset;

        /* Load the segment */
        Fast486LoadSegment(State,
                           (Opcode == 0xB4)
                           ? FAST486_REG_FS : FAST486_REG_GS,
                           Segment);
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeMovzxByte)
{
    UCHAR Value;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    TOGGLE_ADSIZE(AddressSize);

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xB6);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Read the operands */
    if (!Fast486ReadModrmByteOperands(State, &ModRegRm, NULL, &Value))
    {
        /* Exception occurred */
        return;
    }

    /* Write back the zero-extended value */
    Fast486WriteModrmDwordOperands(State,
                                   &ModRegRm,
                                   TRUE,
                                   (ULONG)Value);
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeMovzxWord)
{
    USHORT Value;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    TOGGLE_ADSIZE(AddressSize);

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xB7);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Read the operands */
    if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
    {
        /* Exception occurred */
        return;
    }

    /* Write back the zero-extended value */
    Fast486WriteModrmDwordOperands(State,
                                   &ModRegRm,
                                   TRUE,
                                   (ULONG)Value);
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBtc)
{
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    UINT DataSize;
    ULONG BitNumber;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the number of bits */
    if (OperandSize) DataSize = 32;
    else DataSize = 16;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Get the bit number */
    BitNumber = OperandSize ? State->GeneralRegs[ModRegRm.Register].Long
                            : (ULONG)State->GeneralRegs[ModRegRm.Register].LowWord;

    if (ModRegRm.Memory)
    {
        /*
         * For memory operands, add the bit offset divided by
         * the data size to the address
         */
        ModRegRm.MemoryAddress += (BitNumber / DataSize) * (DataSize / 8);
    }

    /* Normalize the bit number */
    BitNumber %= DataSize;

    if (OperandSize)
    {
        ULONG Value;

        /* Read the value */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Set CF to the bit value */
        State->Flags.Cf = (Value >> BitNumber) & 1;

        /* Toggle the bit */
        Value ^= 1 << BitNumber;

        /* Write back the result */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    }
    else
    {
        USHORT Value;

        /* Read the value */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Set CF to the bit value */
        State->Flags.Cf = (Value >> BitNumber) & 1;

        /* Toggle the bit */
        Value ^= 1 << BitNumber;

        /* Write back the result */
        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBsf)
{
    UINT i;
    ULONG Value = 0;
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    ULONG BitNumber;
    UINT DataSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xBC);

    /* Get the number of bits */
    if (OperandSize) DataSize = 32;
    else DataSize = 16;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Read the value */
    if (OperandSize)
    {
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }
    }
    else
    {
        if (!Fast486ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          (PUSHORT)NULL,
                                          (PUSHORT)&Value))
        {
            /* Exception occurred */
            return;
        }
    }

    /* Set ZF */
    State->Flags.Zf = (Value == 0);
    if (State->Flags.Zf) return;

    for (i = 0; i < DataSize; i++)
    {
        if (Value & (1 << i))
        {
            /* Save the bit number */
            BitNumber = i;

            /* Exit the loop */
            break;
        }
    }

    /* Write back the result */
    if (OperandSize) Fast486WriteModrmDwordOperands(State, &ModRegRm, TRUE, BitNumber);
    else Fast486WriteModrmWordOperands(State, &ModRegRm, TRUE, LOWORD(BitNumber));
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBsr)
{
    INT i;
    ULONG Value = 0;
    BOOLEAN OperandSize, AddressSize;
    FAST486_MOD_REG_RM ModRegRm;
    ULONG BitNumber;
    UINT DataSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xBD);

    /* Get the number of bits */
    if (OperandSize) DataSize = 32;
    else DataSize = 16;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Read the value */
    if (OperandSize)
    {
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }
    }
    else
    {
        if (!Fast486ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          (PUSHORT)NULL,
                                          (PUSHORT)&Value))
        {
            /* Exception occurred */
            return;
        }
    }

    /* Set ZF according to the value */
    State->Flags.Zf = (Value == 0);
    if (State->Flags.Zf) return;

    for (i = DataSize - 1; i >= 0; i--)
    {
        if (Value & (1 << i))
        {
            /* Save the bit number */
            BitNumber = i;

            /* Exit the loop */
            break;
        }
    }

    /* Write back the result */
    if (OperandSize) Fast486WriteModrmDwordOperands(State, &ModRegRm, TRUE, BitNumber);
    else Fast486WriteModrmWordOperands(State, &ModRegRm, TRUE, LOWORD(BitNumber));
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeMovsxByte)
{
    CHAR Value;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    TOGGLE_ADSIZE(AddressSize);

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xBE);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Read the operands */
    if (!Fast486ReadModrmByteOperands(State, &ModRegRm, NULL, (PUCHAR)&Value))
    {
        /* Exception occurred */
        return;
    }

    /* Write back the sign-extended value */
    Fast486WriteModrmDwordOperands(State,
                                   &ModRegRm,
                                   TRUE,
                                   (ULONG)((LONG)Value));
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeMovsxWord)
{
    SHORT Value;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    TOGGLE_ADSIZE(AddressSize);

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xBF);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Read the operands */
    if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, (PUSHORT)&Value))
    {
        /* Exception occurred */
        return;
    }

    /* Write back the sign-extended value */
    Fast486WriteModrmDwordOperands(State,
                                   &ModRegRm,
                                   TRUE,
                                   (ULONG)((LONG)Value));
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeConditionalJmp)
{
    BOOLEAN Jump = FALSE;
    LONG Offset = 0;
    BOOLEAN Size = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(Size);
    NO_LOCK_PREFIX();

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF0) == 0x80);

    /* Fetch the offset */
    if (Size)
    {
        if (!Fast486FetchDword(State, (PULONG)&Offset))
        {
            /* Exception occurred */
            return;
        }
    }
    else
    {
        SHORT Value;

        if (!Fast486FetchWord(State, (PUSHORT)&Value))
        {
            /* Exception occurred */
            return;
        }

        /* Sign-extend */
        Offset = (LONG)Value;
    }

    switch ((Opcode & 0x0F) >> 1)
    {
        /* JO / JNO */
        case 0:
        {
            Jump = State->Flags.Of;
            break;
        }

        /* JC / JNC */
        case 1:
        {
            Jump = State->Flags.Cf;
            break;
        }

        /* JZ / JNZ */
        case 2:
        {
            Jump = State->Flags.Zf;
            break;
        }

        /* JBE / JNBE */
        case 3:
        {
            Jump = State->Flags.Cf || State->Flags.Zf;
            break;
        }

        /* JS / JNS */
        case 4:
        {
            Jump = State->Flags.Sf;
            break;
        }

        /* JP / JNP */
        case 5:
        {
            Jump = State->Flags.Pf;
            break;
        }

        /* JL / JNL */
        case 6:
        {
            Jump = State->Flags.Sf != State->Flags.Of;
            break;
        }

        /* JLE / JNLE */
        case 7:
        {
            Jump = (State->Flags.Sf != State->Flags.Of) || State->Flags.Zf;
            break;
        }
    }

    if (Opcode & 1)
    {
        /* Invert the result */
        Jump = !Jump;
    }

    if (Jump)
    {
        /* Move the instruction pointer */
        State->InstPtr.Long += Offset;
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeConditionalSet)
{
    BOOLEAN Value = FALSE;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF0) == 0x90);

    switch ((Opcode & 0x0F) >> 1)
    {
        /* SETO / SETNO */
        case 0:
        {
            Value = State->Flags.Of;
            break;
        }

        /* SETC / SETNC */
        case 1:
        {
            Value = State->Flags.Cf;
            break;
        }

        /* SETZ / SETNZ */
        case 2:
        {
            Value = State->Flags.Zf;
            break;
        }

        /* SETBE / SETNBE */
        case 3:
        {
            Value = State->Flags.Cf || State->Flags.Zf;
            break;
        }

        /* SETS / SETNS */
        case 4:
        {
            Value = State->Flags.Sf;
            break;
        }

        /* SETP / SETNP */
        case 5:
        {
            Value = State->Flags.Pf;
            break;
        }

        /* SETL / SETNL */
        case 6:
        {
            Value = State->Flags.Sf != State->Flags.Of;
            break;
        }

        /* SETLE / SETNLE */
        case 7:
        {
            Value = (State->Flags.Sf != State->Flags.Of) || State->Flags.Zf;
            break;
        }
    }

    if (Opcode & 1)
    {
        /* Invert the result */
        Value = !Value;
    }

    /* Write back the result */
    Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Value);
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeXaddByte)
{
    UCHAR Source, Destination, Result;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xC0);

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (!Fast486ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &Source,
                                      &Destination))
    {
        /* Exception occurred */
        return;
    }

    /* Calculate the result */
    Result = Source + Destination;

    /* Update the flags */
    State->Flags.Cf = (Result < Source) && (Result < Destination);
    State->Flags.Of = ((Source & SIGN_FLAG_BYTE) == (Destination & SIGN_FLAG_BYTE))
                      && ((Source & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = ((((Source & 0x0F) + (Destination & 0x0F)) & 0x10) != 0);
    State->Flags.Zf = (Result == 0);
    State->Flags.Sf = ((Result & SIGN_FLAG_BYTE) != 0);
    State->Flags.Pf = Fast486CalculateParity(Result);

    /* Write the sum to the destination */
    if (!Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Result))
    {
        /* Exception occurred */
        return;
    }

    /* Write the old value of the destination to the source */
    Fast486WriteModrmByteOperands(State, &ModRegRm, TRUE, Destination);
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeXadd)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xC1);

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);
    TOGGLE_OPSIZE(OperandSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG Source, Destination, Result;

        if (!Fast486ReadModrmDwordOperands(State,
                                           &ModRegRm,
                                           &Source,
                                           &Destination))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Result = Source + Destination;

        /* Update the flags */
        State->Flags.Cf = (Result < Source) && (Result < Destination);
        State->Flags.Of = ((Source & SIGN_FLAG_LONG) == (Destination & SIGN_FLAG_LONG))
                          && ((Source & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = ((((Source & 0x0F) + (Destination & 0x0F)) & 0x10) != 0);
        State->Flags.Zf = (Result == 0);
        State->Flags.Sf = ((Result & SIGN_FLAG_LONG) != 0);
        State->Flags.Pf = Fast486CalculateParity(Result);

        /* Write the old value of the destination to the source */
        if (!Fast486WriteModrmDwordOperands(State, &ModRegRm, TRUE, Destination))
        {
            /* Exception occurred */
            return;
        }

        /* Write the sum to the destination */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Result);
    }
    else
    {
        USHORT Source, Destination, Result;

        if (!Fast486ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &Source,
                                          &Destination))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Result = Source + Destination;

        /* Update the flags */
        State->Flags.Cf = (Result < Source) && (Result < Destination);
        State->Flags.Of = ((Source & SIGN_FLAG_WORD) == (Destination & SIGN_FLAG_WORD))
                          && ((Source & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = ((((Source & 0x0F) + (Destination & 0x0F)) & 0x10) != 0);
        State->Flags.Zf = (Result == 0);
        State->Flags.Sf = ((Result & SIGN_FLAG_WORD) != 0);
        State->Flags.Pf = Fast486CalculateParity(Result);

        /* Write the old value of the destination to the source */
        if (!Fast486WriteModrmWordOperands(State, &ModRegRm, TRUE, Destination))
        {
            /* Exception occurred */
            return;
        }

        /* Write the sum to the destination */
        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Result);
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBswap)
{
    PUCHAR Pointer;

    NO_LOCK_PREFIX();

    /* Get a pointer to the value */
    Pointer = (PUCHAR)&State->GeneralRegs[Opcode & 0x07].Long;

    /* Swap the byte order */
    SWAP(Pointer[0], Pointer[3]);
    SWAP(Pointer[1], Pointer[2]);
}

FAST486_OPCODE_HANDLER(Fast486OpcodeExtended)
{
    UCHAR SecondOpcode;

    /* Fetch the second operation code */
    if (!Fast486FetchByte(State, &SecondOpcode))
    {
        /* Exception occurred */
        return;
    }

    /* Call the extended opcode handler */
    Fast486ExtendedHandlers[SecondOpcode](State, SecondOpcode);
}

/* EOF */
