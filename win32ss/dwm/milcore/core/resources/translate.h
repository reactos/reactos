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

MtExtern(CMilTranslateTransformDuce);

// Class: CMilTranslateTransformDuce
class CMilTranslateTransformDuce : public CMilTransformDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilTranslateTransformDuce));

    CMilTranslateTransformDuce(__in_ecount(1) CComposition* pComposition)
        : CMilTransformDuce(pComposition)
    {
        SetDirty(TRUE);
    }
    
    virtual ~CMilTranslateTransformDuce();

    CMilTranslateTransformDuce(__in MilPoint2F *pTransformBy) 
    { 
        m_data.m_pXAnimation = m_data.m_pYAnimation = NULL;
        m_data.m_X = pTransformBy->X;
        m_data.m_Y = pTransformBy->Y;
        SetDirty(true);
    }

public:

    static HRESULT Create(
         __in_ecount(1) MilPoint2F *pTransformBy,
        __deref_out CMilTranslateTransformDuce **ppTranslateTransform
        );
    
    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_TRANSLATETRANSFORM || CMilTransformDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_TRANSLATETRANSFORM* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    HRESULT SynchronizeAnimatedFields();

    virtual HRESULT GetMatrixCore(CMILMatrix *pMatrix);

    CMilTranslateTransformDuce_Data m_data;
};

