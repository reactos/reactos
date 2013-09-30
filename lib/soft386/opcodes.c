/*
 * Soft386 386/486 CPU Emulation Library
 * opcodes.c
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

// #define WIN32_NO_STATUS
// #define _INC_WINDOWS
#include <windef.h>
#include <limits.h>

// #define NDEBUG
#include <debug.h>

#include <soft386.h>
#include "opcodes.h"
#include "opgroups.h"
#include "common.h"

/* PUBLIC VARIABLES ***********************************************************/

SOFT386_OPCODE_HANDLER_PROC
Soft386OpcodeHandlers[SOFT386_NUM_OPCODE_HANDLERS] =
{
    Soft386OpcodeAddByteModrm,
    Soft386OpcodeAddModrm,
    Soft386OpcodeAddByteModrm,
    Soft386OpcodeAddModrm,
    Soft386OpcodeAddAl,
    Soft386OpcodeAddEax,
    Soft386OpcodePushEs,
    Soft386OpcodePopEs,
    Soft386OpcodeOrByteModrm,
    Soft386OpcodeOrModrm,
    Soft386OpcodeOrByteModrm,
    Soft386OpcodeOrModrm,
    Soft386OpcodeOrAl,
    Soft386OpcodeOrEax,
    Soft386OpcodePushCs,
    NULL, // TODO: OPCODE 0x0F NOT SUPPORTED
    Soft386OpcodeAdcByteModrm,
    Soft386OpcodeAdcModrm,
    Soft386OpcodeAdcByteModrm,
    Soft386OpcodeAdcModrm,
    Soft386OpcodeAdcAl,
    Soft386OpcodeAdcEax,
    Soft386OpcodePushSs,
    Soft386OpcodePopSs,
    Soft386OpcodeSbbByteModrm,
    Soft386OpcodeSbbModrm,
    Soft386OpcodeSbbByteModrm,
    Soft386OpcodeSbbModrm,
    Soft386OpcodeSbbAl,
    Soft386OpcodeSbbEax,
    Soft386OpcodePushDs,
    Soft386OpcodePopDs,
    Soft386OpcodeAndByteModrm,
    Soft386OpcodeAndModrm,
    Soft386OpcodeAndByteModrm,
    Soft386OpcodeAndModrm,
    Soft386OpcodeAndAl,
    Soft386OpcodeAndEax,
    Soft386OpcodePrefix,
    Soft386OpcodeDaa,
    Soft386OpcodeCmpSubByteModrm,
    Soft386OpcodeCmpSubModrm,
    Soft386OpcodeCmpSubByteModrm,
    Soft386OpcodeCmpSubModrm,
    Soft386OpcodeCmpSubAl,
    Soft386OpcodeCmpSubEax,
    Soft386OpcodePrefix,
    Soft386OpcodeDas,
    Soft386OpcodeXorByteModrm,
    Soft386OpcodeXorModrm,
    Soft386OpcodeXorByteModrm,
    Soft386OpcodeXorModrm,
    Soft386OpcodeXorAl,
    Soft386OpcodeXorEax,
    Soft386OpcodePrefix,
    Soft386OpcodeAaa,
    Soft386OpcodeCmpSubByteModrm,
    Soft386OpcodeCmpSubModrm,
    Soft386OpcodeCmpSubByteModrm,
    Soft386OpcodeCmpSubModrm,
    Soft386OpcodeCmpSubAl,
    Soft386OpcodeCmpSubEax,
    Soft386OpcodePrefix,
    Soft386OpcodeAas,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeIncrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodeDecrement,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePushReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePopReg,
    Soft386OpcodePushAll,
    Soft386OpcodePopAll,
    Soft386OpcodeBound,
    Soft386OpcodeArpl,
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    Soft386OpcodePushImm,
    Soft386OpcodeImulModrmImm,
    Soft386OpcodePushByteImm,
    Soft386OpcodeImulModrmImm,
    NULL, // TODO: OPCODE 0x6C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6F NOT SUPPORTED
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeShortConditionalJmp,
    Soft386OpcodeGroup8082,
    Soft386OpcodeGroup81,
    Soft386OpcodeGroup8082,
    Soft386OpcodeGroup83,
    Soft386OpcodeTestByteModrm,
    Soft386OpcodeTestModrm,
    Soft386OpcodeXchgByteModrm,
    Soft386OpcodeXchgModrm,
    Soft386OpcodeMovByteModrm,
    Soft386OpcodeMovModrm,
    Soft386OpcodeMovByteModrm,
    Soft386OpcodeMovModrm,
    Soft386OpcodeMovStoreSeg,
    Soft386OpcodeLea,
    Soft386OpcodeMovLoadSeg,
    Soft386OpcodeGroup8F,
    Soft386OpcodeNop,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeCwde,
    Soft386OpcodeCdq,
    Soft386OpcodeCallAbs,
    Soft386OpcodeWait,
    Soft386OpcodePushFlags,
    Soft386OpcodePopFlags,
    Soft386OpcodeSahf,
    Soft386OpcodeLahf,
    Soft386OpcodeMovAlOffset,
    Soft386OpcodeMovEaxOffset,
    Soft386OpcodeMovOffsetAl,
    Soft386OpcodeMovOffsetEax,
    NULL, // TODO: OPCODE 0xA4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA7 NOT SUPPORTED
    Soft386OpcodeTestAl,
    Soft386OpcodeTestEax,
    NULL, // TODO: OPCODE 0xAA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAF NOT SUPPORTED
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovByteRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeMovRegImm,
    Soft386OpcodeGroupC0,
    Soft386OpcodeGroupC1,
    Soft386OpcodeRet,
    Soft386OpcodeRet,
    Soft386OpcodeLdsLes,
    Soft386OpcodeLdsLes,
    Soft386OpcodeGroupC6,
    Soft386OpcodeGroupC7,
    Soft386OpcodeEnter,
    Soft386OpcodeLeave,
    Soft386OpcodeRetFarImm,
    Soft386OpcodeRetFar,
    Soft386OpcodeInt,
    Soft386OpcodeInt,
    Soft386OpcodeInt,
    Soft386OpcodeIret,
    Soft386OpcodeGroupD0,
    Soft386OpcodeGroupD1,
    Soft386OpcodeGroupD2,
    Soft386OpcodeGroupD3,
    Soft386OpcodeAam,
    Soft386OpcodeAad,
    NULL, // TODO: OPCODE 0xD6 NOT SUPPORTED
    Soft386OpcodeXlat,
    NULL, // TODO: OPCODE 0xD8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDF NOT SUPPORTED
    Soft386OpcodeLoop,
    Soft386OpcodeLoop,
    Soft386OpcodeLoop,
    Soft386OpcodeJecxz,
    Soft386OpcodeInByte,
    Soft386OpcodeIn,
    Soft386OpcodeOutByte,
    Soft386OpcodeOut,
    Soft386OpcodeCall,
    Soft386OpcodeJmp,
    Soft386OpcodeJmpAbs,
    Soft386OpcodeShortJump,
    Soft386OpcodeInByte,
    Soft386OpcodeIn,
    Soft386OpcodeOutByte,
    Soft386OpcodeOut,
    Soft386OpcodePrefix,
    NULL, // Invalid
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    Soft386OpcodeHalt,
    Soft386OpcodeComplCarry,
    Soft386OpcodeGroupF6,
    Soft386OpcodeGroupF7,
    Soft386OpcodeClearCarry,
    Soft386OpcodeSetCarry,
    Soft386OpcodeClearInt,
    Soft386OpcodeSetInt,
    Soft386OpcodeClearDir,
    Soft386OpcodeSetDir,
    Soft386OpcodeGroupFE,
    Soft386OpcodeGroupFF,
};

/* PUBLIC FUNCTIONS ***********************************************************/

