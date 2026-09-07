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
//      The abstract TileBrush CSlaveResource contains general functionality
//      common to TileBrush subclasses, and specialized abstract methods
//      TileBrush subclasses must implement.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilTileBrushDuce);

class CBrushIntermediateCache;
class CDrawingContext;

//+-----------------------------------------------------------------------------
//
//  Structure:
//      BrushCachingParameters
//
//  Synopsis:
//      Contains all state passed from this class to the brush caching support
//      in CBrushIntermediateCache.
//
//------------------------------------------------------------------------------
struct BrushCachingParameters
{
    // Object responsible for maintaining cached surfaces
    CBrushIntermediateCache *pIntermediateCache;
    
    // Current content bounds in Viewport space (i.e., current Content->Viewport transform
    // has been applied to the content bounds)
    CMilRectF rcCurrentContentBounds_ViewportSpace;

    //
    // The minimum & maximum values of the CacheInvalidationThreshold range
    // 
    float rCacheInvalidationThresholdMinimum;
    float rCacheInvalidationThresholdMaximum;
};

class CMilTileBrushDuce : public CMilBrushDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilTileBrushDuce));

    CMilTileBrushDuce(__in_ecount(1) CComposition *pComposition);

    virtual ~CMilTileBrushDuce() 
    { 
        delete m_pIntermediateCache;
    }

