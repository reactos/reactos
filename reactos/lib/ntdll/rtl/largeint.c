/* $Id: largeint.c,v 1.4 1999/09/29 23:09:44 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/largeint.c
 * PURPOSE:         Large integer operations
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   08/30/98  RJJ  Implemented several functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/linkage.h>

#define NDEBUG
#include <internal/debug.h>

typedef long long int LLI, *PLLI;
typedef unsigned long long int ULLI, *PULLI;

#define LIFromLLI(X) (*(PLARGE_INTEGER)&(X))
#define LLIFromLI(X) (*(PLLI)&(X))
#define ULIFromULLI(X) (*(PULARGE_INTEGER)&(X))

/* FUNCTIONS *****************************************************************/

LARGE_INTEGER
RtlLargeIntegerDivide(LARGE_INTEGER Dividend,
                      LARGE_INTEGER Divisor,
                      PLARGE_INTEGER Remainder)
{
}

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

BOOLEAN 
RtlLargeIntegerEqualTo(LARGE_INTEGER Operand1,
                       LARGE_INTEGER Operand2)
{
  return Operand1.QuadPart == Operand2.QuadPart;
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
}

BOOLEAN 
RtlLargeIntegerGreaterThanOrEqualTo(LARGE_INTEGER Operand1,
                                    LARGE_INTEGER Operand2)
{
  return Operand1.QuadPart >= Operand2.QuadPart;
}

BOOLEAN 
RtlLargeIntegerGreaterThanOrEqualToZero(LARGE_INTEGER Operand1)
{
  return Operand1.QuadPart > 0;
}

BOOLEAN 
RtlLargeIntegerGreaterThanZero(LARGE_INTEGER Operand1)
{
  return Operand1.QuadPart >= 0; 
}

BOOLEAN 
RtlLargeIntegerLessThan(LARGE_INTEGER Operand1,
                        LARGE_INTEGER Operand2)
{
  return Operand1.QuadPart < Operand2.QuadPart;
}

BOOLEAN 
RtlLargeIntegerLessThanOrEqualTo(LARGE_INTEGER Operand1,
                                 LARGE_INTEGER Operand2)
{
  return Operand1.QuadPart <= Operand2.QuadPart;
}

BOOLEAN 
RtlLargeIntegerLessThanOrEqualToZero(LARGE_INTEGER Operand)
{
  return Operand.QuadPart <= 0;
}

BOOLEAN 
RtlLargeIntegerLessThanZero(LARGE_INTEGER Operand)
{
  return Operand.QuadPart < 0;
}

LARGE_INTEGER
RtlLargeIntegerNegate(LARGE_INTEGER Subtrahend)
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

LARGE_INTEGER
RtlLargeIntegerShiftLeft(LARGE_INTEGER LargeInteger,
                         CCHAR ShiftCount)
{
  LARGE_INTEGER RC;

  RC.QuadPart = LargeInteger.QuadPart << ShiftCount;

  return RC;
}

LARGE_INTEGER
RtlLargeIntegerShiftRight(LARGE_INTEGER LargeInteger,
                          CCHAR ShiftCount)
{
  LARGE_INTEGER RC;

  RC.QuadPart = LargeInteger.QuadPart >> ShiftCount;

  return RC;
}

LARGE_INTEGER
RtlLargeIntegerSubtract(LARGE_INTEGER Minuend,
                        LARGE_INTEGER Subtrahend)
{
  LARGE_INTEGER RC;

  RC.QuadPart = Minuend.QuadPart - Subtrahend.QuadPart;

  return RC;
}

/* EOF */
