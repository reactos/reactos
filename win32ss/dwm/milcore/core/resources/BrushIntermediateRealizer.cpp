// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      CPP file for brush intermediate realizer base class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//
// "Spaces and Transforms"
//
// There are a lot of coordinate spaces and transforms used in this file. This
// section is meant to describe how they relate to each other and what names
// are used to describe these spaces and transforms in the code.
//
// The spaces we really care about are the * Viewport space, *
// (Intermediate)Surface space * World space * Sample space
//
// The user describes a viewport and a mapping from this viewport to World
// space. This is where we get ViewportSpace, WorldSpace, and the
// transformation [ViewportToWorld].
//
// Additionally we have a transformation that takes us from World space to
// Sample (Device) Space. This is the [WorldToSample] transformation.
//
// We draw the brush into an intermediate surface. For tiled brushes, this
// surface is in the same orientation as the viewport, but we use the scale of
// the sample space to do the rendering. This adds complications, causing us to
// split up the [ViewportToWorld] and the [WorldToSample] matrices into their
// scale components and their non-scale components. This gives us four more
// matrices (three of
// which we use):
// * [ScaleOfViewportToWorld]
// * [NonScaleOfViewportToWorld]
// * [ScaleOfViewportToSampleSpace]
//
// Additionally, the intermediate surface must have integer-size. Thus, when we
// calculate the bounds of the intermediate surface, we arrive in
// IdealSurfaceSpace rather than the final "(Intermediate)SurfaceSpace". The
// transformation between these two is called
// [IdealSurfaceToIntermediateSurface]
//
// How do these all relate? They relate in a tree. At the top is the Viewport
// space. At the leaves are the two spaces we care about in the end, the
// (Intermediate)Surface space and the Sample space.
//
//
//                            ViewportSpace
//                                  |
//                           [ScaleOfViewportToWorld]
//                                  |
//                           WorldScaledViewportSpace
//                             /                   \
//          [ScaleOfWorldToSample]               [NonScaleOfViewportToWorld]
//                  /                                  |
//  BaseTile_SampleScaledViewportSpace              WorldSpace
//                  |                                  |
//  [SampleScaledViewportToSurface]             [WorldToSample]
//                  |                                  |
//       BaseTile_SurfaceSpace                      SampleSpace
//                  |
//      [BaseTileToRenderedTile]
//                  |
// RenderedTile(Intermediate)_SurfaceSpace
//
//
// Non-tiled brushes have a simpler graph
//
//                            ViewportSpace
//                                  |
//                          [ViewportToSampleSpace]
//                                  |
//                      [SampleSpace == IdealSurfaceSpace]
//                                  |
//                          IdealSurfaceSpace
//                                  |
//                       [IdealSurfaceToIntermediateSurface]       
//                                  |
//                        (Intermediate)SurfaceSpace
//


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushIntermediateRealizer::CBrushIntermediateRealizer
//
//  Synopsis:
//      ctor
//

CBrushIntermediateRealizer::CBrushIntermediateRealizer(
    __in_ecount(1) const BrushContext *pBrushContext,
        // Brush context information
    __in_ecount(1)  const CMILMatrix *pmatContentToViewport,
        // Transform inferred from the Viewbox, Viewport, Stretch, & Alignment properties
    __in_ecount(1)  const CMILMatrix *pmatViewportToWorld,
        // User-specified Viewport->World transform
    __in_ecount(1)  const MilPointAndSizeD *prcdViewport,
        // User-specified viewport in world coordinates
    __in_ecount_opt(1) const BrushCachingParameters *pCachingParams
        // Optional brush-caching parameters.  If non-null, the realizer will determine
        // whether or not the previous realization can be re-used.
    )
{
    m_pBrushContext = pBrushContext;
    m_pmatContentToViewport = pmatContentToViewport;
    m_pmatViewportToWorld = pmatViewportToWorld;
    m_pCachingParams = pCachingParams;

    // Cast double-precision XYWH-rectangles to single-precision-LTRB
    MilRectFFromMilPointAndSizeD(m_rcViewport, *prcdViewport);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushIntermediateRealizer::CreateSurfaceAndContext
//
//  Synopsis:
//

HRESULT
CBrushIntermediateRealizer::CreateSurfaceAndContext(
    UINT uSurfaceWidth,
        // Width of render target to create
    UINT uSurfaceHeight,
        // Height of render target to create
    MilTileMode::Enum tileMode,                              
        // Brush tile mode
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTarget,
        // Output render target
    __deref_out_ecount(1) CDrawingContext **ppDrawingContext          
        // Output render context
    )
{
    HRESULT hr = S_OK;

    IRenderTargetInternal *pRTInternal = NULL;

    MilColorF transparentColor = { 0.0f, 0.0f, 0.0f, 0.0f };

    CMILMatrix matViewboxToSurface;

    //
    // Instantiate the the intermediate render target and context
    //

    IntermediateRTUsage rtUsage;
    rtUsage.flags = IntermediateRTUsage::ForBlending;
    if (m_pBrushContext->fBrushIsUsedFor3D)
    {
        rtUsage.flags |= IntermediateRTUsage::ForUseIn3D;
    }
    rtUsage.wrapMode = MILBitmapWrapModeFromTileMode(tileMode);

    IFC(m_pBrushContext->pRenderTargetCreator->CreateRenderTargetBitmap(
        uSurfaceWidth,
        uSurfaceHeight,
        rtUsage,
        MilRTInitialization::Default,
        ppIRenderTarget
        ));

    EventWriteWClientCreateIRT(NULL, m_pBrushContext->pBrushDeviceNoRef->GetCurrentResourceNoRef(), IRT_TileBrush);

    IFC((*ppIRenderTarget)->Clear(&transparentColor));

    IFC(CDrawingContext::Create(
        m_pBrushContext->pBrushDeviceNoRef,
        ppDrawingContext
        ));

    //
    // Clip to the surface bounds.
    //
    // Not only does this reduce overdraw, but having a clip is required
    // by CMilRenderContext to determine the size of intermediate surfaces
    // it creates.
    //

    {
        CMilRectF rcSurfaceBounds(
            0.0f,
            0.0f,
            static_cast<FLOAT>(uSurfaceWidth),
            static_cast<FLOAT>(uSurfaceHeight),
            XYWH_Parameters
            );

        IFC((*ppDrawingContext)->PushClipRect(rcSurfaceBounds));
    }

Cleanup:

    ReleaseInterfaceNoNULL(pRTInternal);

    RRETURN(hr);
}




