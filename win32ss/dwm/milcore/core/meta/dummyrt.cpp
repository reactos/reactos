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
//      CDummyRenderTarget implementation
//
//      This is a dummy Render Target for rendering to nothing.  It simply
//      consumes as many calls as it can and returns default information when it
//      has to to look like a normal render target.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


// Class to allow instantiation of a CDummyRenderTarget
class CDummyRenderTargetInstance : public CDummyRenderTarget {};

// Static instance of the dummy rt
static CDummyRenderTargetInstance g_dummyRT;

// Reference to static instance of the dummy rt
CDummyRenderTarget * const CDummyRenderTarget::sc_pDummyRT = &g_dummyRT;


//=============================================================================

CDummyRenderTarget::CDummyRenderTarget()
{
    m_matDeviceTransform.SetToIdentity();
    // m_matDeviceTransform is initialized to identity, so we only
    // need to set _11 and _22
    m_matDeviceTransform._11 = 96.0f;
    m_matDeviceTransform._22 = 96.0f;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CDummyRenderTarget AddRef routine.  Note that since there is only ever
//      one static instance, we don't ref-count the object.
//
//------------------------------------------------------------------------------
ULONG
CDummyRenderTarget::AddRef()
{
    return 1;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CDummyRenderTarget Release routine.  Note that since there is only ever
//      one static instance, we don't ref-count the object.
//
//------------------------------------------------------------------------------
ULONG
CDummyRenderTarget::Release()
{
    return 0;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CDummyRenderTarget QI routine
//
//  Arguments:
//      riid - input IID.
//      ppvObject - output interface pointer.
//
//------------------------------------------------------------------------------

STDMETHODIMP CDummyRenderTarget::QueryInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = S_OK;

    if (ppvObject)
    {
        *ppvObject = NULL;

        // No need to AddRef as this class is not reference counted,
        // but should just be static.
        if (riid == IID_IUnknown)
        {
            // We can resolve this particular ambiguity by picking any path to IUnknown
            // because in this case they all resolve to the same implementation.  So
            // we go through IMILRenderTargetHWND.
            *ppvObject = static_cast<IUnknown*>(static_cast<IMILRenderTargetHWND*>(this));
        }
        else if (riid == IID_IMILRenderTarget)
        {
            // We can resolve this particular ambiguity by picking any path to IMILRenderTarget
            // because in this case they all resolve to the same implementation.  So
            // we go through IMILRenderTargetHWND.
            *ppvObject = static_cast<IMILRenderTarget*>(static_cast<IMILRenderTargetHWND*>(this));
        }
        else if (riid == IID_IRenderTargetInternal)
        {
            *ppvObject = static_cast<IRenderTargetInternal*>(this);
        }
        else if (riid == IID_IMILRenderTargetBitmap)
        {
            *ppvObject = static_cast<IMILRenderTargetBitmap*>(this);
        }
        else if (riid == IID_IMILRenderTargetHWND)
        {
            *ppvObject = static_cast<IMILRenderTargetHWND*>(this);
        }
        else if (riid == IID_IWGXBitmapSource)
        {
            *ppvObject = static_cast<IWGXBitmapSource*>(this);
        }
        else
        {
            hr = THR(E_NOINTERFACE);
        }
    }
    else
    {
        hr = THR(E_INVALIDARG);
    }

    #pragma prefast(suppress: __WARNING_QI_SUCCESS_NO_ADDREF, "CDummyRenderTarget is a dummy singleton w/o ref counting")
    return hr;
}


//+========================================================================
// IMILRenderTarget methods
//=========================================================================

//=============================================================================

STDMETHODIMP_(VOID)
CDummyRenderTarget::GetBounds(
    __out_ecount(1) MilRectF * const pBounds
    )
{
    // Return dummy bounds here...
    *pBounds = CMilRectF::sc_rcEmpty;

    return;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    UNREFERENCED_PARAMETER(pColor);
    UNREFERENCED_PARAMETER(pAliasedClip);

    return S_OK;
}


//=============================================================================

STDMETHODIMP
CDummyRenderTarget::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    UNREFERENCED_PARAMETER(rcBounds);
    UNREFERENCED_PARAMETER(AntiAliasMode);
    UNREFERENCED_PARAMETER(fUseZBuffer);
    UNREFERENCED_PARAMETER(rZ);

    return S_OK;
}

STDMETHODIMP
CDummyRenderTarget::End3D(
    )
{
    return S_OK;
}


//+========================================================================
// IRenderTargetInternal methods
//=========================================================================

//=============================================================================

STDMETHODIMP_(__outro_ecount(1) const CMILMatrix*)
CDummyRenderTarget::GetDeviceTransform() const
{
    return &m_matDeviceTransform;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::DrawBitmap(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::DrawMesh3D(
    __inout_ecount(1) CContextState* pContextState,
    __inout_ecount_opt(1) BrushContext* pBrushContext,
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __inout_ecount_opt(1) CMILShader *pShader,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::DrawPath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) IShapeData *pShape,
    __inout_ecount_opt(1) CPlainPen *pPen,
    __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
    __inout_ecount_opt(1) CBrushRealizer *pFillBrush
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::DrawInfinitePath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) BrushContext *pBrushContext,
    __inout_ecount(1) CBrushRealizer *pFillBrush
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP 
CDummyRenderTarget::ComposeEffect(
    __inout_ecount(1) CContextState *pContextState,
    __in_ecount(1) CMILMatrix *pScaleTransform,
    __inout_ecount(1) CMilEffectDuce* pEffect,
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt IMILRenderTargetBitmap* pImplicitInput
    ) 
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::DrawGlyphs(DrawGlyphsParameters &pars)
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::DrawVideo(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __inout_ecount(1) IWGXBitmapSource *pBitmapSource,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::CreateRenderTargetBitmap(
    UINT width,
    UINT height,
    IntermediateRTUsage usageInfo,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
    __in_opt DynArray<bool> const *pActiveDisplays
    )
{
    HRESULT hr = S_OK;

    *ppIRenderTargetBitmap = this;
    // No need to AddRef as this class is not reference counted,
    // but should just be static.

    RRETURN(hr);
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::BeginLayer(
    __in_ecount(1) MilRectF const &LayerBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    __in_ecount_opt(1) IShapeData const *pGeometricMask,
    __in_ecount_opt(1) CMILMatrix const *pGeometricMaskToTarget,
    FLOAT flAlphaScale,
    __in_ecount_opt(1) CBrushRealizer *pAlphaMask
    )
{
    UNREFERENCED_PARAMETER(LayerBounds);
    UNREFERENCED_PARAMETER(AntiAliasMode);
    UNREFERENCED_PARAMETER(pGeometricMask);
    UNREFERENCED_PARAMETER(pGeometricMaskToTarget);
    UNREFERENCED_PARAMETER(flAlphaScale);
    UNREFERENCED_PARAMETER(pAlphaMask);

    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::EndLayer(
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP_(void) CDummyRenderTarget::EndAndIgnoreAllLayers(
    )
{
    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDummyRenderTarget::ReadEnabledDisplays
//
//  Synopsis:
//      Consume call.
//
//------------------------------------------------------------------------------

STDMETHODIMP
CDummyRenderTarget::ReadEnabledDisplays (
    __inout DynArray<bool> *pEnabledDisplays
    )
{
    UNREFERENCED_PARAMETER(pEnabledDisplays);

    RRETURN(S_OK);
}

//=============================================================================

UINT
CDummyRenderTarget::GetRealizationCacheIndex()
{
    RIP("Currently unused.");

    return CMILResourceCache::InvalidToken;
}

//+========================================================================
// IMILRenderTargetBitmap methods
//=========================================================================

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::GetBitmapSource(
    __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
    )
{
    HRESULT hr = S_OK;

    *ppIBitmapSource = this;
    // No need to AddRef as this class is not reference counted,
    // but should just be static.

    RRETURN(hr);
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::GetCacheableBitmapSource(
    __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
    )
{
    INLINED_RRETURN(GetBitmapSource(ppIBitmapSource));   
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::GetBitmap(
    __deref_out_ecount(1) IWGXBitmap ** const ppIBitmap
    )
{
    RRETURN(WGXERR_NOTIMPLEMENTED);
}


//+========================================================================
// IWGXBitmapSource methods
//=========================================================================

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::GetSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    )
{
    HRESULT hr = S_OK;

    *puWidth = 1;
    *puHeight = 1;

    RRETURN(hr);
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
    )
{
    RRETURN(E_ACCESSDENIED);
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::GetResolution(
    __out_ecount(1) double *pDpiX,
    __out_ecount(1) double *pDpiY
    )
{
    RRETURN(E_ACCESSDENIED);
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::CopyPalette(
    __inout_ecount(1) IWICPalette *pIPalette
    )
{
    RRETURN(E_ACCESSDENIED);
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::CopyPixels(
    __in_ecount_opt(1) const MILRect *prc,
    __in UINT cbStride,
    __in UINT cbBufferSize,
    __out_ecount(cbBufferSize) BYTE *pvPixels
    )
{
    RRETURN(E_ACCESSDENIED);
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::Present(
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::ScrollBlt(
    __in_ecount(1) const RECT *prcSource,
    __in_ecount(1) const RECT *prcDest
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::Invalidate(
    __in_ecount_opt(1) MilRectF const *prc
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::SetPosition(
    __in_ecount(1) MilRectF const *prc
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::GetInvalidRegions(
    __deref_outro_ecount(*pNumRegions) MilRectF const ** const prgRegions,
    __out_ecount(1) UINT *pNumRegions,
    __out bool *fWholeTargetInvalid    
    )
{
    *prgRegions = NULL;
    *pNumRegions = 0;
    *fWholeTargetInvalid = false;
    return S_OK;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::UpdatePresentProperties(
    MilTransparency::Flags transparencyFlags,
    FLOAT constantAlpha,
    __in_ecount(1) MilColorF const &colorKey
    )
{
    return S_OK;
}

//=============================================================================

STDMETHODIMP_(VOID)
CDummyRenderTarget::GetIntersectionWithDisplay(
    UINT iDisplay,
    __out_ecount(1) MilRectL &rcIntersection
    )
{
    rcIntersection = CMilRectL::sc_rcEmpty;
    return;
}

//=============================================================================

STDMETHODIMP
CDummyRenderTarget::WaitForVBlank(
    )
{
    RRETURN(WGXERR_NO_HARDWARE_DEVICE);
}

//=============================================================================

STDMETHODIMP_(VOID)
CDummyRenderTarget::AdvanceFrame(UINT uFrameNumber)
{
}

//=============================================================================

HRESULT
CDummyRenderTarget::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    *puNumQueuedPresents = 0;

    return S_OK;
}

//=============================================================================

STDMETHODIMP_(bool)
CDummyRenderTarget::CanReuseForThisFrame(
    __in_ecount(1) IRenderTargetInternal* pIRTParent
    )
{
    //
    // The dummy render target doesn't have any content, so it cannot render to
    // anything other than another dummy render target. Since we cannot
    // determine here whether the parent is a dummy or not, we return "false"
    // to indicate that this render target should not be used between frames.
    // After all, the parent may need a non-dummy on the next frame.
    //
    return false;
}

//=============================================================================

STDMETHODIMP 
CDummyRenderTarget::CanAccelerateScroll(
    __out_ecount(1) bool *fCanAccelerateScroll
    )
{
    *fCanAccelerateScroll = false;
    RRETURN(S_OK);
}



