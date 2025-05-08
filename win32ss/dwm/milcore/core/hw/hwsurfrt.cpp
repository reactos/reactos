// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//
//      CHwSurfaceRenderTarget implementation.
//
//      This object is the base class for CHwHWNDRenderTarget and provides a
//      basic render target that can output to a dx9 surface.
//

#include "precomp.hpp"


MtDefine(CHwSurfaceRenderTarget, MILRender, "CHwSurfaceRenderTarget");
MtDefine(CHwSurfaceRenderTarget_pvBuffer, MILRender, "CHwSurfaceRenderTarget_pvBuffer");

MtDefine(CHwDeviceRasterizerScanlineAccess, MILRender, "CHwDeviceRasterizerScanlineAccess");
MtDefine(CHwDeviceRasterizerClip, MILRender, "CHwDeviceRasterizerClip");

DeclareTag(tagDisplayMeshBounds, "MIL-HW", "DisplayMeshBounds");
DeclareTag(tagDisplayGeometryStrokeBounds, "MIL-HW", "DisplayGeometryStrokeBounds");
DeclareTag(tagDisplayGeometryBounds, "MIL-HW", "DisplayGeometryBounds");
DeclareTag(tagResetRenderStateWhenDrawing, "MIL-HW", "Reset RenderState When Drawing");

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::CHwSurfaceRenderTarget
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CHwSurfaceRenderTarget::CHwSurfaceRenderTarget(
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
    MilPixelFormat::Enum fmtTarget,
    D3DFORMAT d3dfmtTarget,
    DisplayId associatedDisplay
    ) :
    CBaseSurfaceRenderTarget<CHwRenderTargetLayerData>(associatedDisplay),
    m_pD3DDevice(pDevice),
    m_d3dfmtTargetSurface(d3dfmtTarget)
{
    m_pD3DDevice->AddRef();

    m_pD3DDevice->AssertRenderFormatIsTestedSuccessfully(m_d3dfmtTargetSurface);

    m_fmtTarget = fmtTarget;

    m_fIn3D = false;
    m_fZBufferEnabled = false;

    m_pD3DTargetSurface = NULL;
    m_pD3DIntermediateMultisampleTargetSurface = NULL;
    m_pD3DTargetSurfaceFor3DNoRef = NULL;
    m_pD3DStencilSurface = NULL;

    const auto& primaryDisplayDpi = DpiScale::PrimaryDisplayDpi();
    m_DeviceTransform.Scale(primaryDisplayDpi.DpiScaleX, primaryDisplayDpi.DpiScaleY);

    // Not yet initialized - only valid after EnsureClip
    //m_CurrentClip

#if DBG_STEP_RENDERING
    m_pDisplayRTParent = NULL;
#endif DBG_STEP_RENDERING
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::~CHwSurfaceRenderTarget
//
//  Synopsis:  dtor
//
//  Notes:     This destructor may only be called under the callers threading
//             protection.  If another thread were to be actively rendering
//             while this is processed the D3D render targets may be
//             incorrectly managed.
//
//-----------------------------------------------------------------------------
CHwSurfaceRenderTarget::~CHwSurfaceRenderTarget()
{
    {
        // We are assumming that the caller has called Release under their
        // thread protection.  In free builds we will be broken, but under
        // checked build we will assert if there is another thread actively
        // rendering.
        ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

        ReleaseInterfaceNoNULL(m_pD3DTargetSurface);
        ReleaseInterfaceNoNULL(m_pD3DIntermediateMultisampleTargetSurface);
        ReleaseInterfaceNoNULL(m_pD3DStencilSurface);
    }

    m_pD3DDevice->Release();

#if DBG_STEP_RENDERING
    Assert(m_pDisplayRTParent == NULL);
#endif DBG_STEP_RENDERING

}


//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::Clear
//
//  Synopsis:  Delegate to the device clear
//
//-------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    HRESULT hr = S_OK;

    CMILSurfaceRect rcClip;

    Assert(m_pD3DDevice);
    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

    //
    // Now that we have entered device scope, make sure render target is viable
    // for rendering.  Mostly this means that we haven't noticed a mode change
    // or have been called since then despite letting our caller know about the
    // mode change.
    //

    if (!IsValid())
    {
        Assert(hr == S_OK);
        goto Cleanup;
    }

    if (pColor == NULL)
    {
        goto Cleanup;
    }

    DbgResetStateUponTraceTag();

    if (IntersectCAliasedClipWithSurfaceRect(pAliasedClip,
                                             m_rcBounds,
                                             OUT &rcClip
                                             ))
    {
        IFC(SetAsRenderTarget());

        MilColorB color;

        //  This clear code really only supports 32bpp (P)BGR(X|A) formats well
        switch (m_fmtTarget)
        {
        case MilPixelFormat::PBGRA32bpp:
        case MilPixelFormat::PRGBA64bpp:
        case MilPixelFormat::PRGBA128bppFloat:


            color = Convert_MilColorF_scRGB_To_Premultiplied_MilColorB_sRGB(pColor);
            break;

        default:
            color = Convert_MilColorF_scRGB_To_MilColorB_sRGB(pColor);
        }

        //
        // SetClipRect will setup clipping thru the viewport or scissor
        // rect, both of which affect the clear operation.  Since those
        // setting do affect the clear operation we have to make sure the
        // settings are correct instead of just sending the rect to Clear
        // itself.  We also gain the advantage that this clip will normally
        // be the same clip applied to subsequent rendering and therefore
        // we will have already properly set up clipping for those calls
        // and won't need further state changes.
        //

        IFC(m_pD3DDevice->SetClipRect(&rcClip));

        //
        // Since we've already specified the clip rect thru the viewport or
        // scissor rect then we don't need to send a rect here.  In fact,
        // we save a sliver of time by not passing any rects.  The DX token
        // stream understands 0 rects.
        //

        IFC(m_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, color, 0, 0));
    }

    HW_DBG_RENDERING_STEP(Clear);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::Begin3D
//
//  Synopsis:  Setup current 3D bounds and buffers and clear the z-buffer
//
//-------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    HRESULT hr = S_OK;

    if (m_fIn3D)
    {
        IFC(WGXERR_INVALIDCALL);
    }

    m_rcBoundsPre3D = m_rcBounds;

    //
    // Compute actual 3D bounds
    //

    if (IntersectBoundsRectFWithSurfaceRect(
        AntiAliasMode,
        // rcBounds should be a MultiSpaceRect Device/PageInPixels
        CRectF<CoordinateSpace::Device>::ReinterpretNonSpaceTyped(rcBounds),
        m_rcBounds,
        OUT &m_rcBounds
        ))
    {
        //
        // Determine 3D AA mode
        //

        D3DMULTISAMPLE_TYPE multisampleTypeRequested = D3DMULTISAMPLE_NONE;

        if (AntiAliasMode != MilAntiAliasMode::None)
        {
            // Should we attempt Multisample?
            if (m_pD3DDevice->ShouldAttemptMultisample())
            {
                multisampleTypeRequested = m_pD3DDevice->GetSupportedMultisampleType(m_fmtTarget);
            }
        }

        //
        // Setup buffers and clear Z
        //

        D3DMULTISAMPLE_TYPE multisampleTypeRecevied = multisampleTypeRequested;

        // multisampleTypeRecevied will be modified to reflect the level of multisampling
        // achieved in a successful call to Begin3DInternal
        MIL_THR(Begin3DInternal(rZ, fUseZBuffer, multisampleTypeRecevied));

        // If the called succeeded but could not acquire the requested multisampling level,
        // we should not request multisampling in the future
        if (SUCCEEDED(hr) && (multisampleTypeRecevied != multisampleTypeRequested))
        {
            m_pD3DDevice->SetMultisampleFailed();
        }
    }

    if (FAILED(hr))
    {
        // Not entering 3D context so restore bounds
        m_rcBounds = m_rcBoundsPre3D;
    }
    else
    {
        // Enter 3D context on success, even if bounds are now empty
        m_fIn3D = true;
    }

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::Setup3DRenderTargetAndDepthState
//
//  Synopsis:  Helper method for CHwSurfaceRenderTarget::Begin3DInternal
//
//-------------------------------------------------------------------------

