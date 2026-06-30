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

MtExtern(CMilImageBrushDuce);

class CMilImageBrushDuce : public CMilTileBrushDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilImageBrushDuce));

    CMilImageBrushDuce(__in_ecount(1) CComposition* pComposition)
        : CMilTileBrushDuce(pComposition)
    {
    }

    virtual ~CMilImageBrushDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_IMAGEBRUSH || CMilTileBrushDuce::IsOfType(type);
    }

    override virtual bool NeedsBounds(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        // Shape bounds are needed when ViewportUnits are relative, when a
        // relative transform is used, or when the brush is being realized into
        // an intermediate surface due to a DrawingImage or sub/super-rect
        // tiling.  The bounds are required by
        // CTileBrushUtils::CalculateScaledWorldTile to clip non-visible
        // portions from the intermediate allocation.  Instead of evaluating
        // NeedsIntermediateSurfaceRealization twice to determine the latter
        // case, we'll always request the shape bounds by returning 'true'. 
        //
        // Future Consideration:  We could return 'false' for absolute
        // Viewport's / no relative transform / when not realizing into an
        // intermediate. This would avoid the bounds compututation.  To do that
        // we'd need some way of calling NeedsIntermediateSurfaceRealization
        // twice during a single render pass without invoking the actual logic
        // in NeedsIntermediateSurfaceRealization more than once (e.g., by
        // caching the return value).  If we cached the value, we also need a
        // method to determine when the cached value was no longer valid. 
        // Since this uncommon case would require more work for the common
        // case, this optimization isn't being implemented, but that
        // cost/benefit may change if we received RenderPassOver()
        // notifications in the future.
        return true;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_IMAGEBRUSH* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    override HRESULT GetBrushRealizationInternal(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
        );

    override void FreeRealizationResources();

    override bool RealizationMayNeedNonPow2Tiling(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        UNREFERENCED_PARAMETER(pBrushContext);

        return CMilTileBrushDuce::IsTiling(m_data.m_TileMode);
    }

    override bool RealizationWillHaveSourceClip() const
    {
        return m_data.m_TileMode == MilTileMode::None;
    }

    virtual BOOL HasRealizationContextChanged(
        IN const BrushContext *pBrushContext
        ) const
    { 
        // If this is a 'normal' image brush that doesn't require an intermediate, we use
        // the cached realization as long as the Viewport size didn't change.        
        return  m_fRealizationIsIntermediate ||        
                (m_data.m_ViewportUnits == MilBrushMappingMode::RelativeToBoundingBox &&
                 // Return true if the brush sizing bounds have changed
                 //
                 // We use exact equality here because fuzzy checks are expensive, coming up 
                 // with a fuzzy threshold that defines the point at which visible changes
                 // occur isn't straightforward (i.e., the brush sizing bounds aren't
                 // in device space), and exact equality handles the case we need to optimize
                 // for where a brush fills the exact same geometry more than once.
                 !IsExactlyEqualRectD(pBrushContext->rcWorldBrushSizingBounds, m_cachedBrushSizingBounds));
    }
    

protected:    

    virtual HRESULT DoesContainContent(
        __out_ecount(1) BOOL *pfHasContent
        ) const;

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
        ) const;

    virtual HRESULT GetContentToViewboxScale(
        __out_ecount(1) REAL *pScaleX,
        __out_ecount(1) REAL *pScaleY        
        ) const;

    virtual HRESULT GetContentBounds(
        __in_ecount(1) const BrushContext *pBrushContext,        
        __out_ecount(1) CMilRectF *pContentBounds
        ) ;    

    virtual HRESULT NeedsIntermediateSurfaceRealization(
        __in_ecount(1) const BrushContext *pBrushContext,  
        __in_ecount(1) const CMILMatrix *pContentToViewport,
        __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        __in_ecount(1) const MilPointAndSizeD *pViewport,
        __in MilTileMode::Enum tileMode,   
        __out_ecount(1) BOOL *pfNeedsIntermediateSurfaceRealization,
        __out_ecount(1) BOOL *pfBrushIsEmpty
        );    

    virtual HRESULT DrawIntoBaseTile(
        __in_ecount(1) const BrushContext *pBrushContext,     
        __in_ecount(1) CMilRectF * prcSurfaceBounds,
        __inout_ecount(1) CDrawingContext *pDrawingContext
        );

    virtual HRESULT GetBaseTile(
        __in_ecount(1) const CMILMatrix *pmatWorldToSampleSpace,
        __in_ecount(1) const CMILMatrix *pContentToViewport,
        __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        __in_ecount(1) const MilPointAndSizeD *pViewport,
        __in MilTileMode::Enum tileMode,
        __out_ecount(1) IWGXBitmapSource **ppBaseTile,
        __out_ecount(1) CMILMatrix *pmatBaseTileToXSpace,
        __out_ecount(1) BOOL *pfTileIsEmpty,
        __out_ecount(1) BOOL *pfUseSourceClip,
        __out_ecount(1) BOOL *pfSourceClipIsEntireSource,
        __out_ecount(1) CParallelogram *pSourceClipXSpace,
        __out_ecount(1) XSpaceDefinition *pXSpaceDefinition
        );

    override virtual bool IsCachingEnabled()
    { return m_data.m_CachingHint == MilCachingHint::Cache; }

private:

    HRESULT CalculateSourceClip(
        __in_ecount(1) IWGXBitmapSource *pImageSource,
        __in_ecount(1) const CMILMatrix *pmatWorldToSampleSpace,
        __in_ecount(1) const CMILMatrix *pContentToViewport,
        __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        __in_ecount(1) const MilPointAndSizeD* pViewport,
        __out_ecount(1) CParallelogram *pSourceClipWorldSpace,
        __out_ecount(1) BOOL *pfSourceClipIsEntireSource
        ) const;

    HRESULT SourceClipApproximatesContentBounds(
        __in_ecount(1) const CMilRectF *prcViewport,
        __in_ecount(1) IWGXBitmapSource *pImageSource,
        __in_ecount(1) const CMILMatrix *pmatWorldToSampleSpace,
        __in_ecount(1) const CMILMatrix *pContentToViewport,
        __in_ecount(1) const CMILMatrix *pmatViewportToWorld,
        __out_ecount(1) CMilRectF *prcContentBounds,
        __out_ecount(1) BOOL *pfSourceClipApproximatesContentBounds
        ) const;

private:
        
    BOOL m_fRealizationIsIntermediate;

    //
    // Brush sizing bounds used to create the last realization.  We store this
    // to compare against future sizing bounds so we can avoid re-creating
    // the realization when the brush's sizing bounds haven't changed.
    // This is only used when an intermediate surface wasn't created.
    //
    MilPointAndSizeD m_cachedBrushSizingBounds;    

    CMilImageBrushDuce_Data m_data;
};



