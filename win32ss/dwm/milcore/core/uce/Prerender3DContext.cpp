// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------
//

//
//  Abstract:
//     The Prerender3DContext walks the 3D Visual subtree collecting lights
//     and (optionally) computing the near and far camera planes.
//

#include "precomp.hpp"

MtDefine(CPrerender3DContext, MILRender, "CPrerender3DContext");

//+---------------------------------------------------------------------------
//
//  Member:
//      CPrerender3DContext::Create
//
//  Synopsis:
//      Creates a prerender context.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT
CPrerender3DContext::Create(
    __deref_out_ecount(1) CPrerender3DContext** ppPrerender3DContext
    )
{
    HRESULT hr = S_OK;

    // Instantiate PreComputeContext
    CPrerender3DContext* pCtx = new CPrerender3DContext();
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
//      CPrerender3DContext::Compute
//
//  Synopsis:
//      walks the 3D Visual subtree collecting lights and (optionally) computing
//      the near and far camera planes.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT 
CPrerender3DContext::Compute(
    __in_ecount(1) CMilVisual3D *pRoot,             // Root of the Visual3D tree
    __in_ecount(1) CMILMatrix *pViewTransform,      // Camera view transform (not full projection)
    __inout_ecount(1) CMILLightData *pLightData,    // Ptr to LightData to populate
    bool fComputeClipPlanes,                        // indicates whether visible depth span should be calculated
    float &flNearPlane,                             // If fComputeClipPlanes, returns the near plane.
    float &flFarPlane,                              // If fComputeClipPlanes, returns the far plane.
    __inout bool &fRenderRequired                   // Returns true if a render pass is required.
    )
{
    HRESULT hr = S_OK;

    m_pViewTransform = pViewTransform;
    m_pLightData = pLightData;
    m_fComputeClipPlanes = fComputeClipPlanes;
    m_depthSpan[0] = flNearPlane;
    m_depthSpan[1] = flFarPlane;
    m_fRenderRequired = false;

    IFC(m_pGraphIterator->Walk(pRoot, this));

    flNearPlane = m_depthSpan[0];
    flFarPlane = m_depthSpan[1];
    fRenderRequired |= m_fRenderRequired;

Cleanup:
    Assert(!SUCCEEDED(hr) || (m_transformStack.IsEmpty()));
    m_transformStack.Clear();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CPrerender3DContext::dtor
//
//----------------------------------------------------------------------------

CPrerender3DContext::~CPrerender3DContext()
{
    delete m_pGraphIterator;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CPrerender3DContext::PreSubgraph (IGraphIteratorSink interface)
//
//  Synopsis:
//     PreSubgraph is called by the graph walker when a visual sub-graph is
//     entered. When leaving the sub-graph of a visual the corresponding method
//     PostSubgraph is invoked.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT
CPrerender3DContext::PreSubgraph(
    __out_ecount(1) BOOL* pfVisitChildren
    )
{
    HRESULT hr = S_OK;
    *pfVisitChildren = TRUE;

    Assert(m_pGraphIterator != NULL);

    CMilVisual3D* pNode = DYNCAST(CMilVisual3D, m_pGraphIterator->CurrentNode());
    Assert(pNode != NULL);

    if (pNode->m_pTransform != NULL) 
    {
        CMILMatrix transform;
        IFC(pNode->m_pTransform->GetRealization(&transform));
        IFC(m_transformStack.Push(&transform));
    }

    if (pNode->m_pContent)
    {
        m_fRenderRequired = true;
        
        CMILMatrix worldTransform;
        m_transformStack.Top(&worldTransform);

        CPrerenderWalker modelPrerenderWalker;
        IFC(modelPrerenderWalker.RenderLightsAndPossiblyComputeDepthSpan(
            pNode->m_pContent,
            &worldTransform,
            *m_pViewTransform,
            m_pLightData,
            m_fComputeClipPlanes
            ));
        
        if (m_fComputeClipPlanes)
        {
            const float *span = modelPrerenderWalker.GetSpan();

            // Update our near plane if we found a closer model.
            if (span[0] < m_depthSpan[0])
            {
                m_depthSpan[0] = span[0];
            }

            // Update our far plane if we found a further model.
            if (span[1] > m_depthSpan[1])
            {
                m_depthSpan[1] = span[1];
            }
        }

    }

Cleanup:  
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CPrerender3DContext::PostSubgraph (IGraphIteratorSink interface)
//
//  Synopsis:
//     PreSubgraph is called by the graph walker when a visual sub-graph is
//     entered. When leaving the sub-graph of a visual the corresponding method
//     PostSubgraph is invoked.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT
CPrerender3DContext::PostSubgraph()
{
    HRESULT hr = S_OK;

    Assert(m_pGraphIterator != NULL);

    const CMilVisual3D* pNode = DYNCAST(CMilVisual3D, m_pGraphIterator->CurrentNode());
    Assert(pNode);

    if (pNode->m_pTransform != NULL) 
    {
        m_transformStack.Pop();
    }

    RRETURN(hr);
}


