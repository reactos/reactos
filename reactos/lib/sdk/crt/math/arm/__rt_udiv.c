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

ARM_UDIVRESULT
__rt_udiv(
    unsigned int divisor,
    unsigned int dividend)
{
    ARM_UDIVRESULT result;
    unsigned int BitShift;
    unsigned int BitMask;
    unsigned int Quotient;

    if (divisor == 0)
    {
        /* Raise divide by zero error */
        __brkdiv0();
    }

    if (divisor > dividend)
    {
        result.quotient = 0;
        result.modulus = divisor;
        return result;
    }

    /* Get the difference in count of leading zeros between dividend and divisor */
    BitShift = _CountLeadingZeros(divisor);
    BitShift -= _CountLeadingZeros(dividend);

    /* Shift the divisor to the left, so that it's highest bit is the same
       as the highest bit of the dividend */
    divisor <<= BitShift;

    BitMask = 1 << BitShift;

    do
    {
        if (dividend >= divisor)
        {
            Quotient |= BitMask;
            dividend -= divisor;
        }
        divisor >>= 1;
        BitMask >>= 1;
    }
    while (BitMask);

    result.quotient = Quotient;
    result.modulus = dividend;
    return result;
}

