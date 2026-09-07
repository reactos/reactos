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
//      Implementation of tile brush methods used to create intermediate
//      representations from user-defined state.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
using namespace dxlayer;

//+-----------------------------------------------------------------------------
//
//  Member:
//      CTileBrushUtils::CalculateTileBrushMapping
//
//  Synopsis:
//      Given the current TileBrush mapping properties, this method computes a
//      matrix mapping from the source content to world coordinates (the same
//      coordinate space shapes are defined in).
//
//  Notes:
//      This method is used both during tilebrush realization on the render
//      thread (i.e., unmanaged) & the UI thread during a realization pass
//      (i.e., in managed code).
//
//------------------------------------------------------------------------------
VOID
CTileBrushUtils::CalculateTileBrushMapping( 
    __in_ecount_opt(1) const CMILMatrix *pTransform,
        // Transform that is applied to the Viewport
    __in_ecount_opt(1) const CMILMatrix *pRelativeTransform,    
        // RelativeTransform that is applied to the Viewport
    MilStretch::Enum stretch,
        // Stretch mode to use in Viewbox->Viewport mapping
    MilHorizontalAlignment::Enum alignmentX,
        // X Alignment to use in Viewbox->Viewport mapping    
    MilVerticalAlignment::Enum alignmentY,
        // Y Alignment to use in Viewbox->Viewport mapping  
    MilBrushMappingMode::Enum viewportUnits,
        // Viewport mapping mode (relative or absolute)
    MilBrushMappingMode::Enum viewboxUnits,
        // Viewbox mapping mode (relative or absolute)
    __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        // Shape fill-bounds pViewport is relative to.  
        // Only used when viewportUnits == RelativeToBoundingBox.
    __in_ecount(1) const MilPointAndSizeD *pContentBounds,      
        // Content bounds pViewbox is relative to.
        // Only used when viewboxUnits == RelativeToBoundingBox.
    REAL rContentScaleX,
        // X content->viewbox scale used to support Image DPI
    REAL rContentScaleY,    
        // Y content->viewbox scale used to support Image DPI           
    __inout_ecount(1) MilPointAndSizeD *pViewport,
        // IN: User-specified Viewport to map Viewbox to 
        // OUT: Viewport in absolute units
    __inout_ecount(1) MilPointAndSizeD *pViewbox,
        // IN: User-specified Viewbox to map to Viewport
        // OUT: Viewbox in absolute units  
    __out_ecount_opt(1) CMILMatrix *pContentToViewport,
        // Output Content->Viewport transform
    __out_ecount_opt(1) CMILMatrix *pmatViewportToWorld,
        // Output user-specified Viewport->World transform
    __out_ecount(1) CMILMatrix *pContentToWorld,
        // Combined Content->World transform
    __out_ecount(1) BOOL *pfBrushIsEmpty
        // Whether or not this brush renders nothing because of an empty viewport/viewbox
    )
{
    CMILMatrix matContentToViewport;
    CMILMatrix matViewportToWorld;
    CMILMatrix matViewboxToViewport;

    Assert( pBrushSizingBounds &&
            pViewport &&
            pViewbox &&
            pContentToWorld &&
            pfBrushIsEmpty
            );

    // Initialize output empty boolean to false
    *pfBrushIsEmpty = FALSE;

    //
    //
    // Obtain the absolute values of the viewbox & viewport
    //
    //

    GetAbsoluteViewRectangles(
        IN     viewportUnits,
        IN     viewboxUnits,
        IN     pBrushSizingBounds,
        IN     pContentBounds,
        IN OUT pViewport,
        IN OUT pViewbox,
        OUT    pfBrushIsEmpty
        );

    // Early-out if this brush is empty due to an empty viewbox or viewport
    if (*pfBrushIsEmpty)
    {
        goto Cleanup;
    }

    //
    //
    // Calculate the Content->Viewport transform
    //
    //

    // First, initialize the transform with the Content->Viewbox 
    // scale factors.
    matContentToViewport = matrix::get_scaling(rContentScaleX, rContentScaleY, 1.0f);

    // Calculate the Viewbox->Viewport transform based on the Viewbox, Viewport,
    // Stretch, & Alignment properties
    CTileBrushUtils::CalculateViewboxToViewportMapping(
        IN  pViewport,    
        IN  pViewbox,
        IN  stretch,
        IN  alignmentX,
        IN  alignmentY,
        OUT &matViewboxToViewport
        );     
    
    // After the Content->Viewbox transform, append the Viewbox->Viewport transform
    matContentToViewport.Multiply(matViewboxToViewport);

    //
    //
    // Calculate the Viewport->World transform
    //
    // To map the brush into World coordinates (the same coordinates the shape
    // is specified in), the user-specified Viewport->World transform is applied to the 
    // Viewport. 
    //
    //

    // Obtain user-specified Viewport->World transform by combining the
    // user-specified transforms
    CBrushTypeUtils::GetBrushTransform(
        IN  pRelativeTransform,    
        IN  pTransform,
        IN  pBrushSizingBounds,
        OUT &matViewportToWorld
        );

    // Combine the Brush transform and the Content->Viewport transform to obtain
    // the current Content->World transform.
    pContentToWorld->SetToMultiplyResult(matContentToViewport, matViewportToWorld);

    //
    //
    // Set optional out-params if they are non-NULL
    //
    //

    if (pContentToViewport)
    {
        *pContentToViewport = matContentToViewport;
    }

    if (pmatViewportToWorld)
    {
        *pmatViewportToWorld = matViewportToWorld;
    }

Cleanup:
    ;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CTileBrushUtils::GetAbsoluteViewRectangles
//
//  Synopsis:
//      Obtains the absolute value of the TileBrush Viewbox & Viewport, given
//      their user-specified value, mapping mode, & bounding boxes.
//
//------------------------------------------------------------------------------
VOID
CTileBrushUtils::GetAbsoluteViewRectangles(
    MilBrushMappingMode::Enum viewportUnits,
        // Viewport mapping mode (relative or absolute)
    MilBrushMappingMode::Enum viewboxUnits,
        // Viewbox mapping mode (relative or absolute)
    __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        // World Sampling fill-bounds relative viewports are relative to
    __in_ecount(1) const MilPointAndSizeD *pContentBounds,
        // Content bounds that relative viewboxes are relative to
    __inout_ecount(1) MilPointAndSizeD *pViewport,
        // IN: User-specified Viewport to map Viewbox to 
        // OUT: Viewport in absolute units
    __inout_ecount(1) MilPointAndSizeD *pViewbox,
        // IN: User-specified Viewbox to map to Viewport
        // OUT: Viewbox in absolute units  
    __out_ecount(1) BOOL *pfIsEmpty
        // Whether or not the viewbox or viewport is empty
        )
{
    Assert( pBrushSizingBounds &&
            pContentBounds &&
            pViewport &&
            pViewbox &&
            pfIsEmpty);
            
    *pfIsEmpty = FALSE;
        
    //
    // Convert relative Viewport/Viewbox's into absolute units
    //

    // Handle relative Viewports
    if (viewportUnits == MilBrushMappingMode::RelativeToBoundingBox)
    {
        AdjustRelativeRectangle(pBrushSizingBounds, pViewport);
    }    

    // Handle relative Viewboxes
    if (viewboxUnits == MilBrushMappingMode::RelativeToBoundingBox)
    {
        // Guard that pContentBounds was initialized 
        Assert (pContentBounds->X != MilEmptyPointAndSizeD.X ||
                pContentBounds->Y != MilEmptyPointAndSizeD.Y ||
                pContentBounds->Width != MilEmptyPointAndSizeD.Width ||
                pContentBounds->Height != MilEmptyPointAndSizeD.Height);
        
        AdjustRelativeRectangle(pContentBounds, pViewbox);
    }        


    if (   IsRectEmptyOrInvalid(pViewport)
        || IsRectEmptyOrInvalid(pViewbox))
    {
        // Per spec, this brush renders nothing when either the Viewbox or
        // Viewport are empty.
        *pfIsEmpty = TRUE;   
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CTileBrushUtils::CalculateViewboxToViewportMapping
//
//  Synopsis:
//      Infers a transform from the user-specified Viewbox to the Viewport based
//      on the stretch & alignment attributes.
//
//------------------------------------------------------------------------------
VOID
CTileBrushUtils::CalculateViewboxToViewportMapping(
    __in_ecount(1) const MilPointAndSizeD *prcViewport,         // Final viewport that has been adjusted by other Tile properties
    __in_ecount(1) const MilPointAndSizeD *prcViewbox,          // Final viewbox that has been adjusted by other Tile properties
    MilStretch::Enum stretch,                     // Stretch mode
    MilHorizontalAlignment::Enum halign,          // Horizontal Alignment
    MilVerticalAlignment::Enum valign,            // Vertical Alignment
    __out_ecount(1) CMILMatrix *pmatViewboxToViewport   // Output viewbox to viewport mapping
    )
{
    double scaleX = 1.0;
    double scaleY = 1.0;

    double transX = 0;
    double transY = 0;

    double alignX = 0;
    double alignY = 0;

    //If "none", 1,1 is already correct
    if (stretch != MilStretch::None)
    {
        scaleX = prcViewport->Width / prcViewbox->Width;
        scaleY = prcViewport->Height / prcViewbox->Height;

        switch (stretch)
        {
        case MilStretch::Uniform:
            scaleX = scaleY = min(scaleX, scaleY);
            break;
        case MilStretch::UniformToFill:
            scaleX = scaleY = max(scaleX, scaleY);
            break;
        }
    }

    switch (halign)
    {
    case MilHorizontalAlignment::Left:
        transX = -prcViewbox->X;
        alignX = prcViewport->X;
        break;
    case MilHorizontalAlignment::Center:
        transX = -(prcViewbox->X + prcViewbox->Width / 2.0);
        alignX = prcViewport->X + prcViewport->Width / 2.0;
        break;
    case MilHorizontalAlignment::Right:
        transX = -(prcViewbox->X + prcViewbox->Width);
        alignX = prcViewport->X + prcViewport->Width;
        break;
    }

    switch (valign)
    {
    case MilVerticalAlignment::Top:
        transY = -prcViewbox->Y;
        alignY = prcViewport->Y;
        break;
    case MilVerticalAlignment::Center:
        transY = -(prcViewbox->Y + prcViewbox->Height / 2.0);
        alignY = prcViewport->Y + prcViewport->Height / 2.0;
        break;
    case MilVerticalAlignment::Bottom:
        transY = -(prcViewbox->Y + prcViewbox->Height);
        alignY = prcViewport->Y + prcViewport->Height;
        break;
    }

    // We want to return these operations:
    //  Matrix.CreateTranslation(transX, transY) *
    //  Matrix.CreateScaling(scaleX, scaleY) *
    //  Matrix.CreateTranslation(alignX, alignY);
    // But initializing the matrix directly is more performant:
    CMILMatrix matrix(
        static_cast<float>(scaleX),                 0.0f,                                       0.0f,   0.0f,
        0.0f,                                       static_cast<float>(scaleY),                 0.0f,   0.0f,
        0.0f,                                       0.0f,                                       1.0f,   0.0f,
        static_cast<float>(transX*scaleX + alignX), static_cast<float>(transY*scaleY + alignY), 0.0f,   1.0f
        );

    *pmatViewboxToViewport = matrix;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CTileBrushUtils::GetIntermediateBaseTile
//
//  Synopsis:
//      This method creates an intermediate surface sized to the TileBrush's
//      Viewport, instructs the brush to render into the surface, and sets the
//      source clip for MilTileMode::None.
//
//------------------------------------------------------------------------------
HRESULT 
CTileBrushUtils::GetIntermediateBaseTile(
    __in_ecount(1) CMilTileBrushDuce *pBrushData,
        // Interface to retrieve tile brush content from
    __in_ecount(1) const BrushContext *pBrushContext,
        // Context, including World->Device transform & clip, the brush is 
        // being realized for
    __in_ecount(1) const CMILMatrix *pContentToViewport,
        // Content->Viewport portion of the Content->World transform
    __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        // Viewport->World portion of the Content->World transform.  This
        // transform, along with the World->Device transform, are used
        // to determine the size of the Viewport in device coordinates.
    __in_ecount(1) const MilPointAndSizeD *pViewport,
        // User-specified base tile coordinates
    __in_ecount_opt(1) const BrushCachingParameters *pCachingParams,
        // Optional reuse parameters for brush-caching.  
    __in MilTileMode::Enum tileMode,
        // Wrapping mode used to create the intermediate render target
    __out_ecount(1) IWGXBitmapSource **ppBaseTile,
        // Rasterized base image to be tiled
    __out_ecount(1) CMILMatrix *pmatIntermediateBitmapToXSpace,
        // IntermediateBitmap->SampleSpace matrix
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
        // outside of the viewport.
    __out_ecount(1) XSpaceDefinition *pXSpaceDefinition
        // Space of source clip and pmatIntermediateToXSpace transform
    ) 
{ 
    HRESULT hr = S_OK;

    IMILRenderTargetBitmap *pIRenderTarget = NULL;
    CDrawingContext *pDrawingContext = NULL;

    IWGXBitmapSource *pSurface = NULL;
    
    CMilRectF rcSurfaceBounds;

    //
    // Create an intermediate render target to render content into
    //

    IFC(CTileBrushUtils::CreateTileBrushIntermediate(
        pBrushContext,
        pContentToViewport,
        pmatViewportToWorld,
        pViewport,
        pCachingParams,
        tileMode,
        OUT &pSurface,
        OUT &pIRenderTarget,
        OUT &pDrawingContext,
        OUT pmatIntermediateBitmapToXSpace, // Delegate setting of out-param
        OUT pfTileIsEmpty,                  // Delegate setting of out-param
        OUT pfUseSourceClip,                // Delegate setting of out-param
        OUT pfSourceClipIsEntireSource,     // Delegate setting of out-param
        OUT pSourceClipXSpace,              // Delegate setting of out-param
        OUT pXSpaceDefinition               // Delegate setting of out-param
        ));

    // Early out if the brush is empty (e.g., because of a degenerate transform)
    if (*pfTileIsEmpty)
    {
        goto Cleanup;
    }

    //        
    // Obtain the render target surface bitmap & bounds
    //

    if (NULL == pSurface)
    {    
        //
        // Cached surface wasn't found.  Draw into the new surface.
        //
        
        Assert(pIRenderTarget);
        Assert(pDrawingContext);
            
        IFC(pIRenderTarget->GetBitmapSource(&pSurface));
        Assert(pSurface);
        
        IFC(GetBitmapSourceBounds(
            pSurface,
            &rcSurfaceBounds
            ));            

        //
        // Render the content into the intermediate surface
        //

        IFC(pDrawingContext->BeginFrame(
            pIRenderTarget
            DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Device)
            ));
        
        IFC(pBrushData->DrawIntoBaseTile(
            pBrushContext,
            &rcSurfaceBounds,
            pDrawingContext
            ));
        
        pDrawingContext->EndFrame();
    }
    // else, a cached surface was found

    // Set base tile upon success
    *ppBaseTile = pSurface;
    pSurface = NULL; // Steal ref

Cleanup:

    // Release interfaces used to create the bitmap source
    ReleaseInterfaceNoNULL(pIRenderTarget);
    ReleaseInterfaceNoNULL(pDrawingContext);

    // Release the rendered surface if it's still set
    ReleaseInterfaceNoNULL(pSurface);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CTileBrushUtils::CreateTileBrushIntermediate
//
//  Synopsis:
//      Creates a new intermediate render target & render context to render a
//      base tile of a tile brush into (i.e., a single instance of the bitmap
//      that needs to be tiled), and a matrix that maps from that intermediate
//      bitmap to world coordinates.
//
//      After this method returns and the client renders into the intermediate
//      render target, they can extract the underlying bitmap and, with the
//      transform and source clip, create a texture Brush.
//
//      To avoid scaling the intermediate bitmap after it is created, this
//      method applies the appropriate scale transforms to determine the final
//      size of the rasterized tile, and creates the intermediate render target
//      at this size.
//
//------------------------------------------------------------------------------
HRESULT
CTileBrushUtils::CreateTileBrushIntermediate(
    __in_ecount(1) const BrushContext *pBrushContext,
        // Brush context information
    __in_ecount(1) const CMILMatrix *pmatContentToViewport,
        // Transform inferred from the Viewbox, Viewport, Stretch, & Alignment properties
    __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        // User-specified Viewport->World transform
    __in_ecount(1) const MilPointAndSizeD *prcdViewport,
        // User-specified viewport in world sampling coordinates
    __in_ecount_opt(1) const BrushCachingParameters *pCachingParams,
        // Optional reuse parameters for brush-caching.          
    MilTileMode::Enum tileMode,
        // User-specified tile mode
    __deref_out_ecount_opt(1) IWGXBitmapSource **ppCachedSurface,
        // Cached intermediate surface to use.  If non-NULL, ppIRenderTarget & ppDrawingContext will be NULL.
    __deref_out_ecount_opt(1) IMILRenderTargetBitmap **ppIRenderTarget,
        // Intermediate render target for this tile brush.  If non-NULL, ppCachedSurface will be NULL.
    __deref_out_ecount_opt(1) CDrawingContext **ppDrawingContext,
        // Drawing context to use on intermediate render target.  If non-NULL, ppCachedSurface will be NULL.
    __out_ecount(1) CMILMatrix* pmatSurfaceToXSpace,
        // Transforms from the ouput intermediate surface to world coordinates
    __out_ecount(1) BOOL *pfBrushIsEmpty,
        // Does this brush render nothing?
    __out_ecount_opt(1) BOOL *pfUseSourceClip,
        // Whether or not the source clip output parameters are valid, and should be 
        // applied.
    __out_ecount_opt(1) BOOL *pfSourceClipIsEntireSource,
        // Whether or not the source clip is equal to the entire source content
    __out_ecount_opt(1) CParallelogram *pSourceClipXSpace,
        // Clip parallelogram to apply when rendering the brush.  This is
        // used to implement MilTileMode::None to prevent content from rendering
        // outside of the viewport.
    __out_ecount(1) XSpaceDefinition *pXSpaceDefinition
        // Space of source clip and pmatBaseTileToXSpace transform          
    )
{
    // Assert required in-params
    Assert( pBrushContext &&
            pmatContentToViewport &&
            prcdViewport &&
            ppIRenderTarget &&
            ppDrawingContext &&
            pmatSurfaceToXSpace &&
            pfBrushIsEmpty &&
            pXSpaceDefinition
            );

    if (pfUseSourceClip)
    {
        Assert(pfSourceClipIsEntireSource);
        Assert(pSourceClipXSpace);
    }

    HRESULT hr = S_OK;

    //
    // In 2D for MilTileMode::None and MilTileMode::Extend we realize the intermediate in the orientation of
    // samplespace to avoid double bilinear filtering. (We can't do this for
    // other tile modes because we need the viewport to be mapped to a
    // rectangle in intermediate space such that we can do tiling.)
    //
    // In 3D and for all tiling cases (2D & 3D) we realize the intermediate in
    // the orientation of the viewport so that we can tile the viewport (tiling
    // reason) and so that we can use border mode to handle the viewport clip
    // (3D reason).
    //
    if (   !CMilTileBrushDuce::IsTiling(tileMode)
        && !pBrushContext->fBrushIsUsedFor3D 
        // Don't use a device-aligned realizer if caching is enabled
        //
        // Future Consideration:  Consider brush-caching support for device-aligned realizers
        //
        // Supporting brush-caching on a device-aligned realizer can be done, but requires
        // generalizing CViewportAlignedIntermediateRealizer::CalculateCacheReuseParameters
        // to operate on parallelograms instead of just rectangles.  Specifically,
        // the cached content bounds & intermediate bounds (after they are mapped into
        // the 'New' content space) become parallellograms.
        //
        // To avoid making this already-complex algorithm more complex, we will avoid 
        // doing parallelogram comparisons by always using a Viewport-aligned realizer
        // when caching.
        //
        // This is further supported by the idea that enabling brush-caching is a 
        // quality tradeoff (i.e., pixel-perfect rendering isn't a goal), and that we should 
        // probably stop using intermediate surfaces for device-aligned realization anyways (by
        // rendering directly to the backbuffer with a clip in effect).
        && (NULL == pCachingParams)
       )
    {
        // Initialize out-param only used by the Viewport-aligned realizer to NULL
        if (ppCachedSurface)
        {
            *ppCachedSurface = NULL;
        }

        CDeviceAlignedIntermediateRealizer deviceAlignedIntermediateRealizer(
            pBrushContext,
            pmatContentToViewport,
            pmatViewportToWorld,
            prcdViewport
            );        
        
        IFC(deviceAlignedIntermediateRealizer.Realize(
            ppIRenderTarget,
            ppDrawingContext,
            pmatSurfaceToXSpace, // pmatSurfaceToSampleSpace
            pfBrushIsEmpty,
            pSourceClipXSpace // pSourceClipSampleSpace
            ));

        *pXSpaceDefinition = XSpaceIsSampleSpace;

        if (pfUseSourceClip)
        {
            if (tileMode == MilTileMode::None)
            {
                *pfUseSourceClip = TRUE;

                //
                // The clip is likely not the entire intermediate size
                // because the intermediate is in sample space orientation, not
                // viewport orientation. Even a half pixel translation from
                // viewport space to sample space would cause the source clip
                // to differ from the intermediate size. Therefore we always
                // use the source clip parallelogram to do the clip.
                //
                *pfSourceClipIsEntireSource = FALSE;
            }
            else
            {
                *pfUseSourceClip = FALSE;
            }

        }
    }
    else
    {
        CViewportAlignedIntermediateRealizer viewportAlignedIntermediateRealizer(
            pBrushContext,
            pmatContentToViewport,
            pmatViewportToWorld,
            prcdViewport,
            pCachingParams,
            tileMode
            );
        
        IFC(viewportAlignedIntermediateRealizer.Realize(
            ppCachedSurface,
            ppIRenderTarget,
            ppDrawingContext,
            pmatSurfaceToXSpace, // pmatSurfaceToWorldSpace
            pfBrushIsEmpty,
            pSourceClipXSpace // pSurfaceBoundsWorldSpace
            ));

        *pXSpaceDefinition = XSpaceIsWorldSpace;

        if (pfUseSourceClip)
        {
            if (tileMode == MilTileMode::None)
            {
                *pfUseSourceClip = TRUE;

                //
                // The clip is the entire intermediate size because we have already
                // clipped the intermediate size to the viewport clip.
                //
                *pfSourceClipIsEntireSource = TRUE;
            }
            else
            {
                *pfUseSourceClip = FALSE;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}





