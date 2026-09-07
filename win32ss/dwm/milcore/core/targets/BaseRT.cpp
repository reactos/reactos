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
//      Define base render target class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(DirtyRegionData, MILRender, "CBaseRenderTarget::m_pDirtyRegion");

DeclareTag(tagMILDisablePresent, "CBaseRenderTarget", "Disable present");

//==============================================================================

STDMETHODIMP
CBaseRenderTarget::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (riid == IID_IRenderTargetInternal || riid == IID_IMILRenderTarget)
    {
        *ppvObject = static_cast<IRenderTargetInternal*>(this);

        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}

//==============================================================================

CBaseRenderTarget::CBaseRenderTarget()
{
    m_uWidth = 0;
    m_uHeight = 0;
    m_fmtTarget = MilPixelFormat::Undefined;
    m_DeviceTransform.SetToIdentity();
    m_rcBounds.bottom = m_rcBounds.right = m_rcBounds.top = m_rcBounds.left = 0;

    m_pInvalidRegionRgn = NULL;
    m_hrgnNewRectRegion = NULL;
    m_hrgnInvalidRegion = NULL;
    
    m_fHaveEmptyRegionsBeenInvalidated = false;
    m_fHasInvalidRegions = false;
    m_fHasComplexInvalidRegions = false;

    m_cInvalidRegionSize = 0;
}

//==============================================================================

