/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Float to integer conversion (__dtoi64/dtou64/stoi64/stou64)
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#ifdef _USE_64_BITS_
typedef double FLOAT_TYPE;
typedef unsigned long long FINT_TYPE;
#define FRACTION_LEN 52
#define EXPONENT_LEN 11
#else
typedef float FLOAT_TYPE;
typedef unsigned int FINT_TYPE;
#define FRACTION_LEN 23
#define EXPONENT_LEN 8
#endif

typedef
#ifndef _USE_SIGNED_
unsigned
#endif
long long FTO64_RESULT;

typedef union _FTO64_UNION
{
    FLOAT_TYPE value;
    FINT_TYPE raw;
} FTO64_UNION;

#define SIGN_MASK (((FINT_TYPE)1) << (FRACTION_LEN + EXPONENT_LEN))

#define FRACTION_ONE (((FINT_TYPE)1) << FRACTION_LEN)
#define FRACTION_MASK ((FRACTION_ONE) - 1)

#define EXPONENT_MASK ((1 << EXPONENT_LEN) - 1)
#define EXPONENT_ZERO ((1 << (EXPONENT_LEN - 1)) - 1)

#ifdef _USE_SIGNED_
#define EXPONENT_MAX 62
#define INTNAN       0x8000000000000000ULL
#else
#define EXPONENT_MAX 63
#define INTNAN       0xFFFFFFFFFFFFFFFFULL
#endif

#define EXPONENT_INFINITY EXPONENT_MASK

#define NEGATE(x) (~(x) + 1)

FTO64_RESULT
__fto64(FLOAT_TYPE fvalue)
{
    FTO64_UNION u = { .value = fvalue };
    FINT_TYPE value = u.raw;
    int exponent;
    FTO64_RESULT fraction;

    exponent = (int)(value >> FRACTION_LEN);
    exponent &= EXPONENT_MASK;

    /* infinity and other NaNs */
    if (exponent == EXPONENT_INFINITY)
        return INTNAN;

    /* subnormals and signed zeros */
    if (exponent == 0)
        return 0;

    exponent -= EXPONENT_ZERO;

    /* number is less then one */
    if (exponent < 0)
        return 0;

    /* number is too big */
    if (exponent > EXPONENT_MAX)
        return INTNAN;

#ifndef _USE_SIGNED_
    if (value & SIGN_MASK)
        return INTNAN;
#endif

    fraction = value & FRACTION_MASK;
    fraction |= FRACTION_ONE;

    exponent -= FRACTION_LEN;
    if (exponent != 0)
    {
        if (exponent < 0)
            fraction = fraction >> NEGATE(exponent);
        else
            fraction = fraction << exponent;
    }

#ifdef _USE_SIGNED_
    if (value & SIGN_MASK)
        fraction = NEGATE(fraction);
#endif

    return fraction;
}
