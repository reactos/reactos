// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//------------------------------------------------------------------------

MtExtern(CMilModel3DGroupDuce);

// Class: CMilModel3DGroupDuce
class CMilModel3DGroupDuce : public CMilModel3DDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilModel3DGroupDuce));

    CMilModel3DGroupDuce(__in_ecount(1) CComposition* pComposition)
        : CMilModel3DDuce(pComposition)
    {
    }

    virtual ~CMilModel3DGroupDuce();

public:

    virtual __out_ecount_opt(1) CMilTransform3DDuce *GetTransform();
    virtual HRESULT Render(__in_ecount(1) CModelRenderWalker *pRenderer);
    virtual void PostRender(__in_ecount(1) CModelRenderWalker *pRenderer);

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_MODEL3DGROUP || CMilModel3DDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_MODEL3DGROUP* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

public:

    CMilModel3DGroupDuce_Data m_data;
};

