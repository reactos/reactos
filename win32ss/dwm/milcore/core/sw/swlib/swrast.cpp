// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Software Rasterizer
//
//      The software rasterizer (SR) scan converts a primitive, feeding the
//      scanlines to a CScanPipeline.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Function:
//      IsMatrixIntegerTranslate
//
//  Synopsis:
//      Check that the given matrix only contains translation factors and that
//      the translation factors always include a half translation
//
//------------------------------------------------------------------------------

BOOL IsMatrixIntegerTranslate(const CBaseMatrix *pmat)
{
    REAL rM11 = REALABS(pmat->GetM11() - 1.0f);
    REAL rM12 = REALABS(pmat->GetM12());
    REAL rM21 = REALABS(pmat->GetM21());
    REAL rM22 = REALABS(pmat->GetM22() - 1.0f);

    if ((rM11 < MATRIX_EPSILON) && (rM21 < MATRIX_EPSILON) &&
        (rM12 < MATRIX_EPSILON) && (rM22 < MATRIX_EPSILON))
    {
        REAL dx = pmat->GetDx();
        REAL dy = pmat->GetDy();

        dx = REALABS(static_cast<REAL>(CFloatFPU::Round(dx)) - dx);
        dy = REALABS(static_cast<REAL>(CFloatFPU::Round(dy)) - dy);

        if (dx <= PIXEL_EPSILON && dy <= PIXEL_EPSILON)
            return TRUE;
    }

    return FALSE;
}

CSoftwareRasterizer::CSoftwareRasterizer()
{
    m_pCSCreator = &m_Creator_sRGB;
}

CSoftwareRasterizer::~CSoftwareRasterizer()
{
}

