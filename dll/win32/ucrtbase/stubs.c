/*
 * PROJECT:     VerOS NT System Libraries
 * LICENSE:     GNU Lesser General Public License, version 2.1 or later (LGPL-2.1-or-later)
 * FILE:        dll/win32/ucrtbase/stubs.c
 * PURPOSE:     ucrtbase helpers and C99 math exports softpipe needs
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * lround/cbrtf: guest output/ntdll-apps/serial-App-20261116T224834Z.log
 * marker WARNING: calling stub lround(100.000000) and calling stub cbrtf.
 * In-tree shape: dll/win32/msvcrt/math.c lround via round.
 * Public: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/lround-lroundf-lroundl
 * Public: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/cbrt-cbrtf-cbrtl
 */

#include <stdint.h>
#include <intrin.h>
#include <malloc.h>
#define _USE_MATH_DEFINES
#include <math.h>

// atexit is needed by libsupc++
extern int __cdecl _crt_atexit(void (__cdecl*)(void));
int __cdecl atexit(void (__cdecl* function)(void))
{
    return _crt_atexit(function);
}

void* __cdecl operator_new(size_t size)
{
    return malloc(size);
}

void _cdecl operator_delete(void *mem)
{
    free(mem);
}

#ifdef _M_IX86
void _chkesp_failed(void)
{
    __debugbreak();
}
#endif

int __cdecl __acrt_initialize_sse2(void)
{
    return 0;
}

// The following stubs cannot be implemented as stubs by spec2def, because they are intrinsics

#ifdef _MSC_VER
#pragma warning(disable:4163) // not available as an intrinsic function
#pragma warning(disable:4164) // intrinsic function not declared
#pragma function(fma)
#pragma function(fmaf)
#pragma function(log2)
#pragma function(log2f)
#pragma function(lrint)
#pragma function(lrintf)
#pragma function(lround)
#pragma function(lroundf)
#pragma function(cbrt)
#pragma function(cbrtf)
#endif

double fma(double x, double y, double z)
{
    // Simplistic implementation
    return (x * y) + z;
}

float fmaf(float x, float y, float z)
{
    // Simplistic implementation
    return (x * y) + z;
}

double log2(double x)
{
    // Simplistic implementation: log2(x) = log(x) / log(2)
    return log(x) * M_LOG2E;
}

float log2f(float x)
{
    return (float)log2((double)x);
}

long int lrint(double x)
{
    __debugbreak();
    return 0;
}

long int lrintf(float x)
{
    __debugbreak();
    return 0;
}

/* Shape matches dll/win32/msvcrt/math.c lround: round then cast. */
long int lround(double x)
{
    double d = round(x);
    if (d != (double)(long int)d)
    {
        return 0;
    }
    return (long int)d;
}

long int lroundf(float x)
{
    float f = roundf(x);
    if (f != (float)(long int)f)
    {
        return 0;
    }
    return (long int)f;
}

/* C99 cube root via pow; preserve sign (MSDN cbrt page above). */
double cbrt(double x)
{
    if (x == 0.0)
    {
        return x;
    }
    return copysign(pow(fabs(x), 1.0 / 3.0), x);
}

float cbrtf(float x)
{
    if (x == 0.0f)
    {
        return x;
    }
    return copysignf(powf(fabsf(x), 1.0f / 3.0f), x);
}
