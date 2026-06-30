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
//      Software Render Target (RT) for screen rendering. This RT is always
//      software rasterized.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CSwRenderTargetHWND, MILRender, "CSwRenderTargetHWND");

MtDefine(MSwInvalidRegion, MILRawMemory, "MSwInvalidRegion");

DeclareTag(tagMILLogDirtyRects, "CSwRenderTargetHWND", "Log the dirty rects");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenterBase::CSwPresenterBase
//
//------------------------------------------------------------------------------

CSwPresenterBase::CSwPresenterBase(
    MilPixelFormat::Enum fmt
    ) :
    m_pLock(NULL),
    m_fLocked(FALSE),
    m_nWidth(0),
    m_nHeight(0),
    m_RenderPixelFormat(fmt)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenterBase::GetSize
//
//------------------------------------------------------------------------------

HRESULT CSwPresenterBase::GetSize(
    __out_ecount(1) UINT *puiWidth,
    __out_ecount(1) UINT *puiHeight
    )
{
    *puiWidth = m_nWidth;
    *puiHeight = m_nHeight;
    
    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenterBase::GetPixelFormat
//
//------------------------------------------------------------------------------

HRESULT CSwPresenterBase::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
    )
{   
    *pPixelFormat = m_RenderPixelFormat;

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenterBase::GetResolution
//
//------------------------------------------------------------------------------

HRESULT CSwPresenterBase::GetResolution(
    __out_ecount(1) double *pDpiX,
    __out_ecount(1) double *pDpiY
    )
{
    const auto& primaryDisplayDpi = DpiScale::PrimaryDisplayDpi();

    *pDpiX = primaryDisplayDpi.DpiScaleX;
    *pDpiY = primaryDisplayDpi.DpiScaleY;
    
    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenterBase::CopyPalette
//
//------------------------------------------------------------------------------

HRESULT CSwPresenterBase::CopyPalette(
    __inout_ecount(1) IWICPalette *pIPalette
    )
{
    RRETURN(E_FAIL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenterBase::CopyPixels
//
//------------------------------------------------------------------------------

HRESULT CSwPresenterBase::CopyPixels(
    __in_ecount_opt(1) const MILRect *prc,
    UINT cbStride,
    UINT cbBufferSize,
    __out_ecount(cbBufferSize) BYTE *pvPixels
    )
{
    RRETURN(E_FAIL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenterBase::AddDirtyRect
//
//------------------------------------------------------------------------------

HRESULT CSwPresenterBase::AddDirtyRect(
    __in_ecount(1) const RECT *prcDirtyRect
    )
{
    RRETURN(E_FAIL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenterBase::SetPalette
//
//------------------------------------------------------------------------------

HRESULT CSwPresenterBase::SetPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    RRETURN(E_FAIL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenterBase::SetResolution
//
//------------------------------------------------------------------------------

HRESULT CSwPresenterBase::SetResolution(
    double dpiX,
    double dpiY
    )
{
    RRETURN(E_FAIL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::HrFindInterface
//
//  Synopsis:
//      QI helper routine
//

STDMETHODIMP CSwRenderTargetHWND::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    // HWND classes are protected by CMetaRenderTarget and never
    // need to be QI'ed, therefore never needing to call HrFindInterface
    AssertMsg(false, "CSwRenderTargetHWND is not allowed to be QI'ed.");
    RRETURN(E_NOINTERFACE);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::CSwRenderTargetHWND
//
//  Synopsis:
//      Ensures that the object is constructed in a consistent state.
//

CSwRenderTargetHWND::CSwRenderTargetHWND(
    DisplayId associatedDisplay
    ) :
    CSwRenderTargetSurface(associatedDisplay)
{
    m_hwnd = NULL;
    m_pPresenter = NULL;

    if (g_pMediaControl)
    {
        InterlockedIncrement(reinterpret_cast<LONG *>(
            &(g_pMediaControl->GetDataPtr()->NumSoftwareRenderTargets)
            ));
    }

#if DBG_STEP_RENDERING
    // Set the parent to be itself, not ref counted of course
    m_pDisplayRTParent = this;
#endif DBG_STEP_RENDERING
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::~CSwRenderTargetHWND
//
//  Synopsis:
//      Release the internal RT and the offscreen.
//

CSwRenderTargetHWND::~CSwRenderTargetHWND()
{
    if (m_pPresenter != NULL)
    {
        m_pPresenter->Release();
    }

    if (g_pMediaControl)
    {
        InterlockedDecrement(reinterpret_cast<LONG *>(
            &(g_pMediaControl->GetDataPtr()->NumSoftwareRenderTargets)
            ));
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::Create
//
//  Synopsis:
//      Creates the render target with it's HWND. This constructs the internal
//      RT and back buffer.
//

HRESULT CSwRenderTargetHWND::Create(
    __in_opt HWND hwnd,
    MilWindowLayerType::Enum eWindowLayerType,
    __in_ecount(1) CDisplay const *pIdealDisplay,
    DisplayId associatedDisplay,
    UINT uWidth,
    UINT uHeight,
    MilRTInitialization::Flags nFlags,
    __deref_out_ecount(1) CSwRenderTargetHWND **ppRT
    )
{
    HRESULT hr = S_OK;

    Assert(ppRT);

    // Must have a valid HWND.
    Assert(hwnd != NULL);

    // Allocate object
    *ppRT = new CSwRenderTargetHWND(associatedDisplay);
    IFCOOM(*ppRT);
    (*ppRT)->AddRef();  // CSwRenderTargetHWND::ctor set ref count == 0

    IFC((*ppRT)->Init(
        hwnd,
        eWindowLayerType,
        pIdealDisplay,
        nFlags
        ));

    IFC((*ppRT)->Resize(uWidth, uHeight));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppRT);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::Init
//
//------------------------------------------------------------------------------

HRESULT CSwRenderTargetHWND::Init(
    __in_opt HWND hwnd,
    MilWindowLayerType::Enum eWindowLayerType,
    __in_ecount(1) CDisplay const *pIdealDisplay,
    MilRTInitialization::Flags nFlags
    )
{

    HRESULT hr = S_OK;

    // Initialize presenter
    m_pPresenter = new CSwPresenter32bppGDI(
        pIdealDisplay,
         (nFlags & MilRTInitialization::NeedDestinationAlpha ?
          MilPixelFormat::PBGRA32bpp :
          MilPixelFormat::BGR32bpp)
        );

    IFCOOM(m_pPresenter);

    m_pPresenter->AddRef();

    m_pPresenter->Init(
        hwnd,
        eWindowLayerType,
        nFlags
        );

    m_hwnd = hwnd;

#if DBG_STEP_RENDERING
    m_fDbgClearOnPresent = !(nFlags & MilRTInitialization::PresentRetainContents);
#endif DBG_STEP_RENDERING

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::Present
//
//  Synopsis:
//      Call GDI to Blt the bits to the display.
//

STDMETHODIMP CSwRenderTargetHWND::Present(
    __in_ecount(1) const RECT *pRect
    )
{
    Assert(m_pPresenter != NULL);
    Assert(m_LayerStack.GetCount() == 0);
    DbgAssertBoundsState();

    HRESULT hr = S_OK;

    CMILSurfaceRect presentRect;

    bool fPresent = false;
    
    RGNDATA *pDirtyRegion = NULL;

    IFC(ShouldPresent(
        pRect,
        &presentRect,
        &pDirtyRegion,
        &fPresent
        ));
    
    if (fPresent)
    {
        IFC(m_pPresenter->Present(
            &presentRect,
            &presentRect,
            pDirtyRegion
            ));
    }

#if DBG_STEP_RENDERING
    //
    // When retain contents was not specified for in the
    // creation flags, clear the back buffer in debug mode
    // to alternating colors so that any areas not properly
    // redrawn before the next present will be easily
    // identified.
    //
    // NOTE: It is important that this take place before the dirty rect is
    // reset because Clear will mark the entire surface as dirty.
    //

    if (m_fDbgClearOnPresent)
    {
        static bool fGreen = false;

        CMilColorF green(0.0,1.0,0.0,1.0);
        CMilColorF purple(1.0,0.0,0.5,1.0);

        VerifySUCCEEDED(Clear(fGreen ? &green: &purple, NULL));

        fGreen = !fGreen;
    }
#endif DBG_STEP_RENDERING

Cleanup:

    //
    // Reset dirty rect (even on failure)
    //

    IGNORE_HR(ClearInvalidatedRects());

    RRETURN(hr);
}

STDMETHODIMP
CSwRenderTargetHWND::ScrollBlt (
    __in_ecount(1) const RECT *prcSource,
    __in_ecount(1) const RECT *prcDest
    )
{
    Assert(m_pPresenter != NULL);
    Assert(m_LayerStack.GetCount() == 0);
    DbgAssertBoundsState();

    HRESULT hr = S_OK;

    CMILSurfaceRect source(*prcSource);
    CMILSurfaceRect dest(*prcDest);

    IFC(m_pPresenter->ScrollBlt(&source, &dest, true, true));

Cleanup:
    RRETURN(hr);    
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::SetPosition
//
//  Synopsis:
//      Remember Present position for when UpdateLayeredWindowEx is called.
//
//------------------------------------------------------------------------------

void
CSwRenderTargetHWND::SetPosition(POINT ptOrigin)
{
    m_pPresenter->SetPosition(ptOrigin);

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::UpdatePresentProperties
//
//  Synopsis:
//      Remember Present transparency properties for when UpdateLayeredWindowEx
//      is called.
//
//------------------------------------------------------------------------------

void
CSwRenderTargetHWND::UpdatePresentProperties(
    MilTransparency::Flags transparencyFlags,
    BYTE constantAlpha,
    COLORREF colorKey
    )
{
    m_pPresenter->UpdatePresentProperties(
        transparencyFlags,
        constantAlpha,
        colorKey
        );

    return;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::Resize
//
//  Synopsis:
//      This routine ensures that the DIB backing buffer is properly created for
//      the specified size.  If it's wrong, it'll be recreated - otherwise this
//      routine is a NOP. It also initializes the internal RT surface.
//

STDMETHODIMP CSwRenderTargetHWND::Resize(
    UINT uWidth,
    UINT uHeight
    )
{
    Assert(m_pPresenter);

    HRESULT hr = S_OK;

    if (uWidth == 0 || uHeight == 0)
    {
        m_pPresenter->FreeResources();
    }
    else
    {
        IFC(m_pPresenter->Resize(uWidth, uHeight));

        IFC(CSwRenderTargetSurface::SetSurface(
            m_pPresenter
            ));
    }

Cleanup:
    RRETURN(hr);
}

#if DBG_STEP_RENDERING
//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::ShowSteppedRendering
//
//  Synopsis:
//      Present the current backbuffer when enabled in debug builds
//
//------------------------------------------------------------------------------
#if !DBG
volatile BOOL g_fStepSWRendering = false;
#endif

void CSwRenderTargetHWND::ShowSteppedRendering(
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

    IWGXBitmap *pSurfaceBitmapNoRef = NULL;
    pRT->DbgGetSurfaceBitmapNoRef(
        &pSurfaceBitmapNoRef
        );

    if (pSurfaceBitmapNoRef != m_pIInternalSurface)
    {
        //
        // Future Consideration:   Handle stepped rendering in a SW surface displayed on a SW display RT
        //
        // See hwdisplayrt.cpp for an example of how do to this. Steps needed:
        // 1. Determine how much area of the SW surface can be displayed
        // 2. Save the contents of the display RT that we are about to change
        // 3. Copy the contents from the SW surface to the display RT
        // 3. (Present)
        // 4. Restore the contents of the display RT that were changed
        //
        OutputDebugString(TEXT("Missing stepped rendering feature prevents display of "));
        OutputDebugString(pszRenderDesc);
        OutputDebugString(TEXT("\n"));
        return;
    }

    CMILSurfaceRect rcSource(
        0,
        0,
        m_uWidth,
        m_uHeight,
        XYWH_Parameters
        );

    BOOL fOrgDbgClearOnPresent = m_fDbgClearOnPresent;
    m_fDbgClearOnPresent = FALSE;

    if (FAILED(m_pPresenter->Present(
            &rcSource,
            &rcSource,
            NULL
            )))
    {
        TraceTag((tagWarning, "Incremental Present failed."));
    }

    m_fDbgClearOnPresent = fOrgDbgClearOnPresent;

    OutputDebugString(pszRenderDesc);
    OutputDebugString(TEXT(" results are displayed.\n"));

    if (!IsTagEnabled(tagMILStepRenderingDisableBreak))
    {
        AvalonDebugBreak();
    }
}
#endif DBG_STEP_RENDERING

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::WaitForVBlank
//
//  Synopsis:
//      Fake wait always fails because we have no device
//
//  Returns:
//      returns D3DERR_NOTAVAILABLE
//
//------------------------------------------------------------------------------
STDMETHODIMP CSwRenderTargetHWND::WaitForVBlank()
{
    RRETURN(WGXERR_NO_HARDWARE_DEVICE);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetHWND::AdvanceFrame
//
//  Synopsis:
//      Nothing required, SW doesn't track this.
//
//------------------------------------------------------------------------------
STDMETHODIMP_(VOID) CSwRenderTargetHWND::AdvanceFrame(
    UINT uFrameNumber
    )
{
}




