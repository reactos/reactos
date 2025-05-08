// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_lighting
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilSpotLightDuce);

// Class: CMilSpotLightDuce
class CMilSpotLightDuce : public CMilPointLightDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilSpotLightDuce));

    CMilSpotLightDuce(__in_ecount(1) CComposition* pComposition)
        : CMilPointLightDuce(pComposition)
    {
    }

    virtual ~CMilSpotLightDuce();

public:

    virtual __out_ecount_opt(1) CMilTransform3DDuce *GetTransform();
    virtual HRESULT PreRender(
        __in_ecount(1) CPrerenderWalker *pPrerenderer,
        __in_ecount(1) CMILMatrix *pTransform
        );

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_SPOTLIGHT || CMilPointLightDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_SPOTLIGHT* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    static void ClearRealization();
    HRESULT GetRealization(
        __deref_out_ecount_opt(1) CMILLightSpot **ppRealizationNoRef
        );

    HRESULT SynchronizeAnimatedFields();

    CMilSpotLightDuce_Data m_data;

    CMILLightSpot m_spotLightRealization;
};

