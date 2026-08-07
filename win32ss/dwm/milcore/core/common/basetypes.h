// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************\
*

*
* Module Name:
*
*   BaseTypes.h
*
* Abstract:
*
*   Basic types used by GDI+ implementation
*
\**************************************************************************/

typedef FLOAT REAL;

#define REAL_EPSILON        1.192092896e-07F        /* FLT_EPSILON */

#if DBG
#ifdef FASTCALL
    #undef FASTCALL
#endif
#define FASTCALL
#else
#ifdef _X86_
#define FASTCALL _fastcall
#else
#define FASTCALL
#endif
#endif

//--------------------------------------------------------------------------
//
// MIL_FORCEINLINE : Use this ONLY after profiling the code and determining
//                   that function-call overhead is significant!
//
//--------------------------------------------------------------------------

#if DBG
#define MIL_FORCEINLINE inline
#else // !DBG
#define MIL_FORCEINLINE __forceinline
#endif // !DBG

//--------------------------------------------------------------------------
// Primitive data types
//
// NOTE:
//  Types already defined in standard header files:
//      INT8
//      UINT8
//      INT16
//      UINT16
//      INT32
//      UINT32
//      INT64
//      UINT64
//
//  Avoid using the following types:
//      LONG - use INT
//      ULONG - use UINT
//      DWORD - use UINT32
//--------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:
//
//      IsSizeDotEmpty<TSize>
//
//  Synopsis:
//
//      Returns whether or not the passed-in rectangle is the sentinel 
//      Size.Empty value. 
//
//      Size.Empty is defined as (-INF, -INF), and is the
//      only case where the Width or Height is allowed to be < 0.0.
//      
//      In the invalid case where Width or Height are not >= 0 (including NaN)
//      this method also returns 'TRUE'. This is important because this method 
//      is often used to determine whether or not the size is functionality 
//      usable, which it is not in the case if the Width or Height is invalid.
//
//  Returns:
//      
//      TRUE if Width >= 0.0 && Height >= 0.0, FALSE otherwise
//
//----------------------------------------------------------------------------
template <class TSize>
inline BOOL
IsSizeDotEmpty(
    __in_ecount(1) const TSize *pSize // Size to evaluate
    )
{
    Assert(pSize);

    // This check is designed to handle NaN's.
    //
    // If Width or Height is invalid (including NaN's) the following check will fail, 
    // causing this method to return 'TRUE'.  This allows us to treat invalid sizes as 
    // 'Empty'.
    if (pSize->Width >= 0.0 && pSize->Height >= 0.0) 
    {
        return FALSE;
    }
    else
    {           
        return TRUE;        
    }
}

//+----------------------------------------------------------------------------
//
//  Function:
//
//      IsRectEmptyOrInvalid<TRect>
//
//  Synopsis:
//
//      Returns whether or not the passed-in rectangle is the sentinel
//      Rect.Empty value or some other rectangle that is invalid.
//
//      Rect.Empty is defined as (+INF, +INF, -INF, -INF), and is the only case
//      where the Width or Height is allowed to be < 0.0.
//
//      In the invalid case where Width or Height are not >= 0 (including NaN)
//      this method also returns 'TRUE'. NaN X and Y values also cause the
//      rectangle to be invalid.
//
//  Notes:
//
//      Rectangles with +/-INF X and Y are still valid by this method.
//      Rectangles with +INF Width and Height are still valid.
//      Rectangles with -INF Width and Height are not valid.
//
//-----------------------------------------------------------------------------
template <class TRect>
inline BOOL
IsRectEmptyOrInvalid(
    __in_ecount(1) const TRect *pRect  // Rectangle to evaluate
    )
{
    Assert(pRect);

    // This check is designed to handle NaN's.
    //
    // If Width or Height is invalid (including NaN's) the following check will fail, 
    // causing this method to return 'TRUE'.  This allows us to treat invalid rects as 
    // 'Empty'.
    if (   pRect->Width >= 0.0
        && pRect->Height >= 0.0
        && pRect->X == pRect->X
        && pRect->Y == pRect->Y
       ) 
    {
        return FALSE;
    }
    else
    {           
        return TRUE;        
    }
}

enum PathPointType
{
    PathPointTypeStart           = 0,    // move
    PathPointTypeLine            = 1,    // line
    PathPointTypeBezier          = 3,    // default Bezier (= cubic Bezier)
    PathPointTypePathTypeMask    = 0x07, // type mask (lowest 3 bits).
    PathPointTypeCloseSubpath    = 0x80, // closed flag
};

struct MILGradientStop
{
    FLOAT     rPosition; // Position on [0.0, 1.0] gradient line
    MilColorF color;     // scRGB Color of the gradient stop
};

// MILGradientTextureFormat is required seperate from MilPixelFormat::Enum because
// it includes an internal-only pixel format that shouldn't be exposed.
//
// If the imaging team creates an internal-only pixel format enum in the future,
// we may consider using it instead of MILGradientTextureFormat
enum MILGradientTextureFormat
{
    MILGradientTextureFormat32bppPARGB_sRGB,    // Standard gradient texture format
    MILGradientTextureFormat64bppP0A0G0R0B_sRGB // Special format used by SW implementation
};





