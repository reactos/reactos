/*
 * Fast486 386/486 CPU Emulation Library
 * fpu.c
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
#include "common.h"
#include "fpu.h"

/* PRIVATE FUNCTIONS **********************************************************/

#ifndef FAST486_NO_FPU

static ULONGLONG
UnsignedMult128(ULONGLONG Multiplicand,
                ULONGLONG Multiplier,
                ULONGLONG *HighProduct)
{
    ULONG MultiplicandLow, MultiplicandHigh, MultiplierLow, MultiplierHigh;
    ULONG IntermediateLow, IntermediateHigh;
    ULONGLONG LowProduct, Intermediate, Intermediate1, Intermediate2;
    
    MultiplicandLow = (ULONG)(Multiplicand & 0xFFFFFFFFULL);
    MultiplicandHigh = (ULONG)(Multiplicand >> 32);
    MultiplierLow = (ULONG)(Multiplier & 0xFFFFFFFFULL);
    MultiplierHigh = (ULONG)(Multiplier >> 32);

    LowProduct = (ULONGLONG)MultiplicandLow * (ULONGLONG)MultiplierLow;
    Intermediate1 = (ULONGLONG)MultiplicandLow * (ULONGLONG)MultiplierHigh;
    Intermediate2 = (ULONGLONG)MultiplicandHigh * (ULONGLONG)MultiplierLow;
    *HighProduct = (ULONGLONG)MultiplicandHigh * (ULONGLONG)MultiplierHigh;

    Intermediate = Intermediate1 + Intermediate2;
    if (Intermediate < Intermediate1) *HighProduct += 1ULL << 32;

    IntermediateLow = (ULONG)(Intermediate & 0xFFFFFFFFULL);
    IntermediateHigh = (ULONG)(Intermediate >> 32);

    LowProduct += (ULONGLONG)IntermediateLow << 32;
    if ((ULONG)(LowProduct >> 32) < IntermediateLow) (*HighProduct)++;

    *HighProduct += IntermediateHigh;
    return LowProduct;
}

static inline VOID FASTCALL
Fast486FpuFromInteger(PFAST486_STATE State,
                      LONGLONG Value,
                      PFAST486_FPU_DATA_REG Result)
{
    ULONG ZeroCount;

    Result->Sign = Result->Exponent = Result->Mantissa = 0;
    if (Value == 0LL) return;

    if (Value < 0LL)
    {
        Result->Sign = 1;
        Value = -Value;
    }

    Result->Mantissa = (ULONGLONG)Value;
    ZeroCount = CountLeadingZeros64(Result->Mantissa);
    
    Result->Mantissa <<= ZeroCount;
    Result->Exponent = FPU_REAL10_BIAS + 63 - ZeroCount;
}

static inline BOOLEAN FASTCALL
Fast486FpuToInteger(PFAST486_STATE State,
                    PFAST486_FPU_DATA_REG Value,
                    PLONGLONG Result)
{
    ULONG Bits;
    ULONGLONG Remainder;
    SHORT UnbiasedExp = (SHORT)Value->Exponent - FPU_REAL10_BIAS;

    if (FPU_IS_ZERO(Value))
    {
        *Result = 0LL;
        return TRUE;
    }
    
    if (FPU_IS_NAN(Value) || !FPU_IS_NORMALIZED(Value)
        || (UnbiasedExp < 0) || (UnbiasedExp > 63))
    {
        State->FpuStatus.Ie = TRUE;
        return FALSE;
    }

    Bits = 63 - UnbiasedExp;

    /* Calculate the result and the remainder */
    *Result = (LONGLONG)(Value->Mantissa >> Bits);
    Remainder = Value->Mantissa & ((1 << Bits) - 1);

    /* The result must be positive here */
    ASSERT(*Result >= 0LL);

    switch (State->FpuControl.Rc)
    {
        case FPU_ROUND_NEAREST:
        {
            /* Check if the highest bit of the remainder is set */
            if (Remainder & (1 << (Bits - 1)))
            {
                (*Result)++;

                /* Check if all the other bits are clear */
                if (!(Remainder & ((1 << (Bits - 1)) - 1)))
                {
                    /* Round to even */
                    *Result &= ~1;
                }
            }

            break;
        }

        case FPU_ROUND_DOWN:
        {
            if ((Remainder != 0ULL) && Value->Sign) (*Result)++;
            break;
        }

        case FPU_ROUND_UP:
        {
            if ((Remainder != 0ULL) && !Value->Sign) (*Result)++;
            break;
        }

        default:
        {
            /* Leave it truncated */
        }
    }

    if (Value->Sign) *Result = -*Result;
    return TRUE;
}

