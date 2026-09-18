// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#include "effects\effectlist.h"


MtDefine(CSwRenderTargetSurface, MILRender, "CSwRenderTargetSurface");
MtDefine(CSwRenderTargetBitmap, MILRender, "CSwRenderTargetBitmap");
MtDefine(MSwRenderTargetScanlineBuffers, MILRawMemory, "MSwRenderTargetScanlineBuffers");


CSwRenderTargetSurface::CSwRenderTargetSurface(
    DisplayId associatedDisplay
    ) :
    CBaseSurfaceRenderTarget<CSwRenderTargetLayerData>(associatedDisplay)
{
    m_pIInternalSurface = NULL;
    m_pILock = NULL;
    m_pvBuffer = NULL;
    m_pHw3DRT = NULL;

#if DBG_ANALYSIS
    m_fDbgBetweenBeginAndEnd3D = false;
#endif
}

void CSwRenderTargetSurface::CleanUp(
    BOOL fRelease3DRT
    )
{
    ReleaseInterface(m_pIInternalSurface);

    m_IntermediateBuffers.FreeBuffers();

    //
    // The 3D RT supports resizing, so we don't always need to release it.
    //
    if (fRelease3DRT)
    {
        ReleaseInterface(m_pHw3DRT);
    }

    Assert(m_pILock == NULL);
    Assert(m_pvBuffer == NULL);

    m_pILock = NULL;
    m_pvBuffer = NULL;
}

CSwRenderTargetSurface::~CSwRenderTargetSurface()
{
    CleanUp(TRUE);
}

