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

/* CONSTANTS ******************************************************************/

/* 0.00 */
static const FAST486_FPU_DATA_REG FpuZero = {0ULL, 0, FALSE};

/* 1.00 */
static const FAST486_FPU_DATA_REG FpuOne = {0x8000000000000000ULL, FPU_REAL10_BIAS, FALSE};

/* Pi */
static const FAST486_FPU_DATA_REG FpuPi = {0xC90FDAA22168C234ULL, FPU_REAL10_BIAS + 1, FALSE};

/* lb(10) */
static const FAST486_FPU_DATA_REG FpuL2Ten = {0xD49A784BCD1D8AFEULL, FPU_REAL10_BIAS + 1, FALSE};

/* lb(e) */
static const FAST486_FPU_DATA_REG FpuL2E = {0xB8AA3B295C17F0BBULL, FPU_REAL10_BIAS, FALSE};

/* lg(2) */
static const FAST486_FPU_DATA_REG FpuLgTwo = {0x9A209A84FBCFF798ULL, FPU_REAL10_BIAS - 2, FALSE};

/* ln(2) */
static const FAST486_FPU_DATA_REG FpuLnTwo = {0xB17217F7D1CF79ABULL, FPU_REAL10_BIAS - 1, FALSE};

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

static ULONGLONG
UnsignedDivMod128(ULONGLONG DividendLow,
                  ULONGLONG DividendHigh,
                  ULONGLONG Divisor,
                  PULONGLONG QuotientLow,
                  PULONGLONG QuotientHigh)
{
    ULONGLONG ValueLow = DividendLow;
    ULONGLONG ValueHigh = DividendHigh;
    ULONGLONG CurrentLow = 0ULL;
    ULONGLONG CurrentHigh = Divisor;
    ULONG Bits;

    ASSERT(Divisor != 0ULL);

    /* Initialize the quotient */
    *QuotientLow = *QuotientHigh = 0ULL;

    /* Exit early if the dividend is lower than the divisor */
    if ((DividendHigh == 0ULL) && (DividendLow < Divisor)) return ValueLow;

    /* Normalize the current divisor */
    Bits = CountLeadingZeros64(CurrentHigh);
    CurrentHigh <<= Bits;

    while (TRUE)
    {
        /* Shift the quotient left by one bit */
        *QuotientHigh <<= 1;
        *QuotientHigh |= *QuotientLow >> 63;
        *QuotientLow <<= 1;

        /* Check if the value is higher than or equal to the current divisor */
        if ((ValueHigh > CurrentHigh)
            || ((ValueHigh == CurrentHigh) && (ValueLow >= CurrentLow)))
        {
            BOOLEAN Carry = ValueLow < CurrentLow;

            /* Subtract the current divisor from the value */
            ValueHigh -= CurrentHigh;
            ValueLow -= CurrentLow;
            if (Carry) ValueHigh--;

            /* Set the lowest bit of the quotient */
            *QuotientLow |= 1;

            /* Stop if the value is lower than the original divisor */
            if ((ValueHigh == 0ULL) && (ValueLow < Divisor)) break;
        }

        /* Shift the current divisor right by one bit */
        CurrentLow >>= 1;
        CurrentLow |= (CurrentHigh & 1) << 63;
        CurrentHigh >>= 1;
    }

    /*
     * Calculate the number of significant bits the current
     * divisor has more than the original divisor
     */
    Bits = CountLeadingZeros64(Divisor) + 64;
    Bits -= (CurrentHigh > 0ULL) ? CountLeadingZeros64(CurrentHigh) : 64;
    Bits -= (CurrentLow > 0ULL) ? CountLeadingZeros64(CurrentLow) : 64;

    if (Bits)
    {
        /* Shift the quotient left by that amount */
        *QuotientHigh <<= Bits;
        *QuotientHigh |= *QuotientLow >> (64 - Bits);
        *QuotientLow <<= Bits;
    }

    /* Return the remainder */
    return ValueLow;
}

