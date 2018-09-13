/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1996  Intel Corporation
Copyright (c) 1990  Microsoft Corporation

Module Name:

    largeic.c

Abstract:

    This module implements routines for performing extended integer
    division.

Author:

    William K. Cheung (wcheung) 12-Apr-1996

Environment:

    Any mode.

Revision History:

--*/

#include "ntrtlp.h"


LARGE_INTEGER
RtlExtendedLargeIntegerDivide (
    IN LARGE_INTEGER Dividend,
    IN ULONG Divisor,
    IN OUT PULONG Remainder OPTIONAL
    )

/*++

Routine Description:

    This function divides an unsigned large integer by an unsigned long
    and returns the resultant quotient and optionally the remainder.

Arguments:

    Dividend - Supplies the dividend value.

    Divisor - Supplies the divisor value.

    Remainder - Supplies an optional pointer to a variable
        that receives the remainder.

Return Value:

    The large integer result is stored at the address supplied by a0.

--*/

{
    LARGE_INTEGER Quotient;

    Quotient.QuadPart = Dividend.QuadPart / Divisor;
    if (Remainder) {
        *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
    }
    return Quotient;
}

LARGE_INTEGER
RtlLargeIntegerDivide (
    IN LARGE_INTEGER Dividend,
    IN LARGE_INTEGER Divisor,
    IN PLARGE_INTEGER Remainder OPTIONAL
    )

/*++

Routine Description:

    This function divides an unsigned large integer by an unsigned
    large and returns the resultant quotient and optionally the remainder.

Arguments:

    Dividend - Supplies the dividend value.

    Divisor - Supplies the divisor value.

    Remainder - Supplies an optional pointer to a variable
        that receives the remainder.

Return Value:

    The large integer result is stored at the address supplied by a0.

--*/
{
    LARGE_INTEGER Quotient;

    Quotient.QuadPart = (ULONGLONG)Dividend.QuadPart / (ULONGLONG)Divisor.QuadPart;
    if (Remainder) {
        Remainder->QuadPart = ((ULONGLONG)Dividend.QuadPart % (ULONGLONG)Divisor.QuadPart);
    }
    return Quotient;
}

LARGE_INTEGER
RtlExtendedIntegerMultiply (
    IN LARGE_INTEGER Multiplicand,
    IN LONG Multiplier
    )
/*++


Routine Description:

    This function multiplies a signed large integer by a signed integer and
    returns the signed large integer result.

    N.B. An overflow is possible, but no exception is generated.

Arguments:

    Multiplicand - Supplies the multiplicand value.

    Multiplier - Supplies the multiplier value.

Return Value:

    The large integer result is returned

--*/
{
    LARGE_INTEGER Result;

    Result.QuadPart = Multiplicand.QuadPart * (LONGLONG)Multiplier;
    return (Result);
}