public:

    override bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_TILEBRUSH || CMilBrushDuce::IsOfType(type);
    }

    override void FreeRealizationResources();

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      DoesContainContent
    //
    //  Synopsis:
    //      Determines whether or not the TileBrush contains content.
    //
    //  Notes:
    //      If no content exists, then methods that require content such as
    //      GetContentBounds and GetBaseTile won't be called, and can assume
    //      that they aren't called.
    //
    //--------------------------------------------------------------------------
    virtual HRESULT DoesContainContent(
        __out_ecount(1) BOOL *pfHasContent
        // Whether or not the TileBrush has content
        ) const = 0;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetTilePropertyCurrentValues
    //
    //  Synopsis:
    //      Obtains the current value of the brush's TileBrush properties.
    //
    //--------------------------------------------------------------------------
    HRESULT GetTilePropertyCurrentValues(
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
            // Current value of the low end of the CacheInvalidationThreshold range
        __out_ecount(1) double *pCacheInvalidationThresholdMaximum    
            // Current value of the high end of the CacheInvalidationThreshold range                 
        ) const;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetContentToViewboxScale
    //
    //  Synopsis:
    //      Obtains the Content->Viewbox scale factors.
    //
    //--------------------------------------------------------------------------
    virtual HRESULT GetContentToViewboxScale( 
        __out_ecount(1) REAL *pScaleX,
            // X Scale factor to apply to the content
        __out_ecount(1) REAL *pScaleY        
            // Y Scale factor to apply to the content
        ) const;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetContentBounds
    //
    //  Synopsis:
    //      Obtains the bounds of the content, in device-independent content
    //      units.
    //
    //--------------------------------------------------------------------------
    virtual HRESULT GetContentBounds(
        __in_ecount(1) const BrushContext *pBrushContext, 
            // Context the brush is being realized for        
        __out_ecount(1) CMilRectF *pContentBounds
            // Output content bounds
        ) = 0;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      NeedsIntermediateSurfaceRealization
    //
    //  Synopsis:
    //      Determines whether or not the brush should be realized into an
    //      intermediate surface.  Intermediate surfaces are needed to rasterize
    //      vector content into a texture brush (i.e., by DrawingBrush &
    //      VisualBrush).  They are used by ImageBrush when the source image
    //      needs to be tiled, while also being clipped to the viewport, or
    //      padded with transparent pixels to fill the viewport.
    //
    //  Notes:
    //      If TRUE is returned, GetBaseTile must not be called.  If FALSE is
    //      returned, DrawIntoBaseTile must not be called.
    //
    //--------------------------------------------------------------------------
    virtual HRESULT NeedsIntermediateSurfaceRealization(
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
        // Default implementation for DrawingBrush & VisualBrush returns
        // TRUE because unlike ImageBrush, they never have a source texture.
        // Instead, they must always first rasterize their vector content 
        // into an intermediate surface.
        *pfNeedsIntermediateSurfaceRealization = TRUE;

        *pfBrushIsEmpty = FALSE;

        RRETURN(S_OK);
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      DrawIntoBaseTile
    //
    //  Synopsis:
    //      Draws this brush's content into an already-allocated DrawingContext. 
    //      This method is used to populate an intermediate surface realization.
    //
    //--------------------------------------------------------------------------
    virtual HRESULT DrawIntoBaseTile(
        __in_ecount(1) const BrushContext *pBrushContext,
            // Context the brush is being realized for        
        __in_ecount(1) CMilRectF * prcSurfaceBounds,
            // Bounds of the intermediate suface
        __inout_ecount(1) CDrawingContext *pDrawingContext
            // Drawing context to draw content into
        ) = 0;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetBaseTile
    //
    //  Synopsis:
    //      Obtains the base image to be tiled.  This method is called to obtain
    //      the ImageBrush image & source clip when it's not using an
    //      intermediate surface realiation.
    //
    //--------------------------------------------------------------------------
    virtual HRESULT GetBaseTile(
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
        // Default implementation for DrawingBrush & VisualBrush returns
        // E_NOTIMPL because they always use an intermediate surface
        RIP("Unexpected call to CMilTileBrushDuce::GetBaseTile"); 
        RRETURN(E_NOTIMPL);
    }

    static bool IsTiling(MilTileMode::Enum tileMode)
    {
        switch (tileMode)
        {
        case MilTileMode::FlipX:
        case MilTileMode::FlipY:
        case MilTileMode::FlipXY:
        case MilTileMode::Tile:
            return true;
        default:
            return false;
        }
    }

protected:    

    override HRESULT GetBrushRealizationInternal(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
        );
    
    virtual HRESULT GetTilePropertyResources(
        __out_ecount(1) double *pOpacity,
        __out_ecount(1) CMilSlaveDouble **ppOpacityAnimation,
        __out_ecount(1) CMilTransformDuce **ppTransformResource,
        __out_ecount(1) CMilTransformDuce **ppRelativeTransformResource,
        __out_ecount(1) MilBrushMappingMode::Enum *pViewportUnits,
        __out_ecount(1) MilBrushMappingMode::Enum *pViewboxUnits,
        __out_ecount(1) MilPointAndSizeD *pViewport,
        __out_ecount(1) CMilSlaveRect **ppViewportAnimations,
        __out_ecount(1) MilPointAndSizeD *pViewbox,
        __out_ecount(1) CMilSlaveRect **ppViewboxAnimations,
        __out_ecount(1) MilStretch::Enum *pStretch,
        __out_ecount(1) MilTileMode::Enum *pTileMode,
        __out_ecount(1) MilHorizontalAlignment::Enum *pAlignmentX,
        __out_ecount(1) MilVerticalAlignment::Enum *pAlignmentY,
        __out_ecount(1) double *pCacheInvalidationThresholdMinimum,
        __out_ecount(1) double *pCacheInvalidationThresholdMaximum   
        ) const = 0;

protected:

#if DBG

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      CMilTileBrushDuce::DbgHasContent
    //
    //  Synopsis:
    //      DBG-only methods that returns whether or not the brush has content,
    //      but ignores the HRESULT returned from DoesContainContent
    //
    //--------------------------------------------------------------------------
    BOOL DbgHasContent() const
    {
        BOOL fHasContent;
        IGNORE_HR(DoesContainContent(&fHasContent));
        return fHasContent;
    }
#endif

    virtual bool IsCachingEnabled() = 0;

private:

    VOID SetupCachingParameters(
        __in_ecount(1) CMILMatrix &matContentToViewport,
        __in_ecount(1) CMilRectF &rcContentBoundsF,
        __in_ecount(1) double &rCacheInvalidationThresholdMinimumD,
        __in_ecount(1) double &rCacheInvalidationThresholdMaximumD,
        __out_ecount(1) BrushCachingParameters &brushCachingParams
        );  
    
private:

    CBrushIntermediateCache *m_pIntermediateCache;

    LocalMILObject<CMILBrushBitmap> m_realizedBitmapBrush;
};