CBaseRenderTarget::~CBaseRenderTarget()
{
    // Ignore return values here - can't do anything else.
    DeleteObject(m_hrgnInvalidRegion);
    DeleteObject(m_hrgnNewRectRegion);
    WPFFree(ProcessHeap, m_pInvalidRegionRgn);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseRenderTarget::Init
//
//  Synopsis:
//      Initialize properties of render target.
//
//      This method expect these member to already be initialized:
//           m_uWidth
//           m_uHeight
//           m_fmtSurface
//
//      Based on those it will initialize:
//           m_rcBounds
//           m_flAlphaMin
//
//      As well as:
//           m_pInvalidatedRegion
//           m_cMaxInvalidatedRegions
//

HRESULT
CBaseRenderTarget::Init()
{
    HRESULT hr = S_OK;

    ColorSpace csSurface;

    m_rcBounds.left = 0;
    m_rcBounds.top  = 0;
    m_rcBounds.right  = m_uWidth;
    m_rcBounds.bottom = m_uHeight;

    IFC( GetPixelFormatColorSpace(m_fmtTarget, &csSurface) );

    if (csSurface == CS_scRGB)
    {
        m_flAlphaMin = FLT_MIN;
    }
    else
    {
        Assert(csSurface == CS_sRGB);

        m_flAlphaMin = Get_Smallest_scRGB_Significant_for_sRGB();
    }

    //
    // Allocate initial dirty region data
    //  (Must be done before creating the flipping chain to allow
    //   dirty state assertions in UpdateFlippingChain.)
    //

    // Future Consideration:  Don't have to allocate for non-display targets
    //
    // Right now we allocate for all classes derived from CBaseRenderTarget,
    // but not all of them need this functionality.
    //

    if (m_hrgnInvalidRegion == NULL)
    {
        IFCW32(m_hrgnInvalidRegion = CreateRectRgn(0, 0, 0, 0));
    }
    else
    {
        IFCW32(SetRectRgn(m_hrgnInvalidRegion, 0, 0, 0, 0));
    }

    if (m_hrgnNewRectRegion == NULL)
    {
        IFCW32(m_hrgnNewRectRegion = CreateRectRgn(0, 0, 0, 0));
    }
    else
    {
        IFCW32(SetRectRgn(m_hrgnNewRectRegion, 0, 0, 0, 0));
    }

    IFCW32_CHECKOOH(GR_GDIOBJECTS, m_hrgnInvalidRegion);
    IFCW32_CHECKOOH(GR_GDIOBJECTS, m_hrgnNewRectRegion);

    IFC(WPFRealloc(
                ProcessHeap,
                Mt(DirtyRegionData),
                (void **)&m_pInvalidRegionRgn,
                s_cbInvalidRegionInitialSize));

    m_cInvalidRegionSize = s_cbInvalidRegionInitialSize;

Cleanup:

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseRenderTarget::GetBounds
//
//  Synopsis:
//      Return bounds of this render target surface
//
//------------------------------------------------------------------------------
STDMETHODIMP_(VOID)
CBaseRenderTarget::GetBounds(
    __out_ecount(1) MilRectF * const pBounds
    )
{
    // Note that we are returning the maximal bounds of the surface and not
    // the current bounds, which may be altered by active layers.
    pBounds->left   = 0;
    pBounds->top    = 0;
    pBounds->right  = static_cast<FLOAT>(m_uWidth);
    pBounds->bottom = static_cast<FLOAT>(m_uHeight);

    return;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseRenderTarget::GetDeviceTransform
//
//  Synopsis:
//      Return a matrix representing the device transform including the DPI
//      adjustment.
//
//------------------------------------------------------------------------------

__outro_ecount(1) const CMILMatrix *
CBaseRenderTarget::GetDeviceTransform() const
{
    return &m_DeviceTransform;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseRenderTarget::UpdateCurrentClip
//
//  Synopsis:
//      Update clip from current bounds and clip state
//

bool
CBaseRenderTarget::UpdateCurrentClip(
    __in_ecount(1) const CAliasedClip &aliasedClip
    )
{
    return IntersectCAliasedClipWithSurfaceRect(
        &aliasedClip,
        m_rcBounds,
        &m_rcCurrentClip
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseRenderTarget::InvalidateRect
//
//  Synopsis:
//      Invalidates a rectangle on the target, this will force this rectangle to
//      be updated on the next present call.
//

HRESULT
CBaseRenderTarget::InvalidateRect(
    __in_ecount(1) CMILSurfaceRect const *pRect
    )
{
    HRESULT hr = S_OK;

    if (pRect->IsEmpty())
    {
        m_fHaveEmptyRegionsBeenInvalidated = true;
        goto Cleanup;
    }

    AssertMsg(
        (pRect->left < pRect->right) &&
        (pRect->top < pRect->bottom),
        "IMILRenderTargetHWND::Invalidate: invalid rect."
        );

    IFCW32(SetRectRgn(m_hrgnNewRectRegion, 
                    pRect->left,
                    pRect->top,
                    pRect->right,
                    pRect->bottom));

    int iResult;
    IFCW32(iResult = CombineRgn(m_hrgnInvalidRegion,
                                m_hrgnInvalidRegion,
                                m_hrgnNewRectRegion,
                                RGN_OR
                                ));
    
    m_fHasInvalidRegions = (iResult != NULLREGION);
    m_fHasComplexInvalidRegions = (iResult == COMPLEXREGION);

Cleanup:
    RRETURN(hr);
}

//==============================================================================

HRESULT
CBaseRenderTarget::ClearInvalidatedRects()
{
    HRESULT hr = S_OK;
    
    IFCW32(SetRectRgn(m_hrgnInvalidRegion, 0, 0, 0, 0));
    IFCW32(SetRectRgn(m_hrgnNewRectRegion, 0, 0, 0, 0));

    m_fHaveEmptyRegionsBeenInvalidated = false;
    m_fHasComplexInvalidRegions = false;
    m_fHasInvalidRegions = false;
    
Cleanup:
    RRETURN(hr);
}

//==============================================================================

HRESULT
CBaseRenderTarget::GetInvalidatedRegion(HRGN rgn, __deref_out RGNDATA **pRegion)
{
    HRESULT hr = S_OK;
    //
    // Compute last field for dirty region data
    //

    // Get required size
    DWORD requiredSize = GetRegionData(m_hrgnInvalidRegion, 0, NULL);

    // Check it's big enough, realloc if necessary
    if (requiredSize > m_cInvalidRegionSize)
    {
        IFC(WPFRealloc(
                    ProcessHeap,
                    Mt(DirtyRegionData),
                    (void **)&m_pInvalidRegionRgn,
                    requiredSize));

        m_cInvalidRegionSize = requiredSize;
    }

    // Get region data
    DWORD returnSize;
    IFCW32(returnSize = GetRegionData(rgn, m_cInvalidRegionSize, m_pInvalidRegionRgn));

    Assert(returnSize == requiredSize);
        
    *pRegion = m_pInvalidRegionRgn;

Cleanup:
    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CBaseRenderTarget::ShouldPresent
//
//  Synopsis:
//      Take the Destination rectangle, compare with dirty bounds regions and 
//      invalidated rectangles to determine if we need to present, and if so 
//      what the minimum region to present is.
//
//      This function does not handle offset presents, because the WPF platform
//      does not support them. Therefore the only input is a destination rectangle,
//      and the output is another (possibly smaller) destination rectangle
//

HRESULT
CBaseRenderTarget::ShouldPresent(
    __in_ecount(1) RECT const *pInputRect,
    __out_ecount(1) CMILSurfaceRect *pResultRect,
    __inout_bcount((CDirtyRegion2::MaxDirtyRegionCount) * sizeof(RECT) + sizeof(RGNDATA)) RGNDATA **ppDirtyRegion,
    __out bool *fPresent
    )
{
    HRESULT hr = S_OK;
    
    bool fShouldPresent = false;
    RGNDATA *pDirtyRegion = NULL;
    CMILSurfaceRect resultRect;

    // Initially set out rect equal to in rect
    resultRect.left   = pInputRect->left;
    resultRect.right  = pInputRect->right;
    resultRect.top    = pInputRect->top;
    resultRect.bottom = pInputRect->bottom;

    Assert(static_cast<UINT>(pInputRect->right)  <= m_uWidth);
    Assert(static_cast<UINT>(pInputRect->bottom) <= m_uHeight);

    if (IsTagEnabled(tagMILDisablePresent))
    {
        goto Cleanup;
    }

    if (m_fHaveEmptyRegionsBeenInvalidated)
    {
        // Present everything.  
        fShouldPresent = true;
    }
    else if (m_fHasInvalidRegions)
    {
        // Intersect the out rect with the invalid regions. May not need to present anything
        bool fSimpleRect = true;
        HRGN rectSourceRegion = m_hrgnInvalidRegion;
        HRGN combineRegion = NULL;

        // Handle the complex invalid region case (more than 1 rectangle)
        if (m_fHasComplexInvalidRegions)
        {
            int regionType = 0;
            IFCNULL(combineRegion = CreateRectRgnIndirect(pInputRect));

            // AND the present rect with invalid regions
            IFCW32(regionType = CombineRgn(combineRegion, combineRegion, m_hrgnInvalidRegion, RGN_AND));

            if (regionType == COMPLEXREGION)
            {
                // Intersection is still complex
                IFC(GetInvalidatedRegion(combineRegion, &pDirtyRegion));
                fShouldPresent = true;
                fSimpleRect = false;
            }
            else
            {
                // Intersection is single rect
                rectSourceRegion = combineRegion;
            }
        }

        if (fSimpleRect)
        {
            // Invalid region is just a single rect or possibly empty. Intersect it and don't return a dirty region.
            
            pDirtyRegion = NULL;
        
            RECT rect;
            IFCW32(GetRgnBox(rectSourceRegion, &rect));

            CMILSurfaceRect invalidBounds(rect.left, rect.top, rect.right, rect.bottom, LTRB_Parameters);
            fShouldPresent = resultRect.Intersect(
                invalidBounds
                );
        }
        
        DeleteObject(combineRegion);
    }

    *ppDirtyRegion = pDirtyRegion;
    *fPresent = fShouldPresent;
    *pResultRect = resultRect;

Cleanup:
    RRETURN(hr);
}

//==============================================================================

#if DBG
void
CBaseRenderTarget::DbgAssertNothingInvalid() const
{
    RECT rect;
    Assert(GetRgnBox(m_hrgnInvalidRegion, &rect) == NULLREGION);
}
#endif




