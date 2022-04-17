/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __rt_div_worker
 * COPYRIGHT:   Copyright 2015 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

/*
 * See also:
 * http://research.microsoft.com/pubs/70645/tr-2008-141.pdf
 * https://web.archive.org/web/20100110044008/http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/arm/_div10.s.htm
 * https://github.com/bawoodruff/BeagleBoard/blob/2731e3174af6daefe4a287a6be82e5ff9c46c99a/OS/BootLoader/Runtime/_udiv.c
 * https://github.com/bawoodruff/BeagleBoard/blob/2731e3174af6daefe4a287a6be82e5ff9c46c99a/OS/BootLoader/Runtime/arm/_udivsi3.s
 * https://github.com/bawoodruff/BeagleBoard/blob/2731e3174af6daefe4a287a6be82e5ff9c46c99a/OS/BootLoader/Runtime/arm/divide.s
 * https://github.com/qemu/edk2/blob/e3c7db50cac9125607df49d5873991df6df11eae/ArmPkg/Library/CompilerIntrinsicsLib/Arm/uldiv.asm
 * https://github.com/jmonesti/qemu-4.1.1/blob/55fb6a81039a62174c2763759324c43a67d752a1/roms/ipxe/src/arch/arm32/libgcc/lldivmod.S
 */

#ifdef _USE_64_BITS_
typedef unsigned long long UINT3264;
typedef long long INT3264;
typedef struct
{
    unsigned long long quotient; /* to be returned in R0,R1 */
    unsigned long long modulus;  /* to be returned in R2,R3 */
} RETURN_TYPE;
#define _CountLeadingZeros _CountLeadingZeros64
#else
typedef unsigned int UINT3264;
typedef int INT3264;
typedef unsigned long long RETURN_TYPE; /* to be returned in R0,R1 */
#endif

__forceinline
void
__brkdiv0(void)
{
    __emit(0xDEF9);
}

typedef union _ARM_DIVRESULT
{
    RETURN_TYPE raw_data;
    struct
    {
        UINT3264 quotient;
        UINT3264 modulus;
    } data;
} ARM_DIVRESULT;

RETURN_TYPE
__rt_div_worker(
    UINT3264 divisor,
    UINT3264 dividend)
{
    ARM_DIVRESULT result;
    UINT3264 shift;
    UINT3264 mask;
    UINT3264 quotient;
#ifdef _SIGNED_DIV_
    int dividend_sign = 0;
    int divisor_sign = 0;
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
        result.data.quotient = 0;
#ifdef _SIGNED_DIV_
        if (dividend_sign)
            dividend = -(INT3264)dividend;
#endif // _SIGNED_DIV_
        result.data.modulus = dividend;
        return result.raw_data;
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

    result.data.quotient = quotient;
    result.data.modulus = dividend;
    return result.raw_data;
}
