// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//------------------------------------------------------------------------

MtExtern(CMilGeometryModel3DDuce);

// Class: CMilGeometryModel3DDuce
class CMilGeometryModel3DDuce : public CMilModel3DDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilGeometryModel3DDuce));

    CMilGeometryModel3DDuce(__in_ecount(1) CComposition* pComposition)
        : CMilModel3DDuce(pComposition)
    {
    }

    virtual ~CMilGeometryModel3DDuce();

public:

    virtual __out_ecount_opt(1) CMilTransform3DDuce *GetTransform();
    virtual HRESULT Render(__in_ecount(1) CModelRenderWalker *pRenderer);
    virtual HRESULT GetDepthSpan(
        __in_ecount(1) CMILMatrix *pTransform,
        __inout float &zmin,
        __inout float &zmax
        );

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GEOMETRYMODEL3D || CMilModel3DDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_GEOMETRYMODEL3D* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

public:

    CMilGeometryModel3DDuce_Data m_data;
};


