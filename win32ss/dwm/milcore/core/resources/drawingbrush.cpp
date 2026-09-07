// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      The DrawingBrush CSlaveResource is responsible for maintaining
//      the current base values & animation resources for all  
//      DrawingBrush properties.  This class processes updates to those
//      properties, and updates a realization based on their current
//      value during GetBrushRealizationInternal.
//
//------------------------------------------------------------------------
#include "precomp.hpp"

MtDefine(DrawingBrushResource, MILRender, "DrawingBrush Resource");

MtDefine(CMilDrawingBrushDuce, DrawingBrushResource, "CMilDrawingBrushDuce");

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilDrawingBrushDuce::~CMilDrawingBrushDuce
//
//  Synopsis:  
//      Class destructor.
//
//-------------------------------------------------------------------------  
CMilDrawingBrushDuce::~CMilDrawingBrushDuce()
{
    UnRegisterNotifiers();
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilDrawingBrushDuce::DoesContainContent, CMilTileBrushDuce
//
//  Synopsis:  
//      Returns whether or not the drawing is non-NULL.
//
//  Notes:
//      If no content exists, then methods that require content such as
//      GetContentBounds and GetBaseTile won't be called, and can
//      assume that they aren't called.
//
//-------------------------------------------------------------------------    
HRESULT
CMilDrawingBrushDuce::DoesContainContent(
    __out_ecount(1) BOOL *pfHasContent
    // Whether or not the TileBrush has content
    ) const
{
    *pfHasContent = (m_data.m_pDrawing != NULL);
        
    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilDrawingBrushDuce::GetTilePropertyResources
//
//  Synopsis:  
//      Obtains the base values & resources of this brush's tile properties.
//
//-------------------------------------------------------------------------     
HRESULT 
CMilDrawingBrushDuce::GetTilePropertyResources(
    __out_ecount(1) double *pOpacity,
        // Base opacity property value
    __out_ecount(1) CMilSlaveDouble **ppOpacityAnimation,
        // Opacity property animations
    __out_ecount(1) CMilTransformDuce **ppTransformResource,
        // Transform property resource
    __out_ecount(1) CMilTransformDuce **ppRelativeTransformResource,
        // RelativeTransform property resource
    __out_ecount(1) MilBrushMappingMode::Enum *pViewportUnits,
        // ViewportUnits property value
    __out_ecount(1) MilBrushMappingMode::Enum *pViewboxUnits,
        // ViewboxUnits property value
    __out_ecount(1) MilPointAndSizeD *pViewport,
        // Base Viewport property value
    __out_ecount(1) CMilSlaveRect **ppViewportAnimations,
        // Viewport property animations
    __out_ecount(1) MilPointAndSizeD *pViewbox,
        // Base Viewbox property value
    __out_ecount(1) CMilSlaveRect **ppViewboxAnimations,
        // Viewbox property animations
    __out_ecount(1) MilStretch::Enum *pStretch,
        // Stretch property value
    __out_ecount(1) MilTileMode::Enum *pTileMode,
        // MilTileMode::Enum property value
    __out_ecount(1) MilHorizontalAlignment::Enum *pAlignmentX,
        // AlignmentX property value
    __out_ecount(1) MilVerticalAlignment::Enum *pAlignmentY,
        // AlignmentY property value
    __out_ecount(1) double *pCacheInvalidationThresholdMinimum,
        // Low end of the CacheInvalidationThreshold range
    __out_ecount(1) double *pCacheInvalidationThresholdMaximum    
        // High end of the CacheInvalidationThreshold range    
    ) const
{
    *pOpacity = m_data.m_Opacity;
    *ppOpacityAnimation = m_data.m_pOpacityAnimation;
    *ppTransformResource = m_data.m_pTransform;
    *ppRelativeTransformResource = m_data.m_pRelativeTransform;
    *pViewportUnits = m_data.m_ViewportUnits;
    *pViewboxUnits = m_data.m_ViewboxUnits;
    *pViewport = m_data.m_Viewport;
    *ppViewportAnimations = m_data.m_pViewportAnimation;
    *pViewbox = m_data.m_Viewbox;
    *ppViewboxAnimations = m_data.m_pViewboxAnimation;
    *pStretch = m_data.m_Stretch;
    *pTileMode = m_data.m_TileMode;
    *pAlignmentX = m_data.m_AlignmentX;
    *pAlignmentY = m_data.m_AlignmentY;
    *pCacheInvalidationThresholdMinimum = m_data.m_CacheInvalidationThresholdMinimum;
    *pCacheInvalidationThresholdMaximum = m_data.m_CacheInvalidationThresholdMaximum;

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilDrawingBrushDuce::GetContentBounds, CMilTileBrushDuce
//
//  Synopsis:  
//      Obtains the bounds of the source content, in device-independent 
//      content units.
//
//-------------------------------------------------------------------------    
HRESULT 
CMilDrawingBrushDuce::GetContentBounds(
    __in_ecount(1) const BrushContext *pBrushContext, 
        // Context the brush is being realized for        
    __out_ecount(1) CMilRectF *pContentBounds
        // Output content bounds
    )
{
    Assert(pBrushContext && pContentBounds);

    #if DBG
    Assert(DbgHasContent());
    #endif
    
    INLINED_RRETURN(pBrushContext->pContentBounder->GetContentBounds(
        m_data.m_pDrawing,
        pContentBounds // Delegate setting of out-param
        ));
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilDrawingBrushDuce::DrawIntoBaseTile, CMilTileBrushDuce
//
//  Synopsis:  
//      Draws this brush's content into an already-allocated
//      DrawingContext.  This method is used to populate the intermediate
//      surface realization.
//
//-------------------------------------------------------------------------  
HRESULT 
CMilDrawingBrushDuce::DrawIntoBaseTile(
    __in_ecount(1) const BrushContext *pBrushContext,
        // Context the brush is being realized for        
    __in_ecount(1) CMilRectF * prcSurfaceBounds,
        // Bounds of the intermediate suface
    __inout_ecount(1) CDrawingContext *pDrawingContext
        // Drawing context to draw content into
    ) 
{ 

    // DrawIntoBaseTile isn't called if this brush has no content
    Assert(m_data.m_pDrawing != NULL);    
    
    RRETURN(m_data.m_pDrawing->Draw(pDrawingContext));
}