static inline VOID FASTCALL
Fast486FpuRound(PFAST486_STATE State,
                PULONGLONG Result,
                BOOLEAN Sign,
                ULONGLONG Remainder,
                INT RemainderHighBit)
{
    switch (State->FpuControl.Rc)
    {
        case FPU_ROUND_NEAREST:
        {
            /* Check if the highest bit of the remainder is set */
            if (Remainder & (1ULL << RemainderHighBit))
            {
                (*Result)++;

                /* Check if all the other bits are clear */
                if (!(Remainder & ((1ULL << RemainderHighBit) - 1)))
                {
                    /* Round to even */
                    *Result &= ~1;
                }
            }

            break;
        }

        case FPU_ROUND_DOWN:
        {
            if ((Remainder != 0ULL) && Sign) (*Result)++;
            break;
        }

        case FPU_ROUND_UP:
        {
            if ((Remainder != 0ULL) && !Sign) (*Result)++;
            break;
        }

        default:
        {
            /* Leave it truncated */
        }
    }
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
                    PCFAST486_FPU_DATA_REG Value,
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
        /* Raise an invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            *Result = 0LL;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    Bits = 63 - UnbiasedExp;

    /* Calculate the result and the remainder */
    *Result = (LONGLONG)(Value->Mantissa >> Bits);
    Remainder = Value->Mantissa & ((1 << Bits) - 1);

    /* The result must be positive here */
    ASSERT(*Result >= 0LL);

    /* Perform rounding */
    Fast486FpuRound(State, (PULONGLONG)Result, Value->Sign, Remainder, Bits - 1);

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

static inline BOOLEAN FASTCALL
Fast486FpuToSingleReal(PFAST486_STATE State,
                       PCFAST486_FPU_DATA_REG Value,
                       PULONG Result)
{
    ULONGLONG Remainder;
    SHORT UnbiasedExp = (SHORT)Value->Exponent - FPU_REAL10_BIAS;
    ULONGLONG Result64;

    if (FPU_IS_ZERO(Value))
    {
        *Result = 0;
        return TRUE;
    }

    /* Calculate the mantissa */
    *Result = (ULONG)(Value->Mantissa >> 40) & 0x7FFFFF;

    if (FPU_IS_NAN(Value))
    {
        *Result |= FPU_REAL4_INFINITY;
        goto SetSign;
    }

    /* Check for underflow */
    if (!FPU_IS_NORMALIZED(Value) || (UnbiasedExp < -127))
    {
        /* Raise the underflow exception */
        State->FpuStatus.Ue = TRUE;

        if (State->FpuControl.Um)
        {
            /* The result is zero due to underflow */
            *Result = 0ULL;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Check for overflow */
    if (UnbiasedExp > 127)
    {
        /* Raise the overflow exception */
        State->FpuStatus.Oe = TRUE;

        if (State->FpuControl.Om)
        {
            /* The result is infinity due to overflow */
            *Result = FPU_REAL4_INFINITY;
            goto SetSign;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Calculate the remainder */
    Remainder = Value->Mantissa & ((1ULL << 40) - 1);

    /* Perform rounding */
    Result64 = (ULONGLONG)*Result;
    Fast486FpuRound(State, &Result64, Value->Sign, Remainder, 39);
    *Result = (ULONG)Result64;

    /* Store the biased exponent */
    *Result |= (ULONG)(UnbiasedExp + FPU_REAL4_BIAS) << 23;

SetSign:

    if (Value->Sign) *Result |= 0x80000000;
    return TRUE;
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
                       PCFAST486_FPU_DATA_REG Value,
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
        *Result |= FPU_REAL8_INFINITY;
        goto SetSign;
    }

    /* Check for underflow */
    if (!FPU_IS_NORMALIZED(Value) || (UnbiasedExp < -1023))
    {
        /* Raise the underflow exception */
        State->FpuStatus.Ue = TRUE;

        if (State->FpuControl.Um)
        {
            /* The result is zero due to underflow */
            *Result = 0ULL;
            return TRUE;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Check for overflow */
    if (UnbiasedExp > 1023)
    {
        /* Raise the overflow exception */
        State->FpuStatus.Oe = TRUE;

        if (State->FpuControl.Om)
        {
            /* The result is infinity due to overflow */
            *Result = FPU_REAL8_INFINITY;
            goto SetSign;
        }
        else
        {
            Fast486FpuException(State);
            return FALSE;
        }
    }

    /* Calculate the remainder */
    Remainder = Value->Mantissa & ((1 << 11) - 1);

    /* Perform rounding */
    Fast486FpuRound(State, Result, Value->Sign, Remainder, 10);

    /* Store the biased exponent */
    *Result |= (ULONGLONG)(UnbiasedExp + FPU_REAL8_BIAS) << 52;

SetSign:

    if (Value->Sign) *Result |= 1ULL << 63;
    return TRUE;
}

static inline VOID FASTCALL
Fast486FpuFromPackedBcd(PFAST486_STATE State,
                        PUCHAR Value,
                        PFAST486_FPU_DATA_REG Result)
{
    INT i;
    LONGLONG IntVal = 0LL;

    for (i = 8; i >= 0; i--)
    {
        IntVal *= 100LL;
        IntVal += (Value[i] >> 4) * 10 + (Value[i] & 0x0F);
    }

    /* Apply the sign */
    if (Value[9] & 0x80) IntVal = -IntVal;

    /* Now convert the integer to FP80 */
    Fast486FpuFromInteger(State, IntVal, Result);
}

static inline BOOLEAN FASTCALL
Fast486FpuToPackedBcd(PFAST486_STATE State,
                      PCFAST486_FPU_DATA_REG Value,
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
              PCFAST486_FPU_DATA_REG FirstOperand,
              PCFAST486_FPU_DATA_REG SecondOperand,
              PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG FirstAdjusted = *FirstOperand;
    FAST486_FPU_DATA_REG SecondAdjusted = *SecondOperand;
    FAST486_FPU_DATA_REG TempResult;

    if ((!FPU_IS_NORMALIZED(FirstOperand) || !FPU_IS_NORMALIZED(SecondOperand)))
    {
        /* Raise the denormalized exception */
        State->FpuStatus.De = TRUE;

        if (!State->FpuControl.Dm)
        {
            Fast486FpuException(State);
            return;
        }
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
        if (FirstAdjusted.Sign) FirstAdjusted.Mantissa = -(LONGLONG)FirstAdjusted.Mantissa;
        if (SecondAdjusted.Sign) SecondAdjusted.Mantissa = -(LONGLONG)SecondAdjusted.Mantissa;

        /* Calculate the mantissa of the result */
        TempResult.Mantissa = FirstAdjusted.Mantissa + SecondAdjusted.Mantissa;
    }

    /* Did it overflow? */
    if (FPU_IS_NORMALIZED(&FirstAdjusted) && FPU_IS_NORMALIZED(&SecondAdjusted))
    {
        if (TempResult.Exponent == FPU_MAX_EXPONENT)
        {
            /* Raise the overflow exception */
            State->FpuStatus.Oe = TRUE;

            if (State->FpuControl.Om)
            {
                /* Total overflow, return infinity */
                TempResult.Mantissa = FPU_MANTISSA_HIGH_BIT;
                TempResult.Exponent = FPU_MAX_EXPONENT + 1;
            }
            else
            {
                Fast486FpuException(State);
                return;
            }
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
                   PCFAST486_FPU_DATA_REG FirstOperand,
                   PCFAST486_FPU_DATA_REG SecondOperand,
                   PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG NegativeSecondOperand = *SecondOperand;

    /* Invert the sign */
    NegativeSecondOperand.Sign = !NegativeSecondOperand.Sign;

    /* And perform an addition instead */
    Fast486FpuAdd(State, FirstOperand, &NegativeSecondOperand, Result);
}

static inline VOID FASTCALL
Fast486FpuCompare(PFAST486_STATE State,
                  PCFAST486_FPU_DATA_REG FirstOperand,
                  PCFAST486_FPU_DATA_REG SecondOperand)
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
                   PCFAST486_FPU_DATA_REG FirstOperand,
                   PCFAST486_FPU_DATA_REG SecondOperand,
                   PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG TempResult;
    LONG Exponent;

    if (FPU_IS_INDEFINITE(FirstOperand)
        || FPU_IS_INDEFINITE(SecondOperand)
        || (FPU_IS_ZERO(FirstOperand) && FPU_IS_INFINITY(SecondOperand))
        || (FPU_IS_INFINITY(FirstOperand) && FPU_IS_ZERO(SecondOperand)))
    {
        /* The result will be indefinite */
        Result->Sign = TRUE;
        Result->Exponent = FPU_MAX_EXPONENT + 1;
        Result->Mantissa = FPU_INDEFINITE_MANTISSA;
        return;
    }

    if (FPU_IS_ZERO(FirstOperand) || FPU_IS_ZERO(SecondOperand))
    {
        /* The result will be zero */
        Result->Sign = FirstOperand->Sign ^ SecondOperand->Sign;
        Result->Exponent = 0;
        Result->Mantissa = 0ULL;
        return;
    }

    if (FPU_IS_INFINITY(FirstOperand) || FPU_IS_INFINITY(SecondOperand))
    {
        /* The result will be infinity */
        Result->Sign = FirstOperand->Sign ^ SecondOperand->Sign;
        Result->Exponent = FPU_MAX_EXPONENT + 1;
        Result->Mantissa = FPU_MANTISSA_HIGH_BIT;
        return;
    }

    if ((!FPU_IS_NORMALIZED(FirstOperand) || !FPU_IS_NORMALIZED(SecondOperand)))
    {
        /* Raise the denormalized exception */
        State->FpuStatus.De = TRUE;

        if (!State->FpuControl.Dm)
        {
            Fast486FpuException(State);
            return;
        }
    }

    /* Calculate the sign */
    TempResult.Sign = FirstOperand->Sign ^ SecondOperand->Sign;

    /* Calculate the exponent */
    Exponent = (LONG)FirstOperand->Exponent + (LONG)SecondOperand->Exponent - FPU_REAL10_BIAS;

    /* Calculate the mantissa */
    UnsignedMult128(FirstOperand->Mantissa,
                    SecondOperand->Mantissa,
                    &TempResult.Mantissa);

    if (Exponent < 0)
    {
        /* Raise the underflow exception */
        State->FpuStatus.Ue = TRUE;

        if (!State->FpuControl.Um)
        {
            Fast486FpuException(State);
            return;
        }

        /* The exponent will be zero */
        TempResult.Exponent = 0;

        /* If possible, denormalize the result, otherwise make it zero */
        if (Exponent > -64) TempResult.Mantissa >>= (-Exponent);
        else TempResult.Mantissa = 0ULL;
    }
    else if (Exponent > FPU_MAX_EXPONENT)
    {
        /* Raise the overflow exception */
        State->FpuStatus.Oe = TRUE;

        if (!State->FpuControl.Om)
        {
            Fast486FpuException(State);
            return;
        }

        /* Make the result infinity */
        TempResult.Exponent = FPU_MAX_EXPONENT + 1;
        TempResult.Mantissa = FPU_MANTISSA_HIGH_BIT;
    }
    else TempResult.Exponent = (USHORT)Exponent;

    /* Normalize the result */
    Fast486FpuNormalize(State, &TempResult);
    *Result = TempResult;
}

static inline VOID FASTCALL
Fast486FpuDivide(PFAST486_STATE State,
                 PCFAST486_FPU_DATA_REG FirstOperand,
                 PCFAST486_FPU_DATA_REG SecondOperand,
                 PFAST486_FPU_DATA_REG Result)
{
    FAST486_FPU_DATA_REG TempResult;
    ULONGLONG QuotientLow, QuotientHigh, Remainder;
    LONG Exponent;

    if (FPU_IS_INDEFINITE(FirstOperand)
        || FPU_IS_INDEFINITE(SecondOperand)
        || (FPU_IS_INFINITY(FirstOperand) && FPU_IS_INFINITY(SecondOperand))
        || (FPU_IS_ZERO(FirstOperand) && FPU_IS_ZERO(SecondOperand)))
    {
        /* Raise the invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            /* Return the indefinite NaN */
            Result->Sign = TRUE;
            Result->Exponent = FPU_MAX_EXPONENT + 1;
            Result->Mantissa = FPU_INDEFINITE_MANTISSA;
        }
        else Fast486FpuException(State);

        return;
    }

    if (FPU_IS_ZERO(SecondOperand) || FPU_IS_INFINITY(FirstOperand))
    {
        /* Raise the division by zero exception */
        State->FpuStatus.Ze = TRUE;

        if (State->FpuControl.Zm)
        {
            /* Return infinity */
            Result->Sign = FirstOperand->Sign;
            Result->Exponent = FPU_MAX_EXPONENT + 1;
            Result->Mantissa = FPU_MANTISSA_HIGH_BIT;
        }
        else Fast486FpuException(State);

        return;
    }

    /* Calculate the sign of the result */
    TempResult.Sign = FirstOperand->Sign ^ SecondOperand->Sign;

    if (FPU_IS_ZERO(FirstOperand) || FPU_IS_INFINITY(SecondOperand))
    {
        /* Return zero */
        Result->Sign = TempResult.Sign;
        Result->Mantissa = 0ULL;
        Result->Exponent = 0;
        return;
    }

    /* Calculate the exponent of the result */
    Exponent = (LONG)FirstOperand->Exponent - (LONG)SecondOperand->Exponent - 64;

    /* Divide the two mantissas */
    Remainder = UnsignedDivMod128(0ULL,
                                  /* Notice the 64 above - this is the high part */
                                  FirstOperand->Mantissa,
                                  SecondOperand->Mantissa,
                                  &QuotientLow,
                                  &QuotientHigh);
    UNREFERENCED_PARAMETER(Remainder); // TODO: Rounding

    TempResult.Mantissa = QuotientLow;

    if (QuotientHigh > 0ULL)
    {
        ULONG BitsToShift = 64 - CountLeadingZeros64(QuotientHigh);

        TempResult.Mantissa >>= BitsToShift;
        TempResult.Mantissa |= QuotientHigh << (64 - BitsToShift);
        Exponent += BitsToShift;

        // TODO: Rounding
    }

    if (Exponent < -FPU_REAL10_BIAS)
    {
        TempResult.Mantissa >>= -(Exponent + FPU_REAL10_BIAS);
        Exponent = -FPU_REAL10_BIAS;

        // TODO: Rounding
    }

    TempResult.Exponent = (USHORT)(Exponent + FPU_REAL10_BIAS);

    /* Normalize the result */
    Fast486FpuNormalize(State, &TempResult);
    *Result = TempResult;
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

static inline BOOLEAN FASTCALL
Fast486FpuLoadEnvironment(PFAST486_STATE State,
                          INT Segment,
                          ULONG Address,
                          BOOLEAN Size)
{
    UCHAR Buffer[28];

    if (!Fast486ReadMemory(State, Segment, Address, FALSE, Buffer, (Size + 1) * 14))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Check if this is a 32-bit save or a 16-bit save */
    if (Size)
    {
        PULONG Data = (PULONG)Buffer;

        State->FpuControl.Value = (USHORT)Data[0];
        State->FpuStatus.Value = (USHORT)Data[1];
        State->FpuTag = (USHORT)Data[2];
        State->FpuLastInstPtr.Long = Data[3];
        State->FpuLastCodeSel = (USHORT)Data[4];
        State->FpuLastOpPtr.Long = Data[5];
        State->FpuLastDataSel = (USHORT)Data[6];
    }
    else
    {
        PUSHORT Data = (PUSHORT)Buffer;

        State->FpuControl.Value = Data[0];
        State->FpuStatus.Value = Data[1];
        State->FpuTag = Data[2];
        State->FpuLastInstPtr.LowWord = Data[3];
        State->FpuLastCodeSel = Data[4];
        State->FpuLastOpPtr.LowWord = Data[5];
        State->FpuLastDataSel = Data[6];
    }

    return TRUE;
}

static inline BOOLEAN FASTCALL
Fast486FpuSaveEnvironment(PFAST486_STATE State,
                          INT Segment,
                          ULONG Address,
                          BOOLEAN Size)
{
    UCHAR Buffer[28];

    /* Check if this is a 32-bit save or a 16-bit save */
    if (Size)
    {
        PULONG Data = (PULONG)Buffer;

        Data[0] = (ULONG)State->FpuControl.Value;
        Data[1] = (ULONG)State->FpuStatus.Value;
        Data[2] = (ULONG)State->FpuTag;
        Data[3] = State->FpuLastInstPtr.Long;
        Data[4] = (ULONG)State->FpuLastCodeSel;
        Data[5] = State->FpuLastOpPtr.Long;
        Data[6] = (ULONG)State->FpuLastDataSel;
    }
    else
    {
        PUSHORT Data = (PUSHORT)Buffer;

        Data[0] = State->FpuControl.Value;
        Data[1] = State->FpuStatus.Value;
        Data[2] = State->FpuTag;
        Data[3] = State->FpuLastInstPtr.LowWord;
        Data[4] = State->FpuLastCodeSel;
        Data[5] = State->FpuLastOpPtr.LowWord;
        Data[6] = State->FpuLastDataSel;
    }

    return Fast486WriteMemory(State, Segment, Address, Buffer, (Size + 1) * 14);
}

#endif

/* PUBLIC FUNCTIONS ***********************************************************/

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD8)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    PFAST486_FPU_DATA_REG SourceOperand, DestOperand;
    FAST486_FPU_DATA_REG MemoryData;

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

    /* The destination operand is ST0 */
    DestOperand = &FPU_ST(0);

    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
    {
        /* Raise the invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            /* Return the indefinite NaN */
            DestOperand->Sign = TRUE;
            DestOperand->Exponent = FPU_MAX_EXPONENT + 1;
            DestOperand->Mantissa = FPU_INDEFINITE_MANTISSA;

            FPU_SET_TAG(0, FPU_TAG_SPECIAL);
        }
        else Fast486FpuException(State);

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

        FPU_SAVE_LAST_OPERAND();
    }
    else
    {
        if (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
        {
            /* Raise the invalid operation exception */
            State->FpuStatus.Ie = TRUE;

            if (State->FpuControl.Im)
            {
                /* Return the indefinite NaN */
                DestOperand->Sign = TRUE;
                DestOperand->Exponent = FPU_MAX_EXPONENT + 1;
                DestOperand->Mantissa = FPU_INDEFINITE_MANTISSA;

                FPU_SET_TAG(0, FPU_TAG_SPECIAL);
            }
            else Fast486FpuException(State);

            return;
        }

        /* Load the source operand from an FPU register */
        SourceOperand = &FPU_ST(ModRegRm.SecondRegister);
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, SourceOperand, DestOperand);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD9)
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

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    if (ModRegRm.Memory)
    {
        switch (ModRegRm.Register)
        {
            /* FLD */
            case 0:
            {
                ULONG Value;
                FAST486_FPU_DATA_REG MemoryData;

                FPU_SAVE_LAST_INST();
                FPU_SAVE_LAST_OPERAND();

                if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, &Value))
                {
                    /* Exception occurred */
                    return;
                }

                Fast486FpuFromSingleReal(State, Value, &MemoryData);
                Fast486FpuPush(State, &MemoryData);

                break;
            }

            /* FST */
            case 2:
            /* FSTP */
            case 3:
            {
                ULONG Value = FPU_REAL4_INDEFINITE;

                FPU_SAVE_LAST_INST();
                FPU_SAVE_LAST_OPERAND();

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }
                else if (!Fast486FpuToSingleReal(State, &FPU_ST(0), &Value))
                {
                    /* Exception occurred */
                    return;
                }

                if (!Fast486WriteModrmDwordOperands(State, &ModRegRm, FALSE, Value))
                {
                    /* Exception occurred */
                    return;
                }

                if (ModRegRm.Register == 3) Fast486FpuPop(State);
                break;
            }

            /* FLDENV */
            case 4:
            {
                Fast486FpuLoadEnvironment(State,
                                          (State->PrefixFlags & FAST486_PREFIX_SEG)
                                          ? FAST486_REG_DS : State->SegmentOverride,
                                          ModRegRm.MemoryAddress,
                                          OperandSize);
                break;
            }

            /* FLDCW */
            case 5:
            {
                Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, &State->FpuControl.Value);
                break;
            }

            /* FSTENV */
            case 6:
            {
                Fast486FpuSaveEnvironment(State,
                                          (State->PrefixFlags & FAST486_PREFIX_SEG)
                                          ? FAST486_REG_DS : State->SegmentOverride,
                                          ModRegRm.MemoryAddress,
                                          OperandSize);
                break;
            }

            /* FSTCW */
            case 7:
            {
                Fast486WriteModrmWordOperands(State, &ModRegRm, FALSE, State->FpuControl.Value);
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
    else
    {
        switch ((ModRegRm.Register << 3) | ModRegRm.SecondRegister)
        {
            /* FLD */
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            {
                if (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }

                Fast486FpuPush(State, &FPU_ST(ModRegRm.SecondRegister));
                break;
            }

            /* FXCH */
            case 0x08:
            case 0x09:
            case 0x0A:
            case 0x0B:
            case 0x0C:
            case 0x0D:
            case 0x0E:
            case 0x0F:
            {
                FAST486_FPU_DATA_REG Temp;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                    || FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
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

            /* FNOP */
            case 0x10:
            {
                /* Do nothing */
                break;
            }

            /* FSTP */
            case 0x18:
            case 0x19:
            case 0x1A:
            case 0x1B:
            case 0x1C:
            case 0x1D:
            case 0x1E:
            case 0x1F:
            {
                FPU_ST(ModRegRm.SecondRegister) = FPU_ST(0);
                FPU_UPDATE_TAG(ModRegRm.SecondRegister);

                Fast486FpuPop(State);
                break;
            }

            /* FCHS */
            case 0x20:
            {
                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                /* Invert the sign */
                FPU_ST(0).Sign = !FPU_ST(0).Sign;

                break;
            }

            /* FABS */
            case 0x21:
            {
                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                /* Set the sign to positive */
                FPU_ST(0).Sign = FALSE;

                break;
            }

            /* FTST */
            case 0x24:
            {
                Fast486FpuCompare(State, &FPU_ST(0), &FpuZero);
                break;
            }

            /* FXAM */
            case 0x25:
            {
                /* The sign bit goes in C1, even if the register's empty */
                State->FpuStatus.Code1 = FPU_ST(0).Sign;

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Code0 = 1;
                    State->FpuStatus.Code2 = 0;
                    State->FpuStatus.Code3 = 1;
                }
                else if (FPU_GET_TAG(0) == FPU_TAG_SPECIAL)
                {
                    if (FPU_IS_INFINITY(&FPU_ST(0)))
                    {
                        State->FpuStatus.Code0 = 1;
                        State->FpuStatus.Code2 = 1;
                        State->FpuStatus.Code3 = 0;
                    }
                    else
                    {
                        State->FpuStatus.Code0 = 1;
                        State->FpuStatus.Code2 = 0;
                        State->FpuStatus.Code3 = 0;
                    }
                }
                else if (FPU_GET_TAG(0) == FPU_TAG_ZERO)
                {
                    State->FpuStatus.Code0 = 0;
                    State->FpuStatus.Code2 = 0;
                    State->FpuStatus.Code3 = 1;
                }
                else
                {
                    if (FPU_IS_NORMALIZED(&FPU_ST(0)))
                    {
                        State->FpuStatus.Code0 = 0;
                        State->FpuStatus.Code2 = 1;
                        State->FpuStatus.Code3 = 0;
                    }
                    else
                    {
                        State->FpuStatus.Code0 = 0;
                        State->FpuStatus.Code2 = 1;
                        State->FpuStatus.Code3 = 1;
                    }
                }

                break;
            }

            /* FLD1 */
            case 0x28:
            /* FLDL2T */
            case 0x29:
            /* FLDL2E */
            case 0x2A:
            /* FLDPI */
            case 0x2B:
            /* FLDLG2 */
            case 0x2C:
            /* FLDLN2 */
            case 0x2D:
            /* FLDZ */
            case 0x2E:
            {
                PCFAST486_FPU_DATA_REG Constants[] =
                {
                    &FpuOne,
                    &FpuL2Ten,
                    &FpuL2E,
                    &FpuPi,
                    &FpuLgTwo,
                    &FpuLnTwo,
                    &FpuZero
                };

                Fast486FpuPush(State, Constants[ModRegRm.SecondRegister]);
                break;
            }

            /* F2XM1 */
            case 0x30:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FYL2X */
            case 0x31:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FPTAN */
            case 0x32:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FPATAN */
            case 0x33:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FXTRACT */
            case 0x34:
            {
                FAST486_FPU_DATA_REG Value = FPU_ST(0);

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || FPU_IS_INDEFINITE(&Value))
                {
                    State->FpuStatus.Ie = TRUE;
                    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY) State->FpuStatus.Sf = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (FPU_IS_ZERO(&Value))
                {
                    /* The exponent of zero is negative infinity */
                    FPU_ST(0).Sign = TRUE;
                    FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
                    FPU_ST(0).Mantissa = FPU_MANTISSA_HIGH_BIT;
                }
                else if (FPU_IS_INFINITY(&Value))
                {
                    /* The exponent of infinity is positive infinity */
                    FPU_ST(0).Sign = FALSE;
                    FPU_ST(0).Exponent = FPU_MAX_EXPONENT + 1;
                    FPU_ST(0).Mantissa = FPU_MANTISSA_HIGH_BIT;
                }
                else
                {
                    /* Store the unbiased exponent in ST0 */
                    Fast486FpuFromInteger(State,
                                          (LONGLONG)Value.Exponent - (LONGLONG)FPU_REAL10_BIAS,
                                          &FPU_ST(0));
                }

                /* Now push the mantissa as a real number, with the original sign */
                Value.Exponent = FPU_REAL10_BIAS;
                Fast486FpuPush(State, &Value);

                break;
            }

            /* FPREM1 */
            case 0x35:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FDECSTP */
            case 0x36:
            {
                State->FpuStatus.Top--;
                break;
            }

            /* FINCSTP */
            case 0x37:
            {
                State->FpuStatus.Top++;
                break;
            }

            /* FPREM */
            case 0x38:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FYL2XP1 */
            case 0x39:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FSQRT */
            case 0x3A:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FSINCOS */
            case 0x3B:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FRNDINT */
            case 0x3C:
            {
                INT Bits;
                ULONGLONG Result = 0ULL;
                ULONGLONG Remainder;

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im) Fast486FpuException(State);
                    break;
                }

                if (!FPU_IS_NORMALIZED(&FPU_ST(0)))
                {
                    State->FpuStatus.De = TRUE;

                    if (!State->FpuControl.Dm)
                    {
                        Fast486FpuException(State);
                        break;
                    }
                }

                Bits = min(max(0, (INT)FPU_ST(0).Exponent - FPU_REAL10_BIAS + 1), 64);
                if (Bits == 64) break;

                if (Bits)
                {
                    Result = FPU_ST(0).Mantissa >> (64 - Bits);
                    Remainder = FPU_ST(0).Mantissa & ((1 << (64 - Bits)) - 1);
                }
                else Remainder = FPU_ST(0).Mantissa;

                /* Perform the rounding */
                Fast486FpuRound(State, &Result, FPU_ST(0).Sign, Remainder, 63 - Bits);

                State->FpuStatus.Pe = TRUE;
                if (!State->FpuControl.Pm) Fast486FpuException(State);

                break;
            }

            /* FSCALE */
            case 0x3D:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FSIN */
            case 0x3E:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

                break;
            }

            /* FCOS */
            case 0x3F:
            {
                // TODO: NOT IMPLEMENTED
                UNIMPLEMENTED;

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

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDA)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    PFAST486_FPU_DATA_REG SourceOperand, DestOperand;
    LONG Value;
    FAST486_FPU_DATA_REG MemoryData;

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

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
            /* Raise the invalid operation exception*/
            State->FpuStatus.Ie = TRUE;

            if (!State->FpuControl.Im) Fast486FpuException(State);
            return;
        }

        /* Compare */
        Fast486FpuCompare(State, &FPU_ST(0), &FPU_ST(1));

        /* Pop twice */
        Fast486FpuPop(State);
        Fast486FpuPop(State);

        return;
    }

    FPU_SAVE_LAST_OPERAND();

    /* Load the source operand from memory */
    if (!Fast486ReadModrmDwordOperands(State, &ModRegRm, NULL, (PULONG)&Value))
    {
        /* Exception occurred */
        return;
    }

    /* The destination operand is always ST0 */
    DestOperand = &FPU_ST(0);

    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
    {
        /* Raise the invalid operation exception */
        State->FpuStatus.Ie = TRUE;

        if (State->FpuControl.Im)
        {
            /* Return the indefinite NaN */
            DestOperand->Sign = TRUE;
            DestOperand->Exponent = FPU_MAX_EXPONENT + 1;
            DestOperand->Mantissa = FPU_INDEFINITE_MANTISSA;

            FPU_SET_TAG(0, FPU_TAG_SPECIAL);
        }
        else Fast486FpuException(State);

        return;
    }

    Fast486FpuFromInteger(State, (LONGLONG)Value, &MemoryData);
    SourceOperand = &MemoryData;

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, SourceOperand, DestOperand);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDB)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

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
        FPU_SAVE_LAST_INST();
        FPU_SAVE_LAST_OPERAND();

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
                LONGLONG Temp = 0;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(0) == FPU_TAG_SPECIAL))
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }
                else if (!Fast486FpuToInteger(State, &FPU_ST(0), &Temp))
                {
                    /* Exception occurred */
                    return;
                }

                /* Check if it can fit in a signed 32-bit integer */
                if ((((ULONGLONG)Temp >> 31) + 1ULL) > 1ULL)
                {
                    State->FpuStatus.Ie = TRUE;

                    if (State->FpuControl.Im) Temp = 0LL;
                    else
                    {
                        Fast486FpuException(State);
                        return;
                    }
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

                if (FPU_GET_TAG(0) != FPU_TAG_EMPTY)
                {
                    *((PULONGLONG)Buffer) = FPU_ST(0).Mantissa;
                    *((PUSHORT)&Buffer[sizeof(ULONGLONG)]) = FPU_ST(0).Exponent
                                                             | (FPU_ST(0).Sign ? 0x8000 : 0);
                }
                else
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (State->FpuControl.Im)
                    {
                        *((PULONGLONG)Buffer) = FPU_INDEFINITE_MANTISSA;
                        *((PUSHORT)&Buffer[sizeof(ULONGLONG)]) = 0x8000 | (FPU_MAX_EXPONENT + 1);
                    }
                    else
                    {
                        Fast486FpuException(State);
                        return;
                    }
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

            /* Invalid */
            default:
            {
                Fast486Exception(State, FAST486_EXCEPTION_UD);
                return;
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
            /* FSETPM */
            case 0x24:
            /* FRSTPM */
            case 0x25:
            {
                /* These do nothing */
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

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDC)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;
    PFAST486_FPU_DATA_REG SourceOperand, DestOperand;
    FAST486_FPU_DATA_REG MemoryData;

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

    if (ModRegRm.Memory)
    {
        ULONGLONG Value;

        /* The destination operand is ST0 */
        DestOperand = &FPU_ST(0);

        if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
        {
            /* Raise the invalid operation exception */
            State->FpuStatus.Ie = TRUE;

            if (State->FpuControl.Im)
            {
                /* Return the indefinite NaN */
                DestOperand->Sign = TRUE;
                DestOperand->Exponent = FPU_MAX_EXPONENT + 1;
                DestOperand->Mantissa = FPU_INDEFINITE_MANTISSA;

                FPU_SET_TAG(0, FPU_TAG_SPECIAL);
            }
            else Fast486FpuException(State);

            return;
        }

        /* Load the source operand from memory */
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

        FPU_SAVE_LAST_OPERAND();
    }
    else
    {
        /* The source operand is ST0 */
        SourceOperand = &FPU_ST(0);

        /* Load the destination operand from an FPU register */
        DestOperand = &FPU_ST(ModRegRm.SecondRegister);

        if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
            || (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY))
        {
            /* Raise the invalid operation exception */
            State->FpuStatus.Ie = TRUE;

            if (State->FpuControl.Im)
            {
                /* Return the indefinite NaN */
                DestOperand->Sign = TRUE;
                DestOperand->Exponent = FPU_MAX_EXPONENT + 1;
                DestOperand->Mantissa = FPU_INDEFINITE_MANTISSA;

                FPU_SET_TAG(ModRegRm.SecondRegister, FPU_TAG_SPECIAL);
            }
            else Fast486FpuException(State);

            return;
        }
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, SourceOperand, DestOperand);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDD)
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

                FPU_SAVE_LAST_INST();
                FPU_SAVE_LAST_OPERAND();

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
                ULONGLONG Value = FPU_REAL8_INDEFINITE;

                FPU_SAVE_LAST_INST();
                FPU_SAVE_LAST_OPERAND();

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }
                else if (!Fast486FpuToDoubleReal(State, &FPU_ST(0), &Value))
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
                INT i;
                UCHAR AllRegs[80];

                /* Save the environment */
                if (!Fast486FpuLoadEnvironment(State,
                                               (State->PrefixFlags & FAST486_PREFIX_SEG)
                                               ? FAST486_REG_DS : State->SegmentOverride,
                                               ModRegRm.MemoryAddress,
                                               OperandSize))
                {
                    /* Exception occurred */
                    return;
                }

                /* Load the registers */
                if (!Fast486ReadMemory(State,
                                       (State->PrefixFlags & FAST486_PREFIX_SEG)
                                       ? FAST486_REG_DS : State->SegmentOverride,
                                       ModRegRm.MemoryAddress + (OperandSize + 1) * 14,
                                       FALSE,
                                       AllRegs,
                                       sizeof(AllRegs)))
                {
                    /* Exception occurred */
                    return;
                }

                for (i = 0; i < FAST486_NUM_FPU_REGS; i++)
                {
                    State->FpuRegisters[i].Mantissa = *((PULONGLONG)&AllRegs[i * 10]);
                    State->FpuRegisters[i].Exponent = *((PUSHORT)&AllRegs[(i * 10) + sizeof(ULONGLONG)]) & 0x7FFF;

                    if (*((PUSHORT)&AllRegs[(i * 10) + sizeof(ULONGLONG)]) & 0x8000)
                    {
                        State->FpuRegisters[i].Sign = TRUE;
                    }
                    else
                    {
                        State->FpuRegisters[i].Sign = FALSE;
                    }
                }

                break;
            }

            /* FSAVE */
            case 6:
            {
                INT i;
                UCHAR AllRegs[80];

                /* Save the environment */
                if (!Fast486FpuSaveEnvironment(State,
                                               (State->PrefixFlags & FAST486_PREFIX_SEG)
                                               ? FAST486_REG_DS : State->SegmentOverride,
                                               ModRegRm.MemoryAddress,
                                               OperandSize))
                {
                    /* Exception occurred */
                    return;
                }

                /* Save the registers */
                for (i = 0; i < FAST486_NUM_FPU_REGS; i++)
                {
                    *((PULONGLONG)&AllRegs[i * 10]) = State->FpuRegisters[i].Mantissa;
                    *((PUSHORT)&AllRegs[(i * 10) + sizeof(ULONGLONG)]) = State->FpuRegisters[i].Exponent;

                    if (State->FpuRegisters[i].Sign)
                    {
                        *((PUSHORT)&AllRegs[(i * 10) + sizeof(ULONGLONG)]) |= 0x8000;
                    }
                }

                Fast486WriteMemory(State,
                                   (State->PrefixFlags & FAST486_PREFIX_SEG)
                                   ? FAST486_REG_DS : State->SegmentOverride,
                                   ModRegRm.MemoryAddress + (OperandSize + 1) * 14,
                                   AllRegs,
                                   sizeof(AllRegs));

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
        FPU_SAVE_LAST_INST();

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

                    if (!State->FpuControl.Im) Fast486FpuException(State);
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

                    if (!State->FpuControl.Im) Fast486FpuException(State);
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

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

    if (ModRegRm.Memory)
    {
        SHORT Value;
        FAST486_FPU_DATA_REG MemoryData;

        /* The destination operand is ST0 */
        DestOperand = &FPU_ST(0);

        if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
        {
            /* Raise the invalid operation exception */
            State->FpuStatus.Ie = TRUE;

            if (State->FpuControl.Im)
            {
                /* Return the indefinite NaN */
                DestOperand->Sign = TRUE;
                DestOperand->Exponent = FPU_MAX_EXPONENT + 1;
                DestOperand->Mantissa = FPU_INDEFINITE_MANTISSA;

                FPU_SET_TAG(0, FPU_TAG_SPECIAL);
            }
            else Fast486FpuException(State);

            return;
        }

        /* Load the source operand from memory */
        if (!Fast486ReadModrmWordOperands(State, &ModRegRm, NULL, (PUSHORT)&Value))
        {
            /* Exception occurred */
            return;
        }

        Fast486FpuFromInteger(State, (LONGLONG)Value, &MemoryData);
        SourceOperand = &MemoryData;

        FPU_SAVE_LAST_OPERAND();
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

        if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY)
            || (FPU_GET_TAG(ModRegRm.SecondRegister) == FPU_TAG_EMPTY))
        {
            /* Raise the invalid operation exception, if unmasked */
            State->FpuStatus.Ie = TRUE;

            if (!State->FpuControl.Im) Fast486FpuException(State);
            return;
        }
    }

    /* Perform the requested operation */
    Fast486FpuArithmeticOperation(State, ModRegRm.Register, SourceOperand, DestOperand);
    if (!ModRegRm.Memory) Fast486FpuPop(State);

