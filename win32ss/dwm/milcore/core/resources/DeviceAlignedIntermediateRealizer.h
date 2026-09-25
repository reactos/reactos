// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        
//      Header file for CDeviceAlignedIntermediateRealizer
//



//+----------------------------------------------------------------------------
//
//  Class:     
//      CDeviceAlignedIntermediateRealizer
//
//  Synopsis:  
//      Realizes device aligned intermediates for tile brushes. This is to be
//      used only with non-tiled* brushes in 2D.
//
//      * What it a non-tiled brush? In this context it is a TileBrush with
//        MilTileMode::Enum == MilTileMode::None. It does not include gradient brushes or
//        solid color brush.
//
//-----------------------------------------------------------------------------

class CDeviceAlignedIntermediateRealizer : public CBrushIntermediateRealizer
{
public:
    CDeviceAlignedIntermediateRealizer(
        __in_ecount(1) const BrushContext *pBrushContext,
        __in_ecount(1)  const CMILMatrix *pmatContentToViewport,
        __in_ecount(1)  const CMILMatrix *pmatViewportToWorld,
        __in_ecount(1)  const MilPointAndSizeD *prcdViewport
        );

    HRESULT Realize(
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTarget,
        __deref_out_ecount(1) CDrawingContext **ppDrawingContext,
        __out_ecount(1) CMILMatrix* pmatSurfaceToSampleSpace,
        __out_ecount(1) BOOL *pfBrushIsEmpty,
        __out_ecount_opt(1) CParallelogram *pSourceClipSampleSpace
        );

private:

    void CalculateSurfaceSizeAndMapping(
        __in_ecount(1) MilRectF *prcSampleSpaceRenderBounds,           
        __out_ecount(1) UINT *puSurfaceWidth,                             
        __out_ecount(1) UINT *puSurfaceHeight,                           
        __out_ecount(1) CMILMatrix *pmatSampleSpaceToSurface
        );

    void CalculateSurfaceSizeAndMapping1D(
        float rIdealSurfaceMin,
        float rIdealSurfaceMax,
        __out_ecount(1) __deref_out_range(1,INT_MAX) UINT *puSize,
        __out_ecount(1) float *prIdealToIntermediateScale,
        __out_ecount(1) float *prIdealToIntermediateOffset
        );

    static void AdjustSurfaceSizeAndMappingForMaxIntermediateSize1D(
        __inout_ecount(1) __deref_out_range(1,MAX_TILEBRUSH_INTERMEDIATE_SIZE) UINT *puSize,
        __inout_ecount(1) float *prIdealToIntermediateScale,
        __inout_ecount(1) float *prIdealToIntermediateOffset
        );
};


