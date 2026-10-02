// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      CHwDisplayRenderTarget implementation
//
//      This object creates the HW abstraction for the render target, manages a
//      dirty rect list and performs the logic for stepped rendering.
//

#include "precomp.hpp"

//+------------------------------------------------------------------------
//
//  Function:  IsOnPixelBoundary
//
//  Synopsis:  Returns TRUE if pixel is on a pixel boundary
//
//-------------------------------------------------------------------------
BOOL IsOnPixelBoundary(float f)
{
    return !((CFloatFPU::Round(f * FIX4_ONE) * c_nShiftSize) %  (FIX4_ONE * c_nShiftSize));
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::Create
//
//  Synopsis:  1. Create the CD3DDeviceLevel1
//             2. Check format support
//             3. Create and initialize the CHwDisplayRenderTarget
//
//-------------------------------------------------------------------------
HRESULT
CHwDisplayRenderTarget::Create(
    __in_ecount_opt(1) HWND hwnd,
    MilWindowLayerType::Enum eWindowLayerType,
    __in_ecount(1) CDisplay const *pDisplay,
    D3DDEVTYPE type,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) CHwDisplayRenderTarget **ppRenderTarget
    )
{
    HRESULT hr = S_OK;

    Assert(hwnd);

    *ppRenderTarget = NULL;

    CD3DDeviceLevel1 *pD3DDevice = NULL;

    D3DPRESENT_PARAMETERS D3DPresentParams;
    UINT AdapterOrdinalInGroup;

    //
    // First, we try to find the d3d device and
    // determine our present parameters.
    //

    CD3DDeviceManager *pD3DDeviceManager = CD3DDeviceManager::Get();
    Assert(pDisplay->D3DObject()); // we should not get here with null pID3D

    IFC(pD3DDeviceManager->GetD3DDeviceAndPresentParams(
        hwnd,
        dwFlags,
        pDisplay,
        type,
        &pD3DDevice,
        &D3DPresentParams,
        &AdapterOrdinalInGroup
        ));

    //
    // Make sure render target format has been tested.
    //

    HRESULT const *phrTestGetDC;

    IFC(pD3DDevice->CheckRenderTargetFormat(
        D3DPresentParams.BackBufferFormat,
        OUT &phrTestGetDC
        ));

    //
    // If a DC will be needed, make sure that has been successfully done once
    // with this format.
    //

    if ((dwFlags & MilRTInitialization::PresentUsingMask) != MilRTInitialization::PresentUsingHal)
    {
        Assert(*phrTestGetDC != WGXERR_NOTINITIALIZED);

        if (FAILED(*phrTestGetDC))
        {
            IFC(WGXERR_NO_HARDWARE_DEVICE);
        }
    }


    {
        DisplayId associatedDisplay = pDisplay->GetDisplayId();

        *ppRenderTarget = new CHwHWNDRenderTarget(
            pD3DDevice,
            D3DPresentParams,
            AdapterOrdinalInGroup,
            associatedDisplay,
            eWindowLayerType
            );
        
        IFCOOM(*ppRenderTarget);
        (*ppRenderTarget)->AddRef(); // CHwDisplayRenderTarget::ctor sets ref count == 0
    }

    //
    // Call init
    //

    IFC((*ppRenderTarget)->Init(
        hwnd,
        pDisplay,
        type,
        dwFlags
        ));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppRenderTarget);
    }
    ReleaseInterfaceNoNULL(pD3DDevice);
    pD3DDeviceManager->Release();
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::Init
//
//  Synopsis:  Initialize the presentation context (MILDC)
//
//  Remarks:   Subclasses are responsible for initializing
//             m_pD3DSwapChain in this method
//
//-------------------------------------------------------------------------
HRESULT
CHwDisplayRenderTarget::Init(
    __in_ecount_opt(1) HWND hwnd,
    __in_ecount(1) CDisplay const *pDisplay,
    D3DDEVTYPE type,
    MilRTInitialization::Flags dwFlags
    )
{
    Assert(hwnd);

    m_MILDC.Init(
        hwnd,
        dwFlags
        );

#if DBG_STEP_RENDERING
    m_fDbgClearOnPresent = !(dwFlags & MilRTInitialization::PresentRetainContents);
#endif DBG_STEP_RENDERING

    RRETURN(S_OK);
}


