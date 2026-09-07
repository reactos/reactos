// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Module Name:
//
//    hwndtarget.cpp
//
//---------------------------------------------------------------------------------

#include "precomp.hpp"
#include <shellscalingapi.h>

MtDefine(CSlaveHWndRenderTarget, MILRender, "CSlaveHWndRenderTarget");

//------------------------------------------------------------------
// CSlaveHwndRenderTarget::ctor
// 
// Note:
//      It is important that m_disableCookie start at zero to 
//      allow an initial UpdateWindowSettings(enable) command to 
//      enable the render target without a preceeding 
//      UpdateWindowSettings(disable) command.
//
//      This is a good place to force sw fallback by initializing
//      m_fSoftwareFallback = true
//  
//      Everything else not initialized here is initialized to zero
//      by DECLARE_METERHEAP_CLEAR
//
//      DpiProvider is a CDelegatingUnknown, which requires the correct
//      version of IUnknown to delegate AddRef/Release/QueryInterface
//      calls to. This is accomplished by passing the CMILCOMBase* 
//      version of the IUnknown to it. 
//------------------------------------------------------------------

CSlaveHWndRenderTarget::CSlaveHWndRenderTarget(CComposition *pComposition) : 
    CRenderTarget(pComposition), 
    m_DisplayDevicesAvailabilityChangedWindowMessage(RegisterWindowMessage(L"DisplayDevicesAvailabilityChanged")), 
    m_LastKnownDisplayDevicesAvailabilityChangedWParam(-1), 
    m_WindowLayerType(MilWindowLayerType::NotLayered), 
    m_WindowTransparency(MilTransparency::Opaque), 
    m_hWnd(nullptr),                                // Will be changed during ProcessCreate
    m_UCETargetFlags(MilRTInitialization::Null),    // Will be changed during ProcessCreate
    m_RenderTargetFlags(MilRTInitialization::Null), // Will be changed during ProcessCreate
    m_fNeedsFullRender(true),
    m_fNeedsPresent(false),
    m_fRenderingEnabled(true),
    m_fTransparencyDirty(true),
    m_disableCookie(0),
    DpiProvider(static_cast<CMILCOMBase*>(this))    // There is more than one IUnknown in the inheritance chain, select the primary one.
{
    m_rcWindow.SetEmpty();
    m_clearColor.a = 1.0f;
    Assert(m_pRenderTarget == NULL);
}

//------------------------------------------------------------------
// CSlaveHwndRenderTarget::dtor (virtual)
//------------------------------------------------------------------

CSlaveHWndRenderTarget::~CSlaveHWndRenderTarget()
{
    ReleaseResources();
}

//+-----------------------------------------------------------------------
//
//  Member: CSlaveHWndRenderTarget::Render
//
//  Synopsis:  Renders, but does not not present the target
//
//  Returns:  S_OK if succeeds
//
//------------------------------------------------------------------------
HRESULT
CSlaveHWndRenderTarget::Render(
    __out_ecount(1) bool *pfNeedsPresent
    )
{
    HRESULT hr = S_OK;

    IFC(EnsureRenderTargetInternal());

    CDrawingContext *pDrawingContext = NULL;
    IFC(GetDrawingContext(&pDrawingContext));
    
    if (m_pRenderTarget
        && m_fRenderingEnabled
        && !m_fNoScreenAccess  // If we don't have screen access we don't need to render. 
        && !m_fIsZombie)       // The hwnd target is going away and therefore we don't need to render.
    {
        UINT uNumInvalidTargetRegions = 0;
        MilRectF const *rgInvalidTargetRegions = NULL;

        UINT renderedRegionCount = 0;

        if (m_hWnd)
        {
            //
            // Get list of areas of target that don't have valid contents. 
            // Later add the list to the dirty areas.
            // This happens when SetPosition() has been called on the render target
            // to change the window size.
            //
            bool fWholeTargetInvalid = false;
            
            IFC(m_pRenderTarget->GetInvalidRegions(
                &rgInvalidTargetRegions,
                &uNumInvalidTargetRegions,
                &fWholeTargetInvalid
                ));

            m_fNeedsFullRender |= fWholeTargetInvalid;
        }

        //
        // If we are not retaining contents in the swap-chain, then we must
        // present the entire scene each frame.
        //

        if ((m_UCETargetFlags & MilRTInitialization::PresentRetainContents) == 0)
        {
            m_fNeedsFullRender = true;
        }

        if (m_pRoot == NULL)
        {
            if (m_fNeedsFullRender)
            {
                //
                // If we need a full-render we clear always the back-buffer to
                // ensure that we don't render any junk. The flag is set when we
                // re-created the render target, resized the target, etc.
                //
                IFC(m_pRenderTarget->Clear(&m_clearColor));
                IFC(InvalidateInternal());

                m_fNeedsPresent = true;
            }
        }
        else
        {
            //
            // If we have a composition context we render now the composition tree.
            //
            BOOL fNeedsFullPresent = FALSE;

            //
            // Can't accelerate scroll if:
            // - We're rendering the whole window anyway
            // - We have invalid regions (due to resize or window position changes)
            // - We have a layered window of any type (I don't know why this doesn't work if
            //   the layered window is opaque, I just tried it and artifacts appeared everywhere,
            //   and since it's not a goal to make it work, I haven't debugged it).
            //
            bool fCanAccelerateScroll =    !m_fNeedsFullRender 
                                        && (uNumInvalidTargetRegions == 0)
                                        && (m_WindowLayerType == MilWindowLayerType::NotLayered);

            //
            // Get the effective bounds of the render target
            //
            CMilRectF renderTargetBounds;
            m_pRenderTarget->GetBounds(&renderTargetBounds);

            // If our bounds are empty we won't render anything.  This will happen with hardware rendering
            // when the window is minimized - we'll switch to a dummy RT with no bounds.
            if (!renderTargetBounds.IsEmpty())
            {
                //
                // Render the composition tree
                //

                IFC(pDrawingContext->BeginFrame(
                    m_pRenderTarget
                    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::PageInPixels)
                    ));

                IFC(pDrawingContext->Render(
                    m_pRoot,
                    m_pRenderTarget,
                    &m_clearColor,
                    renderTargetBounds,
                    m_fNeedsFullRender,
                    uNumInvalidTargetRegions,
                    rgInvalidTargetRegions,
                    fCanAccelerateScroll,
                    &fNeedsFullPresent                    
                    ));

                pDrawingContext->EndFrame();

                //
                // Get the dirty region and invalidate them on the render target.
                //
                if (!m_fNeedsFullRender && !fNeedsFullPresent)
                {
                    const CMilRectF *renderedRegions = pDrawingContext->GetRenderedRegions(&renderedRegionCount);
                    for (UINT i = 0; i < renderedRegionCount; i++)
                    {
                        Assert(!renderedRegions[i].IsEmpty());
                        IFC(InvalidateInternal(&renderedRegions[i]));
                    }
                }
                else
                {
                    IFC(InvalidateInternal());
                }
            }
        }

        //
        // If we have invalid regions we need to let the device know
        //
        // If we have no invalid regions for this frame, we will
        // not present and will keep the previous frames invalid regions
        // because we did not copy them in CDrawingContext::Render. We 
        // will copy them on the next frame that actually does get presented.
        //
        if (m_fHasInvalidRegions)
        {   
            IFC(SendInvalidRegions());
        }

    }

    if (!m_fNeedsPresent)
    {
        m_fNeedsFullRender = false;
    }

    *pfNeedsPresent = m_fNeedsPresent;

