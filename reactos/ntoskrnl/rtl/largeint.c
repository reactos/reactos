/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/largeint.c
 * PURPOSE:         Large integer operations
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

LARGE_INTEGER RtlConvertLongToLargeInteger(LONG SignedInteger)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlConvertUlongToLargeInteger(ULONG UnsignedInteger)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlEnlargedIntegerMultiply(LONG Multiplicand,
					 LONG Multipler)
{
   UNIMPLEMENTED;
}

ULONG RtlEnlargedUnsignedDivide(ULARGE_INTEGER Dividend,
				ULONG Divisor,
				PULONG Remainder)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlEnlargedUnsignedMultiply(ULONG Multiplicand,
					  ULONG Multipler)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlExtendedIntegerMultiply(LARGE_INTEGER Multiplicand,
					 LONG Multiplier)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlExtendedLargeIntegerDivide(LARGE_INTEGER Dividend,
					    ULONG Divisor,
					    PULONG Remainder)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlExtendedMagicDivide(LARGE_INTEGER Dividend,
				     LARGE_INTEGER MagicDivisor,
				     CCHAR ShiftCount)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER ExInterlockedAddLargeInteger(PLARGE_INTEGER Addend,
					   LARGE_INTEGER Increment,
					   PKSPIN_LOCK Lock)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlLargeIntegerAdd(LARGE_INTEGER Addend1,
				 LARGE_INTEGER Addend2)
{
   UNIMPLEMENTED;
}

VOID RtlLargeIntegerAnd(PLARGE_INTEGER Result,
			LARGE_INTEGER Source,
			LARGE_INTEGER Mask)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlLargeIntegerArithmeticShift(LARGE_INTEGER LargeInteger,
					     CCHAR ShiftCount)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlLargeIntegerDivide(LARGE_INTEGER Dividend,
				    LARGE_INTEGER Divisor,
				    PLARGE_INTEGER Remainder)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerEqualTo(LARGE_INTEGER Operand1,
			       LARGE_INTEGER Operand2)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerEqualToZero(LARGE_INTEGER Operand)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerGreaterThan(LARGE_INTEGER Operand1,
				   LARGE_INTEGER Operand2)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerGreaterThanOrEqualTo(LARGE_INTEGER Operand1,
					    LARGE_INTEGER Operand2)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerGreaterThanOrEqualToZero(LARGE_INTEGER Operand1)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerGreaterThanZero(LARGE_INTEGER Operand1)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerLessThan(LARGE_INTEGER Operand1,
				   LARGE_INTEGER Operand2)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerLessThanOrEqualTo(LARGE_INTEGER Operand1,
					 LARGE_INTEGER Operand2)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerLessThanOrEqualToZero(LARGE_INTEGER Operand1)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerLessThanZero(LARGE_INTEGER Operand1)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlLargeIntegerNegate(LARGE_INTEGER Subtrahend)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerNotEqualTo(LARGE_INTEGER Operand1,
				  LARGE_INTEGER Operand2)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlLargeIntegerNotEqualToZero(LARGE_INTEGER Operand)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlLargeIntegerShiftLeft(LARGE_INTEGER LargeInteger,
				       CCHAR ShiftCount)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlLargeIntegerShiftRight(LARGE_INTEGER LargeInteger,
					CCHAR ShiftCount)
{
   UNIMPLEMENTED;
}

LARGE_INTEGER RtlLargeIntegerSubtract(LARGE_INTEGER Minuend,
				      LARGE_INTEGER Subtrahend)
{
   UNIMPLEMENTED;
}
