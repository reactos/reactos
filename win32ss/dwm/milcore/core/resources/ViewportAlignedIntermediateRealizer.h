// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        
//      Header file for CViewportAlignedIntermediateRealizer
//


namespace TileMode1D
{
    enum Enum
    {
        None   = 0,
        Flip   = 1,
        Tile   = 2
    };
};


//+----------------------------------------------------------------------------
//
//  Class:     
//      CViewportAlignedIntermediateRealizer
//
//  Synopsis:  
//      Realizes intermediates for tiled brushes
//
//-----------------------------------------------------------------------------

class CViewportAlignedIntermediateRealizer : public CBrushIntermediateRealizer
{
public:
    CViewportAlignedIntermediateRealizer(
        __in_ecount(1) const BrushContext *pBrushContext,
        __in_ecount(1)  const CMILMatrix *pmatContentToViewport,
        __in_ecount(1)  const CMILMatrix *pmatViewportToWorld,
        __in_ecount(1)  const MilPointAndSizeD *prcdViewport,
        __in_ecount_opt(1) const BrushCachingParameters *pCachingParams,
        MilTileMode::Enum tileMode
        );

    HRESULT Realize(
        __deref_out_ecount_opt(1) IWGXBitmapSource **ppCachedSurface,
        __deref_out_ecount_opt(1) IMILRenderTargetBitmap **ppIRenderTarget,
        __deref_out_ecount_opt(1) CDrawingContext **ppDrawingContext,
        __out_ecount(1) CMILMatrix* pmatSurfaceToWorldSpace,
        __out_ecount(1) BOOL *pfBrushIsEmpty,
        __out_ecount_opt(1) CParallelogram *pSurfaceBoundsWorldSpace
        );

private:

    void CalculateIdealSurfaceSpaceBaseTile(
        __in_ecount_opt(1) const CMILMatrix *pmatScaleOfViewportToWorld,
        __in_ecount_opt(1) const CMILMatrix *pmatNonScaleOfViewportToWorld,  
        __in_ecount(1) const CMILMatrix *pmatScaleOfWorldToSampleSpace,
        __in_ecount(1) const CMILMatrix *pmatWorldToSampleSpace,
        __inout_ecount(1) BOOL *pfBrushIsEmpty,
        __out_ecount(1) MilRectF *prcBaseTile_SampleScaledViewportSpace,
        __out_ecount(1) MilRectF *rcRenderBounds_SampleScaledViewportSpace
        );

    void CalculateSurfaceSizeAndMapping(
        __in_ecount(1) MilRectF *prcBaseTile_SampleScaledViewportSpace,
        __in_ecount(1) MilRectF *rcRenderBounds_SampleScaledViewportSpace,
        __inout_ecount(1) BOOL *pfBrushIsEmpty,             
        __out_ecount(1) UINT *puSurfaceWidth,
        __out_ecount(1) UINT *puSurfaceHeight,
        __out_ecount(1) CMILMatrix *pmatBaseTile_SampleScaledViewportSpaceToBaseTile_SurfaceSpace,
        __out_ecount(1) CMilPoint2F *pvecTranslationFromRenderedTileToBaseTile
        );

    void CalculateSurfaceSizeAndMapping1D(
        TileMode1D::Enum tileMode1D,
        float rBaseTileMin_SampledScaledViewport,
        float rBaseTileMax_SampledScaledViewport,
        float rRenderBoundsMin,
        float rRenderBoundsMax,
        __inout_ecount(1) BOOL *pfBrushIsEmpty,             
        __out_ecount(1) __deref_out_range(1,INT_MAX) UINT *puSize,
        __out_ecount(1) float *prSampleScaledViewportToSurfaceScale,
        __out_ecount(1) float *prSampleScaledViewportToSurfaceOffset,
        __out_ecount(1) float *pflTranslationFromRenderedTileToBaseTile
        );
    
    static void CalculateTileShiftForPixel(
        INT iPixel,
        INT iTileSize,
        __out_ecount(1) INT *piTileShift
        );

    void AdjustSurfaceSizeAndMapping1D(
        UINT uSizeInOtherDimension,
        __inout_ecount(1) __deref_out_range(1,MAX_TILEBRUSH_INTERMEDIATE_SIZE) UINT *puSize,
        __inout_ecount(1) float *prSampleScaledViewportToSurfaceScale,
        __inout_ecount(1) float *prSampleScaledViewportToSurfaceOffset
        );

    HRESULT CreateSurfaceAndContext(
        __in_ecount(1) const CMILMatrix *pmatContentToViewport,
        __in_ecount(1) const CMILMatrix *pmatScaleOfViewportToWorld,
        __in_ecount(1) const CMILMatrix *matScaleOfWorldToSampleSpace,
        __in_ecount(1) const CMILMatrix *pmatSampleScaledViewportSpaceToSurfaceSpace,
        UINT uSurfaceWidth,
        UINT uSurfaceHeight,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTarget,
        __deref_out_ecount(1)  CDrawingContext **ppDrawingContext
        );

    static void CalculateSurfaceToWorldMapping(
        __in_ecount(1)  const CMILMatrix *pmatRenderedTile_SurfaceSpaceToBaseTile_SampleScaledViewportSpace,
        __in_ecount(1)  const CMILMatrix *pmatUserBrushWithoutScale,
        __in_ecount(1)  const CMILMatrix *pmatWorldScale,
        __out_ecount(1) CMILMatrix *pmatSurfaceToWorld
        );

private:
    MilTileMode::Enum m_tileMode;
};


