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
//      Header file for brush intermediate realizer base class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------



//
// We choose 2048 as the maximum tilebrush size because this is the largest
// dimension that a hardware intermediate can be before we reach the limits of
// some graphics cards. 2048 == minimum max texture size.
//
#define MAX_TILEBRUSH_INTERMEDIATE_SIZE 2048

//+-----------------------------------------------------------------------------
//
//  Class:
//      CBrushIntermediateRealizer
//
//  Synopsis:
//      Realizes intermediates for brushees. Tiled and non-tiled brushes are
//      specaial cased with subclasses
//
//------------------------------------------------------------------------------

class CBrushIntermediateRealizer
{
public:
    CBrushIntermediateRealizer(
        __in_ecount(1) const BrushContext *pBrushContext,
        __in_ecount(1)  const CMILMatrix *pmatContentToViewport,
        __in_ecount(1)  const CMILMatrix *pmatViewportToWorld,
        __in_ecount(1)  const MilPointAndSizeD *prcdViewport,
        __in_ecount_opt(1) const BrushCachingParameters *pCachingParams
        );

protected:

    HRESULT CreateSurfaceAndContext(
        UINT uSurfaceWidth,
        UINT uSurfaceHeight,
        MilTileMode::Enum tileMode,                              
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTarget,
        __deref_out_ecount(1) CDrawingContext **ppDrawingContext          
        );

protected:

    const BrushContext *m_pBrushContext;
    const CMILMatrix *m_pmatContentToViewport;
    const CMILMatrix *m_pmatViewportToWorld;     
    const BrushCachingParameters *m_pCachingParams;
    MilRectF m_rcViewport;
};


