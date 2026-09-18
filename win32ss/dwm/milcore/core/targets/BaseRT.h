// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//      Declare base render target class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#pragma once

class CBaseRenderTarget :
    public IRenderTargetInternal
{
protected:

    CBaseRenderTarget();
    virtual ~CBaseRenderTarget();

    HRESULT Init();

public:

    // IMILRenderTarget.

    STDMETHODIMP_(VOID) GetBounds(
        __out_ecount(1) MilRectF * const pBounds
        );

    //
    // IRenderTargetInternal methods
    //

    STDMETHODIMP_(__outro_ecount(1) const CMILMatrix *) GetDeviceTransform(
        ) const;

protected:

    HRESULT InvalidateRect(
        __in_ecount(1) CMILSurfaceRect const *pRect
        );

    HRESULT ClearInvalidatedRects();

    STDMETHOD(HrFindInterface)(
        __in_ecount(1) REFIID riid,
        __deref_out void **ppv
        );

    HRESULT ShouldPresent(
        __in_ecount(1) RECT const *pInputRect,
        __out_ecount(1) CMILSurfaceRect *pResultRect,
        __inout_bcount((CDirtyRegion2::MaxDirtyRegionCount) * sizeof(RECT) + sizeof(RGNDATA)) RGNDATA **ppDirtyRegion,
        __out bool *fPresent
        );
      
#if DBG
        void DbgAssertNothingInvalid() const;
#endif

protected:

    bool UpdateCurrentClip(
        __in_ecount(1) const CAliasedClip &aliasedClip
        );

    bool AlphaScalePreservesOpacity(
        FLOAT flAlphaScale
        ) const
    {
        Assert(flAlphaScale >= 0.0f);
        Assert(flAlphaScale <= 1.0f);

        return (1.0f - flAlphaScale < m_flAlphaMin);
    }

    bool AlphaScaleEliminatesRenderOutput(
        FLOAT flAlphaScale
        ) const
    {
        Assert(flAlphaScale >= 0.0f);
        Assert(flAlphaScale <= 1.0f);

        return (flAlphaScale < m_flAlphaMin);
    }

protected:

    //
    // Basic RenderTarget properties
    //

    UINT m_uWidth;
    UINT m_uHeight;
    MilPixelFormat::Enum m_fmtTarget;
    CMILMatrix m_DeviceTransform;


    //
    // RenderTarget State
    //

    CMILSurfaceRect m_rcBounds;

    CMILSurfaceRect m_rcCurrentClip;


    //
    // RenderTarget rendering properties
    //

private:
    //__out_bcount((return->rdh.nCount) * sizeof(RECT) + sizeof(RGNDATA)) RGNDATA *GetInvalidatedRegion();
    HRESULT GetInvalidatedRegion(HRGN rgn, __deref_out RGNDATA **pRegion);


    FLOAT m_flAlphaMin;  // Smallest value such that m_flAlphaEpsilon times max
                         // normalized color value > 0
                         //
                         // Which may also be worded as:
                         //
                         //  1. smallest value that is not guarenteed to...
                         //
                         //  2. value just greater than largest value that will
                         //     always...
                         //
                         // ...produce a zero result when multiplied by any
                         // color in this surface.

    //
    // Initial size is large enough for a RGNDATA* containing 8 rectangles, which
    // is the largest number we currently see (0x20 size header, 0x80 for rectangles)
    //
    static const UINT s_cbInvalidRegionInitialSize = 160;

    HRGN m_hrgnNewRectRegion;
    HRGN m_hrgnInvalidRegion;

    RGNDATA *m_pInvalidRegionRgn;
    UINT m_cInvalidRegionSize;

    bool m_fHasInvalidRegions;
    bool m_fHaveEmptyRegionsBeenInvalidated;
    bool m_fHasComplexInvalidRegions;
};



