/* $Id: largeint.c,v 1.14 2003/06/01 18:13:23 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/largeint.c
 * PURPOSE:         Large integer operations
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   08/30/98  RJJ  Implemented several functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

LARGE_INTEGER
STDCALL
RtlConvertLongToLargeInteger (
	LONG	SignedInteger
	)
{
	LARGE_INTEGER RC;

	RC.QuadPart = SignedInteger;

	return RC;
}

LARGE_INTEGER
STDCALL
RtlConvertUlongToLargeInteger (
	ULONG	UnsignedInteger
	)
{
	LARGE_INTEGER RC;

	RC.QuadPart = UnsignedInteger;

	return RC;
}

LARGE_INTEGER
STDCALL
RtlEnlargedIntegerMultiply (
	LONG	Multiplicand,
	LONG	Multiplier
	)
{
	LARGE_INTEGER RC;

	RC.QuadPart = (LONGLONG) Multiplicand * Multiplier;

	return RC;
}

ULONG
STDCALL
RtlEnlargedUnsignedDivide (
	ULARGE_INTEGER	Dividend,
	ULONG		Divisor,
	PULONG		Remainder
	)
{
	if (Remainder)
		*Remainder = Dividend.QuadPart % Divisor;

	return (ULONG)(Dividend.QuadPart / Divisor);
}

LARGE_INTEGER
STDCALL
RtlEnlargedUnsignedMultiply (
	ULONG	Multiplicand,
	ULONG	Multiplier
	)
{
	LARGE_INTEGER RC;

	RC.QuadPart = (ULONGLONG) Multiplicand * Multiplier;

	return RC;
}

LARGE_INTEGER
STDCALL
RtlExtendedIntegerMultiply (
	LARGE_INTEGER	Multiplicand,
	LONG		Multiplier
	)
{
	LARGE_INTEGER RC;

	RC.QuadPart = Multiplicand.QuadPart * Multiplier;

	return RC;
}

LARGE_INTEGER
STDCALL
RtlExtendedLargeIntegerDivide (
	LARGE_INTEGER	Dividend,
	ULONG		Divisor,
	PULONG		Remainder
	)
{
	LARGE_INTEGER RC;

	if (Remainder)
		*Remainder = Dividend.QuadPart % Divisor;

	RC.QuadPart = Dividend.QuadPart / Divisor;

	return RC;
}


/******************************************************************************
 * RtlExtendedMagicDivide
 *
 * Allows replacing a division by a longlong constant with a multiplication by
 * the inverse constant.
 *
 * RETURNS
 *  (Dividend * MagicDivisor) >> (64 + ShiftCount)
 *
 * NOTES
 *  If the divisor of a division is constant, the constants MagicDivisor and
 *  shift must be chosen such that
 *  MagicDivisor = 2^(64 + ShiftCount) / Divisor.
 *
 *  Then we have RtlExtendedMagicDivide(Dividend,MagicDivisor,ShiftCount) ==
 *  Dividend * MagicDivisor / 2^(64 + ShiftCount) == Dividend / Divisor.
 *
 *  The Parameter MagicDivisor although defined as LONGLONG is used as
 *  ULONGLONG.
 */

#define LOWER_32(A) ((A) & 0xffffffff)
#define UPPER_32(A) ((A) >> 32)

