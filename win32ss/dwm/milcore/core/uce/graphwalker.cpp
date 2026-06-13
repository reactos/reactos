// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Module Name:
//
//    graphwalker.cpp
//
//---------------------------------------------------------------------------------

#include "precomp.hpp"

//---------------------------------------------------------------------------------
//  Meter declarations
//---------------------------------------------------------------------------------

MtDefine(CGraphIterator, Mem, "CGraphIterator");
#define NULL_HANDLE NULL

//---------------------------------------------------------------------------------
//  CGraphWalker::ctor
//
//  Initializes the graph walker.
//---------------------------------------------------------------------------------

CGraphWalker::CGraphWalker(Direction dir)
{
    m_walkDirection = dir;
    Initialize();
}

//---------------------------------------------------------------------------------
//  CGraphWalker::Initialize
//
//  Initializes the current graph walker.
//  Note that this doesn't setup the graph to be walked, just zeros members
//---------------------------------------------------------------------------------

VOID
CGraphWalker::Initialize()
{
#ifdef DBG
    m_walkable = false;
#endif
    m_currentDepth = 0;
    m_pCurrentNode = NULL;
    m_pCurrentParent = NULL;
    m_stack.Clear();
}

//---------------------------------------------------------------------------------
//  CGraphWalker::BeginWalk
//
//  BeginWalk; Initializes the graph walker for another walk. Note that before the
//  GraphWalker can be used again, EndWalk must be called.
//---------------------------------------------------------------------------------

VOID
CGraphWalker::BeginWalk(IGraphNode* pRoot)
{
    Assert(pRoot != NULL);
    Assert(m_pCurrentNode == NULL); // Indicates that the graphwalker was correctly intialized
                                    // and that the matching EndWalk call was performed.
    Assert(m_stack.GetSize() == 0);

    m_currentDepth = 0;
    m_pCurrentNode = pRoot;

    m_pCurrentParent = NULL;
    m_currentChildIndex = 0;

#ifdef DBG
    // Graph is fully initialised and is now "walkable"
    m_walkable = true;
#endif
}

//---------------------------------------------------------------------------------
//  CGraphWalker::EndWalk
//
//     Reinitializes the graph walker into its startup configuration. This method
//     must be called even if the walk is aborted. Otherwise the graph walker can
//     no be reused anymore.
//---------------------------------------------------------------------------------

VOID
CGraphWalker::EndWalk()
{
    Initialize();
    m_stack.Optimize();
}

//---------------------------------------------------------------------------------
//  CGraphWalker::GotoFirstChild
//
//  Returns: S_OK and the first child || S_FALSE and NULL if there aren't any
//           children.
//---------------------------------------------------------------------------------

