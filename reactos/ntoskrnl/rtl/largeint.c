/* $Id: largeint.c,v 1.8 1999/11/09 18:01:43 ekohl Exp $
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

ULONG
RtlEnlargedUnsignedDivide(ULARGE_INTEGER Dividend,
                          ULONG Divisor,
                          PULONG Remainder)
{
  if (Remainder)
    *Remainder = Dividend.QuadPart % Divisor;

  return (ULONG)(Dividend.QuadPart / Divisor);
}

LARGE_INTEGER
RtlEnlargedUnsignedMultiply(ULONG Multiplicand,
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
RtlExtendedLargeIntegerDivide(LARGE_INTEGER Dividend,
                              ULONG Divisor,
                              PULONG Remainder)
{
  LARGE_INTEGER RC;

  if (Remainder)
    *Remainder = Dividend.QuadPart % Divisor;

  RC.QuadPart = Dividend.QuadPart / Divisor;

  return RC;
}

LARGE_INTEGER
RtlExtendedMagicDivide(LARGE_INTEGER Dividend,
                       LARGE_INTEGER MagicDivisor,
                       CCHAR ShiftCount)
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

VOID
RtlLargeIntegerAnd(PLARGE_INTEGER Result,
                   LARGE_INTEGER Source,
                   LARGE_INTEGER Mask)
{
  Result->QuadPart = Source.QuadPart & Mask.QuadPart;
}

LARGE_INTEGER
RtlLargeIntegerArithmeticShift(LARGE_INTEGER LargeInteger,
                               CCHAR ShiftCount)
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
RtlLargeIntegerDivide(LARGE_INTEGER Dividend,
                      LARGE_INTEGER Divisor,
                      PLARGE_INTEGER Remainder)
{
  LARGE_INTEGER RC;

  if (Remainder)
    Remainder->QuadPart = Dividend.QuadPart % Divisor.QuadPart;

  RC.QuadPart = Dividend.QuadPart / Divisor.QuadPart;

  return RC;
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
  return Operand1.QuadPart >= 0;
}

BOOLEAN 
RtlLargeIntegerGreaterThanZero(LARGE_INTEGER Operand1)
{
  return Operand1.QuadPart > 0; 
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
  CHAR Shift;

  Shift = ShiftCount % 64;

  RC.QuadPart = LargeInteger.QuadPart << Shift;

  return RC;
}

LARGE_INTEGER
RtlLargeIntegerShiftRight(LARGE_INTEGER LargeInteger,
                          CCHAR ShiftCount)
{
  LARGE_INTEGER RC;
  CHAR Shift;

  Shift = ShiftCount % 64;

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
