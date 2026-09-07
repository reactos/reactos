// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Contents:  Special arithmetic operations
//
//-----------------------------------------------------------------------------

#pragma once

//
//
// Constant definitions
//
//

// Define maximum single-precision value that can be represented precisely
//
// The mantissa of single-precision numbers is 24 bits (including the 
// extra implicit bit). 
const FLOAT FLT_MAX_PRECISION = 0xFFFFFF;
const FLOAT INVERSE_FLT_MAX_PRECISION = 1 / FLT_MAX_PRECISION;

// Define maximum double-precision value that can be represented precisely
//        
// The mantissa of double-precision numbers is 53 bits (including the 
// extra implicit bit). 
const DOUBLE DBL_MAX_PRECISION = 0x1FFFFFFFFFFFFF;
const DOUBLE INVERSE_DBL_MAX_PRECISION = 1 / DBL_MAX_PRECISION;


//
//
// Inline methods
//
//

// Overflow-protected addition and multiplication.
// If they turn out to be too slow these functions can be improved on x86 machines through 
// assembly implementation (to take advantage of the overflow flag).

MIL_FORCEINLINE HRESULT AddUINT(UINT a, UINT b, __out_ecount(1) __deref_out_range(==,a+b) UINT &sum)
{
    UINT c = a + b;
    if (c < a)
    {
        // c is also less than b.
        __pfx_assert(c < b, "UINT overflow test does not hold");
        return WINCODEC_ERR_VALUEOVERFLOW;
    }
    else
    {
        // c is also >= b.
        __pfx_assert(c >= b, "UINT overflow test does not hold");
        sum = c;
        return S_OK;
    }
}

MIL_FORCEINLINE HRESULT MultiplyUINT(UINT a, UINT b, __out_ecount(1) __deref_out_range(==,a*b) UINT &product)
{
    ULONGLONG c = UInt32x32To64(a, b);
    if (c > UINT_MAX)
    {
        return WINCODEC_ERR_VALUEOVERFLOW;
    }
    else
    {
        product = UINT(c);
        return S_OK;
    }
}

MIL_FORCEINLINE HRESULT IncrementUINT(
    __inout_ecount(1)
    __deref_in_range(<, UINT_MAX) __deref_out_range(==,pre(u)+1) UINT &u
    )
{
    if (u == UINT_MAX)
    {
        return WINCODEC_ERR_VALUEOVERFLOW;
    }
    else
    {
        u++;
        return S_OK;
    }
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        RoundUpToAlignDWORD
// 
//    Synopsis:
//        Rounds up the value to the nearest value divisible by sizeof(DWORD).
// 
//------------------------------------------------------------------------------

#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~(((LONG)(Pow2))-1)) )

inline HRESULT 
RoundUpToAlignDWORD(
    __inout_ecount(1) UINT *value
    )
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;

    UINT aligned = ROUND_UP_COUNT(*value, sizeof(DWORD));

    if (aligned >= *value)
    {
        *value = aligned;
        hr = S_OK;
    }

    INLINED_RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Function:   IsCloseToDivideByZero*
//
//  Synopsis:   Determines whether or not the calculation, Numerator / Denominator,
//              is close to dividing by zero.  
//
//              Mathematically, dividing by zero is undefined.  But in practice 
//              we also want to avoid dividing by numbers close to zero.  This 
//              is because the result will be very large and imprecise  
//              (single-precision numbers lose accuracy after 2^24, and 
//              2^53 is the boundary for double-precision numbers).  
//
//              The result a divide will hit this boundary when the 
//              denominator(D) is much smaller than the numerator(N).  Once
//              the denominator is so small, relative to the numerator, that
//              the accuracy boundary is hit, the computation
//              N + D will result in N (N + D = N).  That is, D is
//              computationally 0 with respect to N.  When this occurs,
//              this method will return TRUE because D is small enough
//              to be considered 0.
//
//  Returns:    TRUE if the denominator is close enough to zero, with respect to N,
//              to be considered zero for practical applications.
//
//  Notes:      We want to determine whether or not this division will result in a 
//              number that is too large to be accurately represented.  Mathematically, 
//              we can ask this question using the following inequality:
//
//                  |(N / D)| >= MAX_VALUE
//
//              To avoid the division and potential overflow, we rearrange the 
//              inequality to the implementation below:
//
//                  |D| <= |N| * (1 / MAX_VALUE)
//
//              When updating this, also update corresponding code in FloatUtils.cs
//
//------------------------------------------------------------------------
MIL_FORCEINLINE BOOL 
IsCloseToDivideByZeroReal(
    REAL rNumerator,        // Numerator of the division
    REAL rDenominator       // Denominator the division
    )
{       
    return fabsf(rDenominator) <= (fabsf(rNumerator) * INVERSE_FLT_MAX_PRECISION);
}

MIL_FORCEINLINE BOOL 
IsCloseToDivideByZeroDouble(
    DOUBLE dNumerator,      // Numerator of the division
    DOUBLE dDenominator     // Denominator the division
    )
{
    return fabs(dDenominator) <= (fabs(dNumerator) * INVERSE_DBL_MAX_PRECISION);
}


