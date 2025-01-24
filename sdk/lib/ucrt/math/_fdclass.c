//
// _fdclass.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of _fdclass.
//
// SPDX-License-Identifier: MIT
//

#include <math.h>
#include <stdint.h>

#ifdef _MSC_VER
#pragma function(_fdclass)
#endif

//
// Returns the floating-point classification of _X.
//
//     FP_NAN - A quiet, signaling, or indeterminate NaN
//     FP_INFINITE - A positive or negative infinity
//     FP_NORMAL - A positive or negative normalized non-zero value
//     FP_SUBNORMAL - A positive or negative subnormal (denormalized) value
//     FP_ZERO - A positive or negative zero value
//
_Check_return_
short
__cdecl
_fdclass(_In_ float _X)
{
    union { float f; uint32_t ui32; } u = { _X };
    uint32_t e = u.ui32 & 0x7F800000u;
    uint32_t m = u.ui32 & 0x007FFFFFu;

    if (e == 0x7F800000u)
    {
        return m ? FP_NAN : FP_INFINITE;
    }
    else if (e == 0)
    {
        return m ? FP_SUBNORMAL : FP_ZERO;
    }
    else
    {
        return FP_NORMAL;
    }
}