HRESULT
CGraphWalker::GotoFirstChild(
    __deref_out_opt IGraphNode** ppFirstChild
    )
{
    HRESULT hr = S_OK;

#ifdef DBG
    // A root node needs to be set before the graph walker can be used.    
    Assert(m_walkable);
#endif

    UINT currentChildrenCount = m_pCurrentNode->GetChildrenCount();
    
    if (currentChildrenCount > 0)
    {
        IGraphNode *pFirstChild = NULL;

        //
        // Backup the context information.
        CFrame parentFrame = { m_pCurrentParent, m_currentChildIndex };
        IFC(m_stack.Push(parentFrame));

        //
        // Depending on which direction the graphwalker is set to, the first 
        // child is either left-most or the right-most child.
        
        if (m_walkDirection == RightDirection)
        {                
            m_currentChildIndex = 0;
        }
        else
        {   
            // Don't have to do safe math here because nCurrentChildCount is greater than 0 (inside this if)
            m_currentChildIndex = currentChildrenCount - 1; 
        }

        pFirstChild = m_pCurrentNode->GetChildAt(m_currentChildIndex);                                 


        // Update the remaining graphwalker members.

        m_pCurrentParent = m_pCurrentNode;
        m_pCurrentNode = pFirstChild;
        m_currentDepth++;

        *ppFirstChild = m_pCurrentNode;
    }
    else
    {
        // S_FALSE indicates that there are no children to visit.
        hr = S_FALSE;
        *ppFirstChild = NULL;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------------------------
//  CGraphWalker::GotoSibling
//
//  Returns: S_OK and the next sibling || S_FALSE and NULL if there isn't another
//  sibling.
//
//  Remarks: Currently this method does not skip any invalid siblings. If we decide
//           to ignore invalid siblings in future we need a mean to walk over it.
//---------------------------------------------------------------------------------

HRESULT
CGraphWalker::GotoSibling(__deref_out IGraphNode** ppSibling)
{
    HRESULT hr = S_OK;    

#ifdef DBG
    // A root node needs to be set with BeginWalk() before the graph walker can be used 
    Assert(m_walkable);
#endif

    if ((m_pCurrentParent != NULL) && // Make sure that we are not at the root.
        
        // Check that there is another sibling to the right.
        ((m_walkDirection == RightDirection) &&                               
         (m_currentChildIndex < UINT_MAX) &&                                   
         (m_currentChildIndex + 1 < m_pCurrentParent->GetChildrenCount()) ||  

        // Check that there is another sibling to the left.
         (m_walkDirection == LeftDirection) &&                                 
         (m_currentChildIndex > 0)
         )
         )
    {
        //
        // Move to the next sibling.
        
        UINT nextChildIndex = (m_walkDirection == RightDirection) ? (m_currentChildIndex + 1) : (m_currentChildIndex - 1);
        m_pCurrentNode = m_pCurrentParent->GetChildAt(nextChildIndex);
        Assert(m_pCurrentNode);

        m_currentChildIndex = nextChildIndex;

        *ppSibling = m_pCurrentNode;
    }
    else
    {
        //
        // We visited all siblings.
        
        hr = S_FALSE;
        *ppSibling = NULL;
    }

    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------------------------
//  CGraphWalker::GotoParent
//
//  Returns: S_OK and the parent || S_FALSE and NULL if there is no other parent.
//---------------------------------------------------------------------------------

VOID
CGraphWalker::GotoParent(__deref_out IGraphNode** ppParent)
{
    HRESULT hr = S_OK;

    Assert(ppParent);
    
#ifdef DBG
    // A root node needs to be set with BeginWalk() before the graph walker can be used 
    Assert(m_walkable);
#endif

    if (m_currentDepth == 0)
    {
        // We don't have another parent.
        *ppParent = NULL;
        hr = S_FALSE;

#ifdef DBG
        m_walkable = false;
#endif
    }
    else
    {
        CFrame parentFrame = { NULL, 0 };
        Verify(m_stack.Pop(&parentFrame));

        m_pCurrentNode = m_pCurrentParent;
        m_pCurrentParent = parentFrame.m_pParent;
        m_currentChildIndex = parentFrame.m_childIndex;

        m_currentDepth--;
        *ppParent = m_pCurrentNode;
    }
}

//---------------------------------------------------------------------------------
//  CGraphIterator::Walk
//
//     This method uses the graph walker to walk a scene graph. For each node
//     the iterator calls PreSubgraph and PostSubgraph on the IGraphIteratorInterface
//     and passes in the current node. A node can control if the sub-graph
//     below itself is skipped by returning TRUE for fShouldVisitChildren when
//     called for PreSubgraph.
//
//  Warning: The graph iterator does not support continuation after a failure. It
//     will automatically reset its state. To reuse it a new root node has to be
//     attached.
//---------------------------------------------------------------------------------

HRESULT
CGraphIterator::Walk(
    IGraphNode* pRoot,
    IGraphIteratorSink* pSink)
{
    Assert(pRoot != NULL);
    Assert(pSink);

    HRESULT hr = S_OK;

    m_graphWalker.BeginWalk(pRoot);
    IGraphNode* pCurrent = m_graphWalker.CurrentNode();
    Assert(pCurrent);

    // outer-loop:
    for (;;)
    {
        // Outer-loop:
        //      The purpose of the outer loop is to check if the current
        //      node has children, if it does, it will proceed with the
        //      first child.

        BOOL fShouldVisitChildren;

        // A flag to tell us to skip postgraph and not to reset the visited flag.
        bool skipWalk = false;
        

        // If a loop exists, then we skip entering this node again (and its subgraph).
        // So we skip it pre and post subgraph.
        if (pCurrent->EnterNode())
        {
            IFC(pSink->PreSubgraph(OUT &fShouldVisitChildren));

            // If fShouldVisitChildren is set to FALSE we skip the subgraph of pCurrent.
            if (fShouldVisitChildren)
            {
                IGraphNode* pChild = NULL;
                IFC(m_graphWalker.GotoFirstChild(&pChild));

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
        
        for (;;)
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
                IFC(pSink->PostSubgraph()); 
            }
            else
            {
                skipWalk = false;
            }

            pCurrent->LeaveNode();


            IGraphNode* pSibling = NULL;
            IFC(m_graphWalker.GotoSibling(&pSibling));
            if (pSibling != NULL)
            {
                pCurrent = pSibling;
                break;
            }
            else
            {
                IGraphNode* pParent = NULL;
                m_graphWalker.GotoParent(&pParent);

                if (pParent == NULL)
                {
                    goto Cleanup;
                }
                pCurrent = pParent;
            }
        }
    }

Cleanup:
    // The graph walker returns S_FALSE if it can't execute one of the Goto methods.
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
            if (!(pCurrent->CanEnterNode()))
            {
                pCurrent->LeaveNode();
            }

            // Walk up the parent chain.
            m_graphWalker.GotoParent(&pCurrent);
        }
    }

    m_graphWalker.EndWalk();

    RRETURN(hr);
}


