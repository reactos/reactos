// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains generic render utility routines.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//+------------------------------------------------------------------------
//
//  Function:  RoundToPow2
//
//  Synopsis:  Round num to the closest power of 2 that is equal or greater
//             than num.  Input must be greater than 0 and less than equal
//             to (1 <<31)
//
//-------------------------------------------------------------------------
UINT
RoundToPow2(__in_range(1,ROUNDTOPOW2_UPPER_BOUND) UINT num)
{

    AssertMsg(num != 0, ("Zero passed to RoundToPow2"));
    AssertMsg(num <= (1 << 31), ("Num passed to RoundToPow2 is too high"));
    C_ASSERT(ROUNDTOPOW2_UPPER_BOUND == (1<<31));

#if defined(_USE_X86_ASSEMBLY)

    _asm {
        bsr ecx, num    // Scan for high bit
        mov eax, 1      // Prepare eax to recieve 1 << power
        shl eax, cl     // Get less than or equal power of 2
        cmp eax, num    // Check if num is greater
        setne cl        // Set to 1 if greater, 0 if equal
        shl eax, cl     // x2 if needed
    }

#else

    // This function is currently used to round up surface
    // sizes.  They are normally not more than 2^10 so start
    // the high bit scan there if so.  Othwerwise start at
    // bit 31.
#define INITIAL_HIGH_BIT    10

    UINT    Pow2 = (1 << INITIAL_HIGH_BIT);

#if (INITIAL_HIGH_BIT != 31)
    {
        if (num > Pow2)
        {
            Pow2 = (1U << 31);
        }
    }
#endif

    // Scan for highest set bit
    while (!(num & Pow2))
    {
        Pow2 >>= 1;
    }

    // If num isn't a power of two round up
    if (num != Pow2)
    {
        Pow2 <<= 1;
    }

    return Pow2;

#endif
}

//+------------------------------------------------------------------------
//
//  Function:  Log2
//
//  Synopsis:  Return smallest N such that 2^(N+1) is greater than input.
//
//-------------------------------------------------------------------------
__range(0,32) UINT
Log2(UINT ui)
{
    UINT uiPower = 0;

    while (ui >>= 1)
    {
        ++uiPower;
    }

    return uiPower;
}

//+------------------------------------------------------------------------
//
//  Function:  Distance
//
//  Synopsis:  Returns the distance between 2 MilPoint2F points
//
//-------------------------------------------------------------------------
float Distance( MilPoint2F pt1, MilPoint2F pt2 )
{
    return sqrtf( (pt1.X - pt2.X)*(pt1.X - pt2.X) + (pt1.Y - pt2.Y)*(pt1.Y - pt2.Y) );
}

//+------------------------------------------------------------------------
//
//  Function:  DoesUseMipMapping
//
//  Synopsis:  Returns whether the filter mode uses mipmapping.
//
//-------------------------------------------------------------------------
bool DoesUseMipMapping(MilBitmapInterpolationMode::Enum interpolationMode)
{
    return (   interpolationMode == MilBitmapInterpolationMode::TriLinear
            || interpolationMode == MilBitmapInterpolationMode::Anisotropic
               );
}

UINT GetPaddedByteCount(UINT cbSize)
{
    // We want precisely one of _X86_, _AMD64_, _ARM_, or _ARM64_ to be defined. 
#if !((!defined(_X86_) && defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_)) || \
      (defined(_X86_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_)) || \
      (!defined(_X86_) && !defined(_AMD64_) && defined(_ARM_) && !defined(_ARM64_)) || \
      (!defined(_X86_) && !defined(_AMD64_) && !defined(_ARM_) && defined(_ARM64_)))
#error Exactly one of _X86_, _AMD64_, _ARM_, _ARM64_ should be defined
#endif 

#if defined(_X86_) 
    return cbSize;
#elif defined(_AMD64_)
    return cbSize;
#elif defined(_ARM_) 
    const UINT alignment = 4;
    UINT padding = alignment - (cbSize % alignment);
    return (padding < alignment) ? (cbSize + padding) : cbSize;
#elif defined(_ARM64_)
    const UINT alignment = 8;
    UINT padding = alignment - (cbSize % alignment);
    return (padding < alignment) ? (cbSize + padding) : cbSize;
#endif
}


