//
// _dclass.c
//
//      Copyright (c) 2024 Timo Kreuzer
//
// Implementation of _dclass.
//
// SPDX-License-Identifier: MIT
//

#include <math.h>
#include <stdint.h>

#ifdef _MSC_VER
#pragma function(_dclass)
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
_dclass(_In_ double _X)
{
    union { double f; uint64_t ui64; } u = { _X };
    uint64_t e = u.ui64 & 0x7FF0000000000000ull;
    uint64_t m = u.ui64 & 0x000FFFFFFFFFFFFFull;

    if (e == 0x7FF0000000000000ull)
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
