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

MtExtern(CMilQuaternionRotation3DDuce);

// Class: CMilQuaternionRotation3DDuce
class CMilQuaternionRotation3DDuce : public CMilRotation3DDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilRotation3DDuce));

    CMilQuaternionRotation3DDuce(__in_ecount(1) CComposition* pComposition)
        : CMilRotation3DDuce(pComposition)
    {
    }

    override virtual ~CMilQuaternionRotation3DDuce();

public:

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_QUATERNIONROTATION3D || CMilRotation3DDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_QUATERNIONROTATION3D* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    override virtual HRESULT GetRealization(__out_ecount(1) CMILMatrix *pRealization);

    override virtual HRESULT SynchronizeAnimatedFields();

    CMilQuaternionRotation3DDuce_Data m_data;
};