static inline VOID FASTCALL
Fast486FpuFromSingleReal(PFAST486_STATE State,
                         ULONG Value,
                         PFAST486_FPU_DATA_REG Result)
{
    /* Extract the sign, exponent and mantissa */
    Result->Sign = (UCHAR)(Value >> 31);
    Result->Exponent = (USHORT)((Value >> 23) & 0xFF);
    Result->Mantissa = (((ULONGLONG)Value & 0x7FFFFFULL) | 0x800000ULL) << 40;

    /* If this is a zero, we're done */
    if (Value == 0) return;

    if (Result->Exponent == 0xFF) Result->Exponent = FPU_MAX_EXPONENT + 1;
    else
    {
        /* Adjust the exponent bias */
        Result->Exponent += (FPU_REAL10_BIAS - FPU_REAL4_BIAS);
    }
}

static inline VOID FASTCALL
Fast486FpuFromDoubleReal(PFAST486_STATE State,
                         ULONGLONG Value,
                         PFAST486_FPU_DATA_REG Result)
{
    /* Extract the sign, exponent and mantissa */
    Result->Sign = (UCHAR)(Value >> 63);
    Result->Exponent = (USHORT)((Value >> 52) & 0x7FF);
    Result->Mantissa = (((ULONGLONG)Value & 0xFFFFFFFFFFFFFULL) | 0x10000000000000ULL) << 11;

    /* If this is a zero, we're done */
    if (Value == 0) return;

    if (Result->Exponent == 0x3FF) Result->Exponent = FPU_MAX_EXPONENT + 1;
    else
    {
        /* Adjust the exponent bias */
        Result->Exponent += (FPU_REAL10_BIAS - FPU_REAL8_BIAS);
    }
}

static inline BOOLEAN FASTCALL
Fast486FpuToDoubleReal(PFAST486_STATE State,
                       PFAST486_FPU_DATA_REG Value,
                       PULONGLONG Result)
{
    ULONGLONG Remainder;
    SHORT UnbiasedExp = (SHORT)Value->Exponent - FPU_REAL10_BIAS;

    if (FPU_IS_ZERO(Value))
    {
        *Result = 0LL;
        return TRUE;
    }

    /* Calculate the mantissa */
    *Result = (Value->Mantissa >> 11) & ((1ULL << 52) - 1);

    if (FPU_IS_NAN(Value))
    {
        *Result |= 0x3FF0000000000000ULL;
        goto SetSign;
    }

    /* Check for underflow */
    if (!FPU_IS_NORMALIZED(Value) || (UnbiasedExp < -1023))
    {
        State->FpuStatus.Ue = TRUE;
        return FALSE;
    }

    /* Check for overflow */
    if (UnbiasedExp > 1023)
    {
        State->FpuStatus.Oe = TRUE;
        return FALSE;
    }

    /* Calculate the remainder */
    Remainder = Value->Mantissa & ((1 << 11) - 1);

    switch (State->FpuControl.Rc)
    {
        case FPU_ROUND_NEAREST:
        {
            /* Check if the highest bit of the remainder is set */
            if (Remainder & (1 << 10))
            {
                (*Result)++;

                /* Check if all the other bits are clear */
                if (!(Remainder & ((1 << 10) - 1)))
                {
                    /* Round to even */
                    *Result &= ~1;
                }
            }

            break;
        }

        case FPU_ROUND_DOWN:
        {
            if ((Remainder != 0ULL) && Value->Sign) (*Result)++;
            break;
        }

        case FPU_ROUND_UP:
        {
            if ((Remainder != 0ULL) && !Value->Sign) (*Result)++;
            break;
        }

        default:
        {
            /* Leave it truncated */
        }
    }

    /* Store the biased exponent */
    *Result |= (ULONGLONG)(UnbiasedExp + FPU_REAL8_BIAS) << 52;

SetSign:

    if (Value->Sign) *Result |= 1ULL << 63;
    return TRUE;
}

