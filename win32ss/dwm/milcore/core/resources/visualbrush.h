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

MtExtern(CMilVisualBrushDuce);

class CPreComputeContext;

class CMilVisualBrushDuce : public CMilTileBrushDuce, public CMilCyclicResourceListEntry
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilVisualBrushDuce));

    CMilVisualBrushDuce(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilSlaveHandleTable *pHTable
        ) : CMilTileBrushDuce(pComposition),
            CMilCyclicResourceListEntry(pHTable)
    {
        m_pPreComputeContext = NULL;
        SetDirty(TRUE);     
    }

    virtual ~CMilVisualBrushDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_VISUALBRUSH || CMilTileBrushDuce::IsOfType(type);
    }

    override virtual bool NeedsBounds(
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
        __in_ecount(1) const MILCMD_VISUALBRUSH* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();
    override CMilSlaveResource* GetResource();

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

    virtual bool RealizationSourceClipMayBeEntireSource(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        Assert(RealizationWillHaveSourceClip());
        // code duplicated in drawingbrush.h
        return pBrushContext->fBrushIsUsedFor3D;
    }

    static HRESULT PreComputeHelper(
        __in CPreComputeContext *pPreComputeContext,
        __in CMilVisual *pVisual
        );

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

    virtual HRESULT GetContentBounds(
        __in_ecount(1) const BrushContext *pBrushContext,        
        __out_ecount(1) CMilRectF *pContentBounds
        ) ;

    virtual HRESULT DrawIntoBaseTile(
        __in_ecount(1) const BrushContext *pBrushContext,     
        __in_ecount(1) CMilRectF * prcSurfaceBounds,
        __inout_ecount(1) CDrawingContext *pDrawingContext
        );    
    
    override virtual bool IsCachingEnabled()
    { return m_data.m_CachingHint == MilCachingHint::Cache; }    

private:
    
    HRESULT PreCompute(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const;

private:

    // Mutable because it can be lazily allocated by const functions 
    // without breaking the const semantic
    mutable CPreComputeContext *m_pPreComputeContext;
   
public:
    CMilVisualBrushDuce_Data m_data;
};