//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::CHwDisplayRenderTarget
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CHwDisplayRenderTarget::CHwDisplayRenderTarget(
    __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    __in_ecount(1) D3DPRESENT_PARAMETERS const &D3DPresentParams,
    UINT AdapterOrdinalInGroup,
    DisplayId associatedDisplay
    ) :
    m_D3DPresentParams(D3DPresentParams),
    m_AdapterOrdinalInGroup(AdapterOrdinalInGroup),
    CHwSurfaceRenderTarget(
        pD3DDevice,
        D3DFormatToPixelFormat(D3DPresentParams.BackBufferFormat, TRUE),
        D3DPresentParams.BackBufferFormat,
        associatedDisplay
        )
{
    m_pD3DSwapChain = NULL;
    m_dwPresentFlags = 0;
    m_fEnableRendering = TRUE;
    m_hrDisplayInvalid = S_OK;
    DbgSetInvalidContents();
#if DBG_STEP_RENDERING
    // Set the parent to be itself, not ref counted of course
    m_pDisplayRTParent = this;
#endif DBG_STEP_RENDERING

    //
    // Update hw render target stats
    //

    if (g_pMediaControl)
    {
        InterlockedIncrement(reinterpret_cast<LONG *>(
            &(g_pMediaControl->GetDataPtr()->NumHardwareRenderTargets)
            ));
    }
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::~CHwDisplayRenderTarget
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CHwDisplayRenderTarget::~CHwDisplayRenderTarget()
{
    ReleaseInterface(m_pD3DSwapChain);
#if DBG_STEP_RENDERING
    // NULL the parent so CHwSurfaceRT won't try to release it
    m_pDisplayRTParent = NULL;
#endif DBG_STEP_RENDERING

    //
    // Update hw render target stats
    //

    if (g_pMediaControl)
    {
        InterlockedDecrement(reinterpret_cast<LONG *>(
            &(g_pMediaControl->GetDataPtr()->NumHardwareRenderTargets)
            ));

    }
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::HrFindInterface
//
//  Synopsis:  HrFindInterface implementation that responds to render
//             target QI's.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    // HWND classes are protected by CMetaRenderTarget and never
    // need to be QI'ed, therefore never needing to call HrFindInterface
    AssertMsg(false, "CHwDisplayRenderTarget is not allowed to be QI'ed.");
    RRETURN(E_NOINTERFACE);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::Clear
//
//  Synopsis:  1. Clear the surface to a given color.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    HRESULT hr = S_OK;

    AssertNoDeviceEntry(*m_pD3DDevice);

    if (m_fEnableRendering)
    {
        IFC(CHwSurfaceRenderTarget::Clear(pColor, pAliasedClip));

        DbgSetValidContents();
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:    CHwDisplayRenderTarget::Begin3D
//
//  Synopsis:  Delegate to CHwSurfaceRenderTarget if enabled
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    HRESULT hr = S_OK;

    AssertNoDeviceEntry(*m_pD3DDevice);

    if (m_fEnableRendering)
    {
        // No instrumentation to optimize call and return
        hr = CHwSurfaceRenderTarget::Begin3D(rcBounds, AntiAliasMode, fUseZBuffer, rZ);
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:    CHwDisplayRenderTarget::End3D
//
//  Synopsis:  Delegate to CHwSurfaceRenderTarget if enabled
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::End3D(
    )
{
    HRESULT hr = S_OK;

    AssertNoDeviceEntry(*m_pD3DDevice);

    if (m_fEnableRendering)
    {
        // No instrumentation to optimize call and return
        hr = CHwSurfaceRenderTarget::End3D();
    }

    return hr;
}


//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::Present
//
//  Synopsis:  1. Present the flipping chain
//             2. Update the render target
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::Present(
    __in_ecount(1) const RECT *pRect
    )
{
    HRESULT hr = S_OK;

    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

    Assert(m_LayerStack.GetCount() == 0);
    DbgAssertBoundsState();

    CMILSurfaceRect presentRect;

    //
    // Don't present if rendering is disabled
    //

    if (!m_fEnableRendering)
    {
        MIL_THR(m_hrDisplayInvalid);
        goto Cleanup;
    }

    bool fPresent = false;
    RGNDATA *pDirtyRegion = NULL;
    IFC(ShouldPresent(
        pRect,
        &presentRect,
        &pDirtyRegion,
        &fPresent
        ));

    if (!fPresent) goto Cleanup;

    //
    // If swap chain creation failed then we must
    //  fail here.
    //

    if (!m_pD3DSwapChain)
    {
        TraceTag((tagWarning,
                  "CHwDisplayRenderTarget::Present called in absence of a valid swap chain."));
        IFC(E_FAIL);
    }

    //
    // Call present and check for mode change
    //

#if DBG
    AssertMsg(m_fDbgInvalidContents,
              "A render target is being Presented, but its contents\r\n"
              "are not valid.  This is usually failure of the caller\r\n"
              "to make any rendering requests of this target after a\r\n"
              "Resize operation or creation.  Ignoring this error will\r\n"
              "likely result in garbage being displayed.");
#endif

    // Note that WGXERR_DISPLAYSTATEINVALID is bubbled up here so the caller is
    // responsible for recreating this object
    if (m_D3DPresentParams.SwapEffect == D3DSWAPEFFECT_COPY)
    {        
        AssertConstMsgW(   (  m_MILDC.GetRTInitializationFlags() 
                            & MilRTInitialization::PresentRetainContents
                              )
                        || IsTagEnabled(tagMILStepRendering),
                        L"SwapEffect is copy, but flag don't request this.\n"
                        L" !!! Ignore this if changing tagMILStepRendering.");
    }
    else
    {
        // Can't use the dirty region for any other SwapEffect
        pDirtyRegion = NULL;
    }

    // Source and destination are always the same for us.
    IFC(PresentInternal(
        &presentRect,
        &presentRect,
        pDirtyRegion
        ));

    if (m_D3DPresentParams.SwapEffect == D3DSWAPEFFECT_DISCARD)
    {
        DbgSetInvalidContents();
    }

#if DBG_STEP_RENDERING
    //
    // When retain contents was not specified for in the
    // creation flags, clear the back buffer in debug mode
    // to alternating colors so that any areas not properly
    // redrawn before the next present will be easily
    // identified.
    //

    if (m_fDbgClearOnPresent)
    {
        static bool fGreen = false;

        static const MilColorB green = MIL_COLOR(255, 0, 255, 0);
        static const MilColorB purple = MIL_COLOR(255, 255, 0, 128);

        if (SUCCEEDED(SetAsRenderTarget()))
        {
            IGNORE_HR(m_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, fGreen ? green: purple, 0, 0));
        }

        fGreen = !fGreen;
    }
#endif DBG_STEP_RENDERING

Cleanup:
    //
    // Reset invalidated rects (even on failure). Nothing we can do if the clear
    // fails either.
    //
    IGNORE_HR(ClearInvalidatedRects());

    if (FAILED(hr))
    {
        //
        // Remember if the display is invalid, because we want to be consistent
        // about returning WGXERR_DISPLAYSTATEINVALID during Present.
        //

        if (hr == WGXERR_DISPLAYSTATEINVALID)
        {
            m_hrDisplayInvalid = hr;
        }

        m_fEnableRendering = FALSE;
    }

    RRETURN1(hr, S_PRESENT_OCCLUDED);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwDisplayRenderTarget::InvalidateRect
//
//  Synopsis:  Check for enabled rendering and retain contents before 
//             delegating to base class.
//

HRESULT 
CHwDisplayRenderTarget::InvalidateRect(
    __in_ecount(1) CMILSurfaceRect const *pRect
    )
{
    HRESULT hr = S_OK;

    if (m_fEnableRendering)
    {
        IFC(CBaseSurfaceRenderTarget<CHwRenderTargetLayerData>::InvalidateRect(pRect));
    }

Cleanup:
    RRETURN(hr)
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwDisplayRenderTarget::PresentInternal
//
//  Synopsis:  Detects parameters for the present call based on RenderTarget
//             layout, and desired swap effect.
//

HRESULT 
CHwDisplayRenderTarget::PresentInternal(
    __in_ecount(1) CMILSurfaceRect const *prcSource,
    __in_ecount(1) CMILSurfaceRect const *prcDest,
    __in_ecount_opt(1) RGNDATA const *pDirtyRegion
    ) const
{
    HRESULT hr = S_OK;

    Assert(m_D3DPresentParams.hDeviceWindow == m_MILDC.GetHWND());

    if (m_D3DPresentParams.SwapEffect == D3DSWAPEFFECT_COPY)
    {
        IFC(m_pD3DDevice->Present(
            m_pD3DSwapChain,
            prcSource,
            prcDest,
            &m_MILDC,
            pDirtyRegion,
            m_dwPresentFlags
            ));
    }
    else
    {
        //
        // When we're flipping we may not specify source/dest rectangles and
        // dirty regions.
        //
        IFC(m_pD3DDevice->Present(
            m_pD3DSwapChain,
            NULL,
            NULL,
            &m_MILDC,
            NULL,
            m_dwPresentFlags
            ));
    }

Cleanup:
    RRETURN1(hr, S_PRESENT_OCCLUDED);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwDisplayRenderTarget::IsValid
//
//  Synopsis:  Returns FALSE when rendering with this render target or any use
//             is no longer allowed.  Mode change is a common cause of of
//             invalidation.
//
//-----------------------------------------------------------------------------

bool
CHwDisplayRenderTarget::IsValid() const
{
    Assert(m_pD3DSwapChain);
    return m_fEnableRendering && m_pD3DSwapChain->IsValid();
}


#if DBG_STEP_RENDERING

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::ShowSteppedRendering
//
//  Synopsis:  Present the current backbuffer or the given texture
//             when enabled in debug builds
//
//-------------------------------------------------------------------------
DeclareTag(tagMILStepRenderingLockHW, "MIL", "MIL Step Rendering Lock HW");

#if !DBG
volatile BOOL g_fStepHWRendering = false;
volatile BOOL g_fStepHWRenderingLock = false;
#endif

void 
CHwDisplayRenderTarget::ShowSteppedRendering(
    __in LPCTSTR pszRenderDesc,
    __in_ecount(1) const ISteppedRenderingSurfaceRT *pRT
    )
{
    if (
#if DBG
        !IsTagEnabled(tagMILStepRendering)
#else
        !g_fStepHWRendering
#endif
        )
    {
        return;
    }

    HRESULT hr = S_OK;

    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    Assert(m_pD3DTargetSurface);

    CMILSurfaceRect rcPresentSource;
    CMILSurfaceRect rcPresentDest(
        0,
        0, 
        m_uWidth, 
        m_uHeight,
        XYWH_Parameters
        );

    rcPresentSource = rcPresentDest;

    IDirect3DSurface9 *pID3DLockableSurface = NULL;
    CD3DSurface *pD3DSurface = NULL;
    RECT rcCopySrc;
    bool fShrink = false;

    bool fSaveRestore = false;
    MilPointAndSizeL rcSave;

    bool fBreak = false;
    bool fUnlock = false;

    m_pD3DDevice->DbgBeginStepRenderingPresent();

    pRT->DbgGetTargetSurface(&pD3DSurface);

    if (pD3DSurface == NULL)
    {
        //
        // The target RT is a software render target. Use the bitmap obtained
        // from DbgGetSurfaceBitmap to generate a CD3DSurface
        //

        IWGXBitmap *pSurfaceBitmapNoRef = NULL;

        pRT->DbgGetSurfaceBitmapNoRef(&pSurfaceBitmapNoRef);

        MIL_THR(DbgStepCreateD3DSurfaceFromBitmapSource(
            pSurfaceBitmapNoRef,
            &pD3DSurface
            ));

        if (FAILED(hr))
        {
            TraceTag((tagWarning, "Failed to create CD3DSurface from software surface."));
            goto Cleanup;
        }
    }

    //
    // Check if we need to show the contents of a surface other
    // than the presentable one (this HWND RT's backbuffer)
    //
    D3DSURFACE_DESC const &d3dsd = pD3DSurface->Desc();

    if (pD3DSurface != m_pD3DTargetSurface)
    {

        rcCopySrc.left = 0;
        rcCopySrc.top = 0;

        //
        // Try to show the whole D3D target area, but if it is too
        // large show as much as possible, shrinking if necessary.
        //

        if (d3dsd.Width <= m_uWidth)
        {
            // Show entire width 1:1
            rcPresentDest.right = d3dsd.Width;
            rcCopySrc.right = d3dsd.Width;
        }
        else
        {
            if (pRT->DbgTargetWidth() <= m_uWidth)
            {
                // Show some border
                rcCopySrc.right = m_uWidth;
            }
            else
            {
                // Revert to a shrink to show at least all
                // of the important source area
                fShrink = true;
                rcCopySrc.right = pRT->DbgTargetWidth();
            }
        }

        if (d3dsd.Height <= m_uHeight)
        {
            // Show entire height 1:1
            rcPresentDest.bottom = d3dsd.Height;
            rcCopySrc.bottom = d3dsd.Height;
        }
        else
        {
            if (pRT->DbgTargetHeight() <= m_uHeight)
            {
                // Show some border
                rcCopySrc.bottom = m_uHeight;
            }
            else
            {
                // Revert to a shrink to show at least all
                // of the important source area
                fShrink = true;
                rcCopySrc.bottom = pRT->DbgTargetHeight();
            }
        }

        // Save/restore the area overwritten by the copy/stretch.
        fSaveRestore = true;
        rcSave.Width  = rcPresentDest.right;
        rcSave.Height = rcPresentDest.bottom;
    }

    //
    // If the contents could be lost on Present then make sure
    // to save/restore the entire presentation surface.
    //

    if (m_D3DPresentParams.SwapEffect != D3DSWAPEFFECT_COPY)
    {
        fSaveRestore = true;
        rcSave.Width = m_uWidth;
        rcSave.Height = m_uHeight;
    }

    if (fSaveRestore)
    {
        rcSave.X = 0;
        rcSave.Y = 0;

        hr = THR(m_pD3DDevice->DbgSaveSurface(
            m_pD3DTargetSurface,
            rcSave
            ));

        if (FAILED(hr))
        {
            fSaveRestore = false;

            TraceTag((tagWarning,
                      "Unable to save RT for incremental Present."));
            if (!pD3DSurface)
            {
                TraceTag((tagWarning,
                          "  Try enabling tagMILStepRendering prior to"
                          " RT creation."));
            }
        }
    }

    if (SUCCEEDED(hr) && (pD3DSurface != m_pD3DTargetSurface))
    {
        hr = THR(m_pD3DDevice->StretchRect(
            pD3DSurface,
            &rcCopySrc,
            m_pD3DTargetSurface,
            &rcPresentDest,
            ((fShrink && m_pD3DDevice->DbgCanShrinkRectLinear()) ?
             D3DTEXF_LINEAR : D3DTEXF_NONE)
            ));

        if (FAILED(hr))
        {
            TraceTag((tagWarning, "Incremental offscreen Present failed."));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = THR(PresentInternal(
            &rcPresentSource,
            &rcPresentDest,
            NULL
            ));

        if (hr == S_PRESENT_OCCLUDED)
        {
            TraceTag((tagWarning, "Incremental Present was occluded.\n"));
            hr = S_OK;
        }
        else if (FAILED(hr))
        {
            TraceTag((tagWarning, "Incremental Present failed.\n"));
        }
        else if (fShrink)
        {
            TraceTag((tagWarning, "Presented offsceen contents are shrunk."));
        }

        if (!IsTagEnabled(tagMILStepRenderingDisableBreak))
        {
            fBreak = true;
        }
    }


    //
    // Handle locking the surface for debugger dump
    //

    if (
#if DBG
        IsTagEnabled(tagMILStepRenderingLockHW)
#else
        g_fStepHWRenderingLock
#endif
        )
    {
        HRESULT hrLocking;
        RECT rcLock = {
            0, 0,
            static_cast<LONG>(d3dsd.Width), static_cast<LONG>(d3dsd.Height)
        };
        D3DLOCKED_RECT d3dlr = { 0, NULL };

        // Check if target is lockable
        if ((d3dsd.Pool == m_pD3DDevice->GetManagedPool()) ||
            (d3dsd.Pool == D3DPOOL_SYSTEMMEM))
        {
            pID3DLockableSurface = pD3DSurface->ID3DSurface();
            pID3DLockableSurface->AddRef();
            hrLocking = S_OK;
        }
        else
        {
            //
            // Create a lockable copy
            //

            IDirect3DDevice9 *pID3DDevice = NULL;

            MIL_THRX(hrLocking, pD3DSurface->ID3DSurface()->GetDevice(&pID3DDevice));

            if (SUCCEEDED(hrLocking))
            {
                // Not going to guard the allocation with the BEGIN/END
                // macros because this is DBG code only

                MIL_THRX(hrLocking, pID3DDevice->CreateRenderTarget(
                    d3dsd.Width,
                    d3dsd.Height,
                    d3dsd.Format,
                    d3dsd.MultiSampleType,
                    d3dsd.MultiSampleQuality,
                    TRUE,
                    &pID3DLockableSurface,
                    NULL
                    ));

                if (SUCCEEDED(hrLocking))
                {
                    MIL_THRX(hrLocking, pID3DDevice->StretchRect(
                        pD3DSurface->ID3DSurface(),
                        &rcLock,
                        pID3DLockableSurface,
                        &rcLock,
                        D3DTEXF_NONE
                        ));

                    if (FAILED(hrLocking))
                    {
                        pID3DLockableSurface->Release();
                        pID3DLockableSurface = NULL;
                    }
                }

                pID3DDevice->Release();
            }
        }

        if (SUCCEEDED(hrLocking))
        {
            hrLocking = THR(pID3DLockableSurface->LockRect(
                                                        &d3dlr,
                                                        &rcLock,
                                                        D3DLOCK_READONLY));
        }

        if (SUCCEEDED(hrLocking))
        {
            DbgPrintEx(
                g_uDPFltrID,
                DPFLTR_ERROR_LEVEL,
                "Target surface (0x%x x 0x%x) contents at 0x%p, Pitch=0x%x\n",
                d3dsd.Width, d3dsd.Height, d3dlr.pBits, d3dlr.Pitch);
            fBreak = true;
            fUnlock = true;
        }
        else
        {
            DbgPrintEx(
                g_uDPFltrID,
                DPFLTR_ERROR_LEVEL,
                ((!pID3DLockableSurface) ?
                 "No surface available to lock for read.\n" :
                 "Failed to lock surface for read.\n"));
        }
    }

    //
    // Display rendering step description and optionally break
    //

    OutputDebugString(pszRenderDesc);
    OutputDebugString(TEXT(" results are displayed.\n"));

    if (fBreak)
    {
        AvalonDebugBreak();
    }

Cleanup:

    if (pID3DLockableSurface)
    {
        if (fUnlock)
        {
            IGNORE_HR(pID3DLockableSurface->UnlockRect());
        }

        pID3DLockableSurface->Release();
    }

    if (fSaveRestore)
    {
        // Restore the saved area
        if (FAILED(m_pD3DDevice->DbgRestoreSurface(
            m_pD3DTargetSurface,
            rcSave
            )))
        {
            AssertMsg(FALSE,
                      "Unable to restore RT after destructive "
                      "incremental Present.");
        }
    }

    ReleaseInterfaceNoNULL(pD3DSurface);

    m_pD3DDevice->DbgEndStepRenderingPresent();

    IGNORE_HR(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwDisplayRenderTarget::DbgStepCreateD3DSurfaceFromBitmapSource
//
//  Synopsis:  
//      Creates a CD3DSurface in video memory from an IWGXBitmapSource. This
//      method was written for stepped rendering, so it is named as such.
//      Before renaming this function and using it retail, investigate whether
//      there are faster ways of doing this operation.
//

HRESULT
CHwDisplayRenderTarget::DbgStepCreateD3DSurfaceFromBitmapSource(
    __in_ecount(1) IWGXBitmapSource *pBitmap,
    __deref_out_ecount(1) CD3DSurface **ppD3DSurface
    )
{
    HRESULT hr = S_OK;

    CHwVidMemTextureManager hwVidMemManager;
    MilPixelFormat::Enum fmtBitmap;
    D3DLOCKED_RECT d3dLockedRect;
    WICRect rcCopy;
    UINT uBitmapWidth;
    UINT uBitmapHeight;
    UINT uBufferSize;

    IFC(pBitmap->GetSize(
        &uBitmapWidth,
        &uBitmapHeight
        ));

    IFC(pBitmap->GetPixelFormat(
        &fmtBitmap
        ));
    
    Assert(fmtBitmap == MilPixelFormat::PBGRA32bpp);

    hwVidMemManager.SetRealizationParameters(
        m_pD3DDevice,
        D3DFMT_A8R8G8B8,
        uBitmapWidth,
        uBitmapHeight,
        TMML_One
        DBG_COMMA_PARAM(true) // Conditional Non Pow 2 OK
        );

    IFC(hwVidMemManager.ReCreateAndLockSysMemSurface(
        &d3dLockedRect
        ));

    rcCopy.X = 0;
    rcCopy.Y = 0;
    rcCopy.Width = uBitmapWidth;
    rcCopy.Height = uBitmapHeight;

    IFC(HrGetRequiredBufferSize(
        MilPixelFormat::PBGRA32bpp,
        d3dLockedRect.Pitch,
        rcCopy.Width,
        rcCopy.Height,
        &uBufferSize
        ));

    IFC(pBitmap->CopyPixels(
        &rcCopy,
        d3dLockedRect.Pitch,
        uBufferSize,
        static_cast<BYTE*>(d3dLockedRect.pBits)
        ));

    IFC(hwVidMemManager.UnlockSysMemSurface());

    IFC(hwVidMemManager.PushBitsToVidMemTexture());

    IFC(hwVidMemManager.GetVidMemTextureNoRef()->GetD3DSurfaceLevel(
        0,
        ppD3DSurface
        ));

Cleanup:
    RRETURN(hr);
}

#endif DBG_STEP_RENDERING

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::DrawBitmap
//
//  Synopsis:  If rendering is enabled, delegate to base class
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::DrawBitmap(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    AssertNoDeviceEntry(*m_pD3DDevice);

    if (m_fEnableRendering)
    {
        IFC(CHwSurfaceRenderTarget::DrawBitmap(
            pContextState,
            pIBitmap,
            pIEffect
            ));

        DbgSetValidContents();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::DrawMesh3D
//
//  Synopsis:  If rendering is enabled, delegate to base class
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::DrawMesh3D(
    __inout_ecount(1) CContextState* pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __inout_ecount_opt(1) CMILShader *pShader,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    AssertNoDeviceEntry(*m_pD3DDevice);

    if (m_fEnableRendering)
    {
        IFC(CHwSurfaceRenderTarget::DrawMesh3D(
            pContextState,
            pBrushContext,
            pMesh3D,
            pShader,
            pIEffect
            ));

        DbgSetValidContents();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::DrawPath
//
//  Synopsis:  If rendering is enabled, delegate to base class
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::DrawPath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) IShapeData *pShape,
    __inout_ecount_opt(1) CPlainPen *pPen,
    __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
    __inout_ecount_opt(1) CBrushRealizer *pFillBrush
    )
{
    HRESULT hr = S_OK;

    AssertNoDeviceEntry(*m_pD3DDevice);

    if (m_fEnableRendering)
    {
        IFC(CHwSurfaceRenderTarget::DrawPath(
            pContextState,
            pBrushContext,
            pShape,
            pPen,
            pStrokeBrush,
            pFillBrush
            ));

        DbgSetValidContents();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::DrawInfinitePath
//
//  Synopsis:  If rendering is enabled, delegate to base class
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::DrawInfinitePath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) BrushContext *pBrushContext,
    __inout_ecount(1) CBrushRealizer *pFillBrush
    )
{
    HRESULT hr = S_OK;

    AssertNoDeviceEntry(*m_pD3DDevice);

    if (m_fEnableRendering)
    {
        IFC(CHwSurfaceRenderTarget::DrawInfinitePath(
            pContextState,
            pBrushContext,
            pFillBrush
            ));

        DbgSetValidContents();
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::DrawGlyphs
//
//  Synopsis:  If rendering is enabled, delegate to base class
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::DrawGlyphs(
    __inout_ecount(1) DrawGlyphsParameters &pars
    )
{
    HRESULT hr = S_OK;

    AssertNoDeviceEntry(*m_pD3DDevice);

    if (m_fEnableRendering)
    {
        IFC(CHwSurfaceRenderTarget::DrawGlyphs(pars));

        DbgSetValidContents();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::DrawVideo
//
//  Synopsis:  If rendering is enabled, check if we can draw directly to
//             the backbuffer, otherwise delegate to base class
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::DrawVideo(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __inout_ecount_opt(1) IWGXBitmapSource *pBitmapSource,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    AssertNoDeviceEntry(*m_pD3DDevice);

    if (m_fEnableRendering)
    {
        IFC(CHwSurfaceRenderTarget::DrawVideo(
            pContextState,
            pSurfaceRenderer,
            pBitmapSource,
            pIEffect));

        DbgSetValidContents();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::WaitForVBlank
//
//  Synopsis: Wait for the display to enter vblank
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwDisplayRenderTarget::WaitForVBlank(
    )
{
    BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
        WGXERR_NO_HARDWARE_DEVICE
    END_MILINSTRUMENTATION_HRESULT_LIST

    HRESULT hr = WGXERR_NO_HARDWARE_DEVICE;

    if (SUCCEEDED(m_hrDisplayInvalid) && m_pD3DDevice)
    {
        ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

        // Wait for vblank on the monitor containing
        // swap chain 0.
        //  Need to determine the swap chain to use instead of guessing chain 0
        MIL_THR(m_pD3DDevice->WaitForVBlank(0));

    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwDisplayRenderTarget::AdvanceFrame
//
//  Synopsis: Advance frame counter.
//
//-------------------------------------------------------------------------
STDMETHODIMP_(VOID)
CHwDisplayRenderTarget::AdvanceFrame(
    UINT uFrameNumber
    )
{
    if (SUCCEEDED(m_hrDisplayInvalid) && m_pD3DDevice)
    {
        ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);
        
        m_pD3DDevice->AdvanceFrame(uFrameNumber);
    }
}



