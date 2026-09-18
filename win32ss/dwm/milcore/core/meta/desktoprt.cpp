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
//      CDesktopRenderTarget implementation
//
//      This is a multiple or meta Render Target for rendering on multiple
//      desktop devices.  It handles enumerating the devices and managing an
//      array of sub-targets.
//
//      If necessary it is able to hardware accelerate and fall back to software
//      RTs as appropriate.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

DeclareTag(tagMILRenderClearAfterPresent, "MIL", "Clear after present");
DeclareTag(tagMILTraceDesktopState, "MIL", "Trace MILRender desktop state");
DeclareTag(tagUseRgbRasterizer, "MIL-HW", "Use RGB rasterizer")

#if PERFMETER
// Set to true in the debugger to open the memory monitor
BOOL g_fDbgMemMonitor = FALSE;
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::Create
//
//  Synopsis:
//      This method supports the factory. It creates an initialized and
//      referenced RT instance.
//
//  Returns:
//      HRESULT success/failure including (but not limited to):
//          S_OK if successful
//          E_NOTIMPL if HWND is non-null and MilRTInitialization::FullScreen is
//              passed in dwFlags
//          WGXERR_DISPLAYSTATEINVALID if a mode change has
//              occured or there are 0 monitors on the system.
//
//------------------------------------------------------------------------------
HRESULT CDesktopRenderTarget::Create(
    __in_opt HWND hwnd,                                 // RT is initialized for this window
    __in_ecount(1) CDisplaySet const *pDisplaySet,            // Display Set
    MilWindowLayerType::Enum eWindowLayerType,          // Win32 layered window type
    MilRTInitialization::Flags dwFlags,                 // Initialization flags
    __deref_out_ecount(1) IMILRenderTargetHWND **ppIRT  // Output RT instance
    )
{
    HRESULT hr = S_OK;

    UINT cAdapters = 0;

    *ppIRT = NULL;

    // Check for the null render target
    if ((dwFlags & MilRTInitialization::TypeMask) == MilRTInitialization::Null)
    {
        *ppIRT = CDummyRenderTarget::sc_pDummyRT;
        goto Cleanup;
    }

    // NULL RT -  Make sure it is ok to treat these as exclusive flags below
    Assert(!((dwFlags & MilRTInitialization::SoftwareOnly) && (dwFlags & MilRTInitialization::HardwareOnly)));
    // Check is done in HrValidateInitializeCall
    Assert(hwnd);

    cAdapters = pDisplaySet->GetDisplayCount();
    if (cAdapters == 0)
    {
        // At current moment the system has no displays attached to desktop.
        // This case is not supported.
        IFC(WGXERR_DISPLAYSTATEINVALID);
    }

    //
    // check whether any adapters don't support Hw acceleration or D3D is not
    // available.
    //
    if (   pDisplaySet->IsNonLocalDisplayPresent()
        || !pDisplaySet->D3DObject())
    {
        // If possible, just revert to Sw.  This simply prevents trying Hw and
        // later falling back.
        if (!(dwFlags & MilRTInitialization::HardwareOnly))
        {
            dwFlags |= MilRTInitialization::SoftwareOnly;
        }
        else
        {
            // Otherwise propagate the error which prevented D3D usage
            IFC(pDisplaySet->GetD3DInitializationError());
            // D3D is available, but no hardware acceleration
            IFC(WGXERR_NO_HARDWARE_DEVICE);
        }
    }

    //
    // Allocate and initialize a desktop render target
    //

    CDesktopRenderTarget *pRT = NULL;

    pRT = new(cAdapters) CDesktopHWNDRenderTarget(cAdapters, pDisplaySet, eWindowLayerType);
    IFCOOM(pRT);
    pRT->AddRef();

    IFCSUB1(pRT->Init(
        hwnd,
        eWindowLayerType,
        dwFlags
        ));

    IFCSUB1(pRT->QueryInterface(IID_IMILRenderTargetHWND, (void **)ppIRT));

SubCleanup1:
    ReleaseInterfaceNoNULL(pRT);

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::SetSingleSubRT
//
//  Synopsis:
//      Set up meta data for a single sub-RT to handle entire desktop.  First
//      entry will have bounds for desktop.
//
//------------------------------------------------------------------------------

void CDesktopRenderTarget::SetSingleSubRT(
    )
{
    // The one RT needed has been acquired.  Set its device bounds to the
    // desktop and then change RT count to one, which will avoid any future
    // walking of sub-RTs that won't possibly get enabled.
    m_rgMetaData[0].rcVirtualDeviceBounds = m_pDisplaySet->GetBounds();

#if DBG_ANALYSIS
    // Paranoid assert that no other sub-RTs are valid.  Start
    // at 1 since 0 is the one valid sub-RT.
    for (UINT extras = 1; extras < m_cRT; extras++)
    {
        Assert(m_rgMetaData[extras].pInternalRT == NULL);
        Assert(m_rgMetaData[extras].rcVirtualDeviceBounds.IsEmpty());
        Assert(m_rgMetaData[extras].pInternalRTHWND == NULL);
        Assert(m_rgMetaData[extras].pHwDisplayRT == NULL);
        Assert(m_rgMetaData[extras].pSwHWNDRT == NULL);
    }
#endif

    const_cast<UINT &>(m_cRT) = 1;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::Init
//
//  Synopsis:
//      Initializes the desktop render target by retrieving device information
//      about each display, and then creating display-specific internal render
//      targets. Additionally this method detects hardware acceleration failure
//      and falls back to software per-adapter.
//
//      Subclasses are respsonsible for setting the window origin in this method
//
//  Returns:
//      HRESULT Success/Failure
//
//------------------------------------------------------------------------------
HRESULT CDesktopRenderTarget::Init(
    __in HWND hwnd,
    MilWindowLayerType::Enum eWindowLayerType,
    MilRTInitialization::Flags dwFlags
    )
{
    HRESULT hr = S_OK;

    Assert(m_cRT > 0);
    // The constructor of the subclass should initialize this to something else
    Assert(m_eState != Invalid);

    m_hwnd = hwnd;
    m_dwRTInitFlags = dwFlags;

    wpf::util::DpiAwarenessScope<HWND> dpiScope(m_hwnd);

#if DBG
    //
    // Assert MetaData is initialized
    //

    for (UINT i = 0; i < m_cRT; i++)
    {
        const MetaData &metadata = m_rgMetaData[i];

        // Initialize internal RT
        Assert(metadata.pInternalRT == NULL);
        Assert(metadata.fEnable == FALSE);
        Assert(metadata.ptInternalRTOffset.x == 0);
        Assert(metadata.ptInternalRTOffset.y == 0);
        Assert(metadata.rcLocalDeviceRenderBounds.IsEmpty());
        Assert(metadata.rcLocalDevicePresentBounds.IsEmpty());
        Assert(metadata.rcVirtualDeviceBounds.IsEmpty());
        Assert(metadata.rcLocalDeviceValidContentBounds.IsEmpty());

        // Initialize internal HWND RTs
        Assert(metadata.pInternalRTHWND == NULL);
        Assert(metadata.pHwDisplayRT == NULL);
        Assert(metadata.pSwHWNDRT == NULL);
    }
#endif

    Assert(m_cRT == DisplaySet()->GetDisplayCount());
    Assert(m_cRT > 0);

    //
    // When presenting via UpdateLayeredWindow, but the OS doesn't have partial
    // update support (XP SP2), then either:
    //  1) One Sw RT is needed Or
    //  2) All RTs must be Hw
    //

    bool fUse1SwRTOrHwOnly = false;
    bool fRetry1SwRT = false;

    if (   ((dwFlags & MilRTInitialization::PresentUsingMask) == MilRTInitialization::PresentUsingUpdateLayeredWindow)
        && !OSSupportsUpdateLayeredWindowIndirect())
    {
        fUse1SwRTOrHwOnly = true;

        // If not Hw Only, Sw Only, or NULL try only Hw first, but allow retry
        // with single Sw.
        if ((dwFlags & MilRTInitialization::TypeMask) == MilRTInitialization::Default)
        {
            dwFlags |= MilRTInitialization::HardwareOnly;
            fRetry1SwRT = true;
        }
    }

    bool const fLimitRenderToDisplayBounds =
           // Has unlimited option been requested?
           ((dwFlags & MilRTInitialization::DisableDisplayClipping) == 0)
           // Or can limited request not be reasonably fulfilled.
           //    When we are multimon, it takes extra resources to not clip and
           //    DWM does not have sufficient logic to track multiple
           //    overlapping targets with "device" (display) clipping disabled.
           //    The DWM assumes that, per pixel, valid window contents come
           //    from exactly one buffer.  So, with multimon and DWM enabled,
           //    display clipping will leave offscreen areas invalid. For V1
           //    this is acceptable.
        || (DisplaySet()->GetDisplayCount() > 1);

    // If we're not HW only, we may fall back to SW at a later time and not 
    // know. Assume we only have a single device when we're HW only, and 
    // then while creating the RTs in the loop below, we'll set it to false
    // if we learn that's not the case.
    UINT uCacheIndex = CMILResourceCache::InvalidToken;
    
    //
    // Create all of the render targets
    //
    for (UINT i = 0; i < m_cRT; i++)
    {
        MetaData &metadata = m_rgMetaData[i];

        CDisplay const *pDisplay = DisplaySet()->Display(i);
        Assert(pDisplay);

        if (fLimitRenderToDisplayBounds)
        {
            metadata.rcVirtualDeviceBounds = pDisplay->GetDisplayRect();
        }
        else
        {
            //
            // Setting the virtual device bounds to infinite lets this render
            // target be moved anywhere within reason without it going
            // offscreen.
            //
            //  There may be some accidental clipping if
            // someone programatically sets the window position so far away
            // that it is beyond our "infinite" values. This should be
            // acceptable as the scenario is a little out there (pardon the
            // pun).
            //
            metadata.rcVirtualDeviceBounds.SetInfinite();
        }

        // Is HW allowed?
        if (!(dwFlags & MilRTInitialization::SoftwareOnly))
        {
            D3DDEVTYPE d3dDeviceType = D3DDEVTYPE_HAL;

            if ((dwFlags & MilRTInitialization::UseRefRast))
            {
                d3dDeviceType = D3DDEVTYPE_REF;
            }
            else if ((dwFlags & MilRTInitialization::UseRgbRast) ||
                     IsTagEnabled(tagUseRgbRasterizer))
            {
                d3dDeviceType = D3DDEVTYPE_SW;
            }

            //
            // Create a hardware accelerated render target for this adapter.
            //
            // Multihead support aka GroupAdapter support is only available
            // for fullscreen RTs.  Since they will always be fullscreen on
            // each display the HW RTs handle this internally.  They assume
            // each display is targeted and automatically create the proper
            // D3D device to handle all displays in the group when the first
            // RT create request is made.
            //

            MIL_THR(CHwDisplayRenderTarget::Create(
                m_hwnd,
                eWindowLayerType,
                pDisplay,
                d3dDeviceType,
                dwFlags,
                &metadata.pHwDisplayRT
                ));
        }

        // Is SW allowed?
        if (!(dwFlags & MilRTInitialization::HardwareOnly))
        {
            //
            // If we failed to create the D3D render target or HW is disabled,
            // attempt to create a software render target.
            //

            if (!metadata.pHwDisplayRT)
            {
                MIL_THR(CSwRenderTargetHWND::Create(
                    m_hwnd,
                    eWindowLayerType,
                    pDisplay,
                    pDisplay->GetDisplayId(),
                    0,
                    0,
                    dwFlags,
                    &metadata.pSwHWNDRT
                    ));

                // Check for successful creation of Sw when one Sw RT is
                // requested
                if (   SUCCEEDED(hr)
                    && fUse1SwRTOrHwOnly)
                {
                    Assert((dwFlags & MilRTInitialization::TypeMask) == MilRTInitialization::SoftwareOnly);
                    Assert(i == 0);
                    // The one Sw RT needed has been acquired.  Call will set
                    // its device bounds to the desktop and then change RT
                    // count to one, which will trigger loop termination and
                    // avoid any future walking of sub-RTs that won't possibly
                    // get enabled.
                    SetSingleSubRT();
                }

                if (   SUCCEEDED(hr)
                    && !(dwFlags & MilRTInitialization::SoftwareOnly))
                {
                    EventWriteUnexpectedSoftwareFallback(UnexpectedSWFallback_NoHardwareAvailable);
                }
            }
        }

        if (FAILED(hr))
        {
            if (fRetry1SwRT)
            {
                //
                // Clean up prior RT creations in preparation for Sw only attempt
                //

                metadata.rcVirtualDeviceBounds.SetEmpty();

                while (i-- > 0)
                {
                    Assert(m_rgMetaData[i].pSwHWNDRT == NULL);
                    Assert(m_rgMetaData[i].pHwDisplayRT != NULL);
                    ReleaseInterface(m_rgMetaData[i].pHwDisplayRT);
                    m_rgMetaData[i].pInternalRTHWND = NULL;
                    ReleaseInterface(m_rgMetaData[i].pInternalRT);
                    m_rgMetaData[i].rcVirtualDeviceBounds.SetEmpty();
                }

                //
                // Restart loop - now attempting Sw only
                //

                dwFlags = (dwFlags & ~MilRTInitialization::TypeMask) | MilRTInitialization::SoftwareOnly;
                fRetry1SwRT = false;
                continue;
            }

            goto Cleanup;
        }

        if (metadata.pHwDisplayRT)
        {
            metadata.pInternalRTHWND = metadata.pHwDisplayRT;
            metadata.pInternalRT = metadata.pHwDisplayRT;

            UINT uCurrentCacheIndex = 
                metadata.pHwDisplayRT->GetRealizationCacheIndex();

            Assert(uCurrentCacheIndex != CMILResourceCache::SwRealizationCacheIndex);

            if (uCurrentCacheIndex != CMILResourceCache::InvalidToken)
            {
                if (uCacheIndex == CMILResourceCache::InvalidToken)
                {
                    // Found our first index
                    uCacheIndex = uCurrentCacheIndex;
                }
            }
        }
        else
        {
            metadata.pInternalRTHWND = metadata.pSwHWNDRT;
            metadata.pInternalRT = metadata.pSwHWNDRT;
        }
        metadata.pInternalRT->AddRef();
    }

    IFC(EditMetaData());

Cleanup:

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      CDesktopRenderTarget constructor.
//
//------------------------------------------------------------------------------

CDesktopRenderTarget::CDesktopRenderTarget(
    __out_ecount(cMaxRTs) MetaData *pMetaData,
    UINT cMaxRTs,
    __inout_ecount(1) CDisplaySet const *pDisplaySet
    )
    : CMetaRenderTarget(pMetaData, cMaxRTs, pDisplaySet)
{
    m_hwnd = NULL;
    m_rcSurfaceBounds = m_pDisplaySet->GetBounds();
    m_dwRTInitFlags = MilRTInitialization::Default;
    m_eState = Invalid;
    m_rcCurrentPosition.SetEmpty();
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CDesktopRenderTarget destructor.
//
//------------------------------------------------------------------------------

CDesktopRenderTarget::~CDesktopRenderTarget()
{
    for (UINT i = 0; i < m_cRT; i++)
    {
        // InternalRTHWND is not ref counted
        //ReleaseInterface(m_rgMetaData[i].pInternalRTHWND)

        if (m_rgMetaData[i].pHwDisplayRT)
        {
            m_rgMetaData[i].pHwDisplayRT->Release();
        }

        if (m_rgMetaData[i].pSwHWNDRT)
        {
            m_rgMetaData[i].pSwHWNDRT->Release();
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CDesktopRenderTarget QI helper routine
//
//  Arguments:
//      riid - input IID.
//      ppvObject - output interface pointer.
//
//------------------------------------------------------------------------------

STDMETHODIMP CDesktopRenderTarget::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IMILRenderTargetHWND)
        {
            *ppvObject = static_cast<IMILRenderTargetHWND*>(this);

            hr = S_OK;
        }
        else
        {
            hr = CMetaRenderTarget::HrFindInterface(riid, ppvObject);
        }
    }

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::Clear
//
//  Synopsis:
//      Assert state then delegate to base class
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDesktopRenderTarget::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    Assert(m_eState == Ready);

    return CMetaRenderTarget::Clear(pColor, pAliasedClip);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::Begin3D, End3D
//
//  Synopsis:
//      Assert state then delegate to base class
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDesktopRenderTarget::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    Assert(m_eState == Ready);

    return CMetaRenderTarget::Begin3D(rcBounds, AntiAliasMode, fUseZBuffer, rZ);
}

STDMETHODIMP
CDesktopRenderTarget::End3D(
    )
{
    Assert(m_eState == Ready);

    return CMetaRenderTarget::End3D();
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      Present - Cause the contents of the back-buffer to show up on the
//      various devices. Helper method used by subclasses Present method
//
//  Arguments:
//      prc - area of the back-buffer to use as the source
//      NULL is treated as the entire back-buffer for this parameter.
//
//------------------------------------------------------------------------------

STDMETHODIMP CDesktopRenderTarget::Present()
{
    HRESULT hr = S_OK;

    Assert(m_eState == Ready);

#if DBG
    if (g_fDbgMemMonitor)
    {
        // If this variable is set to a non-zero value in the debugger,
        // the memory monitor will open
        MtOpenMonitor();

        // but one instance only!
        g_fDbgMemMonitor = FALSE;
    }
#endif

    //
    // Compute portion of the window to Present and store to rcPresent
    //

    RECT rcPresent = {
        m_rcSurfaceBounds.left,
        m_rcSurfaceBounds.top,
        m_rcSurfaceBounds.right,
        m_rcSurfaceBounds.bottom
        };
 
#if DBG
    static bool fDbgClearToAqua = false;
#endif

    for (UINT i = 0; i < m_cRT; i++)
    {
        // Don't present if we haven't drawn anything on this RT yet.

        if (m_rgMetaData[i].fEnable)
        {
            RECT rcSubRTPresent = m_rgMetaData[i].rcLocalDevicePresentBounds;

            // Clip present rect to sub RT's present bounds to find sub
            // RT's present rect.

            if (IntersectRect(&rcSubRTPresent, &rcPresent, &rcSubRTPresent))
            {
                HRESULT hrPresent;

                // Translate the rectangle from meta render target space into
                // the internal render target's coordinate space
                OffsetRect(&rcSubRTPresent,
                    -m_rgMetaData[i].ptInternalRTOffset.x,
                    -m_rgMetaData[i].ptInternalRTOffset.y);

                if (m_fAccumulateValidBounds)
                {
                    Assert(m_rgMetaData[i].rcLocalDeviceValidContentBounds.DoesContain(rcSubRTPresent));
                }

                hrPresent = THR(m_rgMetaData[i].pInternalRTHWND->Present(
                    &rcSubRTPresent
                    ));

                if (FAILED(hrPresent))
                {
                    // If the display state has changed that is the error
                    // we want to return.
                    if (DangerousHasDisplayChanged())
                    {
                        hrPresent = THR(WGXERR_DISPLAYSTATEINVALID);
                    }

                    // Remember the most recent Present failure, but don't
                    // stop processing since we want to update as much
                    // of the desktop as we can.
                    hr = hrPresent;

                    if (hrPresent == WGXERR_DISPLAYSTATEINVALID)
                    {
                        // Remember display invalid
                        TransitionToState(NeedRecreate
                                          DBG_COMMA_PARAM("Present"));

                        // If the failure is display invalid then there is
                        // no point in updating as much as we can.
                        goto Cleanup;
                    }
                }
                else
                {
                    if (   (hrPresent == S_PRESENT_OCCLUDED)
                        && SUCCEEDED(hr))
                    {
                        // Promote return to S_PRESENT_OCCLUDED.
                        Assert(hr == S_OK || hr == S_PRESENT_OCCLUDED);
                        hr = S_PRESENT_OCCLUDED;
                    }
#if DBG
                    if (IsTagEnabled(tagMILRenderClearAfterPresent))
                    {
                        //
                        // Disable stepped rendering if enabled so as not
                        // to whack what was just presented.
                        //

                        BOOL fDbgStepRendering = IsTagEnabled(tagMILStepRendering);

                        //
                        // We could always use EnableTag( ,FALSE) and
                        // always call after Clear to restore, but that can
                        // mess with manual toggles of the tag during
                        // debugging; so, just toggle when it is actually
                        // needed.
                        //

                        if (fDbgStepRendering)
                        {
                            EnableTag(tagMILStepRendering, FALSE);
                        }

                        //
                        // Clear target
                        //

                        static CMilColorF const aqua(0.0,0.75,0.5,1.0);
                        static CMilColorF const orange(1.0,0.75,0.0,1.0);

                        IGNORE_HR(m_rgMetaData[i].pInternalRT->Clear(
                            fDbgClearToAqua ? &aqua: &orange
                            ));

                        //
                        // Restore stepped rendering tag
                        //

                        if (fDbgStepRendering)
                        {
                            EnableTag(tagMILStepRendering, TRUE);
                        }
                    }
#endif DBG
                }
            }
        }
    }

#if DBG
    // Toggle clear color
    fDbgClearToAqua = !fDbgClearToAqua;
#endif

Cleanup:

    // Note: There didn't appear to be a compelling reason to check for
    //       NeedRecreate and avoid ClearDirtyList while investigating an 
    //       an Assert in CHwHWNDRenderTarget::UpdateFlippingChain on mode changes.
    //       However the bug was fixed by allowing to
    //       SetPosition to be called even when the dirty list hasn't been
    //       cleared, which is fine since there is logic to make sure the right
    //       updates are made after a SetPosition call.
    if (g_pMediaControl)
    {
        g_pMediaControl->UpdatePerFrameCounters();
    }

    if (m_eState != NeedRecreate)
    {
        // At this point, independent of most errors and whether or not the
        // specified present rectangle intersect a given RT the dirty list
        // on all of the RTs should now be cleared.
        for (UINT i = 0; i < m_cRT; i++)
        {
            if (m_rgMetaData[i].fEnable)
            {
                IGNORE_HR(m_rgMetaData[i].pInternalRTHWND->ClearInvalidatedRects());
            }
        }
    }

    RRETURN1(hr, S_PRESENT_OCCLUDED);
}

//=============================================================================
//
// See comment on CPreComputeContext::ScrollableAreaHandling for details
//
//=============================================================================

STDMETHODIMP CDesktopRenderTarget::ScrollBlt(
    __in_ecount(1) const RECT *prcSource,
    __in_ecount(1) const RECT *prcDest
    )
{
    HRESULT hr = S_OK;

    if (   ((prcSource->right - prcSource->left) != (prcDest->right - prcDest->left))
        || ((prcSource->bottom - prcSource->top) != (prcDest->bottom - prcDest->top)))
    {
        FreAssert(false);//, "Attempted to call CDesktopRenderTarget::ScrollBlt with different sized source and destination rectangles");
    }

    //bool fScrolled = false;

    for (UINT i = 0; i < m_cRT; i++)
    {
        if (m_rgMetaData[i].fEnable)
        {
            RECT rcSubRTPresent = m_rgMetaData[i].rcLocalDevicePresentBounds;

            // Clip scroll rect to sub RT's present bounds to find sub
            // RT's scroll rect.

            if (IntersectRect(&rcSubRTPresent, prcSource, &rcSubRTPresent))
            {
                /*
                if (fScrolled)
                {
                    // We're scrolling on another RT after we've already scrolled one
                    // This isn't supported.
                    FreAssert(false, "Attempted to call CDesktopRenderTarget::ScrollBlt more than once... ");
                }
                fScrolled = true;
                */

                RECT source = *prcSource;
                RECT dest = *prcDest;

                // Translate the rectangle from meta render target space into
                // the internal render target's coordinate space
                OffsetRect(&source,
                    -m_rgMetaData[i].ptInternalRTOffset.x,
                    -m_rgMetaData[i].ptInternalRTOffset.y);

                OffsetRect(&dest, 
                    -m_rgMetaData[i].ptInternalRTOffset.x,
                    -m_rgMetaData[i].ptInternalRTOffset.y);

                HRESULT hrScroll = S_OK;
                
                hrScroll = THR(m_rgMetaData[i].pInternalRTHWND->ScrollBlt(
                    &source,
                    &dest
                    ));
    
                if (FAILED(hrScroll))
                {
                    // If the display state has changed that is the error
                    // we want to return.
                    if (DangerousHasDisplayChanged())
                    {
                        hrScroll = THR(WGXERR_DISPLAYSTATEINVALID);
                    }

                    // Remember the most recent ScrollBlt failure, but don't
                    // stop processing since we want to update as much
                    // of the desktop as we can.
                    hr = hrScroll;

                    if (hrScroll == WGXERR_DISPLAYSTATEINVALID)
                    {
                        // Remember display invalid
                        TransitionToState(NeedRecreate
                                          DBG_COMMA_PARAM("ScrollBlt"));

                        // If the failure is display invalid then there is
                        // no point in updating as much as we can.
                        goto Cleanup;
                    }
                }
            }
        }
    }
Cleanup:
    RRETURN(hr);
}
    


//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Invalidate: invalidates internal render targets
//
//------------------------------------------------------------------------------

STDMETHODIMP CDesktopRenderTarget::Invalidate(
    __in_ecount_opt(1) MilRectF const *prc
    )
{
    HRESULT hr = S_OK;

    CMILSurfaceRect rcRTSurfaceSpace;

    Assert(m_eState == Ready);

    if (prc)
    {
        if (!IntersectAliasedBoundsRectFWithSurfaceRect(*prc, m_rcSurfaceBounds, &rcRTSurfaceSpace))
        {
            goto Cleanup;
        }
    }
    else
    {
        rcRTSurfaceSpace = m_rcSurfaceBounds;
    }

    TraceTag((tagMILTraceDesktopState,
              "0x%p Desktop::Invalidate: (%d, %d) - (%d, %d)",
              this,
              rcRTSurfaceSpace.left,
              rcRTSurfaceSpace.top,
              rcRTSurfaceSpace.right,
              rcRTSurfaceSpace.bottom
              ));

    for (UINT i = 0; i < m_cRT; i++)
    {
        MetaData const &oDevData = m_rgMetaData[i];

        if (oDevData.fEnable)
        {
            //
            // Invalid region starts out as the local present bounds.
            //
            CMILSurfaceRect rcInvalid = oDevData.rcLocalDevicePresentBounds;

            //
            // Intersect the invalid surface region with the present bounds.
            //
            //
            if (rcInvalid.Intersect(rcRTSurfaceSpace))
            {
                //
                // Translate the rectangle from meta render target space into
                // the internal render target's coordinate space.
                //
                rcInvalid.Offset(
                    -oDevData.ptInternalRTOffset.x,
                    -oDevData.ptInternalRTOffset.y
                    );
            }
            else
            {
                //
                // By passing an empty region we let the rendertarget know
                // that invalidation is taking place even though it doesn't
                // have anything to invalidate.
                //
                Assert(rcInvalid.IsEmpty());
            }

            if (m_fAccumulateValidBounds)
            {
                //Assert(oDevData.rcLocalDeviceValidContentBounds.DoesContain(rcInvalid));
            }

            IFC(oDevData.pInternalRTHWND->InvalidateRect(&rcInvalid));
        }
    }

Cleanup:
    RRETURN(hr);
}


#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::DrawBitmap
//
//  Synopsis:
//      Assert state then delegate to base class
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDesktopRenderTarget::DrawBitmap(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    Assert(m_eState == Ready);

    return CMetaRenderTarget::DrawBitmap(
            pContextState,
            pIBitmap,
            pIEffect
            );
}
#endif

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::DrawMesh3D
//
//  Synopsis:
//      Assert state then delegate to base class
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDesktopRenderTarget::DrawMesh3D(
    __inout_ecount(1) CContextState* pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) CMILMesh3D* pMesh3D,
    __inout_ecount_opt(1) CMILShader *pShader,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    Assert(m_eState == Ready);

    return CMetaRenderTarget::DrawMesh3D(
            pContextState,
            pBrushContext,
            pMesh3D,
            pShader,
            pIEffect
            );
}
#endif

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::DrawPath
//
//  Synopsis:
//      Assert state then delegate to base class
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDesktopRenderTarget::DrawPath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) IShapeData *pShape,
    __inout_ecount_opt(1) CPlainPen *pPen,
    __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
    __inout_ecount_opt(1) CBrushRealizer *pFillBrush
    )
{
    Assert(m_eState == Ready);

    return CMetaRenderTarget::DrawPath(
            pContextState,
            pBrushContext,
            pShape,
            pPen,
            pStrokeBrush,
            pFillBrush
            );
}
#endif


#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::DrawGlyphs
//
//  Synopsis:
//      Assert state then delegate to base class
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDesktopRenderTarget::DrawGlyphs(
    __inout_ecount(1) DrawGlyphsParameters &pars
    )
{
    Assert(m_eState == Ready);

    return CMetaRenderTarget::DrawGlyphs(pars);
}
#endif

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::DrawVideo
//
//  Synopsis:
//      Assert state then delegate to base class
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDesktopRenderTarget::DrawVideo(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __inout_ecount(1) IWGXBitmapSource *pBitmapSource,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    Assert(m_eState == Ready);

    return CMetaRenderTarget::DrawVideo(
            pContextState,
            pSurfaceRenderer,
            pBitmapSource,
            pIEffect
            );
}
#endif


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::CreateRenderTargetBitmap
//
//  Synopsis:
//      If rendering is enabled, delegate to base class.  Otherwise return a
//      dummy RT.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDesktopRenderTarget::CreateRenderTargetBitmap(
    UINT width,
    UINT height,
    IntermediateRTUsage usageInfo,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
    __in_opt DynArray<bool> const *pActiveDisplays
    )
{
    HRESULT hr = S_OK;

    // The width and height are converted to floats when clipping,
    // make sure we don't expect values TOO big as input.
    if (width > MAX_INT_TO_FLOAT || height > MAX_INT_TO_FLOAT)
    {
        IFC(WGXERR_UNSUPPORTEDTEXTURESIZE);
    }

    Assert(m_eState == Ready);

    // If we're creating a meta RTB on specific displays, go straight
    // to the base class.  Otherwise, delegate to the base class if
    // we have any displays enabled.
    for (UINT i = 0; i < m_cRT; i++)
    {
        if (m_rgMetaData[i].fEnable || pActiveDisplays != NULL)
        {
            hr = THR(CMetaRenderTarget::CreateRenderTargetBitmap(
                width,
                height,
                usageInfo,
                dwFlags,
                ppIRenderTargetBitmap,
                pActiveDisplays
                ));
            goto Cleanup;
        }
    }

    {
        // Techinically, we never want to get here as our caller should not
        //  call unless some display is enabled.

        //
        // Return a dummy RT that does nothing, but consume
        // calls to it and return what little dummy values it has too.
        //

        *ppIRenderTargetBitmap = CDummyRenderTarget::sc_pDummyRT;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::TransitionToState
//
//  Synopsis:
//      Set new state (Validate in debug)
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void
CDesktopRenderTarget::TransitionToState(
    enum State eNewState
#if DBG
    , const char *pszMethod
#endif
    )
{
    #if DBG
    Assert(DbgIsValidTransition(eNewState));
    #endif

    if (IsTagEnabled(tagMILTraceDesktopState))
    {
        static const char * const rgStateName[] = {
            "Invalid",
            "Ready",
            "NeedSetPosition",
            "NeedResize",
            "NeedRecreate",
        };

        C_ASSERT(ARRAY_SIZE(rgStateName) == NeedRecreate + 1);

        #if DBG
        TraceTag((tagMILTraceDesktopState,
                  "0x%p Desktop::%s: %s to %s",
                  this,
                  pszMethod,
                  rgStateName[m_eState],
                  rgStateName[eNewState]
                  ));
        #endif
    }

    m_eState = eNewState;

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::GetBounds
//
//  Synopsis:
//      Gets the effective bounds of the render target
//
//------------------------------------------------------------------------------
STDMETHODIMP_(VOID)
CDesktopRenderTarget::GetBounds(
    __out_ecount(1) MilRectF * const pBounds
    )
{
    pBounds->left   = static_cast<FLOAT>(m_rcSurfaceBounds.left);
    pBounds->top    = static_cast<FLOAT>(m_rcSurfaceBounds.top);
    pBounds->right  = static_cast<FLOAT>(m_rcSurfaceBounds.right);
    pBounds->bottom = static_cast<FLOAT>(m_rcSurfaceBounds.bottom);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::WaitForVBlank
//
//  Synopsis:
//      Waits until the associated HW display is in vertical blank
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDesktopRenderTarget::WaitForVBlank()
{
    BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
        WGXERR_NO_HARDWARE_DEVICE
    END_MILINSTRUMENTATION_HRESULT_LIST

    HRESULT hr = WGXERR_NO_HARDWARE_DEVICE;

    //  Waiting on VBlank on the first enabled adapter
    // If the adapter does not match the adapter used to get
    // frame rate scheduling will likely fail in strange ways.
    UINT i = 0;
    while( i < m_cRT && !m_rgMetaData[i].fEnable)
    {
        i++;
    }
    if (i < m_cRT)
    {
        hr = m_rgMetaData[0].pInternalRTHWND->WaitForVBlank();
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::AdvanceFrame
//
//  Synopsis:
//      Advances frame count
//
//------------------------------------------------------------------------------
STDMETHODIMP_(VOID)
CDesktopRenderTarget::AdvanceFrame(
    UINT uFrameNumber
    )
{
    for (UINT i = 0; i < m_cRT; ++i)
    {
        m_rgMetaData[i].pInternalRTHWND->AdvanceFrame(uFrameNumber);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDesktopRenderTarget::GetNumQueuedPresents
//
//  Synopsis:
//      Forward the call to the CMetaRenderTarget member.
//
//------------------------------------------------------------------------------
HRESULT
CDesktopRenderTarget::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    RRETURN(CMetaRenderTarget::GetNumQueuedPresents(
        puNumQueuedPresents
        ));
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if the current HWND straddles more than 1 monitor. If it does,
//      we currently can't accelerate scrolling due to known bug. 
// Details: 
// If the app is straddling an edge of the screen which does not align with another 
// monitor(eg the right side of the right hand monitor), we do not present the content 
// that is offscreen.This means that the content in the DWM thumbnailand flip3d is 
// incorrect.
//
// In singlemon, we appear to present the whole rect regardless.Somewhere we 
// make an incorrect optimization for the multimon case.If the app straddles 2 monitors,
// we can present the partial rects to each monitor, and DWM will splice them together
// for the thumbnail.We appear to be taking excessive advantage of this optimization.
//
//------------------------------------------------------------------------------

STDMETHODIMP 
CDesktopRenderTarget::CanAccelerateScroll(
    __out_ecount(1) bool *pfCanAccelerateScroll
    )
{
    HRESULT hr = S_OK;
	*pfCanAccelerateScroll = true;

    DynArray<bool> rgActiveDisplays;
    
    // Now check if this Hwnd extends onto multiple physical displays. If so, we can't scroll
    // because that would involve BLTing from one display to another, which we don't support 
    // currently.              

    const CDisplaySet *pDisplaySet = NULL;
    g_DisplayManager.GetCurrentDisplaySet(&pDisplaySet);

    UINT displayCount = pDisplaySet->GetDisplayCount();

    ReleaseInterface(pDisplaySet);                

    IFC(rgActiveDisplays.AddAndSet(displayCount, false));
    IFC(ReadEnabledDisplays(&rgActiveDisplays));


    bool fFoundIntersection = false;
    for (UINT i = 0; i < displayCount; i++)
    {
        if (rgActiveDisplays[i])
        {
            MilRectL intersection;
            GetIntersectionWithDisplay(i, intersection);
            if (!IsRectEmpty(intersection))
            {
                if (fFoundIntersection)
                {
                    // This HWND is already on another display, and it intersects this display,
                    // meaning it is straddling multiple windows. Disable scrolling.
                    *pfCanAccelerateScroll = false;
                    break;
                }
                else
                {
                    fFoundIntersection = true;
                }
            }
        }
    }
    
Cleanup:
    RRETURN(hr);
}