Cleanup:
    if (FAILED(hr))
    {
        m_fNeedsPresent = false;
    }

    // HandleWindowErrors also handles some success codes (e.g. S_PRESENT_OCCLUDED). Hence calling it
    // outside of the if-FAILED block.
    hr = HandleWindowErrors(hr);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member: CSlaveHWndRenderTarget::Present
//
//  Synopsis:  Presents previously rendered but unpresented contents if any
//
//  Returns:  S_OK if succeeds
//
//------------------------------------------------------------------------
HRESULT
CSlaveHWndRenderTarget::Present()
{
    HRESULT hr = S_OK;
    if (m_fNeedsPresent)
    {
        m_fNeedsPresent = false;
        
        if (   m_pRenderTarget
            && m_fRenderingEnabled
            && !m_fNoScreenAccess  // If we don't have screen access; so no need to present.
            && !m_fIsZombie)
        {
            
            IFC(m_pRenderTarget->Present());
            if (g_uMilPerfInstrumentationFlags & MilPerfInstrumentation_SignalPresent)
            {
                // Special perf measurement instrumentation:
                // post the window message to notify that the frame has been presented.
                // This message can be intercepted by application
                // that will generate new frame in response
                // (TextRender.exe and RtPerf2.exe behave this way).
                // This allow to measure overall throughput that can
                // include Visual Tree and transportation expenses.
                ::PostMessage(m_hWnd, WM_USER, 123, 456);
            }

        }
        m_fNeedsFullRender = false;
    }

Cleanup:

    // HandleWindowErrors also handles some success codes (e.g. S_PRESENT_OCCLUDED). Hence calling it
    // outside of the if-FAILED block.
    hr = HandleWindowErrors(hr);
    
    RRETURN1(hr, S_PRESENT_OCCLUDED);
}

//+-----------------------------------------------------------------------
//
//  Member: CSlaveHWndRenderTarget::PostDisplayAvailabilityMessage
//
//  Synopsis:  Sends m_DisplayDevicesAvailabilityChangedWindowMessage to the 
//             HWND. 
//
//             wParam for this message is set to 0 if no displays are available, 
//             and it is set to 1 when > 0 displays are available. 
//
//             lParam is unused.
//  Parameters:
//             displayCount: Indicates the number of valid displays available
//                           in the display set.
//
//  Returns:  TRUE if successful, FALSE otherwise
//
//------------------------------------------------------------------------
BOOL CSlaveHWndRenderTarget::PostDisplayAvailabilityMessage(int displayCount)
{
    m_LastKnownDisplayDevicesAvailabilityChangedWParam = (displayCount > 0) ? 1 : 0;
    return 
        PostMessage(
            m_hWnd, 
            m_DisplayDevicesAvailabilityChangedWindowMessage, 
            m_LastKnownDisplayDevicesAvailabilityChangedWParam, 
            0);
}

