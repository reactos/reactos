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
//      The abtract TileBrush CSlaveResource contains general functionality
//      common to TileBrush subclasses, and specialized abstract methods
//      TileBrush subclases must implement.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(TileBrushResource, MILRender, "TileBrush Resource");
MtDefine(CMilTileBrushDuce, TileBrushResource, "CMilTileBrushDuce");


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilTileBrushDuce::CMilTileBrushDuce
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CMilTileBrushDuce::CMilTileBrushDuce(
    __in_ecount(1) CComposition *pComposition
    )
    : CMilBrushDuce(pComposition)
{ 
    m_pIntermediateCache = NULL;  
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilTileBrushDuce::GetBrushRealizationCore, CMilBrushDuce
//
//  Synopsis:
//
//------------------------------------------------------------------------------
HRESULT
CMilTileBrushDuce::GetBrushRealizationInternal(
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
    )
{
    HRESULT hr = S_OK;

    //
    // Variables to hold current TileBrush property values
    //

    FLOAT opacity;
    const CMILMatrix *pTransform = NULL;
    const CMILMatrix *pRelativeTransform = NULL;
    MilBrushMappingMode::Enum viewportUnits;
    MilBrushMappingMode::Enum viewboxUnits;
    MilPointAndSizeD viewport;
    MilPointAndSizeD viewbox;
    MilStretch::Enum stretch;
    MilTileMode::Enum tileMode;
    MilHorizontalAlignment::Enum alignmentX;
    MilVerticalAlignment::Enum alignmentY;  
    double rCacheInvalidationThresholdMinimumD;
    double rCacheInvalidationThresholdMaximumD;

    // Source clip variables
    BOOL fUseSourceClip;
    BOOL fSourceClipIsEntireSource;
    CParallelogram sourceClipXSpace;
    XSpaceDefinition xSpaceDefinition;

    //
    // Variables that hold transform information
    //

    // Combined result matrix that maps from the source content to the final transformed
    // tile in world coordinates (the same coordinate space shapes exist in).  
    CMILMatrix matContentToWorld;

    // Matrix that maps from the content to the user-specified Viewport
    CMILMatrix matContentToViewport;

    // User-specified transform that is applied after the Viewbox to Viewport transform
    CMILMatrix matViewportToWorld;

    // Matrix that maps from the base tile of the brush to the sample space
    CMILMatrix matBaseTileToXSpace;

    //
    // Other variable declarations
    // 

    IWGXBitmapSource *pBaseTile  = NULL;
    BOOL fBrushIsEmpty = FALSE;
    BOOL fHasContent = FALSE;

    REAL rContentScaleX, rContentScaleY; 

    //
    // Bounding rectangles for content
    //

    CMilRectF rcContentBoundsF;
    MilPointAndSizeD rcContentBoundsD;

    // Initialize rcContentBoundsD in DBG builds to guard against using an uninitialized
    // content bounding box.
    rcContentBoundsF.SetEmpty();
    WHEN_DBG(rcContentBoundsD = MilEmptyPointAndSizeD);

    bool fInvalidateBrushCache = true;

    //
    // Validate required in-params
    //
    
    Assert (pBrushContext);

    // 
    // First, determine whether or not the brush has content.
    // This check will allow all subsequent methods to assume
    // content does exist.
    // 
    
    IFC(DoesContainContent(&fHasContent));
    if (!fHasContent)
    {
        fBrushIsEmpty = TRUE;
        goto Cleanup;
    }

    //
    // Obtain the current value of all TileBrush properties
    //

    IFC(GetTilePropertyCurrentValues(
        OUT &opacity,
        OUT &pTransform,
        OUT &pRelativeTransform,
        OUT &viewportUnits,
        OUT &viewboxUnits,
        OUT &viewport,
        OUT &viewbox,
        OUT &stretch,
        OUT &tileMode,
        OUT &alignmentX,
        OUT &alignmentY,
        OUT &rCacheInvalidationThresholdMinimumD,
        OUT &rCacheInvalidationThresholdMaximumD
        ));

    //
    // Obtain the content->viewbox scale
    //

    IFC(GetContentToViewboxScale(
        &rContentScaleX,
        &rContentScaleY
        ));       

    //
    // Obtain the content bounds, if necessary
    //
    
    if ((viewboxUnits == MilBrushMappingMode::RelativeToBoundingBox) ||
        IsCachingEnabled()
        )
    {
        //
        // Obtain the bounds the Viewbox is relative to
        //
        
        IFC(GetContentBounds(
            pBrushContext,
            &rcContentBoundsF
            ));

        if (rcContentBoundsF.IsEmpty())
        {
            fBrushIsEmpty = TRUE;
            goto Cleanup;
        }

        MilPointAndSizeDFromMilRectF(OUT rcContentBoundsD, rcContentBoundsF);
    }        

    //
    // Calculate the Content->Viewbox->Viewport->World mapping
    //
    
    CTileBrushUtils::CalculateTileBrushMapping(
        pTransform,
        pRelativeTransform,
        stretch,
        alignmentX,
        alignmentY,
        viewportUnits,
        viewboxUnits,
        &pBrushContext->rcWorldBrushSizingBounds,
        &rcContentBoundsD,
        rContentScaleX,
        rContentScaleY,
        IN OUT &viewport,
        IN OUT &viewbox,        
        OUT &matContentToViewport,
        OUT &matViewportToWorld,
        OUT &matContentToWorld,
        OUT &fBrushIsEmpty
        );          

    //
    // Early-out if the brush was determined empty during the matrix mapping calculation
    //
    if (fBrushIsEmpty)
    {
        goto Cleanup;
    }

    //
    // Obtain the IWGXBitmapSource representation of the base tile
    //

    BOOL fNeedsIntermediateSurfaceRealization;

    IFC(NeedsIntermediateSurfaceRealization(
        pBrushContext,
        &matContentToViewport,
        &matViewportToWorld,
        &viewport,
        tileMode,
        OUT &fNeedsIntermediateSurfaceRealization,
        OUT &fBrushIsEmpty
        ));

    if (fBrushIsEmpty)
    {
        goto Cleanup;
    }
       
    if (fNeedsIntermediateSurfaceRealization)
    {         
        BrushCachingParameters brushCachingParams; 

        //
        // Setup the cached intermediate surface & reuse parameters, if
        // caching is enabled
        //
        if (IsCachingEnabled())
        {            
            // Lazily create m_pIntermediateCache first time caching is enabled,
            // before it is copied to brushCachingParams in SetupCachingParameters
            if (NULL == m_pIntermediateCache)
            {
                IFC(CBrushIntermediateCache::Create(&m_pIntermediateCache));
            }            
            
            // Initialize brushCachingParams
            SetupCachingParameters(
                IN matContentToViewport,
                IN rcContentBoundsF,
                IN rCacheInvalidationThresholdMinimumD,
                IN rCacheInvalidationThresholdMaximumD,
                OUT brushCachingParams
                );            

            // Invalidate the cache if any brush properties or content
            // has changed
            if (IsDirty())
            {
                m_pIntermediateCache->InvalidateCache();
            }            
        }

        EventWriteWClientPotentialIRTResource(static_cast<CMilSlaveResource*>(this));

        //
        // Obtain the intermediate base tile.  If caching is enabled, the cached 
        // intermediate will be returned if it is re-usable.
        //
        IFC(CTileBrushUtils::GetIntermediateBaseTile(
            IN this,
            IN pBrushContext,
            IN &matContentToViewport,
            IN &matViewportToWorld,
            IN &viewport,
            IN IsCachingEnabled() ? &brushCachingParams : NULL,            
            IN tileMode,
            IN OUT &pBaseTile,
            OUT &matBaseTileToXSpace,
            OUT &fBrushIsEmpty,
            OUT &fUseSourceClip,
            OUT &fSourceClipIsEntireSource,
            OUT &sourceClipXSpace,
            OUT &xSpaceDefinition
            ));

        //
        // Early-out if the brush was determined empty during the base tile creation
        //
        if (fBrushIsEmpty)
        {
            goto Cleanup;
        }        

        if (IsCachingEnabled())
        {
            // The intermediate cache code-path was active, and succeeded, during this 
            // call to GetBrushRealizationInternal
            fInvalidateBrushCache = false;            
        }
    }
    else
    {        
        IFC(GetBaseTile(
            &pBrushContext->matWorldToSampleSpace,
            &matContentToViewport,
            &matViewportToWorld,
            &viewport,
            tileMode,
            OUT &pBaseTile,
            OUT &matBaseTileToXSpace,
            OUT &fBrushIsEmpty,
            OUT &fUseSourceClip,
            OUT &fSourceClipIsEntireSource,
            OUT &sourceClipXSpace,
            OUT &xSpaceDefinition
            ));        

        //
        // Early-out if the brush was determined empty during the base tile creation
        //
        if (fBrushIsEmpty)
        {
            goto Cleanup;
        }        
    }

    //
    //
    // Retrieve realization & set all BitmapBrush properties, now that we've 
    // obtained all of the data neccessary to create a bitmap brush.
    // 
    //

    // Set the brush properties
    
    IFC(m_realizedBitmapBrush.SetBitmap(pBaseTile));
    
    m_realizedBitmapBrush.SetBitmapToXSpaceTransform(
        &matBaseTileToXSpace,
        xSpaceDefinition
        DBG_COMMA_PARAM(&pBrushContext->matWorldToSampleSpace)
        );

    IFC(m_realizedBitmapBrush.SetWrapMode(
        MILBitmapWrapModeFromTileMode(tileMode), 
        NULL
        ));

    IFC(m_realizedBitmapBrush.SetSourceClipXSpace(
        fUseSourceClip,
        fSourceClipIsEntireSource,
        &sourceClipXSpace
        DBG_COMMA_PARAM(xSpaceDefinition)
        DBG_COMMA_PARAM(&pBrushContext->matWorldToSampleSpace)
        ));

    m_realizedBitmapBrush.SetOpacity(opacity);

    *ppBrushRealizationNoRef = &m_realizedBitmapBrush;

Cleanup:

    if (fBrushIsEmpty)
    {
        *ppBrushRealizationNoRef = NULL;
    }


    if (fInvalidateBrushCache && m_pIntermediateCache)
    {
        // When this function is called and intermediate caching isnt used (e.g., because caching is
        // disabled, the brush was empty, or any other reason why NeedsIntermediateSurfaceRealization
        // returns false), any intermediates that are currently cache become invalid -- they
        // no longer represent the current brush state.  This is especially important because
        // calling this function always clears the Dirty bit.
        m_pIntermediateCache->InvalidateCache();
    }

    // Release our reference after it has been set on the CMILBrushBitmap or after
    // we've failed
    ReleaseInterfaceNoNULL(pBaseTile);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilTileBrushDuce::SetupCachingParameters
//
//  Synopsis:
//      Initializes the values of the BrushCachingParameters structure that will
//      be passed to GetIntermediateBaseTile.
//
//------------------------------------------------------------------------------
VOID
CMilTileBrushDuce::SetupCachingParameters(
        // Whether or not we will attempt to re-use the cached realization
    __in_ecount(1) CMILMatrix &matContentToViewport,
        // Context->viewport transform used on rcContentBoundsF
    __in_ecount(1) CMilRectF &rcContentBoundsF,
        // Content bounds in content space
    __in_ecount(1) double &rCacheInvalidationThresholdMinimumD,
        // Non-validated low-range of the user-specified cache invalidation threshold 
    __in_ecount(1) double &rCacheInvalidationThresholdMaximumD,
        // Non-validated high-range of the user-specified cache invalidation threshold     
    __out_ecount(1) BrushCachingParameters &brushCachingParams
        // Output caching parameters
    )
{    
    //
    // Set content bounds, which is needed whether or not we are going to
    // try and re-use a cached realization
    //

    Assert(m_pIntermediateCache);
    brushCachingParams.pIntermediateCache = m_pIntermediateCache;

    matContentToViewport.Transform2DBounds(
        rcContentBoundsF,
        OUT brushCachingParams.rcCurrentContentBounds_ViewportSpace
        );     
    
    //
    // Setup the min & max caching thresholds
    //

    float rMinThreshold = static_cast<float>(rCacheInvalidationThresholdMinimumD);
    float rMaxThreshold = static_cast<float>(rCacheInvalidationThresholdMaximumD); 

    // Clamp rMinThreshold to the range 0.0 <= rMinThreshold <= 1.0.  
    // NaN's are OK.
    if (rMinThreshold < 0.0)
        rMinThreshold = 0.0;
    if (rMinThreshold > 1.0)
        rMinThreshold = 1.0;

    // Clamp rMaxThreshold to be >= 1.0.  NaN's are OK.
    if (rMaxThreshold < 1.0)
        rMaxThreshold = 1.0;
  
    brushCachingParams.rCacheInvalidationThresholdMinimum = rMinThreshold;                
    brushCachingParams.rCacheInvalidationThresholdMaximum = rMaxThreshold;    
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilTileBrushDuce::FreeRealizationResources, CMilBrushDuce
//
//  Synopsis:
//      Frees realized resource that shouldn't last longer than a single
//      primitive.  That is currently true for intermediate RTs, which this
//      object may retain.  It is up to derivatives to not call this when
//      retained resource in m_pCurrentRealization is not an intermediate render
//      target -- see CMilImageBrushDuce::FreeRealizationResources for example.
//
//------------------------------------------------------------------------------
void
CMilTileBrushDuce::FreeRealizationResources()
{
    //
    // Note that when the caching feature is on, a reference to the bitmap
    // texture may still be present in the CBrushIntermediateCache.
    //
    m_realizedBitmapBrush.SetBitmap(NULL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilTileBrushDuce::GetContentToViewboxScale
//
//  Synopsis:
//      Default GetContentToViewboxScale implementation that sets *pfScaleValid
//      to false.
//
//------------------------------------------------------------------------------
HRESULT 
CMilTileBrushDuce::GetContentToViewboxScale( 
    __out_ecount(1) REAL *pScaleX,
        // X Scale factor to apply to the content
    __out_ecount(1) REAL *pScaleY        
        // Y Scale factor to apply to the content      
    ) const 
{
    *pScaleX = 1.0f;
    *pScaleY = 1.0f;
    RRETURN(S_OK); 
}  

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilTileBrushDuce::GetTilePropertyCurrentValues
//
//  Synopsis:
//      Obtains the current value of the brush's TileBrush properties by
//      querying the subclass for their properties base values & resources, and
//      then obtaining their current value.
//
//------------------------------------------------------------------------------
HRESULT 
CMilTileBrushDuce::GetTilePropertyCurrentValues(
    __out_ecount(1) FLOAT *pOpacity,
        // Current value of the TileBrush's Opacity property
    __out_ecount(1) const CMILMatrix **ppTransform,
        // Current value of the TileBrush's Transform property
    __out_ecount(1) const CMILMatrix **ppRelativeTransform,
        // Current value of the TileBrush's RelativeTransform property 
    __out_ecount(1) MilBrushMappingMode::Enum *pViewportUnits,
        // Current value of the TileBrush's ViewportUnits property
    __out_ecount(1) MilBrushMappingMode::Enum *pViewboxUnits,
        // Current value of the TileBrush's ViewboxUnits property
    __out_ecount(1) MilPointAndSizeD *pViewport,
        // Current value of the TileBrush's Viewport property  
    __out_ecount(1) MilPointAndSizeD *pViewbox,
        // Current value of the TileBrush's Viewbox property        
    __out_ecount(1) MilStretch::Enum *pStretch,
        // Current value of the TileBrush's Stretch property
    __out_ecount(1) MilTileMode::Enum *pTileMode,
        // Current value of the TileBrush's MilTileMode::Enum property        
    __out_ecount(1) MilHorizontalAlignment::Enum *pAlignmentX,
        // Current value of the TileBrush's AlignmentX property
    __out_ecount(1) MilVerticalAlignment::Enum *pAlignmentY,
        // Current value of the TileBrush's AlignmentY property
    __out_ecount(1) double *pCacheInvalidationThresholdMinimum,
        // Low end of the CacheInvalidationThreshold range
    __out_ecount(1) double *pCacheInvalidationThresholdMaximum    
        // High end of the CacheInvalidationThreshold range            
    ) const
{
    HRESULT hr = S_OK;

    DOUBLE opacity;
    CMilSlaveDouble *pOpacityAnimations;
    
    CMilTransformDuce *pTransformResource;
    CMilTransformDuce *pRelativeTransformResource;
    
    MilPointAndSizeD viewport;
    CMilSlaveRect *pViewportAnimations;

    MilPointAndSizeD viewbox;
    CMilSlaveRect *pViewboxAnimations;
    
    //
    // Obtain the constant values & mutable resources of the properties we 
    // need the current value for.  
    //
    // If we are retrieving a property that isn't ever backed by a resource
    // (e.g., Stretch), avoid using a local variable and just pass the parameter 
    // directly.
    //

    //
    // Future Consideration:  - It would be ideal if we didn't have to call
    // GetTilePropertyResource here, and instead could access the data directly.  
    //
    // But for this to occur, the data held onto by the subclasses would have to
    // derive from a base TileBrush data type.
    //
    // For example, to eliminate this GetTilePropertyResources call, we could 
    // modify codegen to have CMilImageBrushDuce_Data inherit it's tile properties from
    // CMilTileBrushDuce_Data, instead of declaring them in CMilImageBrushDuce_Data    
    // (and every other tilebrush subclass) directly.
    //
        
    IFC(GetTilePropertyResources(
        &opacity,
        &pOpacityAnimations,
        &pTransformResource,
        &pRelativeTransformResource,
        pViewportUnits,                     // Delegate setting of out-param
        pViewboxUnits,                      // Delegate setting of out-param
        &viewport,
        &pViewportAnimations,
        &viewbox,
        &pViewboxAnimations,
        pStretch,                           // Delegate setting of out-param
        pTileMode,                          // Delegate setting of out-param
        pAlignmentX,                        // Delegate setting of out-param
        pAlignmentY,                        // Delegate setting of out-param
        pCacheInvalidationThresholdMinimum, // Delegate setting of out-param
        pCacheInvalidationThresholdMaximum  // Delegate setting of out-param
        ));

    //
    // Obtain current value of properties that can be backed be resources
    //

    IFC(GetOpacity(
        opacity,
        pOpacityAnimations,
        pOpacity
        ));

    // Get current Transform value
    IFC(GetMatrixCurrentValue(
        pTransformResource,
        ppTransform // Delegate setting of out-param
        ));

    // Get current RelativeTransform value
    IFC(GetMatrixCurrentValue(
        pRelativeTransformResource,
        ppRelativeTransform // Delegate setting of out-param
        ));    

    // Get current Viewport value
    IFC(GetRectCurrentValue(
        &viewport,
        pViewportAnimations,
        reinterpret_cast<MilPointAndSizeD*>(pViewport) // Delegate setting of out-param
        ));

    // Get current Viewbox value
    IFC(GetRectCurrentValue(
        &viewbox,
        pViewboxAnimations,
        reinterpret_cast<MilPointAndSizeD*>(pViewbox)  // Delegate setting of out-param
        ));
    
Cleanup:        
    RRETURN(hr);
}


