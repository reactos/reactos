// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Module Name:
//
//    modelwalker.h
//    
//---------------------------------------------------------------------------------

// Meter declarations -------------------------------------------------------------

MtExtern(CModelWalker);
MtExtern(CModelIterator);

//---------------------------------------------------------------------------------
// class CModelWalker
//---------------------------------------------------------------------------------

class CModelWalker
{
    friend class CResourceFactory;

public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CModelIterator));

    CModelWalker();
    ~CModelWalker() { }

    VOID BeginWalk(__in_ecount(1) CMilModel3DDuce *pRoot);
    VOID EndWalk();

    HRESULT GotoFirstChild(__deref_out_ecount_opt(1) CMilModel3DDuce** ppFirstChild);
    HRESULT GotoSibling(__deref_out_ecount_opt(1) CMilModel3DDuce** ppSibling);
    HRESULT GotoParent(__deref_out_ecount_opt(1) CMilModel3DGroupDuce** ppParent);

    __out_ecount_opt(1) CMilModel3DDuce* CurrentNode()
    {
        Assert(m_uCurrentDepth != c_uUninitializedDepth);
        return m_pCurrentNode;
    }

private:
    // UINT_MAX is perfectly fine because the managed API only
    // supports INT_MAX children
    static const UINT c_uRootIndex = UINT_MAX;

    // It's possible that we could have UINT_MAX Model3DGroups linked 
    // together which would then make UINT_MAX a poor choice here. 
    // However, this scenario is highly unlikely and it'll just cause 
    // the walker to be confused.
    static const UINT c_uUninitializedDepth = UINT_MAX;

    struct CFrame
    {
        CMilModel3DGroupDuce* m_pParent;            // The top stack contains the parent of m_pCurrentParent.
        UINT m_uChildIndex;                         // This is the index of m_pCurrentParent in his parents child array.
    };

    UINT                    m_uCurrentDepth;        // Current distance from the root node.
    CMilModel3DDuce*        m_pCurrentNode;         // Current node.
    CMilModel3DGroupDuce*   m_pCurrentParent;       // Current node's parent.
    UINT                    m_uCurrentChildIndex;   // Index of the current node in its parent's child array.
    
    CWatermarkStack<
        CFrame,
        64 /* MinCapacity */,
        2 /* GrowFactor */,
        10 /* TrimCount */
        >         
        m_stack;                                     // Stack of child indices and parent nodes.
};

//---------------------------------------------------------------------------------
//  CModelIterator
//    
//     The CModelIterator uses the model walker to walk a model tree. For each node
//     the iterator calls PreSubgraph and PostSubgraph on the IModelIteratorSink interface.  
//     The interface can control if the sub-graph below is skipped by returning TRUE 
//     for fShouldVisitChildren when called for PreSubgraph.
//---------------------------------------------------------------------------------

class IModelIteratorSink
{
public:
    virtual HRESULT PreSubgraph(
        __in_ecount(1) CMilModel3DDuce *pModel,
        __out_ecount(1) bool *pfVisitChildren
        ) = 0;
    virtual HRESULT PostSubgraph(
        __in_ecount(1) CMilModel3DDuce *pModel
        ) = 0;
};

class CModelIterator
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CModelIterator));

    CModelIterator() {}
    HRESULT Walk(
        __in_ecount(1) CMilModel3DDuce *pRoot,
        __in_ecount(1) IModelIteratorSink *pSink
        );

private:
    CModelWalker m_modelWalker;
};