//+-----------------------------------------------------------------------
//
//  Member: CSlaveHWndRenderTarget::NotifyInvalidDisplaySet
//
//  Synopsis:  Invalidates this particulare HWND render target if it isn't 
//             a full-screen render target (typically used by the DWM) 
//  Parameters:
//                  invalid:  When true, indicates that the new display set 
//                            obtained after the recent mode-change is invalid. 
//           oldDisplayCount: The number of valid displays available before this
//                            change.
//              displayCount: Indicates the number of valid displays available
//                            in the new display set.
//
//  Returns:  Currently always succeeds.
//
//------------------------------------------------------------------------
override
HRESULT
CSlaveHWndRenderTarget::NotifyDisplaySetChange(bool invalid , int oldDisplayCount, int displayCount)
{
    if (m_hWnd != nullptr && IsWindow(m_hWnd))
    {
        if (invalid)
        {
            ReleaseResources();
            InvalidateWindow();
        }

        // Let the UI thread know that the display-set has changed in a meaningful way.
        // This helps in ensuring that the UI thread does not repeatedly attempt to process
        // WM_PAINT or invalidate itself when the render-thread is unable to render.
        if ((invalid || (displayCount == 0)) && 
            (oldDisplayCount != displayCount))
        {
            PostDisplayAvailabilityMessage(displayCount); // ignore return value
        }
    }

    return S_OK;
}

//------------------------------------------------------------------
// CSlaveHWndRenderTarget::GetIntersectionWithDisplay
//------------------------------------------------------------------

void CSlaveHWndRenderTarget::GetIntersectionWithDisplay(
    __in UINT iDisplay,
    __out_ecount(1) CMILSurfaceRect &rcIntersection
    )
{
    if (m_pRenderTarget)
    {
        m_pRenderTarget->GetIntersectionWithDisplay(iDisplay, rcIntersection);
    }
    else
    {
        rcIntersection = rcIntersection.sc_rcEmpty;
    }
}


//------------------------------------------------------------------
// CSlaveHWndRenderTarget::ProcessCreate
//------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::ProcessCreate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_HWNDTARGET_CREATE* pCmd
    )
{
    HRESULT hr = S_OK;

    RtlCopyMemory(&m_clearColor, &pCmd->clearColor, sizeof(m_clearColor));
    
    m_hWnd = (HWND)pCmd->hwnd;
    
    DpiProvider::UpdateDpi(DpiScale(pCmd->DpiX, pCmd->DpiY));
    DpiProvider::SetDpiAwarenessContext(pCmd->DpiAwarenessContext);

    MilRTInitialization::Flags UCETargetFlags = pCmd->flags;

    IFC(UpdateRenderTargetFlags(UCETargetFlags));
    m_UCETargetFlags = UCETargetFlags;

Cleanup:
    RRETURN(hr);
}

HRESULT 
CSlaveHWndRenderTarget::ProcessSuppressLayered(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_HWNDTARGET_SUPPRESSLAYERED* pCmd
    )
{
    RRETURN(S_OK);
}

