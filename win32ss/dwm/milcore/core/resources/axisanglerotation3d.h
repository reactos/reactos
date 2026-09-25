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

MtExtern(CMilAxisAngleRotation3DDuce);

// Class: CMilAxisAngleRotation3DDuce
class CMilAxisAngleRotation3DDuce : public CMilRotation3DDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilAxisAngleRotation3DDuce));

    CMilAxisAngleRotation3DDuce(__in_ecount(1) CComposition* pComposition)
        : CMilRotation3DDuce(pComposition)
    {
    }

    virtual ~CMilAxisAngleRotation3DDuce();

public:

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_AXISANGLEROTATION3D || CMilRotation3DDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_AXISANGLEROTATION3D* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    override HRESULT GetRealization(__out_ecount(1) CMILMatrix *pRealization);

    HRESULT SynchronizeAnimatedFields();

    CMilAxisAngleRotation3DDuce_Data m_data;
};