HRESULT
CHwSurfaceRenderTarget::Setup3DRenderTargetAndDepthState(
    FLOAT rZ,
    bool fUseZBuffer,
    __inout_ecount(1) D3DMULTISAMPLE_TYPE &MultisampleType
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure we have a render target ready for 3D rendering.
    //

    Ensure3DRenderTarget(MultisampleType);

    //
    // Ensure we have the right render target set.  Note that this step
    // is necessary since we can't select a z-buffer with a different
    // size than our render target, so we need to make sure we have the
    // right render target before enabling the z-buffer.
    //
    // We also want to do this before resetting the clip rect to avoid extra
    // state set (as a NULL clip rect is computed from the currently set RT's
    // description.)
    //

    IFC(SetAsRenderTargetFor3D());

    //
    // We must set the clip rect, otherwise when the depth buffer is cleared a
    // scissor rect could result the wrong part of the z-buffer being cleared.
    //

    IFC(m_pD3DDevice->SetClipRect(&m_rcBounds));

    m_fZBufferEnabled = fUseZBuffer;

    if (m_fZBufferEnabled)
    {
        //
        // Now, it is safe to ensure we have a z-buffer and it is set
        //

        IFC(EnsureDepthState());

        Assert(m_pD3DStencilSurface);

        //
        // Clear the z-buffer
        //

        IFC(m_pD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, rZ, 0));
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::Begin3DInternal
//
//  Synopsis:  Setup buffers and clear the z-buffer
//
//-------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::Begin3DInternal(
    FLOAT rZ,
    bool fUseZBuffer,
    __inout_ecount(1) D3DMULTISAMPLE_TYPE &MultisampleType
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDevice);
    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

    //
    // Now that we have entered device scope, make sure render target is viable
    // for rendering.  Mostly this means that we haven't noticed a mode change
    // or have been called since then despite letting our caller know about the
    // mode change.
    //

    if (!IsValid())
    {
        Assert(hr == S_OK);

        //
        // Since we're going to skip the Begin3D operation we need to make sure
        // the bounds are empty so that End3D will execute, but will have no
        // region to copy.
        //
        m_rcBounds.SetEmpty();

        goto Cleanup;
    }

    DbgResetStateUponTraceTag();

    hr = Setup3DRenderTargetAndDepthState(
        rZ,
        fUseZBuffer,
        MultisampleType);

    // If we ran out of memory and we were attempting multisampling, we should try again
    // without multisampling.
    if ((hr == D3DERR_OUTOFVIDEOMEMORY) && (MultisampleType != D3DMULTISAMPLE_NONE))
    {
        MultisampleType = D3DMULTISAMPLE_NONE;

        // Clear out the previous render target
        m_pD3DTargetSurfaceFor3DNoRef = NULL;

        // Release the multisample intermediate if we had one
        ReleaseInterface(m_pD3DIntermediateMultisampleTargetSurface);

        // Try again with the new Multisample type
        IFC(Setup3DRenderTargetAndDepthState(
            rZ,
            fUseZBuffer,
            MultisampleType));
    }

    //
    // If 3D target is different than the regular one, blt bits up
    //

    if (m_pD3DTargetSurfaceFor3DNoRef != m_pD3DTargetSurface)
    {
        IFC(m_pD3DDevice->StretchRect(
            m_pD3DTargetSurface,
            &m_rcBounds,
            m_pD3DTargetSurfaceFor3DNoRef,
            &m_rcBounds,
            D3DTEXF_NONE  // No stretching, so NONE is fine.  NONE is better
                          // than POINT only because RefRast doesn't expose the
                          // cap and D3D would fail this call.
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::End3D
//
//  Synopsis:  Complete 3D rendering
//

STDMETHODIMP
CHwSurfaceRenderTarget::End3D(
    )
{
    HRESULT hr = S_OK;

    if (!m_fIn3D)
    {
        IFC(WGXERR_INVALIDCALL);
    }

    if (!m_rcBounds.IsEmpty())
    {
        Assert(m_pD3DTargetSurfaceFor3DNoRef);

        //
        // If 3D target is different than the regular one, blt bits back
        //

        if (m_pD3DTargetSurfaceFor3DNoRef != m_pD3DTargetSurface)
        {
            ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

            IFC(m_pD3DDevice->StretchRect(
                m_pD3DTargetSurfaceFor3DNoRef,
                &m_rcBounds,
                m_pD3DTargetSurface,
                &m_rcBounds,
                D3DTEXF_NONE  // No stretching, so NONE is fine.  NONE is
                              // better than POINT only because RefRast doesn't
                              // expose the cap and D3D would fail this call.
                ));

            HW_DBG_RENDERING_STEP(End3D_AntiAliased);
        }
    }

Cleanup:

    //
    // Always leave 3D context, even on error
    //

    if (m_fIn3D)
    {
        m_fIn3D = false;
        // NOTE: don't unset m_fEnableZBuffer as the last-set value
        //       is still used for Sw stepped rendering.

        // Restore bound iff just in 3D context
        m_rcBounds = m_rcBoundsPre3D;
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::DrawBitmap
//
//  Synopsis:  Use FillPath to draw the bitmap
//
//  Notes:     No attempt is made to call SW fallback if there is failure
//             before calling DrawPath.  DrawPath will call SW fallback code if
//             it needs to though.
//

STDMETHODIMP
CHwSurfaceRenderTarget::DrawBitmap(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
    __inout_ecount_opt(1) IMILEffectList *pIEffects
    )
{
    HRESULT hr = S_OK;

    CParallelogram bitmapShape;
    CRectF<CoordinateSpace::Shape> rcSource;

    CMILBrushBitmap *pBitmapBrushNoAddRef = NULL;

    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);
    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    //
    // Now that we have entered device scope, make sure render target is viable
    // for rendering.  Mostly this means that we haven't noticed a mode change
    // or have been called since then despite letting our caller know about the
    // mode change.
    //

    if (!IsValid())
    {
        Assert(hr == S_OK);
        goto Cleanup;
    }

    IFC(m_pD3DDevice->GetScratchDrawBitmapBrushNoAddRef(&pBitmapBrushNoAddRef));

    //
    // Compute the destination rectangle. If the renderstate source
    // rectangle is invalid, make a rectangle using the bitmap
    // dimensions.
    //
    // Code copied from swrast.cpp.
    //

    if (pContextState->RenderState->Options.SourceRectValid)
    {
        MilRectFFromMilPointAndSizeL(OUT rcSource, pContextState->RenderState->SourceRect);
    }
    else
    {
        // Default source rect covers the bounds of the source, which
        // is 1/2 beyond the extreme sample points in each direction.

        IFC(GetBitmapSourceBounds(
            pIBitmapSource,
            &rcSource
            ));
    }

    EventWriteDrawBitmapInfo(pIBitmapSource, static_cast<INT>(rcSource.right - rcSource.left), static_cast<INT>(rcSource.bottom - rcSource.top));

    //
    // Initialize shape
    //

    bitmapShape.Set(rcSource);

    //
    // Draw the Path
    //

    IFC(EnsureState(pContextState));
    if (hr == WGXHR_CLIPPEDTOEMPTY)
    {
        goto Cleanup;
    }

    {
        //
        // Initialize bitmap brush
        //

        CMILBrushBitmapLocalSetterWrapper brushBitmapLocalWrapper(
            pBitmapBrushNoAddRef,
            pIBitmapSource,
            MilBitmapWrapMode::Extend,
            &pContextState->WorldToDevice, //  pmatBitmapToXSpace
            XSpaceIsSampleSpace
            DBG_COMMA_PARAM(
                &ReinterpretLocalRenderingAsBaseSampling(pContextState->WorldToDevice)) // pmatDBGWorldToSampleSpace
            );

        LocalMILObject<CImmediateBrushRealizer> fillBrush;
        fillBrush.SetMILBrush(
            pBitmapBrushNoAddRef,
            pIEffects,
            true // skip meta-fixups (they are already handled in CMetaRenderTarget::DrawBitmap)
            );

        //
        // For 2D rendering, local rendering and world sampling spaces are identical
        //

        const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &
            matBaseSamplingToDevice =
            ReinterpretLocalRenderingAsBaseSampling(pContextState->WorldToDevice);

        //
        // Note that we do not call EnsureRealization on this brush. This is
        // okay, because it does nothing for CImmediateBrushRealizer's that are
        // skipping meta-fixups
        //

        IFC(FillPath(
            pContextState,
            NULL,
            &bitmapShape,
            &static_cast<const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> &>(pContextState->WorldToDevice), // pmatShapeToDevice
            &rcSource,
            &fillBrush,
            matBaseSamplingToDevice  // pmatBrushHPCToDeviceIPC
            ));
    }

Cleanup:
    #if DBG
    Assert(pBitmapBrushNoAddRef == NULL || !DbgHasMultipleReferences(pBitmapBrushNoAddRef));
    #endif

    // Some failure HRESULTs should only cause the primitive
    // in question to not draw.
    IgnoreNoRenderHRESULTs(&hr);

    if (hr == WGXHR_CLIPPEDTOEMPTY)
    {
        hr = S_OK;
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::DrawMesh3D
//
//  Synopsis:  Use the D3D device to draw the mesh, otherwise
//             use SW fallback
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwSurfaceRenderTarget::DrawMesh3D(
    __inout_ecount(1) CContextState* pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __inout_ecount_opt(1) CMILShader *pShader,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    Assert(m_fIn3D);

    HRESULT hr = S_OK;
    CHwShader *pHwShader = NULL;
    CMILSurfaceRect rcRenderBoundsDeviceSpace;
    CRectF<CoordinateSpace::BaseSampling> rcBrushSamplingBounds;

    bool fMeshVisible = false;

    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);
    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    //
    // If the bounds are empty then we must quick out as we might not have even
    // setup the 3D render target.
    //

    if (m_rcBounds.IsEmpty())
    {
        goto Cleanup;
    }

    //
    // Check if 3d drawing has been disabled
    //

    if (g_pMediaControl != NULL
        && g_pMediaControl->GetDataPtr()->Draw3DDisabled)
    {
        goto Cleanup;
    }

    //
    // We shouldn't lose validity on the surface between begin3d and End3D, if
    // we failed the IsValid check in Begin3D the bounds should be empty which
    // will cause us to already bail out of this function.
    //
    Assert(IsValid());

    Assert(pContextState);

    IFC(pShader->EnsureBrushRealizations(
        m_pD3DDevice->GetRealizationCacheIndex(),
        m_associatedDisplay,
        pBrushContext,
        pContextState,
        this
        ));

    IFC(EnsureState(pContextState));
    if (hr == WGXHR_CLIPPEDTOEMPTY)
    {
        hr = S_OK;
    }
    else
    {
        CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::IdealSampling> matBaseSamplingToIdealSampling;

        //
        // Calculate the rendering bounds & transform
        //
        // Future Consideration:   mesh perf
        //
        // We just did the same thing in the model render
        // walker. Can we avoid calling this twice?
        //

        IFC(ApplyProjectedMeshTo2DState(
            pContextState,
            pMesh3D,
            m_rcCurrentClip,
            OUT &matBaseSamplingToIdealSampling,
            OUT &rcRenderBoundsDeviceSpace,
            OUT &fMeshVisible,
            OUT &rcBrushSamplingBounds
            ));

        if (fMeshVisible)
        {
            CHwBrushContext hwBrushContext(
                pContextState,
                ReinterpretIdealSamplingAsDevice(matBaseSamplingToIdealSampling),
                m_fmtTarget,
                FALSE // fCanFallback
                );

            // matBaseSamplingToIdealSampling is always a positive scale for 3D.
            Assert(matBaseSamplingToIdealSampling.IsPureNonNegative2DScale());
            hwBrushContext.SetBaseSamplingBounds(rcBrushSamplingBounds);

            //
            // Since we're in 3D we don't need to transform the brush by the 2D
            // World to Device Transform.  So we pass identity.
            //

            IFC(m_pD3DDevice->DeriveHWShader(
                pShader,
                hwBrushContext,
                &pHwShader
                ));

            IFC(pHwShader->DrawMesh3D(
                m_pD3DDevice,
                this,
                pMesh3D,
                rcRenderBoundsDeviceSpace,
                pContextState,
                m_fZBufferEnabled
                ));


            if (IsTagEnabled(tagDisplayMeshBounds))
            {
                MilPointAndSize3F boxMeshBounds;

                if (SUCCEEDED(pMesh3D->GetBounds(
                    &boxMeshBounds
                    )))
                {
                    IGNORE_HR(m_pD3DDevice->DrawBox(
                        &boxMeshBounds,
                        D3DFILL_WIREFRAME,
                        0x80000000
                        ));
                }
            }

            HW_DBG_RENDERING_STEP(DrawMesh3D);
        }
    }

Cleanup:

    //
    // Catch the non-invertible matrix error. Rendering nothing is acceptable
    // for cases where we hit a non-invertible transform.
    // Warning to future modifiers: this error is caught elsewhere as well.
    //
    if (hr == WGXERR_NONINVERTIBLEMATRIX)
    {
        hr = S_OK;
    }

    ReleaseInterfaceNoNULL(pHwShader);
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::SoftwareFillPath
//
//  Synopsis:  Fill the path using SW fallback.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::SoftwareFillPath(
    __in_ecount(1) const CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice,
    __in_ecount(1) const IShapeData *pShape,
    __in_ecount(1) CBrushRealizer *pBrushRealizer,
    HRESULT hrReasonForFallback
    )
{
    HRESULT hr = S_OK;

    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    CMILBrush *pBrushNoRef = NULL;
    IMILEffectList *pIEffectNoRef = NULL;

    //
    // Realize the brush again for SW fallback
    //

    {
        CSwIntermediateRTCreator swRTCreator(
            MilPixelFormat::PBGRA32bpp,      // Tile texture format
            m_associatedDisplay
            DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
            );

        IFC(pBrushRealizer->EnsureRealization(
            CMILResourceCache::SwRealizationCacheIndex,
            m_associatedDisplay,
            pBrushContext,
            pContextState,
            &swRTCreator
            ));

        pBrushNoRef = pBrushRealizer->GetRealizedBrushNoRef(false /* fConvertNULLToTransparent */);
        IFC(pBrushRealizer->GetRealizedEffectsNoRef(&pIEffectNoRef));
    }

    Assert(pBrushNoRef); // The NULL case should have been handled by the hardware FillPath

    //
    // Note: It is not necessary to call EnsureState because the brush
    //       realization is done in SW
    //

    CHwSoftwareFallback *pswFallback = NULL;

    IFC(m_pD3DDevice->GetSoftwareFallback(
        &pswFallback,
        hrReasonForFallback
        ));

    IFC(pswFallback->FillPath(
        pContextState,
        pmatShapeToDevice,
        pShape,
        pBrushNoRef,
        pIEffectNoRef,
        m_uWidth,
        m_uHeight
        ));

Cleanup:
    // Should SW fallback class use AddRef/Release
    //ReleaseInterface(pswFallback);
    
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::AcceleratedFillPath
//
//  Synopsis:  Fill the path using HW.
//

HRESULT
CHwSurfaceRenderTarget::AcceleratedFillPath(
    MilCompositingMode::Enum CompositingMode,
    __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
    __in_ecount(1) CHwBrush *pBrush,
    __in_ecount_opt(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext *pEffectContext,
    __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds,
    bool fNeedInside
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDevice->IsInAUseContext());
    Assert(CanUseShaderPipeline());

    IFC(ShaderAcceleratedFillPath(
        CompositingMode,
        pGeometryGenerator,
        pBrush,
        pIEffects,
        pEffectContext,
        prcOutsideBounds,
        fNeedInside
        ));

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::ShaderAcceleratedFillPath
//
//  Synopsis:  Fill the path using HW shaders.
//

HRESULT
CHwSurfaceRenderTarget::ShaderAcceleratedFillPath(
    MilCompositingMode::Enum CompositingMode,
    __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
    __in_ecount(1) CHwBrush *pBrush,
    __in_ecount_opt(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext *pEffectContext,
    __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds,
    bool fNeedInside
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDevice->IsInAUseContext());

    CHwShaderPipeline Pipeline(
        true, /* this is a 2D pipeline */
        m_pD3DDevice
        );

    IFC(Pipeline.InitializeForRendering(
        CompositingMode,
        pGeometryGenerator,
        pBrush,
        pIEffects,
        pEffectContext,
        prcOutsideBounds,
        fNeedInside
        ));

    IFC(Pipeline.Execute());

Cleanup:

    Pipeline.ReleaseExpensiveResources();

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::FillPath
//
//  Synopsis:  Fill the path either using the shader pipeline, or
//             the fixed function pipeline.
//

/*
//
// TessellatorUnion is used only to compute the size of a stack buffer needed
// to hold an instance of any of the tessellators.
//
// There is a compilation issue, error C2621, that prevents use of this
// technique.  Error C2621 is "member of union has copy constructor", but this
// must be a compiler generated one, because I can't find an explicit one.
//
union TessellatorUnion {
    CTessellator _general;
    CRectFillTessellator _rect;
};

enum {
    kMaxTessellatorAlignment = __alignof(TessellatorUnion),
    kMaxTessellatorSize = kMaxTessellatorAlignment-1+sizeof(TessellatorUnion)
};
*/

// The MAX macro has been phased out and replaced by the function max, but here a function
// call is not allowed.  To avoid misuse elsewhere, the BAD_MAX macro is defined ad-hoc here.
//
// NOTE-2005/04/13-chrisra BAD_MAX is also defined in hwvertexbuffer.h
//
#define BAD_MAX(a, b) ((a) >= (b) ? (a) : (b))
enum {
    kMaxTessellatorSize = BAD_MAX(BAD_MAX(
        MAX_SPACE_FOR_TYPE(CTessellator),
        MAX_SPACE_FOR_TYPE(CRectFillTessellator)),
        MAX_SPACE_FOR_TYPE(CHwRasterizer))
};
// This macro should not be used elsewhere.
#undef BAD_MAX

HRESULT
CHwSurfaceRenderTarget::FillPath(
    __in_ecount(1) const CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __in_ecount(1) const IShapeData *pShape,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice, // (NULL OK)
    __in_ecount(1) const CRectF<CoordinateSpace::Shape> *prcShapeBounds, // in shape space
    __in_ecount(1) CBrushRealizer *pBrushRealizer,
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice
    )
{
    AssertDeviceEntry(*m_pD3DDevice);

    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    HRESULT hr = S_OK;

    CMILBrush *pFillBrushNoRef = NULL;
    IMILEffectList *pIEffectsNoRef = NULL;
    CRectF<CoordinateSpace::Shape> rcShapeBounds;


    // Clip shape to safe device bounds if needed.
    CShape clippedShape;
    bool fWasShapeClipped = false;

    IFC(ClipToSafeDeviceBounds(pShape, pmatShapeToDevice, prcShapeBounds, &clippedShape, &fWasShapeClipped));

    if (fWasShapeClipped)
    {
        pShape = &clippedShape;
        pmatShapeToDevice = NULL;
        pShape->GetTightBounds(rcShapeBounds);
        prcShapeBounds = &rcShapeBounds;
    }

    Assert(pShape);

    pFillBrushNoRef = pBrushRealizer->GetRealizedBrushNoRef(false /* fConvertNULLToTransparent */);
    IFC(pBrushRealizer->GetRealizedEffectsNoRef(&pIEffectsNoRef));

    if (pFillBrushNoRef == NULL)
    {
        // Nothing to draw
        goto Cleanup;
    }


    //
    // EnsureState must happen after brush realization since brush realization
    // can mess with device state
    //
    // Note: EnsureState will be called twice now for DrawPath if there is a
    //       fill brush and a stroke brush. Given that no one is calling
    //       DrawPath with both a fill brush and a stroke brush at the same
    //       time, it is not worth optimizing this call pattern. If we did want
    //       to optimize the extra EnsureState away, we could do so iff the
    //       brush did not create a HW intermediate
    //

    IFC(EnsureState(pContextState));
    if (hr == WGXHR_CLIPPEDTOEMPTY)
    {
        hr = S_OK;
        goto Cleanup;
    }

    IFC(FillPathWithBrush(
        pContextState,
        pShape,
        pmatShapeToDevice, //pmatShapeToDevice
        prcShapeBounds,
        pFillBrushNoRef,
        matWorldToDevice,
        pIEffectsNoRef
        ));

    HW_DBG_RENDERING_STEP(FillPath);

    
Cleanup:

    if (hr == E_NOTIMPL)
    {
        MIL_THR(SoftwareFillPath(
            pContextState,
            pBrushContext,
            pmatShapeToDevice,
            pShape,
            pBrushRealizer,
            hr
            ));

        if (SUCCEEDED(hr))
        {
            HW_DBG_RENDERING_STEP(SoftwareFillPath);
        }
    }

    // Some failure HRESULTs should only cause the primitive
    // in question to not draw.
    IgnoreNoRenderHRESULTs(&hr);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::FillPathWithBrush
//
//  Synopsis:  Fill a path using the fixed function pipeline
//
//             Note: The caller is responsible for software fallback.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::FillPathWithBrush(
    __in_ecount(1) const CContextState *pContextState,
    __in_ecount(1) const IShapeData *pShape,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice,
    __in_ecount(1) const CRectF<CoordinateSpace::Shape> *prcShapeBounds, // in shape space
    __in_ecount(1) CMILBrush *pFillBrush,
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice,
    __in_ecount_opt(1) const IMILEffectList *pIEffects
    )
{
    HRESULT hr = S_OK;
    CHwBrush *pHwBrush = NULL;
    CFillTessellator *pFillTessellator = NULL;
    CHwRasterizer *pHwRasterizer = NULL;
    IGeometryGenerator *pIGeometryGenerator = NULL;
    CDispensableBuffer<kMaxTessellatorSize, 1> localBuffer;
    CRectF<CoordinateSpace::Device> rcShapeBoundsDeviceSpace;
    CMILSurfaceRect rcRenderingBounds;

    Assert(m_pD3DDevice->IsInAUseContext());

    //
    // Modify shape as needed for source clipping
    //

    CShapeClipperForFEB clipper(pShape, prcShapeBounds, pmatShapeToDevice);

    IFC(clipper.ApplyGuidelines(
        pContextState->m_pSnappingStack,
        m_pD3DDevice->GetScratchSnapShape()
        ));

    IFC(clipper.ApplyBrush(
        pFillBrush,
        matWorldToDevice,
        m_pD3DDevice->GetScratchFillShape()
        ));

    IFC(clipper.GetBoundsInDeviceSpace(&rcShapeBoundsDeviceSpace));

    //
    // Calculate the rendering bounds and don't draw if they are empty
    //

    if (IntersectBoundsRectFWithSurfaceRect(
            pContextState->RenderState->AntiAliasMode,
            rcShapeBoundsDeviceSpace,
            m_rcCurrentClip,
            &rcRenderingBounds
            ))
    {
        EventWriteDWMDraw_Info(rcShapeBoundsDeviceSpace.left, rcShapeBoundsDeviceSpace.top, rcShapeBoundsDeviceSpace.right, rcShapeBoundsDeviceSpace.bottom);

        CHwBrushContext hwBrushContext(
            pContextState,
            matWorldToDevice, // matWorld2DToSampleSpace
            m_fmtTarget,
            TRUE // fCanFallback
            );

        hwBrushContext.SetDeviceRenderingAndSamplingBounds(rcRenderingBounds);


        //
        // Lookup the brush
        //

        //
        // In 2D the Sample Space is nearly equivalent to the Device Space.
        // There is a 0.5 adjustment here because the rendering bounds are
        // aliased to the edge of a pixel, but samples are taken at the center
        // of a pixel.  Therefore shrink the sample bounds by 0.5.
        //

        IFC(m_pD3DDevice->DeriveHWBrush(
            pFillBrush,
            hwBrushContext,
            &pHwBrush
            ));

        //
        // If we are anti-aliased, go to trapezoidal AA
        //

        if (pContextState->RenderState->AntiAliasMode != MilAntiAliasMode::None)
        {
            pHwRasterizer = new(&localBuffer) CHwRasterizer();
            IFCOOM(pHwRasterizer);

            IFC(pHwRasterizer->Setup(
                m_pD3DDevice,
                clipper.GetShape(),
                m_pD3DDevice->GetScratchPoints(),
                m_pD3DDevice->GetScratchTypes(),
                clipper.GetShapeToDeviceTransformOrNull()
                ));

            pIGeometryGenerator = pHwRasterizer;
        }
        else
        {
            IFC(clipper.GetShape()->SetupFillTessellator(
                clipper.GetShapeToDeviceTransformOrNull(),
                &localBuffer,
                &pFillTessellator
                ));

            pIGeometryGenerator = pFillTessellator;
        }

        //
        // Draw the shape
        //

        if (pIGeometryGenerator)
        {
            Assert(hr == S_OK);

            IFC(AcceleratedFillPath(
                pContextState->RenderState->CompositingMode,
                pIGeometryGenerator,
                pHwBrush,
                pIEffects,
                &hwBrushContext
                ));
        }
        else
        {
            //
            // The only other success value we can have at this point is empty shape
            //

            Assert(hr == WGXHR_EMPTYFILL);
            hr = S_OK;
        }
    }

Cleanup:
    if (pHwBrush)
    {
        pHwBrush->Release();
    }

    delete pFillTessellator;
    delete pHwRasterizer;

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::DrawPathInternal
//
//  Synopsis:  Break down the fill and stroke operations to two fill
//             operations, then pass to FillPath for rendering.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::DrawPathInternal(
    __in_ecount(1) const CContextState *pContextState,
    __in_ecount_opt(1) CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> const *pmatShapeToDevice,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) const IShapeData *pShape,
    __inout_ecount_opt(1) const CPlainPen *pPen,
    __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
    __inout_ecount_opt(1) CBrushRealizer *pFillBrush
    )
{
    HRESULT hr = S_OK;

    // We require that these are checked by the API proxy class.
    Assert(pContextState);
    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);
    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    //
    // Now that we have entered device scope, make sure render target is viable
    // for rendering.  Mostly this means that we haven't noticed a mode change
    // or have been called since then despite letting our caller know about the
    // mode change.
    //

    if (!IsValid())
    {
        Assert(hr == S_OK);
        goto Cleanup;
    }

    //
    // For 2D rendering, local rendering and world sampling spaces are identical
    //

    const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &
        matBaseSamplingToDevice =
        ReinterpretLocalRenderingAsBaseSampling(pContextState->WorldToDevice);

    if (pFillBrush)
    {
        //
        // EnsureRealization must be called before we get any scratch objects
        // from the device. We call it in DrawPath (once for the fill, once for
        // the stroke) instead of in FillPath to avoid re-using the scratch
        // widen shape.
        //
        IFC(pFillBrush->EnsureRealization(
            m_pD3DDevice->GetRealizationCacheIndex(),
            m_associatedDisplay,
            pBrushContext,
            pContextState,
            this
            ));

        CRectF<CoordinateSpace::Shape> rcShapeBounds;
        IFC(pShape->GetTightBounds(OUT rcShapeBounds));
        
        IFC(FillPath(
            pContextState,
            pBrushContext,
            pShape,
            pmatShapeToDevice,
            &rcShapeBounds,
            pFillBrush,
            matBaseSamplingToDevice
            ));
    }

    if (pPen && pStrokeBrush)
    {
        Assert(pShape);
        
        //
        // EnsureRealization must be called before we get any scratch objects
        // from the device. We call it in DrawPath (once for the fill, once for
        // the stroke) instead of in FillPath to avoid re-using the scratch
        // widen shape.
        //
        IFC(pStrokeBrush->EnsureRealization(
            m_pD3DDevice->GetRealizationCacheIndex(),
            m_associatedDisplay,
            pBrushContext,
            pContextState,
            this
            ));


        // Widen and then fill the path
        CRectF<CoordinateSpace::Shape> rcShapeBounds;
        CShape *pShapeWiden = m_pD3DDevice->GetScratchWidenShape();

        Assert(pShapeWiden);
        pShapeWiden->Reset();

        IFC(pShape->WidenToShape(
            *pPen,
            DEFAULT_FLATTENING_TOLERANCE,
            false,
            *pShapeWiden,
            CMILMatrix::ReinterpretBase(pmatShapeToDevice),
            &m_rcBounds
            ));

        IFC(pShapeWiden->GetTightBounds(OUT rcShapeBounds));

        IFC(FillPath(
            pContextState,
            pBrushContext,
            pShapeWiden,
            NULL,              // pmatShapeToDevice
            &rcShapeBounds,
            pStrokeBrush,
            matBaseSamplingToDevice
            ));
    }

#if DBG
    DbgDrawBoundingRectangles(
        pContextState,
        pBrushContext,
        pShape,
        pPen,
        *pmatShapeToDevice
        );
#endif

Cleanup:

    // Some failure HRESULTs should only cause the primitive
    // in question to not draw.
    IgnoreNoRenderHRESULTs(&hr);

    RRETURN(hr);

}


//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::DrawPath
//
//  Synopsis:  Small wrapper around DrawPathInternal that passed the WorldToDevice
//             matrix as ShapeToDevice.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::DrawPath(
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
        &static_cast<CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> const &>
         (pContextState->WorldToDevice),
        pBrushContext,
        pShape,
        pPen,
        pStrokeBrush,
        pFillBrush
        );
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::DrawInfinitePath
//
//  Synopsis:  Fill entire render target with brush.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::DrawInfinitePath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) BrushContext *pBrushContext,
    __inout_ecount(1) CBrushRealizer *pFillBrush
    )
{
    CParallelogram parallelogram;
    CMilRectF rect(
        static_cast<float>(m_rcBounds.left),
        static_cast<float>(m_rcBounds.top),
        static_cast<float>(m_rcBounds.right),
        static_cast<float>(m_rcBounds.bottom),
        LTRB_Parameters);

    parallelogram.Set(rect);
    
    return DrawPathInternal(
        pContextState,
        NULL,                   // Shape To Device
        pBrushContext,
        &parallelogram,
        NULL,
        NULL,
        pFillBrush
        );
}

#if DBG
//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::DbgDrawBoundingRectangles
//
//  Synopsis:  Draw the shape's associated bounding rectangles (stroke and fill)
//
//  Notes:     The stroke's bounding rectangle is guaranteed to be tight, but
//             the fill bounding rectangle is computed pre-transform and then
//             transformed, so for no axis-preserving transforms the bounds may
//             not be tight.
//
//-----------------------------------------------------------------------------
void
CHwSurfaceRenderTarget::DbgDrawBoundingRectangles(
    __in_ecount(1) const CContextState *pContextState,
    __in_ecount(1) BrushContext *pBrushContext,
    __in_ecount(1) const IShapeData *pShape,
    __in_ecount_opt(1) const CPlainPen *pPen,
    __in_ecount(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> &matWorldToDevice
    )
{
    if (IsTagEnabled(tagDisplayGeometryStrokeBounds))
    {
        if (pPen)
        {
            CRectF<CoordinateSpace::Device> rcShapeBounds;

            if (SUCCEEDED(pShape->GetTightBounds(
                        OUT rcShapeBounds,
                        pPen,
                        CMILMatrix::ReinterpretBase(&matWorldToDevice)
                        )))
            {
                MilColorF color = {0.0f /*r*/, 0.0f /*g*/, 1.0f /*b*/, 1.0f /*a*/};

                DbgDrawBox(
                    pContextState,
                    pBrushContext,
                    &rcShapeBounds,
                    &color
                    );
            }
        }
    }

    if (IsTagEnabled(tagDisplayGeometryBounds))
    {
        CRectF<CoordinateSpace::Shape> rcShapeBounds;
        CRectF<CoordinateSpace::Device> rcShapeDeviceBounds;

        if (SUCCEEDED(pShape->GetTightBounds(
                    OUT rcShapeBounds
                    )))
        {
            matWorldToDevice.Transform2DBoundsNullSafe(
                rcShapeBounds,
                rcShapeDeviceBounds
                );

            MilColorF color = {1.0f /*r*/, 0.0f /*g*/, 0.0f /*b*/, 1.0f /*a*/};

            DbgDrawBox(
                pContextState,
                pBrushContext,
                &rcShapeDeviceBounds,
                &color
                );
        }
    }

}

//+----------------------------------------------------------------------------
//
//  Function:
//      DbgReinterpretDeviceAsShape
//
//  Synopsis:
//      Helper method to reinterpret Device coordinate space as Shape
//      (LocalRendering) coordinate space.  This is useful when shape has been
//      flattened to Device and ShapeToDevice transform is set to identity, but
//      a help method is called that expects Shape based coordinates.
//
//-----------------------------------------------------------------------------

MIL_FORCEINLINE __returnro const CRectF<CoordinateSpace::Shape> &
DbgReinterpretDeviceAsShape(
    __in_ecount(1) const CRectF<CoordinateSpace::Device> &rc
    )
{
    C_ASSERT(sizeof(rc) == sizeof(CRectF<CoordinateSpace::Shape>));
    return reinterpret_cast<const CRectF<CoordinateSpace::Shape> &>(rc);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::DbgDrawBox
//
//  Synopsis:  Draw a box with coordinates prcBox
//
//-----------------------------------------------------------------------------
void
CHwSurfaceRenderTarget::DbgDrawBox(
    __in_ecount(1) const CContextState *pContextState,
    __in_ecount(1) BrushContext *pBrushContext,
    __in_ecount(1) CRectF<CoordinateSpace::Device> *prcBox,
    __in_ecount(1) MilColorF *pColor
    )
{
    CParallelogram boundsRect;
    CPlainPen boundsPen;
    CShape shapeWiden;
    CBrushRealizer *pBrushRealizer = NULL;

    if (!prcBox->IsEmpty())
    {
        CRectF<CoordinateSpace::Device> rcWidenedBounds(*prcBox);
        rcWidenedBounds.Inflate(.5,.5);

        boundsPen.Set(1 /* width */, 1 /* height */, 0 /* angle */);

        boundsRect.Set(*prcBox);

        if (SUCCEEDED(boundsRect.WidenToShape(
                    boundsPen,
                    DEFAULT_FLATTENING_TOLERANCE,
                    false,
                    shapeWiden,
                    NULL,                // pmatShapeToDevice
                    NULL                 // No bounds check
                    )))
        {
            if (SUCCEEDED(CBrushRealizer::CreateImmediateRealizer(
                            pColor,
                            &pBrushRealizer
                            )))
            {
                IGNORE_HR((FillPath(
                            pContextState,
                            pBrushContext,
                            &shapeWiden,
                            NULL,              // pmatShapeToDevice
                            &DbgReinterpretDeviceAsShape(rcWidenedBounds),
                            pBrushRealizer,
                            CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device>::refIdentity()
                            )));
            }
        }
    }

    ReleaseInterface(pBrushRealizer);
}
#endif // DBG

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTarget::ComposeEffect
//
//  Synopsis:
//      Uses the Meta-RT to find the associated Hw-RT, then applies the effect
//      to the underlying bitmap.
//------------------------------------------------------------------------------

STDMETHODIMP 
CHwSurfaceRenderTarget::ComposeEffect(
    __inout_ecount(1) CContextState *pContextState,
    __in_ecount(1) CMILMatrix *pScaleTransform,
    __inout_ecount(1) CMilEffectDuce* pEffect,
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt IMILRenderTargetBitmap* pImplicitInput
    ) 
{
    HRESULT hr = S_OK;
    CHwTextureRenderTarget* pTextureRTNoRef = NULL;
    CMetaBitmapRenderTarget* pMetaBitmapRT = NULL; 
    

    // We require that these are checked by the API proxy class.
    Assert(pContextState);

    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);
    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    if (!IsValid())
    {
        Assert(hr == S_OK);
        goto Cleanup;
    }

    IFC(EnsureState(pContextState));
    if (hr == WGXHR_CLIPPEDTOEMPTY)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // Setting the blend mode in the drawing context's render state has no
    //                effect because we don't use the hw shader pipeline.  We should create 
    //                a convention for setting the blend mode to ensure that it is set when 
    //                necessary in the future, perhaps in EnsureState (called above).
    IFC(m_pD3DDevice->SetAlphaBlendMode(&CD3DRenderState::sc_abmSrcOverPremultiplied));

    if (pImplicitInput != NULL)
    {
        // In the common scenario, pIBitmaps are meta render targets.
        hr = pImplicitInput->QueryInterface(
            IID_CMetaBitmapRenderTarget,
            reinterpret_cast<void **>(&pMetaBitmapRT));

        if (SUCCEEDED(hr))
        {
            IMILRenderTargetBitmap* pIBitmapRTNoRef = NULL;
            IFC(pMetaBitmapRT->GetCompatibleSubRenderTargetNoRef(m_pD3DDevice->GetRealizationCacheIndex(), m_associatedDisplay, &pIBitmapRTNoRef));
            pTextureRTNoRef = static_cast<CHwTextureRenderTarget*>(pIBitmapRTNoRef);
        }
        else
        {
            // If the QI fails, we are inside a visual brush which does not use meta RTs.  If that's
            // the case, we were directly handed a HW texture RT, since we force compatible RTs to be 
            // created (a HwSurfRT will only create HwTextureRTs for effects).
            hr = S_OK;
            pTextureRTNoRef = static_cast<CHwTextureRenderTarget*>(pImplicitInput);
        }

        // Since we've entered the device, we must ensure the textures we're operating on are valid.
        // After this point, EnsureState and SetAsRenderTarget assert that this is the case.
        if (!pTextureRTNoRef->IsValid())
        {
            Assert(hr == S_OK);
            goto Cleanup;
        }
    }

    IFC(pEffect->ApplyEffect(
        pContextState, 
        this,
        pScaleTransform, 
        m_pD3DDevice, 
        uIntermediateWidth,
        uIntermediateHeight,
        pTextureRTNoRef
        ));

       
Cleanup:
    ReleaseInterface(pMetaBitmapRT);

    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::HwShaderFillPath
//
//  Synopsis:  Fill the path with a Hw realized shader.
//
//             Note: Currently software fallback is not supported with this
//             2D shader code path.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::HwShaderFillPath(
    __in_ecount(1) const CContextState *pContextState,
    __in_ecount(1) CHwShader *pHwShader,
    __in_ecount(1) const IShapeData *pShapeData,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDeviceOrNULL,
    __in_ecount(1) const CMilRectL &rcRenderingBounds
    )
{
    HRESULT hr = S_OK;

    CHwRasterizer *pHwRasterizer = NULL;
    CFillTessellator *pFillTessellator = NULL;
    CDispensableBuffer<kMaxTessellatorSize, 1> localBuffer;
    IGeometryGenerator *pIGeometryGenerator = NULL;
    CDispensableBuffer<kMaxVertexBuilderSize, 3> dbScratch;

    Assert(m_pD3DDevice->IsInAUseContext());

    //
    // If we are anti-aliased, go to trapezoidal AA
    //

    if (pContextState->RenderState->AntiAliasMode != MilAntiAliasMode::None)
    {
        pHwRasterizer = new(&localBuffer) CHwRasterizer();
        IFCOOM(pHwRasterizer);

        IFC(pHwRasterizer->Setup(
            m_pD3DDevice,
            pShapeData,
            m_pD3DDevice->GetScratchPoints(),
            m_pD3DDevice->GetScratchTypes(),
            pmatShapeToDeviceOrNULL
            ));

        pIGeometryGenerator = pHwRasterizer;
    }
    else
    {
        IFC(pShapeData->SetupFillTessellator(
            pmatShapeToDeviceOrNULL,
            &localBuffer,
            &pFillTessellator
            ));

        pIGeometryGenerator = pFillTessellator;
    }

    if (!pIGeometryGenerator)
    {
        Assert(hr == WGXHR_EMPTYFILL);
        goto Cleanup;
    }

    Assert(hr == S_OK);

    //
    // Render the shape with the shader
    //
    IFC(pHwShader->DrawHwVertexBuffer(
        m_pD3DDevice,
        this,
        pIGeometryGenerator,
        &dbScratch,
        rcRenderingBounds,
        FALSE,
        m_fZBufferEnabled
        ));

Cleanup:
    if (hr == WGXHR_EMPTYFILL)
    {
        // Note that WGXHR_EMPTYFILL is a success code.  It means we didn't have to render.
        hr = S_OK;
    }

    if (hr == S_OK)
    {
        HW_DBG_RENDERING_STEP(HwShaderFillPath);
    }

    delete pFillTessellator;
    delete pHwRasterizer;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::SoftwareDrawGlyphs
//
//  Synopsis:  Use SW fallback to draw the glyph run
//
//-------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::SoftwareDrawGlyphs(
    __inout_ecount(1) DrawGlyphsParameters &pars,
    bool fTargetSupportsClearType,
    HRESULT hrReasonForFallback
    )
{
    HRESULT hr = S_OK;

    CMILBrush *pBrushNoRef = NULL;
    FLOAT flEffectAlpha;

    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    //
    // Realize the brush again for SW fallback
    //

    {
        CSwIntermediateRTCreator swRTCreator(
            MilPixelFormat::PBGRA32bpp,      // Tile texture format
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
        flEffectAlpha = pars.pBrushRealizer->GetOpacityFromRealizedBrush();
    }

    // The NULL case might not have been handled by the hardware DrawGlyphs
    // because sometimes we don't even try to draw glyphs in hardware (see
    // fCanDrawText)
    if (pBrushNoRef == NULL)
    {
        // Nothing to draw
        goto Cleanup;
    }

    //
    // Note: It is not necessary to call EnsureState because the brush
    //       realization is done in SW
    //

    CHwSoftwareFallback *pswFallback = NULL;

    IFC(m_pD3DDevice->GetSoftwareFallback(
        &pswFallback,
        hrReasonForFallback
        ));

    IFC(pswFallback->DrawGlyphs(
        pars,
        fTargetSupportsClearType,
        pBrushNoRef,
        flEffectAlpha,
        m_pD3DDevice->GetGlyphBank()->GetGlyphPainterMemory(),
        m_uWidth,
        m_uHeight
        ));

Cleanup:
    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::DrawGlyphs
//
//  Synopsis:  Draw the glyph run with a given brush.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwSurfaceRenderTarget::DrawGlyphs(
    __inout_ecount(1) DrawGlyphsParameters &pars
    )
{
    HRESULT hr = S_OK;
    
    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);
    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    //
    // Now that we have entered device scope, make sure render target is viable
    // for rendering.  Mostly this means that we haven't noticed a mode change
    // or have been called since then despite letting our caller know about the
    // mode change.
    //

    bool fTargetSupportsClearType = m_forceClearType || !HasAlpha();

    if (!IsValid())
    {
        Assert(hr == S_OK);
        goto Cleanup;
    }

    //
    // We can only draw text in hardware if the device is capable of it.
    //
    BOOL fAttemptHWText = m_pD3DDevice->CanDrawText();

    //
    // We can only draw text if the realized hardware brush will not need
    // waffling in order to work.
    //
    // Additionally we can only draw text in HW with a brush that uses source
    // clipping if the device supports border color.
    //
    if (   fAttemptHWText
        && (   (   m_pD3DDevice->SupportsTextureCap(D3DPTEXTURECAPS_POW2)
                && pars.pBrushRealizer->RealizedBrushMayNeedNonPow2Tiling(pars.pBrushContext)
               )
            || (pars.pBrushRealizer->RealizedBrushWillHaveSourceClip()
                && (   !m_pD3DDevice->SupportsBorderColor()
                    || !pars.pBrushRealizer->RealizedBrushSourceClipMayBeEntireSource(pars.pBrushContext)
                   )
               )
           )
        )
    {
        fAttemptHWText = FALSE;
    }

    //
    // Realize the HW MIL Brush only if we will need it
    //
    if (fAttemptHWText)
    {
        CMILBrush *pBrushNoRef = NULL;

        IFC(pars.pBrushRealizer->EnsureRealization(
            m_pD3DDevice->GetRealizationCacheIndex(),
            m_associatedDisplay,
            pars.pBrushContext,
            pars.pContextState,
            this
            ));
        pBrushNoRef = pars.pBrushRealizer->GetRealizedBrushNoRef(false /* fConvertNULLToTransparent */);

        if (pBrushNoRef == NULL)
        {
            // Nothing to draw
            goto Cleanup;
        }

        //
        // Because we implement source clipping using a transparent border
        // color, we can only draw text in HW with a brush that uses source
        // clipping if the source clip is equal to the entire bitmap size.
        //
        if (pBrushNoRef->GetType() == BrushBitmap)
        {
            const CMILBrushBitmap *pBrushBitmapNoRef = DYNCAST(CMILBrushBitmap, pBrushNoRef);
            Assert(pBrushBitmapNoRef);

            if (pBrushBitmapNoRef->HasSourceClip())
            {
                Assert(m_pD3DDevice->SupportsBorderColor());
                if (!pBrushBitmapNoRef->SourceClipIsEntireSource())
                {
                    //
                    // It might seem that we unnessarily pay for the cost of
                    // realizing the brush twice in this scenario, but in fact
                    // we don't. Whenever a brush realizes itself using an
                    // intermediate, SourceClipIsEntireSource will return TRUE.
                    // The only time we get here is with an image brush that
                    // does not realize to an intermediate, and
                    // EnsureRealization is smart enough to know that we don't
                    // need to re-realize brushes that do not use hw intermediates.
                    //
                    fAttemptHWText = FALSE;
                }
            }
        }
    }


    //
    // EnsureState must happen after brush realization since brush realization
    // can mess with device state. It also must happen before SW fallback.
    //
    IFC(EnsureState(pars.pContextState));
    if (hr == WGXHR_CLIPPEDTOEMPTY)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (fAttemptHWText)
    {
        //
        // Implement/use DeriveHwColorSource
        //   Rather than passing extra brush specific data around we should get a
        //   generic HW color source here and pass it down.
        //

        CD3DGlyphRunPainter painter;

        IFC(painter.Paint(
            pars,
            fTargetSupportsClearType,
            m_pD3DDevice,
            m_fmtTarget
            ));

        HW_DBG_RENDERING_STEP(DrawGlyphs);
    }
    else
    {
        hr = WGXERR_DEVICECANNOTRENDERTEXT;
    }

Cleanup:
    
    if (   hr == WGXERR_DEVICECANNOTRENDERTEXT
        || hr == E_NOTIMPL)
    {
        MIL_THR(SoftwareDrawGlyphs(
            pars, 
            fTargetSupportsClearType,
            hr
            ));
        
        if (SUCCEEDED(hr))
        {
            HW_DBG_RENDERING_STEP(SoftwareDrawGlyphs);
        }
    }
    // Some failure HRESULTs should only cause the primitive
    // in question to not draw.
    IgnoreNoRenderHRESULTs(&hr);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::CreateRenderTargetBitmap
//
//  Synopsis:  Create a render target that renders to a new secondary
//             surface intended to become a source for this render
//             target in the near future
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwSurfaceRenderTarget::CreateRenderTargetBitmap(
    UINT uWidth,
    UINT uHeight,
    IntermediateRTUsage usageInfo,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
    __in_opt DynArray<bool> const *pActiveDisplays
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pActiveDisplays);
    
    Assert(ppIRenderTargetBitmap);

    Assert(m_pD3DDevice);
    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

    *ppIRenderTargetBitmap = NULL;

    // The width and height are converted to floats when clipping,
    // make sure we don't expect values TOO big as input.
    if (uWidth > MAX_INT_TO_FLOAT || uHeight > MAX_INT_TO_FLOAT)
    {
        IFC(WGXERR_UNSUPPORTEDTEXTURESIZE);
    }

    bool fWrapModeForcesSW = false;
    if (usageInfo.wrapMode != MilBitmapWrapMode::Extend)
    {
        if (usageInfo.flags & IntermediateRTUsage::ForUseIn3D)
        {
            //
            // Because all 3D tiled intermediates are power of 2 dimensions, we
            // should try to hardware accelerate them all.
            //
            Assert(uWidth == RoundToPow2(uWidth));
            Assert(uHeight == RoundToPow2(uHeight));
        }
        else
        {
            //
            // In 2D, some intermediates will be pow-2 and others will not. For
            // consistency, cause all tiled intermediates to be in software
            //
            // Future Consideration:   If we ever support waffling for
            // intermediates, we should change this.
            //

            fWrapModeForcesSW = true;
        }
    }

    //  Both hw/sw ignore scRGB here

    if (   !fWrapModeForcesSW
        && dwFlags != MilRTInitialization::SoftwareOnly
        && !m_pD3DDevice->IsSWDevice() // use our SW rasterizer instead of RGB rast for intermediates
        && m_pD3DDevice->GetRealizationCacheIndex() != CMILResourceCache::InvalidToken
        )
    {
        //
        // Create a texture render target
        //

        CHwTextureRenderTarget *pTextureRT;

        //
        // NOTICE-2006/05/22-milesc  Don't try to fallback to a software
        // intermediate here on D3DERR_OUTOFVIDEOMEMORY. Falling back here
        // would only delay the problem, since you still need video memory to
        // draw a software intermediate.
        //
        IFC(CHwTextureRenderTarget::Create(
            uWidth,
            uHeight,
            m_pD3DDevice,
            m_associatedDisplay,
            (usageInfo.flags & IntermediateRTUsage::ForBlending) ? true : false,
            &pTextureRT
            DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
            ));

        SetUsedToCreateHardwareRT();

        // Assign return value and steal reference
        *ppIRenderTargetBitmap = pTextureRT;

        // Increment our HW Intermediate Render Target counter
        if (g_pMediaControl)
        {
            // Add one to our count of IRTs used this frame
            InterlockedIncrement(reinterpret_cast<LONG *>(
                &(g_pMediaControl->GetDataPtr()->NumHardwareIntermediateRenderTargets)
                ));
        }
    }
    else if (dwFlags == MilRTInitialization::ForceCompatible)
    {
        // If we cannot create a hw render target, but the flags specify that we
        // cannot create a software one from a hardware surface, we must fail.
        IFC(E_FAIL);
    }
    else
    {
        // We do not support the wrap mode yet,
        // so we create a SW rendertarget.

        // NOTE-2003/05/21-chrisra This may result in several copies of the same
        //  SW rendertarget (one for each adapter), but this is the simplest and
        //  safest approach for now.
        // NOTE on NOTE-2005/09/30-milesc We may not actually generate several copies
        // for brushes that are realized by the internal render target. See
        // CBrushRealizer::EnsureRealization for details.

        // NOTE-2003/05/20-chrisra We are grabbing the DPI from the
        // horizontal and vertical scale of the device transform.  If
        // anyone ever changes the usage of the device transform this
        // will probably have to be fixed.

        IFC(CSwRenderTargetBitmap::Create(
            uWidth,
            uHeight,
            MilPixelFormat::PBGRA32bpp,
            m_DeviceTransform.GetM11(), // Horizontal DPI
            m_DeviceTransform.GetM22(), // Vertical DPI
            m_associatedDisplay,
            ppIRenderTargetBitmap
            DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::BeginLayerInternal
//
//  Synopsis:  Begin accumulation of rendering into a layer.  Modifications to
//             layer, as specified in arguments, are handled and result is
//             applied to render target when the matching EndLayer call is
//             made.
//
//             Calls to BeginLayer may be nested, but other calls that depend
//             on the current contents, such as Present, are not
//             allowed until all layers have been resolved with EndLayer.
//

HRESULT CHwSurfaceRenderTarget::BeginLayerInternal(
    __inout_ecount(1) CRenderTargetLayer *pNewLayer
    )
{
    HRESULT hr = S_OK;

    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);
    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    if (!IsValid())
    {
        Assert(hr == S_OK);
        goto Cleanup;
    }

    EventWriteLayerEventStart();

    CMILSurfaceRect rgCopyRects[MAX_NUM_PARTIAL_LAYER_CAPTURE_RECTS];
    UINT cCopyRects = 0;
    bool fCopyEntireLayer = true;

    //
    // Check for cases that are not supported.
    //
    //  a) an alpha mask
    //
    if (pNewLayer->pAlphaMaskBrush)
    {
        IFC(E_NOTIMPL);
    }

    //
    // Check to see if we can avoid copying the entire layer.
    // Right now the only case we handle is an aliased geometric mask shape that
    // is an axis aligned rectangle.
    // If there is an alpha scale, we will need the entire bitmap anyway.
    //
    // We need to check HasAlpha() because EndLayerInternal special-cases
    // HasAlpha() and wants the entire bounds in that case.
    //
    const BOOL fAttemptToCopyPartialLayer = !HasAlpha();

    if (fAttemptToCopyPartialLayer)
    {
        fCopyEntireLayer = !GetPartialLayerCaptureRects(
            pNewLayer,
            rgCopyRects,
            &cCopyRects
            );
    }

    //
    // Create backup of current surface within layer bounds if such a backup is necessary
    //
    // Note: Layer has ownership of created resource
    //
    if (   fCopyEntireLayer
        || cCopyRects > 0
        )
    {
        if (!fCopyEntireLayer)
        {
            Assert(cCopyRects >= 1);
            Assert(cCopyRects <= ARRAY_SIZE(rgCopyRects));
        }

        //
        // Load Rendertarget data into the destination texture.
        //
        IFC(m_pD3DDevice->GetHwDestinationTexture(
            this,
            pNewLayer->rcLayerBounds,
            fCopyEntireLayer ? NULL : rgCopyRects, // prgSubDestCopyRects
            fCopyEntireLayer ? 0 : cCopyRects,  // crgSubDestCopyRects
            &pNewLayer->oTargetData.m_pSourceBitmap
            ));

        //
        // If RT has alpha we clear the portion of the RT under the layer to transparent because
        // we can't just use source over to bring back the saved layer in EndLayerInternal if the
        // saved layer isn't opaque.  Instead we will render onto transparent and then do an "under"
        // (just like over but under) operation with the saved layer.
        //
        // Before removing this clear and changing EndLayerInternal consider that this
        // handles an extremely subtle "bug" in D3D behavior where some blend operations
        // that should, using perfect math, result in a pixel being unchanged actually are
        // off by 1.  In push/pop layer we cannot afford to be off by even 1 for pixels
        // that aren't touched by any rendering in the layer.  See Windows OS Bug #
        // 1134646 for the original case where this showed up.
        if (HasAlpha())
        {
            // For now the copy part code is not enabled.  If it
            // is we need to handle clearing just the regions copied.
            Assert(fCopyEntireLayer);

            IFC(m_pD3DDevice->ColorFill(
                m_pD3DTargetSurface->ID3DSurface(),
                CMILSurfaceRectAsRECT(&pNewLayer->rcLayerBounds),
                0 // Transparent
                ));
        }
    }

Cleanup:
    EventWriteLayerEventEnd();

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::HasAlpha
//
//  Synopsis:  Does this render target have alpha.
//

bool CHwSurfaceRenderTarget::HasAlpha() const
{
    switch (m_fmtTarget)
    {
    case MilPixelFormat::PBGRA32bpp:
    case MilPixelFormat::PRGBA64bpp:
    case MilPixelFormat::PRGBA128bppFloat:
        return true;
    default:
        return false;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::EndLayerInternal
//
//  Synopsis:  End accumulation of rendering into current layer.  Modifications
//             to layer, as specified in BeginLayer arguments, are handled and
//             result is applied to render target.
//

HRESULT CHwSurfaceRenderTarget::EndLayerInternal(
    )
{
    HRESULT hr = S_OK;

    CFillTessellator *pFillTessellator = NULL;
    CHwRasterizer *pHwRasterizer = NULL;
    IGeometryGenerator *pIGeometryGenerator = NULL;
    CDispensableBuffer<kMaxTessellatorSize, 1> localBuffer;

    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    CShape *pScratchFill = m_pD3DDevice->GetScratchFillShape();

    CRenderTargetLayer const &layer = m_LayerStack.Top();

    Assert(layer.oTargetData.m_pSourceBitmap);

    DbgResetStateUponTraceTag();

    //
    // We assume we have rendering to do
    //
    Assert(   (layer.pGeometricMaskShape != NULL)
           || (!AlphaScalePreservesOpacity(layer.rAlpha))
           || (layer.pAlphaMaskBrush != NULL)
           );


    //
    // Prepare for rendering
    //

    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

    //
    // Prepare the HW brushes
    //

    MilColorF opaqueBlack = { 0,0,0,1 };
    CHwSolidBrush oHwBlackBrush(m_pD3DDevice);
    oHwBlackBrush.SetColor(opaqueBlack);
    CHwTexturedColorSourceBrush oHwBB(m_pD3DDevice, layer.oTargetData.m_pSourceBitmap);

    // If we don't have alpha we use the inverse clip geometry / PushOpacity geometry
    // to put the saved layer back using a SrcOver operation.  Otherwise we do the
    // same over operation but neglect to add the source (this is what
    // MilCompositingMode::SourceInverseAlphaMultiply does.)  Then we put the saved layer
    // "under" the current rendering afterwards.
    //
    // We use source over NonPremultiplied because of a subtle D3D weirdity.  If we multiply the
    // texture color by the alpha in the texture blend and then do a SourceOver in the alpha blend
    // the result can be off-by-one which is unacceptable in end layer.  Moving the alpha multiply
    // into the alpha blend fixes this.  Though it is not clear that this is guaranteed by anybody.
    // What we need here is the ability to OVER a color with itself and get the color back regardless
    // of alpha.
    //
    // If we are doing anti-aliased rendering we can tell the geometry builder to produce zero-alpha
    // areas outside the clip shape & use a blend mode that complements the alpha.  (In fact we can
    // scale the alpha prior to complementing it to handle PushOpacity.  The blend modes for this
    // case are stored in eCompositingModeForRegularMaskAlpha (because the alpha is equal to the
    // layer alpha.)  If we are doing aliased rendering we have to complement the geometry using
    // Combine and therefore the blend mode (eCompositingModeForComplementedMaskAlpha) needs to
    // handle the inverted alpha.

    CHwBrush *pFixupBrush = HasAlpha() ? (CHwBrush*) &oHwBlackBrush
                                       : (CHwBrush*) &oHwBB;
    MilCompositingMode::Enum eCompositingModeForComplementedMaskAlpha
        =   HasAlpha()
        ? MilCompositingMode::SourceInverseAlphaMultiply
        : MilCompositingMode::SourceOverNonPremultiplied;

    MilCompositingMode::Enum eCompositingModeForRegularMaskAlpha
        = HasAlpha()
        ? MilCompositingMode::SourceAlphaMultiply
        : MilCompositingMode::SourceInverseAlphaOverNonPremultiplied;

    //
    // Layer bounding rect
    //

    CRectF<CoordinateSpace::Device> rcLayerFloat(
        static_cast<FLOAT>(layer.rcLayerBounds.left),
        static_cast<FLOAT>(layer.rcLayerBounds.top),
        static_cast<FLOAT>(layer.rcLayerBounds.right),
        static_cast<FLOAT>(layer.rcLayerBounds.bottom),
        LTRB_Parameters
        );

    //
    // Effect context
    //
    // NOTICE-2006/06/13-JasonHa  Effect context is not actually used.
    //   Opacity mask has yet to be wired through Begin/EndLayer, but all
    // methods that accept an effect list expect to have effect context. 
    // This effect context is effectively a dummy though it is believed to
    // be completely correct.

    CContextState oContextState(TRUE);
    CHwBrushContext oEffectContext(
        &oContextState, CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device>::refIdentity(),
        m_fmtTarget, FALSE
        );
    oEffectContext.SetDeviceSamplingBounds(rcLayerFloat);


    //
    // Create a layer bounding shape
    //

    CShape boundShape;

    IFC(boundShape.AddRect(rcLayerFloat));


    //
    // Now that we have entered device scope, make sure render target is viable
    // for rendering.  Mostly this means that we haven't noticed a mode change
    // or have been called since then despite letting our caller know about the
    // mode change.
    //

    if (!IsValid())
    {
        Assert(hr == S_OK);
        goto Cleanup;
    }

    // Now set the target
    IFC(SetAsRenderTarget());

    // then the clip
    IFC(m_pD3DDevice->SetClipRect(&m_rcCurrentClip));
    Assert(hr != WGXHR_CLIPPEDTOEMPTY);

    // and finally basic 2D state.
    IFC(Ensure2DState());


    //
    // Render fixups
    //

    bool fNeedConstantAlphaFixup = !AlphaScalePreservesOpacity(layer.rAlpha);

    //
    // Check for geometric mask fixups
    //

    if (layer.pGeometricMaskShape)
    {
        // These initial values disable complement rendering.
        const CMILSurfaceRect *prcComplementBounds = NULL;
        bool fNeedInside = true;

        //
        // If we are anti-aliased, go to trapezoidal AA
        //

        MilCompositingMode::Enum eCompositingMode;

        const IMILEffectList *pEffectListNoRef = NULL;
        CEffectList effectList;
        AlphaScaleParams alphaParams;

        if (layer.AntiAliasMode != MilAntiAliasMode::None)
        {
            pHwRasterizer = new(&localBuffer) CHwRasterizer();
            IFCOOM(pHwRasterizer);

            IFC(pHwRasterizer->Setup(
                m_pD3DDevice,
                layer.pGeometricMaskShape,
                m_pD3DDevice->GetScratchPoints(),
                m_pD3DDevice->GetScratchTypes(),
                NULL
                ));

            prcComplementBounds = &layer.rcLayerBounds;
            fNeedInside = fNeedConstantAlphaFixup;

            fNeedConstantAlphaFixup = false; // Complement handles this also.
            eCompositingMode = eCompositingModeForRegularMaskAlpha;
            pIGeometryGenerator = pHwRasterizer;

            alphaParams.scale = layer.rAlpha;

            // Set AlphaScale effect
            IFC(effectList.Add(
                CLSID_MILEffectAlphaScale,
                sizeof(alphaParams),
                &alphaParams
                ));

            pEffectListNoRef = &effectList;
        }
        else
        {
            //
            // Create an inverted geometry to simulate coverage inversion
            //

            pScratchFill->Reset();

            IFC(CShapeBase::Combine(
                    &boundShape,
                    layer.pGeometricMaskShape,
                    MilCombineMode::Xor,
                    false,  // ==> Do not retrieve curves from the flattened result
                    pScratchFill
                    ));

            IFC(pScratchFill->SetupFillTessellator(
                NULL,
                &localBuffer,
                &pFillTessellator
                ));

            eCompositingMode = eCompositingModeForComplementedMaskAlpha;
            pIGeometryGenerator = pFillTessellator;
        }

        if (pIGeometryGenerator)
        {
            Assert(hr == S_OK);

            //
            // Draw the shape
            //

            IFC(AcceleratedFillPath(
                eCompositingMode,
                pIGeometryGenerator,
                pFixupBrush,
                pEffectListNoRef,
                &oEffectContext,
                prcComplementBounds,
                fNeedInside
                ));
        }
        else
        {
            //
            // The only success value we can have with a NULL pIGeometryGenerator
            //
            Assert(hr == WGXHR_EMPTYFILL);
            hr = S_OK;
        }
    }


    //
    // Check for constant opacity fixups
    //
    // Use an inverted opacity scale restore original target colors
    //

    if (fNeedConstantAlphaFixup)
    {
        AlphaScaleParams alphaParams;
        alphaParams.scale = 1.0f - layer.rAlpha;

        Assert(!AlphaScaleEliminatesRenderOutput(alphaParams.scale));

        // Clean up previous geometry generators if there were any so that we
        // can setup a fill tesselator from a clean tack allocator object.
        delete pFillTessellator;
        pFillTessellator = NULL;
        delete pHwRasterizer;
        pHwRasterizer = NULL;

        CEffectList effectList;

        // Set AlphaScale effect
        IFC(effectList.Add(
            CLSID_MILEffectAlphaScale,
            sizeof(alphaParams),
            &alphaParams
            ));

        // This operation is pixel aligned so no antialiasing is needed.
        // A specialized large triangle class may be appropriate here.
        IFC(boundShape.SetupFillTessellator(
            NULL,
            &localBuffer,
            &pFillTessellator
            ));

        // We shouldn't even be in EndLayerInternal if the bounds are empty
        Assert(hr == S_OK);
        Assert(pFillTessellator);

        pIGeometryGenerator = pFillTessellator;

        //
        // Draw the shape
        //

        IFC(AcceleratedFillPath(
            eCompositingModeForComplementedMaskAlpha,
            pIGeometryGenerator,
            pFixupBrush,
            &effectList,
            &oEffectContext
            ));
    }

    //
    // If RT has alpha we must "under" the backed up surface.
    //
    if (HasAlpha())
    {

        // Clean up previous geometry generators if there were any so that we
        // can setup a fill tesselator from a clean tack allocator object.
        delete pFillTessellator;
        pFillTessellator = NULL;
        delete pHwRasterizer;
        pHwRasterizer = NULL;


        // This operation is pixel aligned so no antialiasing is needed.
        // A specialized large triangle class may be appropriate here.
        IFC(boundShape.SetupFillTessellator(
            NULL,
            &localBuffer,
            &pFillTessellator
            ));

        pIGeometryGenerator = pFillTessellator;

        //
        // Draw the shape
        //

        IFC(AcceleratedFillPath(
            MilCompositingMode::SourceUnder,
            pIGeometryGenerator,
            &oHwBB,
            NULL,
            &oEffectContext
            ));
    }

    HW_DBG_RENDERING_STEP(EndLayer);


Cleanup:

    delete pFillTessellator;
    delete pHwRasterizer;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:  CHwSurfaceRenderTarget::GetNumQueuedPresents
//
//  Synopsis:  Forward the call to the CMetaRenderTarget member.
//
//-------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    HRESULT hr = S_OK;

    if (m_pD3DDevice && IsValid())
    {
        ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

        IFC(m_pD3DDevice->GetNumQueuedPresents(
            puNumQueuedPresents
            ));
    }
    else
    {
        *puNumQueuedPresents = 0;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::DrawVideo
//
//  Synopsis:
//        Draw the video. This path is used when video is drawn
//        using an intermediate surface. Optionally Video could be
//        drawn directly to the backbuffer as a perf optimization
//        Look at CHwDisplayRenderTarget::DrawVideo for more.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwSurfaceRenderTarget::DrawVideo(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __inout_ecount_opt(1) IWGXBitmapSource *pBitmapSource,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;
    IWGXBitmapSource *pWGXBitmapSource = NULL;
    bool fSavePrefilterEnable = pContextState->RenderState->PrefilterEnable;
    BOOL fDrewVideo = FALSE;

    Assert(pSurfaceRenderer || pBitmapSource);
    Assert(m_pD3DDevice);

    ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);
    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    //
    // Now that we have entered device scope, make sure render target is viable
    // for rendering.  Mostly this means that we haven't noticed a mode change
    // or have been called since then despite letting our caller know about the
    // mode change.
    //

    if (!IsValid())
    {
        Assert(hr == S_OK);
        goto Cleanup;
    }

    //
    // The very last call in DrawVideoToSurface is BeginRender, every BeginRender,
    // *must* have an EndRender.
    //
    if (NULL != pSurfaceRenderer)
    {
        IFC(m_pD3DDevice->DrawVideoToSurface(pSurfaceRenderer, &pWGXBitmapSource));

        fDrewVideo = TRUE;
    }
    else
    {
        //
        // In this case, we don't have to call EndRender because the bitmap source
        // was supplied.
        //
        SetInterface(pWGXBitmapSource, pBitmapSource);
    }

    //
    // Workaround for people playing audio files using a MediaElement (common case).
    //
    if (pWGXBitmapSource == NULL)
    {
        goto Cleanup;
    }

    // Disable prefiltering for video.
    pContextState->RenderState->PrefilterEnable = false;
    IFC(CHwSurfaceRenderTarget::DrawBitmap(pContextState, pWGXBitmapSource, pIEffect));

    HW_DBG_RENDERING_STEP(DrawVideo);

Cleanup:

    if (fDrewVideo)
    {
        IGNORE_HR(pSurfaceRenderer->EndRender());
    }

    ReleaseInterface(pWGXBitmapSource);
    pContextState->RenderState->PrefilterEnable = fSavePrefilterEnable;
    
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::Ensure3DRenderTarget
//
//  Synopsis:  Ensure that a render target is available for 3D rendering
//

void
CHwSurfaceRenderTarget::Ensure3DRenderTarget(
    D3DMULTISAMPLE_TYPE MultisampleType
    )
{
    //
    // Default to using current render target independent of multisample type
    //

    m_pD3DTargetSurfaceFor3DNoRef = m_pD3DTargetSurface;

    //
    // Check if default target is sufficient
    //
    //  If desired multisample type is none, but target is multisample then go
    //  with default target and disable multisample via render state later.
    //

    if (   MultisampleType != D3DMULTISAMPLE_NONE
        && m_pD3DTargetSurface->Desc().MultiSampleType != MultisampleType)
    {
        //
        // Future Consideration:   Restrict intermediate 3D surface to need
        //  Currently an intermediate the size of this target is used.  To use
        //  a smaller size the transforms employed in DrawMesh3D will need
        //  adjusted (as well and the pre- and post-3D copies.)
        //

        UINT uMinWidth = m_uWidth;
        UINT uMinHeight = m_uHeight;

        //
        // Default is not sufficient
        //
        // Check for an existing intermediate that is in sufficient, note that
        // size changes for this render target fully releases all targets.
        //

        if (m_pD3DIntermediateMultisampleTargetSurface)
        {
            D3DSURFACE_DESC const &d3dsdIntermediate =
                m_pD3DIntermediateMultisampleTargetSurface->Desc();

            if (   d3dsdIntermediate.Width < uMinWidth
                || d3dsdIntermediate.Height < uMinHeight
                || d3dsdIntermediate.MultiSampleType != MultisampleType
               )
            {
                // Insufficient intermediate - release it
                m_pD3DIntermediateMultisampleTargetSurface->Release();
                m_pD3DIntermediateMultisampleTargetSurface = NULL;
            }
        }

        //
        // Allocate new intermediate multisample buffer as needed.
        //
        // Future Consideration:   Share intermediate multisample targets
        //  rather than have each RT allocate its own.
        //

        if (!m_pD3DIntermediateMultisampleTargetSurface)
        {
            HRESULT hr = S_OK;

            //
            // Future Consideration:   Improve intermediate growth
            //  especially in resize scenarios as it is always released.
            //

            IFC(m_pD3DDevice->CreateRenderTarget(
                uMinWidth,
                uMinHeight,
                m_pD3DTargetSurface->Desc().Format,
                MultisampleType, 0,
                FALSE,
                &m_pD3DIntermediateMultisampleTargetSurface
                ));
        }

        //
        // Success - Use intermediate for 3D rendering
        //

        m_pD3DTargetSurfaceFor3DNoRef = m_pD3DIntermediateMultisampleTargetSurface;
    }

Cleanup:

    return;
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::EnsureDepthState
//
//  Synopsis:  Ensures that the depth buffer is set correctly.
//
//-------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::EnsureDepthState(
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDevice != NULL);

    //
    // We need to get the actual surface size since it can differ from the
    // expected Target Size.  (Could be rounded up to a power of 2)
    //

    D3DSURFACE_DESC const &descTargetSurface = m_pD3DTargetSurfaceFor3DNoRef->Desc();

    // Release the depth buffer if it's not valid or the wrong size
    if (m_pD3DStencilSurface != NULL)
    {
        D3DSURFACE_DESC const &descDepthSurface = m_pD3DStencilSurface->Desc();


        // If there is a change in a depth buffer being required or clip
        // buffer being required we will force a recreation of the
        // surface.  We should investigate minimizing these re-creations.

        if (   !m_pD3DStencilSurface->IsValid()
            || descDepthSurface.Width < descTargetSurface.Width
            || descDepthSurface.Height < descTargetSurface.Height
            || descDepthSurface.MultiSampleType != descTargetSurface.MultiSampleType
           )
        {
            // Release the depth buffer now so that it will
            // be recreated in the next if statement
            ReleaseInterface(m_pD3DStencilSurface);
        }
    }

    // Create the buffer if it's needed
    if (m_pD3DStencilSurface == NULL)
    {
        IFC(m_pD3DDevice->CreateDepthBuffer(
            descTargetSurface.Width,
            descTargetSurface.Height,
            descTargetSurface.MultiSampleType,
            &m_pD3DStencilSurface
            ));
    }

    IFC(m_pD3DDevice->SetDepthStencilSurface(m_pD3DStencilSurface));

Cleanup:
    if (FAILED(hr))
    {
        IGNORE_HR(m_pD3DDevice->SetDepthStencilSurface(NULL));
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::EnsureClip
//
//  Synopsis:  Ensures that the clip is properly set
//
//-------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::EnsureClip(
    __in_ecount(1) const CContextState *pContextState
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDevice != NULL);
    Assert(pContextState != NULL);

    if (UpdateCurrentClip(pContextState->AliasedClip))
    {
        IFC(m_pD3DDevice->SetClipRect(&m_rcCurrentClip));
    }
    else
    {
        hr = WGXHR_CLIPPEDTOEMPTY;
    }

Cleanup:

    RRETURN1(hr, WGXHR_CLIPPEDTOEMPTY);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::SetAsRenderTarget, SetAsRenderTargetFor3D
//
//  Synopsis:  Since we can have multiple render target per D3D device each
//             time we want to have D3D target rendering to one of our render
//             targets we need to make sure it is the current D3D target.
//
//             Note that this call is a quick no-ops in the common case where
//             we are rendering many primitives with one render target.  In the
//             case where we are not, this call is incredibly expensive for D3D
//             state changes.
//

HRESULT
CHwSurfaceRenderTarget::SetAsRenderTarget(
    )
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*m_pD3DDevice);
    Assert(IsValid());

    IFC(m_pD3DDevice->SetRenderTarget(
        m_fIn3D ?
        m_pD3DTargetSurfaceFor3DNoRef :
        m_pD3DTargetSurface
        ));

Cleanup:

    RRETURN(hr);
}

HRESULT
CHwSurfaceRenderTarget::SetAsRenderTargetFor3D(
    )
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*m_pD3DDevice);
    Assert(IsValid());

    IFC(m_pD3DDevice->SetRenderTarget(m_pD3DTargetSurfaceFor3DNoRef));

Cleanup:

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::Ensure2DState
//
//  Synopsis:  Set up 2D specific render state (mostly disable 3D state)
//

HRESULT
CHwSurfaceRenderTarget::Ensure2DState(
    )
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*m_pD3DDevice);
    Assert(IsValid());

    //
    // Set 2D specific state
    //

    IFC(m_pD3DDevice->SetDepthStencilSurface(NULL));

    //
    // Future Consideration:  Remove this Set2DTransformForFixedFunction
    //
    // Currently we have some shader code that will extract the 2d transform
    // from what's set in the fixed function transforms, so we still need to
    // set these for fixed function here.  We should change that code so we set
    // either fixed function or shader transforms right before we render and
    // remove this call here.
    //
    IFC(m_pD3DDevice->Set2DTransformForFixedFunction());

    IFC(m_pD3DDevice->SetRenderState(
        D3DRS_CULLMODE,
        D3DCULL_NONE
        ));

    IFC(m_pD3DDevice->SetRenderState(
        D3DRS_ZFUNC,
        D3DCMP_LESSEQUAL
        ));

    IFC(m_pD3DDevice->SetRenderState(
        D3DRS_ZWRITEENABLE,
        FALSE
        ));

    if (m_pD3DTargetSurface->Desc().MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        IFC(m_pD3DDevice->SetRenderState(
            D3DRS_MULTISAMPLEANTIALIAS,
            FALSE
            ));
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwSurfaceRenderTarget::Ensure3DState
//
//  Synopsis:  Set up 3D specific render state
//

HRESULT
CHwSurfaceRenderTarget::Ensure3DState(
    __in_ecount(1) CContextState const *pContextState
    )
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*m_pD3DDevice);
    Assert(IsValid());

    //
    // Set 3D specific state
    //

    IFC(m_pD3DDevice->Set3DTransforms(
        &pContextState->WorldTransform3D,
        &pContextState->ViewTransform3D,
        &pContextState->ProjectionTransform3D,
        pContextState->ViewportProjectionModifier3D
        ));

    IFC(m_pD3DDevice->SetRenderState(
        D3DRS_CULLMODE,
        pContextState->CullMode3D
        ));

    if (m_fZBufferEnabled)
    {
        Assert(m_pD3DStencilSurface);
        IFC(m_pD3DDevice->SetDepthStencilSurface(m_pD3DStencilSurface));

        IFC(m_pD3DDevice->SetRenderState(
            D3DRS_ZFUNC,
            pContextState->DepthBufferFunction3D
            ));
    }
    else
    {
        IFC(m_pD3DDevice->SetDepthStencilSurface(NULL));
    }

    if (m_pD3DTargetSurfaceFor3DNoRef->Desc().MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        IFC(m_pD3DDevice->SetRenderState(
            D3DRS_MULTISAMPLEANTIALIAS,
            pContextState->RenderState->AntiAliasMode != MilAntiAliasMode::None
            ));
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::EnsureState
//
//  Synopsis:  Since we can have multiple render targets per d3ddevice,
//             each call to a public method on the render target must
//             ensure the state it expects on the render target.
//
//             Note that most of these calls are quick no-opts in the
//             common case where we are rendering many primitives
//             with one render target.
//
//-------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::EnsureState(
    __in_ecount(1) CContextState const *pContextState
    )
{
    HRESULT hr = S_OK;

    AssertDeviceEntry(*m_pD3DDevice);
    Assert(IsValid());

    DbgResetStateUponTraceTag();

    //
    // Ensure we have the right render target set.
    //

    IFC(SetAsRenderTarget());

    //
    // Set the clip.
    //

    IFC(EnsureClip(pContextState));
    if (hr == WGXHR_CLIPPEDTOEMPTY)
    {
        goto Cleanup;
    }

    //
    // We're beginning a primitive which means that we don't have to hold onto
    // any of previous primitive's resources.  Let the device know so it may do
    // any required cleanup.
    //
    m_pD3DDevice->ResetPerPrimitiveResourceUsage();


    if (pContextState->In3D)
    {
        //
        // Set 3D specific state
        //

        IFC(Ensure3DState(
            pContextState
            ));
    }
    else
    {
        //
        // Set 2D specific state
        //

        IFC(Ensure2DState());
    }

Cleanup:
    RRETURN1(hr, WGXHR_CLIPPEDTOEMPTY);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::GetPixelFormat
//
//  Synopsis:  Get pixel format of render target
//
//-------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
    )
{
    HRESULT hr = S_OK;

    Assert(pPixelFormat);

    *pPixelFormat = m_fmtTarget;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::GetSize
//
//  Synopsis:  Get size of render target
//
//-------------------------------------------------------------------------
HRESULT CHwSurfaceRenderTarget::GetSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    ) const
{
    HRESULT hr = S_OK;

    *puWidth = m_uWidth;
    *puHeight = m_uHeight;

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::GetHwDestinationTexture
//
//  Synopsis:  Retrieves a destination texture, using one created for the
//             current layer if possible.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::GetHwDestinationTexture(
    __in_ecount(1) const CMILSurfaceRect &rcDestRect,
    __in_ecount_opt(crgSubDestCopyRects) const CMILSurfaceRect *prgSubDestCopyRects,
    UINT crgSubDestCopyRects,
    bool fUseLayeredDestinationTexture,
    __deref_out_ecount(1) CHwDestinationTexture **ppHwDestinationTexture
    )
{
    HRESULT hr = S_OK;
    bool fHaveSuitableCachedTexture = false;

    if (!fHaveSuitableCachedTexture)
    {
        IFC(m_pD3DDevice->GetHwDestinationTexture(
            this,
            rcDestRect,
            prgSubDestCopyRects,
            crgSubDestCopyRects,
            ppHwDestinationTexture
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::PopulateDestinationTexture
//
//  Synopsis:  Copies the rendertarget into a texture.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTarget::PopulateDestinationTexture(
    __in_ecount(1) const CMILSurfaceRect *prcSource,
    __in_ecount(1) const CMILSurfaceRect *prcDest,
    __inout_ecount(1) IDirect3DTexture9 *pD3DTexture
    )
{
    HRESULT hr = S_OK;
    IDirect3DSurface9 *pD3DSurface = NULL;

    ENTER_USE_CONTEXT_FOR_SCOPE(*m_pD3DDevice);

    Assert(prcSource->left != prcSource->right);
    Assert(prcSource->top != prcSource->bottom);

    Assert(prcDest->left != prcDest->right);
    Assert(prcDest->top != prcDest->bottom);

    Assert((prcDest->right - prcDest->left) == (prcSource->right - prcSource->left));
    Assert((prcDest->bottom - prcDest->top) == (prcSource->bottom - prcSource->top));

    IFC(pD3DTexture->GetSurfaceLevel(
        0,
        &pD3DSurface
        ));

    const D3DTEXTUREFILTERTYPE d3dFilter = D3DTEXF_NONE;

    IFC(m_pD3DDevice->StretchRect(
        m_pD3DTargetSurface,
        prcSource,
        pD3DSurface,
        prcDest,
        d3dFilter
        ));

Cleanup:
    ReleaseInterface(pD3DSurface);
    
    RRETURN(hr);
}

#if DBG
//+----------------------------------------------------------------------------
//
//  Function:  CHwSurfaceRenderTarget::DbgResetStateUponTraceTag
//
//  Synopsis:  If the trace tag is set, this function resets all renderstate
//             to the default values. This can be used to discover funky
//             device state bugs
//
//-----------------------------------------------------------------------------
void
CHwSurfaceRenderTarget::DbgResetStateUponTraceTag()
{
    if (IsTagEnabled(tagResetRenderStateWhenDrawing))
    {
        IGNORE_HR(m_pD3DDevice->ResetState());
    }
}
#endif