HRESULT
CSwRenderTargetSurface::SetSurface(
    __in_ecount(1) IWGXBitmap *pISurface
    )
{
    HRESULT hr = S_OK;

    CleanUp(FALSE);

    m_pIInternalSurface = pISurface;
    m_pIInternalSurface->AddRef();

    m_resizeUniqueness.UpdateUniqueCount();

    MilPixelFormat::Enum fmtSurface;
    ColorSpace csSurface;

    IFC(m_pIInternalSurface->GetSize(&m_uWidth, &m_uHeight));

    IFC(m_pIInternalSurface->GetPixelFormat(&fmtSurface));
    IFC(GetPixelFormatColorSpace(fmtSurface, &csSurface));

    if (!IsRenderingPixelFormat(fmtSurface))
    {
        IFC(E_INVALIDARG);
    }

    if (csSurface == CS_scRGB)
    {
        m_sr.SetColorDataPixelFormat(MilPixelFormat::PRGBA128bppFloat);
    }
    else
    {
        Assert(csSurface == CS_sRGB);

        m_sr.SetColorDataPixelFormat(MilPixelFormat::PBGRA32bpp);
    }

    {
        FLOAT rxDPI, ryDPI;
        double dblDpiX = 0.0;
        double dblDpiY = 0.0;

        // Get the surface resolution so we can appropriately build the
        // Page Space to Device Space transform.

        IFC(m_pIInternalSurface->GetResolution(&dblDpiX, &dblDpiY));

        rxDPI = static_cast<FLOAT>(dblDpiX);
        ryDPI = static_cast<FLOAT>(dblDpiY);

        // NOTE: Assuming Page-Space is in inches.

        m_DeviceTransform.SetToIdentity();
        m_DeviceTransform.Scale(rxDPI, ryDPI);
    }

    // compute the size of one pixel in bytes.

    m_cbPixel = GetPixelFormatSize(fmtSurface) >> 3;

    IFC(m_IntermediateBuffers.AllocateBuffers(
        Mt(MSwRenderTargetScanlineBuffers),
        m_uWidth
        ));

    // Record the surface format for SetupPipeline

    m_fmtTarget = fmtSurface;

    IFC(CBaseRenderTarget::Init());

    if (m_pHw3DRT)
    {
        IFC(m_pHw3DRT->Resize(
            m_uWidth,
            m_uHeight
            ));
    }

Cleanup:
    if (FAILED(hr))
    {
        CleanUp(TRUE);
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::LockInternalSurface
//
//  Synopsis:
//      Handle common practice of locking internal surface for rendering
//

HRESULT
CSwRenderTargetSurface::LockInternalSurface(
    __in_ecount_opt(1) const WICRect *pRect,
    DWORD dwLockFlags
    )
{
    HRESULT hr = S_OK;

    UINT cbBufferSize = 0;
    const MILRect rcFull = { 0, 0, m_uWidth, m_uHeight };

    Assert(m_pIInternalSurface);
    Assert(m_pILock == NULL);
    Assert(m_pvBuffer == NULL);     // Not required, but expected

    if (!pRect)
    {
        pRect = &rcFull;
    }
    IFC(m_pIInternalSurface->Lock(
        pRect,
        dwLockFlags,
        &m_pILock
        ));

#if DBG
    // We assume that the width and height of the surface do not change.
    // One can view this as an interface rule for IWGXBitmap.

    // If they had changed, then m_pDefaultClipper, for one, would be stale.

    UINT nWidth, nHeight;
    Assert(SUCCEEDED(m_pILock->GetSize(&nWidth, &nHeight)));
    Assert(m_uWidth == nWidth);
    Assert(m_uHeight == nHeight);
#endif

    IFC(m_pILock->GetStride(&m_cbStride));

    IFC(m_pILock->GetDataPointer(&cbBufferSize, reinterpret_cast<BYTE **>(&m_pvBuffer)));

Cleanup:
    if (FAILED(hr))
    {
        UnlockInternalSurface();
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::UnlockInternalSurface
//
//  Synopsis:
//      Release internal surface lock obtained by LockInternalSurface
//
//  Note:
//      Safe to call even if LockInternalSurface fails.
//

void
CSwRenderTargetSurface::UnlockInternalSurface(
    )
{
    m_pvBuffer = NULL;
    ReleaseInterface(m_pILock);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::Clear
//
//  Synopsis:
//      Clear the entire bitmap to the given solid (non-premultiplied) color.
//
//      Some 32bpp formats are 'accelerated' by using memset. For other
//      destination pixel formats, we use the CSoftwareRasterizer::Clear.
//

STDMETHODIMP CSwRenderTargetSurface::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    HRESULT hr = S_OK;

    if (pColor == NULL)
    {
        goto Cleanup;
    }

    // Lock the internal surface so that we can clear the pixels.

    IFC(LockInternalSurface(
        NULL,
        MilBitmapLock::Write
        ));

    IFC(ClearLockedSurface(pColor, pAliasedClip));

Cleanup:
    // Unlock internal surface (despite any other failures)
    UnlockInternalSurface();

    // SW_DBG_RENDERING_STEP must happen after UnlockInternalSurface
    SW_DBG_RENDERING_STEP(Clear);

    RRETURN(hr);
}

HRESULT CSwRenderTargetSurface::ClearLockedSurface(
    __in_ecount(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    HRESULT hr = S_OK;

    Assert(NULL != m_pILock);
    Assert(NULL != m_pvBuffer);

    bool fClearCompleted = false;
    CMILSurfaceRect rcClip;
    if (!IntersectCAliasedClipWithSurfaceRect(pAliasedClip,
                                              m_rcBounds,
                                              OUT &rcClip
                                              ))
    {
        fClearCompleted = true;
    }
    else
    {
        // Guard that rcClip is within the surface bounds
        //
        // This should always be true because of the intersection, but if it
        // wasn't (e.g., because of a bug in the intersection routine), we would
        // be writing to unowned memory.
        Assert( (rcClip.left >= m_rcBounds.left) &&
                (rcClip.top >= m_rcBounds.top) &&
                (rcClip.right <= m_rcBounds.right) &&
                (rcClip.bottom <= m_rcBounds.bottom)
                );

        MilPointAndSizeL rcClipWH = {
            rcClip.left,
            rcClip.top,
            rcClip.right - rcClip.left,
            rcClip.bottom - rcClip.top
        };

        MilColorF colorF = *pColor;

        switch (m_fmtTarget)
        {
        case MilPixelFormat::PBGRA32bpp:
        case MilPixelFormat::BGRA32bpp:
        case MilPixelFormat::BGR32bpp:
            {
                ARGB argb;

                if (m_fmtTarget == MilPixelFormat::PBGRA32bpp)
                {

                    argb = Convert_MilColorF_scRGB_To_Premultiplied_MilColorB_sRGB(pColor);
                }
                else
                {
                    argb = Convert_MilColorF_scRGB_To_MilColorB_sRGB(pColor);
                }

                BYTE *pbScan =
                    static_cast<BYTE *>(m_pvBuffer) +
                    static_cast<INT_PTR>(rcClip.top) * m_cbStride +
                    static_cast<INT_PTR>(rcClip.left) * m_cbPixel;

                INT Width = rcClip.right - rcClip.left;

                for (INT h = rcClip.top; h < rcClip.bottom; h++)
                {
                    FillMemoryInt32(pbScan, Width, argb);
                    pbScan += m_cbStride;
                }

                //
                // Make sure display RTs know bits have been touched
                //

                AddDirtyRect(&rcClipWH);

                fClearCompleted = true;
            }
            break;

        case MilPixelFormat::PRGBA128bppFloat:
            Premultiply(&colorF);
            __fallthrough;  // intentional fall through

        case MilPixelFormat::RGB128bppFloat:
        case MilPixelFormat::RGBA128bppFloat:
            RIP("128 bit pixel formats should never come up in software");
            break;

        default:
            {
                // Use CSoftwareRasterizer for more complex pixel formats.
            }
            break;
        }
    }

    //
    // Check if we still need to handle clear case which means we have complex
    // clipping or a complex pixel format.
    //

    if (!fClearCompleted)
    {
        CRectClipper Clipper;

        Clipper.SetClip(rcClip);

        MIL_THR(m_sr.Clear(
            this,
            &Clipper,
            pColor
            ));
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::Begin3D
//
//  Synopsis:
//      Prepare for 3D scene within bounds given and clear Z to given value
//

STDMETHODIMP
CSwRenderTargetSurface::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    UNREFERENCED_PARAMETER(AntiAliasMode);
    bool f3DCapable = true;

    HRESULT hr = S_OK;

    CMILSurfaceRect rc3DBounds;

    IFC(LockInternalSurface(
        NULL,
        MilBitmapLock::Write | MilBitmapLock::Read
        ));

    if (!m_pHw3DRT)
    {
        MIL_THR(CHw3DSoftwareSurface::Create(
            m_fmtTarget,
            m_associatedDisplay,
            m_uWidth,
            m_uHeight,
            &m_pHw3DRT
            ));
        if (FAILED(hr))
        {
            if (   hr == D3DERR_NOTAVAILABLE
                || hr == D3DERR_NOTFOUND)
            {
                //
                // When we can't create a software surface, we can't draw 3D. We
                // will eat this error later in this function and then consume
                // calls to DrawMesh3D and End3D
                //
                f3DCapable = false;
            }
            
            m_pHw3DRT = NULL;
            goto Cleanup;
        }
    }

    IntersectAliasedBoundsRectFWithSurfaceRect(
        rcBounds,
        m_rcBounds,
        &rc3DBounds
        );

    IFC(m_pHw3DRT->BeginSw3D(
            m_pvBuffer,
            DBG_ANALYSIS_PARAM_COMMA(m_uHeight*m_cbStride)
            m_cbStride,
            rc3DBounds,
            fUseZBuffer,
            &rZ
            ));

#if DBG_STEP_RENDERING
    // Remember these debug params independent of success
    m_Dbg3DBounds = m_rcBounds;
    m_Dbg3DAAMode = AntiAliasMode;
#endif DBG_STEP_RENDERING

Cleanup:

    if (FAILED(hr))
    {
        // Safe to call even without calling LockInternalSurface
        UnlockInternalSurface();
    }

    if (!f3DCapable)
    {
        //
        // We eat this error here as opposed to higher in the stack so that in
        // multimon scenarios, other displays are still given the rendering
        // instructions and have a chance of working.
        //
        Assert(!m_pHw3DRT);
        hr = S_OK;
    }

    // Future Consideration: Move Cleanup3DResources to match Hw behavior
    //
    // This call was in Present, which is where we cleanup resources in hw
    // but the RenderTargetBitmap object renders in sw and doesn't call
    // present.  This results in us leaking system memory resources with
    // the sw 3d rendertarget.
    // 
    // Since we're just releasing objects in system memory and don't have
    // to pay for a flush, it's safe for us to call this after every 3D
    // rendering operation.
    //
    Cleanup3DResources();

#if DBG_ANALYSIS
    if (SUCCEEDED(hr))
    {
        m_fDbgBetweenBeginAndEnd3D = true;
    }
#endif

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::End3D
//
//  Synopsis:
//      Have D3D surface composited back to this surface
//

STDMETHODIMP
CSwRenderTargetSurface::End3D(
    )
{
    HRESULT hr = S_OK;

    Assert(m_fDbgBetweenBeginAndEnd3D);

    if (!m_pHw3DRT)
    {
        // eat rendering instruction
        goto Cleanup;
    }

    if (!m_pvBuffer)
    {
        IFC(WGXERR_INVALIDCALL);
    }

    IFC(m_pHw3DRT->EndSw3D(
        this,
        // For fast composite
        m_pvBuffer,
        DBG_ANALYSIS_PARAM_COMMA(m_uHeight*m_cbStride)
        m_cbStride,
        // For slow composite
        &m_sr
        ));

Cleanup:

    //
    // Restore all state
    //

    // Safe to call even without calling Begin3D/LockInternalSurface
    UnlockInternalSurface();

    //  Future Consideration: Move Cleanup3DResources to match Hw behavior
    //
    // This call was in Present, which is where we cleanup resources in hw
    // but the RenderTargetBitmap object renders in sw and doesn't call
    // present.  This results in us leaking system memory resources with
    // the sw 3d rendertarget.
    // 
    // Since we're just releasing objects in system memory and don't have
    // to pay for a flush, it's safe for us to call this after every 3D
    // rendering operation.
    //
    Cleanup3DResources();

#if DBG_ANALYSIS
    m_fDbgBetweenBeginAndEnd3D = false;
#endif

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::UpdateCurrentClip
//
//  Synopsis:
//      Realizes a clip object on the render target
//
//------------------------------------------------------------------------------

bool CSwRenderTargetSurface::UpdateCurrentClip(
    __in_ecount(1) const CAliasedClip &aliasedClip,
    __out_ecount(1) CRectClipper *pRectClipperOut
    )
{
    return CBaseRenderTarget::UpdateCurrentClip(aliasedClip) ?
        (pRectClipperOut->SetClip(m_rcCurrentClip), true) :
        false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::HasAlpha
//
//  Synopsis:
//      Returns true if the destination has alpha.
//
//------------------------------------------------------------------------------

bool CSwRenderTargetSurface::HasAlpha() const
{
    // This should be reviewed when
    // CScanPipeline::InitializeForTextRendering will support more
    // formats.
    Assert(m_fmtTarget == MilPixelFormat::PBGRA32bpp || m_fmtTarget == MilPixelFormat::BGR32bpp);

    return m_fmtTarget == MilPixelFormat::PBGRA32bpp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::DrawBitmap
//
//  Synopsis:
//      The Render Target is given the opportunity to accelerate this primitive
//      using some internal knowledge, if possible.
//

STDMETHODIMP CSwRenderTargetSurface::DrawBitmap(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    Assert(m_pIInternalSurface);

    // Render this primitive using an accelerated rendering technique.

    // Fall through to the SR

    HRESULT hr = S_OK;

    CRectClipper Clipper;

    if (!UpdateCurrentClip(pContextState->AliasedClip, &Clipper))
    {
        // Clipping yields no area; so be done
        goto Cleanup;
    }

    // Lock the internal surface so that consecutive calls to NextBuffer
    // do not have to take the overhead of calling Lock/Unlock.

    if (SUCCEEDED(hr))
    {
        MIL_THR(LockInternalSurface(
            NULL,
            MilBitmapLock::Write | MilBitmapLock::Read
            ));
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(m_sr.DrawBitmap(
            this,
            &Clipper,
            pContextState,
            pIBitmap,
            pIEffect
            ));
    }

Cleanup:
    // Unlock internal surface (despite any other failures)
    UnlockInternalSurface();

    RRETURN(hr);
}

#if DBG_STEP_RENDERING
#if !DBG
extern volatile BOOL g_fStepSWRendering;
#endif
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwSurfaceTargetSurface::DrawMesh3D
//
//------------------------------------------------------------------------------
STDMETHODIMP CSwRenderTargetSurface::DrawMesh3D(
    __inout_ecount(1) CContextState* pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __inout_ecount_opt(1) CMILShader *pShader,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    //
    // Check if 3d drawing has been disabled
    //

    if (g_pMediaControl != NULL
        && g_pMediaControl->GetDataPtr()->Draw3DDisabled)
    {
        goto Cleanup;
    }

    Assert(m_fDbgBetweenBeginAndEnd3D);
    
    if (!m_pHw3DRT)
    {
        // eat rendering instruction
        goto Cleanup;
    }

    IFC(m_pHw3DRT->DrawMesh3D(
        pContextState,
        pBrushContext,
        pMesh3D,
        pShader,
        pIEffect
        ));

#if DBG_STEP_RENDERING
    if (
#if DBG
        IsTagEnabled(tagMILStepRendering)
#else
        g_fStepSWRendering
#endif
        )
    {
        Assert(m_pILock);
        Assert(m_pvBuffer);

        IGNORE_HR(m_pHw3DRT->EndSw3D(
            this,
            // For fast composite
            m_pvBuffer,
            DBG_ANALYSIS_PARAM_COMMA(m_uHeight*m_cbStride)
            m_cbStride,
            // For slow composite
            &m_sr
            ));

        SW_DBG_RENDERING_STEP(DrawMesh3D);

        FreAssert(SUCCEEDED(m_pHw3DRT->BeginSw3D(
            m_pvBuffer,
            DBG_ANALYSIS_PARAM_COMMA(m_uHeight*m_cbStride)
            m_cbStride,
            m_Dbg3DBounds,
            true,   // fUseZBuffer - Ignored
            NULL    // Do not clear depth
            )));
    }
#endif

Cleanup:
    //  Future Consideration: Move Cleanup3DResources to match Hw behavior
    //
    // This call was in Present, which is where we cleanup resources in hw
    // but the RenderTargetBitmap object renders in sw and doesn't call
    // present.  This results in us leaking system memory resources with
    // the sw 3d rendertarget.
    // 
    // Since we're just releasing objects in system memory and don't have
    // to pay for a flush, it's safe for us to call this after every 3D
    // rendering operation.
    //
    Cleanup3DResources();

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::DrawPathInternal
//
//  Synopsis:
//      Implementation of DrawPath and DrawInfinitePath.  Treats NULL shape as
//      infinite.
//
//------------------------------------------------------------------------------
HRESULT CSwRenderTargetSurface::DrawPathInternal(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount_opt(1) const IShapeData *pShape,
    __inout_ecount_opt(1) const CPlainPen *pPen,
    __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
    __inout_ecount_opt(1) CBrushRealizer *pFillBrush
    )
{
    // We require that this is checked by the API proxy class.

    Assert(m_pIInternalSurface);

    HRESULT hr = S_OK;

    CRectClipper Clipper;

    if (!UpdateCurrentClip(pContextState->AliasedClip, &Clipper))
    {
        // Clipping yields no area; so be done
        goto Cleanup;
    }

    // Lock the internal surface so that consecutive calls to NextBuffer
    // do not have to take the overhead of calling Lock/Unlock.

    Assert(NULL == m_pILock);
    Assert(NULL == m_pvBuffer);

    IFC(LockInternalSurface(
        NULL,
        MilBitmapLock::Write | MilBitmapLock::Read
        ));

    //
    // For 2D rendering, local rendering and world sampling spaces are identical
    //

    const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &
        matBaseSamplingToDevice =
        ReinterpretLocalRenderingAsBaseSampling(pContextState->WorldToDevice);

    if (pFillBrush)
    {
        // Fill the path
        IFC(m_sr.FillPathUsingBrushRealizer(
            this,
            m_fmtTarget,
            m_associatedDisplay,
            &Clipper,
            pContextState,
            pBrushContext,
            pShape,
            &static_cast<const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> &>(pContextState->WorldToDevice),
            pFillBrush,
            matBaseSamplingToDevice
            DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
            ));
    }

    if (pPen && pStrokeBrush)
    {
        // Widen and then fill the path
        CShape widened;

        IFC(pShape->WidenToShape(
            *pPen,
            DEFAULT_FLATTENING_TOLERANCE,
            false,
            widened,
            CMILMatrix::ReinterpretBase
             (&static_cast<const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> &>
              (pContextState->WorldToDevice)),
            &m_rcBounds));

        IFC(m_sr.FillPathUsingBrushRealizer(
            this,
            m_fmtTarget,
            m_associatedDisplay,
            &Clipper,
            pContextState,
            pBrushContext,
            &widened,
            NULL,
            pStrokeBrush,
            matBaseSamplingToDevice
            DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
            ));
    }

Cleanup:
    // Some failure HRESULTs should only cause the primitive
    // in question to not draw.
    IgnoreNoRenderHRESULTs(&hr);

    // Unlock internal surface (despite any other failures)
    UnlockInternalSurface();

    if (SUCCEEDED(hr))
    {
        // SW_DBG_RENDERING_STEP must happen after UnlockInternalSurface
        SW_DBG_RENDERING_STEP(DrawPathInternal);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::DrawPath
//
//  Synopsis:
//      The Render Target is given the opportunity to accelerate this primitive
//      using some internal knowledge, if possible.
//
//------------------------------------------------------------------------------
STDMETHODIMP CSwRenderTargetSurface::DrawPath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) IShapeData *pShape,
    __inout_ecount_opt(1) CPlainPen *pPen,
    __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
    __inout_ecount_opt(1) CBrushRealizer *pFillBrush
    )
{
    return DrawPathInternal(
        pContextState,
        pBrushContext,
        pShape,
        pPen,
        pStrokeBrush,
        pFillBrush
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::DrawInfinitePath
//
//  Synopsis:
//      Draw a shape filling the entire render target.
//
//------------------------------------------------------------------------------
STDMETHODIMP CSwRenderTargetSurface::DrawInfinitePath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) BrushContext *pBrushContext,
    __inout_ecount(1) CBrushRealizer *pFillBrush
    )
{
    return DrawPathInternal(
        pContextState,
        pBrushContext,
        NULL,
        NULL,
        NULL,
        pFillBrush
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::ComposeEffect
//
//------------------------------------------------------------------------------

STDMETHODIMP 
CSwRenderTargetSurface::ComposeEffect(
    __inout_ecount(1) CContextState *pContextState,
    __in_ecount(1) CMILMatrix *pScaleTransform,
    __inout_ecount(1) CMilEffectDuce* pEffect,
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt IMILRenderTargetBitmap* pImplicitInputRTB
    ) 
{
    // We require that this is checked by the API proxy class.

    Assert(pContextState);

    HRESULT hr = S_OK;

    CMetaBitmapRenderTarget *pImplicitInputMetaRT = NULL;
    CSwRenderTargetBitmap *pImplicitInputSwBitmapRTNoRef = NULL;
    IWGXBitmap *pIImplicitInput = NULL;


    // In the common scenario, our input texture is a meta RT.
    if (pImplicitInputRTB != NULL)
    {
        hr = pImplicitInputRTB->QueryInterface(
                IID_CMetaBitmapRenderTarget,
                reinterpret_cast<void **>(&pImplicitInputMetaRT));
        
        if (SUCCEEDED(hr))
        {
            IMILRenderTargetBitmap* pIImplicitInputBitmapRTNoRef = NULL;        

            IFC(pImplicitInputMetaRT->GetCompatibleSubRenderTargetNoRef(
                CMILResourceCache::SwRealizationCacheIndex, 
                m_associatedDisplay, 
                &pIImplicitInputBitmapRTNoRef));
            
            pImplicitInputSwBitmapRTNoRef = static_cast<CSwRenderTargetBitmap*>(pIImplicitInputBitmapRTNoRef);
        }
        else
        {
            // If the QI fails, we are inside a visual brush which does not use meta RTs.  If that's
            // the case, we were directly handed a SW texture RT, since we force compatible RTs to be 
            // created (a SwRTSurf will only create SwRTBs for effects).
            hr = S_OK;
            pImplicitInputSwBitmapRTNoRef = static_cast<CSwRenderTargetBitmap*>(pImplicitInputRTB);
        }

        IFC(pImplicitInputSwBitmapRTNoRef->GetBitmap(&pIImplicitInput));
    }

    IFC(pEffect->ApplyEffectSw(
        pContextState, 
        this, 
        pScaleTransform,
        uIntermediateWidth,
        uIntermediateHeight,
        pIImplicitInput
        ));
        

Cleanup:
    ReleaseInterface(pImplicitInputMetaRT);
    ReleaseInterface(pIImplicitInput);


    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::DrawGlyphs
//
//  Synopsis:
//      Draw the glyph run
//
//------------------------------------------------------------------------------
STDMETHODIMP CSwRenderTargetSurface::DrawGlyphs(
    __inout_ecount(1) DrawGlyphsParameters &pars
    )
{
    Assert(pars.pContextState);
    Assert(pars.pGlyphRun);
    Assert(pars.pBrushRealizer);

    HRESULT hr = S_OK;

    CRectClipper Clipper;
    CMILBrush *pBrushNoRef;
    float flAlphaScale;

    if (!UpdateCurrentClip( pars.pContextState->AliasedClip, &Clipper ))
    {
        // Clipping yields no area; so be done
        goto Cleanup;
    }

    {
        CSwIntermediateRTCreator swRTCreator(
            m_fmtTarget,
            m_associatedDisplay
            DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
            );

        IFC(pars.pBrushRealizer->EnsureRealization(
            CMILResourceCache::SwRealizationCacheIndex,
            m_associatedDisplay,
            pars.pBrushContext,
            pars.pContextState,
            &swRTCreator
            ));

        pBrushNoRef = pars.pBrushRealizer->GetRealizedBrushNoRef(false /* fConvertNULLToTransparent */);
        flAlphaScale = pars.pBrushRealizer->GetOpacityFromRealizedBrush();

        if (pBrushNoRef == NULL)
        {
            // Nothing to draw
            goto Cleanup;
        }
    }

    //
    // This target supports ClearType rendering if ClearTypeHint has beeen set
    // (m_forceClearType) or it doesn't support per pixel transparency
    //
    bool fTargetSupportsClearType = m_forceClearType || !HasAlpha();

    // Lock the internal surface so that we can access the pixels.
    IFC(LockInternalSurface(NULL, MilBitmapLock::Write | MilBitmapLock::Read));

    IFC(m_sr.DrawGlyphRun(
        this,
        &Clipper,
        pars,
        pBrushNoRef,
        flAlphaScale,
        &m_glyphPainterMemory,
        fTargetSupportsClearType
        ));

Cleanup:

    // Some failure HRESULTs should only cause the primitive
    // in question to not draw.
    IgnoreNoRenderHRESULTs(&hr);

    // Unlock internal surface (despite any other failures)
    UnlockInternalSurface();

    if (SUCCEEDED(hr))
    {
        // SW_DBG_RENDERING_STEP must happen after UnlockInternalSurface
        SW_DBG_RENDERING_STEP(DrawGlyphs);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CreateRenderTargetBitmap Create a bitmap compatible with this
//      RenderTarget and wrap a new RenderTarget around it.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP CSwRenderTargetSurface::CreateRenderTargetBitmap(
    UINT width,
    UINT height,
    IntermediateRTUsage usageInfo,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
    __in_opt DynArray<bool> const *pActiveDisplays
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pActiveDisplays);
    
    CSwIntermediateRTCreator swRTCreator(
        m_fmtTarget,
        m_associatedDisplay
        DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
        );

    IFC(swRTCreator.CreateRenderTargetBitmap(
        width,
        height,
        usageInfo,
        dwFlags,
        ppIRenderTargetBitmap
        ));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::BeginLayerInternal
//
//  Synopsis:
//      Begin accumulation of rendering into a layer.  Modifications to layer,
//      as specified in arguments, are handled and result is applied to render
//      target when the matching EndLayer call is made.
//
//      Calls to BeginLayer may be nested, but other calls that depend on the
//      current contents, such as Present, are not allowed until all
//      layers have been resolved with EndLayer.
//

HRESULT CSwRenderTargetSurface::BeginLayerInternal(
    __inout_ecount(1) CRenderTargetLayer *pNewLayer
    )
{
    HRESULT hr = S_OK;

    CMILSurfaceRect rgCopyRects[MAX_NUM_PARTIAL_LAYER_CAPTURE_RECTS];
    UINT cCopyRects = 0;
    bool fCopyEntireLayer = true;
    WICRect rcLayerBounds;
    IWGXBitmapLock *pIBitmapLock = NULL;

    //
    // Check for cases that are not supported.
    //
    //  a) an alpha mask
    //  b) render target has alpha
    //
    if ( pNewLayer->pAlphaMaskBrush ||
         HasAlpha())
    {
        IFC(E_NOTIMPL);
    }

    //
    // Check to see if we can avoid copying the entire layer.
    // Right now the only case we handle is an aliased geometric mask shape that
    // is an axis aligned rectangle.
    // If there is an alpha scale, we will need the entire bitmap anyway.
    //

    fCopyEntireLayer = !GetPartialLayerCaptureRects(
        pNewLayer,
        rgCopyRects,
        &cCopyRects
        );

    if (   fCopyEntireLayer
        || cCopyRects > 0
        )
    {
        //
        // Create backup of current surface within layer bounds
        //

        rcLayerBounds.X = pNewLayer->rcLayerBounds.left;
        rcLayerBounds.Y = pNewLayer->rcLayerBounds.top;
        rcLayerBounds.Width = pNewLayer->rcLayerBounds.right - pNewLayer->rcLayerBounds.left;
        rcLayerBounds.Height = pNewLayer->rcLayerBounds.bottom - pNewLayer->rcLayerBounds.top;

        IFC(CreateBitmapFromSourceRect(
            m_pIInternalSurface,
            rcLayerBounds.X,
            rcLayerBounds.Y,
            rcLayerBounds.Width,
            rcLayerBounds.Height,
            fCopyEntireLayer, // fCopySource
            &(pNewLayer->oTargetData.m_pSourceBitmap)
            ));

        if (!fCopyEntireLayer)
        {
            UINT uStride;
            BYTE *pvData = NULL;
            UINT cbBufferSize;

            {
                WICRect rcLock;
                rcLock.X = 0;
                rcLock.Y = 0;
                rcLock.Width = rcLayerBounds.Width;
                rcLock.Height = rcLayerBounds.Height;

                IFC(pNewLayer->oTargetData.m_pSourceBitmap->Lock(
                    &rcLock,
                    MilBitmapLock::Write,
                    &pIBitmapLock
                    ));
            }

            IFC(pIBitmapLock->GetStride(&uStride));

            #if DBG
            {
                MilPixelFormat::Enum dbgPixelFormat;
                IGNORE_HR(pIBitmapLock->GetPixelFormat(&dbgPixelFormat));

                Assert(dbgPixelFormat == m_fmtTarget);
            }
            #endif

            IFC(pIBitmapLock->GetDataPointer(
                &cbBufferSize,
                &pvData
                ));

            // initialize buffer with strange color
            #if DBG
            if (m_cbPixel == sizeof(GpCC))
            {
                for (int y = 0; y < rcLayerBounds.Height; y++)
                {
                    for (UINT x = 0; x < uStride / sizeof(GpCC); x++)
                    {
                        // fill to some kind of purple
                        GpCC *pFillColor = reinterpret_cast<GpCC*>(
                              pvData
                            + y*uStride
                            + x*sizeof(GpCC)
                            );
                        Assert(
                            reinterpret_cast<BYTE*>(pFillColor) + sizeof(GpCC)
                            <= pvData + cbBufferSize
                            );
                        pFillColor->a = 255;
                        pFillColor->r = 255;
                        pFillColor->g = 0;
                        pFillColor->b = 128;
                    }
                }
            }
            #endif

            for (UINT i = 0; i < cCopyRects; i++)
            {
                WICRect rcCopyRect;
                rcCopyRect.X = rgCopyRects[i].left;
                rcCopyRect.Y = rgCopyRects[i].top;
                rcCopyRect.Width = rgCopyRects[i].right - rgCopyRects[i].left;
                rcCopyRect.Height = rgCopyRects[i].bottom - rgCopyRects[i].top;

                // convert start point to bitmap coordinates
                UINT copyStartX = rcCopyRect.X - rcLayerBounds.X;
                UINT copyStartY = rcCopyRect.Y - rcLayerBounds.Y;

                //
                // CopyPixels does not take a destination rect-
                // we must find the offset of the first pixel ourselves
                //
                UINT uOffsetOfFirstPixel =
                      copyStartY * uStride
                    + copyStartX * m_cbPixel;

                IFC(m_pIInternalSurface->CopyPixels(
                    &rcCopyRect,
                    uStride,
                    cbBufferSize - uOffsetOfFirstPixel,
                    pvData + uOffsetOfFirstPixel
                    ));
            }
        }
    }

Cleanup:
    ReleaseInterfaceNoNULL(pIBitmapLock);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::EndLayerInternal
//
//  Synopsis:
//      End accumulation of rendering into current layer.  Modifications to
//      layer, as specified in BeginLayer arguments, are handled and result is
//      applied to render target.
//

HRESULT CSwRenderTargetSurface::EndLayerInternal(
    )
{
    HRESULT hr = S_OK;

    CRenderTargetLayer const &layer = m_LayerStack.Top();

    Assert(layer.oTargetData.m_pSourceBitmap);

    //
    // Create a layer bounding shape
    //

    CMilRectF rcLayerFloat(
        static_cast<FLOAT>(layer.rcLayerBounds.left),
        static_cast<FLOAT>(layer.rcLayerBounds.top),
        static_cast<FLOAT>(layer.rcLayerBounds.right),
        static_cast<FLOAT>(layer.rcLayerBounds.bottom),
        LTRB_Parameters
        );

    CShape boundShape;

    IFC(boundShape.AddRect(rcLayerFloat));


    //
    // Prepare for rendering
    //

    // Lock the internal surface so that we can access the pixels.
    IFC(LockInternalSurface(
        NULL,
        MilBitmapLock::Write | MilBitmapLock::Read
        ));

    {
        //
        // Set clip to layer bounds
        //

        CRectClipper Clipper;
        Clipper.SetClip(layer.rcLayerBounds);

        //
        // Setup a default context and render state
        //

        CMILMatrix matLayerToOriginalCopy(true);
        CContextState contextState(TRUE /* => basic initialization only */);
        CRenderState renderState;

        renderState.InterpolationMode = MilBitmapInterpolationMode::NearestNeighbor;
        renderState.PrefilterEnable = false;
        //renderState.AntiAliasMode = set below
        //renderState.CompositingMode = MILCompositingModeLayer;

        contextState.RenderState = &renderState;
        contextState.AliasedClip = CAliasedClip(NULL);
        //
        // Use a temporary bitmap brush to be passed to DrawPath.  This stack
        // brush may not be reference counted since its lifetime is exactly the
        // scope in which it is defined, but no longer.  LocalMILObject helps
        // enforce this via asserts on checked builds.
        //

        LocalMILObject<CMILBrushBitmap> bbBrush;

        //
        // World == Target(Device) space so LayerToTarget can be used as
        // BrushToWorld.
        //
        // The scale is 1:1 so no change is needed for scale factors.
        //
        // Bitmap origin (0,0) = Brush origin (0,0) should map to Layer origin
        //  (layer.rcLayerBounds.left, layer.rcLayerBounds.top)
        //

        matLayerToOriginalCopy._41 = static_cast<FLOAT>(layer.rcLayerBounds.left);
        matLayerToOriginalCopy._42 = static_cast<FLOAT>(layer.rcLayerBounds.top);

        {
            Assert(contextState.WorldToDevice.IsIdentity());

            CMILBrushBitmapLocalSetterWrapper brushBitmapLocalWrapper(
                &bbBrush,
                layer.oTargetData.m_pSourceBitmap, // !No AddRef!
                // Wrap won't matter because we're doing a pixel-perfect copy.  Use border here
                // because that's supported by the IdentitySpan.
                MilBitmapWrapMode::Border,
                &matLayerToOriginalCopy, //  pmatBitmapToXSpace
                XSpaceIsSampleSpace
                DBG_COMMA_PARAM(NULL) // pmatDBGWorldToSampleSpace
                );

            //
            // Render fixups
            //

            bool fNeedConstantAlphaFixup = !AlphaScalePreservesOpacity(layer.rAlpha);

            //
            // Check for geometric mask fixups
            //

            if (layer.pGeometricMaskShape)
            {
                //
                // Render geometric mask fixups (and take care of constant opacity also.)
                //

                renderState.AntiAliasMode = layer.AntiAliasMode;

                // The brush realizer is needed to fix up meta-intermediates in the brush
                LocalMILObject<CImmediateBrushRealizer> fillBrush;
                fillBrush.SetMILBrush(
                    &bbBrush,
                    NULL,
                    false // don't skip meta-fixups
                    );

                if (layer.AntiAliasMode == MilAntiAliasMode::None)
                {
                    //
                    // Complement not yet supported in aliased geometry, so we create inverted
                    // geometry to simulate coverage inversion
                    //

                    CShape invertedGeometricMask;

                    MIL_THR(CShapeBase::Combine(
                                &boundShape,
                                layer.pGeometricMaskShape,
                                MilCombineMode::Xor,
                                false,  // ==> Do not retrieve curves from the flattened result
                                &invertedGeometricMask
                                ));

                    if (SUCCEEDED(hr))
                    {
                        //
                        // Render geometric mask fixups
                        //

                        renderState.AntiAliasMode = layer.AntiAliasMode;

                        Assert(contextState.WorldToDevice.IsIdentity());

                        MIL_THR(m_sr.FillPathUsingBrushRealizer(
                                    this,
                                    m_fmtTarget,
                                    m_associatedDisplay,
                                    &Clipper,
                                    &contextState,
                                    NULL,
                                    &invertedGeometricMask,
                                    CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device>::pIdentity(),
                                    &fillBrush,
                                    CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device>::refIdentity()
                                    DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
                                    ));
                    }
                }
                else
                {
                    MIL_THR(m_sr.FillPath(
                                this,
                                &Clipper,
                                &contextState,
                                layer.pGeometricMaskShape,
                                NULL,
                                &bbBrush,
                                CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device>::refIdentity(),
                                NULL,
                                layer.rAlpha,
                                &layer.rcLayerBounds
                                ));

                    fNeedConstantAlphaFixup = false;
                }
            }

            if (fNeedConstantAlphaFixup)
            {
                //
                // Check for constant opacity fixups.  If we had a geometric
                // mask shape (and we are using AA rendering) then the opacity
                // was handled as part of that fill path but if we don't have a
                // geometric mask shape then we can go even simpler and render
                // an aliased rectangle for the opacity.
                //
                // Use an inverted opacity scale restore original target colors
                //

                AlphaScaleParams alphaParams;
                alphaParams.scale = 1.0f - layer.rAlpha;
                Assert(!AlphaScaleEliminatesRenderOutput(alphaParams.scale));

                LocalMILObject<CEffectList> effectList;

                // Set AlphaScale effect
                MIL_THR(effectList.Add(
                            CLSID_MILEffectAlphaScale,
                            sizeof(alphaParams),
                            &alphaParams
                            ));

                if (SUCCEEDED(hr))
                {
                    // The brush realizer is needed to fix up meta-intermediates in the brush
                    LocalMILObject<CImmediateBrushRealizer> fillBrush;
                    fillBrush.SetMILBrush(
                        &bbBrush,
                        &effectList,
                        false // don't skip meta-fixups
                        );

                    // This operation is pixel aligned so hint to SW rasterizer
                    // that no antialiasing is needed.
                    renderState.AntiAliasMode = MilAntiAliasMode::None;

                    MIL_THR(m_sr.FillPathUsingBrushRealizer(
                                this,
                                m_fmtTarget,
                                m_associatedDisplay,
                                &Clipper,
                                &contextState,
                                NULL,
                                &boundShape,
                                CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device>::pIdentity(),
                                &fillBrush,
                                CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device>::refIdentity()
                                DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
                                ));
                }
            }
        }
    }

Cleanup:

    //
    // Cleanup rendering
    //

    // Unlock internal surface (despite any other failures)
    UnlockInternalSurface();

    if (SUCCEEDED(hr))
    {
        // SW_DBG_RENDERING_STEP must happen after UnlockInternalSurface
        SW_DBG_RENDERING_STEP(EndLayer);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::GetNumQueuedPresents
//
//  Synopsis:
//      Sw doesn't queue up any rendering calls, so it always returns 0.
//
//------------------------------------------------------------------------------
HRESULT
CSwRenderTargetSurface::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    *puNumQueuedPresents = 0;

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::DrawVideo
//
//  Synopsis:
//      Draw the video
//
//------------------------------------------------------------------------------
STDMETHODIMP CSwRenderTargetSurface::DrawVideo(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __inout_ecount(1) IWGXBitmapSource *pBitmapSource,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;
    IWGXBitmapSource *pMILBitmapSource = NULL;
    bool fSavePrefilterEnable = pContextState->RenderState->PrefilterEnable;
    BOOL fBeginRenderCalled = FALSE;

    Assert(pSurfaceRenderer || pBitmapSource);

    if (pSurfaceRenderer)
    {
        IFC(pSurfaceRenderer->BeginRender(NULL, &pMILBitmapSource));
        fBeginRenderCalled = TRUE;
    }
    else
    {
        SetInterface(pMILBitmapSource, pBitmapSource);
    }

    // BeginRender is not guaranteed to return a surface if the stream
    // does not have video
    // Workaround for people playing audio files using the Video object
    if (pMILBitmapSource)
    {
        // Disable prefiltering for video.
        pContextState->RenderState->PrefilterEnable = false;
        IFC(DrawBitmap(pContextState, pMILBitmapSource, pIEffect));
    }

Cleanup:

    if (fBeginRenderCalled)
    {
        IGNORE_HR(pSurfaceRenderer->EndRender());
    }

    if (SUCCEEDED(hr))
    {
        // SW_DBG_RENDERING_STEP must happen after EndRender
        SW_DBG_RENDERING_STEP(DrawVideo);
    }

    ReleaseInterfaceNoNULL(pMILBitmapSource);
    pContextState->RenderState->PrefilterEnable = fSavePrefilterEnable;
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::AddDirtyRect, CSpanSink
//
//------------------------------------------------------------------------------

void CSwRenderTargetSurface::AddDirtyRect(
    __in_ecount(1) const MilPointAndSizeL *prc
    )
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::OutputSpan, CSpanSink (COutputSpan)
//
//  Synopsis:
//      Outputs the given span into the surface. How it is output, depends on
//      how the scan pipeline has been set up.
//

void CSwRenderTargetSurface::OutputSpan(
    INT y,
    INT xMin,
    INT xMax
    )
{
    Assert(y >= 0);
    Assert((UINT)y < m_uHeight);

    Assert(xMin >= 0);

    // The value passed to the count parameter, xMax - xMin, must be at least one
    // one, as this is assumed by many output span implementations.
    Assert(xMax > xMin);

    Assert(static_cast<UINT>(xMax) <= m_uWidth);

    Assert(m_pvBuffer);

    // Calculate the destination for the scan:

    VOID *pvDest = static_cast<BYTE*>(m_pvBuffer) +
        static_cast<INT_PTR>(xMin) * m_cbPixel +
        static_cast<INT_PTR>(y) * m_cbStride;

    UINT cPixels = xMax - xMin;
    m_ScanPipeline.Run(
        pvDest, 
        NULL, // pvSrc
        cPixels, // iCount
        xMin, 
        y
        DBG_ANALYSIS_COMMA_PARAM(cPixels * m_cbPixel)
        DBG_ANALYSIS_COMMA_PARAM(0)
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::SetAntialiasedFiller, CSpanSink
//

VOID CSwRenderTargetSurface::SetAntialiasedFiller(
    __inout_ecount(1) CAntialiasedFiller *pFiller
    )
{
    m_ScanPipeline.SetAntialiasedFiller(pFiller);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::SetupPipeline, CSpanSink
//

HRESULT CSwRenderTargetSurface::SetupPipeline(
    MilPixelFormat::Enum fmtColorData,
    __in_ecount(1) CColorSource *pColorSource,
    BOOL fPPAA,
    bool fComplementAlpha,
    MilCompositingMode::Enum eCompositingMode,
    __in_ecount(1) CSpanClipper *pSpanClipper,
    __in_ecount_opt(1) IMILEffectList *pIEffectList,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device> *pmatEffectToDevice,
    __in_ecount(1) const CContextState *pContextState
    )
{
    //   fmtColorData is ignored
    // To fix:
    //  * Merge sRGB/scRGB CS creators (make the choice programmatic).
    //  * Remove the call to SetColorDataPixelFormat.

    CMILSurfaceRect rcClipBounds;

    Assert(pSpanClipper);
    pSpanClipper->GetClipBounds(&rcClipBounds);

    return m_ScanPipeline.InitializeForRendering(
        m_IntermediateBuffers,
        m_fmtTarget,
        pColorSource,
        fPPAA,
        fComplementAlpha,
        eCompositingMode,
        rcClipBounds.Width(),
        pIEffectList,
        pmatEffectToDevice,
        pContextState);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::SetupPipelineForText
//
//  Synopsis:
//      Recall m_ScanPipeline to prepare for text rendering
//
//------------------------------------------------------------------------------
HRESULT CSwRenderTargetSurface::SetupPipelineForText(
    __in_ecount(1) CColorSource *pColorSource,
    MilCompositingMode::Enum eCompositingMode,
    __inout_ecount(1) CSWGlyphRunPainter &painter,
    bool fNeedsAA
    )
{
    return m_ScanPipeline.InitializeForTextRendering(
         m_IntermediateBuffers,
         m_fmtTarget,
         pColorSource,
         eCompositingMode,
         painter,
         fNeedsAA
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::ReleaseExpensiveResources, CSpanSink
//

VOID CSwRenderTargetSurface::ReleaseExpensiveResources()
{
    m_ScanPipeline.ReleaseExpensiveResources();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetSurface::Cleanup3DResources
//
//  Synopsis:
//      Free unused resources left over from rendering.
//
//------------------------------------------------------------------------------
void
CSwRenderTargetSurface::Cleanup3DResources()
{
    if (m_pHw3DRT)
    {
        m_pHw3DRT->CleanupFreedResources();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::Create
//
//  Synopsis:
//      Initialize the bitmap render target with the bitmap it should use as the
//      output.
//

HRESULT CSwRenderTargetBitmap::Create(
    __in_ecount(1) IWGXBitmap *pIBitmap,
    DisplayId associatedDisplay,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap
    DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount_opt(1) ISteppedRenderingDisplayRT *pDisplayRTParent)
    )
{
    HRESULT hr = S_OK;

    *ppIRenderTargetBitmap = NULL;

    CSwRenderTargetBitmap *pRT = new CSwRenderTargetBitmap(associatedDisplay);
    if (pRT == NULL)
    {
        MIL_THR(E_OUTOFMEMORY);
    }
    else
    {
        pRT->AddRef();
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(pRT->SetSurface(pIBitmap));
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(pRT->QueryInterface(
            IID_IMILRenderTargetBitmap,
            (void **)ppIRenderTargetBitmap
            ));
    }

    // Since the creation of any SW intermediate render target ends up calling
    // this function eventually, we use this to increment our tracking count
    if (   SUCCEEDED(hr)
        && g_pMediaControl
       )
    {
        // Add one to our count of IRTs used this frame
        InterlockedIncrement(reinterpret_cast<LONG *>(
            &(g_pMediaControl->GetDataPtr()->NumSoftwareIntermediateRenderTargets)
            ));
    }

    //
    // Step Rendering code
    //

#if DBG_STEP_RENDERING
    pRT->m_pDisplayRTParent = pDisplayRTParent;
    if (pRT->m_pDisplayRTParent) { pRT->m_pDisplayRTParent->AddRef(); }
#endif DBG_STEP_RENDERING


    if (pRT != NULL)
    {
        pRT->Release();
    }

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::CSwRenderTargetBitmap
//
//  Synopsis:
//      ctor
//

CSwRenderTargetBitmap::CSwRenderTargetBitmap(
    DisplayId associatedDisplay
    ) :
    CSwRenderTargetSurface(associatedDisplay)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::Create
//
//  Synopsis:
//      Create a new bitmap and supply it to a new render target.
//

HRESULT CSwRenderTargetBitmap::Create(
    UINT width,
    UINT height,
    MilPixelFormat::Enum format,
    FLOAT dpiX,
    FLOAT dpiY,
    DisplayId associatedDisplay,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap
    DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount_opt(1) ISteppedRenderingDisplayRT *pDisplayRTParent)
    )
{
    HRESULT hr = S_OK;

    *ppIRenderTargetBitmap = NULL;

    CSystemMemoryBitmap *pBitmap = NULL;

    IFC(CSystemMemoryBitmap::Create(
        width,
        height,
        format,
        /* fClear = */ TRUE, 
        /* fIsDynamic = */ FALSE,
        &pBitmap
        ));

    // Set the resolution
    IFC(pBitmap->SetResolution(dpiX, dpiY));

    // Create the RT to wrap the bitmap
    IFC(CSwRenderTargetBitmap::Create(
        pBitmap,
        associatedDisplay,
        ppIRenderTargetBitmap
        DBG_STEP_RENDERING_COMMA_PARAM(pDisplayRTParent)
        ));

Cleanup:
    ReleaseInterface(pBitmap);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::HrFindInterface
//
//  Synopsis:
//      CSwRenderTargetBitmap QI helper routine
//

STDMETHODIMP CSwRenderTargetBitmap::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IMILRenderTargetBitmap)
        {
            *ppvObject = static_cast<IMILRenderTargetBitmap*>(this);

            hr = S_OK;
        }
        else
        {
            hr = CBaseRenderTarget::HrFindInterface(riid, ppvObject);
        }
    }

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::GetBounds
//
//  Synopsis:
//      Delegate to CSwRenderTargetSurface::GetBounds
//
//------------------------------------------------------------------------------

STDMETHODIMP_(VOID) CSwRenderTargetBitmap::GetBounds(
    __out_ecount(1) MilRectF * const pBounds
    )
{
    return CSwRenderTargetSurface::GetBounds(pBounds);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::Clear
//
//  Synopsis:
//      Delegate to CSwRenderTargetSurface::Clear
//

STDMETHODIMP CSwRenderTargetBitmap::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    HRESULT hr = S_OK;

    hr = CSwRenderTargetSurface::Clear(pColor, pAliasedClip);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::Begin3D, End3D
//
//  Synopsis:
//      Delegate to CSwRenderTargetSurface
//
//------------------------------------------------------------------------------

STDMETHODIMP
CSwRenderTargetBitmap::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    return CSwRenderTargetSurface::Begin3D(rcBounds, AntiAliasMode, fUseZBuffer, rZ);
}

STDMETHODIMP
CSwRenderTargetBitmap::End3D(
    )
{
    return CSwRenderTargetSurface::End3D();
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::TintBitmapSource
//
//  Synopsis:
//      Add a tint to the bitmap this surface outputs to.
//

VOID CSwRenderTargetBitmap::TintBitmapSource()
{
    Assert(g_pMediaControl);

    IWGXBitmapLock *pBitmapLock;

    if (SUCCEEDED(m_pIInternalSurface->Lock(NULL,
                                            MilBitmapLock::Read | MilBitmapLock::Write,
                                            &pBitmapLock
                                            )))
    {
        MilPixelFormat::Enum pixelFormat;

        if (SUCCEEDED(pBitmapLock->GetPixelFormat(&pixelFormat)))
        {
            Assert(pixelFormat == MilPixelFormat::PBGRA32bpp || pixelFormat == MilPixelFormat::BGRA32bpp);

            if (   pixelFormat == MilPixelFormat::PBGRA32bpp
                || pixelFormat == MilPixelFormat::BGRA32bpp
               )
            {
                UINT cbBufferSize;
                BYTE *pbBuffer;
                UINT uWidth;
                UINT uHeight;
                UINT cbStride;

                if (   SUCCEEDED(pBitmapLock->GetDataPointer(&cbBufferSize, &pbBuffer))
                    && SUCCEEDED(pBitmapLock->GetSize(&uWidth, &uHeight))
                    && SUCCEEDED(pBitmapLock->GetStride(&cbStride))
                   )
                {
                    g_pMediaControl->TintARGBBitmap(
                        reinterpret_cast<ARGB *>(pbBuffer),
                        uWidth,
                        uHeight,
                        cbStride
                        );
                }
            }
        }

        ReleaseInterfaceNoNULL(pBitmapLock);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::GetBitmapSource
//
//  Synopsis:
//      Return the bitmap this render target outputs to.
//

HRESULT CSwRenderTargetBitmap::GetBitmapSource(
    __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
    )
{
    HRESULT hr = S_OK;

    Assert(m_pIInternalSurface);

    // Check to see if we need to color all software bits purple
    if (   g_pMediaControl != NULL
        && g_pMediaControl->GetDataPtr()->RecolorSoftwareRendering
       )
    {
        TintBitmapSource();
    }

    *ppIBitmapSource = m_pIInternalSurface;
    (*ppIBitmapSource)->AddRef();

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::GetCacheableBitmapSource
//
//  Synopsis:
//      Return the bitmap this render target outputs to.  Unlike hardware,
//      nothing has to be done to make this bitmap-source cachable
//

HRESULT CSwRenderTargetBitmap::GetCacheableBitmapSource(
    __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
    )
{
    HRESULT hr = S_OK;
    
    IFC(GetBitmapSource(ppIBitmapSource));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::GetBitmap
//
//  Synopsis:
//      Return the bitmap this render target outputs to.
//

HRESULT CSwRenderTargetBitmap::GetBitmap(
    __deref_out_ecount(1) IWGXBitmap ** const ppIBitmap
    )
{
    HRESULT hr = S_OK;

    Assert(m_pIInternalSurface);

    *ppIBitmap = m_pIInternalSurface;
    (*ppIBitmap)->AddRef();

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwRenderTargetBitmap::GetNumQueuedPresents
//
//  Synopsis:
//      Sw doesn't queue up any rendering calls, so it always returns 0.
//
//------------------------------------------------------------------------------
HRESULT
CSwRenderTargetBitmap::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    RRETURN(CSwRenderTargetSurface::GetNumQueuedPresents(
        puNumQueuedPresents
        ));
}




