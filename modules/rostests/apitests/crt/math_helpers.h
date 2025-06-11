/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Helpers for testing math functions
 * COPYRIGHT:   Copyright 2021-2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <apitest.h>

#if defined(__GNUC__) || defined(__clang__)
#define __ATTRIBUTE_SSE2__ __attribute__((__target__("sse2")))
#else
#define __ATTRIBUTE_SSE2__
#endif

static
__inline
double
u64_to_dbl(UINT64 x)
{
    return *(double*)(&x);
}

static
__inline
UINT64
dbl_to_u64(double x)
{
    return *(UINT64*)(&x);
}

static
__inline
float
u32_to_flt(UINT32 x)
{
    return *(float*)(&x);
}

static
__inline
UINT32
flt_to_u32(float x)
{
    return *(UINT32*)(&x);
}

typedef struct _TESTENTRY_DBL
{
    unsigned long long x;
    unsigned long long result;
} TESTENTRY_DBL;

typedef struct _TESTENTRY_FLT
{
    unsigned long x;
    unsigned long result;
} TESTENTRY_FLT;

typedef struct _TESTENTRY_DBL_INT
{
    unsigned long long x;
    int result;
} TESTENTRY_DBL_INT;

typedef struct _TESTENTRY_FLT_INT
{
    unsigned long x;
    int result;
} TESTENTRY_FLT_INT;

typedef struct _PRECISE_VALUE
{
    double rounded;
    double delta;
} PRECISE_VALUE;

typedef struct _TESTENTRY_DBL_APPROX
{
    double x;
    PRECISE_VALUE expected;
    uint32_t max_error;
} TESTENTRY_DBL_APPROX;

// Convert a double to a 64-bit integer with lexicographical ordering.
static inline int64_t double_to_int64_lex(double d)
{
    union {
        double d;
        int64_t i;
    } u;
    u.d = d;
    // For negative numbers, flip the ordering.
    if (u.i < 0)
        u.i = 0x8000000000000000LL - u.i;
    return u.i;
}

// Convert a float to a 64-bit integer with lexicographical ordering.
static inline int float_to_int_lex(float f)
{
    union {
        float f;
        int i;
    } u;
    u.f = f;
    // For negative numbers, flip the ordering.
    if (u.i < 0)
        u.i = 0x80000000 - u.i;
    return u.i;
}

// Returns the ULP difference between 'expected' and 'result'.
// If result is greater than expected, the result is positive.
// (If either value is NaN the function returns INT64_MAX.)
static __inline int64_t ulp_error_dbl(double expected, double result)
{
    // Exact equality: error is 0 ULPs.
    if (expected == result)
        return 0;

    // If either value is not a number, return a sentinel.
    if (_isnan(expected) || _isnan(result))
        return INT64_MAX;

    int64_t i_expected = double_to_int64_lex(expected);
    int64_t i_result   = double_to_int64_lex(result);

    return i_result - i_expected;
}

static __inline int ulp_error_flt(float expected, float result)
{
    // Exact equality: error is 0 ULPs.
    if (expected == result)
        return 0;

    // If either value is not a number, return a sentinel.
    if (_isnan(expected) || _isnan(result))
        return INT_MAX;

    int i_expected = float_to_int_lex(expected);
    int i_result   = float_to_int_lex(result);

    return i_result - i_expected;
}

static __inline int64_t ulp_error_precise(PRECISE_VALUE *expected, double result)
{
    // Calculate error to expected rouned value as ULP
    int64_t error = ulp_error_dbl(expected->rounded, result);
    if ((error == INT64_MAX) || (error == 0))
        return error;

    // Check if the error has the same sign as the delta
    if ((expected->delta >= 0.0) == (error >= 0))
    {
        // We have an error in the same direction as the delta.
        // This means the error is correct as it is.
        return error;
    }
    else
    {
        // We have an error in the opposite direction of the delta.
        // This means the actual error is larger than the calculated error.
        return (error < 0) ? error - 1 : error + 1;
    }
}

#define ok_eq_dbl_exact_(file, line, func, ullx, z, ullexp) \
    { \
        double x = u64_to_dbl(ullx); \
        unsigned long long ullz = dbl_to_u64(z); \
        double exp = u64_to_dbl(ullexp); \
        ok_(file, line)(ullz == ullexp, "Wrong value for '%s(%f)' [0x%016I64x], expected: %f [0x%016I64x], got: %f [0x%016I64x]\n", \
           func, x, ullx, exp, ullexp, z, ullz); \
    }
#define ok_eq_dbl_exact(func, ullx, z, ullexp) ok_eq_dbl_exact_(__FILE__, __LINE__, func, ullx, z, ullexp)

#define ok_eq_flt_exact_(file, line, func, ux, z, uexp) \
    { \
        float x = u32_to_flt(ux); \
        unsigned int uz = flt_to_u32(z); \
        float exp = u32_to_flt(uexp); \
        ok_(file, line)(uz == uexp, "Wrong value for '%s(%f)' [0x%08x], expected: %f [0x%08x], got: %f [0x%08x]\n", \
           func, x, (unsigned)ux, exp, (unsigned)uexp, z, (unsigned)uz); \
    }
#define ok_eq_flt_exact(func, ux, z, uexp) ok_eq_flt_exact_(__FILE__, __LINE__, func, ux, z, uexp)
