// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//       DrawingGroup Duce resource definition.
//

MtExtern(CMilDrawing);
MtExtern(CMilDrawingGroupDuce);

//+----------------------------------------------------------------------------
//
//  Class:     CMilDrawingGroupDuce
//
//  Synopsis:  CMilDrawingDuce implementation that contains a group of Drawings.  
//             This class is the resource that backs the managed DrawingGroup
//             class.
//
//-----------------------------------------------------------------------------
class CMilDrawingGroupDuce : public CMilDrawingDuce, public CMilCyclicResourceListEntry
{
    friend class CResourceFactory;

protected:
    
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilDrawingGroupDuce));

    CMilDrawingGroupDuce(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilSlaveHandleTable *pHTable
        ) : CMilDrawingDuce(pComposition),
            CMilCyclicResourceListEntry(pHTable)
    {
        m_pContent = NULL;
    }

    virtual ~CMilDrawingGroupDuce() 
    { 
        UnRegisterNotifier(m_pContent);
        UnRegisterNotifiers(); 
    }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_DRAWINGGROUP || CMilDrawingDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_DRAWINGGROUP* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    
    override void UnRegisterNotifiers();
    override CMilSlaveResource* GetResource();

    virtual HRESULT Draw(
        __in_ecount(1) CDrawingContext *pDrawingContext
        );    

private:

    HRESULT GetChildrenBounds(__in_ecount(1) CContentBounder *pContentBounder,
                              __out_ecount(1) CRectF<CoordinateSpace::LocalRendering> *pBounds);
    
    CMilDrawingGroupDuce_Data m_data;
    CMilDrawingDuce *m_pContent;  // Points to a drawing
    bool m_fInBoundsCalculation;
};



