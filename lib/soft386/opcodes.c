/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            opcodes.c
 * PURPOSE:         Opcode handlers.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

// #define WIN32_NO_STATUS
// #define _INC_WINDOWS
#include <windef.h>

#include <soft386.h>
#include "opcodes.h"
#include "common.h"

// #define NDEBUG
#include <debug.h>

/* PUBLIC VARIABLES ***********************************************************/

SOFT386_OPCODE_HANDLER_PROC
Soft386OpcodeHandlers[SOFT386_NUM_OPCODE_HANDLERS] =
{
    NULL, // TODO: OPCODE 0x00 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x01 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x02 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x03 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x04 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x05 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x06 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x07 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x08 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x09 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x0F NOT SUPPORTED
    NULL, // TODO: OPCODE 0x10 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x11 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x12 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x13 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x14 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x15 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x16 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x17 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x18 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x19 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x1F NOT SUPPORTED
    NULL, // TODO: OPCODE 0x20 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x21 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x22 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x23 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x24 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x25 NOT SUPPORTED
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x27 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x28 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x29 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x2A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x2B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x2C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x2D NOT SUPPORTED
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x2F NOT SUPPORTED
    NULL, // TODO: OPCODE 0x30 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x31 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x32 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x33 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x34 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x35 NOT SUPPORTED
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x37 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x38 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x39 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x3A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x3B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x3C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x3D NOT SUPPORTED
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x3F NOT SUPPORTED
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
    NULL, // TODO: OPCODE 0x60 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x61 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x62 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x63 NOT SUPPORTED
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0x68 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x69 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x6F NOT SUPPORTED
    NULL, // TODO: OPCODE 0x70 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x71 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x72 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x73 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x74 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x75 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x76 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x77 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x78 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x79 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x7A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x7B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x7C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x7D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x7E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x7F NOT SUPPORTED
    NULL, // TODO: OPCODE 0x80 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x81 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x82 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x83 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x84 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x85 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x86 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x87 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x88 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x89 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x8F NOT SUPPORTED
    Soft386OpcodeNop,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    Soft386OpcodeExchangeEax,
    NULL, // TODO: OPCODE 0x98 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x99 NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9A NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9B NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9C NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9D NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9E NOT SUPPORTED
    NULL, // TODO: OPCODE 0x9F NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA0 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA1 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA2 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA3 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA7 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xA9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xAF NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB0 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB1 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB2 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB3 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB7 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xB9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xBA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xBB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xBC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xBD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xBE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xBF NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC0 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC1 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC2 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC3 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC7 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xC9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xCF NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD0 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD1 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD2 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD3 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD7 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xD9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xDF NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE0 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE1 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE2 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE3 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE7 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xE9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xEA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xEB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xEC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xED NOT SUPPORTED
    NULL, // TODO: OPCODE 0xEE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xEF NOT SUPPORTED
    Soft386OpcodePrefix,
    NULL, // Invalid
    Soft386OpcodePrefix,
    Soft386OpcodePrefix,
    NULL, // TODO: OPCODE 0xF4 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xF5 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xF6 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xF7 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xF8 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xF9 NOT SUPPORTED
    NULL, // TODO: OPCODE 0xFA NOT SUPPORTED
    NULL, // TODO: OPCODE 0xFB NOT SUPPORTED
    NULL, // TODO: OPCODE 0xFC NOT SUPPORTED
    NULL, // TODO: OPCODE 0xFD NOT SUPPORTED
    NULL, // TODO: OPCODE 0xFE NOT SUPPORTED
    NULL, // TODO: OPCODE 0xFF NOT SUPPORTED
};

BOOLEAN
FASTCALL
Soft386OpcodePrefix(PSOFT386_STATE State, UCHAR Opcode)
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

BOOLEAN
FASTCALL
Soft386OpcodeIncrement(PSOFT386_STATE State, UCHAR Opcode)
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

BOOLEAN
FASTCALL
Soft386OpcodeDecrement(PSOFT386_STATE State, UCHAR Opcode)
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

BOOLEAN
FASTCALL
Soft386OpcodePushReg(PSOFT386_STATE State, UCHAR Opcode)
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

BOOLEAN
FASTCALL
Soft386OpcodePopReg(PSOFT386_STATE State, UCHAR Opcode)
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

BOOLEAN
FASTCALL
Soft386OpcodeNop(PSOFT386_STATE State, UCHAR Opcode)
{
    if (State->PrefixFlags & ~(SOFT386_PREFIX_OPSIZE | SOFT386_PREFIX_REP))
    {
        /* Allowed prefixes are REP and OPSIZE */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_REP)
    {
        // TODO: Handle PAUSE instruction.
    }

    return TRUE;
}

BOOLEAN
FASTCALL
Soft386OpcodeExchangeEax(PSOFT386_STATE State, UCHAR Opcode)
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
