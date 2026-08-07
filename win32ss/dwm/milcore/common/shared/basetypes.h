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

#ifdef FASTCALL
    #undef FASTCALL
#endif

#if DBG
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

// Returns TRUE if the intersection is not empty. The intersection of
// prcSrc1 and prcSrc2 is stored in prcDst.

template<typename T>
BOOL IntersectRectT(
    __out_ecount(1) T *prcDst,
    __in_ecount(1) const T *prcSrc1,
    __in_ecount(1) const T *prcSrc2
    );

MIL_FORCEINLINE
BOOL IntersectRect(
    __out_ecount(1) MilPointAndSizeL *prcDst,
    __in_ecount(1) const MilPointAndSizeL *prcSrc1,
    __in_ecount(1) const MilPointAndSizeL *prcSrc2
    )
{
    return IntersectRectT(prcDst,prcSrc1,prcSrc2);
}

MIL_FORCEINLINE
BOOL IntersectRect(
    __out_ecount(1) WICRect *prcDst,
    __in_ecount(1) const WICRect *prcSrc1,
    __in_ecount(1) const WICRect *prcSrc2
    )
{
    return IntersectRectT(prcDst,prcSrc1,prcSrc2);
}

//--------------------------------------------------------------------------
// Represents a 32-bit ARGB color in AARRGGBB format
//--------------------------------------------------------------------------

typedef MilColorB ARGB;
typedef UINT64 ARGB64;
typedef MilColorF ABGR128;

// Union for converting between ARGB and 4 separate BYTE channel values.

union GpCC
{
    struct {
        BYTE b;
        BYTE g;
        BYTE r;
        BYTE a;
    };
    ARGB argb;
};

union GpCC64
{
    struct {
        UINT16 b;
        UINT16 g;
        UINT16 r;
        UINT16 a;
    };
    ARGB64 argb;
};

// The AGRB64TEXEL structure is sort of funky in order to optimize the
// inner loop of our C-code linear gradient routine.
struct AGRB64TEXEL      // Note that it's 'AGRB', not 'ARGB'
{
    UINT32 A00rr00bb;   // Texel's R and B components
    UINT32 A00aa00gg;   // Texel's A and G components
};

enum DllLoadState
{
    Uninitialized = 0,
    Loaded,
    LoadFailed
};



