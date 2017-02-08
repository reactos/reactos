/* Math functions for i387.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by John C. Bowman <bowman@ipp-garching.mpg.de>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <win32k.h>

INT
APIENTRY
EngMulDiv(
    _In_ INT iMultiplicand,
    _In_ INT iMultiplier,
    _In_ INT iDivisor)
{
    INT64 i64Multiplied, i64Result;

    /* Check for divide by zero */
    if (iDivisor == 0)
    {
        /* Quick sign check and return "infinite" */
        return ((iMultiplicand ^ iMultiplier) < 0) ? INT_MIN : INT_MAX;
    }

    /* We want to deal with a positive divisor to simplify the logic. */
    if (iDivisor < 0)
    {
        iMultiplicand = -iMultiplicand;
        iDivisor = -iDivisor;
    }

    /* Do the multiplication */
    i64Multiplied = Int32x32To64(iMultiplicand, iMultiplier);

    /* If the result is positive, we add to round, else we subtract to round. */
    if (i64Multiplied >= 0)
    {
        i64Multiplied += (iDivisor / 2);
    }
    else
    {
        i64Multiplied -= (iDivisor / 2);
    }

    /* Now do the divide */
    i64Result = i64Multiplied / iDivisor;

    /* Check for positive overflow */
    if (i64Result > INT_MAX)
    {
        return INT_MAX;
    }

    /* Check for negative overflow. */
    if (i64Result < INT_MIN)
    {
        return INT_MIN;
    }

    return (INT)i64Result;
}

