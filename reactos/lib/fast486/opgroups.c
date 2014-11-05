/*
 * Fast486 386/486 CPU Emulation Library
 * opgroups.c
 *
 * Copyright (C) 2014 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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

/* PRIVATE FUNCTIONS **********************************************************/

static
inline
ULONG
Fast486ArithmeticOperation(PFAST486_STATE State,
                           INT Operation,
                           ULONG FirstValue,
                           ULONG SecondValue,
                           UCHAR Bits)
{
    ULONG Result;
    ULONG SignFlag = 1 << (Bits - 1);
    ULONG MaxValue = (SignFlag - 1) | SignFlag;

    /* Make sure the values don't exceed the maximum for their size */
    FirstValue &= MaxValue;
    SecondValue &= MaxValue;

    /* Check which operation is this */
    switch (Operation)
    {
        /* ADD */
        case 0:
        {
            Result = (FirstValue + SecondValue) & MaxValue;

            /* Update CF, OF and AF */
            State->Flags.Cf = (Result < FirstValue) && (Result < SecondValue);
            State->Flags.Of = ((FirstValue & SignFlag) == (SecondValue & SignFlag))
                              && ((FirstValue & SignFlag) != (Result & SignFlag));
            State->Flags.Af = ((((FirstValue & 0x0F) + (SecondValue & 0x0F)) & 0x10) != 0);

            break;
        }

        /* OR */
        case 1:
        {
            Result = FirstValue | SecondValue;
            break;
        }

        /* ADC */
        case 2:
        {
            INT Carry = State->Flags.Cf ? 1 : 0;

            Result = (FirstValue + SecondValue + Carry) & MaxValue;

            /* Update CF, OF and AF */
            State->Flags.Cf = ((SecondValue == MaxValue) && (Carry == 1))
                              || ((Result < FirstValue) && (Result < (SecondValue + Carry)));
            State->Flags.Of = ((FirstValue & SignFlag) == (SecondValue & SignFlag))
                              && ((FirstValue & SignFlag) != (Result & SignFlag));
            State->Flags.Af = ((FirstValue ^ SecondValue ^ Result) & 0x10) != 0;

            break;
        }

        /* SBB */
        case 3:
        {
            INT Carry = State->Flags.Cf ? 1 : 0;

            Result = (FirstValue - SecondValue - Carry) & MaxValue;

            /* Update CF, OF and AF */
            State->Flags.Cf = Carry
                              ? (FirstValue <= SecondValue)
                              : (FirstValue < SecondValue);
            State->Flags.Of = ((FirstValue & SignFlag) != (SecondValue & SignFlag))
                              && ((FirstValue & SignFlag) != (Result & SignFlag));
            State->Flags.Af = ((FirstValue ^ SecondValue ^ Result) & 0x10) != 0;

            break;
        }

        /* AND */
        case 4:
        {
            Result = FirstValue & SecondValue;
            break;
        }

        /* SUB or CMP */
        case 5:
        case 7:
        {
            Result = (FirstValue - SecondValue) & MaxValue;

            /* Update CF, OF and AF */
            State->Flags.Cf = (FirstValue < SecondValue);
            State->Flags.Of = ((FirstValue & SignFlag) != (SecondValue & SignFlag))
                              && ((FirstValue & SignFlag) != (Result & SignFlag));
            State->Flags.Af = (FirstValue & 0x0F) < (SecondValue & 0x0F);

            break;
        }

        /* XOR */
        case 6:
        {
            Result = FirstValue ^ SecondValue;
            break;
        }

        default:
        {
            /* Shouldn't happen */
            ASSERT(FALSE);
        }
    }

    /* Update ZF, SF and PF */
    State->Flags.Zf = (Result == 0);
    State->Flags.Sf = ((Result & SignFlag) != 0);
    State->Flags.Pf = Fast486CalculateParity(LOBYTE(Result));

    /* Return the result */
    return Result;
}