static inline BOOLEAN FASTCALL
Fast486FpuFromPackedBcd(PFAST486_STATE State,
                        PUCHAR Value,
                        PFAST486_FPU_DATA_REG Result)
{
    INT i;
    LONGLONG IntVal = 0LL;

    for (i = 8; i >= 0; i--)
    {
        if (((Value[i] & 0x0F) > 9) || ((Value[i] >> 4) > 9))
        {
            /* Invalid BCD */
            State->FpuStatus.Ie = TRUE;
            return FALSE;
        }

        IntVal *= 100LL;
        IntVal += (Value[i] >> 4) * 10 + (Value[i] & 0x0F);
    }

    /* Apply the sign */
    if (Value[9] & 0x80) IntVal = -IntVal;

    /* Now convert the integer to FP80 */
    Fast486FpuFromInteger(State, IntVal, Result);
    return TRUE;
}

static inline BOOLEAN FASTCALL
Fast486FpuToPackedBcd(PFAST486_STATE State,
                      PFAST486_FPU_DATA_REG Value,
                      PUCHAR Result)
{
    INT i;
    LONGLONG IntVal;

    /* Convert it to an integer first */
    if (!Fast486FpuToInteger(State, Value, &IntVal)) return FALSE;

    if (IntVal < 0LL)
    {
        IntVal = -IntVal;
        Result[9] = 0x80;
    }

    for (i = 0; i < 9; i++)
    {
        Result[i] = (UCHAR)((IntVal % 10) + (((IntVal / 10) % 10) << 4));
        IntVal /= 100LL;
    }

    return TRUE;
}

static inline VOID FASTCALL
Fast486FpuAdd(PFAST486_STATE State,
              PFAST486_FPU_DATA_REG FirstOperand,
              PFAST486_FPU_DATA_REG SecondOperand,
              PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG FirstAdjusted = *FirstOperand;
    FAST486_FPU_DATA_REG SecondAdjusted = *SecondOperand;
    FAST486_FPU_DATA_REG TempResult;

    if (!FPU_IS_NORMALIZED(FirstOperand) || !FPU_IS_NORMALIZED(SecondOperand))
    {
        /* Denormalized */
        State->FpuStatus.De = TRUE;
    }

    /* Find the largest exponent */
    TempResult.Exponent = max(FirstOperand->Exponent, SecondOperand->Exponent);

    /* Adjust the first operand to it... */
    if (FirstAdjusted.Exponent < TempResult.Exponent)
    {
        FirstAdjusted.Mantissa >>= (TempResult.Exponent - FirstAdjusted.Exponent);
        FirstAdjusted.Exponent = TempResult.Exponent;
    }

    /* ... and the second one too */
    if (SecondAdjusted.Exponent < TempResult.Exponent)
    {
        SecondAdjusted.Mantissa >>= (TempResult.Exponent - SecondAdjusted.Exponent);
        SecondAdjusted.Exponent = TempResult.Exponent;
    }

    if (FirstAdjusted.Sign == SecondAdjusted.Sign)
    {
        /* Calculate the mantissa and sign of the result */
        TempResult.Mantissa = FirstAdjusted.Mantissa + SecondAdjusted.Mantissa;
        TempResult.Sign = FirstAdjusted.Sign;
    }
    else
    {
        /* Calculate the sign of the result */
        if (FirstAdjusted.Mantissa > SecondAdjusted.Mantissa) TempResult.Sign = FirstAdjusted.Sign;
        else if (FirstAdjusted.Mantissa < SecondAdjusted.Mantissa) TempResult.Sign = SecondAdjusted.Sign;
        else TempResult.Sign = FALSE;

        /* Invert the negative mantissa */
        if (FirstAdjusted.Sign) FirstAdjusted.Mantissa = -FirstAdjusted.Mantissa;
        if (SecondAdjusted.Sign) SecondAdjusted.Mantissa = -SecondAdjusted.Mantissa;

        /* Calculate the mantissa of the result */
        TempResult.Mantissa = FirstAdjusted.Mantissa + SecondAdjusted.Mantissa;
    }

    /* Did it overflow? */
    if (FPU_IS_NORMALIZED(&FirstAdjusted) && FPU_IS_NORMALIZED(&SecondAdjusted))
    {
        if (TempResult.Exponent == FPU_MAX_EXPONENT)
        {
            /* Total overflow, return infinity */
            TempResult.Mantissa = FPU_MANTISSA_HIGH_BIT;
            TempResult.Exponent = FPU_MAX_EXPONENT + 1;

            /* Update flags */
            State->FpuStatus.Oe = TRUE;
        }
        else
        {
            /* Lose the LSB in favor of the carry */
            TempResult.Mantissa >>= 1;
            TempResult.Mantissa |= FPU_MANTISSA_HIGH_BIT;
            TempResult.Exponent++;
        }
    }
    
    /* Normalize the result and return it */
    Fast486FpuNormalize(State, &TempResult);
    *Result = TempResult;
}