void CSoftwareRasterizer::SetColorDataPixelFormat(MilPixelFormat::Enum fmtPixels)
{
    if (fmtPixels == MilPixelFormat::PBGRA32bpp)
    {
        m_pCSCreator = &m_Creator_sRGB;
    }
    else
    {
        // we cannot rasterize in other formats for now.
        Assert (FALSE);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSoftwareRasterizer::Clear
//
//  Synopsis:
//      The Software Rasterizer must clear the entire device to the given solid
//      color.
//
//      The device extents are computed by requesting the device clipper and
//      retrieving its bounding rectangle. This is then cleared by calling
//      RasterizePath which will handle all the device clipping, format
//      conversion etc.
//

HRESULT CSoftwareRasterizer::Clear(
    __inout_ecount(1) CSpanSink *pSpanSink,
    __in_ecount(1) CSpanClipper *pSpanClipper,
    __in_ecount(1) const MilColorF *pColor
    )
{
    Assert(pSpanSink);
    Assert(pSpanClipper);

    HRESULT hr = S_OK;

    if (m_pCSCreator == NULL)
    {
        MIL_THR(WGXERR_WRONGSTATE);
    }

    // Make a solid color output span class for this color.
    CColorSource *pColorSource = NULL;
    if (SUCCEEDED(hr))
    {
        MIL_THR(m_pCSCreator->GetCS_Constant(pColor, &pColorSource));
    }

    if (SUCCEEDED(hr))
    {
        // Get the bounds of the clipper which is the maximal rectangle we
        // need to clear.

        CMILSurfaceRect rc;
        pSpanClipper->GetClipBounds(&rc);

        // Make a path for this rectangle.

        MilPoint2F points[] = {
            { float(rc.left), float(rc.top) },
            { float(rc.right), float(rc.top) },
            { float(rc.right), float(rc.bottom) },
            { float(rc.left), float(rc.bottom) }
        };

        static const BYTE types[] = { 0, 1, 1, 0x81 };

        if (SUCCEEDED(hr))
        {
            MIL_THR(pSpanSink->SetupPipeline(
                m_pCSCreator->GetPixelFormat(),
                pColorSource,
                FALSE,          // No AA
                false,          // No complement
                MilCompositingMode::SourceCopy,
                pSpanClipper,
                NULL,
                NULL,
                NULL));
        }

        if (SUCCEEDED(hr))
        {
            // Fill the path.

            C_ASSERT(ARRAY_SIZE(points) == ARRAY_SIZE(types));

            MilPointAndSizeL rcMilPointAndSizeL = {rc.left, rc.top, rc.Width(), rc.Height()};

            MIL_THR(RasterizePath(
                points,
                types,
                ARRAY_SIZE(points),
                &IdentityMatrix,
                MilFillMode::Alternate,
                MilAntiAliasMode::None,
                pSpanSink,
                pSpanClipper,
                &rcMilPointAndSizeL
                ));

            pSpanSink->ReleaseExpensiveResources();
        }

        m_pCSCreator->ReleaseCS(pColorSource);
    }

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSoftwareRasterizer::DrawBitmap
//
//  Synopsis:
//      The Software Rasterizer is being instructed to scan convert this
//      primitive into the provided Render Target.
//

HRESULT CSoftwareRasterizer::DrawBitmap(
    __inout_ecount(1) CSpanSink *pSpanSink,
    __in_ecount(1) CSpanClipper *pSpanClipper,
    __in_ecount(1) const CContextState *pContextState,
    __in_ecount(1) IWGXBitmapSource *pIBitmap,
    __in_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    // Compute the proper source rectangle. If prcSource is NULL, make a
    // rectangle equal to the bitmap dimensions.
    CRectF<CoordinateSpace::RealizationSampling> rcSource;

    CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::Device> const &
        matLocalToDevice = pContextState->WorldToDevice;

    // Effect is the same as local rendering for DrawBitmap
    CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device> const &
        matEffectToDevice =
            ReinterpretLocalRenderingAsBaseSampling(matLocalToDevice);

    // Realization source sampling is the same as local rendering for DrawBitmap
    CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> const &
        matSourceToDevice =
            ReinterpretLocalRenderingAsRealizationSampling(matLocalToDevice);

    // Source rectangle is also the local rendering rectangle.
    CRectF<CoordinateSpace::LocalRendering> const &rcLocal =
            ReinterpretRealizationSamplingAsLocalRendering(rcSource);

    if (m_pCSCreator == NULL)
    {
        MIL_THR(WGXERR_WRONGSTATE);
    }

    MilPointAndSizeL rcBounds;
    CMILSurfaceRect rcClipBounds;

    if (SUCCEEDED(hr))
    {
        pSpanClipper->GetClipBounds(&rcClipBounds);

        // figure out the source rect

        if(pContextState->RenderState->Options.SourceRectValid)
        {
            MilRectFFromMilPointAndSizeL(OUT rcSource, pContextState->RenderState->SourceRect);
        }
        else
        {
            // Default source rect covers the bounds of the source, which
            // is 1/2 beyond the extreme sample points in each direction.

            MIL_THR(GetBitmapSourceBounds(
                pIBitmap,
                &rcSource
                ));
        }
    }

    if (SUCCEEDED(hr))
    {
        // Compute the bounding rectangle.

        CRectF<CoordinateSpace::Device> rc;

        matSourceToDevice.Transform2DBounds(rcSource, OUT rc);

        MIL_THR(InflateRectFToPointAndSizeL(rc, OUT rcBounds));
    }

    CColorSource *pColorSource = NULL;
    if (SUCCEEDED(hr))
    {
        // Make an appropriate output span class based on the transform and
        // the filter mode.

        CMilColorF defColor;

        MIL_THR(m_pCSCreator->GetCS_PrefilterAndResample(
            pIBitmap,
            MilBitmapWrapMode::Extend,
            &defColor,
            &matSourceToDevice,
            pContextState->RenderState->InterpolationMode,
            pContextState->RenderState->PrefilterEnable,
            pContextState->RenderState->PrefilterThreshold,
            NULL,
            &pColorSource
            ));
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(pSpanSink->SetupPipeline(
            m_pCSCreator->GetPixelFormat(),
            pColorSource,
            IsPPAAMode(pContextState->RenderState->AntiAliasMode),
            false, // Complement support not required
            pContextState->RenderState->CompositingMode,
            pSpanClipper,
            pIEffect,
            &matEffectToDevice,
            pContextState
            ));

        if (SUCCEEDED(hr))
        {
            // This is ensured by our caller.
            // If rcSource is empty, we will fail in AddRect and draw
            // nothing, so all source rectangle flips are handled at
            // in the engine by applying the flip to the matrix instead.

            Assert(rcLocal.IsWellOrdered());

            // make a path for this call.

            MilPoint2F points[] = {
                { rcLocal.left , rcLocal.top },
                { rcLocal.right, rcLocal.top },
                { rcLocal.right, rcLocal.bottom },
                { rcLocal.left , rcLocal.bottom }
            };

            //
            // Apply pixel snapping.
            // This should be done in device space, so we convert points
            // and let rasterizer to know that they are already converted
            // by passing identity matrix to RasterizePath().
            //

            CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::Device> const *
                pmatLocalToDevice = &matLocalToDevice;

            CSnappingFrame *pSnappingFrame = pContextState->m_pSnappingStack;
            if (pSnappingFrame && !pSnappingFrame->IsEmpty())
            {
                for (UINT i = 0; i < ARRAY_SIZE(points); i++)
                {
                    TransformPoint(matLocalToDevice, points[i]);
                    pSnappingFrame->SnapPoint(points[i]);
                }

                pmatLocalToDevice = pmatLocalToDevice->pIdentity();
            }

            static const BYTE types[] = { 0, 1, 1, 0x81 };

            // fill the path.

            C_ASSERT(ARRAY_SIZE(points) == ARRAY_SIZE(types));            

            MIL_THR(RasterizePath(
                points,
                types,
                ARRAY_SIZE(points),
                pmatLocalToDevice,
                MilFillMode::Alternate,
                pContextState->RenderState->AntiAliasMode,
                pSpanSink,
                pSpanClipper,
                &rcBounds
                ));

            pSpanSink->ReleaseExpensiveResources();
        }

        m_pCSCreator->ReleaseCS(pColorSource);
    }

    // Some failure HRESULTs should only cause the primitive
    // in question to not draw.
    IgnoreNoRenderHRESULTs(&hr);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSoftwareRasterizer::DrawGlyphRun
//
//  Synopsis:
//      scan convert text primitive into the provided Render Target.
//
//------------------------------------------------------------------------------
HRESULT CSoftwareRasterizer::DrawGlyphRun(
    __inout_ecount(1) CSpanSink *pSpanSink,
    __in_ecount(1) CSpanClipper *pSpanClipper,
    __inout_ecount(1) DrawGlyphsParameters &pars,
    __inout_ecount(1) CMILBrush *pBrush,
    FLOAT flEffectAlpha,
    __in_ecount(1) CGlyphPainterMemory *pGlyphPainterMemory,
    bool fTargetSupportsClearType,
    __out_opt bool* pfClearTypeUsedToRender
    )
{
    HRESULT hr = S_OK;
    CSWGlyphRunPainter painter;
    BOOL fVisible = FALSE;
    CColorSource *pColorSource = NULL;

    CMILSurfaceRect rcClipBounds;

    MilPointAndSizeL rcBounds;

    if (pfClearTypeUsedToRender) 
    {
        *pfClearTypeUsedToRender = false;
    }
    
    pSpanClipper->GetClipBounds(&rcClipBounds);

    {
        // Do a rough check for glyph run visibility.
        // We need it, at least, to protect against
        // overflows in rendering routines.

        CRectF<CoordinateSpace::Device> rcClipBoundsF(
            static_cast<float>(rcClipBounds.left),
            static_cast<float>(rcClipBounds.top),
            static_cast<float>(rcClipBounds.right),
            static_cast<float>(rcClipBounds.bottom),
            LTRB_Parameters
            );

        if (!pars.rcBounds.Device().DoesIntersect(rcClipBoundsF))
            goto Cleanup;
    }

    IFC( painter.Init(
        pars,
        flEffectAlpha,
        pGlyphPainterMemory,
        fTargetSupportsClearType,
        &fVisible
        ) );

    if (pfClearTypeUsedToRender)
    {
        *pfClearTypeUsedToRender = !!painter.IsClearType();
    }

    if (!fVisible) goto Cleanup;

    //
    // For text rendering, local rendering and world sampling spaces are identical
    //

    const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &
        matBaseSamplingToDevice =
        ReinterpretLocalRenderingAsBaseSampling(pars.pContextState->WorldToDevice);

    IFC( GetCS_Brush(
        pBrush,
        matBaseSamplingToDevice,
        pars.pContextState,
        &pColorSource
        ) );

    {
        CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> matGlyphRunToDevice;
        const CRectF<CoordinateSpace::Shape> &rcfGlyphRun =
            painter.GetOutlineRect(OUT matGlyphRunToDevice);

        CShape shapeGlyphRun;
        IFC(shapeGlyphRun.AddRect(rcfGlyphRun));

        {
            MilAntiAliasMode::Enum antiAliasMode = pars.pContextState->RenderState->AntiAliasMode;

            CRectF<CoordinateSpace::Device> rcShapeBoundsDeviceSpace;
            CShape scratchClipperShape;

            CShapeClipperForFEB clipper(&shapeGlyphRun, &rcfGlyphRun, &matGlyphRunToDevice);
            
            IFC(clipper.ApplyBrush(pBrush, matBaseSamplingToDevice, &scratchClipperShape));

            // We should not call ApplyGuidelines to glyph run here,
            // because guidelines should not stretch it.
            // We only need to shift it as a whole, using guidelines closest to
            // glyph run anchor point. CBaseGlyphRunPainter takes care of it.

            IFC( pSpanSink->SetupPipelineForText(
                pColorSource,
                pars.pContextState->RenderState->CompositingMode,
                painter,
                antiAliasMode != MilAntiAliasMode::None && clipper.ShapeHasBeenCorrected()
                ) );

            IFC(clipper.GetBoundsInDeviceSpace(&rcShapeBoundsDeviceSpace));

            IFC(clipper.GetShape()->ConvertToGpPath(m_rgPoints, m_rgTypes));

            IFC(InflateRectFToPointAndSizeL(rcShapeBoundsDeviceSpace, OUT rcBounds));

            Assert(m_rgPoints.GetCount() == m_rgTypes.GetCount());

            hr = RasterizePath(
                    m_rgPoints.GetDataBuffer(),
                    m_rgTypes.GetDataBuffer(),
                    m_rgPoints.GetCount(),
                    clipper.GetShapeToDeviceTransform(),
                    MilFillMode::Alternate,
                    antiAliasMode,
                    pSpanSink,
                    pSpanClipper,
                    &rcBounds
                    );
        }
    }

    pSpanSink->ReleaseExpensiveResources();

Cleanup:

    // Always reset the geometry scratch buffers to prevent stale
    // types/points from being present on the next DrawGlyphRun call.
    m_rgPoints.Reset(FALSE);
    m_rgTypes.Reset(FALSE);
    
    if (pColorSource != NULL)
    {
        m_pCSCreator->ReleaseCS(pColorSource);
    }
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSoftwareRasterizer::FillPathUsingBrushRealizer
//
//  Synopsis:
//      version of FillPath that takes a CBrushRealizer
//

HRESULT
CSoftwareRasterizer::FillPathUsingBrushRealizer(
    __inout_ecount(1) CSpanSink *pSpanSink,
    MilPixelFormat::Enum fmtTarget,
    DisplayId associatedDisplay,
    __inout_ecount(1) CSpanClipper *pSpanClipper,
    __in_ecount(1) const CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __in_ecount_opt(1) const IShapeData *pShape,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice, // (NULL OK)
    __in_ecount(1) CBrushRealizer *pBrushRealizer,
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice
    DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount(1) ISteppedRenderingDisplayRT *pDisplayRTParent)
    )
{
    HRESULT hr = S_OK;

    CMILBrush *pFillBrushNoRef = NULL;
    IMILEffectList *pIEffectsNoRef = NULL;

    {
        CSwIntermediateRTCreator swRTCreator(
            fmtTarget,
            associatedDisplay
            DBG_STEP_RENDERING_COMMA_PARAM(pDisplayRTParent)
            );

        IFC(pBrushRealizer->EnsureRealization(
            CMILResourceCache::SwRealizationCacheIndex,
            associatedDisplay,
            pBrushContext,
            pContextState,
            &swRTCreator
            ));


        pFillBrushNoRef = pBrushRealizer->GetRealizedBrushNoRef(false /* fConvertNULLToTransparent */);
        IFC(pBrushRealizer->GetRealizedEffectsNoRef(&pIEffectsNoRef));
    }

    if (pFillBrushNoRef == NULL)
    {
        // Nothing to draw
        goto Cleanup;
    }

    IFC(FillPath(
        pSpanSink,
        pSpanClipper,
        pContextState,
        pShape,
        pmatShapeToDevice,
        pFillBrushNoRef,
        matWorldToDevice,
        pIEffectsNoRef
        ));

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSoftwareRasterizer::FillPath
//
//  Synopsis:
//      Scan convert this shape into the provided Render Target. This is a low
//      level utility, called both for filling, and during stroking.
//
//------------------------------------------------------------------------------
HRESULT
CSoftwareRasterizer::FillPath(
    __inout_ecount(1) CSpanSink *pSpanSink,
    __inout_ecount(1) CSpanClipper *pSpanClipper,
    __in_ecount(1) const CContextState *pContextState,
    __in_ecount_opt(1) const IShapeData *pShape,            // NULL treated as infinite shape
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice, // NULL OK
    __in_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice,
    __in_ecount_opt(1) IMILEffectList *pIEffect,
    float rComplementFactor,
    __in_ecount_opt(1) const CMILSurfaceRect *prcComplementBounds
    )
{
    HRESULT hr = S_OK;

    // Clip shape to safe device bounds if needed.
    CShape clippedShape;
    bool fWasShapeClipped = false;
    
    CRectF<CoordinateSpace::Shape> rcShapeBounds;  // in shape space

    if (pShape)
    {
        IFC(pShape->GetTightBounds(OUT rcShapeBounds));
    }
    
    IFC(ClipToSafeDeviceBounds(pShape, pmatShapeToDevice, &rcShapeBounds, &clippedShape, &fWasShapeClipped));
    
    if (fWasShapeClipped)
    {
        pShape = &clippedShape;
        pmatShapeToDevice = NULL;
        IFC(pShape->GetTightBounds(OUT rcShapeBounds));
    }

    Assert(pShape != NULL); // NULL (infinite) shapes should be clipped to the device bounds.

    {
        CShape scratchClipperShape;
        CShape scratchSnappedShape;
        MilPointAndSizeL rcBounds;

        CRectF<CoordinateSpace::Device> rcShapeBoundsDeviceSpace;
        CMILSurfaceRect rcClipBounds;

        CShapeClipperForFEB clipper(pShape, &rcShapeBounds, pmatShapeToDevice);

        IFC(clipper.ApplyGuidelines(
            pContextState->m_pSnappingStack,
            &scratchSnappedShape
            ));

        IFC(clipper.ApplyBrush(pBrush, matWorldToDevice, &scratchClipperShape));

        IFC(clipper.GetBoundsInDeviceSpace(&rcShapeBoundsDeviceSpace));

        // save the clip bounds in the context state

        pSpanClipper->GetClipBounds(&rcClipBounds);

        // Compute the bounding rectangle of the shape in device space.

        IFC(InflateRectFToPointAndSizeL(rcShapeBoundsDeviceSpace, rcBounds));

        IFC(clipper.GetShape()->ConvertToGpPath(m_rgPoints, m_rgTypes));

        if (m_rgPoints.GetCount() > 0)
        {
            Assert(m_rgPoints.GetCount() == m_rgTypes.GetCount());

            CColorSource *pColorSource = NULL;

            // Make an appropriate output span class based on the transform and
            // the filter mode.

            IFC(GetCS_Brush(
                pBrush,
                matWorldToDevice,
                pContextState,
                &pColorSource
                ));

            MIL_THR(pSpanSink->SetupPipeline(
                m_pCSCreator->GetPixelFormat(),
                pColorSource,
                IsPPAAMode(pContextState->RenderState->AntiAliasMode),
                rComplementFactor >= 0, // Requires support for complement?
                pContextState->RenderState->CompositingMode,
                pSpanClipper,
                pIEffect,
                &matWorldToDevice, // Effect coord space == World Sampling coord space
                pContextState
                ));

            if (SUCCEEDED(hr))
            {
                MIL_THR(RasterizePath(
                    m_rgPoints.GetDataBuffer(),
                    m_rgTypes.GetDataBuffer(),
                    m_rgPoints.GetCount(),
                    clipper.GetShapeToDeviceTransform(),
                    clipper.GetShape()->GetFillMode(),
                    pContextState->RenderState->AntiAliasMode,
                    pSpanSink,
                    pSpanClipper,
                    &rcBounds,
                    rComplementFactor,
                    prcComplementBounds
                    ));

                pSpanSink->ReleaseExpensiveResources();
            }

            m_pCSCreator->ReleaseCS(pColorSource);
        }
    }

Cleanup:

    // Some failure HRESULTs should only cause the primitive
    // in question to not draw.
    IgnoreNoRenderHRESULTs(&hr);

    // Always reset the geometry scratch buffers to prevent stale
    // types/points from being present on the next DrawPath call.
    m_rgPoints.Reset(FALSE);
    m_rgTypes.Reset(FALSE);    

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSoftwareRasterizer::GetCS_Brush
//
//  Synopsis:
//      Get a CColorSource which is appropriate for the given brush.
//
//------------------------------------------------------------------------------

HRESULT
CSoftwareRasterizer::GetCS_Brush(
    CMILBrush *pBrush,
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> &matWorldHPCToDeviceHPC,
    const CContextState *pContextState,
    OUT CColorSource **ppColorSource
    )
{
    HRESULT hr = S_OK;

    Assert( pBrush != NULL );
    Assert( pContextState != NULL );
    Assert( ppColorSource != NULL );

    switch( pBrush->GetType() )
    {
        case BrushSolid:
        {
            const CMILBrushSolid *pSolidBrush = static_cast<CMILBrushSolid *>(pBrush);

            MIL_THR(m_pCSCreator->GetCS_Constant(
                &pSolidBrush->m_SolidColor,
                ppColorSource));
        }
        break;

        case BrushGradientLinear:
        {
            CMILBrushLinearGradient *pGradBrush = static_cast<CMILBrushLinearGradient *>(pBrush);

            UINT nColorCount = pGradBrush->GetColorData()->GetCount();

            if (nColorCount < 2)
            {
                // Specifiying at least 2 gradient stops is required
                MIL_THR(WGXERR_INVALIDPARAMETER);
                break;
            }

            MilPoint2F ptsGradient[3];
            pGradBrush->GetEndPoints(&ptsGradient[0], &ptsGradient[1], &ptsGradient[2]);

            if (SUCCEEDED(hr))
            {
                hr = m_pCSCreator->GetCS_LinearGradient(
                    ptsGradient,
                    pGradBrush->GetColorData()->GetCount(),
                    pGradBrush->GetColorData()->GetColorsPtr(),
                    pGradBrush->GetColorData()->GetPositionsPtr(),
                    pGradBrush->GetWrapMode(),
                    pGradBrush->GetColorInterpolationMode(),
                    &matWorldHPCToDeviceHPC,
                    ppColorSource);
            }
        }
        break;

        case BrushGradientRadial:
        {
            CMILBrushRadialGradient *pGradBrush = static_cast<CMILBrushRadialGradient *>(pBrush);

            UINT nColorCount = pGradBrush->GetColorData()->GetCount();

            if (nColorCount < 2)
            {
                // Specifiying at least 2 gradient stops is required
                MIL_THR(WGXERR_INVALIDPARAMETER);
                break;
            }

            MilPoint2F ptsGradient[3];

            // Center is first gradient point

            pGradBrush->GetEndPoints(
                &ptsGradient[0],
                &ptsGradient[1],
                &ptsGradient[2]
                );
            
            if (!(pGradBrush->HasSeparateOriginFromCenter()))
            {
                // Create a standard radial gradient if no focal point was set, or
                // the focal point & center are very close to each other
                hr = m_pCSCreator->GetCS_RadialGradient(
                    ptsGradient,
                    pGradBrush->GetColorData()->GetCount(),
                    pGradBrush->GetColorData()->GetColorsPtr(),
                    pGradBrush->GetColorData()->GetPositionsPtr(),
                    pGradBrush->GetWrapMode(),
                    pGradBrush->GetColorInterpolationMode(),
                    &matWorldHPCToDeviceHPC,
                    ppColorSource);
            }
            else
            {
                // Create a focal gradient
                hr = m_pCSCreator->GetCS_FocalGradient(
                    ptsGradient,
                    pGradBrush->GetColorData()->GetCount(),
                    pGradBrush->GetColorData()->GetColorsPtr(),
                    pGradBrush->GetColorData()->GetPositionsPtr(),
                    pGradBrush->GetWrapMode(),
                    pGradBrush->GetColorInterpolationMode(),
                    &pGradBrush->GetGradientOrigin(),
                    &matWorldHPCToDeviceHPC,
                    ppColorSource);
            }
        }
        break;

        case BrushBitmap:
        {
            CMILBrushBitmap *pBitmapBrush = static_cast<CMILBrushBitmap *>(pBrush);
            CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> matBitmapToDeviceHPC;
            pBitmapBrush->GetBitmapToSampleSpaceTransform(
                matWorldHPCToDeviceHPC,
                OUT matBitmapToDeviceHPC
                );

            hr = m_pCSCreator->GetCS_PrefilterAndResample(
                pBitmapBrush->GetTextureNoAddRef(),
                pBitmapBrush->GetWrapMode(),
                &pBitmapBrush->GetBorderColorRef(),
                &matBitmapToDeviceHPC,
                pContextState->RenderState->InterpolationMode,
                pContextState->RenderState->PrefilterEnable,
                pContextState->RenderState->PrefilterThreshold,
                pBitmapBrush,
                ppColorSource
                );
        }
        break;

        case BrushShaderEffect:
        {
            CMILBrushShaderEffect *pShaderEffectBrush = static_cast<CMILBrushShaderEffect *>(pBrush);
            
            CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> bitmapToSampleSpaceTransform;
            pShaderEffectBrush->GetBitmapToSampleSpaceTransform(matWorldHPCToDeviceHPC, OUT &bitmapToSampleSpaceTransform);
                       
            hr = m_pCSCreator->GetCS_EffectShader(
                reinterpret_cast<const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC>*>(&bitmapToSampleSpaceTransform),
                pShaderEffectBrush,
                OUT ppColorSource);
        }
        break;
        
    }

    if (hr == WGXERR_INVALIDPARAMETER)
    {
        // Invalid parameter triggered this, so just simply create
        // a fully transparent brush.
        CMilColorF trans(1.0, 1.0, 1.0, 0.0);
        hr = m_pCSCreator->GetCS_Constant(&trans, ppColorSource);
    }

    return hr;
}

CResampleSpanCreator_sRGB::CResampleSpanCreator_sRGB()
{
    m_pIdentitySpan = NULL;
    m_pNearestNeighborSpan = NULL;
    m_pBilinearSpanMMX = NULL;

// Future Consideration: 
// Remove the non-optimized codepath once Intel integration is complete
#ifndef ENABLE_INTEL_OPTIMIZED_BILINEAR
    m_pUnoptimizedBilinearSpan = NULL;
#else
    m_pBilinearSpan = NULL;
#endif
}

CResampleSpanCreator_sRGB::~CResampleSpanCreator_sRGB()
{
    delete m_pIdentitySpan;
    delete m_pNearestNeighborSpan;

    delete m_pBilinearSpanMMX;

// Future Consideration: 
// Remove the non-optimized codepath once Intel integration is complete
#ifndef ENABLE_INTEL_OPTIMIZED_BILINEAR   
    delete m_pUnoptimizedBilinearSpan;
#else
    delete m_pBilinearSpan;
#endif
}

HRESULT CResampleSpanCreator_sRGB::GetCS_Resample(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount_opt(1) const MilColorF *pBorderColor,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
    MilBitmapInterpolationMode::Enum interpolationMode,
    __deref_out_ecount(1) CColorSource **ppColorSource
    )
{
    HRESULT hr = S_OK;

    // Ensure that the given bitmap source is in an acceptable format.

    IWGXBitmapSource *pWICWrapper = NULL;
    IWICFormatConverter *pConverter = NULL;
    IWICImagingFactory *pIWICFactory = NULL;

    IWICBitmapSource *pWGXWrapper = NULL;
    IWICBitmapSource *pIWICBitmapSourceNoRef = NULL;
    
    MilPixelFormat::Enum pixelFormat;

    IFC(pIBitmapSource->GetPixelFormat(&pixelFormat));

    IFC(WrapInClosestBitmapInterface(pIBitmapSource, &pWGXWrapper));
    pIWICBitmapSourceNoRef = pWGXWrapper;   // No ref change
    
    if (!CSoftwareRasterizer::IsValidPixelFormat32(pixelFormat))
    {
        IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));

        IFC(pIWICFactory->CreateFormatConverter(&pConverter));
        
        IFC(pConverter->Initialize(
                pIWICBitmapSourceNoRef,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.0f,
                WICBitmapPaletteTypeCustom
                ));

        pIWICBitmapSourceNoRef = pConverter; // No ref change
    }

    UINT width = 0, height = 0;
    IFC(pIWICBitmapSourceNoRef->GetSize(&width, &height));

    CResampleSpan<GpCC> *pResampleSpan = NULL;

    // Go through our hierarchy of scan drawers:
    if (IsMatrixIntegerTranslate(pmatTextureHPCToDeviceHPC) &&
        ((wrapMode == MilBitmapWrapMode::Tile) ||
         (wrapMode == MilBitmapWrapMode::Border)))
    {
        if (m_pIdentitySpan == NULL)
        {
            m_pIdentitySpan = new CIdentitySpan();
        }

        pResampleSpan = m_pIdentitySpan;
    }
    else
    {
        if (interpolationMode == MilBitmapInterpolationMode::NearestNeighbor)
        {
            if (m_pNearestNeighborSpan == NULL)
            {
                m_pNearestNeighborSpan = new CNearestNeighborSpan();
            }

            pResampleSpan = m_pNearestNeighborSpan;
        }
        else
        {

// Future Consideration: 
// Remove this non-optimized codepath once Intel integration is complete
#ifndef ENABLE_INTEL_OPTIMIZED_BILINEAR
            if (g_fUseMMX &&
                CBilinearSpan_MMX::CanHandleInputRange(width, height, wrapMode)
                )
            {
                if (m_pBilinearSpanMMX == NULL)
                {
                    m_pBilinearSpanMMX = new CBilinearSpan_MMX();
                }

                pResampleSpan = m_pBilinearSpanMMX;
            }
            else
            {                
                if (m_pUnoptimizedBilinearSpan == NULL)
                {
                    m_pUnoptimizedBilinearSpan = new CUnoptimizedBilinearSpan();
                }

                pResampleSpan = m_pUnoptimizedBilinearSpan;
            }

#else // _defined(ENABLE_INTEL_OPTIMIZED_BILINEAR)

            BOOL fSupportsSSE2 = FALSE;

#if defined(_X86_) 
            // Check for SSE2 on x86 machines.  SSE2 acceleration
            // is disabled for 64-bit targets because intrinsics
            // are causing compile errors.
            fSupportsSSE2 = g_fUseSSE2;
#endif
            // Check for MMX acceleration on machines that don't
            // support SSE2.
            if (!fSupportsSSE2 &&   
                 g_fUseMMX &&
                 CBilinearSpan_MMX::CanHandleInputRange(width, height, wrapMode)
                )
            {
                if (m_pBilinearSpanMMX == NULL)
                {
                    m_pBilinearSpanMMX = new CBilinearSpan_MMX();
                }

                pResampleSpan = m_pBilinearSpanMMX;
            }
            else
            {          
                // Use CBilinearSpan for SSE2-enabled machines,
                // machines that don't support either SSE2 or MMX,
                // or width/height's outside of the Fixed16 range.
                //
                // CBilinearSpan only optimizes for SSE2-enabled
                // machines (not MMX machines), but it has non-optimized
                // support for all machines types, and can support
                // the full UINT range for all wrap modes.  
                if (m_pBilinearSpan == NULL)
                {
                    m_pBilinearSpan = new CBilinearSpan();
                }

                pResampleSpan = m_pBilinearSpan;  
            }
            
#endif // !_defined(ENABLE_INTEL_OPTIMIZED_BILINEAR)
        }
    }

    IFCOOM(pResampleSpan);

    IFC(WrapInClosestBitmapInterface(pIWICBitmapSourceNoRef, &pWICWrapper));

    IFC(pResampleSpan->Initialize(
        pWICWrapper,
        wrapMode,
        pBorderColor,
        pmatTextureHPCToDeviceHPC
        ));
    
    *ppColorSource = pResampleSpan;

Cleanup:
    ReleaseInterfaceNoNULL(pWGXWrapper);
    ReleaseInterfaceNoNULL(pIWICFactory);
    ReleaseInterfaceNoNULL(pConverter);
    ReleaseInterfaceNoNULL(pWICWrapper);

    RRETURN(hr);
}

HRESULT CColorSourceCreator::GetCS_PrefilterAndResample(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount_opt(1) const MilColorF *pBorderColor,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
    MilBitmapInterpolationMode::Enum interpolationMode,
    bool prefilterEnable,
    FLOAT prefilterThreshold,
    __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
    __deref_out_ecount(1) CColorSource **ppColorSource
    )
{
    HRESULT hr = S_OK;
    IWGXBitmap *pBitmap = NULL;

    if (prefilterEnable)
    {
        if (g_pMediaControl && g_pMediaControl->GetDataPtr()->FantScalerDisabled)
        {
            prefilterEnable = false;
        }
    }

    CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> matAdjustedTextureToDevice(*pmatTextureHPCToDeviceHPC);

    IFC(CSwBitmapColorSource::DeriveFromBitmapAndContext(
        pIBitmapSource,
        IN OUT &matAdjustedTextureToDevice,
        this,
        prefilterEnable,
        prefilterThreshold,
        pICacheAlternate,
        &pBitmap
        ));

    pIBitmapSource = pBitmap;
    pmatTextureHPCToDeviceHPC = &matAdjustedTextureToDevice;

    IFC(GetCS_Resample(
        pIBitmapSource,
        wrapMode,
        pBorderColor,
        pmatTextureHPCToDeviceHPC,
        interpolationMode,
        ppColorSource
        ));

Cleanup:
    ReleaseInterfaceNoNULL(pBitmap);

    RRETURN(hr);
}

CColorSourceCreator_sRGB::CColorSourceCreator_sRGB()
    : m_ResampleSpans()
{
    m_pConstantColorSpan = NULL;
    m_pLinearGradientSpan = NULL;
    m_pRadialGradientSpan = NULL;
    m_pFocalGradientSpan = NULL;
    m_pShaderEffectSpan = NULL;
}

CColorSourceCreator_sRGB::~CColorSourceCreator_sRGB()
{
    delete m_pConstantColorSpan;
    delete m_pLinearGradientSpan;
    delete m_pRadialGradientSpan;
    delete m_pFocalGradientSpan;
    delete m_pShaderEffectSpan;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CColorSourceCreator_sRGB::ReleaseCS, CColorSourceCreator
//
//  Synopsis:
//      Returns the color source to the color source creator.
//
//------------------------------------------------------------------------------

VOID CColorSourceCreator_sRGB::ReleaseCS(CColorSource *pColorSource)
{
    // For now, we use a simplistic caching system:
    //
    // 1) Always hold on to the CColorSource objects we create (as a 1-deep
    //    cache per CColorSource type.)
    // 2) Always release any expensive resources held by the color source
    //    object.
    //
    // When changing this to a more sophisticated caching system, note:
    //    CResampleSpanCreator_* owns caching decisions about resampling color
    //    sources, but ReleaseCS would have to do another virtual call to know
    //    whether to pass control on to CResampleSpanCreator_*.
    //
    //    Another option is to collapse CResampleSpanCreator_* into
    //    CColorSourceCreator_*. But CMaskAlphaSpan_* currently uses
    //    CResampleSpanCreator_* without the overhead of CColorSourceCreator_*.

    pColorSource->ReleaseExpensiveResources();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CColorSourceCreator_sRGB::GetSupportedSourcePixelFormat
//
//  Synopsis:
//      Return pixel format needed by rasterizer when source is of given format
//

MilPixelFormat::Enum CColorSourceCreator_sRGB::GetSupportedSourcePixelFormat(
    MilPixelFormat::Enum fmtSourceGiven,
    bool fForceAlpha
    ) const
{
    //
    // sRGB supports only two source formats - 32bppBGR and PBGRA.
    // If source is not BGR, then require PBGRA.
    //

    if (   fmtSourceGiven != MilPixelFormat::BGR32bpp
        || fForceAlpha
           )
    {
        fmtSourceGiven = MilPixelFormat::PBGRA32bpp;
    }

    return fmtSourceGiven;
}

HRESULT
CColorSourceCreator_sRGB::GetCS_Constant(
    const MilColorF *pColor,
    OUT CColorSource **ppColorSource
    )
{
    HRESULT hr = S_OK;

    if (m_pConstantColorSpan == NULL)
    {
        m_pConstantColorSpan = new CConstantColorBrushSpan;
        if (m_pConstantColorSpan == NULL)
        {
            MIL_THR(E_OUTOFMEMORY);
        }
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(m_pConstantColorSpan->Initialize(pColor));
    }

    if (SUCCEEDED(hr))
    {
        *ppColorSource = m_pConstantColorSpan;
    }

    return hr;
}



HRESULT 
CColorSourceCreator_sRGB::GetCS_EffectShader(
    __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
    __inout CMILBrushShaderEffect* pShaderEffectBrush,
    __deref_out CColorSource **ppColorSource
    )
{
    HRESULT hr = S_OK;

    if (m_pShaderEffectSpan == NULL)
    {
        m_pShaderEffectSpan = new CShaderEffectBrushSpan();
        IFCOOM(m_pShaderEffectSpan);
    }

    IFC(m_pShaderEffectSpan->Initialize(
        pRealizationSamplingToDevice,
        pShaderEffectBrush));

    *ppColorSource = m_pShaderEffectSpan;

Cleanup:

    RRETURN(hr);
}



HRESULT
CColorSourceCreator_sRGB::GetCS_LinearGradient(
    const MilPoint2F *pGradientPoints,
    UINT nColorCount,
    const MilColorF *pColors,
    const FLOAT *pPositions,
    MilGradientWrapMode::Enum wrapMode,
    MilColorInterpolationMode::Enum colorInterpolationMode,
    const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    OUT CColorSource **ppColorSource
    )
{
    HRESULT hr = S_OK;

    Assert(nColorCount >= 2);

    if (m_pLinearGradientSpan == NULL)
    {
        if (g_fUseMMX)
        {
            m_pLinearGradientSpan = new CLinearGradientBrushSpan_MMX;
        }
        else
        {
            m_pLinearGradientSpan = new CLinearGradientBrushSpan;
        }

        if (m_pLinearGradientSpan == NULL)
        {
            MIL_THR(E_OUTOFMEMORY);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (g_fUseMMX)
        {
            MIL_THR(((CLinearGradientBrushSpan_MMX *)m_pLinearGradientSpan)->Initialize(
                pmatWorldHPCToDeviceHPC,
                pGradientPoints,
                pColors,
                pPositions,
                nColorCount,
                wrapMode,
                colorInterpolationMode));
        }
        else
        {
            MIL_THR(m_pLinearGradientSpan->Initialize(
                pmatWorldHPCToDeviceHPC,
                pGradientPoints,
                pColors,
                pPositions,
                nColorCount,
                wrapMode,
                colorInterpolationMode));
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppColorSource = m_pLinearGradientSpan;
    }

    return hr;
}

HRESULT
CColorSourceCreator_sRGB::GetCS_RadialGradient(
    const MilPoint2F *pGradientPoints,
    const UINT nColorCount,
    const MilColorF *pColors,
    const FLOAT *pPositions,
    MilGradientWrapMode::Enum wrapMode,
    MilColorInterpolationMode::Enum colorInterpolationMode,
    const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    OUT CColorSource **ppColorSource
    )
{
    HRESULT hr = S_OK;

    Assert(nColorCount >= 2);

    if (SUCCEEDED(hr))
    {
        if (m_pRadialGradientSpan == NULL)
        {
            m_pRadialGradientSpan = new CRadialGradientBrushSpan;
            if (m_pRadialGradientSpan == NULL)
            {
                MIL_THR(E_OUTOFMEMORY);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(m_pRadialGradientSpan->Initialize(
            pmatWorldHPCToDeviceHPC,
            pGradientPoints,
            pColors,
            pPositions,
            nColorCount,
            wrapMode,
            colorInterpolationMode));
    }

    if (SUCCEEDED(hr))
    {
        *ppColorSource = m_pRadialGradientSpan;
    }

    return hr;
}

HRESULT
CColorSourceCreator_sRGB::GetCS_FocalGradient(
    const MilPoint2F *pGradientPoints,
    UINT nColorCount,
    const MilColorF *pColors,
    const FLOAT *pPositions,
    MilGradientWrapMode::Enum wrapMode,
    MilColorInterpolationMode::Enum colorInterpolationMode,
    const MilPoint2F *pptOrigin,
    const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    OUT CColorSource **ppColorSource
    )
{
    HRESULT hr = S_OK;

    Assert(nColorCount >= 2);

    if (SUCCEEDED(hr))
    {
        if (m_pFocalGradientSpan == NULL)
        {
            m_pFocalGradientSpan = new CFocalGradientBrushSpan;
            if (m_pFocalGradientSpan == NULL)
            {
                MIL_THR(E_OUTOFMEMORY);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(m_pFocalGradientSpan->Initialize(
            pmatWorldHPCToDeviceHPC,
            pGradientPoints,
            pColors,
            pPositions,
            nColorCount,
            wrapMode,
            colorInterpolationMode,
            pptOrigin));
    }

    if (SUCCEEDED(hr))
    {
        *ppColorSource = m_pFocalGradientSpan;
    }

    return hr;
}

HRESULT
CColorSourceCreator_sRGB::GetCS_Resample(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount_opt(1) const MilColorF *pBorderColor,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
    MilBitmapInterpolationMode::Enum interpolationMode,
    __deref_out_ecount(1) CColorSource **ppColorSource
    )
{
    return m_ResampleSpans.GetCS_Resample(
        pIBitmapSource,
        wrapMode,
        pBorderColor,
        pmatTextureHPCToDeviceHPC,
        interpolationMode,
        ppColorSource);
}


CResampleSpanCreator_scRGB::CResampleSpanCreator_scRGB()
{
    Assert(FALSE);
}

CResampleSpanCreator_scRGB::~CResampleSpanCreator_scRGB()
{
    Assert(FALSE);
}

HRESULT CResampleSpanCreator_scRGB::GetCS_Resample(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount_opt(1) const MilColorF *pBorderColor,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
    MilBitmapInterpolationMode::Enum interpolationMode,
    __deref_out_ecount(1) CColorSource **ppColorSource
    )
{
    Assert(FALSE);

    RRETURN(E_NOTIMPL);
}


CColorSourceCreator_scRGB::CColorSourceCreator_scRGB()
{
    Assert(FALSE);
}

CColorSourceCreator_scRGB::~CColorSourceCreator_scRGB()
{
    Assert(FALSE);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CColorSourceCreator_scRGB::ReleaseCS, CColorSourceCreator
//
//  Synopsis:
//      Returns the color source to the color source creator.
//
//------------------------------------------------------------------------------

VOID CColorSourceCreator_scRGB::ReleaseCS(CColorSource *pColorSource)
{
    // See CColorSourceCreator_sRGB::ReleaseCS.

    Assert(FALSE);
}

HRESULT
CColorSourceCreator_scRGB::GetCS_Constant(
    const MilColorF *pColor,
    OUT CColorSource **ppColorSource
    )
{
    Assert(FALSE);

    RRETURN(E_NOTIMPL);
}


HRESULT 
CColorSourceCreator_scRGB::GetCS_EffectShader(
    __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
    __inout CMILBrushShaderEffect* pShaderEffectBrush,
    __deref_out CColorSource **ppColorSource
    )
{
    Assert(false);
    RRETURN(E_NOTIMPL);
}


HRESULT
CColorSourceCreator_scRGB::GetCS_LinearGradient(
    const MilPoint2F *pGradientPoints,
    UINT nColorCount,
    const MilColorF *pColors,
    const FLOAT *pPositions,
    MilGradientWrapMode::Enum wrapMode,
    MilColorInterpolationMode::Enum colorInterpolationMode,
    const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    OUT CColorSource **ppColorSource
    )
{
    Assert(FALSE);

    RRETURN(E_NOTIMPL);
}

HRESULT
CColorSourceCreator_scRGB::GetCS_RadialGradient(
    const MilPoint2F *pGradientPoints,
    UINT nColorCount,
    const MilColorF *pColors,
    const FLOAT *pPositions,
    MilGradientWrapMode::Enum wrapMode,
    MilColorInterpolationMode::Enum colorInterpolationMode,
    const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    OUT CColorSource **ppColorSource
    )
{
    Assert(FALSE);

    RRETURN(E_NOTIMPL);
}

HRESULT
CColorSourceCreator_scRGB::GetCS_FocalGradient(
    const MilPoint2F *pGradientPoints,
    UINT nColorCount,
    const MilColorF *pColors,
    const FLOAT *pPositions,
    MilGradientWrapMode::Enum wrapMode,
    MilColorInterpolationMode::Enum colorInterpolationMode,
    const MilPoint2F *pptOrigin,
    const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
    OUT CColorSource **ppColorSource
    )
{

    Assert(FALSE);

    RRETURN(E_NOTIMPL);
}

HRESULT
CColorSourceCreator_scRGB::GetCS_Resample(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount_opt(1) const MilColorF *pBorderColor,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
    MilBitmapInterpolationMode::Enum interpolationMode,
    __deref_out_ecount(1) CColorSource **ppColorSource
    )
{
    Assert(FALSE);

    RRETURN(E_NOTIMPL);
}