static
inline
ULONG
Fast486RotateOperation(PFAST486_STATE State,
                       INT Operation,
                       ULONG Value,
                       UCHAR Bits,
                       UCHAR Count)
{
    ULONG HighestBit = 1 << (Bits - 1);
    ULONG MaxValue = HighestBit | (HighestBit - 1);
    ULONG Result;

    /* Normalize the count */
    Count &= 0x1F;

    if ((Operation == 2) || (Operation == 3)) Count %= Bits + 1;

    /* If the count is zero, do nothing */
    if (Count == 0) return Value;

    /* Check which operation is this */
    switch (Operation)
    {
        /* ROL */
        case 0:
        {
            Count %= Bits;
            Result = (Value << Count) | (Value >> (Bits - Count));

            /* Update CF and OF */
            State->Flags.Cf = Result & 1;
            if (Count == 1) State->Flags.Of = State->Flags.Cf
                                              ^ ((Result & HighestBit) != 0);

            break;
        }

        /* ROR */
        case 1:
        {
            Count %= Bits;
            Result = (Value >> Count) | (Value << (Bits - Count));

            /* Update CF and OF */
            State->Flags.Cf = ((Result & HighestBit) != 0);
            if (Count == 1) State->Flags.Of = State->Flags.Cf
                                              ^ ((Result & (HighestBit >> 1)) != 0);

            break;
        }

        /* RCL */
        case 2:
        {
            Result = (Value << Count) | (State->Flags.Cf << (Count - 1));
            
            /* Complete the calculation, but make sure we don't shift by too much */
            if ((Bits - Count) < 31) Result |= Value >> (Bits - Count + 1);

            /* Update CF and OF */
            State->Flags.Cf = ((Value & (1 << (Bits - Count))) != 0);
            if (Count == 1) State->Flags.Of = State->Flags.Cf ^ ((Result & HighestBit) != 0);

            break;
        }

        /* RCR */
        case 3:
        {
            /* Update OF */
            if (Count == 1) State->Flags.Of = State->Flags.Cf ^ ((Value & HighestBit) != 0);

            Result = (Value >> Count) | (State->Flags.Cf << (Bits - Count));

            /* Complete the calculation, but make sure we don't shift by too much */
            if ((Bits - Count) < 31) Result |= Value << (Bits - Count + 1);

            /* Update CF */
            State->Flags.Cf = ((Value & (1 << (Count - 1))) != 0);

            break;
        }

        /* SHL/SAL */
        case 4:
        case 6:
        {
            Result = Value << Count;

            /* Update CF and OF */
            State->Flags.Cf = ((Value & (1 << (Bits - Count))) != 0);
            if (Count == 1) State->Flags.Of = State->Flags.Cf
                                              ^ ((Result & HighestBit) != 0);

            break;
        }

        /* SHR */
        case 5:
        {
            Result = Value >> Count;

            /* Update CF and OF */
            State->Flags.Cf = ((Value & (1 << (Count - 1))) != 0);
            if (Count == 1) State->Flags.Of = ((Value & HighestBit) != 0);

            break;
        }

        /* SAR */
        case 7:
        {
            Result = Value >> Count;

            /* Fill the top Count bits with the sign bit */
            if (Value & HighestBit) Result |= ((1 << Count) - 1) << (Bits - Count);

            /* Update CF and OF */
            State->Flags.Cf = ((Value & (1 << (Count - 1))) != 0);
            if (Count == 1) State->Flags.Of = FALSE;

            break;
        }
    }

    if (Operation >= 4)
    {
        /* Update ZF, SF and PF */
        State->Flags.Zf = ((Result & MaxValue) == 0);
        State->Flags.Sf = ((Result & HighestBit) != 0);
        State->Flags.Pf = Fast486CalculateParity(Result);
    }

    /* Return the result */
    return Result;
}

/* PUBLIC FUNCTIONS ***********************************************************/

