/*
 * COPYRIGHT:       BSD, see COPYING.ARM in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/math/arm/__rt_udiv.c
 * PURPOSE:         Implementation of __rt_udiv
 * PROGRAMMER:      Timo Kreuzer
 * REFERENCE:       http://research.microsoft.com/pubs/70645/tr-2008-141.pdf
 *                  http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_div10.s.htm
 *                  http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_udiv.c.htm
 */

#ifdef _USE_64_BITS_
typedef unsigned long long UINT3264;
typedef long long INT3264;
#define _CountLeadingZeros _CountLeadingZeros64
#else
typedef unsigned int UINT3264;
typedef int INT3264;
#endif

__forceinline
void
__brkdiv0(void)
{
    __emit(0xDEF9);
}

typedef struct _ARM_DIVRESULT
{
    UINT3264 quotient; /* to be returned in R0 */
    UINT3264 modulus;  /* to be returned in R1 */
} ARM_DIVRESULT;

#ifndef _USE_64_BITS_
__forceinline
#endif
void
__rt_div_worker(
    ARM_DIVRESULT *result,
    UINT3264 divisor,
    UINT3264 dividend)
{
    UINT3264 shift;
    UINT3264 mask;
    UINT3264 quotient;
#ifdef _SIGNED_DIV_
    int dividend_sign;
    int divisor_sign;
#endif // _SIGNED_DIV_

    if (divisor == 0)
    {
        /* Raise divide by zero error */
        __brkdiv0();
    }

#ifdef _SIGNED_DIV_
    if ((INT3264)dividend < 0)
    {
        dividend_sign = 1;
        dividend = -(INT3264)dividend;
    }

    if ((INT3264)divisor < 0)
    {
        divisor_sign = 1;
        divisor = -(INT3264)divisor;
    }
#endif // _SIGNED_DIV_

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

    mask = (UINT3264)1 << shift;

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

#ifdef _SIGNED_DIV_
    if (dividend_sign ^ divisor_sign)
    {
        quotient = -(INT3264)quotient;
    }

    if (dividend_sign)
    {
        dividend = -(INT3264)dividend;
    }
#endif // _SIGNED_DIV_

    result->quotient = quotient;
    result->modulus = dividend;
    return;
}
