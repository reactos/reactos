// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Fixed Point types, defines, and methods
//
//------------------------------------------------------------------------------

#pragma once

#ifndef _FIX_HPP
#define _FIX_HPP

typedef INT FIX4;       // 28.4 fixed point value
typedef INT FIX16;      // 16.16 fixed point value

// constants for working with 28.4 fixed point values
#define FIX4_SHIFT  4
#define FIX4_PRECISION  4
#define FIX4_ONE        (1 << FIX4_PRECISION)
#define FIX4_HALF       (1 << (FIX4_PRECISION-1))
#define FIX4_MASK       (FIX4_ONE - 1)


//+----------------------------------------------------------------------------
//
//  Constant:  REAL_FIX4_ROUNDUP_FRACTION
//
//  Synopsis:  Floating point fraction that rounds up to the next integer
//             rather than down.
//
//-----------------------------------------------------------------------------

#define REAL_FIX4_ROUNDUP_FRACTION  ((FIX4_ONE+1.0f)/(2.0f*FIX4_ONE))

enum
{
    FIX16_SHIFT = 16,
    FIX16_ONE = 1 << FIX16_SHIFT,
    FIX16_HALF = 1 << (FIX16_SHIFT - 1),
    FIX16_MASK = FIX16_ONE - 1
};

inline INT
GpFix16Floor(
    FIX16        fixedValue
    )
{
    return fixedValue >> FIX16_SHIFT;
}

inline INT
GpFix16Ceiling(
    FIX16        fixedValue
    )
{
    return (fixedValue + FIX16_MASK) >> FIX16_SHIFT;
}

inline INT
GpFix16Round(FIX16 fixedValue)
{
    // Add half and truncate down towards negative infinity.
    return (fixedValue + FIX16_HALF) >> FIX16_SHIFT;
}

inline FIX16
GpRealToFix16(
    REAL        realValue
    )
{
    return GpRound(realValue * FIX16_ONE);
}

inline FIX16
GpIntToFix16(
    INT intValue
    )
{
    return intValue * FIX16_ONE;
}

/**************************************************************************\
*
* Function Description:
*
*   Take the ceiling of a fixed-pt value, without doing overflow checking
*
* Arguments:
*
*   [IN] fixedValue - 28.4 Fixed Point value
*
* Return Value:
*
*   INT - The integer ceiling (32.0) of the fixed point value
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/

inline INT
GpFix4Ceiling(
    FIX4        fixedValue
    )
{
    return (fixedValue + FIX4_MASK) >> FIX4_PRECISION;
}

inline INT
GpFix4Floor(
    FIX4        fixedValue
    )
{
    return fixedValue >> FIX4_PRECISION;
}

inline INT
GpFix4Round(
    FIX4        fixedValue
    )
{
    // Add half and truncate down towards negative infinity.

    return (fixedValue + FIX4_HALF) >> FIX4_PRECISION;
}


/**************************************************************************\
*
* Function Description:
*
*   Convert a real, floating point value to a 28.4 fixed-point value,
*   without doing overflow checking
*
* Arguments:
*
*   [IN] r         - Real class to make rounding faster
*   [IN] realValue - the real value to convert
*
* Return Value:
*
*   FIX4 - The 28.4 fixed point value
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
inline FIX4
GpRealToFix4(
    REAL        realValue
    )
{
    return GpRound(realValue * FIX4_ONE);
}

inline REAL
FIX4TOREAL(
    FIX4    fix
    )
{
    return (((REAL)fix)/16);
}

class PointFix4
{
public:
    FIX4    X;
    FIX4    Y;
    
    VOID Set(REAL x, REAL y)
    {
        X = GpRealToFix4(x);
        Y = GpRealToFix4(y);
    }
};

//--------------------------------------------------------------------------
// The following are simple functions to check if a number is within range
// of fixed point types.
//--------------------------------------------------------------------------

#define FIXED16_INT_MAX ( (1 << 15) - 1)
#define FIXED16_INT_MIN (-(1 << 15))

#define FIXED4_INT_MAX ( (1 << 27) - 1)
#define FIXED4_INT_MIN (-(1 << 27))

inline BOOL GpValidFixed16(REAL x)
{
    return (x >= FIXED16_INT_MIN) && (x <= FIXED16_INT_MAX);
}

inline BOOL GpValidFixed4(REAL x)
{
    return (x >= FIXED4_INT_MIN) && (x <= FIXED4_INT_MAX);
}

inline BOOL GpValidFixed16(INT x)
{
    return (x >= FIXED16_INT_MIN) && (x <= FIXED16_INT_MAX);
}

inline BOOL GpValidFixed4(INT x)
{
    return (x >= FIXED4_INT_MIN) && (x <= FIXED4_INT_MAX);
}


/**************************************************************************\
*
* Function Description:
*
*   This function multiplies two 32bit integers into a 64 bit value, and
*   shifts the result to the right by 16 bits.  We provide a hand
*   optimized assembly version on x86 to avoid out of line calls.
*
*   This has the effect of multiplying two 16.16 fixed point numbers and
*   returning a 16.16 fixed point result.
*
* Arguments:
*
*     a - first 32bit integer to multiply
*     b - second 32bit integer to multiply
*
*
\**************************************************************************/

#ifdef _X86_

__inline INT
Int32x32Mod16(
        INT a,
        INT b
        )
{
    __asm
    {
        mov eax, b
        imul a
        shrd eax, edx, 16
    }
}

#else

__inline INT
Int32x32Mod16(
        INT a,
        INT b
        )
{
    return ((INT) Int64ShraMod32(Int32x32To64(a, b), 16));
}

#endif


/**************************************************************************\
*
* Function Description:
*
*   This function takes two input numbers treated as 16.16. They are multiplied
*   together to give an internal 32.32 fixed point representation. The 
*   fractional bits are then rounded to the nearest whole number and the
*   result is returned as a BYTE. 
*
*   This is particularly useful for color channel computation requiring 16 
*   bits of fractional precision.
*
* Arguments:
*
*     a - first 32bit integer to multiply
*     b - second 32bit integer to multiply
*
*
\**************************************************************************/

#ifdef _X86_

__inline BYTE
Fix16MulRoundToByte(DWORD a, DWORD b)
{
    __asm
    {
        mov eax, b
        mul a
        add eax, 0x80000000   // FIX32 half
        adc edx, 0            // carry
        mov eax, edx          // ret dl
    }
}

#else

__inline BYTE
Fix16MulRoundToByte(DWORD a, DWORD b)
{
    return (BYTE)GpFix16Round(Int32x32Mod16(a, b));
}

#endif



#endif // _FIX_HPP



