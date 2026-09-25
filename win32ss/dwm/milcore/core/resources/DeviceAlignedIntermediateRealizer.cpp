// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        
//      CPP file for CDeviceAlignedIntermediateRealizer
//

#include "precomp.hpp"

//
// "Spaces and Transforms"
// See BrushIntermediateRealizer.cpp
//


//+----------------------------------------------------------------------------
//
//  Member: 
//      CDeviceAlignedIntermediateRealizer::CDeviceAlignedIntermediateRealizer
//
//  Synopsis:  
//      ctor
//

CDeviceAlignedIntermediateRealizer::CDeviceAlignedIntermediateRealizer(
    __in_ecount(1) const BrushContext *pBrushContext,
        // Brush context information
    __in_ecount(1)  const CMILMatrix *pmatContentToViewport,
        // Transform inferred from the Viewbox, Viewport, Stretch, & Alignment properties
    __in_ecount(1)  const CMILMatrix *pmatViewportToWorld,
        // User-specified Viewport->World transform
    __in_ecount(1)  const MilPointAndSizeD *prcdViewport
        // User-specified viewport in world coordinates
    ) : CBrushIntermediateRealizer(
            pBrushContext,
            pmatContentToViewport,
            pmatViewportToWorld,
            prcdViewport,
            NULL
            )
{
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDeviceAlignedIntermediateRealizer::Realize
//
//  Synopsis:  
//      Realizes brush to intermediate surface.
//

HRESULT
CDeviceAlignedIntermediateRealizer::Realize(
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTarget,
        // Intermediate render target for this tile brush
    __deref_out_ecount(1) CDrawingContext **ppDrawingContext,
        // Drawing context to use on intermediate render target
    __out_ecount(1) CMILMatrix* pmatSurfaceToSampleSpace,
        // Transforms from the ouput intermediate surface to SampleSpace coordinates
    __out_ecount(1) BOOL *pfBrushIsEmpty,
        // Does this brush render nothing?,
    __out_ecount_opt(1) CParallelogram *pSourceClipSampleSpace
        // Clip parallelogram to apply to the source content.  This is used
        // to prevent content from rendering outside of the tile.
    )
{
    HRESULT hr = S_OK;

    //
    // Calculate ViewportToSampleSpace transform
    //

    CMILMatrix matViewportToIdealSurfaceSampleSpace;
    CMILMatrix matIdealSurfaceToIntermediateSurface;
    CMILMatrix matContentToSurface;
    UINT uSurfaceWidth;
    UINT uSurfaceHeight; 

    matViewportToIdealSurfaceSampleSpace.SetToMultiplyResult(
        *m_pmatViewportToWorld,
        m_pBrushContext->matWorldToSampleSpace
        );

    //
    // Transform viewport to sample space... this is one of the clips we must
    // take into consideration. It is NOT okay to grow this bounds upon a
    // rotate tranform, so we keep it as a parallelogram.
    //

    CParallelogram viewportClipInSampleSpace;
    viewportClipInSampleSpace.Set(m_rcViewport);
    viewportClipInSampleSpace.Transform(&matViewportToIdealSurfaceSampleSpace);


    //
    // Set the source clip
    //
    if (pSourceClipSampleSpace)
    {
        *pSourceClipSampleSpace = viewportClipInSampleSpace;
    }


    //
    // Transform world space bounds into sample space... this, while not
    // a clip, can be treated the same way- it helps to form the extents
    // of the brush that need realizing. It is okay to grow these
    // bounds during the transform process. We want an axis aligned
    // rectangle in the end.
    //

    MilRectF rcShapeBoundsInSampleSpace;
    m_pBrushContext->matWorldToSampleSpace.Transform2DBoundsConservative(
        m_pBrushContext->rcWorldSpaceBounds,
        OUT rcShapeBoundsInSampleSpace
        );

    //
    // Calculate the render bounds in sample space. The render bounds should
    // account for the shape bounds, the sample space clip, and the viewport clip
    //

    CMilRectF rcRenderBoundsInSampleSpace(rcShapeBoundsInSampleSpace);
    if (   !rcRenderBoundsInSampleSpace.HasValidValues()
        || !rcRenderBoundsInSampleSpace.Intersect(m_pBrushContext->rcSampleSpaceClip)
       )
    {
        *pfBrushIsEmpty = TRUE;
        goto Cleanup;
    }

    {
        CMilRectF rcViewportBounds;

        viewportClipInSampleSpace.GetTightBounds(
            OUT rcViewportBounds
            );

        if (!rcRenderBoundsInSampleSpace.Intersect(rcViewportBounds))
        {
            *pfBrushIsEmpty = TRUE;
            goto Cleanup;
        }
    }

    CalculateSurfaceSizeAndMapping(
        &rcRenderBoundsInSampleSpace,     
        &uSurfaceWidth,                             
        &uSurfaceHeight,                           
        &matIdealSurfaceToIntermediateSurface
        );

    //
    // Calculate the IntermediateSurface space to IdealSurface space transform
    //
    // This inverted matrix is needed to calculate the Surface to World 
    // transform.
    //
    pmatSurfaceToSampleSpace->SetToInverseOfTranslateOrScale(
        matIdealSurfaceToIntermediateSurface
        );

    //
    // Calculate the content to surface transform
    //

    matContentToSurface.SetToMultiplyResult(
        *m_pmatContentToViewport,
        matViewportToIdealSurfaceSampleSpace
        );
    matContentToSurface.Multiply(
        matIdealSurfaceToIntermediateSurface
        );

    //
    // Create Surface and Context
    //

    IFC(CreateSurfaceAndContext(
        uSurfaceWidth,
        uSurfaceHeight,
        MilTileMode::None,
        ppIRenderTarget,
        ppDrawingContext
        ));

    IFC((*ppDrawingContext)->PushTransform(
        &matContentToSurface
        ));

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CDeviceAlignedIntermediateRealizer::CalculateSurfaceSizeAndMapping
//
//  Synopsis:  
//      Calculates the size of the intermediate surface and the transform that
//      will map the render bounds to the area of the sample space we wish to
//      draw.
//

void
CDeviceAlignedIntermediateRealizer::CalculateSurfaceSizeAndMapping(
    __in_ecount(1) MilRectF *prcIdealSurfaceSampleSpace,           
        // Bound of rendered shape
    __out_ecount(1) UINT *puSurfaceWidth,                             
        // Output width of intermediate surface
    __out_ecount(1) UINT *puSurfaceHeight,                           
        // Output height of intermediate surface
    __out_ecount(1) CMILMatrix *pmatIdealSurfaceToIntermediateSurface
        // Mapping from ideal surface space to intermediate surface space
    )
{
    pmatIdealSurfaceToIntermediateSurface->SetToIdentity();

    CalculateSurfaceSizeAndMapping1D(
        prcIdealSurfaceSampleSpace->left,
        prcIdealSurfaceSampleSpace->right,
        puSurfaceWidth,
        &pmatIdealSurfaceToIntermediateSurface->_11,
        &pmatIdealSurfaceToIntermediateSurface->_41
        );

    CalculateSurfaceSizeAndMapping1D(
        prcIdealSurfaceSampleSpace->top,
        prcIdealSurfaceSampleSpace->bottom,
        puSurfaceHeight,
        &pmatIdealSurfaceToIntermediateSurface->_22,
        &pmatIdealSurfaceToIntermediateSurface->_42
        );

    AdjustSurfaceSizeAndMappingForMaxIntermediateSize1D(
        puSurfaceWidth,
        &pmatIdealSurfaceToIntermediateSurface->_11,
        &pmatIdealSurfaceToIntermediateSurface->_41
        );

    AdjustSurfaceSizeAndMappingForMaxIntermediateSize1D(
        puSurfaceHeight,
        &pmatIdealSurfaceToIntermediateSurface->_22,
        &pmatIdealSurfaceToIntermediateSurface->_42
        );
}



//+----------------------------------------------------------------------------
//
//  Member:    
//      CDeviceAlignedIntermediateRealizer::CalculateSurfaceSizeAndMapping1D
//
//  Synopsis:
//      For one dimension this helper determines the actual size
//      (pre-max-texture-cap) of the intermediate surface and the mapping from
//      the ideal tile in 'scaled world space' to the intermediate surface.
//
//-----------------------------------------------------------------------------
void
CDeviceAlignedIntermediateRealizer::CalculateSurfaceSizeAndMapping1D(
    float rIdealSurfaceMin,
    float rIdealSurfaceMax,
    __out_ecount(1) __deref_out_range(1,INT_MAX) UINT *puSize,
    __out_ecount(1) float *prIdealToIntermediateScale,
    __out_ecount(1) float *prIdealToIntermediateOffset
    )
{
    //
    // This code only works for 2D, but that is okay because no one should
    // use this class for 3D brush contexts.
    //
    Assert(!m_pBrushContext->fBrushIsUsedFor3D);

    //
    // We don't expect the surface min/max to be outside int-to-float range
    // because in 2D the ideal surface bounds are no bigger than the render
    // target bounds.
    //
    Assert(rIdealSurfaceMin >= -MAX_INT_TO_FLOAT);
    Assert(rIdealSurfaceMax <= MAX_INT_TO_FLOAT);

    INT uSurfaceMin = CFloatFPU::Floor(rIdealSurfaceMin);
    INT uSurfaceMax = CFloatFPU::Ceiling(rIdealSurfaceMax);
    
    *puSize = static_cast<UINT>(uSurfaceMax - uSurfaceMin);
    *puSize = max(*puSize, 1U);

    *prIdealToIntermediateScale = 1.0f;
    *prIdealToIntermediateOffset = -static_cast<FLOAT>(uSurfaceMin);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDeviceAlignedIntermediateRealizer::AdjustSurfaceSizeAndMappingForMaxIntermediateSize1D
//
//  Synopsis:
//      For one dimension this helper determines the actual size
//      (post-max-texture-cap) of the intermediate surface and the mapping from
//      the ideal tile in 'scaled world space' to the intermediate surface.
//
//-----------------------------------------------------------------------------
void
CDeviceAlignedIntermediateRealizer::AdjustSurfaceSizeAndMappingForMaxIntermediateSize1D(
    __inout_ecount(1) __deref_out_range(1,MAX_TILEBRUSH_INTERMEDIATE_SIZE) UINT *puSize,
    __inout_ecount(1) float *prIdealToIntermediateScale,
    __inout_ecount(1) float *prIdealToIntermediateOffset
    )
{
    Assert(*puSize > 0);

    C_ASSERT(IS_POWER_OF_2(MAX_TILEBRUSH_INTERMEDIATE_SIZE));

    //
    // Cap the size at the MAX_TILEBRUSH_INTERMEDIATE_SIZE to avoid creating intermediates
    // with near infinite dimensions.
    //
    if (*puSize > MAX_TILEBRUSH_INTERMEDIATE_SIZE)
    {
        float rSizeCapScale = static_cast<float>(MAX_TILEBRUSH_INTERMEDIATE_SIZE)/static_cast<float>(*puSize);
        *prIdealToIntermediateScale = *prIdealToIntermediateScale * rSizeCapScale;
        *prIdealToIntermediateOffset = *prIdealToIntermediateOffset * rSizeCapScale;
        *puSize = MAX_TILEBRUSH_INTERMEDIATE_SIZE;
    }
}


