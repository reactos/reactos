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

MtExtern(CMilMatrixTransformDuce);

// Class: CMilMatrixTransformDuce
class CMilMatrixTransformDuce : public CMilTransformDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilMatrixTransformDuce));

    CMilMatrixTransformDuce(__in_ecount(1) CComposition* pComposition)
        : CMilTransformDuce(pComposition)
    {
    }

    virtual ~CMilMatrixTransformDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_MATRIXTRANSFORM || CMilTransformDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_MATRIXTRANSFORM* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

public:
    virtual HRESULT GetMatrixCore(CMILMatrix *pMatrix);

    CMilMatrixTransformDuce_Data m_data;

};

