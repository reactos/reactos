/*
 * COPYRIGHT:       BSD, see COPYING.ARM in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/math/arm/__rt_udiv.c
 * PURPOSE:         Implementation of __rt_udiv
 * PROGRAMMER:      Timo Kreuzer
 * REFERENCE:       http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_div10.s.htm
 *                  http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_udiv.c.htm
 */

typedef struct _ARM_UDIVRESULT
{
    unsigned int quotient; /* to be returned in R0 */
    unsigned int modulus;  /* to be returned in R1 */
} ARM_UDIVRESULT;

__forceinline
void
__brkdiv0(void)
{
    __emit(0xDEF9);
}

__forceinline
void
__rt_udiv_internal(
    ARM_UDIVRESULT *result,
    unsigned int divisor,
    unsigned int dividend)
{
    unsigned int shift;
    unsigned int mask;
    unsigned int quotient;

    if (divisor == 0)
    {
        /* Raise divide by zero error */
        __brkdiv0();
    }

    if (divisor > dividend)
    {
        result->quotient = 0;
        result->modulus = divisor;
        return;
    }

    /* Get the difference in count of leading zeros between dividend and divisor */
    shift = _CountLeadingZeros(divisor);
    shift -= _CountLeadingZeros(dividend);

    /* Shift the divisor to the left, so that it's highest bit is the same
       as the highest bit of the dividend */
    divisor <<= shift;

    mask = 1 << shift;

    quotient = 0;
    do
    {
        if (dividend >= divisor)
        {
            quotient |= mask;
            dividend -= divisor;
        }
        divisor >>= 1;
        mask >>= 1;
    }
    while (mask);

    result->quotient = quotient;
    result->modulus = dividend;
    return;
}

ARM_UDIVRESULT
__rt_udiv(
    unsigned int divisor,
    unsigned int dividend)
{
    ARM_UDIVRESULT result;

    __rt_udiv_internal(&result, divisor, dividend);
    return result;
}

typedef struct _ARM_SDIVRESULT
{
    int quotient; /* to be returned in R0 */
    int modulus;  /* to be returned in R1 */
} ARM_SDIVRESULT;

ARM_SDIVRESULT
__rt_sdiv(
    int divisor,
    int dividend)
{
    ARM_SDIVRESULT result;
    int divisor_sign, dividend_sign;

    dividend_sign = divisor & 0x80000000;
    if (dividend_sign)
    {
        dividend = -dividend;
    }

    divisor_sign = divisor & 0x80000000;
    if (divisor_sign)
    {
        divisor = -divisor;
    }

    __rt_udiv_internal((ARM_UDIVRESULT*)&result, divisor, dividend);

    if (dividend_sign ^ divisor_sign)
    {
        result.quotient = -result.quotient;
    }

    if (dividend_sign)
    {
        result.modulus = -result.modulus;
    }

    return result;
}
