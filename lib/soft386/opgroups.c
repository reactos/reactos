/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            opgroups.c
 * PURPOSE:         Opcode group handlers.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

// #define WIN32_NO_STATUS
// #define _INC_WINDOWS
#include <windef.h>

// #define NDEBUG
#include <debug.h>

#include <soft386.h>
#include "opcodes.h"
#include "common.h"

/* PUBLIC FUNCTIONS ***********************************************************/

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup8082)
{
    UCHAR Immediate, Result, Dummy, Value;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Fetch the immediate operand */
    if (!Soft386FetchByte(State, &Immediate))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Read the operands */
    if (!Soft386ReadModrmByteOperands(State, &ModRegRm, &Dummy, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check which operation is this */
    switch (ModRegRm.Register)
    {
        /* ADD */
        case 0:
        {
            Result = Value + Immediate;

            /* Update CF, OF and AF */
            State->Flags.Cf = (Result < Value) && (Result < Immediate);
            State->Flags.Of = ((Value & SIGN_FLAG_BYTE) == (Immediate & SIGN_FLAG_BYTE))
                              && ((Value & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
            State->Flags.Af = (((Value & 0x0F) + (Immediate & 0x0F)) & 0x10) ? TRUE : FALSE;

            break;
        }

        /* OR */
        case 1:
        {
            Result = Value | Immediate;
            break;
        }

        /* ADC */
        case 2:
        {
            INT Carry = State->Flags.Cf ? 1 : 0;

            Result = Value + Immediate + Carry;

            /* Update CF, OF and AF */
            State->Flags.Cf = ((Immediate == 0xFF) && (Carry == 1))
                              || ((Result < Value) && (Result < (Immediate + Carry)));
            State->Flags.Of = ((Value & SIGN_FLAG_BYTE) == (Immediate & SIGN_FLAG_BYTE))
                              && ((Value & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
            State->Flags.Af = (((Value & 0x0F) + ((Immediate + Carry) & 0x0F)) & 0x10)
                              ? TRUE : FALSE;

            break;
        }

        /* SBB */
        case 3:
        {
            INT Carry = State->Flags.Cf ? 1 : 0;

            Result = Value - Immediate - Carry;

            /* Update CF, OF and AF */
            State->Flags.Cf = Value < (Immediate + Carry);
            State->Flags.Of = ((Value & SIGN_FLAG_BYTE) != (Immediate & SIGN_FLAG_BYTE))
                              && ((Value & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
            State->Flags.Af = (Value & 0x0F) < ((Immediate + Carry) & 0x0F);

            break;
        }

        /* AND */
        case 4:
        {
            Result = Value & Immediate;
            break;
        }

        /* SUB or CMP */
        case 5:
        case 7:
        {
            Result = Value - Immediate;

            /* Update CF, OF and AF */
            State->Flags.Cf = Value < Immediate;
            State->Flags.Of = ((Value & SIGN_FLAG_BYTE) != (Immediate & SIGN_FLAG_BYTE))
                              && ((Value & SIGN_FLAG_BYTE) != (Result & SIGN_FLAG_BYTE));
            State->Flags.Af = (Value & 0x0F) < (Immediate & 0x0F);

            break;
        }

        /* XOR */
        case 6:
        {
            Value ^= Immediate;
            break;
        }

        default:
        {
            /* Shouldn't happen */
            ASSERT(FALSE);
        }
    }

    /* Update ZF, SF and PF */
    State->Flags.Zf = (Result == 0) ? TRUE : FALSE;
    State->Flags.Sf = (Result & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Result);

    /* Unless this is CMP, write back the result */
    if (ModRegRm.Register != 7)
    {
        return Soft386WriteModrmByteOperands(State, &ModRegRm, FALSE, Result);
    }
    
    return TRUE;
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup81)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup83)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup8F)
{
    ULONG Value;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register != 0)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Pop a value from the stack */
    if (!Soft386StackPop(State, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (OperandSize)
    {
        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              FALSE,
                                              Value);
    }
    else
    {
        return Soft386WriteModrmWordOperands(State,
                                             &ModRegRm,
                                             FALSE,
                                             LOWORD(Value));
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC0)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC1)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC6)
{
    UCHAR Immediate;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register != 0)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Get the immediate operand */
    if (!Soft386FetchByte(State, &Immediate))
    {
        /* Exception occurred */
        return FALSE;
    }

    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         FALSE,
                                         Immediate);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC7)
{
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    
    OperandSize = AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_OPSIZE)
    {
        /* The OPSIZE prefix toggles the size */
        OperandSize = !OperandSize;
    }

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register != 0)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    if (OperandSize)
    {
        ULONG Immediate;

        /* Get the immediate operand */
        if (!Soft386FetchDword(State, &Immediate))
        {
            /* Exception occurred */
            return FALSE;
        }

        return Soft386WriteModrmDwordOperands(State,
                                              &ModRegRm,
                                              FALSE,
                                              Immediate);
    }
    else
    {
        USHORT Immediate;

        /* Get the immediate operand */
        if (!Soft386FetchWord(State, &Immediate))
        {
            /* Exception occurred */
            return FALSE;
        }

        return Soft386WriteModrmWordOperands(State,
                                             &ModRegRm,
                                             FALSE,
                                             Immediate);
    }
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD0)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD1)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD2)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD3)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupF6)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupF7)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupFE)
{
    UCHAR Dummy, Value;
    SOFT386_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[SOFT386_REG_CS].Size;

    if (State->PrefixFlags & SOFT386_PREFIX_ADSIZE)
    {
        /* The ADSIZE prefix toggles the size */
        AddressSize = !AddressSize;
    }

    if (!Soft386ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register > 1)
    {
        /* Invalid */
        Soft386Exception(State, SOFT386_EXCEPTION_UD);
        return FALSE;
    }

    /* Read the operands */
    if (!Soft386ReadModrmByteOperands(State, &ModRegRm, &Dummy, &Value))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (ModRegRm.Register == 0)
    {
        /* Increment and update OF */
        Value++;
        State->Flags.Of = (Value == SIGN_FLAG_BYTE) ? TRUE : FALSE;
    }
    else
    {
        /* Decrement and update OF */
        State->Flags.Of = (Value == SIGN_FLAG_BYTE) ? TRUE : FALSE;
        Value--;
    }

    /* Update flags */
    State->Flags.Sf = (Value & SIGN_FLAG_BYTE) ? TRUE : FALSE;
    State->Flags.Zf = (Value == 0) ? TRUE : FALSE;
    State->Flags.Af = ((Value & 0x0F) == 0) ? TRUE : FALSE;
    State->Flags.Pf = Soft386CalculateParity(Value);

    /* Write back the result */
    return Soft386WriteModrmByteOperands(State,
                                         &ModRegRm,
                                         FALSE,
                                         Value);
}

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupFF)
{
    UNIMPLEMENTED;
    return FALSE; // TODO: NOT IMPLEMENTED
}

/* EOF */