LARGE_INTEGER STDCALL
RtlExtendedMagicDivide (LARGE_INTEGER Dividend,
			LARGE_INTEGER MagicDivisor,
			CCHAR ShiftCount)
{
  ULONGLONG dividend_high;
  ULONGLONG dividend_low;
  ULONGLONG inverse_divisor_high;
  ULONGLONG inverse_divisor_low;
  ULONGLONG ah_bl;
  ULONGLONG al_bh;
  LARGE_INTEGER result;
  BOOLEAN positive;

  if (Dividend.QuadPart < 0)
    {
      dividend_high = UPPER_32((ULONGLONG) -Dividend.QuadPart);
      dividend_low =  LOWER_32((ULONGLONG) -Dividend.QuadPart);
      positive = FALSE;
    }
  else
    {
      dividend_high = UPPER_32((ULONGLONG) Dividend.QuadPart);
      dividend_low =  LOWER_32((ULONGLONG) Dividend.QuadPart);
      positive = TRUE;
    }
  inverse_divisor_high = UPPER_32((ULONGLONG) MagicDivisor.QuadPart);
  inverse_divisor_low =  LOWER_32((ULONGLONG) MagicDivisor.QuadPart);

  ah_bl = dividend_high * inverse_divisor_low;
  al_bh = dividend_low * inverse_divisor_high;

  result.QuadPart =
    (LONGLONG) ((dividend_high * inverse_divisor_high +
		 UPPER_32(ah_bl) +
		 UPPER_32(al_bh) +
		 UPPER_32(LOWER_32(ah_bl) + LOWER_32(al_bh) +
			  UPPER_32(dividend_low * inverse_divisor_low))) >> ShiftCount);
  if (!positive)
    {
      result.QuadPart = -result.QuadPart;
    }

  return result;
}


LARGE_INTEGER
STDCALL
RtlLargeIntegerAdd (
	LARGE_INTEGER	Addend1,
	LARGE_INTEGER	Addend2
	)
{
	LARGE_INTEGER RC;

	RC.QuadPart = Addend1.QuadPart + Addend2.QuadPart;

	return RC;
}

LARGE_INTEGER
STDCALL
RtlLargeIntegerArithmeticShift (
	LARGE_INTEGER	LargeInteger,
	CCHAR		ShiftCount
	)
{
	LARGE_INTEGER RC;
	CHAR Shift;

	Shift = ShiftCount % 64;

	if (Shift < 32)
	{
		RC.QuadPart = LargeInteger.QuadPart >> Shift;
	}
	else
	{
		/* copy the sign bit */
		RC.u.HighPart |= (LargeInteger.u.HighPart & 0x80000000);
		RC.u.LowPart = LargeInteger.u.HighPart >> Shift;
	}

	return RC;
}

LARGE_INTEGER
STDCALL
RtlLargeIntegerDivide (
	LARGE_INTEGER	Dividend,
	LARGE_INTEGER	Divisor,
	PLARGE_INTEGER	Remainder
	)
{
	LARGE_INTEGER RC;

	if (Remainder)
		Remainder->QuadPart = Dividend.QuadPart % Divisor.QuadPart;

	RC.QuadPart = Dividend.QuadPart / Divisor.QuadPart;

	return RC;
}

LARGE_INTEGER
STDCALL
RtlLargeIntegerNegate (
	LARGE_INTEGER	Subtrahend
	)
{
	LARGE_INTEGER RC;

	RC.QuadPart = - Subtrahend.QuadPart;

	return RC;
}

LARGE_INTEGER
STDCALL
RtlLargeIntegerShiftLeft (
	LARGE_INTEGER	LargeInteger,
	CCHAR		ShiftCount
	)
{
	LARGE_INTEGER RC;
	CHAR Shift;

	Shift = ShiftCount % 64;
	RC.QuadPart = LargeInteger.QuadPart << Shift;

	return RC;
}

LARGE_INTEGER
STDCALL
RtlLargeIntegerShiftRight (
	LARGE_INTEGER	LargeInteger,
	CCHAR		ShiftCount
	)
{
	LARGE_INTEGER RC;
	CHAR Shift;

	Shift = ShiftCount % 64;
	RC.QuadPart = LargeInteger.QuadPart >> ShiftCount;

	return RC;
}

LARGE_INTEGER
STDCALL
RtlLargeIntegerSubtract (
	LARGE_INTEGER	Minuend,
	LARGE_INTEGER	Subtrahend
	)
{
	LARGE_INTEGER RC;

	RC.QuadPart = Minuend.QuadPart - Subtrahend.QuadPart;

	return RC;
}

/* EOF */
