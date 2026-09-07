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

MtExtern(CMilScaleTransform3DDuce);

// Class: CMilScaleTransform3DDuce
class CMilScaleTransform3DDuce : public CMilAffineTransform3DDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilScaleTransform3DDuce));

    CMilScaleTransform3DDuce(__in_ecount(1) CComposition* pComposition)
        : CMilAffineTransform3DDuce(pComposition)
    {
    }

    virtual ~CMilScaleTransform3DDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_SCALETRANSFORM3D || CMilAffineTransform3DDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_SCALETRANSFORM3D* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    static void ClearRealization();
    virtual HRESULT GetRealization(__out_ecount(1) CMILMatrix *pRealization);
    override HRESULT Append(__inout_ecount(1) CMILMatrix *pMat);

    HRESULT SynchronizeAnimatedFields();

    CMilScaleTransform3DDuce_Data m_data;
};

