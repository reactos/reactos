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
//      Definition of tile brush methods used to create intermediate
//      representations from user-defined state.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


//+-----------------------------------------------------------------------------
//
//  Class:
//      CTileBrushUtils
//
//  Synopsis:
//      Contains static utility methods for creating tile brush representations
//      from user-specified state.
//
//------------------------------------------------------------------------------
class CTileBrushUtils
{
public:      

    static HRESULT CreateTileBrushIntermediate(
        __in_ecount(1) const BrushContext *pBrushContext,
        __in_ecount(1) const CMILMatrix *pmatContentToViewport,
        __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        __in_ecount(1) const MilPointAndSizeD *prcdViewport,
        __in_ecount_opt(1) const BrushCachingParameters *pCachingParams,   
        MilTileMode::Enum tileMode,        
        __deref_out_ecount_opt(1) IWGXBitmapSource **ppCachedSurface,
        __deref_out_ecount_opt(1) IMILRenderTargetBitmap **ppIRenderTarget,
        __deref_out_ecount_opt(1) CDrawingContext **ppDrawingContext,
        __out_ecount(1) CMILMatrix* pmatSurfaceToXSpace,
        __out_ecount(1) BOOL *pfBrushIsEmpty,
        __out_ecount_opt(1) BOOL *pfUseSourceClip,
        __out_ecount_opt(1) BOOL *pfSourceClipIsEntireSource,
        __out_ecount_opt(1) CParallelogram *pSourceClipXSpace,
        __out_ecount(1) XSpaceDefinition *pXSpaceDefinition
        );

    static HRESULT GetIntermediateBaseTile(
        __in_ecount(1) CMilTileBrushDuce *pTileBrush,
        __in_ecount(1) const BrushContext *pBrushContext,
        __in_ecount(1) const CMILMatrix *pContentToViewport,
        __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        __in_ecount(1) const MilPointAndSizeD *pViewport,
        __in_ecount_opt(1) const BrushCachingParameters *pCachingParams,
        __in MilTileMode::Enum tileMode,
        __out_ecount(1) IWGXBitmapSource **ppBaseTile,
        __out_ecount(1) CMILMatrix *pmatIntermediateBitmapToXSpace,
        __out_ecount(1) BOOL *pfTileIsEmpty,
        __out_ecount(1) BOOL *pfUseSourceClip,
        __out_ecount(1) BOOL *pfSourceClipIsEntireSource,
        __out_ecount(1) CParallelogram *pSourceClipXeSpace,
        __out_ecount(1) XSpaceDefinition *pXSpaceDefinition
        );
    
    static VOID CalculateTileBrushMapping(
        __in_ecount_opt(1) const CMILMatrix *pTransform,
        __in_ecount_opt(1) const CMILMatrix *pRelativeTransform,    
        MilStretch::Enum stretch,
        MilHorizontalAlignment::Enum alignmentX,
        MilVerticalAlignment::Enum alignmentY,
        MilBrushMappingMode::Enum viewportUnits,
        MilBrushMappingMode::Enum viewboxUnits,
        __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        __in_ecount(1) const MilPointAndSizeD *pContentBounds,      
        REAL rContentScaleX,
        REAL rContentScaleY,    
        __inout_ecount(1) MilPointAndSizeD *pViewport,
        __inout_ecount(1) MilPointAndSizeD *pViewbox,
        __out_ecount_opt(1) CMILMatrix *pContentToViewport,
        __out_ecount_opt(1) CMILMatrix *pmatViewportToWorld,
        __out_ecount(1) CMILMatrix *pContentToWorld,
        __out_ecount(1) BOOL *pfBrushIsEmpty
        );


    static VOID CalculateViewboxToViewportMapping(
        __in_ecount(1) const MilPointAndSizeD *prcViewport,        
        __in_ecount(1) const MilPointAndSizeD *prcViewbox,        
        MilStretch::Enum stretch,
        MilHorizontalAlignment::Enum halign,
        MilVerticalAlignment::Enum valign,
        __out_ecount(1) CMILMatrix *pmatViewboxToViewport
        );    

private:    

    static VOID GetAbsoluteViewRectangles(
        MilBrushMappingMode::Enum viewportUnits,
        MilBrushMappingMode::Enum viewboxUnits,
        __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        __in_ecount(1) const MilPointAndSizeD *pContentBounds,
        __inout_ecount(1) MilPointAndSizeD *pViewport,
        __inout_ecount(1) MilPointAndSizeD *pViewbox,
        __out_ecount(1) BOOL *pfIsEmpty
        );
};



