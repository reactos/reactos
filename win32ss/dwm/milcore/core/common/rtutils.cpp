// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------
//

//
//  Description:
//      Contains utility functions used by several render target implementations
//

#include "precomp.hpp"

// C4356: 'TMilRect<TBase,TBaseRect,unique>::sc_rcEmpty' : static data member cannot be initialized via derived class
#pragma warning(disable:4356)

//+----------------------------------------------------------------------------
//
//  Class:     CMILSurfaceRect
//
//  Synopsis:  LTRB integer based rectangle with range limited to 28.4 based
//             rasterizer and stable integers stored as singles limits.
//
//-----------------------------------------------------------------------------

const CMILSurfaceRect::Rect_t CMILSurfaceRect::sc_rcEmpty(
    0, 0,
    0, 0,
    LTRB_Parameters
    );

const CMILSurfaceRect::Rect_t CMILSurfaceRect::sc_rcInfinite(
    SURFACE_RECT_MIN, SURFACE_RECT_MIN,
    SURFACE_RECT_MAX, SURFACE_RECT_MAX,
    LTRB_Parameters
    );



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
    )
{
    bool fIntersection;

    // Assert surface rect has expected limitations.
    Assert(!rcSurface.IsInfinite());

    //
    // Handle extremes that can't result in intersection and NAN by returning
    // no intersection
    //

    C_ASSERT(SURFACE_RECT_MIN >= FIXED4_INT_MIN);
    C_ASSERT(SURFACE_RECT_MAX <= FIXED4_INT_MAX);

    #define SURFACE_RECT_MIN_FLOAT              (SURFACE_RECT_MIN - 1 + REAL_FIX4_ROUNDUP_FRACTION)
    #define SURFACE_RECT_MAX_FLOAT_PLUS_EPSILON (SURFACE_RECT_MAX + REAL_FIX4_ROUNDUP_FRACTION)

    if (//  (rcBoundsF.left >= rcSurface.sc_rcInfinite.right + REAL_FIX4_ROUNDUP_FRACTION)
           !(rcBoundsF.left <  SURFACE_RECT_MAX_FLOAT_PLUS_EPSILON)
        //  (rcBoundsF.top  >= rcSurface.sc_rcInfinite.bottom + REAL_FIX4_ROUNDUP_FRACTION)
        || !(rcBoundsF.top  <  SURFACE_RECT_MAX_FLOAT_PLUS_EPSILON)
        //  (rcBoundsF.right  <  rcSurface.sc_rcInfinite.left - (1 - REAL_FIX4_ROUNDUP_FRACTION))
        || !(rcBoundsF.right  >= SURFACE_RECT_MIN_FLOAT)
        //  (rcBoundsF.bottom <  rcSurface.sc_rcInfinite.top - (1 - REAL_FIX4_ROUNDUP_FRACTION))
        || !(rcBoundsF.bottom >= SURFACE_RECT_MIN_FLOAT)
       )
    {
        fIntersection = false;
    }
    else
    {
        *prcOut = rcSurface;

        if (rcBoundsF.left >= SURFACE_RECT_MIN_FLOAT)
        {
            SetIfGreater<LONG>(prcOut->left, RasterizerConvertRealToInteger(rcBoundsF.left));
        }

        if (rcBoundsF.top >= SURFACE_RECT_MIN_FLOAT)
        {
            SetIfGreater<LONG>(prcOut->top , RasterizerConvertRealToInteger(rcBoundsF.top ));
        }

        if (rcBoundsF.right < SURFACE_RECT_MAX_FLOAT_PLUS_EPSILON)
        {
            SetIfLess<LONG>(prcOut->right , RasterizerConvertRealToInteger(rcBoundsF.right ));
        }

        if (rcBoundsF.bottom < SURFACE_RECT_MAX_FLOAT_PLUS_EPSILON)
        {
            SetIfLess<LONG>(prcOut->bottom, RasterizerConvertRealToInteger(rcBoundsF.bottom));
        }

        fIntersection = !prcOut->IsEmpty();
    }

    if (!fIntersection)
    {
        // set beautified empty rect
        prcOut->SetEmpty();
    }

    return fIntersection;
}


//+----------------------------------------------------------------------------
//
//  Function:  IntersectAntiAliasedBoundsRectFWithSurfaceRect
//
//  Synopsis:  Specialized rectangle intersection for anti-aliased float bounds
//             and a surface limited rectangle
//
//             The incoming bounding rectangle is given floating-point in
//             device space and is rounded according to "single precision"
//             anti-aliased coverage* when converted to integer space.
//
//             Where "single precision" anti-aliased coverage means the most
//             precise coverage based anti-aliasing results we could generate.
//             Currently we have 8x8 coverage which has precision to 1/8 of a
//             unit.
//

