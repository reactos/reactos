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

MtExtern(CMilSkewTransformDuce);

// Class: CMilSkewTransformDuce
class CMilSkewTransformDuce : public CMilTransformDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilSkewTransformDuce));

    CMilSkewTransformDuce(__in_ecount(1) CComposition* pComposition)
        : CMilTransformDuce(pComposition)
    {
        SetDirty(TRUE);
    }

    virtual ~CMilSkewTransformDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_SKEWTRANSFORM || CMilTransformDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_SKEWTRANSFORM* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    HRESULT SynchronizeAnimatedFields();

    virtual HRESULT GetMatrixCore(CMILMatrix *pMatrix);

    CMilSkewTransformDuce_Data m_data;
};

