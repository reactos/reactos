/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/largeint.c
 * PURPOSE:         Large integer operations
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   08/30/98  RJJ  Implemented several functions
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

typedef long long int LLI, *PLLI;
typedef unsigned long long int ULLI, *PULLI;

#define LIFromLLI(X) (*(PLARGE_INTEGER)&(X))
#define LLIFromLI(X) (*(PLLI)&(X))
#define ULIFromULLI(X) (*(PULARGE_INTEGER)&(X))

/* FUNCTIONS *****************************************************************/

LARGE_INTEGER 
RtlConvertLongToLargeInteger(LONG SignedInteger)
{
  LLI RC;

  RC = SignedInteger;

  return LIFromLLI(RC);
}

LARGE_INTEGER 
RtlConvertUlongToLargeInteger(ULONG UnsignedInteger)
{
  LLI RC;

  RC = UnsignedInteger;

  return LIFromLLI(RC);
}

LARGE_INTEGER 
RtlEnlargedIntegerMultiply(LONG Multiplicand,
                           LONG Multiplier)
{
  LLI RC;

  RC = (LLI) Multiplicand * Multiplier;

  return LIFromLLI(RC);
}

ULONG RtlEnlargedUnsignedDivide(ULARGE_INTEGER Dividend,
				ULONG Divisor,
				PULONG Remainder)
{
  UNIMPLEMENTED;
}

LARGE_INTEGER RtlEnlargedUnsignedMultiply(ULONG Multiplicand,
					  ULONG Multiplier)
{
  LLI RC;

  RC = (ULLI) Multiplicand * Multiplier;

  return LIFromLLI(RC);
}

