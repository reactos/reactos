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
//      CDesktopHWNDRenderTarget implementation
//
//      This is a multiple or meta Render Target for rendering a resizeable hwnd
//      on multiple desktop devices.  It handles enumerating the devices and
//      managing an array of sub-targets.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

// Maximum number of invalid regions that can be produced for the invalid
// portions of device's target in it own device bounds.  See GetInvalidRegions.
#define MAX_INVALID_REGIONS_PER_DEVICE  4

// Number of pixels to inflate device present bounds by to produce device
// render bounds.  (Render bounds are trimmed by actual surface bounds after
// inflate.)
#define RENDER_INFLATION_MARGIN 20


MtDefine(CDesktopHWNDRenderTarget, MILRender, "CDesktopHWNDRenderTarget");

ExternTag(tagMILTraceBBInvalidation);
ExternTag(tagMILTraceDesktopState);


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::ComputeRenderAndAdjustPresentBounds
//
//  Synopsis:
//      Give meta data entry with valid device present bounds and new surface
//      width and height, compute new device render bounds and adjust present
//      bounds as needed.
//
//------------------------------------------------------------------------------

void CDesktopHWNDRenderTarget::ComputeRenderAndAdjustPresentBounds(
    __inout_ecount(1) MetaData &oDevData,
    __in_range(0,INT_MAX) UINT uMaxRight,
    __in_range(0,INT_MAX) UINT uMaxBottom
    ) const
{
    //
    // Assert to validate that RENDER_INFLATION_MARGIN may be safely added to
    // or subtracted from starting rectangle without worry of overflow.
    C_ASSERT(RENDER_INFLATION_MARGIN <= INT_MAX);
    Assert(oDevData.rcLocalDevicePresentBounds.left >= 0);
    Assert(oDevData.rcLocalDevicePresentBounds.top  >= 0);
    Assert(oDevData.rcLocalDevicePresentBounds.right  > 0);
    Assert(oDevData.rcLocalDevicePresentBounds.bottom > 0);

    if (   UNCONDITIONAL_EXPR(RENDER_INFLATION_MARGIN > 0)
           // For layered windows the system retains the contents presented
           // even if offscreen and won't make a paint request.  For these
           // windows the rendered area and the presented area should be the
           // same.
           //
           // But if the system configuration has multiple displays then the
           // present area could overlap and compete with one another if
           // inflated.  System managed layered windows can avoid this conflict
           // when the GDI sprite is kept in video memory.  This is true with
           // XDDM, but WDDM force GDI to use a single sysmtem memory sprite.
           //
           // Therefore render area may be inflated if:
           //   1. window is not layered OR
           //   2. there is only one display OR
           //   3. layered window is application managed and driver model is
           //      XDDM, which is checked by lack of IDirect3D9Ex.
           //
        && (   (m_eWindowLayerType == MilWindowLayerType::NotLayered)
            || (m_cRT == 1)
            || (   (m_eWindowLayerType == MilWindowLayerType::ApplicationManagedLayer)
                && (m_pDisplaySet->D3DExObject() == NULL))
       )   )
    {

        // Operation is essentially:
        //      oDevData.rcLocalDeviceRenderBounds.Inflate(
        //          RENDER_INFLATION_MARGIN, RENDER_INFLATION_MARGIN);
        //      oDevData.rcLocalDeviceRenderBounds.Intersect(
        //          CMILSurfaceRect(0,0,uMaxRight,uMaxBottom,LTRB);
        // but with overflow safe math. rcLocalDevicePresentBounds.right/bottom
        // + RENDER_INFLATION_MARGIN could overflow INT range.

        oDevData.rcLocalDeviceRenderBounds.left =
            max<INT>(0, oDevData.rcLocalDevicePresentBounds.left - RENDER_INFLATION_MARGIN);

        oDevData.rcLocalDeviceRenderBounds.top =
            max<INT>(0, oDevData.rcLocalDevicePresentBounds.top - RENDER_INFLATION_MARGIN);

        oDevData.rcLocalDeviceRenderBounds.right = static_cast<INT>
            (min(uMaxRight, static_cast<UINT>
                 (oDevData.rcLocalDevicePresentBounds.right + RENDER_INFLATION_MARGIN)));

        oDevData.rcLocalDeviceRenderBounds.bottom = static_cast<INT>
            (min(uMaxBottom, static_cast<UINT>
                 (oDevData.rcLocalDevicePresentBounds.bottom + RENDER_INFLATION_MARGIN)));

        // When presented contents are retained, present everything that is
        // rendered.  Contents are retained when the window is layered.
        if (m_eWindowLayerType != MilWindowLayerType::NotLayered)
        {
            oDevData.rcLocalDevicePresentBounds = oDevData.rcLocalDeviceRenderBounds;
        }
    }
    else
    {
        // Don't inflate anything.
        oDevData.rcLocalDeviceRenderBounds = oDevData.rcLocalDevicePresentBounds;
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::EditMetaData
//
//  Synopsis:
//      Called by the CDesktopRenderTarget after the meta data is initialized by
//      the desktop render target. This gives the subclass a way of changing the
//      meta data before the render targets are created.
//
//------------------------------------------------------------------------------

HRESULT CDesktopHWNDRenderTarget::EditMetaData()
{
    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::operator new
//
//  Synopsis:
//      Allocate memory for a CDesktopHWNDRenderTarget with cRTs number of
//      render targets.  Extra space is allocated for MetaData and
//      MAX_INVALID_REGIONS_PER_DEVICE potential invalid rects per render
//      target.
//
//------------------------------------------------------------------------------

void * __cdecl CDesktopHWNDRenderTarget::operator new(
    size_t cb,
    size_t cRTs
    )
{
    void *pNew = NULL;

    // Calculate extra space for MetaData array (m_rgMetaData) and invalid
    // region array (m_rgInvalidRegions). The order of memory layout is this,
    // then MetaData, then invalid regions.
    size_t cbExtraData = cRTs * (sizeof(MetaData) + MAX_INVALID_REGIONS_PER_DEVICE*sizeof(CMilRectF));

    // Check for overflow
    if (cbExtraData > cRTs)
    {
        cb += cbExtraData;

        // Check for overflow
        if (cb >= cbExtraData)
        {
            pNew = WPFAlloc(ProcessHeap,
                                 Mt(CDesktopHWNDRenderTarget),
                                 cb);
        }
    }

    return pNew;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CDesktopRenderTarget constructor.
//
//------------------------------------------------------------------------------

#pragma warning( push )
#pragma warning( disable : 4355 )

CDesktopHWNDRenderTarget::CDesktopHWNDRenderTarget(
    UINT cMaxRTs,
    __inout_ecount(1) CDisplaySet const *pDisplaySet,
    MilWindowLayerType::Enum eWindowLayerType
    )
    : CDesktopRenderTarget(
        reinterpret_cast<MetaData *>(reinterpret_cast<BYTE *>(this) + sizeof(*this)),
        cMaxRTs,
        pDisplaySet
        ),
      m_eWindowLayerType(eWindowLayerType),
      m_rgInvalidRegions(reinterpret_cast<CMilRectF *>(&m_rgMetaData[cMaxRTs]))
{
    // override these variables' initialization
    m_fAccumulateValidBounds = true;
    m_eState = NeedSetPosition;

    // Initial surface bounds for HWND RTs is empty until SetPosition is call
    // to specify a size.
    m_rcSurfaceBounds.SetEmpty();

    m_ePresentTransparency = MilTransparency::Opaque;
    m_bPresentAlpha = 255;
    m_crPresentColorKey = RGB(0, 0, 0);
}

#pragma warning( pop )


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::ResizeSubRT
//
//  Synopsis:
//      Update RT size (Resize) for sub-RT at given index (i).  If Hw fails to
//      resize and Sw is allowed then try to use a Sw RT.
//

HRESULT
CDesktopHWNDRenderTarget::ResizeSubRT(
    __in_range(<=, (this->m_cRT)) UINT i,
    __in_range(>=, 1) UINT uWidthNew,
    __in_range(>=, 1) UINT uHeightNew
    )
{
    HRESULT hr = E_FAIL;

    MetaData *pDevData = &m_rgMetaData[i];

    //
    // We're activating on a new display or recreating a
    // surface and therefore the entire backbuffer is invalid. 
    //

    pDevData->rcLocalDeviceValidContentBounds.SetEmpty();

    //
    // If this monitor has a HW RT, then try to resize with
    // it first.
    //
    if (pDevData->pHwDisplayRT)
    {
        hr = THR(pDevData->pHwDisplayRT->Resize(
            uWidthNew,
            uHeightNew
            ));

        if (SUCCEEDED(hr))
        {
            // Upon success make HW RT the target if it isn't already.
            if (pDevData->pInternalRTHWND != pDevData->pHwDisplayRT)
            {
                //
                // Make new SW RT active, but don't get rid of
                // the HW RT.  We'll try to reactivate it on
                // the next resize.
                //

                pDevData->pInternalRT->Release();
                pDevData->pInternalRT = pDevData->pHwDisplayRT;
                pDevData->pInternalRT->AddRef();
                pDevData->pInternalRTHWND = pDevData->pHwDisplayRT;

                // Mark as not previously enabled to call
                // UpdatePresentProperties
                pDevData->fEnable = false;

                // Release the old SW RT - recreation is not intense
                Verify(pDevData->pSwHWNDRT->Release() == 0);
                pDevData->pSwHWNDRT = NULL;

                //
                // Log when successfully falling back to a
                // completely software based render target.
                //
                // Note that there is no logging when a RT may
                // return to hardware rendering.
                //

                EventWriteUnexpectedSoftwareFallback(UnexpectedSWFallback_ResizeFailed);
            }
        }
    }

    //
    // Check if there is a SW RT to resize or create.
    //

    if (   FAILED(hr)
        && hr != WGXERR_DISPLAYSTATEINVALID)
    {
        // At this point either a HW RT failed to resize
        // or there is no HW RT (hr unchanged from
        // it initial E_FAIL value).
        Assert(pDevData->pHwDisplayRT ||
               (hr == E_FAIL));

        if (pDevData->pSwHWNDRT)
        {
            // In either case, the SW RT should be active
            Assert(pDevData->pInternalRTHWND ==
                   pDevData->pSwHWNDRT);
            // Resize the SW RT.
            hr = THR(pDevData->pSwHWNDRT->Resize(
                uWidthNew,
                uHeightNew
                ));
        }
        else if (!(m_dwRTInitFlags & MilRTInitialization::HardwareOnly))
        {
            // We should only being handling fallback here.
            Assert(pDevData->pHwDisplayRT);

            // The HW RT should be active
            Assert(pDevData->pInternalRTHWND ==
                   pDevData->pHwDisplayRT);

            //
            // Check for special handling of XP SP2 layered windows
            //
            bool const fFullPresentLayeredWindow =
                   ((m_dwRTInitFlags & MilRTInitialization::PresentUsingMask) == MilRTInitialization::PresentUsingUpdateLayeredWindow)
                && !OSSupportsUpdateLayeredWindowIndirect();

            //
            // Create a fallback SW RT
            //
            bool fUsePrimaryDisplayObject = fFullPresentLayeredWindow;

            CSwRenderTargetHWND *pSwHWNDRT;
            CDisplay const *pDisplay = DisplaySet()->Display(fUsePrimaryDisplayObject ? 0 : i);

            hr = THR(CSwRenderTargetHWND::Create(
                m_hwnd,
                m_eWindowLayerType,
                pDisplay,
                pDisplay->GetDisplayId(),
                uWidthNew,
                uHeightNew,
                m_dwRTInitFlags,
                &pSwHWNDRT
                ));


            if (SUCCEEDED(hr))
            {
                // Handle special case for XP SP2 layered windows
                if (fFullPresentLayeredWindow)
                {
                    //
                    // To get here Hw must have been enabled across all
                    // displays.  Since we can only use one Sw RT all other RTs
                    // must now get destroyed.
                    //

                    for (i = 0; i < m_cRT; i++)
                    {
                        Assert(m_rgMetaData[i].pSwHWNDRT == NULL);
                        Assert(m_rgMetaData[i].pHwDisplayRT != NULL);
                        ReleaseInterface(m_rgMetaData[i].pHwDisplayRT);
                        m_rgMetaData[i].pInternalRTHWND = NULL;
                        ReleaseInterface(m_rgMetaData[i].pInternalRT);
                        m_rgMetaData[i].rcVirtualDeviceBounds.SetEmpty();
                    }

                    SetSingleSubRT();

                    // Repoint RT entry to first entry as it is the only valid
                    // one now.
                    pDevData = &m_rgMetaData[0];
                }
                else
                {
                    pDevData->pInternalRT->Release();
                }

                pDevData->pSwHWNDRT = pSwHWNDRT;    // Transfer reference
                pDevData->pInternalRT = pDevData->pSwHWNDRT;
                pDevData->pInternalRT->AddRef();
                pDevData->pInternalRTHWND = pDevData->pSwHWNDRT;

                // Mark as not previously enabled to call
                // UpdatePresentProperties just below here.
                pDevData->fEnable = false;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!pDevData->fEnable)
        {
            pDevData->pInternalRTHWND->UpdatePresentProperties(
                m_ePresentTransparency,
                m_bPresentAlpha,
                m_crPresentColorKey
                );

            pDevData->fEnable = true;
        }
    }
    else
    {
        // Resizing failed - this RT is effectively disabled.
        pDevData->fEnable = false;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::SetPosition
//
//  Synopsis:
//      Update position of render target in desktop including size
//

STDMETHODIMP CDesktopHWNDRenderTarget::SetPosition(
    __in_ecount(1) MilRectF const *prc
    )
{
    HRESULT hr = S_OK;

    // Bounds of the closest-monitor of a window that is
    // withing the overall display bounds (the virtual rectangle created
    // by the sum of all monitors), but doesn't happen to intersect any
    // particular monitor.
    //
    // When this edge-case is detected, we will use MonitorFromWindow function
    // to match the HWND to an appropriate monitor, and query the HMONITOR
    // for its bounds information.
    CMILSurfaceRect rcClosestMonitorBounds(0, 0, 0, 0, LTRB_Parameters);

    CMILSurfaceRect rcNewPosition;

    //
    // Check if display state has changed
    //

    if (DangerousHasDisplayChanged())
    {
        // Mark need to recreate
        TransitionToState(NeedRecreate
                          DBG_COMMA_PARAM("SetPosition"));
    }

    if (m_eState == NeedRecreate)
    {
        // Notify caller of invalid display state
        MIL_THR(WGXERR_DISPLAYSTATEINVALID);

        // Change operation to release all resources
        rcNewPosition.SetEmpty();
    }
    else
    {
        //   Need to filter extreme values
        rcNewPosition.left   = RasterizerConvertRealToInteger(prc->left);
        rcNewPosition.top    = RasterizerConvertRealToInteger(prc->top);
        rcNewPosition.right  = RasterizerConvertRealToInteger(prc->right);
        rcNewPosition.bottom = RasterizerConvertRealToInteger(prc->bottom);
    }

    if (rcNewPosition.IsEquivalentTo(m_rcCurrentPosition))
    {
        if (m_eState & FlagNeedSetPosition)
        {
            Assert(m_eState == NeedResize || rcNewPosition.IsEmpty());
            TransitionToState(Ready DBG_COMMA_PARAM("SetPosition"));
        }
        goto Cleanup;
    }

    Assert(rcNewPosition.IsWellOrdered());
    Assert(m_rcCurrentPosition.IsWellOrdered());

    UINT uWidthNew = static_cast<UINT>(rcNewPosition.right - rcNewPosition.left);
    UINT uHeightNew = static_cast<UINT>(rcNewPosition.bottom - rcNewPosition.top);

    UINT const uWidthOld = static_cast<UINT>(m_rcCurrentPosition.right - m_rcCurrentPosition.left);
    UINT const uHeightOld = static_cast<UINT>(m_rcCurrentPosition.bottom - m_rcCurrentPosition.top);

    bool fResize = ((uWidthNew != uWidthOld) || (uHeightNew != uHeightOld));

    // This flag is usually set when DWM composition is enabled
    bool fDisableDisplayClipping = 
        ((m_dwRTInitFlags & MilRTInitialization::DisableDisplayClipping) != 0); 

    // Our goal is the get multimon behavior to match the single-mon
    // behavior on Win8 and above
    bool fDisableMultimonDisplayClipping = 
        fDisableDisplayClipping && DWMAPI::IsWindows8OrGreater();

    // Check whether the multi-mon behavior needs to be modified based on compat flag set by the user 
    bool fMultimonClippingCompatFlagEnabled =
        ((m_dwRTInitFlags & MilRTInitialization::IsDisableMultimonDisplayClippingValid) != 0);

    if (fMultimonClippingCompatFlagEnabled)
    {
        // Get the value of DisableMultimonDisplayClipping flag
        bool fDisableMultimonDisplayClippingFlag =
            ((m_dwRTInitFlags & MilRTInitialization::DisableMultimonDisplayClipping) != 0);

        // When the user has set the DisableMultimonDisplayClipping flag, we change the default behavior based on the OS we are running on as shown
        // in this K-Map.
        // __________________________________________________________________________________________________________________________________________________
        // |            A                  |      B         |                  C                      |               D                     |   A XNOR B    |
        // |_______________________________|________________|_________________________________________|_____________________________________|_______________|
        // |DisableMultimonDisplayClipping |    OS >= Win8  |   Default                               |   fDisableMultimonDisplayClipping   |               |
        // | compatibility flag            |                |       fDisableMultimonDisplayClipping   |                                     |               |
        // -------------------------------------------------------------------------------------------------------------------------------------------------|
        // |                               |                |                                         |                                     |               |
        // |          false                |      false     |       false                             |      false                          |      true     |
        // |          false                |      true      |       fDisableDisplayClipping           |      !fDisableDisplayClipping       |      false    |
        // |          true                 |      false     |       false                             |      true (!false)                  |      false    |
        // |          true                 |      true      |       fDisableDisplayClipping           |      fDisableDisplayClipping        |      true     |
        // |                               |                |                                         |                                     |               |
        // --------------------------------------------------------------------------------------------------------------------------------------------------
        //
        // From this, we can see that when (A XNOR B), then D = C else D = ~C.
        //
        // D = (A XNOR B) ? C : !C
        //
        //         or
        //
        // fDisableMultimonDisplayClipping =
        //    (fDisableMultimonDisplayClippingFlag == DWMAPI::IsWindows8OrGreater()) ? fDisableMultimonDisplayClipping : !fDisableMultimonDisplayClipping;
        //
        // Note that we can express (A XNOR B) as (A == B) when A and B are bool's.
        
        fDisableMultimonDisplayClipping = 
            (fDisableMultimonDisplayClippingFlag == DWMAPI::IsWindows8OrGreater()) ? fDisableMultimonDisplayClipping : !fDisableMultimonDisplayClipping;   
    }

    //
    // Check intersection of window with each monitor
    //

    // We may need to retry the window-monitor intersection logic if none
    // of the monitors are found to intersect the window, but the window
    // happens to be within the display-bounds anyway. In other words, 
    // the window has 'fallen through the cracks' into the interstitial space between
    // monitors in a multi-monitor coordinate space.
    bool fRetryIdentifyIntersectingMonitor = false;
    bool fIntersectsAnyMonitor = false;

    do
    {
        for (UINT i = 0; i < m_cRT; i++)
        {
            MetaData &oDevData = m_rgMetaData[i];

            //
            // Update local device present bounds by finding intersection of new
            // position and virtual device bounds and then translating into local
            // device space.
            //

            oDevData.rcLocalDevicePresentBounds = rcNewPosition;

            //
            // If the local device present bounds is outside the virtual device present bounds, 
            // but we happen to have prior knowledge that this is the right virtual device, then
            // proceed anyway.
            // In this situation, our prior knowledge is due to the following:
            // (a) The local device doesn't intersect any virtual device bounds (!fIntersectsAnyMonitor) 
            // (b) The local device is within the overall display bounds (rcClosestMonitorBounds != empty)
            // When both of these are true, a retry is requested by setting fRetryIdentifyIntersectingMonitor = true.
            //
            // If rcClosestMonitorBounds == oDevData.rcVirtualDeviceBounds, we know that the current 
            // virtual device under consideration is the one that is the right match.
            if (oDevData.rcLocalDevicePresentBounds.Intersect(oDevData.rcVirtualDeviceBounds) ||
                (fRetryIdentifyIntersectingMonitor && rcClosestMonitorBounds.IsEquivalentTo(oDevData.rcVirtualDeviceBounds)))
            {
                fIntersectsAnyMonitor = true;

                // Check if the window is outside the device (monitor) bounds in a multi-mon setup 
                if (fDisableMultimonDisplayClipping                                // display clipping is disabled
                    && (m_cRT > 1)                                                 // multimon
                    && !oDevData.rcVirtualDeviceBounds.DoesContain(rcNewPosition)) // window extends outside the monitor
                {
                    // do not clip to device bounds
                    oDevData.rcLocalDevicePresentBounds = rcNewPosition;
                }

                //
                // Translate intersection from virtual device space to local device
                // space.  Note that this isn't needed when the intersection is
                // empty, which is why this is under the if.
                //
                oDevData.rcLocalDevicePresentBounds.Offset(-rcNewPosition.left, -rcNewPosition.top);

                //
                // Compute device render bounds from present bounds and complete
                // surface bounds.  Render bounds are present bounds plus an
                // additional margin for area outside present bounds to enable some
                // movement of window through monitor boundaries without need to
                // rerender.  In some cases present bounds may also be adjusted.
                //
                ComputeRenderAndAdjustPresentBounds(oDevData, uWidthNew, uHeightNew);

                //
                // Check for a critical change in visibility
                //

                if (!oDevData.fEnable || fResize)
                {
                    //
                    // Resize and enable this RT for the subsequent frame.
                    //
                    //  The minimal size would be determined by how the window
                    //  location intersects each monitor.  This could be
                    //  significant for windows that extend beyond a single
                    //  monitor and potential exceed HW RT size limits.
                    //

                    if (!oDevData.fEnable)
                    {
                        TraceTag((tagMILTraceDesktopState,
                            "0x%p Desktop: Enabling rendering to monitor %u",
                            this,
                            i));
                    }

                    HRESULT hrResize = ResizeSubRT(i, uWidthNew, uHeightNew);

                    if (FAILED(hrResize))
                    {
                        // Signal need to recreate OR
                        // that positioning is incomplete.
                        TransitionToState((hrResize == WGXERR_DISPLAYSTATEINVALID) ?
                            NeedRecreate :
                            NeedSetPosition
                            DBG_COMMA_PARAM("SetPosition"));

                        // Remember the most recent Resize/Create failure.
                        MIL_THR(hrResize);

                        // Reset window size to 0x0 and restart loop
                        // to resize all RT to 0x0 thereby freeing
                        // no longer usable resources.
                        uWidthNew = 0;
                        uHeightNew = 0;
                        rcNewPosition.SetEmpty();
                        i = UINT_MAX; // -1 (or 0xffffffff) + 1 = 0
                        Assert(i+1==0); // in case i's type is changed.
                        continue;
                    }
                }

                // Always update the position of each render target.
                if (oDevData.fEnable)
                {
                    POINT pos = {rcNewPosition.left, rcNewPosition.top};
                    oDevData.pInternalRTHWND->SetPosition(pos);
                }
            }
            else
            {
                // Is this becoming unused?
                if (oDevData.fEnable)
                {
                    // Set trivial visibility for this to invisible.
                    oDevData.fEnable = false;
                    TraceTag((tagMILTraceDesktopState,
                        "0x%p Desktop: Disabling rendering to monitor %u",
                        this,
                        i));
                    IGNORE_HR(oDevData.pInternalRTHWND->Resize(0, 0));
                }
            }
        }

        if (!fIntersectsAnyMonitor)
        {
            // Identify whether the window is within the overall display bounds
            // even though it failed to intersect any of the monitors bounds. 
            // If it does, then schedule another SetPosition pass.

            // Set thread's DPI_AWARENESS_CONTEXT to match that of the HWND
            // so that m_pDisplaySet->GetBounds() will return the appropriate
            // bounds
            wpf::util::DpiAwarenessScope<HWND> dpiScope(m_hwnd);

            auto rcDisplayBounds = m_pDisplaySet->GetBounds();
            if (rcDisplayBounds.Intersect(rcNewPosition))
            {
                auto hMonitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFOEX mi;
                mi.cbSize = sizeof(MONITORINFOEX);
                if (GetMonitorInfo(hMonitor, &mi))
                {
                    rcClosestMonitorBounds = mi.rcMonitor;
                    fRetryIdentifyIntersectingMonitor = true;
                }
            }
        }
    }
    while (fRetryIdentifyIntersectingMonitor && !fIntersectsAnyMonitor);

    //
    // If processing a resize request, make sure cached bounds match current
    // result
    //

    if (fResize)
    {
        Assert(m_rcSurfaceBounds.left == 0);
        Assert(m_rcSurfaceBounds.top  == 0);
        m_rcSurfaceBounds.right = static_cast<LONG>(uWidthNew);
        m_rcSurfaceBounds.bottom = static_cast<LONG>(uHeightNew);
    }

    m_rcCurrentPosition = rcNewPosition;

    // Future Consideration:   Update surface bounds to collective device bounds
    //  so that caller may trim content that won't be retained as valid.

    if (   SUCCEEDED(hr)
        && (m_eState & FlagNeedSetPosition))
    {
        TransitionToState(Ready DBG_COMMA_PARAM("SetPosition"));
    }
    else if (m_eState == NeedRecreate)
    {
        Assert(hr == WGXERR_DISPLAYSTATEINVALID);
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::GetInvalidRegions
//
//  Synopsis:
//      Return portions of target that have invalid content due to last
//      SetPosition call.  Valid regions are grown when calls to Clear are made
//      after SetPosition.
//

STDMETHODIMP CDesktopHWNDRenderTarget::GetInvalidRegions(
    __deref_outro_ecount(*pNumRegions) MilRectF const ** const prgRegions,
    __out_ecount(1) UINT *pNumRegions,
    __out bool *fWholeTargetInvalid    
    )
{
    bool fWholeTargetInvalidLocal = false;

    UINT uNumInvalidRegions = 0;

    for (UINT i = 0; i < m_cRT; i++)
    {
        MetaData &oDevData = m_rgMetaData[i];

        if (   oDevData.fEnable
               // Present bounds must contain valid content by the time Present
               // is called.  If all is valid then there is nothing to return.
            && !oDevData.rcLocalDeviceValidContentBounds.DoesContain(
                    oDevData.rcLocalDevicePresentBounds)
           )
        {
            // Convenience references to avoid large names through out routine
            CMILSurfaceRect const &rcRender = oDevData.rcLocalDeviceRenderBounds;
            CMILSurfaceRect &rcValid = oDevData.rcLocalDeviceValidContentBounds;

            TraceTag((tagMILTraceDesktopState,
                      "0x%p Desktop: Invalidated present region on monitor %u",
                      this,
                      i));
            //
            // rcValidRenderArea is valid content that contributes to needed
            // content and doesn't need rerendered.
            //

            // Starting situation could look like this:

            //           Render Bounds
            //           +------------------------------+---------+
            //           |                              |         |
            //           |      Invalid Top Edge        |         |
            //           |                              |         |
            //   +--------------------------------------|         |
            //   |                                      | Invalid |
            //   |       `             Valid            |  Right  |
            //   |                     Content          |  Edge   |
            //   |       `             Bounds           |         |
            //   |                                      |         |
            //   |       `                              |         |
            //   +--------------------------------------|         |
            //           |                              |         |
            //           |      Invalid Bottom Edge     |         |
            //           |                              |         |
            //           +------------------------------+---------+

            CMILSurfaceRect rcValidRenderArea = rcValid;

            if (!rcValidRenderArea.Intersect(rcRender))
            {
                // None of valid content really helpful. Don't return bounds
                // just set flag indicating that entire target is invalid.
                
                fWholeTargetInvalidLocal = true;

                //
                // Find largest valid area that can reasonably be retained
                // given that valid area does not intersect render area, which
                // must be included in the new valid area.
                //
                // Extend render rectangle by adjacent portions of currently
                // valid rectangle to form new valid rectangle.
                //

                ExtendBaseByAdjacentSectionsOfRect(
                    IN  rcRender,   /* base rectangle */
                    IN  rcValid,    /* disjoint rectangle */
                    OUT rcValid     /* extended base rectangle */
                    );
            }
            else
            {
                C_ASSERT(MAX_INVALID_REGIONS_PER_DEVICE == 4);
                //
                // Generate up to four invalid rectangles by subtracting valid
                // rectangle from render bounds.  Present bounds must contain
                // valid content by the time Present is called, but since we
                // have to do work we return invalid based on render bounds so
                // that we are ready for the next movement.
                //
                // Note that no attempt is made to generate the smallest number
                // of rectangles possible.  A simple layout is always used:

                //    +---------+---------------------------+---------+
                //    |         |                           |         |
                //    |         |   2. Invalid Top Edge     |         |
                //    |         |                           |         |
                //    |   1.    |---------------------------|    4.   |
                //    | Invalid |                           | Invalid |
                //    |  Left   |       Valid               |  Right  |
                //    |  Edge   |       Region              |  Edge   |
                //    |         |                           |         |
                //    |         |                           |         |
                //    |         |---------------------------|         |
                //    |         |                           |         |
                //    |         |   3. Invalid Bottom Edge  |         |
                //    |         |                           |         |
                //    +---------+---------------------------+---------+
                //

                #if DBG
                UINT uDbgOriginalInvalidCount = uNumInvalidRegions;
                #endif

                // If render bounds don't contain valid content bounds, then
                // below code could return invalid areas bigger than is
                // actually needed.  Intersect above should ensure this.
                Assert(rcRender.DoesContain(rcValidRenderArea));

                if (rcRender.left < rcValidRenderArea.left)
                {
                    // Add invalid left edge
                    m_rgInvalidRegions[uNumInvalidRegions++] = CMilRectF(
                        static_cast<FLOAT>(rcRender.left),
                        static_cast<FLOAT>(rcRender.top),
                        static_cast<FLOAT>(rcValidRenderArea.left),
                        static_cast<FLOAT>(rcRender.bottom),
                        LTRB_Parameters
                        );

                    // Push left valid edge left to include render area
                    rcValid.left = rcRender.left;

                    // Move top and bottom edges to render area edges.
                    rcValid.top = rcRender.top;
                    rcValid.bottom = rcRender.bottom;

                    // Keep potential valid area right of render area; other
                    // cases will trim as needed.
                }

                if (rcRender.top < rcValidRenderArea.top)
                {
                    // Add invalid top edge (between left and right edges)
                    m_rgInvalidRegions[uNumInvalidRegions++] = CMilRectF(
                        static_cast<FLOAT>(rcValidRenderArea.left),
                        static_cast<FLOAT>(rcRender.top),
                        static_cast<FLOAT>(rcValidRenderArea.right),
                        static_cast<FLOAT>(rcValidRenderArea.top),
                        LTRB_Parameters
                        );

                    // Push top valid edge up to include render area
                    rcValid.top = rcRender.top;

                    // Move left and right edges to render area edges.
                    rcValid.left = rcRender.left;
                    rcValid.right = rcRender.right;

                    // Keep potential valid area below render area; other cases
                    // will trim as needed.
                }

                if (rcRender.bottom > rcValidRenderArea.bottom)
                {
                    // Add invalid bottom edge (between left and right edges)
                    m_rgInvalidRegions[uNumInvalidRegions++] = CMilRectF(
                        static_cast<FLOAT>(rcValidRenderArea.left),
                        static_cast<FLOAT>(rcValidRenderArea.bottom),
                        static_cast<FLOAT>(rcValidRenderArea.right),
                        static_cast<FLOAT>(rcRender.bottom),
                        LTRB_Parameters
                        );

                    // Push bottom valid edge down to include render area
                    rcValid.bottom = rcRender.bottom;

                    // Move left and right edges to render area edges.
                    rcValid.left = rcRender.left;
                    rcValid.right = rcRender.right;

                    // Keep potential valid area above render area; other cases
                    // will trim as needed.
                }

                if (rcRender.right > rcValidRenderArea.right)
                {
                    // Add invalid right edge
                    m_rgInvalidRegions[uNumInvalidRegions++] = CMilRectF(
                        static_cast<FLOAT>(rcValidRenderArea.right),
                        static_cast<FLOAT>(rcRender.top),
                        static_cast<FLOAT>(rcRender.right),
                        static_cast<FLOAT>(rcRender.bottom),
                        LTRB_Parameters
                        );

                    // Push right valid edge right to include render area
                    rcValid.right = rcRender.right;

                    // Move top and bottom edges to render area edges.
                    rcValid.top = rcRender.top;
                    rcValid.bottom = rcRender.bottom;

                    // Keep potential valid area left of render area; other
                    // cases will trim as needed.
                }

                Assert(rcValid.DoesContain(rcRender));

                #if DBG
                Assert(uNumInvalidRegions <= uDbgOriginalInvalidCount + MAX_INVALID_REGIONS_PER_DEVICE);
                #endif
            }
        }
    }

    Assert(uNumInvalidRegions <= MAX_INVALID_REGIONS_PER_DEVICE*m_cRT);

    *pNumRegions = uNumInvalidRegions;
    *prgRegions = m_rgInvalidRegions;
    *fWholeTargetInvalid = fWholeTargetInvalidLocal;

    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::GetIntersectionWithDisplay
//
//  Synopsis:
//      Return rectangle of intersection between target position and a single
//      display, in virtual desktop space.
//

STDMETHODIMP_(VOID) CDesktopHWNDRenderTarget::GetIntersectionWithDisplay(
    UINT iDisplay,
    __out_ecount(1) MilRectL &rcIntersection
    )
{
    CMILSurfaceRect &rcIntersectionOut = *(static_cast<CMILSurfaceRect *>(&rcIntersection));

    if (iDisplay < m_cRT)
    {
        rcIntersectionOut = m_rgMetaData[iDisplay].rcVirtualDeviceBounds;

        rcIntersectionOut.Intersect(m_rcCurrentPosition);
    }
    else
    {
        rcIntersectionOut = rcIntersectionOut.sc_rcEmpty;
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::UpdatePresentProperties
//
//  Synopsis:
//

STDMETHODIMP CDesktopHWNDRenderTarget::UpdatePresentProperties(
    MilTransparency::Flags transparencyFlags,
    FLOAT constantAlpha,
    __in_ecount(1) MilColorF const &colorKey
    )
{
    HRESULT hr = S_OK;

    if (m_eWindowLayerType == MilWindowLayerType::NotLayered)
    {
        Assert(transparencyFlags == MilTransparency::Opaque);
        goto Cleanup;
    }

    m_ePresentTransparency = transparencyFlags;
    m_bPresentAlpha = static_cast<BYTE>(CFloatFPU::SmallRound(ClampAlpha(constantAlpha)*255.0f));

    MilColorB oColorKey = Convert_MilColorF_scRGB_To_Premultiplied_MilColorB_sRGB(&colorKey);
    m_crPresentColorKey = RGB(MIL_COLOR_GET_RED(oColorKey),
                              MIL_COLOR_GET_GREEN(oColorKey),
                              MIL_COLOR_GET_BLUE(oColorKey));

    if (m_eWindowLayerType == MilWindowLayerType::SystemManagedLayer)
    {
        // Window updates are handled right here and there is no need to let
        // individual RTs know.
        IFCW32(SetLayeredWindowAttributes(
            m_hwnd,
            m_crPresentColorKey,
            m_bPresentAlpha,
            (  ((m_ePresentTransparency & MilTransparency::ConstantAlpha) ? LWA_ALPHA : 0)
             | ((m_ePresentTransparency & MilTransparency::ColorKey) ? LWA_COLORKEY : 0))
            ));
    }
    else
    {
        Assert(m_eWindowLayerType == MilWindowLayerType::ApplicationManagedLayer);

        // Transparency settings will be updated via UpdateLayeredWindow call
        // made by render targets when Present is called; so make sure each one
        // has updated settings.

        for (UINT i = 0; i < m_cRT; i++)
        {
            MetaData const &oDevData = m_rgMetaData[i];

            // Only update enabled RTs.  Disabled ones may not be sufficiently
            // initialized to properly handle the call.  For example,
            // CSwPresenter32bppGDI needs CreateBackBuffers call before
            // UpdatePresentProperties.  CreateBackBuffers is called only for a
            // non-zero Resize.
            if (oDevData.fEnable)
            {
                oDevData.pInternalRTHWND->UpdatePresentProperties(
                    m_ePresentTransparency,
                    m_bPresentAlpha,
                    m_crPresentColorKey
                    );
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::Present
//
//  Synopsis:
//      Cause the contents of the back-buffer to show up on the various devices.
//
//  Arguments:
//      prc - area of the buffer to use as the source
//      NULL is treated as the entire buffer for this parameter.
//
//------------------------------------------------------------------------------

STDMETHODIMP CDesktopHWNDRenderTarget::Present(
    )
{
    API_ENTRY_NOFPU("CDesktopRenderTarget::Present");

    HRESULT hr = S_OK;

    Assert(m_eState == Ready);

    MIL_THR(CDesktopRenderTarget::Present());

    if (hr == WGXERR_DISPLAYSTATEINVALID)
    {
        //
        // If the display state has just become invalid call
        // SetPosition to release RT resources by sizing them all
        // to 0 x 0.
        //

        Assert(m_eState == NeedRecreate);
        IGNORE_HR(SetPosition(&CMilRectF::sc_rcEmpty));
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_INCORRECT_SIZE))
    {
        //
        // If the window has changed sizes then more coordination
        // between the UI thread and rendering thread is needed.
        //
        // This will means a call to SetPosition is needed.  There are
        // no known cases where the caller won't know there has been a
        // size change.  The caller simply needs to call SetPosition
        // when it notices the change (even if the result is going to
        // be the same size.)
        //

        TransitionToState(NeedResize DBG_COMMA_PARAM("Present"));
    }

    API_CHECK(hr);

    return hr;
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopHWNDRenderTarget::DbgIsValidTransition
//
//  Synopsis:
//      Validate state transition
//
//------------------------------------------------------------------------------
bool
CDesktopHWNDRenderTarget::DbgIsValidTransition(
    enum State eNewState
    )
{
    switch (m_eState)
    {
    case Ready:
        switch (eNewState)
        {
        case NeedSetPosition:
        case NeedResize:
        case NeedRecreate:
            return TRUE;
        }
        break;

    case NeedSetPosition:
        switch (eNewState)
        {
        case Ready:
        case NeedSetPosition:
        case NeedRecreate:
            return TRUE;
        }
        break;

    case NeedResize:
        switch (eNewState)
        {
        case Ready:
        case NeedSetPosition:
        case NeedRecreate:
            return TRUE;
        }
        break;

    case NeedRecreate:
        switch (eNewState)
        {
        case NeedRecreate:
            // Happens when Present first detects change and SetPosition
            // is used to resize RTs to 0 x 0.
            return TRUE;
        }
        break;
    }

    return FALSE;
}
#endif