FAST486_OPCODE_HANDLER(Fast486OpcodeGroup8082)
{
    UCHAR Immediate, Value;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Fetch the immediate operand */
    if (!Fast486FetchByte(State, &Immediate))
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

    /* Calculate the result */
    Value = Fast486ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 8);

    /* Unless this is CMP, write back the result */
    if (ModRegRm.Register != 7)
    {
        Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Value);
    }
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroup81)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Immediate, Value;

        /* Fetch the immediate operand */
        if (!Fast486FetchDword(State, &Immediate))
        {
            /* Exception occurred */
            return;
        }

        /* Read the operands */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Value = Fast486ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 32);

        /* Unless this is CMP, write back the result */
        if (ModRegRm.Register != 7)
        {
            Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
        }
    }
    else
    {
        USHORT Immediate, Value;

        /* Fetch the immediate operand */
        if (!Fast486FetchWord(State, &Immediate))
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

        /* Calculate the result */
        Value = Fast486ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 16);

        /* Unless this is CMP, write back the result */
        if (ModRegRm.Register != 7)
        {
            Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
        }
    }
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroup83)
{
    CHAR ImmByte;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Fetch the immediate operand */
    if (!Fast486FetchByte(State, (PUCHAR)&ImmByte))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Immediate = (ULONG)((LONG)ImmByte); // Sign extend
        ULONG Value;

        /* Read the operands */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Value = Fast486ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 32);

        /* Unless this is CMP, write back the result */
        if (ModRegRm.Register != 7)
        {
            Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
        }
    }
    else
    {
        USHORT Immediate = (USHORT)((SHORT)ImmByte); // Sign extend
        USHORT Value;

        /* Read the operands */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Value = Fast486ArithmeticOperation(State, ModRegRm.Register, Value, Immediate, 16);

        /* Unless this is CMP, write back the result */
        if (ModRegRm.Register != 7)
        {
            Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
        }
    }
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroup8F)
{
    ULONG Value;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Pop a value from the stack - this must be done first */
    if (!Fast486StackPop(State, &Value))
    {
        /* Exception occurred */
        return;
    }

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred - restore SP */
        if (OperandSize) State->GeneralRegs[FAST486_REG_ESP].Long -= sizeof(ULONG);
        else State->GeneralRegs[FAST486_REG_ESP].LowWord -= sizeof(USHORT);

        return;
    }

    if (ModRegRm.Register != 0)
    {
        /* Invalid */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    if (OperandSize) Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    else Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, LOWORD(Value));
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupC0)
{
    UCHAR Value, Count;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Fetch the count */
    if (!Fast486FetchByte(State, &Count))
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

    /* Calculate the result */
    Value = LOBYTE(Fast486RotateOperation(State,
                                          ModRegRm.Register,
                                          Value,
                                          8,
                                          Count));

    /* Write back the result */
    Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Value);
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupC1)
{
    UCHAR Count;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Fetch the count */
    if (!Fast486FetchByte(State, &Count))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Value;

        /* Read the operands */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Value = Fast486RotateOperation(State,
                                       ModRegRm.Register,
                                       Value,
                                       32,
                                       Count);

        /* Write back the result */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    }
    else
    {
        USHORT Value;

        /* Read the operands */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Value = LOWORD(Fast486RotateOperation(State,
                                              ModRegRm.Register,
                                              Value,
                                              16,
                                              Count));

        /* Write back the result */
        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
    }
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupC6)
{
    UCHAR Immediate;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (ModRegRm.Register != 0)
    {
        /* Invalid */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    /* Get the immediate operand */
    if (!Fast486FetchByte(State, &Immediate))
    {
        /* Exception occurred */
        return;
    }

    Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Immediate);
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupC7)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (ModRegRm.Register != 0)
    {
        /* Invalid */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    if (OperandSize)
    {
        ULONG Immediate;

        /* Get the immediate operand */
        if (!Fast486FetchDword(State, &Immediate))
        {
            /* Exception occurred */
            return;
        }

        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Immediate);
    }
    else
    {
        USHORT Immediate;

        /* Get the immediate operand */
        if (!Fast486FetchWord(State, &Immediate))
        {
            /* Exception occurred */
            return;
        }

        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Immediate);
    }
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupD0)
{
    UCHAR Value;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

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

    /* Calculate the result */
    Value = LOBYTE(Fast486RotateOperation(State, ModRegRm.Register, Value, 8, 1));

    /* Write back the result */
    Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Value);

}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupD1)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Value;

        /* Read the operands */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Value = Fast486RotateOperation(State, ModRegRm.Register, Value, 32, 1);

        /* Write back the result */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    }
    else
    {
        USHORT Value;

        /* Read the operands */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Value = LOWORD(Fast486RotateOperation(State, ModRegRm.Register, Value, 16, 1));

        /* Write back the result */
        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
    }
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupD2)
{
    UCHAR Value;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

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

    /* Calculate the result */
    Value = LOBYTE(Fast486RotateOperation(State,
                                          ModRegRm.Register,
                                          Value,
                                          8,
                                          State->GeneralRegs[FAST486_REG_ECX].LowByte));

    /* Write back the result */
    Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Value);
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupD3)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (OperandSize)
    {
        ULONG Value;

        /* Read the operands */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Value = Fast486RotateOperation(State,
                                       ModRegRm.Register,
                                       Value,
                                       32,
                                       State->GeneralRegs[FAST486_REG_ECX].LowByte);

        /* Write back the result */
        Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
    }
    else
    {
        USHORT Value;

        /* Read the operands */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        /* Calculate the result */
        Value = LOWORD(Fast486RotateOperation(State,
                                              ModRegRm.Register,
                                              Value,
                                              16,
                                              State->GeneralRegs[FAST486_REG_ECX].LowByte));

        /* Write back the result */
        Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
    }
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupF6)
{
    UCHAR Value = 0;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

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

    switch (ModRegRm.Register)
    {
        /* TEST */
        case 0:
        case 1:
        {
            UCHAR Immediate, Result;

            /* Fetch the immediate byte */
            if (!Fast486FetchByte(State, &Immediate))
            {
                /* Exception occurred */
                return;
            }

            /* Calculate the result */
            Result = Value & Immediate;

            /* Update the flags */
            State->Flags.Cf = FALSE;
            State->Flags.Of = FALSE;
            State->Flags.Zf = (Result == 0);
            State->Flags.Sf = ((Result & SIGN_FLAG_BYTE) != 0);
            State->Flags.Pf = Fast486CalculateParity(Result);

            break;
        }

        /* NOT */
        case 2:
        {
            /* Write back the result */
            Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, ~Value);

            break;
        }

        /* NEG */
        case 3:
        {
            /* Calculate the result */
            UCHAR Result = -Value;

            /* Update the flags */
            State->Flags.Cf = (Value != 0);
            State->Flags.Of = (Value & SIGN_FLAG_BYTE) && (Result & SIGN_FLAG_BYTE);
            State->Flags.Af = ((Value & 0x0F) != 0);
            State->Flags.Zf = (Result == 0);
            State->Flags.Sf = ((Result & SIGN_FLAG_BYTE) != 0);
            State->Flags.Pf = Fast486CalculateParity(Result);

            /* Write back the result */
            Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Result);

            break;
        }

        /* MUL */
        case 4:
        {
            USHORT Result = (USHORT)Value * (USHORT)State->GeneralRegs[FAST486_REG_EAX].LowByte;

            /* Update the flags */
            State->Flags.Cf = State->Flags.Of = (HIBYTE(Result) != 0);

            /* Write back the result */
            State->GeneralRegs[FAST486_REG_EAX].LowWord = Result;

            break;
        }

        /* IMUL */
        case 5:
        {
            SHORT Result = (SHORT)((CHAR)Value) * (SHORT)((CHAR)State->GeneralRegs[FAST486_REG_EAX].LowByte);

            /* Update the flags */
            State->Flags.Cf = State->Flags.Of = ((Result < -128) || (Result > 127));

            /* Write back the result */
            State->GeneralRegs[FAST486_REG_EAX].LowWord = (USHORT)Result;

            break;
        }

        /* DIV */
        case 6:
        {
            UCHAR Quotient, Remainder;

            if (Value == 0)
            {
                /* Divide error */
                Fast486Exception(State, FAST486_EXCEPTION_DE);
                return;
            }

            Quotient = State->GeneralRegs[FAST486_REG_EAX].LowWord / Value;
            Remainder = State->GeneralRegs[FAST486_REG_EAX].LowWord % Value;

            /* Write back the results */
            State->GeneralRegs[FAST486_REG_EAX].LowByte = Quotient;
            State->GeneralRegs[FAST486_REG_EAX].HighByte = Remainder;

            break;
        }

        /* IDIV */
        case 7:
        {
            CHAR Quotient, Remainder;

            if (Value == 0)
            {
                /* Divide error */
                Fast486Exception(State, FAST486_EXCEPTION_DE);
                return;
            }

            Quotient = (SHORT)State->GeneralRegs[FAST486_REG_EAX].LowWord / (CHAR)Value;
            Remainder = (SHORT)State->GeneralRegs[FAST486_REG_EAX].LowWord % (CHAR)Value;

            /* Write back the results */
            State->GeneralRegs[FAST486_REG_EAX].LowByte = (UCHAR)Quotient;
            State->GeneralRegs[FAST486_REG_EAX].HighByte = (UCHAR)Remainder;

            break;
        }
    }
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupF7)
{
    ULONG Value = 0, SignFlag;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Set the sign flag */
    if (OperandSize) SignFlag = SIGN_FLAG_LONG;
    else SignFlag = SIGN_FLAG_WORD;

    /* Read the operand */
    if (OperandSize)
    {
        /* 32-bit */
        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }
    }
    else
    {
        /* 16-bit */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, (PUSHORT)&Value))
        {
            /* Exception occurred */
            return;
        }
    }

    switch (ModRegRm.Register)
    {
        /* TEST */
        case 0:
        case 1:
        {
            ULONG Immediate = 0, Result = 0;

            if (OperandSize)
            {
                /* Fetch the immediate dword */
                if (!Fast486FetchDword(State, &Immediate))
                {
                    /* Exception occurred */
                    return;
                }
            }
            else
            {
                /* Fetch the immediate word */
                if (!Fast486FetchWord(State, (PUSHORT)&Immediate))
                {
                    /* Exception occurred */
                    return;
                }
            }

            /* Calculate the result */
            Result = Value & Immediate;

            /* Update the flags */
            State->Flags.Cf = FALSE;
            State->Flags.Of = FALSE;
            State->Flags.Zf = (Result == 0);
            State->Flags.Sf = ((Result & SignFlag) != 0);
            State->Flags.Pf = Fast486CalculateParity(Result);

            break;
        }

        /* NOT */
        case 2:
        {
            /* Write back the result */
            if (OperandSize)
            {
                /* 32-bit */
                Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, ~Value);
            }
            else
            {
                /* 16-bit */
                Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, LOWORD(~Value));
            }

            break;
        }

        /* NEG */
        case 3:
        {
            /* Calculate the result */
            ULONG Result = -Value;
            if (!OperandSize) Result &= 0xFFFF;

            /* Update the flags */
            State->Flags.Cf = (Value != 0);
            State->Flags.Of = (Value & SignFlag) && (Result & SignFlag);
            State->Flags.Af = ((Value & 0x0F) != 0);
            State->Flags.Zf = (Result == 0);
            State->Flags.Sf = ((Result & SignFlag) != 0);
            State->Flags.Pf = Fast486CalculateParity(Result);

            /* Write back the result */
            if (OperandSize)
            {
                /* 32-bit */
                Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Result);
            }
            else
            {
                /* 16-bit */
                Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, LOWORD(Result));
            }

            break;
        }

        /* MUL */
        case 4:
        {
            if (OperandSize)
            {
                ULONGLONG Result = (ULONGLONG)Value * (ULONGLONG)State->GeneralRegs[FAST486_REG_EAX].Long;

                /* Update the flags */
                State->Flags.Cf = State->Flags.Of = ((Result & 0xFFFFFFFF00000000ULL) != 0);

                /* Write back the result */
                State->GeneralRegs[FAST486_REG_EAX].Long = Result & 0xFFFFFFFFULL;
                State->GeneralRegs[FAST486_REG_EDX].Long = Result >> 32;
            }
            else
            {
                ULONG Result = (ULONG)Value * (ULONG)State->GeneralRegs[FAST486_REG_EAX].LowWord;

                /* Update the flags */
                State->Flags.Cf = State->Flags.Of = (HIWORD(Result) != 0);

                /* Write back the result */
                State->GeneralRegs[FAST486_REG_EAX].LowWord = LOWORD(Result);
                State->GeneralRegs[FAST486_REG_EDX].LowWord = HIWORD(Result);
            }

            break;
        }

        /* IMUL */
        case 5:
        {
            if (OperandSize)
            {
                LONGLONG Result = (LONGLONG)((LONG)Value) * (LONGLONG)((LONG)State->GeneralRegs[FAST486_REG_EAX].Long);

                /* Update the flags */
                State->Flags.Cf = State->Flags.Of = ((Result < -2147483648LL) || (Result > 2147483647LL));

                /* Write back the result */
                State->GeneralRegs[FAST486_REG_EAX].Long = Result & 0xFFFFFFFFULL;
                State->GeneralRegs[FAST486_REG_EDX].Long = Result >> 32;
            }
            else
            {
                LONG Result = (LONG)((SHORT)Value) * (LONG)((SHORT)State->GeneralRegs[FAST486_REG_EAX].LowWord);

                /* Update the flags */
                State->Flags.Cf = State->Flags.Of = ((Result < -32768) || (Result > 32767));

                /* Write back the result */
                State->GeneralRegs[FAST486_REG_EAX].LowWord = LOWORD(Result);
                State->GeneralRegs[FAST486_REG_EDX].LowWord = HIWORD(Result);
            }

            break;
        }

        /* DIV */
        case 6:
        {
            if (Value == 0)
            {
                /* Divide error */
                Fast486Exception(State, FAST486_EXCEPTION_DE);
                return;
            }

            if (OperandSize)
            {
                ULONGLONG Dividend = (ULONGLONG)State->GeneralRegs[FAST486_REG_EAX].Long
                                     | ((ULONGLONG)State->GeneralRegs[FAST486_REG_EDX].Long << 32);
                ULONG Quotient = Dividend / Value;
                ULONG Remainder = Dividend % Value;

                /* Write back the results */
                State->GeneralRegs[FAST486_REG_EAX].Long = Quotient;
                State->GeneralRegs[FAST486_REG_EDX].Long = Remainder;
            }
            else
            {
                ULONG Dividend = (ULONG)State->GeneralRegs[FAST486_REG_EAX].LowWord
                                 | ((ULONG)State->GeneralRegs[FAST486_REG_EDX].LowWord << 16);
                USHORT Quotient = Dividend / Value;
                USHORT Remainder = Dividend % Value;

                /* Write back the results */
                State->GeneralRegs[FAST486_REG_EAX].LowWord = Quotient;
                State->GeneralRegs[FAST486_REG_EDX].LowWord = Remainder;
            }

            break;
        }

        /* IDIV */
        case 7:
        {
            if (Value == 0)
            {
                /* Divide error */
                Fast486Exception(State, FAST486_EXCEPTION_DE);
                return;
            }

            if (OperandSize)
            {
                LONGLONG Dividend = (LONGLONG)State->GeneralRegs[FAST486_REG_EAX].Long
                                     | ((LONGLONG)State->GeneralRegs[FAST486_REG_EDX].Long << 32);
                LONG Quotient = Dividend / (LONG)Value;
                LONG Remainder = Dividend % (LONG)Value;

                /* Write back the results */
                State->GeneralRegs[FAST486_REG_EAX].Long = (ULONG)Quotient;
                State->GeneralRegs[FAST486_REG_EDX].Long = (ULONG)Remainder;
            }
            else
            {
                LONG Dividend = (LONG)State->GeneralRegs[FAST486_REG_EAX].LowWord
                                 | ((LONG)State->GeneralRegs[FAST486_REG_EDX].LowWord << 16);
                SHORT Quotient = Dividend / (SHORT)LOWORD(Value);
                SHORT Remainder = Dividend % (SHORT)LOWORD(Value);

                /* Write back the results */
                State->GeneralRegs[FAST486_REG_EAX].LowWord = (USHORT)Quotient;
                State->GeneralRegs[FAST486_REG_EDX].LowWord = (USHORT)Remainder;
            }

            break;
        }
    }
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupFE)
{
    UCHAR Value;
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (ModRegRm.Register > 1)
    {
        /* Invalid */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    /* Read the operands */
    if (!Fast486ReadModrmByteOperands(State, &ModRegRm, NULL, &Value))
    {
        /* Exception occurred */
        return;
    }

    if (ModRegRm.Register == 0)
    {
        /* Increment and update OF and AF */
        Value++;
        State->Flags.Of = (Value == SIGN_FLAG_BYTE);
        State->Flags.Af = ((Value & 0x0F) == 0);
    }
    else
    {
        /* Decrement and update OF and AF */
        State->Flags.Of = (Value == SIGN_FLAG_BYTE);
        Value--;
        State->Flags.Af = ((Value & 0x0F) == 0x0F);
    }

    /* Update flags */
    State->Flags.Zf = (Value == 0);
    State->Flags.Sf = ((Value & SIGN_FLAG_BYTE) != 0);
    State->Flags.Pf = Fast486CalculateParity(Value);

    /* Write back the result */
    Fast486WriteModrmByteOperands(State, &ModRegRm, FALSE, Value);
}

FAST486_OPCODE_HANDLER(Fast486OpcodeGroupFF)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (ModRegRm.Register == 7)
    {
        /* Invalid */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    /* Read the operands */
    if (OperandSize)
    {
        ULONG Value;

        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        if (ModRegRm.Register == 0)
        {
            /* Increment and update OF and AF */
            Value++;
            State->Flags.Of = (Value == SIGN_FLAG_LONG);
            State->Flags.Af = ((Value & 0x0F) == 0);
        }
        else if (ModRegRm.Register == 1)
        {
            /* Decrement and update OF and AF */
            State->Flags.Of = (Value == SIGN_FLAG_LONG);
            Value--;
            State->Flags.Af = ((Value & 0x0F) == 0x0F);
        }
        else if (ModRegRm.Register == 2)
        {
            /* Push the current value of EIP */
            if (!Fast486StackPush(State, State->InstPtr.Long))
            {
                /* Exception occurred */
                return;
            }

            /* Set the EIP to the address */
            State->InstPtr.Long = Value;
        }
        else if (ModRegRm.Register == 3)
        {
            USHORT Selector;
            FAST486_SEG_REGS Segment = FAST486_REG_DS;

            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read the selector */
            if (!Fast486ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress + sizeof(ULONG),
                                   FALSE,
                                   &Selector,
                                   sizeof(USHORT)))
            {
                /* Exception occurred */
                return;
            }

            if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
            {
                if (!Fast486ProcessGate(State, Selector, Value, TRUE))
                {
                    /* Gate processed or exception occurred */
                    return;
                }
            }

            /* Push the current value of CS */
            if (!Fast486StackPush(State, State->SegmentRegs[FAST486_REG_CS].Selector))
            {
                /* Exception occurred */
                return;
            }

            /* Push the current value of EIP */
            if (!Fast486StackPush(State, State->InstPtr.Long))
            {
                /* Exception occurred */
                return;
            }

            /* Load the new code segment */
            if (!Fast486LoadSegment(State, FAST486_REG_CS, Selector))
            {
                /* Exception occurred */
                return;
            }

            /* Set the EIP to the address */
            State->InstPtr.Long = Value;
        }
        else if (ModRegRm.Register == 4)
        {
            /* Set the EIP to the address */
            State->InstPtr.Long = Value;
        }
        else if (ModRegRm.Register == 5)
        {
            USHORT Selector;
            FAST486_SEG_REGS Segment = FAST486_REG_DS;

            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read the selector */
            if (!Fast486ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress + sizeof(ULONG),
                                   FALSE,
                                   &Selector,
                                   sizeof(USHORT)))
            {
                /* Exception occurred */
                return;
            }

            if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
            {
                if (!Fast486ProcessGate(State, Selector, Value, FALSE))
                {
                    /* Gate processed or exception occurred */
                    return;
                }
            }

            /* Load the new code segment */
            if (!Fast486LoadSegment(State, FAST486_REG_CS, Selector))
            {
                /* Exception occurred */
                return;
            }

            /* Set the EIP to the address */
            State->InstPtr.Long = Value;
        }
        else if (ModRegRm.Register == 6)
        {
            /* Push the value on to the stack */
            Fast486StackPush(State, Value);
            return;
        }

        if (ModRegRm.Register <= 1)
        {
            /* Update flags */
            State->Flags.Sf = ((Value & SIGN_FLAG_LONG) != 0);
            State->Flags.Zf = (Value == 0);
            State->Flags.Pf = Fast486CalculateParity(Value);

            /* Write back the result */
            Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value);
        }
    }
    else
    {
        USHORT Value;

        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        if (ModRegRm.Register == 0)
        {
            /* Increment and update OF */
            Value++;
            State->Flags.Of = (Value == SIGN_FLAG_WORD);
            State->Flags.Af = ((Value & 0x0F) == 0);
        }
        else if (ModRegRm.Register == 1)
        {
            /* Decrement and update OF */
            State->Flags.Of = (Value == SIGN_FLAG_WORD);
            Value--;
            State->Flags.Af = ((Value & 0x0F) == 0x0F);
        }
        else if (ModRegRm.Register == 2)
        {
            /* Push the current value of IP */
            if (!Fast486StackPush(State, State->InstPtr.LowWord))
            {
                /* Exception occurred */
                return;
            }

            /* Set the IP to the address */
            State->InstPtr.LowWord = Value;

            /* Clear the top half of EIP */
            State->InstPtr.Long &= 0xFFFF;
        }
        else if (ModRegRm.Register == 3)
        {
            USHORT Selector;
            FAST486_SEG_REGS Segment = FAST486_REG_DS;

            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read the selector */
            if (!Fast486ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress + sizeof(USHORT),
                                   FALSE,
                                   &Selector,
                                   sizeof(USHORT)))
            {
                /* Exception occurred */
                return;
            }

            /* Push the current value of CS */
            if (!Fast486StackPush(State, State->SegmentRegs[FAST486_REG_CS].Selector))
            {
                /* Exception occurred */
                return;
            }

            /* Push the current value of IP */
            if (!Fast486StackPush(State, State->InstPtr.LowWord))
            {
                /* Exception occurred */
                return;
            }

            /* Load the new code segment */
            if (!Fast486LoadSegment(State, FAST486_REG_CS, Selector))
            {
                /* Exception occurred */
                return;
            }

            /* Set the IP to the address */
            State->InstPtr.LowWord = Value;

            /* Clear the top half of EIP */
            State->InstPtr.Long &= 0xFFFF;
        }
        else if (ModRegRm.Register == 4)
        {
            /* Set the IP to the address */
            State->InstPtr.LowWord = Value;

            /* Clear the top half of EIP */
            State->InstPtr.Long &= 0xFFFF;
        }
        else if (ModRegRm.Register == 5)
        {
            USHORT Selector;
            FAST486_SEG_REGS Segment = FAST486_REG_DS;

            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read the selector */
            if (!Fast486ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress + sizeof(USHORT),
                                   FALSE,
                                   &Selector,
                                   sizeof(USHORT)))
            {
                /* Exception occurred */
                return;
            }

            /* Load the new code segment */
            if (!Fast486LoadSegment(State, FAST486_REG_CS, Selector))
            {
                /* Exception occurred */
                return;
            }

            /* Set the IP to the address */
            State->InstPtr.LowWord = Value;

            /* Clear the top half of EIP */
            State->InstPtr.Long &= 0xFFFF;
        }
        else if (ModRegRm.Register == 6)
        {
            /* Push the value on to the stack */
            Fast486StackPush(State, Value);
            return;
        }
        else
        {
            /* Invalid */
            Fast486Exception(State, FAST486_EXCEPTION_UD);
            return;
        }

        if (ModRegRm.Register <= 1)
        {
            /* Update flags */
            State->Flags.Sf = ((Value & SIGN_FLAG_WORD) != 0);
            State->Flags.Zf = (Value == 0);
            State->Flags.Pf = Fast486CalculateParity(Value);

            /* Write back the result */
            Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value);
        }
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeGroup0F00)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    NO_LOCK_PREFIX();
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Check which operation this is */
    switch (ModRegRm.Register)
    {
        /* SLDT */
        case 0:
        {
            /* Not recognized in real mode or virtual 8086 mode */
            if (!(State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
                || State->Flags.Vm)
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, State->Ldtr.Selector);
            break;
        }

        /* STR */
        case 1:
        {
            /* Not recognized in real mode or virtual 8086 mode */
            if (!(State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
                || State->Flags.Vm)
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, State->TaskReg.Selector);
            break;
        }

        /* LLDT */
        case 2:
        {
            BOOLEAN Valid;
            USHORT Selector;
            FAST486_SYSTEM_DESCRIPTOR GdtEntry;

            /* Not recognized in real mode or virtual 8086 mode */
            if (!(State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
                || State->Flags.Vm)
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            /* This is a privileged instruction */
            if (Fast486GetCurrentPrivLevel(State) != 0)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return;
            }

            if (!Fast486ReadModrmWordOperands(State,
                                              &ModRegRm,
                                              NULL,
                                              &Selector))
            {
                /* Exception occurred */
                return;
            }

            if (Selector & SEGMENT_TABLE_INDICATOR)
            {
                /* This selector doesn't point to the GDT */
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector);
                return;
            }

            if (Fast486ReadDescriptorEntry(State,
                                           Selector,
                                           &Valid,
                                           (PFAST486_GDT_ENTRY)&GdtEntry))
            {
                /* Exception occurred */
                return;
            }

            if (!Valid)
            {
                /* Invalid selector */
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector);
                return;
            }

            if (GET_SEGMENT_INDEX(Selector) == 0)
            {
                RtlZeroMemory(&State->Ldtr, sizeof(State->Ldtr));
                return;
            }

            if (!GdtEntry.Present)
            {
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_NP, Selector);
                return;
            }

            if (GdtEntry.Signature != FAST486_LDT_SIGNATURE)
            {
                /* This is not a LDT descriptor */
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector);
                return;
            }

            /* Update the LDTR */
            State->Ldtr.Selector = Selector;
            State->Ldtr.Base = GdtEntry.Base | (GdtEntry.BaseMid << 16) | (GdtEntry.BaseHigh << 24);
            State->Ldtr.Limit = GdtEntry.Limit | (GdtEntry.LimitHigh << 16);
            if (GdtEntry.Granularity) State->Ldtr.Limit <<= 12;

            break;
        }

        /* LTR */
        case 3:
        {
            BOOLEAN Valid;
            USHORT Selector;
            FAST486_SYSTEM_DESCRIPTOR GdtEntry;

            /* Not recognized in real mode or virtual 8086 mode */
            if (!(State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
                || State->Flags.Vm)
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            /* This is a privileged instruction */
            if (Fast486GetCurrentPrivLevel(State) != 0)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return;
            }

            if (!Fast486ReadModrmWordOperands(State,
                                              &ModRegRm,
                                              NULL,
                                              &Selector))
            {
                /* Exception occurred */
                return;
            }

            if (Selector & SEGMENT_TABLE_INDICATOR)
            {
                /* This selector doesn't point to the GDT */
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector);
                return;
            }

            if (!Fast486ReadDescriptorEntry(State,
                                           Selector,
                                           &Valid,
                                           (PFAST486_GDT_ENTRY)&GdtEntry))
            {
                /* Exception occurred */
                return;
            }

            if (!Valid)
            {
                /* Invalid selector */
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector);
                return;
            }

            if (GET_SEGMENT_INDEX(Selector) == 0)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return;
            }

            if (!GdtEntry.Present)
            {
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_NP, Selector);
                return;
            }

            if (GdtEntry.Signature != FAST486_TSS_SIGNATURE)
            {
                /* This is not a TSS descriptor */
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector);
                return;
            }

            /* Update the TR */
            State->TaskReg.Selector = Selector;
            State->TaskReg.Base = GdtEntry.Base | (GdtEntry.BaseMid << 16) | (GdtEntry.BaseHigh << 24);
            State->TaskReg.Limit = GdtEntry.Limit | (GdtEntry.LimitHigh << 16);
            if (GdtEntry.Granularity) State->TaskReg.Limit <<= 12;

            break;
        }

        /* VERR/VERW */
        case 4:
        case 5:
        {
            USHORT Selector;
            BOOLEAN Valid;
            FAST486_GDT_ENTRY GdtEntry;

            /* Not recognized in real mode or virtual 8086 mode */
            if (!(State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
                || State->Flags.Vm)
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            /* This is a privileged instruction */
            if (Fast486GetCurrentPrivLevel(State) != 0)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return;
            }

            if (!Fast486ReadModrmWordOperands(State,
                                              &ModRegRm,
                                              NULL,
                                              &Selector))
            {
                /* Exception occurred */
                return;
            }

            if (!Fast486ReadDescriptorEntry(State, Selector, &Valid, &GdtEntry))
            {
                /* Exception occurred */
                return;
            }

            if (!Valid)
            {
                /* Clear ZF */
                State->Flags.Zf = FALSE;
                return;
            }

            /* Set ZF if it is valid and accessible */
            State->Flags.Zf = GdtEntry.Present // must be present
                               && GdtEntry.SystemType // must be a segment
                               && (((ModRegRm.Register == 4)
                               /* code segments are only readable if the RW bit is set */
                               && (!GdtEntry.Executable || GdtEntry.ReadWrite))
                               || ((ModRegRm.Register == 5)
                               /* code segments are never writable, data segments are writable when RW is set */
                               && (!GdtEntry.Executable && GdtEntry.ReadWrite)))
                               /*
                                * for segments other than conforming code segments,
                                * both RPL and CPL must be less than or equal to DPL
                                */
                               && ((!GdtEntry.Executable || !GdtEntry.DirConf)
                               && ((GET_SEGMENT_RPL(Selector) <= GdtEntry.Dpl)
                               && (Fast486GetCurrentPrivLevel(State) <= GdtEntry.Dpl)))
                               /* for conforming code segments, DPL must be less than or equal to CPL */
                               && ((GdtEntry.Executable && GdtEntry.DirConf)
                               && (GdtEntry.Dpl <= Fast486GetCurrentPrivLevel(State)));


            break;
        }

        /* Invalid */
        default:
        {
            Fast486Exception(State, FAST486_EXCEPTION_UD);
        }
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeGroup0F01)
{
    // FAST486_TABLE_REG TableReg;
    UCHAR TableReg[6];
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    NO_LOCK_PREFIX();
    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* Check for the segment override */
    if (State->PrefixFlags & FAST486_PREFIX_SEG)
    {
        /* Use the override segment instead */
        Segment = State->SegmentOverride;
    }

    /* Check which operation this is */
    switch (ModRegRm.Register)
    {
        /* SGDT */
        case 0:
        {
            if (!ModRegRm.Memory)
            {
                /* The second operand must be a memory location */
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            /* Fill the 6-byte table register */
            // TableReg = State->Gdtr;
            *((PUSHORT)&TableReg) = State->Gdtr.Size;
            *((PULONG)&TableReg[sizeof(USHORT)]) = State->Gdtr.Address;

            /* Store the GDTR */
            Fast486WriteMemory(State,
                               Segment,
                               ModRegRm.MemoryAddress,
                               TableReg,
                               sizeof(TableReg));

            break;
        }

        /* SIDT */
        case 1:
        {
            if (!ModRegRm.Memory)
            {
                /* The second operand must be a memory location */
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            /* Fill the 6-byte table register */
            // TableReg = State->Idtr;
            *((PUSHORT)&TableReg) = State->Idtr.Size;
            *((PULONG)&TableReg[sizeof(USHORT)]) = State->Idtr.Address;

            /* Store the IDTR */
            Fast486WriteMemory(State,
                               Segment,
                               ModRegRm.MemoryAddress,
                               TableReg,
                               sizeof(TableReg));

            break;
        }

        /* LGDT */
        case 2:
        {
            /* This is a privileged instruction */
            if (Fast486GetCurrentPrivLevel(State) != 0)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return;
            }

            if (!ModRegRm.Memory)
            {
                /* The second operand must be a memory location */
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            /* Read the new GDTR */
            if (!Fast486ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress,
                                   FALSE,
                                   TableReg,
                                   sizeof(TableReg)))
            {
                /* Exception occurred */
                return;
            }

            /* Load the new GDT */
            // State->Gdtr = TableReg;
            State->Gdtr.Size = *((PUSHORT)&TableReg);
            State->Gdtr.Address = *((PULONG)&TableReg[sizeof(USHORT)]);

            /* In 16-bit mode the highest byte is masked out */
            if (!OperandSize) State->Gdtr.Address &= 0x00FFFFFF;

            break;
        }

        /* LIDT */
        case 3:
        {
            /* This is a privileged instruction */
            if (Fast486GetCurrentPrivLevel(State) != 0)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return;
            }

            if (!ModRegRm.Memory)
            {
                /* The second operand must be a memory location */
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            /* Read the new IDTR */
            if (!Fast486ReadMemory(State,
                                   Segment,
                                   ModRegRm.MemoryAddress,
                                   FALSE,
                                   TableReg,
                                   sizeof(TableReg)))
            {
                /* Exception occurred */
                return;
            }

            /* Load the new IDT */
            // State->Idtr = TableReg;
            State->Idtr.Size = *((PUSHORT)&TableReg);
            State->Idtr.Address = *((PULONG)&TableReg[sizeof(USHORT)]);

            /* In 16-bit mode the highest byte is masked out */
            if (!OperandSize) State->Idtr.Address &= 0x00FFFFFF;

            break;
        }

        /* SMSW */
        case 4:
        {
            /* Store the lower 16 bits (Machine Status Word) of CR0 */
            Fast486WriteModrmWordOperands(State,
                                          &ModRegRm,
                                          FALSE,
                                          LOWORD(State->ControlRegisters[FAST486_REG_CR0]));

            break;
        }

        /* LMSW */
        case 6:
        {
            USHORT MachineStatusWord;

            /* This is a privileged instruction */
            if (Fast486GetCurrentPrivLevel(State) != 0)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return;
            }

            /* Read the new Machine Status Word */
            if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &MachineStatusWord))
            {
                /* Exception occurred */
                return;
            }

            /* This instruction cannot be used to return to real mode */
            if ((State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
                && !(MachineStatusWord & FAST486_CR0_PE))
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return;
            }

            /* Set the lowest 4 bits */
            State->ControlRegisters[FAST486_REG_CR0] &= 0xFFFFFFF0;
            State->ControlRegisters[FAST486_REG_CR0] |= MachineStatusWord & 0x0F;

            break;
        }

        /* INVLPG */
        case 7:
        {
#ifndef FAST486_NO_PREFETCH
            /* Invalidate the prefetch */
            State->PrefetchValid = FALSE;
#endif

            /* This is a privileged instruction */
            if (Fast486GetCurrentPrivLevel(State) != 0)
            {
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return;
            }

            if (!ModRegRm.Memory)
            {
                /* The second operand must be a memory location */
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }

            if (State->Tlb != NULL)
            {
                /* Clear the TLB entry */
                State->Tlb[ModRegRm.MemoryAddress >> 12] = INVALID_TLB_FIELD;
            }

            break;
        }

        /* Invalid */
        default:
        {
            Fast486Exception(State, FAST486_EXCEPTION_UD);
        }
    }
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeGroup0FB9)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    /* All of them are reserved (UD2) */
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return;
}

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeGroup0FBA)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN OperandSize, AddressSize;
    UINT DataSize;
    UCHAR BitNumber;

    OperandSize = AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_OPSIZE(OperandSize);
    TOGGLE_ADSIZE(AddressSize);

    /* Get the number of bits */
    if (OperandSize) DataSize = 32;
    else DataSize = 16;

    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    if (ModRegRm.Register < 4)
    {
        /* Invalid */
        Fast486Exception(State, FAST486_EXCEPTION_UD);
        return;
    }

    /* Get the bit number */
    if (!Fast486FetchByte(State, &BitNumber))
    {
        /* Exception occurred */
        return;
    }

    if (ModRegRm.Memory)
    {
        /*
         * For memory operands, add the bit offset divided by
         * the data size to the address
         */
        ModRegRm.MemoryAddress += BitNumber / DataSize;
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

        if (ModRegRm.Register == 5)
        {
            /* BTS */
            Value |= 1 << BitNumber;
        }
        else if (ModRegRm.Register == 6)
        {
            /* BTR */
            Value &= ~(1 << BitNumber);
        }
        else if (ModRegRm.Register == 7)
        {
            /* BTC */
            Value ^= 1 << BitNumber;
        }

        if (ModRegRm.Register >= 5)
        {
            /* Write back the result */
            if (!Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value))
            {
                /* Exception occurred */
                return;
            }
        }
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

        if (ModRegRm.Register == 5)
        {
            /* BTS */
            Value |= 1 << BitNumber;
        }
        else if (ModRegRm.Register == 6)
        {
            /* BTR */
            Value &= ~(1 << BitNumber);
        }
        else if (ModRegRm.Register == 7)
        {
            /* BTC */
            Value ^= 1 << BitNumber;
        }

        if (ModRegRm.Register >= 5)
        {
            /* Write back the result */
            if (!Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, Value))
            {
                /* Exception occurred */
                return;
            }
        }
    }
}

/* EOF */
