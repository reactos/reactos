/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/largeint.c
 * PURPOSE:         Large integer operations
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   08/30/98  RJJ  Implemented several functions
 */

/* INCLUDES *****************************************************************/

#include <internal/ke.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#define NDEBUG
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
  LARGE_INTEGER RC;

  RC.QuadPart = SignedInteger;

  return RC;
}

LARGE_INTEGER 
RtlConvertUlongToLargeInteger(ULONG UnsignedInteger)
{
  LARGE_INTEGER RC;

  RC.QuadPart = UnsignedInteger;

  return RC;
}

LARGE_INTEGER 
RtlEnlargedIntegerMultiply(LONG Multiplicand,
                           LONG Multiplier)
{
  LARGE_INTEGER RC;

  RC.QuadPart = (LONGLONG) Multiplicand * Multiplier;

  return RC;
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
  LARGE_INTEGER RC;

  RC.QuadPart = (ULONGLONG) Multiplicand * Multiplier;

  return RC;
}

LARGE_INTEGER 
RtlExtendedIntegerMultiply(LARGE_INTEGER Multiplicand,
                           LONG Multiplier)
{
  LARGE_INTEGER RC;

  RC.QuadPart = Multiplicand.QuadPart * Multiplier;

  return RC;
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
  LARGE_INTEGER RC;

  RC.QuadPart = Addend1.QuadPart + Addend2.QuadPart;

  return RC;
}

VOID RtlLargeIntegerAnd(PLARGE_INTEGER Result,
			LARGE_INTEGER Source,
			LARGE_INTEGER Mask)
{
  Result->QuadPart = Source.QuadPart & Mask.QuadPart;
}

LARGE_INTEGER RtlLargeIntegerArithmeticShift(LARGE_INTEGER LargeInteger,
					     CCHAR ShiftCount)
{
  UNIMPLEMENTED;
#if 0
  LARGE_INTEGER RC;


  RC.QuadPart = LargeInteger.QuadPart >> ShiftCount;
  asm ("movb %2, %%cl\n\t"
       "andb $0x3f, %%cl\n\t"
       "movl %3, %%eax\n\t"
       "movl %4,  %%edx\n\t"
       "shrdl %%cl, %%edx, %%eax\n\t"
       "sarl %%cl, %%edx\n\t"
       "movl %%eax, %0\n\t"
       "movl %%edx, %1\n\t"
       : "=m" (LargeInteger.LowPart), 
         "=m" (LargeInteger.HighPart)
       : "m" (ShiftCount), 
         "0" (LargeInteger.LowPart), 
         "1" (LargeInteger.HighPart)
       : "eax", "ecx", "edx"
       );

  return RC;
#endif
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
  return Operand1.QuadPart == Operand2.QuadPart;
#if 0
  return Operand1.HighPart == Operand2.HighPart && 
         Operand1.LowPart == Operand2.LowPart;
#endif
}

BOOLEAN 
RtlLargeIntegerEqualToZero(LARGE_INTEGER Operand)
{
  return Operand.QuadPart == 0 ;
}

BOOLEAN 
RtlLargeIntegerGreaterThan(LARGE_INTEGER Operand1,
                           LARGE_INTEGER Operand2)
{
  return Operand1.QuadPart > Operand2.QuadPart;
#if 0
  return Operand1.HighPart > Operand2.HighPart ||
         (Operand1.HighPart == Operand2.HighPart && 
          Operand1.LowPart > Operand2.LowPart);
#endif
}

BOOLEAN 
RtlLargeIntegerGreaterThanOrEqualTo(LARGE_INTEGER Operand1,
                                    LARGE_INTEGER Operand2)
{
  return Operand1.QuadPart >= Operand2.QuadPart;
#if 0
  return Operand1.HighPart > Operand2.HighPart ||
         (Operand1.HighPart == Operand2.HighPart && 
          Operand1.LowPart >= Operand2.LowPart);
#endif
}

BOOLEAN 
RtlLargeIntegerGreaterThanOrEqualToZero(LARGE_INTEGER Operand1)
{
  return Operand1.QuadPart >= 0;
#if 0
  return Operand1.HighPart >= 0;
#endif
}

BOOLEAN 
RtlLargeIntegerGreaterThanZero(LARGE_INTEGER Operand1)
{
  return Operand1.QuadPart > 0; 
#if 0
  return Operand1.HighPart > 0 || 
         (Operand1.HighPart == 0 && Operand1.LowPart > 0);
#endif
}

BOOLEAN 
RtlLargeIntegerLessThan(LARGE_INTEGER Operand1,
                        LARGE_INTEGER Operand2)
{
  return Operand1.QuadPart < Operand2.QuadPart;
#if 0
  return Operand1.HighPart < Operand2.HighPart ||
         (Operand1.HighPart == Operand2.HighPart && 
          Operand1.LowPart < Operand2.LowPart);
#endif
}

BOOLEAN 
RtlLargeIntegerLessThanOrEqualTo(LARGE_INTEGER Operand1,
                                 LARGE_INTEGER Operand2)
{
  return Operand1.QuadPart <= Operand2.QuadPart;
#if 0
  return Operand1.HighPart < Operand2.HighPart ||
         (Operand1.HighPart == Operand2.HighPart && 
          Operand1.LowPart <= Operand2.LowPart);
#endif
}

BOOLEAN 
RtlLargeIntegerLessThanOrEqualToZero(LARGE_INTEGER Operand)
{
  return Operand.QuadPart <= 0;
#if 0
  return Operand.HighPart < 0 || 
         (Operand.HighPart == 0 && Operand.LowPart == 0);
#endif
}

BOOLEAN 
RtlLargeIntegerLessThanZero(LARGE_INTEGER Operand)
{
  return Operand.QuadPart < 0;
#if 0
  return Operand.HighPart < 0;
#endif
}

LARGE_INTEGER RtlLargeIntegerNegate(LARGE_INTEGER Subtrahend)
{
  LARGE_INTEGER RC;

  RC.QuadPart = - Subtrahend.QuadPart;

  return RC;
}

BOOLEAN 
RtlLargeIntegerNotEqualTo(LARGE_INTEGER Operand1,
                          LARGE_INTEGER Operand2)
{
  return Operand1.QuadPart != Operand2.QuadPart;
}

BOOLEAN 
RtlLargeIntegerNotEqualToZero(LARGE_INTEGER Operand)
{
  return Operand.QuadPart != 0;
}

LARGE_INTEGER RtlLargeIntegerShiftLeft(LARGE_INTEGER LargeInteger,
				       CCHAR ShiftCount)
{
  LARGE_INTEGER RC;

  RC.QuadPart = LargeInteger.QuadPart << ShiftCount;

  return RC;
}

LARGE_INTEGER RtlLargeIntegerShiftRight(LARGE_INTEGER LargeInteger,
					CCHAR ShiftCount)
{
  LARGE_INTEGER RC;

  RC.QuadPart = LargeInteger.QuadPart >> ShiftCount;

  return RC;
}

LARGE_INTEGER RtlLargeIntegerSubtract(LARGE_INTEGER Minuend,
				      LARGE_INTEGER Subtrahend)
{
  LARGE_INTEGER RC;

  RC.QuadPart = Minuend.QuadPart - Subtrahend.QuadPart;

  return RC;
}

