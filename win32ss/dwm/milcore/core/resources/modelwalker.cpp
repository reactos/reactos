// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Module Name:
//
//    modelwalker.cpp
//
//---------------------------------------------------------------------------------

#include "precomp.hpp"

//---------------------------------------------------------------------------------
//  Meter declarations
//---------------------------------------------------------------------------------

MtDefine(CModelWalker, MILRender, "CModelWalker");
MtDefine(CModelIterator, MILRender, "CModelIterator");

#define NULL_HANDLE NULL

//---------------------------------------------------------------------------------
//  CModelWalker::ctor
//
//  Initializes the model walker.
//---------------------------------------------------------------------------------

CModelWalker::CModelWalker()
{
    m_uCurrentDepth = c_uUninitializedDepth;
    m_pCurrentNode = NULL;
    m_pCurrentParent = NULL;
    m_uCurrentChildIndex = 0;
}

//---------------------------------------------------------------------------------
//  CModelWalker::SetRoot
//
//  Sets a new root node of the model walker.
//---------------------------------------------------------------------------------

VOID
CModelWalker::BeginWalk(__in_ecount(1) CMilModel3DDuce* pRoot)
{
    Assert(m_pCurrentNode == NULL); // Indicates that the graphwalker was correctly intialized
                                    // and that the matching EndWalk call was performed.
    Assert(m_stack.GetSize() == 0);

    m_uCurrentDepth = 0;

    m_pCurrentParent = NULL;
    m_pCurrentNode = pRoot;
    m_uCurrentChildIndex = c_uRootIndex;
}

//---------------------------------------------------------------------------------
//  CModelWalker::EndWalk
//
//     Reinitializes the graph walker into its startup configuration. This method
//     must be called even if the walk is aborted. Otherwise the graph walker can
//     no be reused anymore.
//---------------------------------------------------------------------------------

VOID
CModelWalker::EndWalk()
{
    m_uCurrentDepth = c_uUninitializedDepth;
    m_pCurrentNode = NULL;
    m_pCurrentParent = NULL;
    m_uCurrentChildIndex = 0;
    m_stack.Clear();
    m_stack.Optimize();
}

//---------------------------------------------------------------------------------
//  CModelWalker::GotoFirstChild
//
//  Returns: S_OK and the first child || S_FALSE and NULL if there aren't any
//           children.
//---------------------------------------------------------------------------------