LARGE_INTEGER 
RtlExtendedIntegerMultiply(LARGE_INTEGER Multiplicand,
                           LONG Multiplier)
{
  LLI M1, RC;

  M1 = LLIFromLI(Multiplicand);
  RC = M1 * Multiplier;

  return LIFromLLI(RC);
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

LARGE_INTEGER 
RtlLargeIntegerAdd(LARGE_INTEGER Addend1,
                   LARGE_INTEGER Addend2)
{
  LLI RC, A1, A2;

  A1 = LLIFromLI(Addend1);
  A2 = LLIFromLI(Addend2);
  RC = A1 + A2;

  return LIFromLLI(RC);
}

VOID RtlLargeIntegerAnd(PLARGE_INTEGER Result,
			LARGE_INTEGER Source,
			LARGE_INTEGER Mask)
{
  Result->HighPart = Source.HighPart & Mask.HighPart;
  Result->LowPart = Source.LowPart & Mask.LowPart;
}

LARGE_INTEGER RtlLargeIntegerArithmeticShift(LARGE_INTEGER LargeInteger,
					     CCHAR ShiftCount)
{
  LARGE_INTEGER RC;

  asm ("movb %2, %%cl\n\t"
       "andb $0x3f, %%cl\n\t"
       "movl %3, %%eax\n\t"
       "movl %4,  %%edx\n\t"
       "shrdl %%cl, %%edx, %%eax\n\t"
       "sarl %%cl, %%edx\n\t"
       "movl %%eax, %0\n\t"
       "movl %%edx, %1\n\t"
       : "=m" (LargeInteger.LowPart), "=m" (LargeInteger.HighPart)
       : "m" (ShiftCount), "0" (LargeInteger.LowPart), "1" (LargeInteger.HighPart)
       : "eax", "ecx", "edx"
       );

  return RC;
}

LARGE_INTEGER RtlLargeIntegerDivide(LARGE_INTEGER Dividend,
				    LARGE_INTEGER Divisor,
				    PLARGE_INTEGER Remainder)
{
  UNIMPLEMENTED;
}

BOOLEAN 
RtlLargeIntegerEqualTo(LARGE_INTEGER Operand1,
                       LARGE_INTEGER Operand2)
{
  return Operand1.HighPart == Operand2.HighPart && 
         Operand1.LowPart == Operand2.LowPart;
}

BOOLEAN 
RtlLargeIntegerEqualToZero(LARGE_INTEGER Operand)
{
  return Operand.LowPart == 0 && Operand.HighPart == 0;
}

BOOLEAN 
RtlLargeIntegerGreaterThan(LARGE_INTEGER Operand1,
                           LARGE_INTEGER Operand2)
{
  return Operand1.HighPart > Operand2.HighPart ||
         (Operand1.HighPart == Operand2.HighPart && 
          Operand1.LowPart > Operand2.LowPart);
}

BOOLEAN 
RtlLargeIntegerGreaterThanOrEqualTo(LARGE_INTEGER Operand1,
                                    LARGE_INTEGER Operand2)
{
  return Operand1.HighPart > Operand2.HighPart ||
         (Operand1.HighPart == Operand2.HighPart && 
          Operand1.LowPart >= Operand2.LowPart);
}

BOOLEAN 
RtlLargeIntegerGreaterThanOrEqualToZero(LARGE_INTEGER Operand1)
{
  return Operand1.HighPart >= 0;
}

BOOLEAN 
RtlLargeIntegerGreaterThanZero(LARGE_INTEGER Operand1)
{
  return Operand1.HighPart > 0 || 
         (Operand1.HighPart == 0 && Operand1.LowPart > 0);
}

BOOLEAN 
RtlLargeIntegerLessThan(LARGE_INTEGER Operand1,
                        LARGE_INTEGER Operand2)
{
  return Operand1.HighPart < Operand2.HighPart ||
         (Operand1.HighPart == Operand2.HighPart && 
          Operand1.LowPart < Operand2.LowPart);
}

BOOLEAN 
RtlLargeIntegerLessThanOrEqualTo(LARGE_INTEGER Operand1,
                                 LARGE_INTEGER Operand2)
{
  return Operand1.HighPart < Operand2.HighPart ||
         (Operand1.HighPart == Operand2.HighPart && 
          Operand1.LowPart <= Operand2.LowPart);
}

BOOLEAN 
RtlLargeIntegerLessThanOrEqualToZero(LARGE_INTEGER Operand)
{
  return Operand.HighPart < 0 || 
         (Operand.HighPart == 0 && Operand.LowPart == 0);
}

BOOLEAN 
RtlLargeIntegerLessThanZero(LARGE_INTEGER Operand)
{
  return Operand.HighPart < 0;
}

LARGE_INTEGER RtlLargeIntegerNegate(LARGE_INTEGER Subtrahend)
{
  LLI RC;

  RC = - LLIFromLI(Subtrahend);

  return LIFromLLI(RC);
}

BOOLEAN 
RtlLargeIntegerNotEqualTo(LARGE_INTEGER Operand1,
                          LARGE_INTEGER Operand2)
{
  return Operand1.LowPart != Operand2.LowPart || 
         Operand1.HighPart != Operand2.HighPart; 
}

BOOLEAN 
RtlLargeIntegerNotEqualToZero(LARGE_INTEGER Operand)
{
  return Operand.LowPart != 0 || Operand.HighPart != 0; 
}

LARGE_INTEGER RtlLargeIntegerShiftLeft(LARGE_INTEGER LargeInteger,
				       CCHAR ShiftCount)
{
  LLI RC;

  RC = LLIFromLI(LargeInteger);
  RC = RC << ShiftCount;

  return LIFromLLI(RC);
}

LARGE_INTEGER RtlLargeIntegerShiftRight(LARGE_INTEGER LargeInteger,
					CCHAR ShiftCount)
{
  LLI RC;

  RC = LLIFromLI(LargeInteger);
  RC = RC >> ShiftCount;

  return LIFromLLI(RC);
}

LARGE_INTEGER RtlLargeIntegerSubtract(LARGE_INTEGER Minuend,
				      LARGE_INTEGER Subtrahend)
{
  LLI S1, S2, RC;

  S1 = LLIFromLI(Minuend);
  S2 = LLIFromLI(Subtrahend);
  RC = S1 - S2;

  return LIFromLLI(RC);
}

