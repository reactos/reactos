/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Helpers for testing math functions
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <apitest.h>

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

#define ok_eq_dbl_exact_(file, line, func, ullx, z, ullexp) \
    { \
        double x = u64_to_dbl(ullx); \
        unsigned long long ullz = dbl_to_u64(z); \
        double exp = u64_to_dbl(ullexp); \
        ok_(file, line)(ullz == ullexp, "Wrong value for '%s(%f)' [0x%016llx], expected: %f [0x%016llx], got: %f [0x%016llx]\n", \
           func, x, ullx, exp, ullexp, z, ullz); \
    }
#define ok_eq_dbl_exact(func, ullx, z, ullexp) ok_eq_dbl_exact_(__FILE__, __LINE__, func, ullx, z, ullexp)

#define ok_eq_flt_exact_(file, line, func, ux, z, uexp) \
    { \
        float x = u32_to_flt(ux); \
        unsigned int uz = flt_to_u32(z); \
        float exp = u32_to_flt(uexp); \
        ok_(file, line)(uz == uexp, "Wrong value for '%s(%f)' [0x%08x], expected: %f [0x%08x], got: %f [0x%08x]\n", \
           func, x, ux, exp, uexp, z, uz); \
    }
#define ok_eq_flt_exact(func, ux, z, uexp) ok_eq_flt_exact_(__FILE__, __LINE__, func, ux, z, uexp)