SOFT386_OPCODE_HANDLER(Soft386OpcodePrefix)
{
    BOOLEAN Valid = FALSE;

    switch (Opcode)
    {
        /* ES: */
        case 0x26:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_ES;
                Valid = TRUE;
            }

            break;
        }

        /* CS: */
        case 0x2E:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_CS;
                Valid = TRUE;
            }

            break;
        }

        /* SS: */
        case 0x36:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_SS;
                Valid = TRUE;
            }

            break;
        }

        /* DS: */
        case 0x3E:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_DS;
                Valid = TRUE;
            }

            break;
        }

        /* FS: */
        case 0x64:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_FS;
                Valid = TRUE;
            }

            break;
        }

        /* GS: */
        case 0x65:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_SEG))
            {
                State->PrefixFlags |= SOFT386_PREFIX_SEG;
                State->SegmentOverride = SOFT386_REG_GS;
                Valid = TRUE;
            }

            break;
        }

        /* OPSIZE */
        case 0x66:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_OPSIZE))
            {
                State->PrefixFlags |= SOFT386_PREFIX_OPSIZE;
                Valid = TRUE;
            }

            break;
        }

        /* ADSIZE */
        case 0x67:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_ADSIZE))
            {
                State->PrefixFlags |= SOFT386_PREFIX_ADSIZE;
                Valid = TRUE;
            }
            break;
        }

        /* LOCK */
        case 0xF0:
        {
            if (!(State->PrefixFlags & SOFT386_PREFIX_LOCK))
            {
                State->PrefixFlags |= SOFT386_PREFIX_LOCK;
                Valid = TRUE;
            }

            break;
        }

        /* REPNZ */
        case 0xF2:
        {
            /* Mutually exclusive with REP */
            if (!(State->PrefixFlags
                & (SOFT386_PREFIX_REPNZ | SOFT386_PREFIX_REP)))
            {
                State->PrefixFlags |= SOFT386_PREFIX_REPNZ;
                Valid = TRUE;
            }

            break;
        }

        /* REP / REPZ */
        case 0xF3:
        {
            /* Mutually exclusive with REPNZ */
            if (!(State->PrefixFlags
                & (SOFT386_PREFIX_REPNZ | SOFT386_PREFIX_REP)))
            {
                State->PrefixFlags |= SOFT386_PREFIX_REP;
                Valid = TRUE;
            }

            break;
        }
    }

    if (!Valid)
    {
        /* Clear all prefixes */
        State->PrefixFlags = 0;

        /* Throw an exception */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeIncrement)
{
    ULONG Value;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x40);

    if (Size)
    {
        Value = ++State->GeneralRegs[Opcode & 0x07].Long;

        State->Flags.Of = (Value == SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Sf = (Value & SIGN_FLAG_LONG) ? TRUE : FALSE;
    }
    else
    {
        Value = ++State->GeneralRegs[Opcode & 0x07].LowWord;

        State->Flags.Of = (Value == SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Sf = (Value & SIGN_FLAG_WORD) ? TRUE : FALSE;
    }

    State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
    State->Flags.Af = ((Value & 0x0F) == 0) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(LOBYTE(Value));

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeDecrement)
{
    ULONG Value;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x48);

    if (Size)
    {
        Value = --State->GeneralRegs[Opcode & 0x07].Long;

        State->Flags.Of = (Value == (SIGN_FLAG_LONG - 1)) ? TRUE : FALSE;
        State->Flags.Sf = (Value & SIGN_FLAG_LONG) ? TRUE : FALSE;
    }
    else
    {
        Value = --State->GeneralRegs[Opcode & 0x07].LowWord;

        State->Flags.Of = (Value == (SIGN_FLAG_WORD - 1)) ? TRUE : FALSE;
        State->Flags.Sf = (Value & SIGN_FLAG_WORD) ? TRUE : FALSE;
    }

    State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
    State->Flags.Af = ((Value & 0x0F) == 0x0F) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(LOBYTE(Value));

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushReg)
{
    if ((State->PrefixFlags != SOFT386_PREFIX_OPSIZE)
        && (State->PrefixFlags != 0))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x50);

    /* Call the internal function */
    return Soft386StackPush(State, State->GeneralRegs[Opcode & 0x07].Long);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePopReg)
{
    ULONG Value;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_SS].Size;

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x58);

    /* Call the internal function */
    if (!Soft386StackPop(State, &Value)) return FALSE;

    /* Store the value */
    if (Size) State->GeneralRegs[Opcode & 0x07].Long = Value;
    else State->GeneralRegs[Opcode & 0x07].LowWord = Value;

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeNop)
{
    if (State->PrefixFlags & ~(SOFT386_PREFIX_OPSIZE | SOFT386_PREFIX_REP))
    {
        /* Allowed prefixes are REP and OPSIZE */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_REP)
    {
        /* Idle cycle */
        State->IdleCallback(State);
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeExchangeEax)
{
    INT Reg = Opcode & 0x07;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0x90);

    /* Exchange the values */
    if (Size)
    {
        ULONG Value;

        Value = State->GeneralRegs[Reg].Long;
        State->GeneralRegs[Reg].Long = State->GeneralRegs[SOFT386_REG_EAX].Long;
        State->GeneralRegs[SOFT386_REG_EAX].Long = Value;
    }
    else
    {
        USHORT Value;

        Value = State->GeneralRegs[Reg].LowWord;
        State->GeneralRegs[Reg].LowWord = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Value;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeShortConditionalJmp)
{
    BOOLEAN Jump = FALSE;
    CHAR Offset = 0;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF0) == 0x70);

    /* Fetch the offset */
    if (!Soft386FetchByte(State, (PUCHAR)&Offset))
    {
        /* An exception occurred */
        return FALSE;
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

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeClearCarry)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xF8);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Clear CF and return success */
    State->Flags.Cf = FALSE;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSetCarry)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xF9);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Set CF and return success*/
    State->Flags.Cf = TRUE;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeComplCarry)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xF5);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Toggle CF and return success */
    State->Flags.Cf = !State->Flags.Cf;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeClearInt)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xFA);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        /* Check IOPL */
        if (State->Flags.Iopl >= State->SegmentRegs[SOFT386_REG_CS].Dpl)
        {
            /* Clear the interrupt flag */
            State->Flags.If = FALSE;
        }
        else
        {
            /* General Protection Fault */
            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }
    }
    else
    {
        /* Just clear the interrupt flag */
        State->Flags.If = FALSE;
    }

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSetInt)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xFB);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        /* Check IOPL */
        if (State->Flags.Iopl >= State->SegmentRegs[SOFT386_REG_CS].Dpl)
        {
            /* Set the interrupt flag */
            State->Flags.If = TRUE;
        }
        else
        {
            /* General Protection Fault */
            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }
    }
    else
    {
        /* Just set the interrupt flag */
        State->Flags.If = TRUE;
    }

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeClearDir)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xFC);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Clear DF and return success */
    State->Flags.Df = FALSE;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSetDir)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xFD);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Set DF and return success*/
    State->Flags.Df = TRUE;
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeHalt)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xF4);

    /* No prefixes allowed */
    if (State->PrefixFlags)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Privileged instructions can only be executed under CPL = 0 */
    if (State->SegmentRegs[SOFT386_REG_CS].Dpl != 0)
    {
        Soft386Exception(State, SOFT386_EXCEPTION_GP);
        return FALSE;
    }

    /* Halt */
    while (!State->HardwareInt) State->IdleCallback(State);

    /* Return success */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeInByte)
{
    UCHAR Data;
    ULONG Port;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF7) == 0xE4);

    if (Opcode == 0xE4)
    {
        /* Fetch the parameter */
        if (!Soft386FetchByte(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set the port number to the parameter */
        Port = Data;
    }
    else
    {
        /* The port number is in DX */
        Port = State->GeneralRegs[SOFT386_REG_EDX].LowWord;
    }

    /* Read a byte from the I/O port */
    State->IoReadCallback(State, Port, &Data, sizeof(UCHAR));

    /* Store the result in AL */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Data;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeIn)
{
    ULONG Port;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF7) == 0xE5);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Opcode == 0xE5)
    {
        UCHAR Data;

        /* Fetch the parameter */
        if (!Soft386FetchByte(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set the port number to the parameter */
        Port = Data;
    }
    else
    {
        /* The port number is in DX */
        Port = State->GeneralRegs[SOFT386_REG_EDX].LowWord;
    }

    if (Size)
    {
        ULONG Data;

        /* Read a dword from the I/O port */
        State->IoReadCallback(State, Port, &Data, sizeof(ULONG));

        /* Store the value in EAX */
        State->GeneralRegs[SOFT386_REG_EAX].Long = Data;
    }
    else
    {
        USHORT Data;

        /* Read a word from the I/O port */
        State->IoReadCallback(State, Port, &Data, sizeof(USHORT));

        /* Store the value in AX */
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Data;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeOutByte)
{
    UCHAR Data;
    ULONG Port;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF7) == 0xE6);

    if (Opcode == 0xE6)
    {
        /* Fetch the parameter */
        if (!Soft386FetchByte(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set the port number to the parameter */
        Port = Data;
    }
    else
    {
        /* The port number is in DX */
        Port = State->GeneralRegs[SOFT386_REG_EDX].LowWord;
    }

    /* Read the value from AL */
    Data = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    
    /* Write the byte to the I/O port */
    State->IoWriteCallback(State, Port, &Data, sizeof(UCHAR));

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeOut)
{
    ULONG Port;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF7) == 0xE7);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Opcode == 0xE7)
    {
        UCHAR Data;

        /* Fetch the parameter */
        if (!Soft386FetchByte(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set the port number to the parameter */
        Port = Data;
    }
    else
    {
        /* The port number is in DX */
        Port = State->GeneralRegs[SOFT386_REG_EDX].LowWord;
    }

    if (Size)
    {
        /* Get the value from EAX */
        ULONG Data = State->GeneralRegs[SOFT386_REG_EAX].Long;

        /* Write a dword to the I/O port */
        State->IoReadCallback(State, Port, &Data, sizeof(ULONG));
    }
    else
    {
        /* Get the value from AX */
        USHORT Data = State->GeneralRegs[SOFT386_REG_EAX].LowWord;

        /* Write a word to the I/O port */
        State->IoWriteCallback(State, Port, &Data, sizeof(USHORT));
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeShortJump)
{
    CHAR Offset = 0;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xEB);

    /* Fetch the offset */
    if (!Soft386FetchByte(State, (PUCHAR)&Offset))
    {
        /* An exception occurred */
        return FALSE;
    }

    /* Move the instruction pointer */        
    State->InstPtr.Long += Offset;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovRegImm)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0xB8);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Size)
    {
        ULONG Value;

        /* Fetch the dword */
        if (!Soft386FetchDword(State, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Store the value in the register */
        State->GeneralRegs[Opcode & 0x07].Long = Value;
    }
    else
    {
        USHORT Value;

        /* Fetch the word */
        if (!Soft386FetchWord(State, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Store the value in the register */
        State->GeneralRegs[Opcode & 0x07].LowWord = Value;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovByteRegImm)
{
    UCHAR Value;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF8) == 0xB0);

    if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Fetch the byte */
    if (!Soft386FetchByte(State, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (Opcode & 0x04)
    {
        /* AH, CH, DH or BH */
        State->GeneralRegs[Opcode & 0x03].HighByte = Value;
    }
    else
    {
        /* AL, CL, DL or BL */
        State->GeneralRegs[Opcode & 0x03].LowByte = Value;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAddByteModrm)
{
    UCHAR FirstValue, SecondValue, Result;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x00);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue + SecondValue;

    /* Update the flags */
    State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
    State->Flags.Of = ((FirstValue & SIGN_FLAG_BYTE) == (SecondValue & SIGN_FLAG_BYTE))
                      && ((FirstValue & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         Opcode & SOFT386_OPCODE_WRITE_REG,
                                         Result);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAddModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x01);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue + SecondValue;

        /* Update the flags */
        State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
        State->Flags.Of = ((FirstValue & SIGN_FLAG_LONG) == (SecondValue & SIGN_FLAG_LONG))
                          && ((FirstValue & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
    else
    {
        USHORT FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue + SecondValue;

        /* Update the flags */
        State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
        State->Flags.Of = ((FirstValue & SIGN_FLAG_WORD) == (SecondValue & SIGN_FLAG_WORD))
                          && ((FirstValue & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAddAl)
{
    UCHAR FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    UCHAR SecondValue, Result;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x04);

    if (State->PrefixFlags)
    {
        /* This opcode doesn't take any prefixes */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (!Soft386FetchByte(State, &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue + SecondValue;

    /* Update the flags */
    State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
    State->Flags.Of = ((FirstValue & SIGN_FLAG_BYTE) == (SecondValue & SIGN_FLAG_BYTE))
                      && ((FirstValue & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Result;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAddEax)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x05);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG FirstValue = State->GeneralRegs[SOFT386_REG_EAX].Long;
        ULONG SecondValue, Result;

        if (!Soft386FetchDword(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue + SecondValue;

        /* Update the flags */
        State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
        State->Flags.Of = ((FirstValue & SIGN_FLAG_LONG) == (SecondValue & SIGN_FLAG_LONG))
                          && ((FirstValue & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].Long = Result;
    }
    else
    {
        USHORT FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        USHORT SecondValue, Result;

        if (!Soft386FetchWord(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue + SecondValue;

        /* Update the flags */
        State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
        State->Flags.Of = ((FirstValue & SIGN_FLAG_WORD) == (SecondValue & SIGN_FLAG_WORD))
                          && ((FirstValue & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Result;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeOrByteModrm)
{
    UCHAR FirstValue, SecondValue, Result;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x08);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue | SecondValue;

    /* Update the flags */
    State->Flags.Cf = FALSE;
    State->Flags.Of = FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         Opcode & SOFT386_OPCODE_WRITE_REG,
                                         Result);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeOrModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x09);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue | SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
    else
    {
        USHORT FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue | SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeOrAl)
{
    UCHAR FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    UCHAR SecondValue, Result;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x0C);

    if (State->PrefixFlags)
    {
        /* This opcode doesn't take any prefixes */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (!Soft386FetchByte(State, &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue | SecondValue;

    /* Update the flags */
    State->Flags.Cf = FALSE;
    State->Flags.Of = FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Result;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeOrEax)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x0D);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG FirstValue = State->GeneralRegs[SOFT386_REG_EAX].Long;
        ULONG SecondValue, Result;

        if (!Soft386FetchDword(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue | SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].Long = Result;
    }
    else
    {
        USHORT FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        USHORT SecondValue, Result;

        if (!Soft386FetchWord(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue | SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Result;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAndByteModrm)
{
    UCHAR FirstValue, SecondValue, Result;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x20);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue & SecondValue;

    /* Update the flags */
    State->Flags.Cf = FALSE;
    State->Flags.Of = FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         Opcode & SOFT386_OPCODE_WRITE_REG,
                                         Result);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAndModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x21);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue & SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
    else
    {
        USHORT FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue & SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAndAl)
{
    UCHAR FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    UCHAR SecondValue, Result;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x24);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (!Soft386FetchByte(State, &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue & SecondValue;

    /* Update the flags */
    State->Flags.Cf = FALSE;
    State->Flags.Of = FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Result;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAndEax)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x25);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG FirstValue = State->GeneralRegs[SOFT386_REG_EAX].Long;
        ULONG SecondValue, Result;

        if (!Soft386FetchDword(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue & SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].Long = Result;
    }
    else
    {
        USHORT FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        USHORT SecondValue, Result;

        if (!Soft386FetchWord(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue & SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Result;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeXorByteModrm)
{
    UCHAR FirstValue, SecondValue, Result;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x30);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue ^ SecondValue;

    /* Update the flags */
    State->Flags.Cf = FALSE;
    State->Flags.Of = FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         Opcode & SOFT386_OPCODE_WRITE_REG,
                                         Result);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeXorModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x31);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue ^ SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
    else
    {
        USHORT FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue ^ SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeXorAl)
{
    UCHAR FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    UCHAR SecondValue, Result;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x34);

    if (State->PrefixFlags)
    {
        /* This opcode doesn't take any prefixes */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (!Soft386FetchByte(State, &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue ^ SecondValue;

    /* Update the flags */
    State->Flags.Cf = FALSE;
    State->Flags.Of = FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Result;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeXorEax)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x35);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG FirstValue = State->GeneralRegs[SOFT386_REG_EAX].Long;
        ULONG SecondValue, Result;

        if (!Soft386FetchDword(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue ^ SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].Long = Result;
    }
    else
    {
        USHORT FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        USHORT SecondValue, Result;

        if (!Soft386FetchWord(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue ^ SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Result;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeTestByteModrm)
{
    UCHAR FirstValue, SecondValue, Result;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x84);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }
    /* Calculate the result */
    Result = FirstValue & SecondValue;

    /* Update the flags */
    State->Flags.Cf = FALSE;
    State->Flags.Of = FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* The result is discarded */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeTestModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x85);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue & SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);
    }
    else
    {
        USHORT FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue & SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);
    }

    /* The result is discarded */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeTestAl)
{
    UCHAR FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    UCHAR SecondValue, Result;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xA8);

    if (State->PrefixFlags)
    {
        /* This opcode doesn't take any prefixes */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (!Soft386FetchByte(State, &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue & SecondValue;

    /* Update the flags */
    State->Flags.Cf = FALSE;
    State->Flags.Of = FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* The result is discarded */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeTestEax)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xA9);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG FirstValue = State->GeneralRegs[SOFT386_REG_EAX].Long;
        ULONG SecondValue, Result;

        if (!Soft386FetchDword(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue & SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);
    }
    else
    {
        USHORT FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        USHORT SecondValue, Result;

        if (!Soft386FetchWord(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue & SecondValue;

        /* Update the flags */
        State->Flags.Cf = FALSE;
        State->Flags.Of = FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);
    }

    /* The result is discarded */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeXchgByteModrm)
{
    UCHAR FirstValue, SecondValue;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x86);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Write the value from the register to the R/M */
    if (!Soft386WriteModrmByteOperands(State,
                                       &ModRegRm,
                                       FALSE,
                                       FirstValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Write the value from the R/M to the register */
    if (!Soft386WriteModrmByteOperands(State,
                                       &ModRegRm,
                                       TRUE,
                                       SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeXchgModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x87);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Write the value from the register to the R/M */
        if (!Soft386WriteModrmDwordOperands(State,
                                            &ModRegRm,
                                            FALSE,
                                            FirstValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Write the value from the R/M to the register */
        if (!Soft386WriteModrmDwordOperands(State,
                                            &ModRegRm,
                                            TRUE,
                                            SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    }
    else
    {
        USHORT FirstValue, SecondValue;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Write the value from the register to the R/M */
        if (!Soft386WriteModrmWordOperands(State,
                                           &ModRegRm,
                                           FALSE,
                                           FirstValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Write the value from the R/M to the register */
        if (!Soft386WriteModrmWordOperands(State,
                                           &ModRegRm,
                                           TRUE,
                                           SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    }

    /* The result is discarded */
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushEs)
{
    /* Call the internal API */
    return Soft386StackPush(State, State->SegmentRegs[SOFT386_REG_ES].Selector);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePopEs)
{
    ULONG NewSelector;

    if (!Soft386StackPop(State, &NewSelector))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Call the internal API */
    return Soft386LoadSegment(State, SOFT386_REG_ES, LOWORD(NewSelector));
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushCs)
{
    /* Call the internal API */
    return Soft386StackPush(State, State->SegmentRegs[SOFT386_REG_CS].Selector);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAdcByteModrm)
{
    UCHAR FirstValue, SecondValue, Result;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x10);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue + SecondValue + State->Flags.Cf;

    /* Special exception for CF */
    State->Flags.Cf = State->Flags.Cf
                      && ((FirstValue == 0xFF) || (SecondValue == 0xFF));

    /* Update the flags */
    State->Flags.Cf = State->Flags.Cf || ((Result < FirstValue) && (Result < SecondValue));
    State->Flags.Of = ((FirstValue & SIGN_FLAG_BYTE) == (SecondValue & SIGN_FLAG_BYTE))
                      && ((FirstValue & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         Opcode & SOFT386_OPCODE_WRITE_REG,
                                         Result);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAdcModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x11);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue + SecondValue + State->Flags.Cf;

        /* Special exception for CF */
        State->Flags.Cf = State->Flags.Cf
                          && ((FirstValue == 0xFFFFFFFF) || (SecondValue == 0xFFFFFFFF));

        /* Update the flags */
        State->Flags.Cf = State->Flags.Cf || ((Result < FirstValue) && (Result < SecondValue));
        State->Flags.Of = ((FirstValue & SIGN_FLAG_LONG) == (SecondValue & SIGN_FLAG_LONG))
                          && ((FirstValue & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
    else
    {
        USHORT FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Calculate the result */
        Result = FirstValue + SecondValue + State->Flags.Cf;

        /* Special exception for CF */
        State->Flags.Cf = State->Flags.Cf
                          && ((FirstValue == 0xFFFF) || (SecondValue == 0xFFFF));

        /* Update the flags */
        State->Flags.Cf = State->Flags.Cf || ((Result < FirstValue) && (Result < SecondValue));
        State->Flags.Of = ((FirstValue & SIGN_FLAG_WORD) == (SecondValue & SIGN_FLAG_WORD))
                          && ((FirstValue & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }

}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAdcAl)
{
    UCHAR FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    UCHAR SecondValue, Result;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x14);

    if (State->PrefixFlags)
    {
        /* This opcode doesn't take any prefixes */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (!Soft386FetchByte(State, &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue + SecondValue + State->Flags.Cf;

    /* Special exception for CF */
    State->Flags.Cf = State->Flags.Cf &&
                      ((FirstValue == 0xFF) || (SecondValue == 0xFF));

    /* Update the flags */
    State->Flags.Cf = State->Flags.Cf || ((Result < FirstValue) && (Result < SecondValue));
    State->Flags.Of = ((FirstValue & SIGN_FLAG_BYTE) == (SecondValue & SIGN_FLAG_BYTE))
                      && ((FirstValue & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Write back the result */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Result;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAdcEax)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x15);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG FirstValue = State->GeneralRegs[SOFT386_REG_EAX].Long;
        ULONG SecondValue, Result;

        if (!Soft386FetchDword(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue + SecondValue + State->Flags.Cf;

        /* Special exception for CF */
        State->Flags.Cf = State->Flags.Cf &&
                          ((FirstValue == 0xFFFFFFFF) || (SecondValue == 0xFFFFFFFF));

        /* Update the flags */
        State->Flags.Cf = State->Flags.Cf || ((Result < FirstValue) && (Result < SecondValue));
        State->Flags.Of = ((FirstValue & SIGN_FLAG_LONG) == (SecondValue & SIGN_FLAG_LONG))
                          && ((FirstValue & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].Long = Result;
    }
    else
    {
        USHORT FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        USHORT SecondValue, Result;

        if (!Soft386FetchWord(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue + SecondValue + State->Flags.Cf;

        /* Special exception for CF */
        State->Flags.Cf = State->Flags.Cf &&
                          ((FirstValue == 0xFFFF) || (SecondValue == 0xFFFF));

        /* Update the flags */
        State->Flags.Cf = State->Flags.Cf || ((Result < FirstValue) && (Result < SecondValue));
        State->Flags.Of = ((FirstValue & SIGN_FLAG_WORD) == (SecondValue & SIGN_FLAG_WORD))
                          && ((FirstValue & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = (((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) ? TRUE : FALSE;
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Result;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushSs)
{
    /* Call the internal API */
    return Soft386StackPush(State, State->SegmentRegs[SOFT386_REG_SS].Selector);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePopSs)
{
    ULONG NewSelector;

    if (!Soft386StackPop(State, &NewSelector))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Call the internal API */
    return Soft386LoadSegment(State, SOFT386_REG_SS, LOWORD(NewSelector));
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSbbByteModrm)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSbbModrm)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSbbAl)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSbbEax)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushDs)
{
    /* Call the internal API */
    return Soft386StackPush(State, State->SegmentRegs[SOFT386_REG_DS].Selector);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePopDs)
{
    ULONG NewSelector;

    if (!Soft386StackPop(State, &NewSelector))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Call the internal API */
    return Soft386LoadSegment(State, SOFT386_REG_DS, LOWORD(NewSelector));
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeDaa)
{
    UCHAR Value = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    BOOLEAN Carry = State->Flags.Cf;

    /* Clear the carry flag */
    State->Flags.Cf = FALSE;

    /* Check if the first BCD digit is invalid or there was a carry from it */
    if (((Value & 0x0F) > 9) || State->Flags.Af)
    {
        /* Correct it */
        State->GeneralRegs[SOFT386_REG_EAX].LowByte += 0x06;
        if (State->GeneralRegs[SOFT386_REG_EAX].LowByte < 0x06)
        {
            /* A carry occurred */
            State->Flags.Cf = TRUE;
        }

        /* Set the adjust flag */
        State->Flags.Af = TRUE;
    }

    /* Check if the second BCD digit is invalid or there was a carry from it */
    if ((Value > 0x99) || Carry)
    {
        /* Correct it */
        State->GeneralRegs[SOFT386_REG_EAX].LowByte += 0x60;

        /* There was a carry */
        State->Flags.Cf = TRUE;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeCmpSubByteModrm)
{
    UCHAR FirstValue, SecondValue, Result;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xED) == 0x28);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check if this is the instruction that writes to R/M */
    if (!(Opcode & SOFT386_OPCODE_WRITE_REG))
    {
        /* Swap the order */
        FirstValue ^= SecondValue;
        SecondValue ^= FirstValue;
        FirstValue ^= SecondValue;
    }

    /* Calculate the result */
    Result = FirstValue - SecondValue;

    /* Update the flags */
    State->Flags.Cf = FirstValue < SecondValue;
    State->Flags.Of = ((FirstValue & SIGN_FLAG_BYTE) != (SecondValue & SIGN_FLAG_BYTE))
                      && ((FirstValue & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = (FirstValue & 0x0F) < (SecondValue & 0x0F);
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Check if this is not a CMP */
    if (!(Opcode & 0x10))
    {
        /* Write back the result */
        return Soft386WriteModrmByteOperands(State,
                                             &ModRegRm,
                                             Opcode & SOFT386_OPCODE_WRITE_REG,
                                             Result);
    }
    else
    {
        /* Discard the result */
        return TRUE;
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeCmpSubModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xED) == 0x29);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Check if this is the instruction that writes to R/M */
        if (!(Opcode & SOFT386_OPCODE_WRITE_REG))
        {
            /* Swap the order */
            FirstValue ^= SecondValue;
            SecondValue ^= FirstValue;
            FirstValue ^= SecondValue;
        }
    
        /* Calculate the result */
        Result = FirstValue - SecondValue;

        /* Update the flags */
        State->Flags.Cf = FirstValue < SecondValue;
        State->Flags.Of = ((FirstValue & SIGN_FLAG_LONG) != (SecondValue & SIGN_FLAG_LONG))
                          && ((FirstValue & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = (FirstValue & 0x0F) < (SecondValue & 0x0F);
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Check if this is not a CMP */
        if (!(Opcode & 0x10))
        {
            /* Write back the result */
            return Soft386WriteModrmDwordOperands(State,
                                                  &ModRegRm,
                                                  Opcode & SOFT386_OPCODE_WRITE_REG,
                                                  Result);
        }
        else
        {
            /* Discard the result */
            return TRUE;
        }
    }
    else
    {
        USHORT FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        /* Check if this is the instruction that writes to R/M */
        if (!(Opcode & SOFT386_OPCODE_WRITE_REG))
        {
            /* Swap the order */
            FirstValue ^= SecondValue;
            SecondValue ^= FirstValue;
            FirstValue ^= SecondValue;
        }
    
        /* Calculate the result */
        Result = FirstValue - SecondValue;

        /* Update the flags */
        State->Flags.Cf = FirstValue < SecondValue;
        State->Flags.Of = ((FirstValue & SIGN_FLAG_WORD) != (SecondValue & SIGN_FLAG_WORD))
                          && ((FirstValue & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = (FirstValue & 0x0F) < (SecondValue & 0x0F);
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Check if this is not a CMP */
        if (!(Opcode & 0x10))
        {
            /* Write back the result */
            return Soft386WriteModrmWordOperands(State,
                                                 &ModRegRm,
                                                 Opcode & SOFT386_OPCODE_WRITE_REG,
                                                 Result);
        }
        else
        {
            /* Discard the result */
            return TRUE;
        }
    }

}

SOFT386_OPCODE_HANDLER(Soft386OpcodeCmpSubAl)
{
    UCHAR FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    UCHAR SecondValue, Result;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xEF) == 0x2C);

    if (State->PrefixFlags)
    {
        /* This opcode doesn't take any prefixes */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (!Soft386FetchByte(State, &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Calculate the result */
    Result = FirstValue - SecondValue;

    /* Update the flags */
    State->Flags.Cf = FirstValue < SecondValue;
    State->Flags.Of = ((FirstValue & SIGN_FLAG_BYTE) != (SecondValue & SIGN_FLAG_BYTE))
                      && ((FirstValue & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
    State->Flags.Af = (FirstValue & 0x0F) < (SecondValue & 0x0F);
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Check if this is not a CMP */
    if (!(Opcode & 0x10))
    {
        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].LowByte = Result;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeCmpSubEax)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xEF) == 0x2D);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG FirstValue = State->GeneralRegs[SOFT386_REG_EAX].Long;
        ULONG SecondValue, Result;

        if (!Soft386FetchDword(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue - SecondValue;

        /* Update the flags */
        State->Flags.Cf = FirstValue < SecondValue;
        State->Flags.Of = ((FirstValue & SIGN_FLAG_LONG) != (SecondValue & SIGN_FLAG_LONG))
                          && ((FirstValue & SIGN_FLAG_LONG) != (Result & SIGN_FLAG_LONG));
        State->Flags.Af = (FirstValue & 0x0F) < (SecondValue & 0x0F);
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_LONG) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Check if this is not a CMP */
        if (!(Opcode & 0x10))
        {
            /* Write back the result */
            State->GeneralRegs[SOFT386_REG_EAX].Long = Result;
        }
    }
    else
    {
        USHORT FirstValue = State->GeneralRegs[SOFT386_REG_EAX].LowWord;
        USHORT SecondValue, Result;

        if (!Soft386FetchWord(State, &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Calculate the result */
        Result = FirstValue - SecondValue;

        /* Update the flags */
        State->Flags.Cf = FirstValue < SecondValue;
        State->Flags.Of = ((FirstValue & SIGN_FLAG_WORD) != (SecondValue & SIGN_FLAG_WORD))
                          && ((FirstValue & SIGN_FLAG_WORD) != (Result & SIGN_FLAG_WORD));
        State->Flags.Af = (FirstValue & 0x0F) < (SecondValue & 0x0F);
        State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
        State->Flags.Sf = (Result & SIGN_FLAG_WORD) ? TRUE : FALSE;
        State->Flags.Pf = Soft386CalculateParity(Result);

        /* Write back the result */
        State->GeneralRegs[SOFT386_REG_EAX].LowWord = Result;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeDas)
{
    UCHAR Value = State->GeneralRegs[SOFT386_REG_EAX].LowByte;
    BOOLEAN Carry = State->Flags.Cf;

    /* Clear the carry flag */
    State->Flags.Cf = FALSE;

    /* Check if the first BCD digit is invalid or there was a borrow */
    if (((Value & 0x0F) > 9) || State->Flags.Af)
    {
        /* Correct it */
        State->GeneralRegs[SOFT386_REG_EAX].LowByte -= 0x06;
        if (State->GeneralRegs[SOFT386_REG_EAX].LowByte > 0xFB)
        {
            /* A borrow occurred */
            State->Flags.Cf = TRUE;
        }

        /* Set the adjust flag */
        State->Flags.Af = TRUE;
    }

    /* Check if the second BCD digit is invalid or there was a borrow */
    if ((Value > 0x99) || Carry)
    {
        /* Correct it */
        State->GeneralRegs[SOFT386_REG_EAX].LowByte -= 0x60;

        /* There was a borrow */
        State->Flags.Cf = TRUE;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAaa)
{
    UCHAR Value = State->GeneralRegs[SOFT386_REG_EAX].LowByte;

    /*
     * Check if the value in AL is not a valid BCD digit,
     * or there was a carry from the lowest 4 bits of AL
     */
    if (((Value & 0x0F) > 9) || State->Flags.Af)
    {
        /* Correct it */
        State->GeneralRegs[SOFT386_REG_EAX].LowByte += 0x06;
        State->GeneralRegs[SOFT386_REG_EAX].HighByte++;

        /* Set CF and AF */
        State->Flags.Cf = State->Flags.Af = TRUE;
    }
    else
    {
        /* Clear CF and AF */
        State->Flags.Cf = State->Flags.Af = FALSE;
    }

    /* Keep only the lowest 4 bits of AL */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte &= 0x0F;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAas)
{
    UCHAR Value = State->GeneralRegs[SOFT386_REG_EAX].LowByte;

    /*
     * Check if the value in AL is not a valid BCD digit,
     * or there was a borrow from the lowest 4 bits of AL
     */
    if (((Value & 0x0F) > 9) || State->Flags.Af)
    {
        /* Correct it */
        State->GeneralRegs[SOFT386_REG_EAX].LowByte -= 0x06;
        State->GeneralRegs[SOFT386_REG_EAX].HighByte--;

        /* Set CF and AF */
        State->Flags.Cf = State->Flags.Af = TRUE;
    }
    else
    {
        /* Clear CF and AF */
        State->Flags.Cf = State->Flags.Af = FALSE;
    }

    /* Keep only the lowest 4 bits of AL */
    State->GeneralRegs[SOFT386_REG_EAX].LowByte &= 0x0F;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushAll)
{
    INT i;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;
    SOFT386_REG SavedEsp = State->GeneralRegs[SOFT386_REG_ESP];

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x60);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Push all the registers in order */
    for (i = 0; i < SOFT386_NUM_GEN_REGS; i++)
    {
        if (i == SOFT386_REG_ESP)
        {
            /* Use the saved ESP instead */
            if (!Soft386StackPush(State, Size ? SavedEsp.Long : SavedEsp.LowWord))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
        else
        {
            /* Push the register */
            if (!Soft386StackPush(State, Size ? State->GeneralRegs[i].Long
                                              : State->GeneralRegs[i].LowWord))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePopAll)
{
    INT i;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;
    ULONG Value;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x61);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Pop all the registers in reverse order */
    for (i = SOFT386_NUM_GEN_REGS - 1; i >= 0; i--)
    {
        /* Pop the value */
        if (!Soft386StackPop(State, &Value))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Don't modify ESP */
        if (i != SOFT386_REG_ESP)
        {
            if (Size) State->GeneralRegs[i].Long = Value;
            else State->GeneralRegs[i].LowWord = LOWORD(Value);
        }
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeBound)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeArpl)
{
    USHORT FirstValue, SecondValue;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (!(State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
        || State->Flags.Vm
        || (State->PrefixFlags & SOFT386_PREFIX_LOCK))
    {
        /* Cannot be used in real mode or with a LOCK prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Read the operands */
    if (!Soft386ReadModrmWordOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check if the RPL needs adjusting */
    if ((SecondValue & 3) < (FirstValue & 3))
    {
        /* Adjust the RPL */
        SecondValue &= ~3;
        SecondValue |= FirstValue & 3;

        /* Set ZF */
        State->Flags.Zf = TRUE;

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State, &ModRegRm, FALSE, SecondValue);
    }
    else
    {
        /* Clear ZF */
        State->Flags.Zf = FALSE;
        return TRUE;
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushImm)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x68);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG Data;

        if (!Soft386FetchDword(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Call the internal API */
        return Soft386StackPush(State, Data);
    }
    else
    {
        USHORT Data;

        if (!Soft386FetchWord(State, &Data))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Call the internal API */
        return Soft386StackPush(State, Data);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeImulModrmImm)
{
    BOOLEAN OperandSize, AddressSize;
    SOFT386_MOD_REG_RM ModRegRm;
    LONG Multiplier;
    LONGLONG Product;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x69);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    /* Fetch the parameters */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (Opcode == 0x6B)
    {
        CHAR Byte;

        /* Fetch the immediate operand */
        if (!Soft386FetchByte(State, (PUCHAR)&Byte))
        {
            /* Exception occurred */
            return FALSE;
        }

        Multiplier = (LONG)Byte;
    }
    else
    {
        if (OperandSize)
        {
            LONG Dword;

            /* Fetch the immediate operand */
            if (!Soft386FetchDword(State, (PULONG)&Dword))
            {
                /* Exception occurred */
                return FALSE;
            }

            Multiplier = Dword;
        }
        else
        {
            SHORT Word;

            /* Fetch the immediate operand */
            if (!Soft386FetchWord(State, (PUSHORT)&Word))
            {
                /* Exception occurred */
                return FALSE;
            }

            Multiplier = (LONG)Word;
        }
    }

    if (OperandSize)
    {
        LONG RegValue, Multiplicand;

        /* Read the operands */
        if (!Soft386ReadModrmDwordOperands(State,
                                           &ModRegRm,
                                           (PULONG)&RegValue,
                                           (PULONG)&Multiplicand))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Multiply */
        Product = (LONGLONG)Multiplicand * (LONGLONG)Multiplier;
    }
    else
    {
        SHORT RegValue, Multiplicand;

        /* Read the operands */
        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          (PUSHORT)&RegValue,
                                          (PUSHORT)&Multiplicand))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Multiply */
        Product = (LONGLONG)Multiplicand * (LONGLONG)Multiplier;
    }

    /* Check for carry/overflow */
    if ((Product < LONG_MIN) || (Product > LONG_MAX))
    {
        State->Flags.Cf = State->Flags.Of = TRUE;
    }
    else State->Flags.Cf = State->Flags.Of = FALSE;

    /* Write-back the result */
    return Soft386WriteModrmDwordOperands(State,
                                          &ModRegRm,
                                          TRUE,
                                          (ULONG)((LONG)Product));
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushByteImm)
{
    UCHAR Data;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x6A);

    if (!Soft386FetchByte(State, &Data))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Call the internal API */
    return Soft386StackPush(State, Data);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovByteModrm)
{
    UCHAR FirstValue, SecondValue, Result;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x88);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }
    else if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386ReadModrmByteOperands(State,
                                      &ModRegRm,
                                      &FirstValue,
                                      &SecondValue))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (Opcode & SOFT386_OPCODE_WRITE_REG) Result = SecondValue;
    else Result = FirstValue;

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         Opcode & SOFT386_OPCODE_WRITE_REG,
                                         Result);

}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovModrm)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFD) == 0x89);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags
             & ~(SOFT386_PREFIX_ADSIZE
             | SOFT386_PREFIX_OPSIZE
             | SOFT386_PREFIX_SEG
             | SOFT386_PREFIX_LOCK))
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check the operand size */
    if (OperandSize)
    {
        ULONG FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmDwordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }

        if (Opcode & SOFT386_OPCODE_WRITE_REG) Result = SecondValue;
        else Result = FirstValue;
    
        /* Write back the result */
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              Opcode & SOFT386_OPCODE_WRITE_REG,
                                              Result);
    }
    else
    {
        USHORT FirstValue, SecondValue, Result;

        if (!Soft386ReadModrmWordOperands(State,
                                          &ModRegRm,
                                          &FirstValue,
                                          &SecondValue))
        {
            /* Exception occurred */
            return FALSE;
        }
    
        if (Opcode & SOFT386_OPCODE_WRITE_REG) Result = SecondValue;
        else Result = FirstValue;

        /* Write back the result */
        return Soft386WriteModrmWordOperands(State,
                                             &ModRegRm,
                                             Opcode & SOFT386_OPCODE_WRITE_REG,
                                             Result);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovStoreSeg)
{
    BOOLEAN OperandSize, AddressSize;
    SOFT386_MOD_REG_RM ModRegRm;

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x8C);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register >= SOFT386_NUM_SEG_REGS)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (OperandSize)
    {
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              FALSE,
                                              State->SegmentRegs[ModRegRm.Register].Selector);
    }
    else
    {
        return Soft386WriteModrmWordOperands(State,
                                             &ModRegRm,
                                             FALSE,
                                             State->SegmentRegs[ModRegRm.Register].Selector);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeLea)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x8D);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* The second operand must be memory */
    if (!ModRegRm.Memory)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Write the address to the register */
    if (OperandSize)
    {
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              TRUE,
                                              ModRegRm.MemoryAddress);
    }
    else
    {
        return Soft386WriteModrmWordOperands(State,
                                             &ModRegRm,
                                             TRUE,
                                             ModRegRm.MemoryAddress);

    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovLoadSeg)
{
    BOOLEAN OperandSize, AddressSize;
    SOFT386_MOD_REG_RM ModRegRm;

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x8E);

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the address size */
        AddressSize = !AddressSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the operand size */
        OperandSize = !OperandSize;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if ((ModRegRm.Register >= SOFT386_NUM_SEG_REGS)
        || ((SOFT386_SEG_REGS)ModRegRm.Register == SOFT386_REG_CS))
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (OperandSize)
    {
        ULONG Dummy, Selector;

        if (!Soft386ReadModrmDwordOperands(State, &ModRegRm, &Dummy, &Selector))
        {
            /* Exception occurred */
            return FALSE;
        }

        return Soft386LoadSegment(State, ModRegRm.Register, LOWORD(Selector));
    }
    else
    {
        USHORT Dummy, Selector;

        if (!Soft386ReadModrmWordOperands(State, &ModRegRm, &Dummy, &Selector))
        {
            /* Exception occurred */
            return FALSE;
        }

        return Soft386LoadSegment(State, ModRegRm.Register, Selector);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeCwde)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x98);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Size)
    {
        /* Sign extend AX to EAX */
        State->GeneralRegs[SOFT386_REG_EAX].Long = MAKELONG
        (
            State->GeneralRegs[SOFT386_REG_EAX].LowWord,
            (State->GeneralRegs[SOFT386_REG_EAX].LowWord & SIGN_FLAG_WORD)
            ? 0xFFFF : 0x0000
        );
    }
    else
    {
        /* Sign extend AL to AX */
        State->GeneralRegs[SOFT386_REG_EAX].HighByte =
        (State->GeneralRegs[SOFT386_REG_EAX].LowByte & SIGN_FLAG_BYTE)
        ? 0xFF : 0x00;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeCdq)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x99);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Size)
    {
        /* Sign extend EAX to EDX:EAX */
        State->GeneralRegs[SOFT386_REG_EDX].Long =
        (State->GeneralRegs[SOFT386_REG_EAX].Long & SIGN_FLAG_LONG)
        ? 0xFFFFFFFF : 0x00000000;
    }
    else
    {
        /* Sign extend AX to DX:AX */
        State->GeneralRegs[SOFT386_REG_EDX].LowWord =
        (State->GeneralRegs[SOFT386_REG_EAX].LowWord & SIGN_FLAG_WORD)
        ? 0xFFFF : 0x0000;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeCallAbs)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeWait)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePushFlags)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* This OPSIZE prefix toggles the size */
        Size = !Size;
    }

    /* Check for VM86 mode when IOPL is not 3 */
    if (State->Flags.Vm && (State->Flags.Iopl != 3))
    {
        /* Call the VM86 monitor */
        Soft386ExceptionWithErrorCode(State, SOFT386_EXCEPTION_GP, 0);
        return FALSE;
    }

    /* Push the flags */
    if (Size) return Soft386StackPush(State, State->Flags.Long);
    else return Soft386StackPush(State, LOWORD(State->Flags.Long));
}

SOFT386_OPCODE_HANDLER(Soft386OpcodePopFlags)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;
    INT Cpl = Soft386GetCurrentPrivLevel(State);
    ULONG NewFlags;

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* This OPSIZE prefix toggles the size */
        Size = !Size;
    }

    /* Pop the new flags */
    if (!Soft386StackPop(State, &NewFlags))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!State->Flags.Vm)
    {
        /* Check the current privilege level */
        if (Cpl == 0)
        {
            /* Supervisor */

            /* Set the flags */
            if (Size)
            {
                /* Memorize the old state of RF */
                BOOLEAN OldRf = State->Flags.Rf;

                State->Flags.Long = NewFlags;

                /* Restore VM and RF */
                State->Flags.Vm = FALSE;
                State->Flags.Rf = OldRf;

                /* Clear VIF and VIP */
                State->Flags.Vif = State->Flags.Vip = FALSE;
            }
            else State->Flags.LowWord = LOWORD(NewFlags);

            /* Restore the reserved bits */
            State->Flags.AlwaysSet = TRUE;
            State->Flags.Reserved0 = FALSE;
            State->Flags.Reserved1 = FALSE;
        }
        else
        {
            /* User */

            /* Memorize the old state of IF and IOPL */
            BOOLEAN OldIf = State->Flags.If;
            UINT OldIopl = State->Flags.Iopl;

            /* Set the flags */
            if (Size)
            {
                /* Memorize the old state of RF */
                BOOLEAN OldRf = State->Flags.Rf;

                State->Flags.Long = NewFlags;

                /* Restore VM and RF */
                State->Flags.Vm = FALSE;
                State->Flags.Rf = OldRf;

                /* Clear VIF and VIP */
                State->Flags.Vif = State->Flags.Vip = FALSE;
            }
            else State->Flags.LowWord = LOWORD(NewFlags);

            /* Restore the reserved bits and IOPL */
            State->Flags.AlwaysSet = TRUE;
            State->Flags.Reserved0 = FALSE;
            State->Flags.Reserved1 = FALSE;
            State->Flags.Iopl = OldIopl;

            /* Check if the user doesn't have the privilege to change IF */
            if (Cpl > State->Flags.Iopl)
            {
                /* Restore IF */
                State->Flags.If = OldIf;
            }
        }
    }
    else
    {
        /* Check the IOPL */
        if (State->Flags.Iopl == 3)
        {
            if (Size)
            {
                /* Memorize the old state of RF, VIF and VIP */
                BOOLEAN OldRf = State->Flags.Rf;
                BOOLEAN OldVif = State->Flags.Vif;
                BOOLEAN OldVip = State->Flags.Vip;

                State->Flags.Long = NewFlags;

                /* Restore VM, RF, VIF and VIP */
                State->Flags.Vm = TRUE;
                State->Flags.Rf = OldRf;
                State->Flags.Vif = OldVif;
                State->Flags.Vip = OldVip;
            }
            else State->Flags.LowWord = LOWORD(NewFlags);

            /* Restore the reserved bits and IOPL */
            State->Flags.AlwaysSet = TRUE;
            State->Flags.Reserved0 = FALSE;
            State->Flags.Reserved1 = FALSE;
            State->Flags.Iopl = 3;
        }
        else
        {
            /* Call the VM86 monitor */
            Soft386ExceptionWithErrorCode(State, SOFT386_EXCEPTION_GP, 0);
        }
        
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeSahf)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x9E);

    /* Set the low-order byte of FLAGS to AH */
    State->Flags.Long &= 0xFFFFFF00;
    State->Flags.Long |= State->GeneralRegs[SOFT386_REG_EAX].HighByte;

    /* Restore the reserved bits of FLAGS */
    State->Flags.AlwaysSet = TRUE;
    State->Flags.Reserved0 = State->Flags.Reserved1 = FALSE;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeLahf)
{
    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0x9F);

    /* Set AH to the low-order byte of FLAGS */
    State->GeneralRegs[SOFT386_REG_EAX].HighByte = LOBYTE(State->Flags.Long);

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeRet)
{
    ULONG ReturnAddress;
    USHORT BytesToPop = 0;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFE) == 0xC2);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Opcode == 0xC2)
    {
        /* Fetch the number of bytes to pop after the return */
        if (!Soft386FetchWord(State, &BytesToPop)) return FALSE;
    }

    /* Pop the return address */
    if (!Soft386StackPop(State, &ReturnAddress)) return FALSE;

    /* Return to the calling procedure, and if necessary, pop the parameters */
    if (Size)
    {
        State->InstPtr.Long = ReturnAddress;
        State->GeneralRegs[SOFT386_REG_ESP].Long += BytesToPop;
    }
    else
    {
        State->InstPtr.LowWord = LOWORD(ReturnAddress);
        State->GeneralRegs[SOFT386_REG_ESP].LowWord += BytesToPop;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeLdsLes)
{
    UCHAR FarPointer[6];
    BOOLEAN OperandSize, AddressSize;
    SOFT386_MOD_REG_RM ModRegRm;

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xFE) == 0xC4);

    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    /* Get the operands */
    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!ModRegRm.Memory)
    {
        /* Check if this is a BOP and the host supports BOPs */
        if ((Opcode == 0xC4)
            && (ModRegRm.Register == SOFT386_REG_EAX)
            && (ModRegRm.SecondRegister == SOFT386_REG_ESP)
            && (State->BopCallback != NULL))
        {
            USHORT BopCode;

            /* Fetch the BOP code */
            if (!Soft386FetchWord(State, &BopCode))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Call the BOP handler */
            State->BopCallback(State, BopCode);

            /* Return success */
            return TRUE;
        }

        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (!Soft386ReadMemory(State,
                           (State->PrefixFlags & SOFT386_PREFIX_SEG)
                           ? State->SegmentOverride : SOFT386_REG_DS,
                           ModRegRm.MemoryAddress,
                           FALSE,
                           FarPointer,
                           OperandSize ? 6 : 4))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (OperandSize)
    {
        ULONG Offset = *((PULONG)FarPointer);
        USHORT Segment = *((PUSHORT)&FarPointer[sizeof(ULONG)]);

        /* Set the register to the offset */
        State->GeneralRegs[ModRegRm.Register].Long = Offset;

        /* Load the segment */
        return Soft386LoadSegment(State,
                                  (Opcode == 0xC4)
                                  ? SOFT386_REG_ES : SOFT386_REG_DS,
                                  Segment);
    }
    else
    {
        USHORT Offset = *((PUSHORT)FarPointer);
        USHORT Segment = *((PUSHORT)&FarPointer[sizeof(USHORT)]);

        /* Set the register to the offset */
        State->GeneralRegs[ModRegRm.Register].LowWord = Offset;

        /* Load the segment */
        return Soft386LoadSegment(State,
                                  (Opcode == 0xC4)
                                  ? SOFT386_REG_ES : SOFT386_REG_DS,
                                  Segment);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeEnter)
{
    INT i;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;
    USHORT FrameSize;
    UCHAR NestingLevel;
    SOFT386_REG FramePointer;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xC8);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (!Soft386FetchWord(State, &FrameSize))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Soft386FetchByte(State, &NestingLevel))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Push EBP */
    if (!Soft386StackPush(State, State->GeneralRegs[SOFT386_REG_EBP].Long))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Save ESP */
    FramePointer = State->GeneralRegs[SOFT386_REG_ESP];

    /* Set up the nested procedure stacks */
    for (i = 1; i < NestingLevel; i++)
    {
        if (Size)
        {
            State->GeneralRegs[SOFT386_REG_EBP].Long -= 4;
            Soft386StackPush(State, State->GeneralRegs[SOFT386_REG_EBP].Long);
        }
        else
        {
            State->GeneralRegs[SOFT386_REG_EBP].LowWord -= 2;
            Soft386StackPush(State, State->GeneralRegs[SOFT386_REG_EBP].LowWord);
        }
    }

    if (NestingLevel > 0) Soft386StackPush(State, FramePointer.Long);

    /* Set EBP to the frame pointer */
    State->GeneralRegs[SOFT386_REG_EBP] = FramePointer;

    /* Reserve space for the frame */
    if (Size) State->GeneralRegs[SOFT386_REG_ESP].Long -= (ULONG)FrameSize;
    else State->GeneralRegs[SOFT386_REG_ESP].LowWord -= FrameSize;

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeLeave)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xC9);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        /* Set the stack pointer (ESP) to the base pointer (EBP) */
        State->GeneralRegs[SOFT386_REG_ESP].Long = State->GeneralRegs[SOFT386_REG_EBP].Long;

        /* Pop the saved base pointer from the stack */
        return Soft386StackPop(State, &State->GeneralRegs[SOFT386_REG_EBP].Long);
    }
    else
    {
        ULONG Value;

        /* Set the stack pointer (SP) to the base pointer (BP) */
        State->GeneralRegs[SOFT386_REG_ESP].LowWord = State->GeneralRegs[SOFT386_REG_EBP].LowWord;

        /* Pop the saved base pointer from the stack */
        if (Soft386StackPop(State, &Value))
        {
            State->GeneralRegs[SOFT386_REG_EBP].LowWord = LOWORD(Value);
            return TRUE;
        }
        else return FALSE;
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeRetFarImm)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeRetFar)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeInt)
{
    UCHAR IntNum;
    SOFT386_IDT_ENTRY IdtEntry;

    switch (Opcode)
    {
        case 0xCC:
        {
            /* This is the INT3 instruction */
            IntNum = 3;
            break;
        }

        case 0xCD:
        {
            /* Fetch the interrupt number */
            if (!Soft386FetchByte(State, &IntNum))
            {
                /* Exception occurred */
                return FALSE;
            }

            break;
        }

        case 0xCE:
        {
            /* Don't do anything if OF is cleared */
            if (!State->Flags.Of) return TRUE;

            /* Exception #OF */
            IntNum = SOFT386_EXCEPTION_OF;

            break;
        }

        default:
        {
            /* Should not happen */
            ASSERT(FALSE);
        }
    }

    /* Get the interrupt vector */
    if (!Soft386GetIntVector(State, IntNum, &IdtEntry))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Perform the interrupt */
    if (!Soft386InterruptInternal(State,
                                  IdtEntry.Selector,
                                  MAKELONG(IdtEntry.Offset, IdtEntry.OffsetHigh),
                                  IdtEntry.Type))
    {
        /* Exception occurred */
        return FALSE;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeIret)
{
    INT i;
    ULONG InstPtr, CodeSel, StackPtr, StackSel;
    SOFT386_FLAGS_REG NewFlags;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xCF);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    /* Pop EIP */
    if (!Soft386StackPop(State, &InstPtr))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Pop CS */
    if (!Soft386StackPop(State, &CodeSel))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Pop EFLAGS */
    if (!Soft386StackPop(State, &NewFlags.Long))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        INT Cpl = Soft386GetCurrentPrivLevel(State);

        if (State->Flags.Vm)
        {
            /* Return from VM86 mode */

            /* Check the IOPL */
            if (State->Flags.Iopl == 3)
            {
                /* Set new EIP */
                State->InstPtr.Long = LOWORD(InstPtr);

                /* Load new CS */
                if (!Soft386LoadSegment(State, SOFT386_REG_CS, CodeSel))
                {
                    /* Exception occurred */
                    return FALSE;
                }

                /* Set the new flags */
                if (Size) State->Flags.Long = NewFlags.Long & REAL_MODE_FLAGS_MASK;
                else State->Flags.LowWord = NewFlags.LowWord & REAL_MODE_FLAGS_MASK;
                State->Flags.AlwaysSet = State->Flags.Vm = TRUE;
                State->Flags.Iopl = 3;
            }
            else
            {
                /* Call the VM86 monitor */
                Soft386ExceptionWithErrorCode(State, SOFT386_EXCEPTION_GP, 0);
                return FALSE;
            }

            return TRUE;
        }

        if (State->Flags.Nt)
        {
            /* Nested task return */

            UNIMPLEMENTED;
            return FALSE;
        }

        if (NewFlags.Vm)
        {
            /* Return to VM86 mode */
            ULONG Es, Ds, Fs, Gs;

            /* Pop ESP, SS, ES, FS, GS */
            if (!Soft386StackPop(State, &StackPtr)) return FALSE;
            if (!Soft386StackPop(State, &StackSel)) return FALSE;
            if (!Soft386StackPop(State, &Es)) return FALSE;
            if (!Soft386StackPop(State, &Ds)) return FALSE;
            if (!Soft386StackPop(State, &Fs)) return FALSE;
            if (!Soft386StackPop(State, &Gs)) return FALSE;

            /* Set the new IP */
            State->InstPtr.Long = LOWORD(InstPtr);

            /* Set the new flags */
            if (Size) State->Flags.Long = NewFlags.Long & REAL_MODE_FLAGS_MASK;
            else State->Flags.LowWord = NewFlags.LowWord & REAL_MODE_FLAGS_MASK;
            State->Flags.AlwaysSet = State->Flags.Vm = TRUE;

            /* Load the new segments */
            if (!Soft386LoadSegment(State, SOFT386_REG_CS, CodeSel)) return FALSE;
            if (!Soft386LoadSegment(State, SOFT386_REG_SS, StackSel)) return FALSE;
            if (!Soft386LoadSegment(State, SOFT386_REG_ES, Es)) return FALSE;
            if (!Soft386LoadSegment(State, SOFT386_REG_DS, Ds)) return FALSE;
            if (!Soft386LoadSegment(State, SOFT386_REG_FS, Fs)) return FALSE;
            if (!Soft386LoadSegment(State, SOFT386_REG_GS, Gs)) return FALSE;

            return TRUE;
        }

        /* Load the new CS */
        if (!Soft386LoadSegment(State, SOFT386_REG_CS, CodeSel))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set EIP */
        if (Size) State->InstPtr.Long = InstPtr;
        else State->InstPtr.LowWord = LOWORD(InstPtr);

        if (GET_SEGMENT_RPL(CodeSel) > Cpl)
        {
            /* Pop ESP */
            if (!Soft386StackPop(State, &StackPtr))
            {
                /* Exception */
                return FALSE;
            }

            /* Pop SS */
            if (!Soft386StackPop(State, &StackSel))
            {
                /* Exception */
                return FALSE;
            }

            /* Load new SS */
            if (!Soft386LoadSegment(State, SOFT386_REG_SS, StackSel))
            {
                /* Exception */
                return FALSE;
            }

            /* Set ESP */
            if (Size) State->GeneralRegs[SOFT386_REG_ESP].Long = StackPtr;
            else State->GeneralRegs[SOFT386_REG_ESP].LowWord = LOWORD(StackPtr);
        }

        /* Set the new flags */
        if (Size) State->Flags.Long = NewFlags.Long & PROT_MODE_FLAGS_MASK;
        else State->Flags.LowWord = NewFlags.LowWord & PROT_MODE_FLAGS_MASK;
        State->Flags.AlwaysSet = TRUE;

        /* Set additional flags */
        if (Cpl <= State->Flags.Iopl) State->Flags.If = NewFlags.If;
        if (Cpl == 0) State->Flags.Iopl = NewFlags.Iopl;

        if (GET_SEGMENT_RPL(CodeSel) > Cpl)
        {
            /* Update the CPL */
            Cpl = Soft386GetCurrentPrivLevel(State);

            /* Check segment security */
            for (i = 0; i <= SOFT386_NUM_SEG_REGS; i++)
            {
                /* Don't check CS or SS */
                if ((i == SOFT386_REG_CS) || (i == SOFT386_REG_SS)) continue;

                if ((Cpl > State->SegmentRegs[i].Dpl)
                    && (!State->SegmentRegs[i].Executable
                    || !State->SegmentRegs[i].DirConf))
                {
                    /* Load the NULL descriptor in the segment */
                    if (!Soft386LoadSegment(State, i, 0)) return FALSE;
                }
            }
        }
    }
    else
    {
        if (Size && (InstPtr & 0xFFFF0000))
        {
            /* Invalid */
            Soft386ExceptionWithErrorCode(State, SOFT386_EXCEPTION_GP, 0);
            return FALSE;
        }

        /* Set new EIP */
        State->InstPtr.Long = InstPtr;

        /* Load new CS */
        if (!Soft386LoadSegment(State, SOFT386_REG_CS, CodeSel))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Set the new flags */
        if (Size) State->Flags.Long = NewFlags.Long & REAL_MODE_FLAGS_MASK;
        else State->Flags.LowWord = NewFlags.LowWord & REAL_MODE_FLAGS_MASK;
        State->Flags.AlwaysSet = TRUE;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAam)
{
    UCHAR Base;
    UCHAR Value = State->GeneralRegs[SOFT386_REG_EAX].LowByte;

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Fetch the base */
    if (!Soft386FetchByte(State, &Base))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check if the base is zero */
    if (Base == 0)
    {
        /* Divide error */
        Soft386Exception(State, SOFT386_EXCEPTION_DE);
        return FALSE;
    }

    /* Adjust */
    State->GeneralRegs[SOFT386_REG_EAX].HighByte = Value / Base;
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Value %= Base;

    /* Update flags */
    State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Value & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Value);

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeAad)
{
    UCHAR Base;
    UCHAR Value = State->GeneralRegs[SOFT386_REG_EAX].LowByte;

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Fetch the base */
    if (!Soft386FetchByte(State, &Base))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Adjust */
    Value += State->GeneralRegs[SOFT386_REG_EAX].HighByte * Base;
    State->GeneralRegs[SOFT386_REG_EAX].LowByte = Value;

    /* Update flags */
    State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Value & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Value);

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeXlat)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeLoop)
{
    BOOLEAN Condition;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;
    CHAR Offset = 0;

    /* Make sure this is the right instruction */
    ASSERT((Opcode >= 0xE0) && (Opcode <= 0xE2));

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size) Condition = ((--State->GeneralRegs[SOFT386_REG_ECX].Long) != 0);
    else Condition = ((--State->GeneralRegs[SOFT386_REG_ECX].LowWord) != 0);

    if (Opcode == 0xE0)
    {
        /* Additional rule for LOOPNZ */
        if (State->Flags.Zf) Condition = FALSE;
    }

    if (Opcode == 0xE1)
    {
        /* Additional rule for LOOPZ */
        if (!State->Flags.Zf) Condition = FALSE;
    }

    /* Fetch the offset */
    if (!Soft386FetchByte(State, (PUCHAR)&Offset))
    {
        /* An exception occurred */
        return FALSE;
    }

    if (Condition)
    {
        /* Move the instruction pointer */
        State->InstPtr.Long += Offset;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeJecxz)
{
    BOOLEAN Condition;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;
    CHAR Offset = 0;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xE3);

    if (State->PrefixFlags & SOFT386_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size) Condition = (State->GeneralRegs[SOFT386_REG_ECX].Long == 0);
    else Condition = (State->GeneralRegs[SOFT386_REG_ECX].LowWord == 0);

    /* Fetch the offset */
    if (!Soft386FetchByte(State, (PUCHAR)&Offset))
    {
        /* An exception occurred */
        return FALSE;
    }

    if (Condition)
    {
        /* Move the instruction pointer */        
        State->InstPtr.Long += Offset;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeCall)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xE8);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Size)
    {
        LONG Offset = 0;

        /* Fetch the offset */
        if (!Soft386FetchDword(State, (PULONG)&Offset))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Push the current value of the instruction pointer */
        if (!Soft386StackPush(State, State->InstPtr.Long))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Move the instruction pointer */        
        State->InstPtr.Long += Offset;
    }
    else
    {
        SHORT Offset = 0;

        /* Fetch the offset */
        if (!Soft386FetchWord(State, (PUSHORT)&Offset))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Push the current value of the instruction pointer */
        if (!Soft386StackPush(State, State->InstPtr.Long))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Move the instruction pointer */        
        State->InstPtr.LowWord += Offset;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeJmp)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xE9);

    if (State->PrefixFlags == SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }
    else if (State->PrefixFlags != 0)
    {
        /* Invalid prefix */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (Size)
    {
        LONG Offset = 0;

        /* Fetch the offset */
        if (!Soft386FetchDword(State, (PULONG)&Offset))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Move the instruction pointer */        
        State->InstPtr.Long += Offset;
    }
    else
    {
        SHORT Offset = 0;

        /* Fetch the offset */
        if (!Soft386FetchWord(State, (PUSHORT)&Offset))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Move the instruction pointer */        
        State->InstPtr.LowWord += Offset;
    }

    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeJmpAbs)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return FALSE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovAlOffset)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;
    ULONG Offset;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xA0);

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        if (!Soft386FetchDword(State, &Offset))
        {
            /* Exception occurred */
            return FALSE;
        }
    }
    else
    {
        USHORT WordOffset;

        if (!Soft386FetchWord(State, &WordOffset))
        {
            /* Exception occurred */
            return FALSE;
        }

        Offset = (ULONG)WordOffset;
    }

    /* Read from memory */
    return Soft386ReadMemory(State,
                             (State->PrefixFlags & SOFT386_PREFIX_SEG) ?
                             State->SegmentOverride : SOFT386_REG_DS,
                             Offset,
                             FALSE,
                             &State->GeneralRegs[SOFT386_REG_EAX].LowByte,
                             sizeof(UCHAR));
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovEaxOffset)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xA1);

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG Offset;

        if (!Soft386FetchDword(State, &Offset))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Read from memory */
        return Soft386ReadMemory(State,
                                 (State->PrefixFlags & SOFT386_PREFIX_SEG) ?
                                 State->SegmentOverride : SOFT386_REG_DS,
                                 Offset,
                                 FALSE,
                                 &State->GeneralRegs[SOFT386_REG_EAX].Long,
                                 sizeof(ULONG));
    }
    else
    {
        USHORT Offset;

        if (!Soft386FetchWord(State, &Offset))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Read from memory */
        return Soft386ReadMemory(State,
                                 (State->PrefixFlags & SOFT386_PREFIX_SEG) ?
                                 State->SegmentOverride : SOFT386_REG_DS,
                                 Offset,
                                 FALSE,
                                 &State->GeneralRegs[SOFT386_REG_EAX].LowWord,
                                 sizeof(USHORT));
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovOffsetAl)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;
    ULONG Offset;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xA2);

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        if (!Soft386FetchDword(State, &Offset))
        {
            /* Exception occurred */
            return FALSE;
        }
    }
    else
    {
        USHORT WordOffset;

        if (!Soft386FetchWord(State, &WordOffset))
        {
            /* Exception occurred */
            return FALSE;
        }

        Offset = (ULONG)WordOffset;
    }

    /* Write to memory */
    return Soft386WriteMemory(State,
                             (State->PrefixFlags & SOFT386_PREFIX_SEG) ?
                             State->SegmentOverride : SOFT386_REG_DS,
                             Offset,
                             &State->GeneralRegs[SOFT386_REG_EAX].LowByte,
                             sizeof(UCHAR));
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeMovOffsetEax)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_CS].Size;

    /* Make sure this is the right instruction */
    ASSERT(Opcode == 0xA3);

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (Size)
    {
        ULONG Offset;

        if (!Soft386FetchDword(State, &Offset))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Write to memory */
        return Soft386WriteMemory(State,
                                  (State->PrefixFlags & SOFT386_PREFIX_SEG) ?
                                  State->SegmentOverride : SOFT386_REG_DS,
                                  Offset,
                                  &State->GeneralRegs[SOFT386_REG_EAX].Long,
                                  sizeof(ULONG));
    }
    else
    {
        USHORT Offset;

        if (!Soft386FetchWord(State, &Offset))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Write to memory */
        return Soft386WriteMemory(State,
                                  (State->PrefixFlags & SOFT386_PREFIX_SEG) ?
                                  State->SegmentOverride : SOFT386_REG_DS,
                                  Offset,
                                  &State->GeneralRegs[SOFT386_REG_EAX].LowWord,
                                  sizeof(USHORT));
    }
}