static inline VOID FASTCALL
Fast486FpuSubtract(PFAST486_STATE State,
                   PFAST486_FPU_DATA_REG FirstOperand,
                   PFAST486_FPU_DATA_REG SecondOperand,
                   PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG NegativeSecondOperand = *SecondOperand;

    /* Invert the sign */
    NegativeSecondOperand.Sign = !NegativeSecondOperand.Sign;

    /* And perform an addition instead */
    Fast486FpuAdd(State, Result, FirstOperand, &NegativeSecondOperand);
}

static inline VOID FASTCALL
Fast486FpuCompare(PFAST486_STATE State,
                  PFAST486_FPU_DATA_REG FirstOperand,
                  PFAST486_FPU_DATA_REG SecondOperand)
{
    if (FPU_IS_NAN(FirstOperand) || FPU_IS_NAN(SecondOperand))
    {
        if (FPU_IS_POS_INF(FirstOperand) && FPU_IS_NEG_INF(SecondOperand))
        {
            State->FpuStatus.Code0 = FALSE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = FALSE;
        }
        else if (FPU_IS_NEG_INF(FirstOperand) && FPU_IS_POS_INF(SecondOperand))
        {
            State->FpuStatus.Code0 = TRUE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = FALSE;
        }
        else
        {
            State->FpuStatus.Code0 = TRUE;
            State->FpuStatus.Code2 = TRUE;
            State->FpuStatus.Code3 = TRUE;
        }
    }
    else
    {
        FAST486_FPU_DATA_REG TempResult;

        Fast486FpuSubtract(State, FirstOperand, SecondOperand, &TempResult);

        if (FPU_IS_ZERO(&TempResult))
        {
            State->FpuStatus.Code0 = FALSE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = TRUE;
        }
        else if (TempResult.Sign)
        {
            State->FpuStatus.Code0 = TRUE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = FALSE;
        }
        else
        {
            State->FpuStatus.Code0 = FALSE;
            State->FpuStatus.Code2 = FALSE;
            State->FpuStatus.Code3 = FALSE;
        }
    }
}

static inline VOID FASTCALL
Fast486FpuMultiply(PFAST486_STATE State,
                   PFAST486_FPU_DATA_REG FirstOperand,
                   PFAST486_FPU_DATA_REG SecondOperand,
                   PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG TempResult;

    if (!FPU_IS_NORMALIZED(FirstOperand) || !FPU_IS_NORMALIZED(SecondOperand))
    {
        /* Denormalized */
        State->FpuStatus.De = TRUE;
    }

    UnsignedMult128(FirstOperand->Mantissa,
                    SecondOperand->Mantissa,
                    &TempResult.Mantissa);

    TempResult.Exponent = FirstOperand->Exponent + SecondOperand->Exponent;
    TempResult.Sign = FirstOperand->Sign ^ SecondOperand->Sign;

    /* Normalize the result */
    Fast486FpuNormalize(State, &TempResult);
    *Result = TempResult;
}

static inline VOID FASTCALL
Fast486FpuDivide(PFAST486_STATE State,
                 PFAST486_FPU_DATA_REG FirstOperand,
                 PFAST486_FPU_DATA_REG SecondOperand,
                 PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG TempResult;

    if (FPU_IS_ZERO(SecondOperand))
    {
        /* Division by zero */
        State->FpuStatus.Ze = TRUE;
        return;
    }

    TempResult.Exponent = FirstOperand->Exponent - SecondOperand->Exponent;
    TempResult.Sign = FirstOperand->Sign ^ SecondOperand->Sign;

    // TODO: NOT IMPLEMENTED
    UNREFERENCED_PARAMETER(TempResult);
    UNIMPLEMENTED;
}

