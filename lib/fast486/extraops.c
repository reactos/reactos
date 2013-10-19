/*
 * Fast486 386/486 CPU Emulation Library
 * extraops.c
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

// #define NDEBUG
#include <debug.h>

#include <fast486.h>
#include "opcodes.h"
#include "common.h"
#include "extraops.h"

/* PUBLIC VARIABLES ***********************************************************/

FAST486_OPCODE_HANDLER_PROC
Fast486ExtendedHandlers[FAST486_NUM_OPCODE_HANDLERS] =
{
    NULL, // TODO: OPCODE 0x00 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x01 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x02 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x03 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x04 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x05 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x06 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x07 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x08 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x09 NOT IMPLEMENTED
    NULL, // Invalid
    NULL, // Reserved (UD1)
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // TODO: OPCODE 0x10 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x11 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x12 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x13 NOT IMPLEMENTED
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // TODO: OPCODE 0x20 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x21 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x22 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x23 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x24 NOT IMPLEMENTED
    NULL, // Invalid
    NULL, // TODO: OPCODE 0x26 NOT IMPLEMENTED
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // TODO: OPCODE 0x30 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x31 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x32 NOT IMPLEMENTED
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
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
    Fast486ExtOpcodeConditionalJmp,
    NULL, // TODO: OPCODE 0x90 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x91 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x92 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x93 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x94 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x95 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x96 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x97 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x98 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x99 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x9A NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x9B NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x9C NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x9D NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x9E NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0x9F NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xA0 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xA1 NOT IMPLEMENTED
    NULL, // Invalid
    NULL, // TODO: OPCODE 0xA3 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xA4 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xA5 NOT IMPLEMENTED
    NULL, // Invalid
    NULL, // Invalid
    NULL, // TODO: OPCODE 0xA8 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xA9 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xAA NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xAB NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xAC NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xAD NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xAE NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xAF NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB0 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB1 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB2 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB3 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB4 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB5 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB6 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB7 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB8 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xB9 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xBA NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xBB NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xBC NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xBD NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xBE NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xBF NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xC0 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xC1 NOT IMPLEMENTED
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // TODO: OPCODE 0xC8 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xC9 NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xCA NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xCB NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xCC NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xCD NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xCE NOT IMPLEMENTED
    NULL, // TODO: OPCODE 0xCF NOT IMPLEMENTED
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
    NULL, // Invalid
};

/* PUBLIC FUNCTIONS ***********************************************************/

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeConditionalJmp)
{
    BOOLEAN Jump = FALSE;
    LONG Offset = 0;
    BOOLEAN Size = State->SegmentRegs[FAST486_REG_CS].Size;

    if (State->PrefixFlags & FAST486_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        Size = !Size;
    }

    if (State->PrefixFlags & FAST486_PREFIX_LOCK)
    {
        /* Invalid prefix */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return FALSE;
    }

    /* Make sure this is the right instruction */
    ASSERT((Opcode & 0xF0) == 0x80);

    /* Fetch the offset */
    if (Size)
    {
        if (!Fast486FetchDword(State, (PULONG)&Offset))
        {
            /* Exception occurred */
            return FALSE;
        }
    }
    else
    {
        SHORT Value;

        if (!Fast486FetchWord(State, (PUSHORT)&Value))
        {
            /* Exception occurred */
            return FALSE;
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

    /* Return success */
    return TRUE;
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeConditionalSet)
{
    BOOLEAN Value = FALSE;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    FAST486_MOD_REG_RM ModRegRm;

    if (State->PrefixFlags & FAST486_PREFIX_ADSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
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
    return Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Value);
}

FAST486_OPCODE_HANDLER(Fast486OpcodeExtended)
{
    UCHAR SecondOpcode;

    /* Fetch the second operation code */
    if (!Fast486FetchByte(State, &SecondOpcode))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (Fast486ExtendedHandlers[SecondOpcode] != NULL)
    {
        /* Call the extended opcode handler */
        return Fast486ExtendedHandlers[SecondOpcode](State, SecondOpcode);
    }
    else
    {
        /* This is not a valid opcode */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return FALSE;
    }
}
