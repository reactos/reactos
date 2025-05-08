// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Implicit input brush resource header.
//
//------------------------------------------------------------------------------

MtExtern(CMilImplicitInputBrushDuce);

// Class: CMilImplicitInputBrushDuce
class CMilImplicitInputBrushDuce : public CMilBrushDuce
{
    friend class CResourceFactory;
    friend class CWindowRenderTarget;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilImplicitInputBrushDuce));

    CMilImplicitInputBrushDuce(__in_ecount(1) CComposition* pComposition)
        : CMilBrushDuce(pComposition)
    {
        SetDirty(TRUE);
    }

    virtual ~CMilImplicitInputBrushDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_IMPLICITINPUTBRUSH || CMilBrushDuce::IsOfType(type);
    }

   
    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_IMPLICITINPUTBRUSH* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    override virtual bool NeedsBounds(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        return false;
    }
    
    override HRESULT GetBrushRealizationInternal(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
        );

    CMilImplicitInputBrushDuce_Data m_data;

    LocalMILObject<CMILBrushSolid> m_solidBrushRealization;
};

