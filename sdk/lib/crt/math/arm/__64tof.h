/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Integer to float conversion (__i64tod/u64tod/i64tos/u64tos)
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#ifdef _USE_64_BITS_
typedef double FLOAT_TYPE;
typedef unsigned long long FINT_TYPE;
#define FRACTION_LEN 52
#define EXPONENT_LEN 11
#define SHIFT_OFFSET EXPONENT_LEN
#else
typedef float FLOAT_TYPE;
typedef unsigned int FINT_TYPE;
#define FRACTION_LEN 23
#define EXPONENT_LEN 8
#define SHIFT_OFFSET (EXPONENT_LEN + 32)
#endif

#ifdef _USE_SIGNED_
typedef long long INT64SU;
#else
typedef unsigned long long INT64SU;
#endif

typedef union _FLOAT_RESULT
{
    FLOAT_TYPE value;
    FINT_TYPE raw;
} FLOAT_RESULT;

#define SIGN_MASK 0x8000000000000000ULL

#define EXPONENT_ZERO ((1 << (EXPONENT_LEN - 1)) - 1)

#define NEGATE(x) (~(x) + 1)

FLOAT_TYPE
__64tof(INT64SU value)
{
    FLOAT_RESULT result;
    FINT_TYPE exponent = EXPONENT_ZERO + FRACTION_LEN;
    int count = 0;
    unsigned long long mask = SIGN_MASK;

    if (value == 0)
        return (FLOAT_TYPE)0;

#ifdef _USE_SIGNED_
    if (value & SIGN_MASK)
    {
        value = NEGATE(value);
        /* set Sign bit using exponent */
        exponent |= 1 << EXPONENT_LEN;
    }
#endif

    for (; count < 64; count++)
    {
        if (value & mask)
            break;
        mask = mask >> 1;
    }

    count -= SHIFT_OFFSET;
    /* exponent is FRACTION_LEN - count */
    exponent -= count;
    result.raw = exponent << FRACTION_LEN;

    mask--;
    value = value & mask;
    if (value == 0)
        return result.value;

    if (count == 0)
    {
        result.raw |= value;
    }
    else if (count < 0)
    {
        count = NEGATE(count) - 1;
        value = value >> count;
        mask = value & 1;
        result.raw |= value >> 1;

        /* round up if left most bit of lost data is 1 */
        if (mask)
            result.raw++;
    }
    else
    {
        result.raw |= value << count;
    }

    return result.value;
}
