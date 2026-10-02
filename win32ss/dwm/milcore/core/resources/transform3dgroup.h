// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_transform
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilTransform3DGroupDuce);

// Class: CMilTransform3DGroupDuce
class CMilTransform3DGroupDuce : public CMilTransform3DDuce, public CMilCyclicResourceListEntry
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilTransform3DGroupDuce));

    CMilTransform3DGroupDuce(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilSlaveHandleTable *pHTable        
        ) : CMilTransform3DDuce(pComposition),
            CMilCyclicResourceListEntry(pHTable)
    {
    }

    virtual ~CMilTransform3DGroupDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_TRANSFORM3DGROUP || CMilTransform3DDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_TRANSFORM3DGROUP* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();
    override CMilSlaveResource* GetResource();

    static void ClearRealization();
    virtual HRESULT GetRealization(__out_ecount(1) CMILMatrix *pRealization);
    override HRESULT Append(__inout_ecount(1) CMILMatrix *pMat);

    CMilTransform3DGroupDuce_Data m_data;
};