bool
IntersectAntiAliasedBoundsRectFWithSurfaceRect(
    __in_ecount(1) MilRectF const &rcBoundsF,
    __in_ecount(1) CMILSurfaceRect const &rcSurface,
    __out_ecount(1) CMILSurfaceRect *prcOut
    )
{
    bool fIntersection;

    // Assert surface rect has expected limitations.
    Assert(!rcSurface.IsInfinite());

    //
    // Handle extremes that can't result in intersection and NAN by returning
    // no intersection
    //

    C_ASSERT(SURFACE_RECT_MIN >= FIXED4_INT_MIN);
    C_ASSERT(SURFACE_RECT_MAX <= FIXED4_INT_MAX);

    Assert(SURFACE_RECT_MIN == rcSurface.sc_rcInfinite.left  );
    Assert(SURFACE_RECT_MIN == rcSurface.sc_rcInfinite.top   );
    Assert(SURFACE_RECT_MAX == rcSurface.sc_rcInfinite.right );
    Assert(SURFACE_RECT_MAX == rcSurface.sc_rcInfinite.bottom);

    if (//  (floor(rcBoundsF.left) >= rcSurface.sc_rcInfinite.right)
        //   ==>  (rcBoundsF.left  >= rcSurface.sc_rcInfinite.right)
           !(rcBoundsF.left <  SURFACE_RECT_MAX)
        //  (floor(rcBoundsF.top)  >= rcSurface.sc_rcInfinite.bottom)
        //   ==>  (rcBoundsF.top   >= rcSurface.sc_rcInfinite.bottom)
        || !(rcBoundsF.top  <  SURFACE_RECT_MAX)
        //  (ceiling(rcBoundsF.right)  <=  rcSurface.sc_rcInfinite.left)
        //   ==>    (rcBoundsF.right   <=  rcSurface.sc_rcInfinite.left)
        || !(rcBoundsF.right  > SURFACE_RECT_MIN)
        //  (ceiling(rcBoundsF.bottom) <=  rcSurface.sc_rcInfinite.top)
        //   ==>    (rcBoundsF.bottom  <=  rcSurface.sc_rcInfinite.top)
        || !(rcBoundsF.bottom >= SURFACE_RECT_MIN)
       )
    {
        fIntersection = false;
    }
    else
    {
        *prcOut = rcSurface;

        if (rcBoundsF.left >= SURFACE_RECT_MIN+1)
        {
            SetIfGreater<LONG>(prcOut->left, CFloatFPU::Floor(rcBoundsF.left));
        }

        if (rcBoundsF.top >= SURFACE_RECT_MIN+1)
        {
            SetIfGreater<LONG>(prcOut->top , CFloatFPU::Floor(rcBoundsF.top ));
        }

        if (rcBoundsF.right <= SURFACE_RECT_MAX-1)
        {
            SetIfLess<LONG>(prcOut->right , CFloatFPU::Ceiling(rcBoundsF.right ));
        }

        if (rcBoundsF.bottom <= SURFACE_RECT_MAX-1)
        {
            SetIfLess<LONG>(prcOut->bottom, CFloatFPU::Ceiling(rcBoundsF.bottom));
        }

        fIntersection = !prcOut->IsEmpty();
    }

    if (!fIntersection)
    {
        // set beautified empty rect
        prcOut->SetEmpty();
    }

    return fIntersection;
}


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
    )
{
    return ((antiAliasedMode != MilAntiAliasMode::None) ?
            IntersectAntiAliasedBoundsRectFWithSurfaceRect(rcBoundsF,rcSurface,prcOut) :
            IntersectAliasedBoundsRectFWithSurfaceRect(rcBoundsF,rcSurface,prcOut));
}

//+----------------------------------------------------------------------------
//
//  Function:  IntersectCAliasedClipWithSurfaceRect
//
//  Synopsis:  Intersects (optional) cliprect and surface bounds into an output rect
//
//             *prcDeviceClip is set to the precise, device
//             clipping bounds.  
//

bool
IntersectCAliasedClipWithSurfaceRect(
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip,
    __in_ecount(1) CMILSurfaceRect const &rcDeviceBounds,
    __out_ecount(1) CMILSurfaceRect *prcDeviceClip
    )
{
    // Expect the device limits to stay within CMILSurfaceRect value range
    Assert(!rcDeviceBounds.IsInfinite());

    //
    // Check for any clipping
    //

    if (pAliasedClip == NULL ||
        pAliasedClip->IsNullClip())
    {
        *prcDeviceClip = rcDeviceBounds;

        return !prcDeviceClip->IsEmpty();
    }
    else
    {
        CMilRectF rcClipF;
        pAliasedClip->GetAsCMilRectF(&rcClipF);

        return IntersectAliasedBoundsRectFWithSurfaceRect(
            rcClipF,
            rcDeviceBounds,
            prcDeviceClip
            );
    }
}


