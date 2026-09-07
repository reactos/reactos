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
//      The ImageBrush CSlaveResource is responsible for maintaining the current
//      base values & animation resources for all ImageBrush properties.  This
//      class processes updates to those properties, and updates a realization
//      based on their current value during GetBrushRealizationInternal.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"

MtDefine(ImageBrushResource, MILRender, "ImageBrush Resource");

MtDefine(CMilImageBrushDuce, MILRender, "CMilImageBrushDuce");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::~CMilImageBrushDuce
//
//  Synopsis:
//      Class destructor.
//
//------------------------------------------------------------------------------
CMilImageBrushDuce::~CMilImageBrushDuce()
{
    UnRegisterNotifiers();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::GetBrushRealizationInternal
//
//  Synopsis:
//      Stores the brush sizing bounds after
//      CMilTileBrushDuce::GetBrushRealizationInternal returns, so that
//      non-intermediate realizations can be reused.
//
//------------------------------------------------------------------------------
HRESULT
CMilImageBrushDuce::GetBrushRealizationInternal(
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
    )
{
    HRESULT hr = S_OK;

    // Delegate realization update to the base class
    IFC(CMilTileBrushDuce::GetBrushRealizationInternal(
        pBrushContext,
        ppBrushRealizationNoRef
        ));
    
Cleanup:

    if (SUCCEEDED(hr) && !m_fRealizationIsIntermediate)
    {        
        // Because the brush realization is only dependent on the brush sizing bounds,
        // and not other context state such as the world transform or clip, we can
        // cache it to avoid recreating the realization when it doesn't change.
        m_cachedBrushSizingBounds = pBrushContext->rcWorldBrushSizingBounds;
    }
#if DBG    
    else
    {
        // Set to empty so we don't check against an old bounding box
        // in a future call.
        m_cachedBrushSizingBounds = MilEmptyPointAndSizeD;
    }
#endif    

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::FreeRealizationResources, CMilBrushDuce
//
//  Synopsis:
//      Frees realized resource that shouldn't last longer than a single
//      primitive.  That is currently true for intermediate RTs, which this
//      object may retain.
//
//------------------------------------------------------------------------------
void
CMilImageBrushDuce::FreeRealizationResources()
{
    if (m_fRealizationIsIntermediate)
    {
        // Only release the intermediate RT if we created one for the image brush.
        CMilTileBrushDuce::FreeRealizationResources();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::DoesContainContent, ITileBrushData
//
//  Synopsis:
//      Returns whether or not the bitmap resource has non-NULL content
//
//  Notes:
//      If no content exists, then methods that require content such as
//      GetContentBounds and GetBaseTile won't be called, and can assume that
//      they aren't called.
//
//------------------------------------------------------------------------------
HRESULT
CMilImageBrushDuce::DoesContainContent(
    __out_ecount(1) BOOL *pfHasContent
    // Whether or not the TileBrush has content
    ) const
{
    Assert(pfHasContent);

    *pfHasContent = FALSE;

    if (m_data.m_pImageSource && m_data.m_pImageSource->HasContent())
    {
        *pfHasContent = TRUE;
    }

    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::GetTilePropertyResources
//
//  Synopsis:
//      Obtains the base values & resources of this brush's tile properties.
//
//------------------------------------------------------------------------------
HRESULT 
CMilImageBrushDuce::GetTilePropertyResources(
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

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::GetContentToViewboxScale, ITileBrushData
//
//  Synopsis:
//      Obtains the Content->Viewbox scale factors, which are derived from the
//      source image's DPI.
//
//------------------------------------------------------------------------------
HRESULT 
CMilImageBrushDuce::GetContentToViewboxScale( 
    __out_ecount(1) REAL *pScaleX,
        // X Scale factor to apply to the content
    __out_ecount(1) REAL *pScaleY        
        // Y Scale factor to apply to the content
    ) const
{
    HRESULT hr = S_OK;
    
    // We should have already checked for content
    #if DBG
    Assert(DbgHasContent());
    #endif
    Assert(pScaleX && pScaleY);

    REAL rDpiX, rDpiY;
    double dblDpiX = 0.0;
    double dblDpiY = 0.0;

    //
    // Future Consideration:  - Potential performance optimizations
    //
    // Consider caching the scale factors & bitmap source amoungst multiple calls,
    // since the data doesn't change between the call from GetContentToViewboxScale
    // to the call from GetContentBounds.
    //
    // Currently, it isn't clear whether the extra memory cost to cache this data is
    // worth the benefit in CPU cycles.
    //    

    IFC(m_data.m_pImageSource->GetResolution(&dblDpiX, &dblDpiY));

    //
    //  Possible loss of precision
    //
    rDpiX = static_cast<FLOAT>(dblDpiX);
    rDpiY = static_cast<FLOAT>(dblDpiY);

    // If the X DPI is less than or near 0 we do no X DPI scaling.
    if (IsCloseToDivideByZeroReal(96.0f, rDpiX) ||
        rDpiX < 0.0f)
    {
        rDpiX = 96.0f;
    }

    // If the Y DPI is less than or near 0 we do no Y DPI scaling.
    if (IsCloseToDivideByZeroReal(96.0f, rDpiY) ||
        (rDpiY < 0.0f))
    {
        rDpiY = 96.0f;
    }

    *pScaleX = 96.0f / rDpiX;
    *pScaleY = 96.0f / rDpiY; 

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::GetContentBounds, ITileBrushData
//
//  Synopsis:
//      Obtains the bounds of the source image, in device-independent content
//      units.
//
//------------------------------------------------------------------------------
HRESULT 
CMilImageBrushDuce::GetContentBounds(
    __in_ecount(1) const BrushContext *pBrushContext, 
        // Context the brush is being realized for        
    __out_ecount(1) CMilRectF *pContentBounds
        // Output content bounds
    ) 
{
    HRESULT hr = S_OK;

    REAL scaleX;
    REAL scaleY;  

    Assert(pContentBounds);
    // We should have already checked for content
    #if DBG
    Assert(DbgHasContent());
    #endif

    IFC(m_data.m_pImageSource->GetBounds(
        pBrushContext->pContentBounder, 
        pContentBounds
        ));

    IFC(GetContentToViewboxScale(&scaleX, &scaleY));

    pContentBounds->right  *= scaleX;
    pContentBounds->bottom *= scaleY;

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::NeedsIntermediateSurfaceRealization, ITileBrushData
//
//  Synopsis:
//      Determines whether or not the brush should be realized into an
//      intermediate surface.  Intermediate surfaces are needed when the source
//      image needs to be tiled, while also being clipped to the viewport, or
//      padded with transparent pixels to fill the viewport.
//
//------------------------------------------------------------------------------
HRESULT 
CMilImageBrushDuce::NeedsIntermediateSurfaceRealization(
    __in_ecount(1) const BrushContext *pBrushContext,
        // Context the brush is being realized for        
    __in_ecount(1) const CMILMatrix *pContentToViewport,
        // Content->Viewport portion of the Content->World transform
    __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        // Viewport->World portion of the Content->World transform;
    __in_ecount(1) const MilPointAndSizeD *pViewport,
        // User-specified base tile in absolute units
    __in MilTileMode::Enum tileMode,
        // User-specified tiling mode for this brush
    __out_ecount(1) BOOL *pfNeedsIntermediateSurfaceRealization,
        // Whether or not this brush should be realized using an intermediate surface
    __out_ecount(1) BOOL *pfBrushIsEmpty
        // TRUE if we find out during the call to this function that the brush is empty
    )
{
    HRESULT hr = S_OK;

    IWGXBitmapSource *pImageSource = NULL;    

   *pfNeedsIntermediateSurfaceRealization = FALSE;  
   *pfBrushIsEmpty = FALSE;

    // We should have already checked for content
    #if DBG
    Assert(DbgHasContent());
    #endif

    //
    // Obtain the bounds of the source image & map those bounds to the Viewport
    // 
    
    IFC(GetBitmapCurrentValue(
        m_data.m_pImageSource, 
        &pImageSource  // Delegate setting of out-param
        ));

    if (!pImageSource)
    {
        if (m_data.m_pImageSource->CanDrawToIntermediate())
        {
            // Since we have already checked for content, if m_pImageSource is
            // NULL that means we couldn't get a bitmap out of m_pImageSource.
            // In other words, m_pImageSource could be something like
            // DrawingImage that needs an intermediate surface
            *pfNeedsIntermediateSurfaceRealization = TRUE;
            goto Cleanup;
        }
        else
        {
            // If we can't draw to an intermediate, then we are trying to draw
            // something like a cached visual image. The reason it returned
            // NULL for the image source is because there is nothing to draw.
            *pfBrushIsEmpty = TRUE;
            goto Cleanup;
        }
    }

    // Intermediate surfaces are not needed when there is no tiling, because
    // we can instead clip the single tile to the Viewport using a source clip.
    if (CMilTileBrushDuce::IsTiling(tileMode))
    {
        // Determine if the image bounds mapped to the Viewport are approximately equal 
        // to the Viewport.
        // 
        // When we are tiling, using a single source clip would disallow tiles outside 
        // of the clip from being rendered.  To prevent this, we create a temporary
        // copy of the clipped or padded image using an intermediate surface, which is
        // then tiled instead of the source image.  
        //
        // Because temporary surfaces are expensive and can reduce rendering quality,
        // we try to avoid creating them when they aren't needed.  This happens when the 
        // image bounds maps to the same location as the Viewport, such that no clipping 
        // or padding of the image is required.
        //       

        CMilRectF rcContentBoundsViewportSpace;
        CMilRectF rcViewport;
        BOOL fSourceCilpApproximatesContentBounds;

        //
        // Convert the Viewport into a LTRB rectangle
        //
        
        MilRectFFromMilPointAndSizeD(OUT rcViewport, *pViewport);

        IFC(SourceClipApproximatesContentBounds(
            &rcViewport,
            pImageSource,
            &pBrushContext->matWorldToSampleSpace,
            pContentToViewport,
            pmatViewportToWorld,
            OUT &rcContentBoundsViewportSpace,
            OUT &fSourceCilpApproximatesContentBounds
            ));

        // If the transformed rectangles aren't close, then we need an intermediate surface
        *pfNeedsIntermediateSurfaceRealization = !fSourceCilpApproximatesContentBounds;
    }
    
Cleanup:

    ReleaseInterfaceNoNULL(pImageSource);
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::SourceClipApproximatesContentBounds
//
//  Synopsis:
//      Determines whether the source clip is close enough to the content bounds
//      for us to pretend that they are the same.
//
//      If floating-point math had no error, we could compare these rect's
//      without further transformation, using exact equality.  But because error
//      does exist in the both the image bounds computation & Viewport
//      computation, we have to compare these rectangles using an approximate
//      comparison.
//
//      This method is used for two things. It is used to avoid intermediate
//      surfaces in the tiling case, and it is used to avoid software text
//      rendering for the non-tiling case.
//
//      The premise behind this appromixate comparison is that we should avoid
//      our slow code codepaths unless this avoidance would cause the image to
//      incorrectly be clipped or padded.
//

HRESULT CMilImageBrushDuce::SourceClipApproximatesContentBounds(
    __in_ecount(1) const CMilRectF *prcViewport,
        // User-specified base tile coordinates
    __in_ecount(1) IWGXBitmapSource *pImageSource,
        // Image containing content bounds
    __in_ecount(1) const CMILMatrix *pmatWorldToSampleSpace,
        // World->SampleSpace transform (from BrushContext)
    __in_ecount(1) const CMILMatrix *pContentToViewport,
        // Content->Viewport portion of the Content->World transform
    __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        // Viewport->World portion of the Content->World transform;
    __out_ecount(1) CMilRectF *prcContentBoundsViewportSpace,
        // content bounds in viewport space (derived from Image source)
    __out_ecount(1) BOOL *pfSourceClipApproximatesContentBounds
        // The main out param of this method
    ) const
{
    HRESULT hr = S_OK;

    //
    // Compute matrix to transform viewport, and the imageBounds mapped to the viewport,
    // into sample  space.  This allows us to determine the actual pixel
    // differences between the viewport & image bounds.
    //
    // From the coordinate space the Viewport is defined in, the user-specified brush 
    // transform is applied to get into 'World' space, and then the World->SampleSpace 
    // transform is applied to get into sample space.
    //

    CMILMatrix matViewportToSampleSpace = *pmatViewportToWorld;
    matViewportToSampleSpace.Multiply(*pmatWorldToSampleSpace);          

    // 
    // Obtain source rectangle in Viewport coordinate space
    //

    IFC(GetBitmapSourceBounds(
        pImageSource,
        prcContentBoundsViewportSpace // content bounds will be in content space now
        ));

    Assert(pContentToViewport->Is2DAxisAlignedPreservingOrNaN());
    pContentToViewport->Transform2DBounds(
        *prcContentBoundsViewportSpace,
        OUT *prcContentBoundsViewportSpace // now the bounds are in viewport space
        );

    //
    // We equate content bounds within INSIGNIFICANT_PIXEL_COVERAGE_SRGB / 2.0 of the
    // Viewport to be equal to the Viewport.
    //
    // Ignoring differences larger than this will begin to cause the base tile to lose
    // clipping or padding.  But when tiling, this error can accumulate, especially if the tiles
    // are small.  It was shown during testing that this difference wasn't visible even after 
    // 500 tiles which contained fully-saturated colors.  
    //
    // Thus, this constant is a reasonable tradeoff between an overly strict tolerance, which
    // would cause us to unnecessarily pay the performance cost & quality degradation intermediates
    // cause, vs. avoiding intermediates and having tiles increasing displaced because
    // of the accumulated error.
    //
    // Allowing the accumulated displacement is better than eliminating the error by slightly 
    // stretching the image to be exactly equal to the Viewport, because doing so would alter 
    // the rendering behavior of every tile.
    //
    const float renderingTolerance = INSIGNIFICANT_PIXEL_COVERAGE_SRGB / 2.0f;        

    *pfSourceClipApproximatesContentBounds = AreTransformedRectanglesClose(
        prcContentBoundsViewportSpace,
        prcViewport,
        &matViewportToSampleSpace,
        renderingTolerance
        );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::GetBaseTile
//
//  Synopsis:
//      Obtains the base image to be tiled.
//
//------------------------------------------------------------------------------
HRESULT 
CMilImageBrushDuce::GetBaseTile(
    __in_ecount(1) const CMILMatrix *pmatWorldToSampleSpace,
        // World->SampleSpace transform (from BrushContext)
    __in_ecount(1) const CMILMatrix *pContentToViewport,
        // Content->Viewport portion of the Content->World transform
    __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        // Viewport->World portion of the Content->World transform.  This
        // transform, along with the World->Device transform, are used
        // to determine the size of the Viewport in device coordinates.
    __in_ecount(1) const MilPointAndSizeD *pViewport,
        // User-specified base tile coordinates
    __in MilTileMode::Enum tileMode,
        // Wrapping mode used to create the intermediate render target
    __out_ecount(1) IWGXBitmapSource **ppBaseTile,
        // Rasterized base image to be tiled
    __out_ecount(1) CMILMatrix *pmatBaseTileToXSpace,
        // BaseTile(Content)->XSpace transform (XSpace defined below)
    __out_ecount(1) BOOL *pfTileIsEmpty,
        // Whether or not ppBaseTile renders nothing, and should be ignored
    __out_ecount(1) BOOL *pfUseSourceClip,
        // Whether or not the source clip output parameters are valid, and should be 
        // applied.
    __out_ecount(1) BOOL *pfSourceClipIsEntireSource,
        // Whether or not the source clip is equal to the entire source content
    __out_ecount(1) CParallelogram *pSourceClipXSpace,
        // Clip parallelogram to apply when rendering the brush.  This is
        // used to implement MilTileMode::None to prevent content from rendering
        // outside of the viewport. Parallelogram is in space defined below
    __out_ecount(1) XSpaceDefinition *pXSpaceDefinition
        // Space of source clip and pmatBaseTileToXSpace transform
    ) 
{
    HRESULT hr = S_OK;

    Assert(ppBaseTile && pfTileIsEmpty);

    // Set initial value of out-params
    *ppBaseTile = NULL;
    *pfTileIsEmpty = TRUE;
    *pfUseSourceClip = FALSE;    

    //
    // Set the base tile & empty flag
    //

    // The current value of the bitmap is the image that should be tiled
    IFC(GetBitmapCurrentValue(
        m_data.m_pImageSource, 
        ppBaseTile  // Delegate setting of out-param
        ));

    // This method shoudn't be called if the brush doesn't have any content
    Assert(*ppBaseTile);     
    *pfTileIsEmpty = FALSE;

    //
    // Image brushes always use WorldSpace so that we don't need to re-realize
    // them when the World->SampleSpace transform changes
    //
    *pXSpaceDefinition = XSpaceIsWorldSpace;

    //
    // Set the source clip & transform for MilTileMode::None
    //
    // The source clip is needed to clip the image to the Viewport bounds when
    // we aren't tiling, and in conjunction with extend texture wrapping, to avoid 
    // introducing artifacts caused by using border-transparent texture wrapping.
    //
   
    if (MilTileMode::None == tileMode)
    {
        // Obtain the source clip
        IFC(CalculateSourceClip(
            *ppBaseTile,
            pmatWorldToSampleSpace,
            pContentToViewport,
            pmatViewportToWorld,
            pViewport,
            OUT pSourceClipXSpace, // pSourceClipWorldSpace
            OUT pfSourceClipIsEntireSource
            ));
        
        *pfUseSourceClip = TRUE;
    }
    
    //
    // Set the base tile to sample space transform
    //
    pmatBaseTileToXSpace->SetToMultiplyResult(
        *pContentToViewport,
        *pmatViewportToWorld
        );


    // We returned the source image instead of using an intermediate
    // (Unless the image is "dynamic", in which case it is internally
    // using an intermediate and should be freed at the end of rendering).
    m_fRealizationIsIntermediate = m_data.m_pImageSource ? m_data.m_pImageSource->IsDynamicBitmap() : FALSE;

Cleanup:

    // Validate post-conditions.
    //
    // The base tile should be non-NULL and non-empty, unless a failure occured
    Assert(*ppBaseTile || FAILED(hr));
    Assert(!(*pfTileIsEmpty) || FAILED(hr));
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::CalculateSourceClip
//
//  Synopsis:
//      Calculates the source clip for this ImageBrush.  We need to clip any
//      paths filled with this ImageBrush to both the image bounds and the
//      user-specified Viewport.
//
//      This is implemented as an intersection in the Viewport coordinate space
//      because the mapping from Content->Viewbox->Viewport is axis-aligned
//      preserving.  This means that when we map the image(content) bounds into
//      the Viewport coordinate space, it won't be rotated or skewed, allowing
//      us to perform a simple rectangle intersection instead of a more
//      expensive path intersection.
//
//------------------------------------------------------------------------------
HRESULT
CMilImageBrushDuce::CalculateSourceClip(
    __in_ecount(1) IWGXBitmapSource *pImageSource,
        // Image whose bounds should be clipped to
    __in_ecount(1) const CMILMatrix *pmatWorldToSampleSpace,
        // World->SampleSpace transform (from BrushContext)
    __in_ecount(1) const CMILMatrix *pContentToViewport,
        // Content->Viewport portion of the Content->World transform
    __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        // Viewport->World portion of the Content->World transform
    __in_ecount(1) const MilPointAndSizeD* pViewport,
        // User-specified viewport in absolute units
    __out_ecount(1) CParallelogram *pSourceClipWorldSpace,
        // Output source clip in the Viewport's coordinate space
    __out_ecount(1) BOOL *pfSourceClipIsEntireSource
        // Whether or not the source clip is the entire source
    ) const
{
    HRESULT hr = S_OK;
    
    CMilRectF rcContentBoundsViewportSpace;
    CMilRectF rcCombinedClipViewportSpace;

    // Assert pre-conditions

    Assert(pImageSource && 
           pViewport &&
           pContentToViewport &&
           pSourceClipWorldSpace);


    // Convert viewport to _RB from _WH
    MilRectFFromMilPointAndSizeD(
        OUT rcCombinedClipViewportSpace, // currently just the viewport
        *pViewport
        );

    //
    // The "pfSourceClipIsEntireSource" out param is used to determine whether
    // we can implement source clipping by adding a transparent border to the
    // image for special cases like HW text rendering. This algorithm is only
    // valid if the source clip is approximately equal to the content bounds.
    //
    IFC(SourceClipApproximatesContentBounds(
        &rcCombinedClipViewportSpace, // currently just the viewport
        pImageSource,
        pmatWorldToSampleSpace,
        pContentToViewport,
        pmatViewportToWorld,
        OUT &rcContentBoundsViewportSpace,
        OUT pfSourceClipIsEntireSource
        ));

    //
    // Intersect Viewport with the content bounds
    //

    // Add the intersection of the Viewbox & content
    rcCombinedClipViewportSpace.Intersect(rcContentBoundsViewportSpace);

    //
    // Convert the clip to a parallelogram before transforming by a
    // non-axis-aligned preserving matrix
    //

    pSourceClipWorldSpace->Set(rcCombinedClipViewportSpace);
    pSourceClipWorldSpace->Transform(pmatViewportToWorld);

Cleanup:
    RRETURN(hr);   
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilImageBrushDuce::DrawIntoBaseTile, ITileBrushData
//
//  Synopsis:
//      Draws this brush's content into an already-allocated DrawingContext. 
//      This method is used to populate the intermediate surface realization.
//
//------------------------------------------------------------------------------
HRESULT 
CMilImageBrushDuce::DrawIntoBaseTile(
    __in_ecount(1) const BrushContext *pBrushContext,
        // Context the brush is being realized for        
    __in_ecount(1) CMilRectF * prcSurfaceBounds,
        // Bounds of the intermediate suface
    __inout_ecount(1) CDrawingContext *pDrawingContext
        // Drawing context to draw content into
    ) 
{
    HRESULT hr = S_OK;

    // Render the image source into an intermediate surface which is sized to the Viewport
    pDrawingContext->ApplyRenderState();

    // We should have checked for content already
    #if DBG
    Assert(DbgHasContent());
    #endif

    IFC(m_data.m_pImageSource->Draw(
        pDrawingContext,
        MILBitmapWrapModeFromTileMode(m_data.m_TileMode)
        ));
    
    // Remember that we rendered the image source into an intermediate
    m_fRealizationIsIntermediate = TRUE;
    
Cleanup:
    
    RRETURN(hr);
}