HRESULT 
CSlaveHWndRenderTarget::ProcessDpiChanged(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_HWNDTARGET_DPICHANGED* pCmd
    )
{
    DpiProvider::UpdateDpi(DpiScale(pCmd->DpiX, pCmd->DpiY));
    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CSlaveHWndRenderTarget::CalculateWindowRect
//
//  Synopsis:
//      A helper method that obtains the window rect. 
//
//----------------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::CalculateWindowRect()
{
    HRESULT hr = S_OK;
    wpf::util::DpiAwarenessScope<HWND> dpiScope(m_hWnd);


    RECT rcClient;
    IFCW32(GetClientRect(m_hWnd, &rcClient));


    POINT ptClientTopLeft;
    ptClientTopLeft.x = rcClient.left;
    ptClientTopLeft.y = rcClient.top;
    IFCW32(ClientToScreen(m_hWnd, &ptClientTopLeft));

    POINT ptClientBottomRight;
    ptClientBottomRight.x = rcClient.right;
    ptClientBottomRight.y = rcClient.bottom;
    IFCW32(ClientToScreen(m_hWnd, &ptClientBottomRight));

    // RTL windows will cause the right edge to be on the left, so
    // we compensate for that.
    if (ptClientBottomRight.x >= ptClientTopLeft.x)
    {
        m_rcWindow.left = static_cast<float>(ptClientTopLeft.x);
        m_rcWindow.right = static_cast<float>(ptClientBottomRight.x);
    }
    else
    {
        m_rcWindow.left = static_cast<float>(ptClientBottomRight.x);
        m_rcWindow.right = static_cast<float>(ptClientTopLeft.x);
    }

    m_rcWindow.top = static_cast<float>(ptClientTopLeft.y);
    m_rcWindow.bottom = static_cast<float>(ptClientBottomRight.y);

Cleanup:
    if (FAILED(hr))
    {
        // 
        // Since there appear to be random multiple errors being 
        // returned here, and we can't rely on WIN32 error codes being
        // accurate, for the time being we are dying silently on the ones
        // below by returning a WGXERR_GENERIC_IGNORE.
        //
        TraceTag((tagMILWarning, 
                     "CSlaveHWndRenderTarget::GetWindowRect: Failure occurred, converting to WGXERR_GENERIC_IGNORE"));        

        MIL_THR(WGXERR_GENERIC_IGNORE);
    }

    RRETURN(hr);    
}


//------------------------------------------------------------------
// CSlaveHWndRenderTarget::ProcessUpdateWindowSettings
//------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::ProcessUpdateWindowSettings(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable, 
    __in_ecount(1) const MILCMD_TARGET_UPDATEWINDOWSETTINGS* pCmd
    )
{
    HRESULT hr = S_OK;

    MilWindowLayerType::Enum eWindowLayerType = pCmd->windowLayerType;
    MilTransparency::Flags flWindowTransparency = pCmd->transparencyMode;

    if (   (static_cast<UINT>(eWindowLayerType) > MilWindowLayerType::ApplicationManagedLayer)
        || ((flWindowTransparency & ~(MilTransparency::ConstantAlpha |
                                     MilTransparency::PerPixelAlpha |
                                     MilTransparency::ColorKey)) != 0)
       )
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    if (pCmd->isChild)
    {
        //
        // Child windows do not receive enough window messages to drive a
        // push model.  For example, child windows are not notified when
        // their parent is moved, even though it has the effect of moving
        // the child.  Since we have a strong requirement to know where
        // the child window is, we cannot rely on receiving an
        // UpdateWindowSettings for it, and we must query for any
        // information we need each time we render.
        //

        // So for child windows we always assume we are enabled.
        m_fRenderingEnabled = true;
    }
    else
    {
        //
        // Every UpdateWindowSettings command that disables the render target is
        // assigned a new cookie.  Every UpdateWindowSettings command that enables
        // the render target uses the most recent cookie.  This allows the
        // compositor to ignore UpdateWindowSettings(enable) commands that come
        // out of order due to us disabling out-of-band and enabling in-band.
        //
        if (pCmd->renderingEnabled != 0)
        {
            if (m_disableCookie == pCmd->disableCookie)
            {
                m_fRenderingEnabled = true;
            }
            else
            {
                // This command to enable the render target does not match the
                // last disable command, so we should simply ignore this
                // command.
                goto Cleanup;
            }
        }
        else
        {
            // Remember the lastest cookie.
            m_disableCookie = pCmd->disableCookie;
        
            m_fRenderingEnabled = false;
        }
    }

    if ((!pCmd->isChild) || (eWindowLayerType == MilWindowLayerType::ApplicationManagedLayer))
    {
        //
        // Fix up transparency setting per what layer type allows
        //

        if (eWindowLayerType == MilWindowLayerType::NotLayered)
        {
            // Non-layered windows must always be opaque
            flWindowTransparency = MilTransparency::Opaque;
        }
        else if (eWindowLayerType == MilWindowLayerType::SystemManagedLayer)
        {
            // System managed layered windows don't support per-pixel alpha
            flWindowTransparency &= ~MilTransparency::PerPixelAlpha;
        }

        //
        // Update the transparency settings
        //

        if (m_WindowTransparency != flWindowTransparency)
        {
            m_WindowTransparency = flWindowTransparency;
            m_fTransparencyDirty = true;
        }

        if (m_WindowLayerType != eWindowLayerType)
        {
            //
            // The render target needs to be recreated upon layered window state change.
            //

            m_WindowLayerType = eWindowLayerType;
            ReleaseResources();

            m_fTransparencyDirty = true;
        }

        if (m_constantAlpha != pCmd->constantAlpha)
        {
            m_constantAlpha = pCmd->constantAlpha;
            m_fTransparencyDirty = true;
        }

        if (   m_colorKey.r != pCmd->colorKey.r
            || m_colorKey.g != pCmd->colorKey.g
            || m_colorKey.b != pCmd->colorKey.b
            || m_colorKey.a != pCmd->colorKey.a)
        {
            m_colorKey = pCmd->colorKey;
            m_fTransparencyDirty = true;
        }

        //
        // Update the window rectangle.
        //

        m_rcWindow.left = static_cast<float>(pCmd->windowRect.left);
        m_rcWindow.top = static_cast<float>(pCmd->windowRect.top);
        m_rcWindow.right = static_cast<float>(pCmd->windowRect.right);
        m_rcWindow.bottom = static_cast<float>(pCmd->windowRect.bottom);
    }

    m_fChild = pCmd->isChild != 0;
    m_WindowProperties = (pCmd->isRTL != 0) ? MilWindowProperties::RtlLayout : 0;
    m_WindowProperties |= ((pCmd->gdiBlt != 0) ? MilWindowProperties::PresentUsingGdi : 0);

    IFC(UpdateRenderTargetFlags(
        m_UCETargetFlags
        ));
    

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------
// CSlaveHWndRenderTarget::ProcessSetClearColor
//------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::ProcessSetClearColor(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_TARGET_SETCLEARCOLOR* pCmd
    )
{
    HRESULT hr = S_OK;

    RtlCopyMemory(&m_clearColor, &pCmd->clearColor, sizeof(m_clearColor));
    m_fNeedsFullRender = true;

    if (!(m_UCETargetFlags & MilRTInitialization::NeedDestinationAlpha) && (m_clearColor.a < 1.0f))
    {
        //
        // We need to add a new flag to the initalization flags,
        // that also forces us to recreate the render target.
        //
        IFC(SetNewUCETargetFlags(m_UCETargetFlags | MilRTInitialization::NeedDestinationAlpha));
    }

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------
// CSlaveHWndRenderTarget::ProcessSetRenderingFlags
//------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::ProcessSetFlags(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_TARGET_SETFLAGS* pCmd
    )
{
    HRESULT hr = S_OK;

    const MilRTInitialization::Flags c_dwAllowedFlags =
        (MilRTInitialization::TypeMask | MilRTInitialization::UseRefRast | MilRTInitialization::UseRgbRast);

    MilRTInitialization::Flags dwNewInitializationFlags;

    if (pCmd->flags & ~c_dwAllowedFlags)
    {
        IFC(E_INVALIDARG);
    }

    dwNewInitializationFlags =
        pCmd->flags | (m_UCETargetFlags & ~c_dwAllowedFlags);

    IFC(SetNewUCETargetFlags(dwNewInitializationFlags));

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------
// CSlaveHWndRenderTarget::ProcessInvalidate
//------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::ProcessInvalidate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_TARGET_INVALIDATE* pCmd,
    __in_bcount_opt(cbPayload) LPCVOID pPayload,
    UINT cbPayload
    )
{
    HRESULT hr = S_OK; 
    
    // We get a WM_PAINT when the screen is unlocked.... or immediately after
    // invalidation if the window is layered or running in the DWM.

    // If we start rendering again here, this will be the right solution for
    // non-layered windows on XPDM. Other scenarios will still render when the
    // screen is locked but those scenarios are not as bad. See comment in
    // HandleWindowErrors for more information.
    
    m_fNoScreenAccess = false;

    if (   (pCmd->rc.right > pCmd->rc.left)
        && (pCmd->rc.bottom > pCmd->rc.top))
    {
        CMilRectF Rect;

        Rect.left = static_cast<FLOAT>(pCmd->rc.left);
        Rect.top  = static_cast<FLOAT>(pCmd->rc.top);
        Rect.right  = static_cast<FLOAT>(pCmd->rc.right);
        Rect.bottom = static_cast<FLOAT>(pCmd->rc.bottom);

        IFC(InvalidateInternal(&Rect));
    }


Cleanup:
    RRETURN(hr);
}
  

