/* $Id: math.c,v 1.1 2003/01/07 17:48:12 robd Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/math.c
 * PURPOSE:         
 * UPDATE HISTORY:
 *                  Created 20/12/2002
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

LARGE_INTEGER
STDCALL
_alldiv(ULONG	UnsignedInteger)
{
	LARGE_INTEGER RC;

	RC.QuadPart = 0;
	return RC;
}

LARGE_INTEGER
STDCALL
_allmul(ULONG	UnsignedInteger)
{
	LARGE_INTEGER RC;

	RC.QuadPart = 0;
	return RC;
}

LARGE_INTEGER
STDCALL
_allrem(ULONG	UnsignedInteger)
{
	LARGE_INTEGER RC;

	RC.QuadPart = 0;
	return RC;
}

LARGE_INTEGER
STDCALL
_allshl(ULONG	UnsignedInteger)
{
	LARGE_INTEGER RC;

	RC.QuadPart = 0;
	return RC;
}

LARGE_INTEGER
STDCALL
_allshr(ULONG	UnsignedInteger)
{
	LARGE_INTEGER RC;

	RC.QuadPart = 0;
	return RC;
}

LARGE_INTEGER
STDCALL
_aulldiv(ULONG	UnsignedInteger)
{
	LARGE_INTEGER RC;

	RC.QuadPart = 0;
	return RC;
}

LARGE_INTEGER
STDCALL
_aullrem(ULONG	UnsignedInteger)
{
	LARGE_INTEGER RC;

	RC.QuadPart = 0;
	return RC;
}

LARGE_INTEGER
STDCALL
_aullshr(ULONG	UnsignedInteger)
{
	LARGE_INTEGER RC;

	RC.QuadPart = 0;
	return RC;
}


/*
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

LARGE_INTEGER
STDCALL
RtlExtendedMagicDivide (LARGE_INTEGER	Dividend,
			LARGE_INTEGER	MagicDivisor,
			CCHAR		ShiftCount)
{
  LARGE_INTEGER Result;

  Result.QuadPart = (Dividend.QuadPart * MagicDivisor.QuadPart) >> ShiftCount;
  return(Result);
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
		// copy the sign bit
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
 */
/* EOF */
