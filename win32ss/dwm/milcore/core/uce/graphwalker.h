// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Module Name:
//
//    graphwalker.h
//
//-----------------------------------------------------------------------------

#pragma once

// Forward declarations -----------------------------------------------------------

class CMilSlaveHandleTable;

// Meter declarations -------------------------------------------------------------

MtExtern(CGraphIterator);

//---------------------------------------------------------------------------------
// interface IGraphNode
//---------------------------------------------------------------------------------

class IGraphNode
{
public:
    // Returns the number of children.
    virtual UINT GetChildrenCount() const = 0; 

    // Returns the child at the specified index. If the index is out of range,
    // NULL must be returned.
    virtual IGraphNode* GetChildAt(UINT index) = 0;

    // Used for loop detection.
    virtual bool EnterNode() = 0;
    virtual void LeaveNode() = 0;
    virtual bool CanEnterNode() const = 0;
};   

//---------------------------------------------------------------------------------
// class CGraphWalker
//
// Description:
//     CGraphWalker is an iterative walker for walking the scene graph. It can 
//     walk forward and backwards. Depending on the walking direction the first 
//     child is either the left-most or the right-most child of a node.
//---------------------------------------------------------------------------------

class CGraphWalker
{
public:
    enum Direction
    {
        LeftDirection,
        RightDirection
    };

private:
    friend class CGraphIterator;

    CGraphWalker(Direction dir);

    VOID BeginWalk(IGraphNode* pRoot);    
    VOID EndWalk();

    HRESULT GotoFirstChild(__deref_out_opt IGraphNode** ppFirstChild);
    HRESULT GotoSibling(__deref_out IGraphNode** ppSibling);
    VOID GotoParent(__deref_out IGraphNode** ppParent);

    IGraphNode* CurrentNode() 
    { 
#ifdef DBG
        Assert(m_walkable); 
#endif
        return m_pCurrentNode; 
    }

    IGraphNode* CurrentParent() 
    { 
#ifdef DBG
        Assert(m_walkable); 
#endif
        return m_pCurrentParent; 
    }

    VOID Initialize();

    struct CFrame
    {
        IGraphNode* m_pParent;        // The top stack contains the parent of m_pCurrentParent.
        UINT m_childIndex;           // This is the index of m_pCurrentParent in his parents child array.
    };

    bool       m_walkable;            // Specifies whether graph is correctly setup and consistent with 
                                      // being "walkable"

    UINT       m_currentDepth;        // Current distance from the root node.
    
    IGraphNode *m_pCurrentNode;       // Current node.
    
    IGraphNode *m_pCurrentParent;     // Current node's parent.
    
    UINT       m_currentChildIndex;  // Index of the current node in its parent's child array.
    
    Direction  m_walkDirection;       // The direction the walker is walking. Depending on the value 
                                      // of this field the first child is either the right-most
                                      // or the left-most child of a node.
    CWatermarkStack<
        CFrame, 
        64 /* MinCapacity */, 
        2 /* GrowFactor */, 
        10/* TrimCount */
        >  
        m_stack;                      // Stack of child indices and parent nodes.
};

//---------------------------------------------------------------------------------
//  CGraphIterator
//    
//     The CGraphIterator uses the graph walker to walk a scene graph. For each node
//     the iterator calls PreSubgraph and PostSubgraph on the IGraphItertorInterface
//     and passes in the current node. A node can control if the sub-graph
//     below itself is skipped by returning TRUE for fShouldVisitChildren when
//     called for PreSubgraph.
//---------------------------------------------------------------------------------

class IGraphIteratorSink
{
public:
    virtual HRESULT PreSubgraph(OUT BOOL* pfVisitChildren) = 0;
    virtual HRESULT PostSubgraph() = 0;
};

class CGraphIterator
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CGraphIterator));
    CGraphIterator() : m_graphWalker(CGraphWalker::RightDirection) {}
    CGraphIterator(CGraphWalker::Direction dir) : m_graphWalker(dir) {}

    HRESULT Walk(IGraphNode* pRoot, IGraphIteratorSink* pSink);
    IGraphNode* CurrentParent() { return m_graphWalker.CurrentParent(); }
    IGraphNode* CurrentNode() { return m_graphWalker.CurrentNode(); }

private:
    CGraphWalker m_graphWalker;
};

