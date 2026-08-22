// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      The CMilBitmapCacheBrushDuce CSlaveResource is responsible for maintaining
//      the current base values & animation resources for all  
//      BitmapCacheBrush properties.  This class processes updates to those
//      properties, and updates a realization based on their current
//      value during GetBrushRealizationInternal.
//
//------------------------------------------------------------------------

MtExtern(CMilBitmapCacheBrushDuce);

class CPreComputeContext;

class CMilBitmapCacheBrushDuce : public CMilBrushDuce, public CMilCyclicResourceListEntry
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilBitmapCacheBrushDuce));

    CMilBitmapCacheBrushDuce(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilSlaveHandleTable *pHTable
        ) : CMilBrushDuce(pComposition),
            CMilCyclicResourceListEntry(pHTable)
    {
        m_pPreComputeContext = NULL;
        SetDirty(TRUE);     
    }

    virtual ~CMilBitmapCacheBrushDuce();

    override HRESULT GetBrushRealizationInternal(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
        );

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_BITMAPCACHEBRUSH || CMilBrushDuce::IsOfType(type);
    }

    override virtual bool NeedsBounds(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        return true;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_BITMAPCACHEBRUSH* pCmd
        );
    
    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();
    override CMilSlaveResource* GetResource();

    HRESULT  GetRenderTargetBitmap(
        __in CComposition *pComposition,
        __in IRenderTargetInternal *pDestRT,
        __deref_out_opt IMILRenderTargetBitmap **ppRTB
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );

private:
    
    HRESULT PreCompute(
        __in_ecount(1) CComposition *pComposition
        ) const;

    HRESULT GeneratedProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_BITMAPCACHEBRUSH* pCmd
        );
    
private:

    // Mutable because it can be lazily allocated by const functions 
    // without breaking the const semantic
    mutable CPreComputeContext *m_pPreComputeContext;
   
public:
    CMilBitmapCacheBrushDuce_Data m_data;

    LocalMILObject<CMILBrushBitmap> m_brushRealization;
};