static inline VOID FASTCALL
Fast486FpuArithmeticOperation(PFAST486_STATE State,
                              INT Operation,
                              PFAST486_FPU_DATA_REG SourceOperand,
                              PFAST486_FPU_DATA_REG DestOperand)
{
    ASSERT(!(Operation & ~7));

    /* Check the operation */
    switch (Operation)
    {
        /* FADD */
        case 0:
        {
            Fast486FpuAdd(State, DestOperand, SourceOperand, DestOperand);
            break;
        }

        /* FMUL */
        case 1:
        {
            Fast486FpuMultiply(State, DestOperand, SourceOperand, DestOperand);
            break;
        }

        /* FCOM */
        case 2:
        /* FCOMP */
        case 3:
        {
            Fast486FpuCompare(State, DestOperand, SourceOperand);
            if (Operation == 3) Fast486FpuPop(State);

            break;
        }

        /* FSUB */
        case 4:
        {
            Fast486FpuSubtract(State, DestOperand, SourceOperand, DestOperand);
            break;
        }

        /* FSUBR */
        case 5:
        {
            Fast486FpuSubtract(State, SourceOperand, DestOperand, DestOperand);
            break;
        }

        /* FDIV */
        case 6:
        {
            Fast486FpuDivide(State, DestOperand, SourceOperand, DestOperand);
            break;
        }

        /* FDIVR */
        case 7:
        {
            Fast486FpuDivide(State, SourceOperand, DestOperand, DestOperand);
            break;
        }
    }
}

#endif

/* PUBLIC FUNCTIONS ***********************************************************/

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD8)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    PFAST486_FPU_DATA_REG SourceOperand, DestOperand;
    FAST486_FPU_DATA_REG MemoryData;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
    {
        /* Invalid operation */
        State->FpuStatus.Ie = TRUE;
        return;
    }

    if (ModRegRm.Memory)
    {
        /* Load the source operand from memory */
        ULONG Value;

        if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
        {
            /* Exception occurred */
            return;
        }

        Fast486FpuFromSingleReal(State, Value, &MemoryData);
        SourceOperand = &MemoryData;

        /* The destination operand is ST0 */
        DestOperand = &FPU_ST(0);
    }
    else
    {
        if (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
        {
            /* Invalid operation */
            State->FpuStatus.Ie = TRUE;
            return;
        }

        /* Load the source operand from an FPU register */
        SourceOperand = &FPU_ST(ModRegRm.SecondRegister);

        /* The destination operand is ST0 */
        DestOperand = &FPU_ST(0);
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, SourceOperand, DestOperand);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDC)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    PFAST486_FPU_DATA_REG SourceOperand, DestOperand;
    FAST486_FPU_DATA_REG MemoryData;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
    {
        /* Invalid operation */
        State->FpuStatus.Ie = TRUE;
        return;
    }

    if (ModRegRm.Memory)
    {
        /* Load the source operand from memory */
        ULONGLONG Value;

        if (!Fast486ReadMemory(State,
                               (State->PrefixFlags & FAST486_PREFIX_SEG)
                               ? State->SegmentOverride : FAST486_REG_DS,
                               ModRegRm.MemoryAddress,
                               FALSE,
                               &Value,
                               sizeof(ULONGLONG)))
        {
            /* Exception occurred */
            return;
        }

        Fast486FpuFromDoubleReal(State, Value, &MemoryData);
        SourceOperand = &MemoryData;

        /* The destination operand is ST0 */
        DestOperand = &FPU_ST(0);
    }
    else
    {
        if (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
        {
            /* Invalid operation */
            State->FpuStatus.Ie = TRUE;
            return;
        }

        /* The source operand is ST0 */
        SourceOperand = &FPU_ST(0);

        /* Load the destination operand from an FPU register */
        DestOperand = &FPU_ST(ModRegRm.SecondRegister);
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, SourceOperand, DestOperand);

#endif
}


FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD9)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
#else
    /* Do nothing */
#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDA)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    PFAST486_FPU_DATA_REG SourceOperand, DestOperand;
    LONG Value;
    FAST486_FPU_DATA_REG MemoryData;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (!ModRegRm.Memory)
    {
        /* The only valid opcode in this case is FUCOMPP (0xDA 0xE9) */
        if ((ModRegRm.Register != 5) && (ModRegRm.SecondRegister != 1))
        {
            Fast486Exception(State, FAST486_EXCEPTION_UD);
            return;
        }

        if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(1) == FPU_TAG_EMPTY))
        {
            /* FPU Exception */
            State->FpuStatus.Ie = TRUE;
            return;
        }

        /* Compare */
        Fast486FpuCompare(State, &FPU_ST(0), &FPU_ST(1));

        /* Pop twice */
        Fast486FpuPop(State);
        Fast486FpuPop(State);

        return;
    }

    /* Load the source operand from memory */
    if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, (PULONG)&Value))
    {
        /* Exception occurred */
        return;
    }

    Fast486FpuFromInteger(State, (LONGLONG)Value, &MemoryData);
    SourceOperand = &MemoryData;

    /* The destination operand is always ST0 */
    DestOperand = &FPU_ST(0);

    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
    {
        /* Invalid operation */
        State->FpuStatus.Ie = TRUE;
        return;
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, SourceOperand, DestOperand); 
    
