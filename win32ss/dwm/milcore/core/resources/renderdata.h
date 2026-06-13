// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
// Module Name:
//
//    renderdata.h
//
// Abstract:
//
//    Declarations of the MIL render stream(data) resource
//        CMilResourceRenderStream
//
// Environment:
//
//    User mode only.
//---------------------------------------------------------------------------------


MtExtern(CMilSlaveRenderData);
MtExtern(CMilRenderData_arryHandles);

interface IDrawingContext;
class CGuidelineCollection;
class CRenderDataDrawFrame;

class CMilSlaveRenderData : public CMilSlaveResource
{
    friend class CResourceFactory;
    friend class CWindowRenderTarget;

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilSlaveRenderData));

    CMilSlaveRenderData(__in_ecount(1) CComposition* pComposition);

    virtual ~CMilSlaveRenderData();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_RENDERDATA;
    }

    DynArray<CMilSlaveResource*, TRUE> *GetResourcePtrArray()
    {
        return &m_rgpResources;
    }

    HRESULT Draw(__in_ecount(1) IDrawingContext *pIDC);


    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_RENDERDATA* prd,
        __in_bcount_opt(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    CGuidelineCollection* GetGuidelineCollection(UINT index)
    {
        return m_rgpGuidelineKits[index];
    }

    HRESULT ScheduleRender();

private:

    HRESULT GetHandles(CMilSlaveHandleTable *pHandleTable);
    void DestroyRenderData();

    HRESULT BeginBoundingFrame(
        __deref_in_range(>=, 0) __deref_out_range(==, 0) int *piCurrentFrameStackDepth,
        __ecount(1) CRectF<CoordinateSpace::LocalRendering> *prcBounds,
        __deref_inout_ecount_inopt(1) CRenderDataDrawFrame **ppCurrentFrame,
        __deref_inout_ecount_inopt(1)  IDrawingContext **ppCurrentDC
        );

    HRESULT EndBoundingFrame(
        __inout_ecount(1) int *piCurrentFrameStackDepth,
        __deref_inout_ecount_outopt(1) CRenderDataDrawFrame **ppCurrentFrame,
        __deref_out_ecount_opt(1) IDrawingContext **ppCurrentDC,
        __in_ecount(1) IDrawingContext *pOriginalDC
        );

    CComposition *m_pComposition;
    CMilScheduleRecord *m_pScheduleRecord;

    CMilDataStreamWriter m_instructions;

    DynArray<CMilSlaveResource*, TRUE> m_rgpResources;
    DynArray<CGuidelineCollection*> m_rgpGuidelineKits;
};


