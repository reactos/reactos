//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispinterior.cxx
//
//  Contents:   Base class for interior (non-leaf) display nodes.
//
//  Classes:    CDispInteriorNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPINTERIOR_HXX_
#define X_DISPINTERIOR_HXX_
#include "dispinterior.hxx"
#endif

#ifndef X_DISPBALANCE_HXX_
#define X_DISPBALANCE_HXX_
#include "dispbalance.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_SAVEDISPCONTEXT_HXX_
#define X_SAVEDISPCONTEXT_HXX_
#include "savedispcontext.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

DeclareTag(tagRecursiveVerify, "Display: Recursive verify", "Recursively verify the display tree")


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::~CDispInteriorNode
//              
//  Synopsis:   Destruct this node, as well as any children marked for
//              destruction.
//              
//----------------------------------------------------------------------------


CDispInteriorNode::~CDispInteriorNode() 
{
    // delete children
    CDispNode* pChild = _pFirstChildNode;
    while (pChild != NULL)
    {
        CDispNode* pNextChild = pChild->_pNextSiblingNode;
        // delete child if it is marked for destruction OR it is unowned
        if (!pChild->_flags.MaskedEquals(
            CDispFlags::s_destructOrOwned, CDispFlags::s_owned))
        {
#if DBG==1
            // make things tidy for debugging checks in destructor
            pChild->_flags = CDispFlags::s_debugDestruct;
            pChild->_pParentNode = NULL;
            pChild->_pPreviousSiblingNode = NULL;
            pChild->_pNextSiblingNode = NULL;
#endif
            delete pChild;
        }
        else
        {
            // unlink child from parent list
            pChild->_pParentNode = NULL;
            pChild->_pPreviousSiblingNode = NULL;
            pChild->_pNextSiblingNode = NULL;
        }
        pChild = pNextChild;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::DestroyChildren
//              
//  Synopsis:   Delete all children marked for destruction, then fix up this
//              node and its remaining children.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::DestroyChildren()
{
    ClearFlag(CDispFlags::s_destructChildren);
    
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL; /* */ )
    {
        // find first destructed child
        while (pChild != NULL && !pChild->IsSet(CDispFlags::s_destruct))
        {
            pChild = pChild->_pNextSiblingNode;
        }
        
        // no more children to destruct?
        if (pChild == NULL)
        {
            break;
        }
        
        CDispNode* pFirstDestructChild = pChild;
        
        // find node after last destructed child
        while (pChild != NULL &&
               pChild->IsSet(CDispFlags::s_destruct))
        {
            pChild = pChild->_pNextSiblingNode;
        }
        
        // fix up parent and siblings
        CDispNode* pPreviousSibling = pFirstDestructChild->_pPreviousSiblingNode;
        if (pPreviousSibling == NULL)
        {
            _pFirstChildNode = pChild;
        }
        else
        {
            pPreviousSibling->_pNextSiblingNode = pChild;
        }
        if (pChild == NULL)
        {
            _pLastChildNode = pPreviousSibling;
        }
        else
        {
            pChild->_pPreviousSiblingNode = pPreviousSibling;
        }
        
        // now delete all children between pFirstDestructedChild
        // and pChild (but not pChild)
        while (pFirstDestructChild != pChild)
        {
            CDispNode* pNextSibling = pFirstDestructChild->_pNextSiblingNode;
#if DBG==1
            // make things tidy for debugging checks in destructor
            pFirstDestructChild->_flags = CDispFlags::s_debugDestruct;
            pFirstDestructChild->_pParentNode = NULL;
            pFirstDestructChild->_pPreviousSiblingNode = NULL;
            pFirstDestructChild->_pNextSiblingNode = NULL;
#endif
            delete pFirstDestructChild;
            pFirstDestructChild = pNextSibling;
        }
    }
    
#if DBG==1
    VerifyChildrenCount();
#endif
}

                    
//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::GetFirstChildNode
//              
//  Synopsis:   Return the leftmost descendent of this node in the tree which
//              is not marked skipNode.
//              
//  Arguments:  none
//              
//  Returns:    Leftmost node or NULL if there aren't any non-skip children.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispNode*
CDispInteriorNode::GetFirstChildNode() const
{
    CDispNode* pChild;
    for (pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        if (!pChild->IsSet(CDispFlags::s_destruct))
        {
            if (!pChild->IsBalanceNode())
                break;
            
            // get leftmost child of balance node
            CDispNode* pLeftChild =
                DYNCAST(CDispBalanceNode, pChild)->GetFirstChildNode();
            if (pLeftChild != NULL)
            {
                pChild = pLeftChild;
                break;
            }
        }
    }
    return pChild;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::GetFirstChildNodeInLayer
//              
//  Synopsis:   Get the first child node in the specified layer.
//              
//  Arguments:  layer       which layer
//              
//  Returns:    the first child node in the specified layer, or NULL if there
//              are no children in that layer
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispNode*
CDispInteriorNode::GetFirstChildNodeInLayer(DISPNODELAYER layer) const
{
    CDispNode* pChild;
    DISPNODELAYER childLayer;
    
    if (layer == DISPNODELAYER_POSITIVEZ)
    {
        // scan backwards through positive z layer
        pChild = _pLastChildNode;
        CDispNode* pLastChild = NULL;
        while (pChild != NULL)
        {
            childLayer = pChild->GetLayerType();
            
            if (childLayer != DISPNODELAYER_POSITIVEZ)
                break;
            
            if (!pChild->IsSet(CDispFlags::s_destruct))
                pLastChild = pChild;
            
            pChild = pChild->_pPreviousSiblingNode;
        }
        
        while (pLastChild != NULL)
        {
            Assert(!pLastChild->IsSet(CDispFlags::s_destruct));
            if (pLastChild->IsBalanceNode())
            {
                CDispNode* pFirstChild =
                    DYNCAST(CDispBalanceNode,pLastChild)->GetFirstChildNode();
                if (pFirstChild != NULL)
                {
                    return pFirstChild;
                }
                do
                {
                    pLastChild = pLastChild->_pNextSiblingNode;
                }
                while (pLastChild != NULL &&
                       pLastChild->IsSet(CDispFlags::s_destruct));
            }
            else
                return pLastChild;
        }
    }
    
    else
    {
        // scan forwards through layers
        pChild = _pFirstChildNode;
        while (pChild != NULL)
        {
            childLayer = pChild->GetLayerType();
            
            // no children in this layer
            if (childLayer > layer)
            {
                return NULL;
            }
            
            // non-destructed nodes only
            if (childLayer == layer &&
                !pChild->IsSet(CDispFlags::s_destruct))
            {
                if (pChild->IsBalanceNode())
                {
                    CDispNode* pFirstChild = 
                        DYNCAST(CDispBalanceNode,pChild)->GetFirstChildNode();
                    if (pFirstChild != NULL)
                    {
                        return pFirstChild;
                    }
                }
                else
                    return pChild;
            }
            
            pChild = pChild->_pNextSiblingNode;
        }
    }
    
    return NULL;    
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::GetLastChildNode
//              
//  Synopsis:   Return the rightmost descendent of this node in the tree which
//              is not marked skipNode.
//              
//  Arguments:  none
//              
//  Returns:    Rightmost node or NULL if there aren't any non-skip children.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispNode*
CDispInteriorNode::GetLastChildNode() const
{
    CDispNode* pChild;
    for (pChild = _pLastChildNode;
         pChild != NULL;
         pChild = pChild->_pPreviousSiblingNode)
    {
        if (!pChild->IsSet(CDispFlags::s_destruct))
        {
            if (!pChild->IsBalanceNode())
                break;
            
            // get rightmost child of balance node
            CDispNode* pRightChild =
                DYNCAST(CDispBalanceNode, pChild)->GetLastChildNode();
            if (pRightChild != NULL)
            {
                pChild = pRightChild;
                break;
            }
        }
    }
    return pChild;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::GetLastChildNodeInLayer
//              
//  Synopsis:   Get the last child node in the specified layer.
//              
//  Arguments:  layer       which layer
//              
//  Returns:    the last child node in the specified layer, or NULL if there
//              are no children in that layer
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispNode*
CDispInteriorNode::GetLastChildNodeInLayer(DISPNODELAYER layer) const
{
    CDispNode* pChild;
    DISPNODELAYER childLayer;
    
    if (layer <= DISPNODELAYER_NEGATIVEZ)
    {
        // scan forwards through layers
        pChild = _pFirstChildNode;
        CDispNode* pLastChild = NULL;
        while (pChild != NULL)
        {
            childLayer = pChild->GetLayerType();
            
            if (childLayer > layer)
                break;
            
            if (!pChild->IsSet(CDispFlags::s_destruct))
                pLastChild = pChild;
            
            pChild = pChild->_pNextSiblingNode;
        }
        
        while (pLastChild != NULL)
        {
            Assert(!pLastChild->IsSet(CDispFlags::s_destruct));
            if (pLastChild->IsBalanceNode())
            {
                CDispNode* pTestChild = 
                    DYNCAST(CDispBalanceNode,pLastChild)->GetLastChildNode();
                if (pTestChild != NULL)
                {
                    return pTestChild;
                }
                do
                {
                    pLastChild = pLastChild->_pPreviousSiblingNode;
                }
                while (pLastChild != NULL &&
                       pLastChild->IsSet(CDispFlags::s_destruct));
            }
            else
                return pLastChild;
        }
    }
    
    else
    {
        // scan backwards through layers
        pChild = _pLastChildNode;
        while (pChild != NULL)
        {
            childLayer = pChild->GetLayerType();
            
            if (childLayer == layer)
            {
                if (!pChild->IsSet(CDispFlags::s_destruct))
                {
                    if (pChild->IsBalanceNode())
                    {
                        CDispNode* pLastChild =
                            DYNCAST(CDispBalanceNode,pChild)->GetLastChildNode();
                        if (pLastChild != NULL)
                        {
                            return pLastChild;
                        }
                    }
                    else
                        return pChild;
                }
            }
            else if (childLayer < layer)
            {
                return NULL;
            }
            
            pChild = pChild->_pPreviousSiblingNode;
        }
    }
    
    return NULL;    
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::GetChildInFlow
//              
//  Synopsis:   Return the child at the index position in the flow layer.
//              
//  Arguments:  index       index of child within the flow layer
//              
//  Returns:    a pointer to the child, or NULL if none was found at the
//              requested index position
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispNode*
CDispInteriorNode::GetChildInFlow(LONG index) const
{
    if (index < 0)
        return NULL;
    
    // find the first flow node
    CDispNode* pChild;
    for (pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        DISPNODELAYER childLayer = pChild->GetLayerType();
        if (childLayer == DISPNODELAYER_FLOW)
            break;
        if (childLayer == DISPNODELAYER_POSITIVEZ)
            return NULL;
    }
    
    // search for flow child at the given index
    for (;
         pChild != NULL && pChild->GetLayerType() == DISPNODELAYER_FLOW;
         pChild = pChild->_pNextSiblingNode)
    {
        if (!pChild->IsSet(CDispFlags::s_destruct))
        {
            if (!pChild->IsBalanceNode())
            {
                if (index-- == 0)
                {
                    return pChild;
                }
            }
            else
            {
                CDispBalanceNode* pBalance = DYNCAST(CDispBalanceNode,pChild);
                LONG cChildren = pBalance->CountChildren();
                if (index < cChildren)
                {
                    return pBalance->GetChildInFlow(index);
                }
                index -= cChildren;
            }
        }
    }
            
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::CountChildren
//              
//  Synopsis:   Count all the non-balance children belonging to this node.
//              
//  Arguments:  none
//              
//  Returns:    Count of all the non-balance children.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

LONG
CDispInteriorNode::CountChildren() const
{
    LONG count = _cChildren;
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        if (!pChild->IsSet(CDispFlags::s_destruct) && pChild->IsBalanceNode())
        {
            // subtract balance node from count, and add its children
            count += DYNCAST(CDispBalanceNode, pChild)->CountChildren() - 1;
        }
    }
    return count;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::InsertFirstChildNode
//
//  Synopsis:   Insert a node as the first child of this node.
//
//  Arguments:  pNewChild       pointer to node to be inserted
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CDispInteriorNode::InsertFirstChildNode(CDispNode* pNewChild)
{
    // skip down a level if we have a balance node
    if (_pFirstChildNode != NULL &&
        _pFirstChildNode->IsBalanceNode() &&
        !_pFirstChildNode->IsSet(CDispFlags::s_destruct) &&
        _pFirstChildNode->GetLayerType() == pNewChild->GetLayerType())
    {
        DYNCAST(CDispBalanceNode, _pFirstChildNode)->
            InsertFirstChildNode(pNewChild);
    }
    
    InsertChildNode(pNewChild, NULL, _pFirstChildNode);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::InsertLastChildNode
//
//  Synopsis:   Insert a node as the first child of this node.
//
//  Arguments:  pNewChild       pointer to node to be inserted
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CDispInteriorNode::InsertLastChildNode(CDispNode* pNewChild)
{
    // skip down a level if we have a synthetic node
    if (_pLastChildNode != NULL &&
        _pLastChildNode->IsBalanceNode() &&
        !_pLastChildNode->IsSet(CDispFlags::s_destruct) &&
        _pLastChildNode->GetLayerType() == pNewChild->GetLayerType())
    {
        DYNCAST(CDispBalanceNode, _pLastChildNode)->
            InsertLastChildNode(pNewChild);
    }
    
    InsertChildNode(pNewChild, _pLastChildNode, NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::InsertChildInFlow
//              
//  Synopsis:   Insert new child at the end of the flow layer list.
//              
//  Arguments:  pNewChild       node to insert
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::InsertChildInFlow(CDispNode* pNewChild)
{
    // skip backwards over positive z children (this must be fast)
    CDispNode* pChild = _pLastChildNode;
    while (pChild != NULL && pChild->GetLayerType() == DISPNODELAYER_POSITIVEZ)
    {
        pChild = pChild->_pPreviousSiblingNode;
    }
    
    // no flow nodes yet
    if (pChild == NULL)
    {
        InsertFirstChildNode(pNewChild);
        return;
    }
    
    // insert inside balance node if appropriate
    if (pChild->GetLayerType() == DISPNODELAYER_FLOW &&
        pChild->IsBalanceNode() &&
        !pChild->IsSet(CDispFlags::s_destruct))
    {
        DYNCAST(CDispBalanceNode,pChild)->InsertLastChildNode(pNewChild);
        return;
    }

    // if the new child is not already in place, insert it
    if (pChild != pNewChild)
    {
        pChild->InsertNextSiblingNode(pNewChild);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::InsertFirstChildInFlow
//              
//  Synopsis:   Insert new child at the beginning of the flow layer list.
//              
//  Arguments:  pNewChild       node to insert
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::InsertFirstChildInFlow(CDispNode* pNewChild)
{
    // find the first flow node
    CDispNode* pChild;
    for (pChild = _pFirstChildNode;
         pChild != NULL && pChild->GetLayerType() < DISPNODELAYER_FLOW;
         pChild = pChild->_pNextSiblingNode)
        ;
    
    // if no flow nodes or positive z nodes, insert at end
    if (pChild == NULL)
    {
        InsertLastChildNode(pNewChild);
        return;
    }
    
    // if this is a flow layer balance node, insert inside balance node
    if (pChild->GetLayerType() == DISPNODELAYER_FLOW && pChild->IsBalanceNode() &&
        !pChild->IsSet(CDispFlags::s_destruct))
    {
        CDispBalanceNode* pBalance = DYNCAST(CDispBalanceNode,pChild);
        CDispNode* pFirstChild = pBalance->GetFirstChildNode();
        if (pFirstChild != NULL)
            pChild = pFirstChild;
    }

    if (pChild != pNewChild)
    {
        pChild->InsertPreviousSiblingNode(pNewChild);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::InsertChildInNegZ
//              
//  Synopsis:   Insert a negative z-ordered child node.
//              
//  Arguments:  pNewChild       child node to insert
//              zOrder          negative z value
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::InsertChildInNegZ(CDispNode* pNewChild, LONG zOrder)
{
    Assert(zOrder < 0);
    
    // search for child after the given z order
    CDispNode* pChild;
    for (pChild = _pFirstChildNode;
         pChild != NULL && pChild->GetLayerType() == DISPNODELAYER_NEGATIVEZ;
         pChild = pChild->_pNextSiblingNode)
    {
        if (pChild->IsSet(CDispFlags::s_destruct))
            continue;
            
        if (!pChild->IsBalanceNode())
        {
            if (pChild != pNewChild)
            {
                LONG zOrderChild = pChild->GetZOrder();
                if (    zOrderChild > zOrder
                    ||  (   zOrderChild == zOrder
                        &&  pChild->CompareZOrder(pNewChild) > 0))
                {
                    pChild->InsertPreviousSiblingNode(pNewChild);
                    return;
                }
            }
        }
        else
        {
            CDispBalanceNode* pBalance = DYNCAST(CDispBalanceNode,pChild);
            CDispNode* pLastChild = pBalance->GetLastChildNode();
            if (pLastChild != NULL)
            {
                LONG zOrderChild = pLastChild->GetZOrder();
                if (    zOrderChild > zOrder
                    ||  (   zOrderChild == zOrder
                        &&  pLastChild->CompareZOrder(pNewChild) > 0))
                {
                    pBalance->InsertChildInNegZ(pNewChild,zOrder);
                    return;
                }
            }
        }
    }
    
    if (pChild == NULL)
    {
        InsertLastChildNode(pNewChild);
        return;
    }
    
    if (pChild != pNewChild)
    {
        pChild->InsertPreviousSiblingNode(pNewChild);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::InsertChildInPosZ
//              
//  Synopsis:   Insert a positive z-ordered child node.
//              
//  Arguments:  pNewChild       child node to insert
//              zOrder          positive z value
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::InsertChildInPosZ(CDispNode* pNewChild, LONG zOrder)
{
    Assert(zOrder >= 0);
    
    // search for child before the given z order
    CDispNode* pChild;
    for (pChild = _pLastChildNode;
         pChild != NULL && pChild->GetLayerType() == DISPNODELAYER_POSITIVEZ;
         pChild = pChild->_pPreviousSiblingNode)
    {
        if (pChild->IsSet(CDispFlags::s_destruct))
            continue;
            
        if (!pChild->IsBalanceNode())
        {
            if (!pChild->IsSet(CDispFlags::s_destruct) && pChild != pNewChild)
            {
                LONG zOrderChild = pChild->GetZOrder();
                if (    zOrderChild < zOrder
                    ||  (   zOrderChild == zOrder
                        &&  pChild->CompareZOrder(pNewChild) < 0))
                {
                    pChild->InsertNextSiblingNode(pNewChild);
                    return;
                }
            }
        }
        else
        {
            CDispBalanceNode* pBalance = DYNCAST(CDispBalanceNode,pChild);
            CDispNode* pFirstChild = pBalance->GetFirstChildNode();
            if (pFirstChild != NULL)
            {
                LONG zOrderChild = pFirstChild->GetZOrder();
                if (    zOrderChild < zOrder
                    ||  (   zOrderChild == zOrder
                        &&  pFirstChild->CompareZOrder(pNewChild) < 0))
                {
                    pBalance->InsertChildInPosZ(pNewChild,zOrder);
                    return;
                }
            }
        }
    }
    
    if (pChild == NULL)
    {
        if (pNewChild != _pFirstChildNode)
        {
            InsertFirstChildNode(pNewChild);
        }
        return;
    }

    if (pChild != pNewChild)
    {
        pChild->InsertNextSiblingNode(pNewChild);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::TakeChildrenFrom
//              
//  Synopsis:   Transfer the children from an interior node to this one.
//              
//  Arguments:  pOldParent          the old parent node
//              pFirstChildToTake   pointer to first child to take from old
//                                  parent node, or NULL to take all children
//              pChildToInsertAfter pointer to child in new parent to insert
//                                  children after, or NULL to insert after
//                                  the last existing child in this parent
//              
//  Notes:      The children are moved in one block, not interleaved among
//              existing layers.  Therefore, it is the responsibility of
//              the caller to make sure layer ordering is preserved before
//              invoking this method.  Assertions are placed in the code below
//              to catch violations.
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::TakeChildrenFrom(
        CDispInteriorNode* pOldParent,
        CDispNode* pFirstChildToTake,
        CDispNode* pChildToInsertAfter)
{
    Assert(pOldParent != NULL);
    
    // find the first and last children to take from the old parent
    if (pFirstChildToTake == NULL)
    {
        pFirstChildToTake = pOldParent->_pFirstChildNode;
        if (pFirstChildToTake == NULL)
            return;
    }
    CDispNode* pLastChild = pOldParent->_pLastChildNode;
    Assert(pLastChild != NULL);
    
    // find previous, next, and parent nodes.  Note that the new parent node
    // may not be the same as "this", because the caller may have passed a
    // child node which is underneath a balance node under this node.
    CDispNode* pPrevious = pChildToInsertAfter;
    CDispNode* pNext;
    CDispInteriorNode* pNewParent;
    
    if (pPrevious == NULL)
    {
        pPrevious = _pLastChildNode;
    }
    
    // modify previous sibling pointer at front of insertion
    pFirstChildToTake->_pPreviousSiblingNode = pPrevious;
    if (pPrevious == NULL)
    {
        pNewParent = this;
        pNext = NULL;
        pNewParent->_pFirstChildNode = pFirstChildToTake;
    }
    else
    {
        pNewParent = pPrevious->_pParentNode;
        pNext = pPrevious->_pNextSiblingNode;
        pPrevious->_pNextSiblingNode = pFirstChildToTake;
    }
    
    // modify old parent's children pointers
    pOldParent->_pLastChildNode = pFirstChildToTake->_pPreviousSiblingNode;
    if (pOldParent->_pLastChildNode == NULL)
    {
        pOldParent->_pFirstChildNode = NULL;
    }
    
    // fix children's parent pointers
    long cMovedChildren = 0;
    for (CDispNode* pChild = pFirstChildToTake;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        pChild->_pParentNode = pNewParent;
        cMovedChildren++;
    }
    
    // fix parent children counts
    pOldParent->_cChildren -= cMovedChildren;
    pNewParent->_cChildren += cMovedChildren;
    
    // modify next sibling pointer at end of insertion
    pLastChild->_pNextSiblingNode = pNext;
    if (pNext == NULL)
    {
        pNewParent->_pLastChildNode = pLastChild;
    }
    else
    {
        pNext->_pPreviousSiblingNode = pLastChild;
    }
    
    // check layering in new parent
    Assert(pFirstChildToTake->_pPreviousSiblingNode == NULL ||
           pFirstChildToTake->_pPreviousSiblingNode->GetLayerType() <=
           pFirstChildToTake->GetLayerType());
    Assert(pLastChild->_pNextSiblingNode == NULL ||
           pLastChild->_pNextSiblingNode->GetLayerType() >=
           pLastChild->GetLayerType());
    
    // both old and new parent now need recalc
    pOldParent->RequestRecalc();
    pNewParent->RequestRecalc();

#if DBG==1
    VerifyTreeCorrectness();
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::PreDraw
//              
//  Synopsis:   Before drawing starts, PreDraw processes the redraw region,
//              subtracting areas that are blocked by opaque or buffered items.
//              PreDraw is finished when the redraw region becomes empty
//              (i.e., an opaque item completely obscures all content below it)
//              
//  Arguments:  pContext    draw context
//              
//  Returns:    TRUE if first opaque node to draw has been found
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispInteriorNode::PreDraw(CDispDrawContext* pContext)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(AllSet(CDispFlags::s_preDrawSelector));
    Assert(pContext->IntersectsRedrawRegion(_rcVisBounds));
    Assert(!IsSet(CDispFlags::s_generalFlagsNotSetInDraw));
    Assert(!IsSet(CDispFlags::s_interiorFlagsNotSetInDraw));

    // we shouldn't be here if this is an opaque node
    Assert(!IsSet(CDispFlags::s_opaqueNode));
    
    // the only node type that should be executing here is CDispBalanceNode,
    // which can't be filtered
    Assert(!IsFiltered());
    
    CDispContext saveContext(*pContext);

    // continue predraw traversal of children, top layers to bottom
    for (CDispNode* pChild = _pLastChildNode;
         pChild != NULL;
         pChild = pChild->_pPreviousSiblingNode)
    {
        // only children which meet our selection criteria
        if (pChild->AllSet(CDispFlags::s_preDrawSelector))
        {
            // if we found the first child to draw, stop further PreDraw calcs
            if (PreDrawChild(pChild, pContext, saveContext))
                return TRUE;
        }
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::PreDrawChild
//              
//  Synopsis:   Call the PreDraw method of the given child, and post-process
//              the results.
//              
//  Arguments:  pChild      child node to predraw
//              pContext    display context
//              saveContext context of this interior node which may be saved
//                          on the context stack (and may differ from the
//                          child's context in pContext)
//              
//  Returns:    TRUE if first child to draw was this child or one of the
//              descendants in its branch
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispInteriorNode::PreDrawChild(
    CDispNode* pChild,
    CDispDrawContext* pContext,
    const CDispContext& saveContext) const
{
    Assert(pChild != NULL);
    Assert(pChild->AllSet(CDispFlags::s_preDrawSelector));
    
    // apply optional user clip
    CApplyUserClip applyUC(pChild, pContext);
    
    // do the clipped visible bounds of this child intersect the
    // redraw region?
    BOOL fPreDrawDone =
        pContext->IntersectsRedrawRegion(pChild->_rcVisBounds) &&
        pChild->PreDraw(pContext);
    
    if (fPreDrawDone)
    {
        // add context information to stack, which will be
        // used by Draw
        if (pChild != _pLastChildNode ||
            pContext->_pFirstDrawNode == NULL)
        {
            pContext->SaveContext(this, saveContext);
            
            // if this child was the first node to be drawn, remember it
            if (pContext->_pFirstDrawNode == NULL)
            {
                pContext->_pFirstDrawNode = pChild;
            }
        }
    }
    
    return fPreDrawDone;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::DrawSelf
//              
//  Synopsis:   Draw this node's children, no clip or offset changes.
//              
//  Arguments:  pContext        draw context
//              pChild          start drawing at this child
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::DrawSelf(CDispDrawContext* pContext, CDispNode* pChild)
{
    // Interesting nodes are visible and in-view
    Assert(AllSet(pContext->_drawSelector));
    Assert(IsSet(CDispFlags::s_savedRedrawRegion) ||
           pContext->IntersectsRedrawRegion(_rcVisBounds));
    Assert(!IsSet(CDispFlags::s_generalFlagsNotSetInDraw));
    Assert(!IsSet(CDispFlags::s_interiorFlagsNotSetInDraw));
    Assert(!IsFiltered());
    
    // draw children, bottom layers to top
    if (pChild == NULL)
    {
        pChild = _pFirstChildNode;
    }
    for (;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        // only children which meet our visibility and inview criteria
        if (pChild->AllSet(pContext->_drawSelector))
            pChild->Draw(pContext, NULL);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::HitTestPoint
//              
//  Synopsis:   Test children for intersection with hit test point.
//              
//  Arguments:  pContext        hit context
//              
//  Returns:    TRUE if intersection found
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispInteriorNode::HitTestPoint(CDispHitContext* pContext) const
{
    Assert(IsSet(CDispFlags::s_visibleBranch));
    Assert(pContext->FuzzyRectIsHit(_rcVisBounds, IsFatHitTest() ));
    
    // search for a hit from foreground layers to background
    for (CDispNode* pChild = _pLastChildNode;
         pChild != NULL;
         pChild = pChild->_pPreviousSiblingNode)
    {
        // NOTE: we can't select on s_inView because when sometimes we hit test
        // on content that is not in view.
        if (pChild->IsSet(CDispFlags::s_visibleBranch))
        {
            // restrict hits to inside user clip rect
            CRect rcChild;
            if (pChild->HasUserClip())
            {
                // BUGBUG (donmarsh) -- the bounds of the user clip will be
                // be used for the fuzzy hit test, which means we could hit
                // outside the user clip rect
                rcChild = pChild->GetUserClip();
                rcChild.OffsetRect(pChild->GetBounds().TopLeft().AsSize());
                rcChild.IntersectRect(pChild->_rcVisBounds);
            }
            else
            {
                rcChild = pChild->_rcVisBounds;
            }

            //
            // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
            // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
            // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
            //            
            if (pContext->FuzzyRectIsHit(rcChild, IsFatHitTest() ) &&
                pChild->HitTestPoint(pContext))
            {
                return TRUE;
            }
        }
    }
    
    return FALSE;
}


CDispScroller *
CDispInteriorNode::HitScrollInset(CPoint *pptHit, DWORD *pdwScrollDir)
{
    CDispScroller * pDispScroller;

    // search for a hit from foreground layers to background
    for (CDispNode* pChild = _pLastChildNode;
         pChild != NULL;
         pChild = pChild->_pPreviousSiblingNode)
    {
        if (pChild->IsInteriorNode() &&
            pChild->AllSet(CDispFlags::s_visibleBranchAndInView) &&
            pChild->_rcVisBounds.Contains(*pptHit))
        {
            pDispScroller = pChild->HitScrollInset(pptHit, pdwScrollDir);
            if (pDispScroller)
            {
                return pDispScroller;
            }
        }
    }
    
    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::PushContext
//              
//  Synopsis:   Get context information for the given child node.
//              
//  Arguments:  pChild          the child node
//              pContextStack   context stack to save context changes in
//              pContext        display context
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::PushContext(
        const CDispNode* pChild,
        CDispContextStack* pContextStack,
        CDispContext* pContext) const
{
    // top of tree should be CDispRoot, which overrides PushContext
    Assert(_pParentNode != NULL);
    
    // context needs to be saved only if this child is not our last, or this
    // will be the first entry in the context stack
    if (pChild != _pLastChildNode || pContextStack->IsEmpty())
    {
        pContextStack->ReserveSlot(this);
        _pParentNode->PushContext(this, pContextStack, pContext);
        pContextStack->PushContext(*pContext, this);
    }
    else
    {
        _pParentNode->PushContext(this, pContextStack, pContext);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::ComputeVisibleBounds
//
//  Synopsis:   Compute visible bounds for an interior node, marking children
//              that determine the edges of these bounds
//              
//  Arguments:  none
//              
//  Returns:    TRUE if visible bounds changed.
//
//----------------------------------------------------------------------------

BOOL
CDispInteriorNode::ComputeVisibleBounds()
{
    // any node that can be filtered should have overridden ComputeVisibleBounds.
    // The only kind of node that doesn't override currently is CDispBalanceNode.
    Assert(!IsFiltered());
    
    CRect rcBounds;
    
    if (_pFirstChildNode == NULL)
    {
        rcBounds.SetRectEmpty();
    }
    
    else
    {
        rcBounds.SetRect(MAXLONG,MAXLONG,MINLONG,MINLONG);
    
        for (CDispNode* pChild = _pFirstChildNode;
            pChild != NULL;
            pChild = pChild->_pNextSiblingNode)
        {
            const CRect& rcChild = pChild->_rcVisBounds;
            if (!rcChild.IsEmpty())
            {
                rcBounds.Union(rcChild);
            }
        }
    }

    if (rcBounds != _rcVisBounds)
    {
        _rcVisBounds = rcBounds;
        return TRUE;
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::CalculateInView
//
//  Synopsis:   Calculate whether this node and its children are in view or not.
//
//  Arguments:  pContext            display context
//              fPositionChanged    TRUE if position changed
//              fNoRedraw           TRUE to suppress redraw (after scrolling)
//              
//  Returns:    TRUE if this node is in view
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispInteriorNode::CalculateInView()
{
    CDispContext context;
    GetTransformContext(&context, COORDSYS_PARENT, COORDSYS_GLOBAL);
    return CalculateInView(&context, FALSE, FALSE);
}


BOOL
CDispInteriorNode::CalculateInView(
    CDispContext* pContext,
    BOOL fPositionChanged,
    BOOL fNoRedraw)
{
    // we shouldn't have to worry about user clip here, because all nodes that
    // provide user clip override CalculateInView
    Assert(!HasUserClip());
    
    BOOL fInView = _rcVisBounds.Intersects(pContext->_rcClip);
    BOOL fWasInView = IsSet(CDispFlags::s_inView);
    
    // calculate in view status of children unless this node is not in view
    // and was not in view
    if (fInView || fWasInView)
    {
        // accelerated way to clear in view status of all children, unless
        // some child needs change notification
        if (!fInView && !IsSet(CDispFlags::s_inViewChange))
        {
            ClearSubtreeFlags(CDispFlags::s_inView);
            return FALSE;
        }
        
        for (CDispNode* pChild = _pFirstChildNode;
             pChild != NULL;
             pChild = pChild->_pNextSiblingNode)
        {
            pChild->CalculateInView(pContext, fPositionChanged, fNoRedraw);
        }
    }
    
    SetBoolean(CDispFlags::s_inView, fInView);
    return fInView;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::RecalcChildren
//              
//  Synopsis:   Recalculate children.
//              
//  Arguments:  fForceRecalc        TRUE to force recalc of this subtree
//              fSuppressInval      TRUE to suppress child invalidation
//              pContext            draw context
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::RecalcChildren(
        BOOL fForceRecalc,
        BOOL fSuppressInval,
        CDispDrawContext* pContext)
{
    CDispFlags childrenFlags(0);
    
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        Assert(fForceRecalc ||
            pChild->IsSet(CDispFlags::s_recalc) ||
            !pChild->IsSet(CDispFlags::s_inval));
        // destruct flag should not be set, because we should have called
        // DestroyChildren before we got here
        Assert(!pChild->IsSet(CDispFlags::s_destruct));
        
        if (fForceRecalc || pChild->IsSet(CDispFlags::s_recalc))
        {
            pChild->Recalc(fForceRecalc, fSuppressInval, pContext);
        }
        Assert(!pChild->IsSet(CDispFlags::s_inval));
        Assert(!pChild->IsSet(CDispFlags::s_recalc));
        Assert(!pChild->IsSet(CDispFlags::s_recalcChildren));
        childrenFlags.Set(pChild->GetFlags());
    }
    
    ComputeVisibleBounds();
    
    // propagate flags from children, and clear recalc flags
    SetFlags(childrenFlags, CDispFlags::s_propagatedAndRecalcAndInval);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::CalcDispInfo
//              
//  Synopsis:   Calculate clipping and positioning info for this node.
//              
//  Arguments:  rcClip          current clip rect
//              pdi             display info structure
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::CalcDispInfo(const CRect& rcClip, CDispInfo* pdi) const
{
    //
    // NOTE: content coordinates == container coordinates, because there are no
    // borders
    // 
    // user clip = current clip, in content coordinates
    //   
    // border clip = current clip INTERSECT container bounds, 
    //   in container coordinates
    //   
    // flow clip = current clip INTERSECT container bounds, 
    //   in content coordinates (same as border clip)
    //   
    
    CDispInfo& di = *pdi;   // notational convenience
    
    // no scrolling or inset
    di._scrollOffset = g_Zero.size;
    di._pInsetOffset = &((CSize&)g_Zero.size);
    
    // content size
    _rcVisBounds.GetSize(&di._sizeContent);
    
    // background size
    di._sizeBackground = di._sizeContent;
    
    // offset to local coordinates
    di._borderOffset = g_Zero.size;
    
    di._rcPositionedClip = rcClip;
    di._rcContainerClip = rcClip;
    
    // flow clip = container clip since there are no borders
    di._contentOffset = g_Zero.size;
    di._rcFlowClip = di._rcBackgroundClip = rcClip;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::ClearSubtreeFlags
//
//  Synopsis:   Clear given flags in this subtree.
//
//  Arguments:  flags       flags to clear
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispInteriorNode::ClearSubtreeFlags(const CDispFlags& flags)
{
    // this routine is optimized to deal only with propagated flag values
    Assert(flags.MaskedEquals(CDispFlags::s_propagatedMask,flags));
    
    ClearFlag(flags);
    
    // process children
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        // only need to clear subtrees whose root has these flags set
        if (pChild->IsSet(flags))
        {
            if (pChild->IsLeafNode())
            {
                pChild->ClearFlag(flags);
            }
            else
            {
                DYNCAST(CDispInteriorNode,pChild)->ClearSubtreeFlags(flags);
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::SetSubtreeFlags
//
//  Synopsis:   Set given flags in this subtree.
//
//  Arguments:  flags       flags to set
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispInteriorNode::SetSubtreeFlags(const CDispFlags& flags)
{
    // only propagated flag values allowed
    // (to be consistent with ClearSubtreeFlags)
    Assert(flags.MaskedEquals(CDispFlags::s_propagatedMask,flags));
    
    SetFlag(flags);
    
    // process children
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        if (pChild->IsLeafNode())
        {
            pChild->SetFlag(flags);
        }
        else
        {
            DYNCAST(CDispInteriorNode,pChild)->SetSubtreeFlags(flags);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::SubtractOpaqueChildren
//              
//  Synopsis:   Subtract bounds of children specified by context selector from
//              the given region.
//              
//  Arguments:  prgn        region to subtract from
//              pContext    display context
//              
//  Returns:    TRUE if the resulting region is not empty
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispInteriorNode::SubtractOpaqueChildren(
        CRegion* prgn,
        CDispContext* pContext)
{
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        // select only opaque visible children
        if (pChild->AllSet(CDispFlags::s_subtractOpaqueSelector))
        {
            if (pChild->IsLeafNode() ||
                pChild->IsSet(CDispFlags::s_opaqueNode))
            {
                CRect rcOpaque;
                pChild->GetBounds(&rcOpaque);
                pContext->Transform(&rcOpaque);
                prgn->Subtract(rcOpaque);
                if (prgn->IsEmpty())
                {
                    return FALSE;
                }
            }
            else
            {
                CDispInteriorNode* pInterior =
                    DYNCAST(CDispInteriorNode, pChild);
                if (!pInterior->SubtractOpaqueChildren(prgn, pContext))
                {
                    return FALSE;
                }
            }
        }
    }
    
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::BalanceTree
//              
//  Synopsis:   Attempt to balance the tree by making sure that one interior
//              node does not have too many children.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::BalanceTree()
{
    // this might take a few passes to get right
    BOOL fContinueBalancing = TRUE;

    while (_cChildren > MAX_CHILDREN_BEFORE_BALANCE && fContinueBalancing)
    {
        fContinueBalancing = FALSE;

        CDispNode* pFirstChild = _pFirstChildNode;
        CDispNode* pNextChild;
    
        CDispFlags nodeKind =
            _pFirstChildNode->GetFlag(CDispFlags::s_balanceGroupSelector);
        LONG cSimilarChildren = 1;
    
        for (CDispNode* pChild = _pFirstChildNode->_pNextSiblingNode;
             pChild != NULL;
             pChild = pNextChild)
        {
            pNextChild = pChild->_pNextSiblingNode;
        
            CDispFlags testKind =
                pChild->GetFlag(CDispFlags::s_balanceGroupSelector);
            BOOL fSimilar = (testKind == nodeKind);
            if (fSimilar)
            {
                cSimilarChildren++;
            }
        
            if (!fSimilar || pNextChild == NULL)
            {
                if (cSimilarChildren >= MIN_CHILDREN_BEFORE_BALANCE)
                {
                    // split into groups having MAX_CHILDREN_AFTER_BALANCE
                    // children
                    CreateBalanceGroups(
                        pFirstChild,
                        (fSimilar) ? NULL : pChild,
                        MAX_CHILDREN_AFTER_BALANCE);
                    fContinueBalancing = TRUE;  // iterate through again
                }
            
                if (pNextChild == NULL)
                    break;
                cSimilarChildren = 1;
                nodeKind = testKind;
                pFirstChild = pChild;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::CreateBalanceGroups
//              
//  Synopsis:   Create groups with the indicated number of children, containg
//              all children between the first and last children.
//              
//  Arguments:  pFirstChild     pointer to first child node to group
//              pStopChild      pointer to first child after group
//              cMaxChildrenInEachGroup    max number of children to put in each
//                              group
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::CreateBalanceGroups(
        CDispNode* pFirstChild,
        CDispNode* pStopChild,
        LONG cMaxChildrenInEachGroup)
{
    Assert(pFirstChild != NULL);
    
    while (pFirstChild != pStopChild &&
           pFirstChild->_pNextSiblingNode != pStopChild)
    {
        // create balance node to reduce this node's number of children
        CDispBalanceNode* pBalance = new CDispBalanceNode();
        if (pBalance == NULL)
            break;

        // move children into group
        CDispNode* pChild = pFirstChild;
        CDispNode* pPreviousChild = pFirstChild->_pPreviousSiblingNode;
        LONG cChildren = 0;
        while (cChildren < cMaxChildrenInEachGroup && pChild != pStopChild)
        {
            pChild->_pParentNode = pBalance;
            cChildren++;
            pChild = pChild->_pNextSiblingNode;
        }
        
        Assert(cChildren > 1);

        // modify balance node's parent
        _cChildren -= cChildren;
        Assert(_cChildren > 1);
        
        // finish this balance node
        CDispNode* pLastChild =
            (pChild == NULL) ? _pLastChildNode : pChild->_pPreviousSiblingNode;
        
        // balance node has same layer as children
        pBalance->SetLayerType(pLastChild->GetLayerType());
        
        pBalance->_pFirstChildNode = pFirstChild;
        pBalance->_pLastChildNode = pLastChild;
        pBalance->_cChildren = cChildren;

        pFirstChild->_pPreviousSiblingNode = NULL;
        pLastChild->_pNextSiblingNode = NULL;

        InsertChildNode(pBalance, pPreviousChild, pChild);

        // don't need to invalidate the newly-inserted balance node
        pBalance->ClearFlag(CDispFlags::s_inval);

        pFirstChild = pChild;
    }
    
#if DBG==1
    VerifyTreeCorrectness();
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::InsertChildNode
//              
//  Synopsis:   Insert the given child node between the indicated sibling nodes,
//              which can be NULL at the beginning or end of the list.
//              
//  Arguments:  pChild              child node to insert
//              pPreviousSibling    previous sibling node (may be NULL if child
//                                  is to be inserted at the beginning)
//              pNextSibling        next sibling node (may be NULL if child
//                                  is to be inserted at the end)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::InsertChildNode(
        CDispNode* pChild,
        CDispNode* pPreviousSibling,
        CDispNode* pNextSibling)
{
    Assert(pChild != NULL);
    Assert(pChild != pPreviousSibling);
    Assert(pChild != pNextSibling);
    Assert(pChild != this);
    
    // is the new child already in the tree?
    if (pChild->_pParentNode != NULL)
    {
        // is it already in the right spot?
        if ((pChild->_pParentNode == this || pChild->GetParentNode() == this) &&
            (pChild->_pPreviousSiblingNode == pPreviousSibling ||
             pChild->GetPreviousSiblingNode() == pPreviousSibling) &&
            (pChild->_pNextSiblingNode == pNextSibling ||
             pChild->GetPreviousSiblingNode() == pNextSibling))
            return;
        
        // extract child from its current location
        pChild->ExtractFromTree();
    }
    
    Assert(pChild->_pParentNode == NULL);
    Assert(pChild->_pPreviousSiblingNode == NULL);
    Assert(pChild->_pNextSiblingNode == NULL);
    
    // change child's parent pointer
    pChild->_pParentNode = this;
    
    // link to previous sibling
    pChild->_pPreviousSiblingNode = pPreviousSibling;
    if (pPreviousSibling == NULL)
    {
        _pFirstChildNode = pChild;
    }
    else
    {
        pPreviousSibling->_pNextSiblingNode = pChild;
    }
    
    // link to next sibling
    pChild->_pNextSiblingNode = pNextSibling;
    if (pNextSibling == NULL)
    {
        _pLastChildNode = pChild;
    }
    else
    {
        pNextSibling->_pPreviousSiblingNode = pChild;
    }

    // if a insertion-aware leaf node, note the change
    if (pChild->IsInsertionAware() &&
        pChild->IsLeafNode())
    {
        pChild->SetFlag(CDispFlags::s_justInserted);
    }
    
    // modify children count
    _cChildren++;
#if DBG==1
    VerifyTreeCorrectness();
#endif
    
    // inval new node and recalc its children
    RequestRecalc();
    pChild->SetFlag(CDispFlags::s_invalAndRecalcChildren);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::SetPositionHasChanged
//              
//  Synopsis:   Set s_positionHasChanged flag on all leaf nodes that have
//              s_positionChange set.  This allows CDispNode::Recalc to
//              properly notify clients who request position change notification.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::SetPositionHasChanged()
{
    Assert(IsSet(CDispFlags::s_positionChange));
    
    SetFlag(CDispFlags::s_recalc);
    
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        if (pChild->IsSet(CDispFlags::s_positionChange))
        {
            if (pChild->IsLeafNode())
            {
                pChild->SetFlag(CDispFlags::s_positionHasChangedAndRecalc);
            }
            else
            {
                CDispInteriorNode* pInterior = DYNCAST(CDispInteriorNode, pChild);
                pInterior->SetPositionHasChanged();
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::GetScrollableContentSize
//              
//  Synopsis:   Calculate the size of scrollable content, excluding child nodes
//              which are marked as not contributing to scroll size calculations.
//              
//  Arguments:  psize       returns the size of scrollable content
//              fAddBorder  if TRUE, take border size into account
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::GetScrollableContentSize(CSize* psize, BOOL fAddBorder) const
{
    Assert(psize != NULL);
    
    BOOL fRightToLeft = IsRightToLeft();
    CRect rcBorderWidths(g_Zero.rc);
    if (fAddBorder && HasBorder())
        GetBorderWidths(&rcBorderWidths);
    
    *psize = g_Zero.size;
    
    // extend size by size of positioned children which contribute to scrollable
    // content area
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        CSize sizeChild;
        CRect rcChild;
        pChild->GetBounds(&rcChild);
        
        // must recurse for interior nodes in the -Z and +Z layers, because
        // they may contain only relatively-positioned content.  For this
        // reason, it is NOT sufficient to use the child's _rcVisBounds.
        if (pChild->IsInteriorNode() &&
            !pChild->IsScroller() &&
            !pChild->IsFiltered() &&
            IsPositionedLayer(pChild->GetLayerType()))
        {
            CDispInteriorNode* pInterior =
                DYNCAST(CDispInteriorNode, pChild);
            pInterior->GetScrollableContentSize(&sizeChild, TRUE);

            // adjust if node did not introduce a new coordinate system
            if (!pInterior->IsContainer())
                rcChild.SetRectEmpty(); // because of addition below
        }
        else
        {
            // relatively-positioned items do not affect the scrollable content size
            sizeChild = pChild->AffectsScrollBounds()
                            ? sizeChild = rcChild.Size()
                            : g_Zero.size;
        }

        if (sizeChild != g_Zero.size)
        {
            if (!fRightToLeft)
            {
                sizeChild.cx += rcBorderWidths.left + rcChild.left;
                if (sizeChild.cx > psize->cx)
                    psize->cx = sizeChild.cx;
            }
            else
            {
                sizeChild.cx += rcBorderWidths.right - rcChild.right;
                if (sizeChild.cx > psize->cx)
                    psize->cx = sizeChild.cx;
            }
            
            sizeChild.cy += rcBorderWidths.top + rcChild.top;
            if (sizeChild.cy > psize->cy)
                psize->cy = sizeChild.cy;
        }
    }
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::VerifyTreeCorrectness
//
//  Synopsis:   Verify the display node integrity
//
//----------------------------------------------------------------------------

void
CDispInteriorNode::VerifyTreeCorrectness() const
{
    // verify the basic node structure
    super::VerifyTreeCorrectness();

    // verify interior flags
    Assert(IsBalanceNode() == (GetNodeType() == DISPNODETYPE_BALANCE));

    // verify first/last child node pointers
    AssertSz((_pFirstChildNode && _pLastChildNode) || 
            (!_pFirstChildNode && !_pLastChildNode), "Inconsistent first/last child nodes");
    AssertSz(!_pFirstChildNode || _pFirstChildNode->_pPreviousSiblingNode == NULL, "Invalid first child node");
    AssertSz(!_pLastChildNode || _pLastChildNode->_pNextSiblingNode == NULL, "Invalid last child node");

    // verify number of children
    VerifyChildrenCount();

    // verify parent-child and sibling relationships
    const CDispNode * pLastChild = NULL;
    const CDispNode * pChild;
    for (pChild = _pFirstChildNode;
         pChild != NULL;
         pLastChild = pChild, pChild = pChild->_pNextSiblingNode)
    {
        if (pLastChild)
        {
            AssertSz(pChild->GetLayerType() >= pLastChild->GetLayerType(), "Invalid layer ordering");
        }
        AssertSz(pChild != this, "Invalid parent-child loop");
        AssertSz(pChild->_pParentNode == this, "Invalid parent-child relationship");
        AssertSz(pChild->_pPreviousSiblingNode == pLastChild, "Invalid previous sibling order");
        AssertSz(pChild->_pNextSiblingNode || pChild == _pLastChildNode, "Invalid last child node");
    }

    // verify each child
    if (IsTagEnabled(tagRecursiveVerify))
    {
        for (pChild = _pFirstChildNode;
             pChild != NULL;
             pChild = pChild->_pNextSiblingNode)
        {
            pChild->VerifyTreeCorrectness();
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::GetMemorySize
//              
//  Synopsis:   Return memory size of the display tree rooted at this node.
//              
//  Arguments:  none
//              
//  Returns:    Memory size of this node and its children.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

size_t
CDispInteriorNode::GetMemorySize() const
{
    size_t size = GetMemorySizeOfThis();
    
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        size += pChild->GetMemorySize();
    }
    
    return size;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::VerifyChildrenCount
//              
//  Synopsis:   Assert that this node's children count accurately reflects
//              its actual number of children.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::VerifyChildrenCount() const
{
    LONG cActualChildren = 0;
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        if (!pChild->IsSet(CDispFlags::s_destruct))
            cActualChildren++;
    }
    Assert(cActualChildren == _cChildren);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispInteriorNode::VerifyFlags
//              
//  Synopsis:   Verify that nodes in this subtree have flags set properly.
//              
//  Arguments:  mask        mask to apply to flags
//              value       value to test after mask has been applied
//              fEqual      TRUE if value must be equal, FALSE if not equal
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispInteriorNode::VerifyFlags(
        const CDispFlags& mask,
        const CDispFlags& value,
        BOOL fEqual) const
{
    AssertSz(_flags.MaskedEquals(mask,value) == fEqual,
             "Display Tree flags are invalid");
    
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        if (pChild->IsLeafNode())
        {
            AssertSz(pChild->_flags.MaskedEquals(mask,value) == fEqual,
                     "Display Tree flags are invalid");
        }
        else
        {
            DYNCAST(CDispInteriorNode,pChild)->VerifyFlags(mask, value, fEqual);
        }
    }
}



#endif
