// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------
//

//
//  Description:
//      Declares utility functions used by several render target implementations
//
//----------------------------------------------------------------------------

#pragma once

// Rename IShapeData to CShapeBase & remove typedef
class CShapeBase;
typedef CShapeBase IShapeData;

//+----------------------------------------------------------------------------
//
//  Class:     CMILSurfaceRect
//
//  Synopsis:  LTRB integer based rectangle with range limited to 28.4 based
//             rasterizer and stable integers stored as singles limits.  See
//             definition of CMILSurfaceRect::sc_rcInfinite.
//
//-----------------------------------------------------------------------------

#if FIXED4_INT_MAX < MAX_INT_TO_FLOAT
    #define SURFACE_RECT_MAX FIXED4_INT_MAX
#else
    #define SURFACE_RECT_MAX MAX_INT_TO_FLOAT
#endif

#if FIXED4_INT_MIN > MIN_INT_TO_FLOAT
    #define SURFACE_RECT_MIN FIXED4_INT_MIN
#else
    #define SURFACE_RECT_MIN MIN_INT_TO_FLOAT
#endif

// Future Consideration:   Use TMilRect (no _) for CMILSurfaceRect
//  The one thing that needs converted is SwFallback.cpp

// template <class TBase, class TBaseRect, class TBaseRect_WH>
typedef
    TMilRect_<
        INT,
        MilRectL,
        MilPointAndSizeL,
        RectUniqueness::_CMILSurfaceRect_ // Uniqueness specified to prevent confusion with CMilRectL
        > CMILSurfaceRect;

class CShape;                   // Forward decl

//+----------------------------------------------------------------------------
//
//  Function:  IgnoreNoRenderHRESULTs
//
//  Synopsis:  Resets non-critical HRESULTS to S_OK
//
//  Notes:
//
//     Rendering nothing is acceptable for cases where we hit a non-invertible
//     transform or other numerical errors.
//

inline void IgnoreNoRenderHRESULTs(__inout_ecount(1) HRESULT *pHr)
{
    HRESULT hr = *pHr;

    if (hr == WGXERR_NONINVERTIBLEMATRIX || hr == WGXERR_BADNUMBER)
    {
        *pHr = S_OK;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:  CMILSurfaceRectAsRECT
//
//  Synopsis:  Casts a CMILSurfaceRect * to a RECT *
//
//  Notes:     If the implementation of CMILSurfaceRect ever changes this
//             function will mark code relying on the congruence between
//             this and RECT.
//
//-----------------------------------------------------------------------------
__out_ecount(1) inline RECT *
CMILSurfaceRectAsRECT( __in_ecount(1) CMILSurfaceRect *pRect )
{
    return reinterpret_cast<RECT*>(pRect);
}


//+----------------------------------------------------------------------------
//
//  Function:  RasterizerConvertRealToInteger
//
//  Synopsis:  Round a real exactly as the rasterizer does
//
//  Notes:     This rounding is the same independent of aliased or
//             per-primitive antialising mode as aliased rendering still uses
//             fixed 28.4.
//

inline INT
RasterizerConvertRealToInteger(
    REAL realValue
    )
{
    //
    // We use inclusive top-left, exclusive bottom-right rule for rounding one
    // half to an integer.  Thus, we subtract 1 before asking GpFix4Round to
    // round.
    //
    //   Verify rasterizer real to integer conversion
    // It is unclear what rules the rasterizer is really using in this case; so
    // we need to review it carefully.  In the meantime anyone using this
    // routine will get the proper result.
    //

    return GpFix4Round(GpRealToFix4(realValue) - 1);
}


//+----------------------------------------------------------------------------
//
//  Function:  IntersectAliasedBoundsRectFWithSurfaceRect
//
//  Synopsis:  Specialized rectangle intersection for aliased float bounds and
//             a surface limited rectangle
//
//             The incoming bounding rectangle is given floating-point in
//             device space and is rounded according to rasterizer 28.4 rules
//             when converted to integer space.
//

bool
IntersectAliasedBoundsRectFWithSurfaceRect(
    __in_ecount(1) MilRectF const &rcBoundsF,
    __in_ecount(1) CMILSurfaceRect const &rcSurface,
    __out_ecount(1) CMILSurfaceRect *prcOut
    );


//+----------------------------------------------------------------------------
//
//  Function:  IntersectBoundsRectFWithSurfaceRect
//
//  Synopsis:  Specialized rectangle intersection for float bounds and
//             a surface limited rectangle
//
//             The incoming bounding rectangle is given floating-point in
//             device space and is rounded according to rasterizer 28.4 rules
//             and antialiasing setting when converted to integer space.
//

bool
IntersectBoundsRectFWithSurfaceRect(
    MilAntiAliasMode::Enum antiAliasedMode,
    __in_ecount(1) CRectF<CoordinateSpace::Device> const &rcBoundsF,
    __in_ecount(1) CMILSurfaceRect const &rcSurface,
    __out_ecount(1) CMILSurfaceRect *prcOut
    );


//+----------------------------------------------------------------------------
//
//  Function:  IntersectCAliasedClipWithSurfaceRect
//
//  Synopsis:  Resolves CAliasedClip with surface clip
//
//             *prcAxisAlignedDeviceClip is set to the precise, device clipping
//             bounds.  For NoClip this is the exact device bounds. For
//             EmptyClip this is an empty rectangle.  For AxisAlignedClip this
//             is the intersection of the device bounds, but should not be an
//             empty rectangle.
//


bool
IntersectCAliasedClipWithSurfaceRect(
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip,
    __in_ecount(1) CMILSurfaceRect const &rcDeviceBounds,
    __out_ecount(1) CMILSurfaceRect *prcDeviceClip
    );

HRESULT
InflateRectFToPointAndSizeL(
    __in_ecount(1) const CMilRectF & rcF,
    __out_ecount(1) MilPointAndSizeL & rcI
    );

HRESULT
GetBitmapSourceBounds(
    __in_ecount(1) IWGXBitmapSource *pBitmapSource,
    __out_ecount(1) CMilRectF *pBitmapSourceBounds
    );

//+------------------------------------------------------------------------
//
//  Function:  ClipToSafeDeviceBounds
//
//  Synopsis:
//      Clip a shape to the "safe bounds" that our rasterizer can
//      handle.  These bounds are determined for the moment by the
//      fixed point representation we use.  If the shape doesn't
//      extend outside the safe region then this function does nothing
//      and returns false in fNeededClip.
//
//      Otherwise it returns true in fNeededClip and puts the clipped
//      shape into the CShape pResult which should be initially empty.
//
//  IMPORTANT:
//      The output shape in pResult already has pmatShapeToDevice
//      applied to it.
//
//      This routine interprets a NULL input shape as the set of all
//      points and always produces a rectangle equal to the safe bounds.
//
//-------------------------------------------------------------------------
HRESULT
ClipToSafeDeviceBounds(
    // Input shape.  NULL interpreted as infinite shape.
    __in_ecount_opt(1) const IShapeData *pShape,

    // Null OK shape to device transform
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice,

    // Bounds of shape in shape space.
    __in_ecount(1) const CRectF<CoordinateSpace::Shape> *prcShapeBounds,

    // Return result here if clipping is necessary.
    __in_ecount(1) CShape *pResult,

    // Return whether clipping was necessary here.
    __out_ecount(1) bool *fNeededClip
    );





