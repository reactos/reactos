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

MtExtern(CMilTransformGroupDuce);

// Class: CMilTransformGroupDuce
class CMilTransformGroupDuce : public CMilTransformDuce, public CMilCyclicResourceListEntry
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilTransformGroupDuce));

    CMilTransformGroupDuce(
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) CMilSlaveHandleTable *pHTable        
        ) : CMilTransformDuce(pComposition),
            CMilCyclicResourceListEntry(pHTable)
    {
        SetDirty(TRUE);
    }

    virtual ~CMilTransformGroupDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_TRANSFORMGROUP || CMilTransformDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_TRANSFORMGROUP* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();
    override virtual CMilSlaveResource* GetResource();

    virtual HRESULT GetMatrixCore(CMILMatrix *pMatrix);

    CMilTransformGroupDuce_Data m_data;
};