#endif
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDF)
{
    FAST486_MOD_REG_RM ModRegRm;
    BOOLEAN AddressSize = State->SegmentRegs[FAST486_REG_CS].Size;

    TOGGLE_ADSIZE(AddressSize);

    /* Get the operands */
    if (!Fast486ParseModRegRm(State, AddressSize, &ModRegRm))
    {
        /* Exception occurred */
        return;
    }

    FPU_CHECK();

#ifndef FAST486_NO_FPU

    FPU_SAVE_LAST_INST();

    if (ModRegRm.Memory)
    {
        FPU_SAVE_LAST_OPERAND();

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
                LONGLONG Temp = 0LL;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(0) == FPU_TAG_SPECIAL))
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }
                else if (!Fast486FpuToInteger(State, &FPU_ST(0), &Temp))
                {
                    /* Exception occurred */
                    return;
                }

                /* Check if it can fit in a signed 16-bit integer */
                if ((((ULONGLONG)Temp >> 15) + 1ULL) > 1ULL)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (State->FpuControl.Im) Temp = 0LL;
                    else
                    {
                        Fast486FpuException(State);
                        return;
                    }
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

                Fast486FpuFromPackedBcd(State, Buffer, &Value);
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
                UCHAR Buffer[10] = {0};

                if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }
                else if (!Fast486FpuToPackedBcd(State, &FPU_ST(0), Buffer))
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
                LONGLONG Temp = 0LL;

                if ((FPU_GET_TAG(0) == FPU_TAG_EMPTY) || (FPU_GET_TAG(0) == FPU_TAG_SPECIAL))
                {
                    /* Raise the invalid operation exception */
                    State->FpuStatus.Ie = TRUE;

                    if (!State->FpuControl.Im)
                    {
                        Fast486FpuException(State);
                        return;
                    }
                }
                else if (!Fast486FpuToInteger(State, &FPU_ST(0), &Temp))
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

                    if (!State->FpuControl.Im) Fast486FpuException(State);
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
