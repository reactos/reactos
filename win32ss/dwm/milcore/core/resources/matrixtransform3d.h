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

MtExtern(CMilMatrixTransform3DDuce);

// Class: CMilMatrixTransform3DDuce
class CMilMatrixTransform3DDuce : public CMilTransform3DDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilMatrixTransform3DDuce));

    CMilMatrixTransform3DDuce(__in_ecount(1) CComposition* pComposition)
        : CMilTransform3DDuce(pComposition)
    {
    }

    virtual ~CMilMatrixTransform3DDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_MATRIXTRANSFORM3D || CMilTransform3DDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_MATRIXTRANSFORM3D* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    void ClearRealization();
    virtual HRESULT GetRealization(__out_ecount(1) CMILMatrix *pRealization);
    override HRESULT Append(__inout_ecount(1) CMILMatrix *pMat);

    CMilMatrixTransform3DDuce_Data m_data;
};

