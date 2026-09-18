// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      The VisualBrush CSlaveResource is responsible for maintaining
//      the current base values & animation resources for all  
//      VisualBrush properties.  This class processes updates to those
//      properties, and updates a realization based on their current
//      value during GetBrushRealizationInternal.
//
//------------------------------------------------------------------------
#include "precomp.hpp"

MtDefine(VisualBrushResource, MILRender, "VisualBrushDuce Resource");

MtDefine(CMilVisualBrushDuce, VisualBrushResource, "CMilVisualBrushDuce");

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilVisualBrushDuce::~CMilVisualBrushDuce
//
//  Synopsis:  
//      Class destructor.
//
//-------------------------------------------------------------------------  
CMilVisualBrushDuce::~CMilVisualBrushDuce()
{
    delete m_pPreComputeContext;
    UnRegisterNotifiers();
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilVisualBrushDuce::DoesContainContent, CMilTileBrushDuce
//
//  Synopsis:  
//      Returns whether or not the visual is non-NULL.
//
//  Notes:
//      If no content exists, then methods that require content such as
//      GetContentBounds and GetBaseTile won't be called, and can
//      assume that they aren't called.
//
//-------------------------------------------------------------------------    
HRESULT
CMilVisualBrushDuce::DoesContainContent(
    __out_ecount(1) BOOL *pfHasContent
    // Whether or not the TileBrush has content
    ) const
{
    *pfHasContent = (m_data.m_pVisual != NULL);
        
    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilVisualBrushDuce::GetTilePropertyResources
//
//  Synopsis:  
//      Obtains the base values & resources of this brush's tile properties.
//
//-------------------------------------------------------------------------     
HRESULT 
CMilVisualBrushDuce::GetTilePropertyResources(
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
//      CMilVisualBrushDuce::GetContentBounds, CMilTileBrushDuce
//
//  Synopsis:  
//      Obtains the bounds of the source content, in device-independent 
//      content units.
//
//-------------------------------------------------------------------------    
HRESULT 
CMilVisualBrushDuce::GetContentBounds(
    __in_ecount(1) const BrushContext *pBrushContext, 
        // Context the brush is being realized for        
    __out_ecount(1) CMilRectF *pContentBounds
        // Output content bounds
    )
{
    HRESULT hr = S_OK;

    // GetContentBounds mustn't be called if visual content doesn't 
    // exist (i.e., m_data.m_pVisual is guaranteed to be non-NULL).
    #if DBG
    Assert(DbgHasContent());
    #endif

    //
    // PreCompute must be called before bounding
    //
    
    IFC(PreCompute(pBrushContext));
    
    //
    // Obtain the bounds the Viewbox is relative to.
    //
    // The entire Visual, including Tranform, Offset, & Clip
    // is rendered into the intermediate surface, so the VisualBrush
    // must be relative to those same bounds (i.e., the outer bounds
    // which includes the Transform, Offset & Clip).
    //       
    
    *pContentBounds = m_data.m_pVisual->GetOuterBounds();
    
Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilVisualBrushDuce::PreCompute
//
//  Synopsis:  
//      Calls PreCompute on the current Visual content
//
//  Notes:
//      CPreComputeContext::PreCompute avoids a full traversal if a
//      PreCompute has already been done and isn't needed, so it
//      is acceptable to call PreCompute multiple times.  This
//      fact allows us to avoid writing logic which would avoid
//      calling PreCompute twice (once potentially during GetContentBounds,
//      and again during DrawIntoBaseTile).
//
//-------------------------------------------------------------------------
HRESULT
CMilVisualBrushDuce::PreCompute(
    __in_ecount(1) const BrushContext *pBrushContext
        // Context the brush is being realized for           
    ) const
{
    HRESULT hr = S_OK;

    if (m_pPreComputeContext == NULL)
    {
        IFC(CPreComputeContext::Create(
            pBrushContext->pBrushDeviceNoRef,
            &m_pPreComputeContext
            ));
    }

    #if DBG
    Assert(DbgHasContent());
    #endif

    IFC(PreComputeHelper(
        m_pPreComputeContext,
        m_data.m_pVisual
        ));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilVisualBrushDuce::PreComputeHelper
//
//  Synopsis:  
//      Calls PreCompute on the current Visual content and takes care of 
//      the special logic of what to do with dirty regions encountered during
//      the walk.
//
//-------------------------------------------------------------------------
HRESULT
CMilVisualBrushDuce::PreComputeHelper(
    __in CPreComputeContext *pPreComputeContext,
    __in CMilVisual *pVisual
    )
{   
    HRESULT hr = S_OK;
    CMilRectF bounds;

    bounds = bounds.sc_rcInfinite;

    IFC(pPreComputeContext->PreCompute(
        pVisual,
        &bounds, // Infinite surface bounds 
        0,       // No extra invalid regions
        NULL,    // No extra invalid regions
        0,       // No dirty region coalescing
        CDrawingContext::DefaultInterpolationMode, // Interpolation mode.       
        NULL     // No scroll area - scrolling not supported inside VB
        ));

    //
    //      Precompute on the VisualBrush root is called during the render pass instead of 
    //      the precompute pass. 
    //      So things are fine if we have just one HwndSource (visualTree) because precompute has
    //         already been called for each node before we start the walk from VisualBrush root
    //      However, if we have two Hwnd Sources, call them A and B, such that 
    //      the visualBrush on B points to a node in A, 
    //     
    //              A                   B
    //      _________________   _________________
    //      |               |   |    (Parent)   |
    //      |               |   |       |       |
    //      |               |   |    (Node1)    |
    //      |               |   |       |       |
    //      |               |   |  VisualBrush  |
    //      |  (Node2) <----------------'       |
    //      |               |   |               |
    //      -----------------   -----------------
    //
    //      then the following happens:-
    //        1) Precompute is called for A's tree
    //        2) Rendering is done for A's tree
    //             The rendering for Node2 (in A) leads to calling precompute for Node1 (in B).
    //             The precompute pass will collect dirty regions and reset the flags on Node1
    //             ** So, to prevent the loss of these dirty regions, we now save them as
    //             ** AdditionalDirtyRegion on the Parent of the VisualBrush root
    //        3) Precompute is called for B's tree
    //             We collect the earlier saved info on Parent through AdditionDirtyRegions
    //        4) Rendering is done for B's tree
    //


    // If we collected any dirty region, then add that as AdditionDirtyRegion
    // on the parent of the root
    CMilVisual *parent = pVisual->GetParent();
    if (parent)
    {
        const MilRectF *prgrcDirtyRegions = pPreComputeContext->GetUninflatedDirtyRegions();
        UINT count = pPreComputeContext->GetDirtyRegionCount();
        Assert(prgrcDirtyRegions);
        
        for (UINT i = 0; i < count; i++)
        {
            MilRectF region = prgrcDirtyRegions[i];
            parent->AddAdditionalDirtyRects(&region);
        }                                
    }

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CMilVisualBrushDuce::DrawIntoBaseTile, CMilTileBrushDuce
//
//  Synopsis:  
//      Draws this brush's content into an already-allocated
//      DrawingContext.  This method is used to populate the intermediate
//      surface realization.
//
//-------------------------------------------------------------------------  
HRESULT 
CMilVisualBrushDuce::DrawIntoBaseTile(
    __in_ecount(1) const BrushContext *pBrushContext,
        // Context the brush is being realized for        
    __in_ecount(1) CMilRectF * prcSurfaceBounds,
        // Bounds of the intermediate suface
    __inout_ecount(1) CDrawingContext *pDrawingContext
        // Drawing context to draw content into
    ) 
{ 
    HRESULT hr = S_OK;      

    MilColorF clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };      
    IRenderTargetInternal *pIRT = NULL;
    
    Assert((prcSurfaceBounds->right - prcSurfaceBounds->left) <= INT_MAX);
    Assert((prcSurfaceBounds->bottom - prcSurfaceBounds->top) <= INT_MAX);

    #if DBG
    Assert(DbgHasContent());
    #endif
    
    // PreCompute must be called before rendering
    IFC(PreCompute(pBrushContext)); 

    // Ensure each cache marked dirty this frame by precompute is up-to-date.
    IFC(pBrushContext->pBrushDeviceNoRef->GetVisualCacheManagerNoRef()->UpdateCaches());

    IFC(pDrawingContext->DrawVisualTree(
        m_data.m_pVisual,
        &clearColor, 
        *prcSurfaceBounds,
        true
        ));
    
Cleanup:
    ReleaseInterface(pIRT);
    RRETURN(hr);
}



