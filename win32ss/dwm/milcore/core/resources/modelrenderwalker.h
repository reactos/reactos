// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Module Name:
//
//    ModelRenderWalker.h
//
//---------------------------------------------------------------------------------

// Meter declarations -------------------------------------------------------------

MtExtern(CModelRenderWalker);

//---------------------------------------------------------------------------------
// class CModelRenderWalker
//---------------------------------------------------------------------------------

class CModelRenderWalker : private IModelIteratorSink
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CModelRenderWalker));

    CModelRenderWalker(__inout_ecount(1) CDrawingContext *pRC);

    HRESULT RenderModels(
        __in_ecount(1) CMilModel3DDuce *pRoot,
        __in_ecount(1) IRenderTargetInternal* pRenderTarget,
        __in_ecount(1) CContextState *pCtxState,
        float viewportWidth,
        float viewportHeight
        );

    HRESULT PushTransform(__in_ecount(1) const CMILMatrix *pTransform)
    {
        HRESULT hr = m_transformStack.Push(pTransform);

        if (SUCCEEDED(hr))
        {
            m_transformStack.Top(&(m_pCtxState->WorldTransform3D));
        }

        return hr;
    }

    void PopTransform()
    {
        m_transformStack.Pop();
        m_transformStack.Top(&m_pCtxState->WorldTransform3D);
    }

    HRESULT RenderGeometryModel3D(__in_ecount(1) CMilGeometryModel3DDuce *pModel);

private:

    // IModelIteratorSink interface implementation
    virtual HRESULT PreSubgraph(
        __in_ecount(1) CMilModel3DDuce *pModel,
        __out_ecount(1) bool *pfVisitChildren
        );
    virtual HRESULT PostSubgraph(
        __in_ecount(1) CMilModel3DDuce *pModel
        );

    HRESULT ProcessMaterialAndRender(
        __in_ecount(1) CMilMaterialDuce *pMaterial,
        __in_ecount(1) CMILMesh3D *pMesh3D,
        bool fFlipCullMode
        );

    HRESULT RealizeMaterialAndRender(
        __in_ecount(1) const DynArray<CMilMaterialDuce *> *pMaterialList,
        __in_ecount(1) CMILMesh3D *pMesh3D
        );

private:
    CDrawingContext *           m_pRC;
    CContextState *             m_pCtxState;
    CGenericMatrixStack         m_transformStack;
    CModelIterator              m_iterator;
    IRenderTargetInternal*      m_pRenderTarget;
    float                       m_viewportWidth;
    float                       m_viewportHeight;
};


