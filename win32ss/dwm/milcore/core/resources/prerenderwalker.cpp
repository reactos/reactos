// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Implementation of pre-render walker that computes a depth span
//      for scene (if necessary) and also adds the scene lights to the
//      context state.
//
//      The depth span of the scene is the range of depth values for
//      all of the visible rendered materials.  This class computes a
//      *CONSERVATIVE* estimate of the depth span.
//
//      Note that the depth span uses POSTIVE DEPTH, so is the
//      negative of z values in camera space.
//
//      It is conservative in a couple of ways.
//
//      1. The computation is based on the depth span of individual
//         geometric models (e.g. the MeshGeometry3D)
//      2. This ignores visibility based on occlusion and clipping.
//         The only visibility calculation is ignore geometric primitives
//         that are ENTIRELY behind the camera
//
//      The computation is
//         D = final depth span of scene
//         F = (0,FLT_MAX) = front of camera span
//         M1, M2 = spans of individual geometric primitives
//
//         D = Union( Intersection(M1,F), Intersection(M2,F), ... Intersection(Mn,F) )
//
//------------------------------------------------------------------------

#include "precomp.hpp"

//---------------------------------------------------------------------------------
//  Meter declarations
//---------------------------------------------------------------------------------

MtDefine(CPrerenderWalker, MILRender, "CPrerenderWalker");
#define NULL_HANDLE NULL

//+-----------------------------------------------------------------------
//
//  Member:    CPrerenderWalker::ctor
//
//------------------------------------------------------------------------
CPrerenderWalker::CPrerenderWalker()
{
    m_depthSpan[0] = +FLT_MAX;
    m_depthSpan[1] = -FLT_MAX;
}

//+-----------------------------------------------------------------------
//
//  Member:    CPrerenderWalker::RenderLightsAndPossiblyComputeDepthSpan
//
//  Synopsis:  Compute depth span of scene given initial transform
//             into camera space, where depth is negative z value.
//             Places depth span into member variables accessable
//             using GetSpan().  Span returned will always inside
//             [0,FLT_MAX].
//
//             Also transforms lights into world space and adds them
//             to the context state.
//
//  Returns:   Success if the computation completed without errors
//
//------------------------------------------------------------------------
HRESULT CPrerenderWalker::RenderLightsAndPossiblyComputeDepthSpan(
    __in_ecount(1) CMilModel3DDuce *pRoot,      // Model to compute depth span
    __in_ecount_opt(1) const CMILMatrix* pWorldTransform,
    __in_ecount(1) const CMILMatrix &viewTransform,            // Initial transform for model into camera space
    __in_ecount(1) CMILLightData *pLightData,
    bool fComputeDepthSpan
    )
{
    HRESULT hr = S_OK;

    m_pLightData = pLightData;

    // Initialize the stack
    m_transformStack.Clear();
    m_transformStack.Push(&viewTransform);
    if (pWorldTransform)
    {
        m_transformStack.Push(pWorldTransform);
    }

    m_needDepthSpan = fComputeDepthSpan;

    // run the iterator
    IFC(m_iterator.Walk(pRoot, this));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CPrerenderWalker::PreSubgraph
//
//  Synopsis:  Overrides IModelIteratorSink::PreSubgraph
//             PreSubgraph is called before the sub-graph of a node is
//             visited. With the output argument fVisitChildren the
//             implementor can control if the sub-graph of this node
//             should be visited at all.
//
//  Returns:   Success if computation completed without errors
//
//------------------------------------------------------------------------
HRESULT
CPrerenderWalker::PreSubgraph(
    __in_ecount(1) CMilModel3DDuce *pModel,
    __out_ecount(1) bool *pfVisitChildren
    )
{
    HRESULT hr = S_OK;

    *pfVisitChildren = TRUE;

    // Add transform for this node to the stack.
    CMilTransform3DDuce *pTransform = pModel->GetTransform();
    if (pTransform)
    {
        CMILMatrix matrix;
        IFC(pTransform->GetRealization(&matrix));
        IFC(m_transformStack.Push(&matrix));
    }

    if (m_needDepthSpan)
    {
        IFC(AddDepthSpan(pModel));
    }

    {
        CMILMatrix transform;
        m_transformStack.Top(&transform);

        IFC(pModel->PreRender(this, &transform));
    }

Cleanup:
    // Note that in case of a failure the graph walker will stop immediately. More importantly
    // there is nothing that is equivalent to the stack unwinding in the recursive case.
    // So cleaning out the stacks has to happen in a different place.
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CPrerenderWalker::AddDepthSpan
//
//  Synopsis:  Adds the depth spanned by a scene graph node to the
//             full scene interval being computed by the walker.
//
//  Returns:   Success if computation completed without errors
//
//------------------------------------------------------------------------
HRESULT
CPrerenderWalker::AddDepthSpan(
    __in_ecount(1) CMilModel3DDuce *pModel  // Resource for which to add depth span to scene depth span.
    )
{
    HRESULT hr = S_OK;

    // Start wtih empty interval
    float zmin = +FLT_MAX;
    float zmax = -FLT_MAX;

    CMILMatrix transform;
    m_transformStack.Top(&transform);
    IFC(pModel->GetDepthSpan(&transform, zmin, zmax));

    if (m_depthSpan[0] > zmin)
    {
        m_depthSpan[0] = zmin;
    }

    if (m_depthSpan[1] < zmax)
    {
        m_depthSpan[1] = zmax;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CPrerenderWalker::PostSubgraph
//
//  Synopsis:  Overrides IGraphIteratorSink::PostSubgraph
//             PostSubgraph is called after the sub-graph of a node
//             was visited.
//
//  Returns:   Success upon completion without errors
//
//------------------------------------------------------------------------
HRESULT
CPrerenderWalker::PostSubgraph(__in_ecount(1) CMilModel3DDuce *pModel)
{
    if (pModel->GetTransform())
    {
        m_transformStack.Pop();
    }

    return S_OK;
}

