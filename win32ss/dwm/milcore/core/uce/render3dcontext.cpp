// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------
//

//
//  Abstract:
//     The Render3DContext renders the 3D Visual subtree.  Note that 3D rendering
//     requires 2-passes. Use the Prerender3DContext to initialize the lights
//     and camera.
//

#include "precomp.hpp"

MtDefine(CRender3DContext, MILRender, "CRender3DContext");

//+---------------------------------------------------------------------------
//
//  Member:
//      CRender3DContext::Create
//
//  Synopsis:
//      Creates a render context.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT
CRender3DContext::Create(
    __deref_out_ecount(1) CRender3DContext** ppPrerender3DContext
    )
{
    HRESULT hr = S_OK;

    // Instantiate PreComputeContext
    CRender3DContext* pCtx = new CRender3DContext();
    IFCOOM(pCtx);

    // Instantiate the graph iterator.  Default walk direction is Left -> Right (in order).
    pCtx->m_pGraphIterator = new CGraphIterator();
    IFCOOM(pCtx->m_pGraphIterator);

    *ppPrerender3DContext = pCtx;
    pCtx = NULL;

Cleanup:
    delete pCtx;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CRender3DContext::dtor
//
//----------------------------------------------------------------------------

CRender3DContext::~CRender3DContext()
{
    delete m_pGraphIterator;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CRender3DContext::Render
//
//  Synopsis:
//      Renders the given Visual3D tree to the provided render target using
//      the given CDrawingContext and CContextState.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT 
CRender3DContext::Render(
    __in_ecount(1) CMilVisual3D *pRoot,                 // Root of the Visual3D tree
    __in_ecount(1) CDrawingContext *pDrawingContext,
    __in_ecount(1) CContextState *pContextState,
    __in_ecount(1) IRenderTargetInternal *pRenderTarget,
    float fWidth,                                       // Width of the viewport
    float fHeight                                       // Height of the viewport
    )
{
    HRESULT hr = S_OK;

    m_pDrawingContext = pDrawingContext;
    m_pContextState = pContextState;
    m_pRenderTarget = pRenderTarget;
    m_fWidth = fWidth;
    m_fHeight = fHeight;

    // The WPF specifies that the winding order of triangles is determined in the
    // mesh's local space (before transformation).  Because reflections change the
    // winding order of the triangle we need to flip the current cull mode if the
    // worldToDevice transform has a negative determinant.
    // 
    // We initialize CullMode here to account for the camera transformation and
    // any 2D transformations on the Viewport3DVisual and above.  Model-to-world
    // transforms will be accounted for later by flipping this initial value if
    // the model-to-world matrix has a negative determinant when we are ready to
    // render the geometry.
    
    float det = pContextState->ViewportProjectionModifier3D.GetDeterminant2D();
    det *= pContextState->ProjectionTransform3D.GetDeterminant3D();
    det *= pContextState->ViewTransform3D.GetDeterminant3D();
    
    pContextState->CullMode3D = det < 0 ? D3DCULL_CCW : D3DCULL_CW;
    
    // Initialize the stack
    m_transformStack.Clear();
    IFC(m_transformStack.Push(&(m_pContextState->WorldTransform3D)));

    IFC(m_pGraphIterator->Walk(pRoot, this));

Cleanup:
#if DBG
    m_pDrawingContext = NULL;
    pContextState = NULL;
    pRenderTarget = NULL;
    fWidth = -7;
    fHeight = -13;
#endif // DBG

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CRender3DContext::PreSubgraph (IGraphIteratorSink interface)
//
//  Synopsis:
//      PreSubgraph is called by the graph walker when a visual sub-graph is
//      entered. When leaving the sub-graph of a visual the corresponding method
//      PostSubgraph is invoked.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT
CRender3DContext::PreSubgraph(
    __out_ecount(1) BOOL* pfVisitChildren
    )
{
    HRESULT hr = S_OK;
    *pfVisitChildren = TRUE;

    Assert(m_pGraphIterator != NULL);

    CMilVisual3D* pNode = DYNCAST(CMilVisual3D, m_pGraphIterator->CurrentNode());
    Assert(pNode);

    if (pNode->m_pTransform != NULL) 
    {
        CMILMatrix transform;
        IFC(pNode->m_pTransform->GetRealization(&transform));
        IFC(m_transformStack.Push(&transform));
        m_transformStack.Top(&(m_pContextState->WorldTransform3D));
    }

    if (!pNode->m_pContent)
    {
        // Early exit if the CMilVisual3D has no content.
        goto Cleanup;
    }

    {
        CModelRenderWalker modelWalker(m_pDrawingContext);

        IFC(modelWalker.RenderModels(
            pNode->m_pContent,
            m_pRenderTarget,
            m_pContextState,
            m_fWidth,
            m_fHeight
            ));
    }

Cleanup:  
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CRender3DContext::PostSubgraph (IGraphIteratorSink interface)
//
//  Synopsis:
//      PreSubgraph is called by the graph walker when a visual sub-graph is
//      entered. When leaving the sub-graph of a visual the corresponding method
//      PostSubgraph is invoked.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT
CRender3DContext::PostSubgraph()
{
    HRESULT hr = S_OK;

    Assert(m_pGraphIterator != NULL);

    const CMilVisual3D* pNode = DYNCAST(CMilVisual3D, m_pGraphIterator->CurrentNode());
    Assert(pNode);

    if (pNode->m_pTransform != NULL) 
    {
        m_transformStack.Pop();
        m_transformStack.Top(&m_pContextState->WorldTransform3D);
    }

    RRETURN(hr);
}