HRESULT
CModelWalker::GotoFirstChild(__deref_out_ecount_opt(1) CMilModel3DDuce** ppFirstChild)
{
    HRESULT hr = S_FALSE;

    Assert(m_uCurrentDepth != c_uUninitializedDepth); // A root node needs to be set before the model walker can be used.

    *ppFirstChild = NULL;

    if (m_pCurrentNode->IsOfType(TYPE_MODEL3DGROUP))
    {
        CMilModel3DGroupDuce *pCurrent = static_cast<CMilModel3DGroupDuce *>(m_pCurrentNode);

        // NOTE: GetChildrenSize returns the buffer size, not the number of children.
        if (pCurrent->m_data.m_cChildren > 0)
        {
            CMilModel3DDuce *pFirstChild = pCurrent->m_data.m_rgpChildren[0];

            IFCNULL(pFirstChild);

            CFrame parentFrame = { m_pCurrentParent, m_uCurrentChildIndex };
            IFC(m_stack.Push(parentFrame));

            m_pCurrentParent = pCurrent;
            m_pCurrentNode = pFirstChild;
            m_uCurrentChildIndex = 0;
            m_uCurrentDepth++;

            *ppFirstChild = m_pCurrentNode;
            hr = S_OK;
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------------------------
//  CModelWalker::GotoSibling
//
//  Returns: S_OK and the next sibling || S_FALSE and NULL if there isn't another
//  sibling.
//
//  Remarks: Currently this method does not skip any invalid siblings. If we decide
//           to ignore invalid siblings in future we need a mean to walk over it.
//---------------------------------------------------------------------------------

HRESULT
CModelWalker::GotoSibling(__deref_out_ecount_opt(1) CMilModel3DDuce** ppSibling)
{
    HRESULT hr = S_FALSE;

    Assert(m_uCurrentDepth != c_uUninitializedDepth); // A root node needs to be set before the model walker can be used.

    *ppSibling = NULL;

    // If the parent is NULL that means we are calling GotoSibling on the root node and the root node doesn't have a sibling.
    if (m_pCurrentParent != NULL)
    {
        UINT uNextChildIndex = m_uCurrentChildIndex + 1;
        UINT nChildrenCount = m_pCurrentParent->m_data.m_cChildren;

        if (uNextChildIndex < nChildrenCount)
        {
            m_pCurrentNode = m_pCurrentParent->m_data.m_rgpChildren[uNextChildIndex];
            IFCNULL(m_pCurrentNode);

            m_uCurrentChildIndex = uNextChildIndex;

            *ppSibling = m_pCurrentNode;
            hr = S_OK;
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------------------------
//  CModelWalker::GotoParent
//
//  Returns: S_OK and the parent || S_FALSE and NULL if there is no other parent.
//---------------------------------------------------------------------------------

HRESULT
CModelWalker::GotoParent(__deref_out_ecount_opt(1) CMilModel3DGroupDuce** ppParent)
{
    HRESULT hr = S_OK;

    Assert(m_uCurrentDepth != c_uUninitializedDepth); // A root node needs to be set before the model walker can be used.

    if (m_uCurrentDepth == 0)
    {
        // We don't have another parent.
        *ppParent = NULL;
        hr = S_FALSE;
        Assert(m_uCurrentChildIndex == c_uRootIndex);
    }
    else
    {
        CFrame parentFrame;
        if (TFAIL(0, m_stack.Pop(&parentFrame))==false)
        {
            // Stack was empty, and shouldn't have been!
            IFC(E_FAIL);
        }
        *ppParent = m_pCurrentParent;
        m_pCurrentNode = m_pCurrentParent;
        m_pCurrentParent = parentFrame.m_pParent;
        m_uCurrentChildIndex = parentFrame.m_uChildIndex;

        m_uCurrentDepth--;
    }
Cleanup:
    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------------------------
//  CModelIterator::Walk
//
//     This method uses the model walker to walk a model tree. For each node
//     the iterator calls PreSubgraph and PostSubgraph on the IModelIteratorSink
//     interface. The interface can control if the sub-graph below is skipped by
//     returning TRUE for fShouldVisitChildren when called for PreSubgraph.
//
//  Warning: The model iterator does not support continuation after a failure. It
//     will automatically reset its state. To reuse it a new root node has to be
//     attached.
//---------------------------------------------------------------------------------

HRESULT
CModelIterator::Walk(
    __in_ecount(1) CMilModel3DDuce *pRoot,
    __in_ecount(1) IModelIteratorSink* pSink
    )
{
    HRESULT hr = S_OK;

    m_modelWalker.BeginWalk(pRoot);
    CMilModel3DDuce* pCurrent = m_modelWalker.CurrentNode();

    // outer-loop:
    while (pCurrent)
    {
        // Outer-loop:
        //      The purpose of the outer loop is to check if the current
        //      node has children, if it does, it will proceed with the
        //      first child.

        bool fShouldVisitChildren;

        // A flag to tell us to skip postgraph and not to reset the visited flag.
        bool skipWalk = false;
        

        // If a loop exists, then we skip entering this node again (and its subgraph).
        // So we skip it pre and post subgraph.
        if (pCurrent->EnterResource())
        {
            IFC(pSink->PreSubgraph(pCurrent, &fShouldVisitChildren));

            // If fShouldVisitChildren is set to FALSE we skip the subgraph of pCurrent.
            if (fShouldVisitChildren)
            {
                CMilModel3DDuce* pChild = NULL;
                IFC(m_modelWalker.GotoFirstChild(&pChild));

                if (pChild != NULL)
                {
                    pCurrent = pChild;
                    continue;
                }
            }
        }
        else
        {
            skipWalk = true;
        }
        
        do
        {
            // Inner-loop:
            //     The purpose of this loop is to first look if the
            //     current node has a sibling. If not it will proceed to the
            //     parent and check if the parent has a sibling, and so on.
            //     This continues till we either find a sibling in which case
            //     we break out of the loop or till we reach the root node, in
            //     which case we are done walking the composition tree.

            // Going to the sibling or up to our parent, means that we are leaving
            // the current node finally. Hence we need to do the post subgraph work
            // here.
            if (!skipWalk)
            {
                IFC(pSink->PostSubgraph(pCurrent));
            }
            else
            {
                skipWalk = false;
            }

            pCurrent->LeaveResource();
            
            CMilModel3DDuce *pSibling = NULL;
            IFC(m_modelWalker.GotoSibling(&pSibling));
            if (pSibling != NULL)
            {
                pCurrent = pSibling;
                break;
            }
            else
            {
                CMilModel3DGroupDuce *pParent = NULL;
                IFC(m_modelWalker.GotoParent(&pParent));
                pCurrent = pParent;
            }

        } while (pCurrent);
    }

Cleanup:
    // The model walker returns S_FALSE if it can't execute one of the Goto methods.
    if (hr == S_FALSE)
    {
        hr = S_OK;
    }

    // If we failed somewhere, then we want to make sure that we reset the state on the node.
    // So we check all the way up its parent chain to make sure that LeaveNode() is called if
    // we had called EnterNode() for that node.
    if (FAILED(hr))
    {
        while (pCurrent != NULL)
        {
            // If we already entered the node, then we need to call LeaveNode() to reset its state.
            if (!(pCurrent->CanEnterResource()))
            {
                pCurrent->LeaveResource();
            }

            // Walk up the parent chain.
            CMilModel3DGroupDuce *pParent = NULL;
            m_modelWalker.GotoParent(&pParent);
            pCurrent = pParent;
        }
    }

    m_modelWalker.EndWalk();

    RRETURN(hr);
}


