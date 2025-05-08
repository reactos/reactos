// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//------------------------------------------------------------------------

MtExtern(CMilModel3DDuce);

class CPrerenderWalker;
class CModelRenderWalker;

// Class: CMilModel3DDuce
class CMilModel3DDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilModel3DDuce));

    CMilModel3DDuce(__in_ecount(1) CComposition*)
    {
    }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_MODEL3D;
    }

    virtual __out_ecount_opt(1) CMilTransform3DDuce *GetTransform() = 0;
    virtual HRESULT PreRender(
        __in_ecount(1) CPrerenderWalker *pPrerenderer,
        __in_ecount(1) CMILMatrix *pTransform
        );
    virtual HRESULT Render(__in_ecount(1) CModelRenderWalker *pRenderer);
    virtual void PostRender(__in_ecount(1) CModelRenderWalker *pRenderer);
    virtual HRESULT GetDepthSpan(
        __in_ecount(1) CMILMatrix *pTransform,
        __inout float &zmin,
        __inout float &zmax
        );
};