#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDB)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (ModRegRm.Memory)
    {
        switch (ModRegRm.Register)
        {
            /* FILD */
            case 0:
            {
                LONG Value;
                FAST486_FPU_DATA_REG Temp;

                if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, (PULONG)&Value))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromInteger(State, (LONGLONG)Value, &Temp);
                Fast486FpuPush(State, &Temp);

                break;
            }

            /* FIST */
            case 2:
            /* FISTP */
            case 3:
            {
                LONGLONG Temp;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(0) == FPU_TAG_SPECIAL))
                {
                    /* Fail */
                    State->FpuStatus.Ie = TRUE;
                    return;
                }

                if (!Fast486FpuToInteger(State, &FPU_ST(0), &Temp))
                {
                    /* Exception occurred */
                    return;
                }

                /* Check if it can fit in a signed 32-bit integer */
                if ((((ULONGLONG)Temp >> 31) + 1ULL) > 1ULL)
                {
                    State->FpuStatus.Ie = TRUE;
                    return;
                }

                if (!Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, (ULONG)((LONG)Temp)))
                {
                    /* Exception occurred */
                    return;
                }

                if (ModRegRm.Register == 3)
                {
                    /* Pop the FPU stack too */
                    Fast486FpuPop(State);
                }

                break;
            }

            /* FLD */
            case 5:
            {
                FAST486_FPU_DATA_REG Value;
                UCHAR Buffer[10];

                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? State->SegmentOverride : FAST486_REG_DS,
                                       ModRegRm.MemoryAddress,
                                       FALSE,
                                       Buffer,
                                       sizeof(Buffer)))
                {
                    /* Exception occurred */
                    return;
                }

                Value.Mantissa = *((PULONGLONG)Buffer);
                Value.Exponent = *((PUSHORT)&Buffer[8]) & (FPU_MAX_EXPONENT + 1);
                Value.Sign = *((PUCHAR)&Buffer[9]) >> 7;

                Fast486FpuPush(State, &Value); 
                break;
            }

            /* FSTP */
            case 7:
            {
                UCHAR Buffer[10];

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    /* Fail */
                    State->FpuStatus.Ie = TRUE;
                    return;
                }

                *((PULONGLONG)Buffer) = FPU_ST(0).Mantissa;
                *((PUSHORT)&Buffer[sizeof(ULONGLONG)]) = FPU_ST(0).Exponent | (FPU_ST(0).Sign ? 0x8000 : 0);

                if (!Fast486WriteMemory(State,
                                        (State->PrefixFlags & FAST486_PREFIX_SEG)
                                        ? State->SegmentOverride : FAST486_REG_DS,
                                        ModRegRm.MemoryAddress,
                                        Buffer,
                                        sizeof(Buffer)))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuPop(State); 
                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
            }
        }
    }
    else
    {
        /* Only a few of these instructions have any meaning on a 487 */
        switch ((ModRegRm.Register << 3) | ModRegRm.SecondRegister)
        {
            /* FCLEX */
            case 0x22:
            {
                /* Clear exception data */
                State->FpuStatus.Ie =
                State->FpuStatus.De =
                State->FpuStatus.Ze =
                State->FpuStatus.Oe =
                State->FpuStatus.Ue =
                State->FpuStatus.Pe =
                State->FpuStatus.Sf =
                State->FpuStatus.Es =
                State->FpuStatus.Busy = FALSE;

                break;
            }

            /* FINIT */
            case 0x23:
            {
                /* Restore the state */
                State->FpuControl.Value = FAST486_FPU_DEFAULT_CONTROL;
                State->FpuStatus.Value = 0;
                State->FpuTag = 0xFFFF;

                break;
            }

            /* FENI */
            case 0x20:
            /* FDISI */
            case 0x21:
            {
                /* These do nothing */
                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
            }
        }
    }

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDD)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (ModRegRm.Memory)
    {
        switch (ModRegRm.Register)
        {
            /* FLD */
            case 0:
            {
                ULONGLONG Value;
                FAST486_FPU_DATA_REG MemoryData;

                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? State->SegmentOverride : FAST486_REG_DS,
                                       ModRegRm.MemoryAddress,
                                       FALSE,
                                       &Value,
                                       sizeof(ULONGLONG)))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromDoubleReal(State, Value, &MemoryData);
                Fast486FpuPush(State, &MemoryData);

                break;
            }

            /* FST */
            case 2:
            /* FSTP */
            case 3:
            {
                ULONGLONG Value;

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    /* Fail */
                    State->FpuStatus.Ie = TRUE;
                    return;
                }

                if (!Fast486FpuToDoubleReal(State, &FPU_ST(0), &Value))
                {
                    /* Exception occurred */
                    return;
                }

                if (!Fast486WriteMemory(State,
                                        (State->PrefixFlags & FAST486_PREFIX_SEG)
                                        ? State->SegmentOverride : FAST486_REG_DS,
                                        ModRegRm.MemoryAddress,
                                        &Value,
                                        sizeof(ULONGLONG)))
                {
                    /* Exception occurred */
                    return;
                }

                if (ModRegRm.Register == 3) Fast486FpuPop(State);
                break;
            }

            /* FRSTOR */
            case 4:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FSAVE */
            case 6:
            {
                // TODO: NOT IMPLEMENTED 
                UNIMPLEMENTED;

                break;
            }

            /* FSTSW */
            case 7:
            {
                Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, State->FpuStatus.Value);
                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
            }
        }
    }
    else
    {
        switch (ModRegRm.Register)
        {
            /* FFREE */
            case 0:
            {
                FPU_SET_TAG(ModRegRm.SecondRegister, FPU_TAG_EMPTY);
                break;
            }

            /* FXCH */
            case 1:
            {
                FAST486_FPU_DATA_REG Temp;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                    || FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;
                    break;
                }

                /* Exchange */
                Temp = FPU_ST(0);
                FPU_ST(0) = FPU_ST(ModRegRm.SecondRegister);
                FPU_ST(ModRegRm.SecondRegister) = Temp;

                FPU_UPDATE_TAG(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);

                break;
            }

            /* FST */
            case 2:
            /* FSTP */
            case 3:
            {
                FPU_ST(ModRegRm.SecondRegister) = FPU_ST(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);

                if (ModRegRm.Register == 3) Fast486FpuPop(State);
                break;
            }

            /* FUCOM */
            case 4:
            /* FUCOMP */
            case 5:
            {
                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                    || (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY))
                {
                    State->FpuStatus.Ie = TRUE;
                    return;
                }

                Fast486FpuCompare(State, &FPU_ST(0), &FPU_ST(ModRegRm.SecondRegister));
                if (ModRegRm.Register == 5) Fast486FpuPop(State);

                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
            }
        }
    }

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDE)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    PFAST486_FPU_DATA_REG SourceOperand, DestOperand;
    BOOLEAN PopStack = FALSE;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
    {
        /* Invalid operation */
        State->FpuStatus.Ie = TRUE;
        return;
    }

    if (ModRegRm.Memory)
    {
        SHORT Value;
        FAST486_FPU_DATA_REG MemoryData;

        /* Load the source operand from memory */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, (PUSHORT)&Value))
        {
            /* Exception occurred */
            return;
        }

        Fast486FpuFromInteger(State, (LONGLONG)Value, &MemoryData);
        SourceOperand = &MemoryData;

        /* The destination operand is ST0 */
        DestOperand = &FPU_ST(0);
    }
    else
    {
        /* FCOMPP check */
        if ((ModRegRm.Register == 3) && (ModRegRm.SecondRegister != 1))
        {
            /* Invalid */
            Fast486Exception(State, FAST486_EXCEPTION_UD);
            return;
        }

        /* The source operand is ST0 */
        SourceOperand = &FPU_ST(0);

        /* Load the destination operand from a register */
        DestOperand = &FPU_ST(ModRegRm.SecondRegister);

        if (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
        {
            /* Invalid operation */
            State->FpuStatus.Ie = TRUE;
            return;
        }

        PopStack = TRUE;
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, SourceOperand, DestOperand);  
    if (PopStack) Fast486FpuPop(State);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDF)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (ModRegRm.Memory)
    {
        switch (ModRegRm.Register)
        {
            /* FILD */
            case 0:
            {
                SHORT Value;
                FAST486_FPU_DATA_REG Temp;

                if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, (PUSHORT)&Value))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromInteger(State, (LONGLONG)Value, &Temp);
                Fast486FpuPush(State, &Temp);

                break;
            }

            /* FIST */
            case 2:
            /* FISTP */
            case 3:
            {
                LONGLONG Temp;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(0) == FPU_TAG_SPECIAL))
                {
                    /* Fail */
                    State->FpuStatus.Ie = TRUE;
                    return;
                }

                if (!Fast486FpuToInteger(State, &FPU_ST(0), &Temp))
                {
                    /* Exception occurred */
                    return;
                }

                /* Check if it can fit in a signed 16-bit integer */
                if ((((ULONGLONG)Temp >> 15) + 1ULL) > 1ULL)
                {
                    State->FpuStatus.Ie = TRUE;
                    return;
                }

                if (!Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, (USHORT)((SHORT)Temp)))
                {
                    /* Exception occurred */
                    return;
                }

                if (ModRegRm.Register == 3)
                {
                    /* Pop the FPU stack too */
                    Fast486FpuPop(State);
                }

                break;
            }

            /* FBLD */
            case 4:
            {
                FAST486_FPU_DATA_REG Value;
                UCHAR Buffer[10];

                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? State->SegmentOverride : FAST486_REG_DS,
                                       ModRegRm.MemoryAddress,
                                       FALSE,
                                       Buffer,
                                       sizeof(Buffer)))
                {
                    /* Exception occurred */
                    return;
                }

                if (!Fast486FpuFromPackedBcd(State, Buffer, &Value))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuPush(State, &Value); 
                break;
            }

            /* FILD (64-bit int) */
            case 5:
            {
                LONGLONG Value;
                FAST486_FPU_DATA_REG Temp;

                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? State->SegmentOverride : FAST486_REG_DS,
                                       ModRegRm.MemoryAddress,
                                       FALSE,
                                       &Value,
                                       sizeof(LONGLONG)))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromInteger(State, (LONGLONG)Value, &Temp);
                Fast486FpuPush(State, &Temp);

                break;
            }

            /* FBSTP */
            case 6:
            {
                UCHAR Buffer[10];

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    /* Fail */
                    State->FpuStatus.Ie = TRUE;
                    return;
                }

                if (!Fast486FpuToPackedBcd(State, &FPU_ST(0), Buffer))
                {
                    /* Exception occurred */
                    return;
                }

                if (!Fast486WriteMemory(State,
                                        (State->PrefixFlags & FAST486_PREFIX_SEG)
                                        ? State->SegmentOverride : FAST486_REG_DS,
                                        ModRegRm.MemoryAddress,
                                        Buffer,
                                        sizeof(Buffer)))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuPop(State); 
                break;
            }

            /* FISTP (64-bit int) */
            case 7:
            {
                LONGLONG Temp;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(0) == FPU_TAG_SPECIAL))
                {
                    /* Fail */
                    State->FpuStatus.Ie = TRUE;
                    return;
                }

                if (!Fast486FpuToInteger(State, &FPU_ST(0), &Temp))
                {
                    /* Exception occurred */
                    return;
                }

                if (!Fast486WriteMemory(State,
                                        (State->PrefixFlags & FAST486_PREFIX_SEG)
                                        ? State->SegmentOverride : FAST486_REG_DS,
                                        ModRegRm.MemoryAddress,
                                        &Temp,
                                        sizeof(LONGLONG)))
                {
                    /* Exception occurred */
                    return;
                }

                /* Pop the FPU stack too */
                Fast486FpuPop(State);

                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
            }
        }
    }
    else
    {
        switch (ModRegRm.Register)
        {
            /* FFREEP */
            case 0:
            {
                FPU_SET_TAG(ModRegRm.SecondRegister, FPU_TAG_EMPTY);
                Fast486FpuPop(State);

                break;
            }

            /* FXCH */
            case 1:
            {
                FAST486_FPU_DATA_REG Temp;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                    || FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;
                    break;
                }

                /* Exchange */
                Temp = FPU_ST(0);
                FPU_ST(0) = FPU_ST(ModRegRm.SecondRegister);
                FPU_ST(ModRegRm.SecondRegister) = Temp;

                FPU_UPDATE_TAG(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);

                break;
            }

            /* FSTP */
            case 2:
            case 3:
            {
                FPU_ST(ModRegRm.SecondRegister) = FPU_ST(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);
                Fast486FpuPop(State);

                break;
            }

            /* FSTSW */
            case 4:
            {
                if (ModRegRm.SecondRegister != 0)
                {
                    /* Invalid */
                    Fast486Exception(State, FAST486_EXCEPTION_UD);
                    return;
                }

                /* Store the status word in AX */
                State->GeneralRegs[FAST486_REG_EAX].LowWord = State->FpuStatus.Value;

                break;
            }

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
            }
        }
    }

#endif
}

/* EOF */
