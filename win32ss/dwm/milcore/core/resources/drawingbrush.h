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

MtExtern(CMilDrawingBrushDuce);

class CMilDrawingBrushDuce : public CMilTileBrushDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilDrawingBrushDuce));

    CMilDrawingBrushDuce(__in_ecount(1) CComposition* pComposition)
        : CMilTileBrushDuce(pComposition)
    {
    }

    virtual ~CMilDrawingBrushDuce();

public:

    override bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_DRAWINGBRUSH || CMilTileBrushDuce::IsOfType(type);
    }

    override bool NeedsBounds(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        // The shape bounds are needed when creating an intermediate
        // surface during CTileBrushUtils::CalculateScaledWorldTile to
        // clip non-visible portions from the intermediate allocation.        
        return true;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_DRAWINGBRUSH* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();
    
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

    override bool RealizationSourceClipMayBeEntireSource(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        Assert(RealizationWillHaveSourceClip());
        // code duplicated in visualbrush.h
        return pBrushContext->fBrushIsUsedFor3D;
    }

protected:

    override HRESULT DoesContainContent(
        __out_ecount(1) BOOL *pfHasContent
        ) const;       

    override HRESULT GetTilePropertyResources(
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

    override HRESULT GetContentBounds(
        __in_ecount(1) const BrushContext *pBrushContext,        
        __out_ecount(1) CMilRectF *pContentBounds
        ) ;

    override HRESULT DrawIntoBaseTile(
        __in_ecount(1) const BrushContext *pBrushContext,     
        __in_ecount(1) CMilRectF * prcSurfaceBounds,
        __inout_ecount(1) CDrawingContext *pDrawingContext
        );       

    override bool IsCachingEnabled()
    {
        return m_data.m_CachingHint == MilCachingHint::Cache;
    }
    
private:

    CMilDrawingBrushDuce_Data m_data;
};