//-----------------------------------------------------------------------------
// CSlaveHWndRenderTarget::SetNewRenderTargetFlags
//-----------------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::SetNewUCETargetFlags(
    MilRTInitialization::Flags NewUCETargetFlags
    )
{
    HRESULT hr = S_OK;

    if (m_UCETargetFlags != NewUCETargetFlags)
    {
        IFC(UpdateRenderTargetFlags(NewUCETargetFlags));

        m_UCETargetFlags = NewUCETargetFlags;
    }

Cleanup:
    RRETURN(hr);
}


//-----------------------------------------------------------------------------
// CSlaveHWndRenderTarget::UpdateRenderTargetFlags
//-----------------------------------------------------------------------------

override
HRESULT
CSlaveHWndRenderTarget::UpdateRenderTargetFlags()
{
    // We want to update the render flags to possibly include software
    // based upon RenderOptions and the requested flags. We don't want to 
    // overwrite the current flags requested by the client so we don't call 
    // SetNewUCETargetFlags().
    RRETURN(UpdateRenderTargetFlags(m_UCETargetFlags));   
}

//-----------------------------------------------------------------------------
// CSlaveHWndRenderTarget::UpdateRenderTargetFlags
//-----------------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::UpdateRenderTargetFlags(
    MilRTInitialization::Flags UCETargetFlags
    )
{
    HRESULT hr = S_OK;

    MilRTInitialization::Flags RenderTargetFlags = UCETargetFlags;

    CMILFactory * const pMILFactory = m_pComposition->GetMILFactory();

    if (m_fSoftwareFallback || m_pComposition->GetLastForceSoftwareForProcessValue())
    {
        // Software fallback. Need to force software render target creation.
        // See HandleWindowErrors.
        Assert((RenderTargetFlags & MilRTInitialization::TypeMask) != MilRTInitialization::HardwareOnly);
        RenderTargetFlags |= MilRTInitialization::SoftwareOnly;
    }

    IFC(pMILFactory->ComputeRenderTargetTypeAndPresentTechnique(
        m_hWnd,
        m_WindowProperties,
        m_WindowLayerType,
        RenderTargetFlags,
        OUT &RenderTargetFlags
        ));

    if (m_RenderTargetFlags != RenderTargetFlags)
    {
        m_RenderTargetFlags = RenderTargetFlags;
        ReleaseResources();
    }

Cleanup:
    pMILFactory->Release();

    RRETURN(hr);
}