//+------------------------------------------------------------------------
// 
//  Function:
//      InflateRectFToPointAndSizeL
// 
//  Synopsis:
//      Find the tightest integer rectangle containing
//      given floating-point rectangle.
//
//-------------------------------------------------------------------------
HRESULT
InflateRectFToPointAndSizeL(
    __in_ecount(1) const CMilRectF & rcF,
    __out_ecount(1) MilPointAndSizeL & rcI
    )
{
    HRESULT hr = S_OK;

    static const int
        intBoundMax =  1073741823, // 0x3FFFFFFF
        intBoundMin = -1073741824; // 0xC0000000

    if (rcF.IsWellOrdered() &&
        rcF.left   >= intBoundMin && rcF.left <= intBoundMax &&
        rcF.top    >= intBoundMin && rcF.top  <= intBoundMax &&
        rcF.right  <= intBoundMax &&
        rcF.bottom <= intBoundMax
        )
    {
        rcI.X = CFloatFPU::Floor(rcF.left);
        rcI.Y = CFloatFPU::Floor(rcF.top);
        rcI.Width  = CFloatFPU::Ceiling(rcF.right) - rcI.X;
        rcI.Height = CFloatFPU::Ceiling(rcF.bottom) - rcI.Y;
    }
    else
    {
        MIL_THR(WGXERR_BADNUMBER);
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  
//      GetBitmapSourceBounds
//
//  Synopsis:  
//      Obtains the integer size of the bitmap & returns it's bounds as
//      floating-point rectangle.
//
//-------------------------------------------------------------------------
HRESULT 
GetBitmapSourceBounds(
    __in_ecount(1) IWGXBitmapSource *pBitmapSource,
        // IWGXBitmapSource to obtain bounds of
    __out_ecount(1) CMilRectF *pBitmapSourceBounds
        // Output bounds of the bitmap source
    )
{
    HRESULT hr = S_OK;

    UINT uContentWidth, uContentHeight;    

    IFC(pBitmapSource->GetSize(&uContentWidth, &uContentHeight));
    
    pBitmapSourceBounds->left = 0.0f;
    pBitmapSourceBounds->top = 0.0f;
    pBitmapSourceBounds->right = static_cast<float>(uContentWidth);
    pBitmapSourceBounds->bottom = static_cast<float>(uContentHeight);    

Cleanup:
    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
// 
//  Function:  ClipToSafeDeviceBounds
// 
//  Synopsis:
//      Clip a shape to the "safe bounds" that our rasterizer can
//      handle.  These bounds are determined for the moment by the
//      fixed point representation we use.  If the shape doesn't
//      extend outside the safe region then we don't clip and return
//      NULL as a result.  This should be the most common case.
// 
//-------------------------------------------------------------------------
HRESULT
ClipToSafeDeviceBounds(
    __in_ecount_opt(1) const IShapeData *pShape,            // NULL interpreted as infinite shape
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice, // (NULL OK)
    __in_ecount(1) const CRectF<CoordinateSpace::Shape> *prcShapeBounds,       // in shape space
    __in_ecount(1) CShape *pResult,
    __out_ecount(1) bool *pfNeededClip                      // Return whether clipping was necessary here.

    )
{
    static const CRectF<CoordinateSpace::Device> c_rcSafeDeviceBounds(
        -SAFE_RENDER_MAX,
        -SAFE_RENDER_MAX,
        +SAFE_RENDER_MAX,
        +SAFE_RENDER_MAX,
        LTRB_Parameters);
    
    HRESULT hr = S_OK;

    *pfNeededClip = false;

    if (pShape)
    {
        CRectF<CoordinateSpace::Device> rcDeviceBounds; // Shape bounds in device space.
   
        pmatShapeToDevice->Transform2DBoundsNullSafe(*prcShapeBounds, OUT rcDeviceBounds);

        if (!c_rcSafeDeviceBounds.DoesContain(rcDeviceBounds))
        {
            if (pmatShapeToDevice)
            {
                CMatrix<CoordinateSpace::Device,CoordinateSpace::Shape> matDeviceToShape;

                if (!matDeviceToShape.Invert(*pmatShapeToDevice))
                {
                    // If shape to device isn't invertible then the shape
                    // won't render anything anyway.
                    hr = S_OK;
                    goto Cleanup;
                }
            }

            // Transform to device space and clip...
            IFC(CShapeBase::ClipWithRect(
                    pShape,
                    &c_rcSafeDeviceBounds,
                    pResult,
                    pmatShapeToDevice
                ));

            *pfNeededClip = true;
        }
    }
    else
    {
        IFC(pResult->AddRect(c_rcSafeDeviceBounds));
        *pfNeededClip = true;
    }
    
Cleanup:

    RRETURN(hr);
}