//---------------------------------------------------------------------------------
// CSlaveHWndRenderTarget::EnsureRenderTargetInternal
//---------------------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::EnsureRenderTargetInternal()
{
    HRESULT hr = S_OK;

    CMILFactory *pMILFactory = NULL;

    //
    // Check DWM composition status when windowed.
    //
    MilRTInitialization::Flags uceFlags = m_UCETargetFlags;
    BOOL fCompositionEnabled = FALSE;

    if (m_fIsZombie)
    {
        goto Cleanup;
    }

    IFC(DWMAPI::OSCheckedIsCompositionEnabled(&fCompositionEnabled));

    if (fCompositionEnabled)
    {
        uceFlags |= MilRTInitialization::DisableDisplayClipping;
    }
    else
    {
        uceFlags &= ~MilRTInitialization::DisableDisplayClipping;
    }

    IFC(SetNewUCETargetFlags(uceFlags));

    if (m_pRenderTarget == NULL)
    {
        MilRTInitialization::Flags creationFlags = m_RenderTargetFlags;

        EventWriteWClientDesktopRTCreateBegin((UINT64)m_hWnd);

        pMILFactory = m_pComposition->GetMILFactory();

        IFC(pMILFactory->CreateDesktopRenderTarget(
            m_hWnd,
            m_WindowLayerType,
            creationFlags,
            &m_pRenderTarget
            ));

        EventWriteWClientDesktopRTCreateEnd((UINT64)m_hWnd);

        m_fNeedsFullRender = true;
        m_fTransparencyDirty = true;        
    }

    Assert(m_pRenderTarget != NULL);

    //
    // See if there is need to update the render target with the window settings changes.
    //
    IFC(UpdateWindowSettingsInternal());

Cleanup:

    ReleaseInterfaceNoNULL(pMILFactory);

    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CSlaveHWndRenderTarget::UpdateWindowSettingsInternal
//---------------------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::UpdateWindowSettingsInternal()
{
    HRESULT hr = S_OK;

    Assert(m_pRenderTarget != NULL);

    //
    // Render targets for child windows do not receive UpdateWindowSettings
    // commands to update their size and location.  So we have to query
    // it all the time.
    //
    
    if (m_fChild)
    {
        IFC(CalculateWindowRect());
    }

    //
    // Tell Render Target where it is now located.  If there are
    // any areas that need re-rendering those areas will be
    // returned by a call to GetInvalidRegions.
    //
    // This is also a point of detection for mode change.
    //

    IFC(m_pRenderTarget->SetPosition(&m_rcWindow));

    //
    // Update Present transparency properties
    //

    if (m_fTransparencyDirty)
    {
        IFC(m_pRenderTarget->UpdatePresentProperties(
            m_WindowTransparency,
            m_constantAlpha,
            m_colorKey
            ));

        m_fTransparencyDirty = false;
        m_fNeedsPresent = true;
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------------
// CSlaveHWndRenderTarget::InvalidateInternal()
//---------------------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::InvalidateInternal(
    __in_ecount_opt(1) const MilRectF *pRect
    )
{
    HRESULT hr = S_OK;

    if (!pRect)
    {
        m_fFullRegionInvalid = true;
    }

    if (!m_fFullRegionInvalid)
    {
        IFC(m_invalidRegions.Add(*pRect));
    }
    
    m_fHasInvalidRegions = true;

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CSlaveHWndRenderTarget::SendInvalidRegions()
//---------------------------------------------------------------------------------

HRESULT
CSlaveHWndRenderTarget::SendInvalidRegions()
{
    HRESULT hr = S_OK;

    if (m_fFullRegionInvalid)
    {
        IFC(m_pRenderTarget->Invalidate(NULL));
    }
    else
    {
        for (UINT i = 0; i < m_invalidRegions.GetCount(); i++)
        {
            IFC(m_pRenderTarget->Invalidate(&m_invalidRegions[i]));
        }
    }

    // Clear the region
    m_invalidRegions.Reset(FALSE);
    m_fFullRegionInvalid = false;
    m_fHasInvalidRegions = false;

    m_fNeedsPresent = true; // If we have invalid regions we also need to present them.

Cleanup:
    RRETURN(hr);

}

//+-----------------------------------------------------------------------
//
//  Member: CSlaveHWndRenderTarget::HandleWindowErrors
//
//  Synopsis:  Factorisation of error handling in Render, Present and EnsureRenderTargetInternal methods
//
//  Returns:  Error value to be returned by said functions
//
//------------------------------------------------------------------------
HRESULT
CSlaveHWndRenderTarget::HandleWindowErrors(HRESULT hr)
{
    HRESULT hrReturn = hr;

    if (FAILED(hr))
    {
        // First check if the window is still valid - actual error won't matter
        // if window is invalid.
        if (m_hWnd && !IsWindow(m_hWnd))
        {
            MIL_THR(__HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE));
        }

        switch (hr)
        {
            case D3DERR_OUTOFVIDEOMEMORY:
                // If we are out of video memory we fall back to software rendering, but only if the hardware only flag is
                // not set. (E.g. the DWM will set the hw only flag and hence never fall back to software).

                if (!(m_UCETargetFlags & MilRTInitialization::HardwareOnly))
                {
                    // Setting the flag will force us to create a software render target on the next render.
                    // This will never allow us to get back into hw mode. To enable resetting the render target
                    // using the SetFlags command simply set 
                    //     m_UCETargetFlags = m_UCETargetFlags | MilRTInitialization::SoftwareOnly;
                    // This feature intentionally not enabled becuase we do not have any API exposure for it and 
                    // there is no test plan. 
                    Assert((m_UCETargetFlags & MilRTInitialization::TypeMask) != MilRTInitialization::HardwareOnly);
                    Assert((m_RenderTargetFlags & MilRTInitialization::TypeMask) != MilRTInitialization::HardwareOnly);
                    m_fSoftwareFallback = true;
                    ReleaseResources();
                    InvalidateWindow();
                    hrReturn = S_OK;
                }
                break;

            case WGXERR_NEED_RECREATE_AND_PRESENT:                  // D3D/Driver bug - retry
            case WGXERR_DISPLAYSTATEINVALID:                        // FALL THROUGH - Display has changed modes
                ReleaseResources();
                SetScreenAccessDenied();

                // If m_LastKnownDisplayDevicesAvailabilityChangedWParam == 0, it means that the 
                // UI thread will eventually call InvalidateRect(HWND) when it receives an updated 
                // m_DisplayDevicesAvailabilityChanged Window Message with wParam = 1. 
                // Until then, we do not need to continue invalidating the window - doing so will 
                // simply generate a series of WM_PAINT messages that would be handled and ignroed by 
                // the UI thread
                if ((hr == WGXERR_NEED_RECREATE_AND_PRESENT) ||
                    ((hr == WGXERR_DISPLAYSTATEINVALID) && (m_LastKnownDisplayDevicesAvailabilityChangedWParam != 0)))
                {
                    InvalidateWindow();
                }

                // The composition object needs to know when the underlying render targets
                // are being recreated; therefore we bubble WGXERR_DISPLAYSTATEINVALID 
                // up. 
                hrReturn = WGXERR_DISPLAYSTATEINVALID;
                break;
                
            case WGXERR_SCREENACCESSDENIED:     // Display is locked right now
                SetScreenAccessDenied();
                __fallthrough;

            case WGXERR_NEED_REATTEMPT_PRESENT:                     // Win32 user-mode bug - need to Present again
            case __HRESULT_FROM_WIN32(ERROR_INCORRECT_SIZE):        // Resize message needed before Present will work again
                InvalidateWindow(); // Ensures WM_PAINT

                hrReturn = S_OK;
                break;
                
            case __HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE): // FALL THROUGH Window has been asynchronously destroyed
                m_fIsZombie = TRUE;
                __fallthrough;

            case WGXERR_GENERIC_IGNORE:                             // FALL THROUGH Ignoring errors with this tag for now to unblock cinch
            case WGXERR_DISPLAYFORMATNOTSUPPORTED:                  // 16 bit mode, suppress error for now
            case WGXERR_NO_HARDWARE_DEVICE:                         // Hw only requested, but not available, suppress error until DWM/render thread get in sync
                ReleaseResources();
                hrReturn = S_OK;
                break;

            default:        // No match was found - this is some other type of error so return original result
                ReleaseResources();
                break;
        }
        m_fNeedsFullRender = true;
        m_fTransparencyDirty = true;
    }

    //
    // Somewhat special case for S_PRESENT_OCCLUDED.  In this case nothing is 
    // wrong with the render target, we just need to do a full present when we 
    // become un-occluded.
    //
    if (hrReturn == S_PRESENT_OCCLUDED)
    {
        InvalidateInternal();
    }

    RRETURN1(hrReturn, S_PRESENT_OCCLUDED);
}

//+-----------------------------------------------------------------------
//
//  Member: CSlaveHWndRenderTarget::SetScreenAccessDenied
//
//  Synopsis:
//       This method sets the screen access denied state.
//
//------------------------------------------------------------------------

void
CSlaveHWndRenderTarget::SetScreenAccessDenied()
{

    // On XPDM we get WGXERR_SCREENACCESSDENIED when the screen
    // is locked for software render targets or for hardware
    // render targets that present to GDI. For hardware render
    // targets that  do not present to GDI we get
    // WGXERR_DISPLAYSTATEINVALID.   
    //
    // On Vista WDDM, D3D Presents return S_PRESENT_OCCLUDED,
    // but lower levels of the code eat this error. GDI
    // Presents return WGXERR_SCREENACCESSDENIED when the
    // screen is locked, but only when the DWM is off.
    //
    // We try to detect when the screen is unlocked by calling
    // InvalidateRect. On ProcessInvalidate we reset the
    // m_fNoScreenAccess flag and start rendering again. As
    // long as the window is non-layered and the DWM is off,
    // the InvalidateRect will cause a WM_PAINT to be issued
    // when the screen is unlocked. Note that in layered or DWM
    // scenarios, this doesn't work so rendering will still
    // occur during screen lock. At least we won't fallback to
    // software (thus doing more work) when the screen is
    // locked, since layered windows are already software, and
    // on WDDM we can still be hardware accelerated when the
    // screen is locked.
    //
    // To ensure that we don't get into trouble with the DWM
    // with this optimization we only do the optimization for
    // windowed hwndtargets (not fullscreen).
    //
    // WARNING WARNING WARNING
    //
    // Extreme caution must be taken with this approach. When
    // the DWM is on, and for layered windows, the WM_PAINT
    // will not be held back until the screen is unlocked. It
    // will come to us immediately. This *could* cause us to
    // loop, producing a WM_PAINT "storm", *if* we fail with
    // one of the above error codes in these cases when the
    // screen is (still) locked. 
    //
    // Fortunately for us, the cases
    // which do not hold back WM_PAINTs also do not return
    // errors from Present... and for the same reason. These
    // cases all involve redirected windows, and redirected
    // windows can still be redirected when the screen is
    // locked. The only area of concern is if we were to stop
    // eating S_PRESENT_OCCLUDED and pass that error code
    // through here.
    //
    // 2009/09/23 BedeJ - Unfortunately for us, this last paragraph
    // is not entirely true. If the monitor is powered off, but the 
    // display is not locked, a D3D present can fail with
    // S_PRESENT_OCCLUDED (see CD3DDeviceLevel1::PresentWithD3D). When
    // I tried to work around this by invalidating the window before 
    // silently eating the error, I ended up in the WM_PAINT storm
    // situation described here. The fix I developed instead was to 
    // continue to ignore the failure, but have the UI thread register
    // for and listen to power broadcast events for monitor power on/off, 
    // and to invalidate the entire window when the monitor is powered
    // back on if a failure . To avoid unnecessary invalidation of windows that
    // haven't had a failed present, we send a window message to the UI
    // thread from the rendering thread when a present fails, to signal
    // that the invalidation is necessary.
    //

    m_fNoScreenAccess = true;  
}               

//+-----------------------------------------------------------------------
//
//  Member: CSlaveHWndRenderTarget::WaitForVBlank
//
//  Synopsis:  Waits until a vblank occurs on the display that the
//             D3D device used by this target is created on
//
//  Returns: S_OK unless DX call fails.
//           S_OK if the D3D device is not yet created
//
//------------------------------------------------------------------------
HRESULT CSlaveHWndRenderTarget::WaitForVBlank()
{
    BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
        WGXERR_NO_HARDWARE_DEVICE
    END_MILINSTRUMENTATION_HRESULT_LIST

    HRESULT hr = S_OK;


    if (m_fRenderingEnabled && !m_fIsZombie && m_pRenderTarget)
    {
        MIL_THR(m_pRenderTarget->WaitForVBlank());
    }
    else
    {
        MIL_THR(WGXERR_NO_HARDWARE_DEVICE);
    }
    
    RRETURN(hr);
}

//------------------------------------------------------------------
// CSlaveHWndRenderTarget::ReleaseResources
// 
// Wrapper function for releasing all render targets and cleaning
// up dependent member variables
//------------------------------------------------------------------
void
CSlaveHWndRenderTarget::ReleaseResources()
{      
    ReleaseInterface(m_pRenderTarget);
    ReleaseDrawingContext();
}
    
//+------------------------------------------------------------------------
//
//  Function:  CSlaveHWndRenderTarget::Advance
//
//  Synopsis:  Advances frame count and inserts a gpu marker.
//             
//-------------------------------------------------------------------------
void
CSlaveHWndRenderTarget::AdvanceFrame(UINT uFrameNumber)
{
    if (m_fRenderingEnabled && !m_fIsZombie && m_pRenderTarget)        
    {
        m_pRenderTarget->AdvanceFrame(uFrameNumber);
    }
}
    
//+------------------------------------------------------------------------
//
//  Function:  CSlaveHWndRenderTarget::GetNumQueuedPresents
//
//  Synopsis:  Forwards to the rendertarget if it's valid, otherwise 
//             returns 0.
//             
//-------------------------------------------------------------------------
HRESULT
CSlaveHWndRenderTarget::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    HRESULT hr = S_OK;

    if (m_fRenderingEnabled 
        && !m_fIsZombie 
        &&  m_pRenderTarget
        )
    {
        IFC(m_pRenderTarget->GetNumQueuedPresents(puNumQueuedPresents));
    }
    else
    {
        *puNumQueuedPresents = 0;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Function:  CSlaveHWndRenderTarget::GetBaseRenderTargetInternal
//
//  Synopsis:  Returns the underlying render target internal.
//
//------------------------------------------------------------------------
HRESULT 
CSlaveHWndRenderTarget::GetBaseRenderTargetInternal(
    __deref_out_opt IRenderTargetInternal **ppIRT
    )
{
    HRESULT hr = S_OK;

    if (m_pRenderTarget != NULL)
    {
        IFC(m_pRenderTarget->QueryInterface(IID_IRenderTargetInternal, (void **)ppIRT));
    }
    else
    {
        *ppIRT = NULL;
    }

Cleanup:
    RRETURN(hr);
}

