//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispnode.cxx
//
//  Contents:   Base class for nodes in display tree.
//
//  Classes:    CDisplayNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPINTERIOR_HXX_
#define X_DISPINTERIOR_HXX_
#include "dispinterior.hxx"
#endif

#ifndef X_DISPLEAF_HXX_
#define X_DISPLEAF_HXX_
#include "displeaf.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPBALANCE_HXX_
#define X_DISPBALANCE_HXX_
#include "dispbalance.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

#ifndef X_SAVEDISPCONTEXT_HXX_
#define X_SAVEDISPCONTEXT_HXX_
#include "savedispcontext.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif


// This constant determines the minimum number of pixels an opaque item has
// to affect in order to qualify for special opaque treatment.
// If this number is too small, regions may get very complex as tiny holes
// are poked in them by small opaque items.  Complex regions take more memory
// and more CPU cycles to process.
const ULONG MINIMUM_OPAQUE_PIXELS = 50*50;


ExternTag(tagHackGDICoords);
MtDefine(DisplayTree, Mem, "Display Tree")

#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::~CDispNode
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------


CDispNode::~CDispNode()
{
    // make sure this is getting called from
    // a legal place (Recalc or Destroy)
    Assert(_flags == CDispFlags::s_debugDestruct);
    Assert(_pParentNode == NULL);
    Assert(_pPreviousSiblingNode == NULL && _pNextSiblingNode == NULL);
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::Destroy
//
//  Synopsis:   Mark this node for destruction.
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::Destroy()
{
    Assert(!IsSet(CDispFlags::s_destruct));

    // if we have a parent, delay destruction until destructor of parent or
    // recalc
    if (_pParentNode != NULL)
    {
        if (!IsSet(CDispFlags::s_inval))
        {
            Invalidate(_rcVisBounds, COORDSYS_PARENT);
        }
        SetFlag(CDispFlags::s_destructAndInval);
        _pParentNode->_cChildren--;
        _pParentNode->SetFlag(CDispFlags::s_destructChildren);
        RequestRecalc();
    }
    else
    {
#if DBG==1
        _flags = CDispFlags::s_debugDestruct;
#endif
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetParentNode
//
//  Synopsis:   Return this node's parent, skipping balance nodes along
//              the way.
//
//  Arguments:  none
//
//  Returns:    Parent node which is not a balance node.
//
//  Notes:
//
//----------------------------------------------------------------------------

CDispInteriorNode*
CDispNode::GetParentNode() const
{
    // find first parent that isn't a balance node
    CDispInteriorNode* pParent = _pParentNode;
    while (pParent != NULL && pParent->IsBalanceNode())
    {
        pParent = pParent->_pParentNode;
    }

    if (pParent && pParent->IsSet(CDispFlags::s_destruct))
        pParent = NULL;

    return pParent;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetPreviousSiblingNode
//
//  Synopsis:   Find previous sibling that isn't a balance node or a
//              destroyed node.
//
//  Arguments:  none
//
//  Returns:    Previous sibling node.
//
//  Notes:
//
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetPreviousSiblingNode() const
{
    CDispNode* pPrevious = _pPreviousSiblingNode;
    CDispInteriorNode* pParent = _pParentNode;

    for (;;)
    {
        // bypass destructed nodes
        while (pPrevious != NULL && pPrevious->IsSet(CDispFlags::s_destruct))
        {
            pPrevious = pPrevious->_pPreviousSiblingNode;
        }

        // found a non-destruct previous sibling
        if (pPrevious != NULL)
        {
            // simple case: it's not a balance node
            if (!pPrevious->IsBalanceNode())
                break;

            // get right-most child of this balance node
            CDispBalanceNode* pBalance = DYNCAST(CDispBalanceNode, pPrevious);
            pPrevious = pBalance->GetLastChildNode();
            if (pPrevious != NULL)
                break;

            // if there were no children, continue scanning at this level
            pPrevious = pBalance->_pPreviousSiblingNode;
        }

        // no previous sibling at this level
        else
        {
            // if we didn't find a previous sibling at this level, and the parent
            // node is a balance node, continue search up one level in the tree
            if (pParent == NULL || !pParent->IsBalanceNode())
                break;

            pPrevious = pParent->_pPreviousSiblingNode;
            pParent = pParent->_pParentNode;
        }
    }

    return pPrevious;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetPreviousSiblingNode
//
//  Synopsis:   Find previous sibling that isn't a balance node or a
//              destroyed node.
//
//  Arguments:  fRestrictToLayer    TRUE if the previous sibling must be in
//                                  the same layer.
//
//  Returns:    Previous sibling node.
//
//  Notes:
//
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetPreviousSiblingNode(BOOL fRestrictToLayer) const
{
    CDispNode* pPrevious = GetPreviousSiblingNode();
    return (pPrevious == NULL ||
            (fRestrictToLayer && pPrevious->GetLayerType() != GetLayerType()))
        ? NULL
        : pPrevious;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetNextSiblingNode
//
//  Synopsis:   Find next sibling that isn't a balance node or a
//              destroyed node.
//
//  Arguments:  none
//
//  Returns:    Next sibling node.
//
//  Notes:
//
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetNextSiblingNode() const
{
    CDispNode* pNext = _pNextSiblingNode;
    CDispInteriorNode* pParent = _pParentNode;

    for (;;)
    {
        // bypass destructed nodes
        while (pNext != NULL && pNext->IsSet(CDispFlags::s_destruct))
        {
            pNext = pNext->_pNextSiblingNode;
        }

        // found a non-destruct next sibling
        if (pNext != NULL)
        {
            // simple case: it's not a balance node
            if (!pNext->IsBalanceNode())
                break;

            // get left-most child of this balance node
            CDispBalanceNode* pBalance = DYNCAST(CDispBalanceNode, pNext);
            pNext = pBalance->GetFirstChildNode();
            if (pNext != NULL)
                break;

            // if there were no children, continue scanning at this level
            pNext = pBalance->_pNextSiblingNode;
        }

        // no next sibling at this level
        else
        {
            // if we didn't find a next sibling at this level, and the parent
            // node is a balance node, continue search up one level in the tree
            if (pParent == NULL || !pParent->IsBalanceNode())
                break;

            pNext = pParent->_pNextSiblingNode;
            pParent = pParent->_pParentNode;
        }
    }

    return pNext;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetNextSiblingNode
//
//  Synopsis:   Find next sibling that isn't a balance node or a
//              destroyed node.
//
//  Arguments:  fRestrictToLayer    TRUE if the next sibling must be in
//                                  the same layer.
//
//  Returns:    Next sibling node.
//
//  Notes:
//
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetNextSiblingNode(BOOL fRestrictToLayer) const
{
    CDispNode* pNext = GetNextSiblingNode();
    return (pNext == NULL ||
            (fRestrictToLayer && pNext->GetLayerType() != GetLayerType()))
        ? NULL
        : pNext;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetRootNode
//
//  Synopsis:   Return root node for this tree containing this node.
//
//  Arguments:  none
//
//  Returns:    A pointer to the root node.
//
//  Notes:      It is not safe to assume, in general, that the root node for
//              this tree has node type ROOT.
//
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetRootNode() const
{
    CDispNode* pNode = (CDispNode*) this;
    while (pNode->_pParentNode != NULL)
    {
        pNode = pNode->_pParentNode;
    }
    return pNode;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetNextNodeInZOrder
//
//  Synopsis:   Get the next node (may be an interior node!) in Z order.
//
//  Arguments:  none
//
//  Returns:    the next node or NULL if this node is the highest node in
//              the tree
//
//  Notes:
//
//----------------------------------------------------------------------------

CDispNode*
CDispNode::GetNextNodeInZOrder() const
{
    const CDispNode* pNode = this;
    while (pNode)
    {
        CDispNode* pNextNode = pNode->_pNextSiblingNode;
        while (pNextNode)
        {
            if (!pNextNode->IsSet(CDispFlags::s_destruct))
                return pNextNode;
            pNextNode = pNextNode->_pNextSiblingNode;
        }
        pNode = pNode->_pParentNode;
    }

    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::TraverseTreeTopToBottom
//
//  Synopsis:   Traverse the display tree from top to bottom, calling the
//              ProcessDisplayTreeTraversal on every client we encounter.
//
//  Arguments:  pClientData     client defined data
//
//  Returns:    TRUE to continue traversal
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::TraverseTreeTopToBottom(void* pClientData)
{
    Assert(!IsSet(CDispFlags::s_destruct));

    if (IsInteriorNode())
    {
        CDispInteriorNode* pInterior = DYNCAST(CDispInteriorNode, this);
        for (CDispNode* pChild = pInterior->_pLastChildNode;
             pChild != NULL;
             pChild = pChild->_pPreviousSiblingNode)
        {
            if (pChild->IsSet(CDispFlags::s_destruct))
                continue;

            if (!pChild->TraverseTreeTopToBottom(pClientData))
            {
                return FALSE;
            }
        }
    }

    CDispClient* pClient = GetDispClient();
    if (pClient && !pClient->ProcessDisplayTreeTraversal(pClientData))
    {
        return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::InsertPreviousSiblingNode
//
//  Synopsis:   Insert a node as the previous sibling of this node.
//
//  Arguments:  pNewSibling     pointer to node to be inserted
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::InsertPreviousSiblingNode(CDispNode* pNewSibling)
{
    // this node must be in the tree
    Assert(_pParentNode != NULL);
    Assert(pNewSibling != NULL);

    if (pNewSibling != this)
    {
        // is the new sibling already in the tree?
        if (pNewSibling->_pParentNode != NULL)
        {
            // is the new sibling already this node's previous sibling?
            if (_pPreviousSiblingNode == pNewSibling ||
                GetPreviousSiblingNode() == pNewSibling)
                return;

            // extract new sibling from its current location
            pNewSibling->ExtractFromTree();
        }

        Assert(pNewSibling->_pParentNode == NULL);
        Assert(pNewSibling->_pPreviousSiblingNode == NULL);
        Assert(pNewSibling->_pNextSiblingNode == NULL);

        _pParentNode->_cChildren++;
        pNewSibling->_pParentNode = _pParentNode;

        CDispNode* pOldSibling = _pPreviousSiblingNode;
        pNewSibling->_pPreviousSiblingNode = pOldSibling;
        pNewSibling->_pNextSiblingNode = this;
        _pPreviousSiblingNode = pNewSibling;
        if (pOldSibling)
        {
            pOldSibling->_pNextSiblingNode = pNewSibling;
        }
        else
        {
            _pParentNode->_pFirstChildNode = pNewSibling;
        }

        // if a insertion-aware leaf node, note the change
        if (pNewSibling->IsInsertionAware() &&
            pNewSibling->IsLeafNode())
        {
            pNewSibling->SetFlag(CDispFlags::s_justInserted);
        }

        // inval new node and recalc its children
        pNewSibling->RequestRecalc();
        pNewSibling->SetFlag(CDispFlags::s_invalAndRecalcChildren);
    }

#if DBG==1
    VerifyTreeCorrectness();
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::InsertNextSiblingNode
//
//  Synopsis:   Insert a node as the next sibling of this node.
//
//  Arguments:  pNewSibling     pointer to node to be inserted
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::InsertNextSiblingNode(CDispNode* pNewSibling)
{
    // this node must be in the tree
    Assert(_pParentNode != NULL);
    Assert(pNewSibling != NULL);

    if (   pNewSibling != this
        && pNewSibling
        && _pParentNode)
    {
        // is the new sibling already in the tree?
        if (pNewSibling->_pParentNode != NULL)
        {
            // is the new sibling already this node's next sibling?
            if (_pNextSiblingNode == pNewSibling ||
                GetNextSiblingNode() == pNewSibling)
                return;

            // extract new sibling from its current location
            pNewSibling->ExtractFromTree();
        }

        Assert(pNewSibling->_pParentNode == NULL);
        Assert(pNewSibling->_pPreviousSiblingNode == NULL);
        Assert(pNewSibling->_pNextSiblingNode == NULL);

        _pParentNode->_cChildren++;
        pNewSibling->_pParentNode = _pParentNode;

        CDispNode* pOldSibling = _pNextSiblingNode;
        pNewSibling->_pNextSiblingNode = pOldSibling;
        pNewSibling->_pPreviousSiblingNode = this;
        _pNextSiblingNode = pNewSibling;
        if (pOldSibling)
        {
            pOldSibling->_pPreviousSiblingNode = pNewSibling;
        }
        else
        {
            _pParentNode->_pLastChildNode = pNewSibling;
        }

        // if a insertion-aware leaf node, note the change
        if (pNewSibling->IsInsertionAware() &&
            pNewSibling->IsLeafNode())
        {
            pNewSibling->SetFlag(CDispFlags::s_justInserted);
        }

        // inval new node and recalc its children
        pNewSibling->RequestRecalc();
        pNewSibling->SetFlag(CDispFlags::s_invalAndRecalcChildren);
    }

#if DBG==1
    VerifyTreeCorrectness();
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ExtractFromTree
//
//  Synopsis:   Extract this node from the display tree.
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::ExtractFromTree()
{
    // check to see if this node is actually in a tree
    if (_pParentNode == NULL)
    {
        Assert(_pPreviousSiblingNode == NULL);
        Assert(_pNextSiblingNode == NULL);
        return;
    }

    // flag this branch as needing recalc
    _pParentNode->RequestRecalc();

    // invalidate current visible bounds
    if (IsSet(CDispFlags::s_visibleNodeAndBranch))
        Invalidate(_rcVisBounds, COORDSYS_PARENT);

    Assert(_pParentNode->_cChildren > 0);
    _pParentNode->_cChildren--;

    // remove this node from parent's list of children
    if (_pPreviousSiblingNode)
    {
        _pPreviousSiblingNode->_pNextSiblingNode =
            _pNextSiblingNode;
        if (_pNextSiblingNode)
        {
            _pNextSiblingNode->_pPreviousSiblingNode =
                _pPreviousSiblingNode;
            _pNextSiblingNode = NULL;
        }
        else
        {
            _pParentNode->_pLastChildNode =
                _pPreviousSiblingNode;
        }
        _pPreviousSiblingNode = NULL;
    }
    else if (_pNextSiblingNode)
    {
        _pNextSiblingNode->_pPreviousSiblingNode =
            _pPreviousSiblingNode;
        _pParentNode->_pFirstChildNode =
            _pNextSiblingNode;
        _pNextSiblingNode = NULL;
    }
    else
    {
        // this node was the parent's only child
        Assert(_pParentNode->_cChildren == 0);
        _pParentNode->_pFirstChildNode =
            _pParentNode->_pLastChildNode = NULL;

        // remove all empty balance nodes above this node
        CDispInteriorNode* pParent = _pParentNode;
        while (pParent->IsBalanceNode() && pParent->_cChildren == 0)
        {
            pParent->SetFlag(CDispFlags::s_destruct);
            pParent = pParent->_pParentNode;
            if (pParent == NULL)
                break;
            pParent->_cChildren--;
            pParent->SetFlag(CDispFlags::s_destructChildren);
        }
    }

    _pParentNode = NULL;

    // destruct any children of the extracted node
    if (IsInteriorNode() && IsSet(CDispFlags::s_destructChildren))
    {
        CDispInteriorNode* pInterior = DYNCAST(CDispInteriorNode, this);
        pInterior->DestroyChildren();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ReplaceNode
//
//  Synopsis:   Replace the indicated node with this one.
//
//  Arguments:  pOldNode        node to replace
//
//  Notes:      This node must have no children.  If this node is a leaf node,
//              any unowned children of the replaced node will be deleted.
//              This node must not already be in a tree.
//
//----------------------------------------------------------------------------

void
CDispNode::ReplaceNode(CDispNode* pOldNode, BOOL fKeep, BOOL fTakeChildren)
{
    Assert(pOldNode != NULL && pOldNode != this);
    Assert(_pParentNode == NULL);
    Assert(_pPreviousSiblingNode == NULL);
    Assert(_pNextSiblingNode == NULL);
    Assert(!pOldNode->IsSet(CDispFlags::s_destruct));

    BOOL fMustInval = GetBounds() != pOldNode->GetBounds() &&
        pOldNode->IsSet(CDispFlags::s_visibleNodeAndBranch);

    // invalidate bounds of old node
    if (fMustInval)
    {
        pOldNode->Invalidate(pOldNode->_rcVisBounds, COORDSYS_PARENT);
    }

    // delete destroyed children of old node
    CDispInteriorNode* pOldInterior = NULL;
    if (pOldNode->IsInteriorNode())
    {
        pOldInterior = DYNCAST(CDispInteriorNode, pOldNode);
        if (pOldInterior->IsSet(CDispFlags::s_destructChildren))
        {
            pOldInterior->DestroyChildren();
        }
    }

    // transfer children if requested
    if (    fTakeChildren
        &&  IsInteriorNode()
        &&  pOldInterior)
    {
        CDispInteriorNode* pInterior = DYNCAST(CDispInteriorNode, this);
        Assert(pInterior->_cChildren == 0);
        Assert(pInterior->_pFirstChildNode == NULL);
        Assert(!IsSet(CDispFlags::s_destructChildren));
        pInterior->TakeChildrenFrom(pOldInterior);
    }

    // place in tree
    _pParentNode = pOldNode->_pParentNode;
    _pPreviousSiblingNode = pOldNode->_pPreviousSiblingNode;
    _pNextSiblingNode = pOldNode->_pNextSiblingNode;
    if (_pPreviousSiblingNode != NULL)
        _pPreviousSiblingNode->_pNextSiblingNode = this;
    if (_pNextSiblingNode != NULL)
        _pNextSiblingNode->_pPreviousSiblingNode = this;
    if (_pParentNode != NULL)
    {
        if (_pParentNode->_pFirstChildNode == pOldNode)
            _pParentNode->_pFirstChildNode = this;
        if (_pParentNode->_pLastChildNode == pOldNode)
            _pParentNode->_pLastChildNode = this;
    }

    // Take the old node's position. If the old node's parent is right to left,
    // use the top/right coordinate to set the position. Recalcing will
    // properly size the new node.
    // REMEMBER: Nodes get the coordinate system of their PARENT.
    if(pOldNode->_pParentNode && pOldNode->_pParentNode->IsRightToLeft())
    {
        CPoint pt;
        pOldNode->GetPositionTopRight(&pt);
        SetPositionTopRight(pt);
    }
    else
    {
        SetPosition(pOldNode->GetPosition());
    }

    CSize size;
    pOldNode->GetSize(&size);
    SetSize(size, TRUE);

    // if requested, delete old node
    if (!fKeep)
    {
#if DBG==1
        pOldNode->_flags = CDispFlags::s_debugDestruct;
        pOldNode->_pParentNode = NULL;
        pOldNode->_pPreviousSiblingNode = pOldNode->_pNextSiblingNode = NULL;
#endif
        delete pOldNode;
    }

    // invalidate new bounds
    if (fMustInval)
    {
        RequestRecalc();
        SetFlag(CDispFlags::s_inval);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ReplaceParent
//
//  Synopsis:   Replace this node's parent with this node, and delete the parent
//              node and any of its other children.
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::ReplaceParent()
{
    CDispInteriorNode* pParent = _pParentNode;

    // skip balance nodes between this node and its true parent
    while (pParent->IsBalanceNode())
    {
        pParent = pParent->_pParentNode;
    }

    Assert(_pParentNode != NULL);

    // copy all information needed to insert child in correct spot in tree
    CDispInteriorNode* pGrandparent = pParent->_pParentNode;
    CDispNode* pPrevious = pParent->_pPreviousSiblingNode;
    CDispNode* pNext = pParent->_pNextSiblingNode;

    pParent->ExtractFromTree();

    // delete the parent and its unowned children (but make this node owned
    // so it doesn't get deleted)
    if (!IsOwned())
    {
        SetOwned(TRUE);
        pParent->Destroy();
        SetOwned(FALSE);
    }
    else
    {
        pParent->Destroy();
    }

    // now insert under old parent's parent node
    if (pGrandparent)
    {
        pGrandparent->InsertChildNode(this, pPrevious, pNext);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::InsertParent
//
//  Synopsis:   Insert a new parent node above this node.
//
//  Arguments:  pNewParent      new parent node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::InsertParent(CDispInteriorNode* pNewParent)
{
    // pNewParent should not be in the tree or have any children
    Assert(pNewParent->_pParentNode == NULL);
    Assert(pNewParent->_pPreviousSiblingNode == NULL);
    Assert(pNewParent->_pNextSiblingNode == NULL);
    Assert(pNewParent->_pFirstChildNode == NULL);
    Assert(pNewParent->_pLastChildNode == NULL);

    if (_pParentNode == NULL)
    {
        Assert(_pPreviousSiblingNode == NULL);
        Assert(_pNextSiblingNode == NULL);

        pNewParent->InsertFirstChildNode(this);
        return;
    }

    // place in tree
    pNewParent->_pParentNode = _pParentNode;
    if (_pParentNode->_pFirstChildNode == this)
    {
        _pParentNode->_pFirstChildNode = pNewParent;
    }
    if (_pParentNode->_pLastChildNode == this)
    {
        _pParentNode->_pLastChildNode = pNewParent;
    }
    _pParentNode = pNewParent;
    pNewParent->_pPreviousSiblingNode = _pPreviousSiblingNode;
    if (_pPreviousSiblingNode != NULL)
    {
        _pPreviousSiblingNode->_pNextSiblingNode = pNewParent;
        _pPreviousSiblingNode = NULL;
    }
    pNewParent->_pNextSiblingNode = _pNextSiblingNode;
    if (_pNextSiblingNode != NULL)
    {
        _pNextSiblingNode->_pPreviousSiblingNode = pNewParent;
        _pNextSiblingNode = NULL;
    }
    pNewParent->_pFirstChildNode = pNewParent->_pLastChildNode = this;
    pNewParent->_cChildren = 1;

#if DBG==1
    pNewParent->VerifyTreeCorrectness();
#endif

    // request recalc for this node and its new parent
    RequestRecalc();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetBounds
//
//  Synopsis:   Get the bounds of this node in the indicated coordinate system.
//
//  Arguments:  prcBounds           returns bounds rect
//              coordinateSystem    which coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetBounds(
        RECT* prcBounds,
        COORDINATE_SYSTEM coordinateSystem) const
{
    GetBounds(prcBounds);
    CSize offset;
    GetTransformOffset(&offset, COORDSYS_PARENT, coordinateSystem);
    ((CRect*)prcBounds)->OffsetRect(offset);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetClippedBounds
//
//  Synopsis:   Get the clipped bounds of this node in the requested coordinate
//              system.
//
//  Arguments:  prcBounds           returns bounds rect
//              coordinateSystem    which coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetClippedBounds(
        RECT* prcBounds,
        COORDINATE_SYSTEM coordinateSystem) const
{
    CDispContext context;

    GetBounds(prcBounds);
    GetNodeTransform(&context, COORDSYS_PARENT, COORDSYS_GLOBAL);
    ((CRect*)prcBounds)->IntersectRect(context._rcClip);

    if (((CRect*)prcBounds)->IsEmpty())
    {
        *prcBounds = g_Zero.rc;
    }
    else if (coordinateSystem != COORDSYS_PARENT)
    {
        CSize offset;
        GetTransformOffset(&offset, COORDSYS_PARENT, coordinateSystem);
        ((CRect*)prcBounds)->OffsetRect(offset);
        if (    IsRightToLeft()
            &&  coordinateSystem != COORDSYS_GLOBAL)
        {
            ((CRect*)prcBounds)->MirrorX();
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::PreDraw
//
//  Synopsis:   Before drawing starts, PreDraw processes the redraw region,
//              subtracting areas that are blocked by opaque or buffered items.
//              PreDraw is finished when the redraw region becomes empty
//              (i.e., an opaque item completely obscures all content below it)
//
//              NOTE: This method is called by subclasses to handle opaque
//              items and containers.
//
//  Arguments:  pContext    draw context
//
//  Returns:    TRUE if first opaque node to draw has been found
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::PreDraw(CDispDrawContext* pContext)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(AllSet(CDispFlags::s_preDrawSelector));
    Assert(pContext->IntersectsRedrawRegion(_rcVisBounds));
    Assert(!IsSet(CDispFlags::s_generalFlagsNotSetInDraw));

    // if this node isn't visible, opacity doesn't matter
    if (!IsVisible())
    {
        return FALSE;
    }

    // BUGBUG (donmarsh) - for now, we do not delve inside a filtered node.
    // Someday, this will be up to the filter to determine whether PreDraw
    // can safely look at its children and come up with the correct answers.
    if (IsFiltered())
        return FALSE;

    // determine if enough of this opaque thing is visible to warrant
    // expensive subtraction from the redraw region
    const CRegion* prgnRedraw = pContext->GetRedrawRegion();
    CRect rcGlobal;
    GetBounds(&rcGlobal);
    if (HasUserClip())
    {
        rcGlobal.IntersectRect(GetUserClip());
    }
    pContext->Transform(&rcGlobal);
    
    // on Win_9x, we can't allow operations on regions that exceed GDI's 16-bit
    // resolution, therefore we don't perform opaque optimizations in these cases
    BOOL fHackForGDI = g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS;
#if DBG==1
    if (IsTagEnabled(tagHackGDICoords)) fHackForGDI = TRUE;
#endif
    if (fHackForGDI &&
        rcGlobal.Width() > 32767 ||
        rcGlobal.Height() > 32767 ||
        rcGlobal.left < -32768 || rcGlobal.left > 32767 ||
        rcGlobal.top < -32768 || rcGlobal.top > 32767 ||
        rcGlobal.right < -32768 || rcGlobal.right > 32767 ||
        rcGlobal.bottom < -32768 || rcGlobal.bottom > 32767)
        return FALSE;
    
    if (prgnRedraw->BoundsInside(rcGlobal))
    {
        // add this node to the redraw region stack
        Verify(!pContext->PushRedrawRegion(rcGlobal,this));
        return TRUE;
    }

    CRegion rgnGlobal(rcGlobal);
    rgnGlobal.Intersect(*prgnRedraw);
    if (rgnGlobal.GetBounds().Area() < MINIMUM_OPAQUE_PIXELS)
    {
        // intersection isn't big enough, simply continue processing
        return FALSE;
    }

    // BUGBUG (donmarsh) - The following code prevents an opaque item from
    // subtracting itself from a rectangular redraw region and producing
    // a non-rectangular result.  Non-rectangular regions are just too
    // slow for Windows to deal with.
    switch (prgnRedraw->ResultOfSubtract(rgnGlobal))
    {
    case CRegion::SUB_EMPTY:
        // this case should have been identified already above
        Assert(FALSE);
        return FALSE;
    case CRegion::SUB_RECTANGLE:
        // subtract this item's region and continue predraw processing
        SetBranchFlag(CDispFlags::s_savedRedrawRegion);
        if (!pContext->PushRedrawRegion(rgnGlobal,this))
            return TRUE;
        break;
    case CRegion::SUB_REGION:
        // don't subtract this item's region to keep redraw region simple
        break;
    case CRegion::SUB_UNKNOWN:
        // see if we can get back to a simple rectangular redraw region
        if (!pContext->PushRedrawRegion(rgnGlobal,this))
        {
            SetBranchFlag(CDispFlags::s_savedRedrawRegion);
            return TRUE;
        }
        prgnRedraw = pContext->GetRedrawRegion();
        if (prgnRedraw->IsRegion())
        {
            pContext->RestorePreviousRedrawRegion();
        }
        else
        {
            SetBranchFlag(CDispFlags::s_savedRedrawRegion);
        }
        break;
    }

    // BUGBUG (donmarsh) - what I would have liked to do instead of the
    // code above.
#ifdef NEVER
    // subtract the item's clipped and offset bounds from the redraw region
    SetBranchFlag(CDispFlags::s_savedRedrawRegion);
    if (!pContext->PushRedrawRegion(rgnGlobal,this))
        return TRUE;
#endif

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::Draw
//              
//  Synopsis:   Determine whether this node should be drawn, and what its
//              clipping rect should be.
//              
//  Arguments:  pContext        display context
//              pChild          if not NULL, which child to start drawing at
//              
//  Notes:      This method should only be called on nodes that are visible
//              and in-view: AllSet(pContext->_drawSelector).
//              
//----------------------------------------------------------------------------

void
CDispNode::Draw(CDispDrawContext* pContext, CDispNode* pChild)
{
    Assert(AllSet(pContext->_drawSelector));
    
    // take care of additional clipping specified on this node
    BOOL fHasUserClip = HasUserClip();
    CRect rcClipOld;

    rcClipOld.SetRectEmpty();

    if (fHasUserClip)
    {
        // combine user clip with clip rect passed from parent
        CRect rcClipNew(GetUserClip());
        rcClipNew.OffsetRect(GetBounds().TopLeft().AsSize());
        rcClipNew.IntersectRect(pContext->GetClipRect());
        if (rcClipNew.IsEmpty())
        {
            // this node shouldn't have saved a redraw region, since it is
            // completely clipped out
            Assert(!IsSet(CDispFlags::s_savedRedrawRegion));
            return;
        }
        
        rcClipOld = pContext->GetClipRect();
        pContext->SetClipRect(rcClipNew);
    }
    
    // do the clipped bounds of this node intersect the redraw region?
    if (!IsSet(CDispFlags::s_savedRedrawRegion))
    {
        CRect rcBounds(_rcVisBounds);
        rcBounds.IntersectRect(pContext->GetClipRect());
        if (!pContext->IntersectsRedrawRegion(rcBounds))
            goto done;
    }
        
    // redirect for filtering
    if (!IsFiltered() || pContext->_fBypassFilter)
    {
        pContext->_fBypassFilter = FALSE;
        DrawSelf(pContext, pChild);
    }
    else
    {
        Assert(pChild == NULL);
        GetFilter()->DrawFiltered(pContext);
    }

done:
    // restore previous clip rect if necessary
    if (fHasUserClip)
    {
        pContext->_rcClip = rcClipOld;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DrawNodeForFilter
//              
//  Synopsis:   This is a special entry point that modifies the context
//              appropriately to draw the contents of a node which will then
//              be filtered.
//              
//  Arguments:  pContext        draw context
//              hdc             destination surface
//              ptOrg           where to draw in the HDC
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispNode::DrawNodeForFilter(
        CDispDrawContext* pContext,
        HDC hdc,
        const CPoint& ptOrg)
{
    Assert(pContext != NULL);
    Assert(hdc != NULL);
    
    // can't do anything if hdc is NULL
    if (hdc == NULL)
        return;

    // REVIEW (donmarsh) -- for clarity, this should really use the context
    // saver object CSaveDispContext.
    CDispContext saveContext(*pContext);

    // if the filter gave us our own HDC, we don't have to do any special
    // clipping
    if (pContext->_pDispSurface->GetRawDC() == hdc)
    {
        // does this disabled filter really intersect the redraw region?
        pContext->_rcClip.IntersectRect(GetBounds());
        Assert(pContext->IntersectsRedrawRegion(pContext->_rcClip));
        
        DrawSelf(pContext, NULL);
    
        // BUGBUG (donmarsh) -- the surface we have here has physical clipping
        // enforced on the DC (see CFilter::DrawFiltered).
        // DrawSelf may modify the surface state to say that no physical clipping
        // is being done, but CFilter::Draw does a RestoreDC that reinstates
        // physical clipping.  Now the clip state in the surface will not match
        // the actual state of the DC, and the next item to draw
        // will assume that no physical clipping has been applied to the DC
        // when it actually has.  The following hack sets surface
        // state so that the next item is forced to reestablish the clip region
        // appropriately.
        pContext->_pDispSurface->ForceClip();
    }

    else
    {
        CDispSurface* pSaveSurface = pContext->_pDispSurface;
        pContext->_pDispSurface = new CDispSurface(hdc);

        if (pContext->_pDispSurface != NULL)
        {
            pContext->_rcClip = GetBounds();
            pContext->_offset = ptOrg - pContext->_rcClip.TopLeft();
    
            CRegion rgnClip(pContext->_rcClip);
            rgnClip.Offset(pContext->_offset);
    
            CRegion* pSaveRedrawRegion = pContext->_prgnRedraw;
            pContext->_prgnRedraw = &rgnClip;
    
            // get surface ready for rendering
            pContext->_pDispSurface->SetClipRgn(&rgnClip);
    
            // draw content that might not be in view
            CDispFlags saveSelector(pContext->_drawSelector);
            pContext->_drawSelector = CDispFlags::s_visibleBranch;
            DrawSelf(pContext, NULL);
            pContext->_drawSelector = saveSelector;
    
            pContext->_prgnRedraw = pSaveRedrawRegion;
    
            delete pContext->_pDispSurface;
        }

        pContext->_pDispSurface = pSaveSurface;
    }

    *pContext = saveContext;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetVisible
//
//  Synopsis:   Set this node's visibility.
//
//  Arguments:  fVisible    TRUE if this node is visible
//
//----------------------------------------------------------------------------

void
CDispNode::SetVisible(BOOL fVisible)
{
    if (fVisible != IsVisible())
    {
        SetBoolean(CDispFlags::s_visibleNodeAndBranch, fVisible);
        RequestRecalc();

        //
        // Make sure we send the view change notification for this
        // visibility change.
        //
        if (IsLeafNode() &&
            IsSet(CDispFlags::s_inViewChange))
        {
            SetFlag(CDispFlags::s_positionHasChanged);
        }

        // if going invisible, invalidate current bounds
        if (!fVisible)
        {
            if (!IsSet(CDispFlags::s_inval))
            {
                PrivateInvalidate(_rcVisBounds, COORDSYS_PARENT);
            }
        }
        else
        {
            SetFlag(CDispFlags::s_inval);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::UnfilteredSetOpaque
//
//  Synopsis:   Set the opacity of this node.  An opaque node must opaquely
//              draw every pixel within its bounds (_rcVisBounds for
//              CDispLeafNodes, _rcContainer for CDispContainer nodes).
//
//  Arguments:  fOpaque     TRUE if this node is opaque
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::UnfilteredSetOpaque(BOOL fOpaque)
{
    if (fOpaque != IsSet(CDispFlags::s_opaqueNode))
    {
        SetBoolean(CDispFlags::s_opaqueNode, fOpaque);
        // note: s_opaqueBranch will be set appropriately in Recalc
        RequestRecalc();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetBranchFlag
//
//  Synopsis:   Set flag(s) all the way up this branch of the tree.
//
//  Arguments:  flag        the flag(s) to set
//
//----------------------------------------------------------------------------

void
CDispNode::SetBranchFlag(const CDispFlags& flag)
{
    // set this flag all the way up the branch until we run into the root
    // node or a node that already has it set
    SetFlag(flag);
    for (CDispNode* pParent = _pParentNode;
        pParent && !pParent->AllSet(flag);
        pParent = pParent->_pParentNode)
    {
        pParent->SetFlag(flag);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ClearBranchFlag
//
//  Synopsis:   Clear flag(s) all the way up this branch of the tree,
//              or until an interior node is reached which has a child with
//              this flag set.
//
//  Arguments:  flag        the flag(s) to clear
//
//----------------------------------------------------------------------------

void
CDispNode::ClearBranchFlag(const CDispFlags& flag)
{
    ClearFlag(flag);

    CDispFlags clearFlag(flag);
    for (CDispInteriorNode* pParent = _pParentNode;
        pParent != NULL;
        pParent = pParent->_pParentNode)
    {
        // if this node has any children with flag set, it stays set in the node
        CDispNode* pChild;
        for (pChild = pParent->_pFirstChildNode;
            pChild != NULL;
            pChild = pChild->_pNextSiblingNode)
        {
            clearFlag.Clear(pChild->_flags);
            if (!clearFlag.IsAnySet())
                return;
        }

        pParent->ClearFlag(clearFlag);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ClearFlagToRoot
//
//  Synopsis:   Clear a flag all the way to the root, without ORing with flag
//              values contributed by other siblings along the way.
//
//  Arguments:  flag        flag to clear
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::ClearFlagToRoot(const CDispFlags& flag)
{
    for (CDispNode* pNode = this;
        pNode != NULL && pNode->IsSet(flag);
        pNode = pNode->_pParentNode)
    {
        pNode->ClearFlag(flag);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::RequestRecalc
//
//  Synopsis:   Set flags on this branch requiring recalc.
//
//  Arguments:
//
//----------------------------------------------------------------------------

void
CDispNode::RequestRecalc()
{
#if DBG==1
    CDispNode* pNode = this;
#endif

    // set recalc flag all the way up this branch, until we run into the root
    // or a recalc flag already set
    SetFlag(CDispFlags::s_recalc);
    CDispNode* pParent = _pParentNode;
    while (pParent && !pParent->IsSet(CDispFlags::s_recalc))
    {
#if DBG==1
        pNode = pParent;
#endif
        pParent->SetFlag(CDispFlags::s_recalc);
        pParent = pParent->_pParentNode;
    }

#if DBG==1
    // recalc must be set all the way to the root
    pNode->VerifyRecalc();

    // check tree open state
    pNode = pNode->GetRootNode();
    if (pNode->IsDispRoot())
    {
        AssertSz(DYNCAST(CDispRoot,pNode)->_cOpen > 0,
            "Display Tree not properly opened before tree modification");
        AssertSz(!DYNCAST(CDispRoot,pNode)->_fDrawLock,
            "Display Tree RequestRecalc called during Draw");
    }
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetClipRect
//
//  Synopsis:   Return the clip rect for this node.
//
//  Arguments:  prcClip     returns clipping rect
//
//  Returns:    TRUE if prcClip is not empty
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::GetClipRect(RECT* prcClip) const
{
    CDispContext context;
    GetNodeTransform(&context, COORDSYS_PARENT, COORDSYS_GLOBAL);
    *prcClip = context._rcClip;
    return !context._rcClip.IsEmpty();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetClippedClientRect
//
//  Synopsis:   Get the clipped client rect of this node in the requested coordinate
//              system.
//
//  Arguments:  prc     returns bounds rect
//              type    which client rect
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetClippedClientRect(RECT* prc, CLIENTRECT type) const
{
    CDispContext context;

    GetClientRect(prc, type);
    GetNodeTransform(&context, COORDSYS_CONTENT, COORDSYS_GLOBAL);
    ((CRect*)prc)->IntersectRect(context._rcClip);

    if (((CRect*)prc)->IsEmpty())
    {
        *prc = g_Zero.rc;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::PrivateInvalidate (protected)
//
//  Synopsis:   Invalidate the given rectangle.
//
//  Arguments:  rcInvalid           invalid rect
//              coordinateSystem    which coordinate system the rect is in
//              fSynchronousRedraw  TRUE to force synchronous redraw
//              fIgnoreFilter       TRUE to ignore filtering on this node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::PrivateInvalidate(
        const CRect& rcInvalid,
        COORDINATE_SYSTEM coordinateSystem,
        BOOL fSynchronousRedraw,
        BOOL fIgnoreFilter)
{
    // conditions under which no invalidation is necessary:
    // 1. rcInvalid is empty
    // 2. this node is not visible or not in view
    // 3. a parent of this node has set flag to suppress invalidation
    // 4. this node isn't in a tree rooted in a CDispRoot node

    CRect rc;
    CDispNode* pChild;
    CDispInteriorNode* pParent;

    if (rcInvalid.IsEmpty() || !MustInvalidate())
        return;

    // copy invalid rect so we can transform it
    rc = rcInvalid;

    // don't clip if we're coming from a parent coordinate system.  This
    // is important to make sure that CDispNode::InvalidateEdges can
    // invalidate areas outside the current display node boundaries when
    // the display node is expanding.
    BOOL fClip = (coordinateSystem >= COORDSYS_CONTAINER);
    
    if (coordinateSystem != COORDSYS_CONTAINER)
    {
        TransformRect(&rc, coordinateSystem, COORDSYS_CONTAINER, fClip);
    }

    // clip to userclip
    if (HasUserClip())
    {
        rc.IntersectRect(GetUserClip());
    }

    TransformRect(&rc, COORDSYS_CONTAINER, COORDSYS_PARENT, fClip);

    if (rc.IsEmpty())
        return;

    pChild = this;

    // traverse tree to root, clipping rect as necessary
    for (pParent = _pParentNode;
        pParent != NULL;
        pChild = pParent, pParent = pParent->_pParentNode)
    {
        // hand off to filter
        if (!fIgnoreFilter)
        {
            if (pChild->IsFiltered())
            {
                pChild->TransformRect(
                    &rc,
                    COORDSYS_PARENT,
                    COORDSYS_CONTAINER,
                    FALSE);
                pChild->GetFilter()->Invalidate(rc, fSynchronousRedraw);
                return;
            }
        }
        else
        {
            // don't ignore filters of parent nodes further up this branch
            fIgnoreFilter = FALSE;
        }

        // check for clipped rect
        pParent->TransformRect(
            &rc,
            pChild->GetContentCoordinateSystem(),
            COORDSYS_PARENT,
            TRUE);
        if (rc.IsEmpty())
            return;
    }

    // we made it to the root with a non-empty rect
    if (pChild->GetNodeType() == DISPNODETYPE_ROOT)
    {
        CDispRoot* pRoot = DYNCAST(CDispRoot,pChild);
        pRoot->InvalidateRoot(rc, fSynchronousRedraw, FALSE);
    }

    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::PrivateInvalidate
//
//  Synopsis:   Invalidate the given region.
//
//  Arguments:  rgnInvalid          invalid region
//              coordinateSystem    which coordinate system the region is in
//              fSynchronousRedraw  TRUE to force synchronous redraw
//              fIgnoreFilter       TRUE to ignore filtering on this node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::PrivateInvalidate(
        const CRegion& rgnInvalid,
        COORDINATE_SYSTEM coordinateSystem,
        BOOL fSynchronousRedraw,
        BOOL fIgnoreFilter)
{
    // conditions under which no invalidation is necessary:
    // 1. rcInvalid is empty
    // 2. this node is not visible or not in view
    // 3. a parent of this node has set flag to suppress invalidation
    // 4. this node isn't in a tree rooted in a CDispRoot node

    if (rgnInvalid.IsEmpty() || !MustInvalidate())
        return;

    // copy invalid region so we can transform it
    CRegion rgn(rgnInvalid);
    CDispNode* pChild;
    CDispInteriorNode* pParent;

    // don't clip if we're coming from a parent coordinate system.  This
    // is important to make sure that CDispNode::InvalidateEdges can
    // invalidate areas outside the current display node boundaries when
    // the display node is expanding.
    BOOL fClip = (coordinateSystem >= COORDSYS_CONTAINER);
    
    if (coordinateSystem != COORDSYS_CONTAINER)
    {
        TransformRegion(&rgn, coordinateSystem, COORDSYS_CONTAINER, fClip);
    }

    // clip to userclip
    if (HasUserClip())
    {
        rgn.Intersect(GetUserClip());
    }

    TransformRegion(&rgn, COORDSYS_CONTAINER, COORDSYS_PARENT, fClip);

    if (rgn.IsEmpty())
        return;

    pChild = this;

    // traverse tree to root, clipping rect as necessary
    for (pParent = _pParentNode;
        pParent != NULL;
        pChild = pParent, pParent = pParent->_pParentNode)
    {
        // hand off to filter
        if (!fIgnoreFilter)
        {
            if (pChild->IsFiltered())
            {
                pChild->TransformRegion(
                    &rgn,
                    COORDSYS_PARENT,
                    COORDSYS_CONTAINER,
                    FALSE);
                if (rgn.IsRegion())
                {
                    HRGN hrgn = rgn.GetRegionAlias();
                    pChild->GetFilter()->Invalidate(hrgn, fSynchronousRedraw);
                }
                else
                {
                    pChild->GetFilter()->Invalidate(rgn.AsRect(), fSynchronousRedraw);
                }
                goto Done;
            }
        }
        else
        {
            // don't ignore filters of parent nodes further up this branch
            fIgnoreFilter = FALSE;
        }

        // check for clipped region
        pParent->TransformRegion(
            &rgn,
            pChild->GetContentCoordinateSystem(),
            COORDSYS_PARENT,
            TRUE);
        if (rgn.IsEmpty())
            goto Done;
    }

    // we made it to the root with a non-empty rect
    if (pChild->GetNodeType() == DISPNODETYPE_ROOT)
    {
        CDispRoot* pRoot = DYNCAST(CDispRoot,pChild);
        pRoot->InvalidateRoot(rgn, fSynchronousRedraw, FALSE);
    }

Done:
    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::HitTest
//
//  Synopsis:   Determine whether and what the given point intersects.
//
//  Arguments:  pptHit              [in] the point to test
//                                  [out] if something was hit, the point is
//                                  returned in container coordinates for the
//                                  thing that was hit
//              coordinateSystem    the initial coordinate system for pptHit
//              pClientData         client data
//              fHitContent         if TRUE, hit test the content regardless
//                                  of whether it is clipped or not. If FALSE,
//                                  take current clipping into account,
//                                  and clip to the bounds of this container.
//              cFuzzyHitTest       Number of pixels to extend hit testing
//                                  rectangles in order to do "fuzzy" hit
//                                  testing
//
//  Returns:    TRUE if the point hit something.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::HitTest(
        CPoint* pptHit,
        COORDINATE_SYSTEM coordinateSystem,
        void* pClientData,
        BOOL fHitContent,
        long cFuzzyHitTest)
{
    // create hit context
    CDispHitContext hitContext;
    hitContext._ptHitTest = *pptHit;
    hitContext._pClientData = pClientData;
    hitContext._cFuzzyHitTest = cFuzzyHitTest;
    hitContext.SetToIdentity();

    // translate to parent coordinates
    if (coordinateSystem != COORDSYS_PARENT)
    {
        TransformPoint(
            &hitContext._ptHitTest,
            coordinateSystem,
            COORDSYS_PARENT);
    }

    BOOL fHit = FALSE;

    // must be a visible node somewhere
    if (IsSet(CDispFlags::s_visibleBranch))
    {
        CPoint ptOriginal(hitContext._ptHitTest);
        
        // are we hit testing content?  If not, take clipping into account for
        // this node and its parents.
        if (!fHitContent)
        {
            GetNodeTransform(&hitContext, COORDSYS_PARENT, COORDSYS_GLOBAL);

            //
            // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
            // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
            // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
            //
            
            fHit =
                hitContext.FuzzyRectIsHit(_rcVisBounds, IsFatHitTest() ) &&
                HitTestPoint(&hitContext);
        }
        else if (IsLeafNode())
        {
            //
            // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
            // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
            // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
            //

            fHit =
                hitContext.FuzzyRectIsHit(_rcVisBounds, IsFatHitTest() ) &&
                HitTestPoint(&hitContext);
        }
        else
        {
            CDispInteriorNode* pInterior = DYNCAST(CDispInteriorNode, this);

            // calculate clip and position info
            CDispInfo di(GetExtras());
            pInterior->CalcDispInfo(hitContext._rcClip, &di);

            // translate hit point to scrolled coordinates
            hitContext._ptHitTest -=
                di._borderOffset + di._contentOffset + di._scrollOffset;

            // search for a hit from foreground layers to background
            int lastLayer = DISPNODELAYER_POSITIVEZ;
            for (CDispNode* pChild = pInterior->_pLastChildNode;
                 pChild != NULL;
                 pChild = pChild->_pPreviousSiblingNode)
            {
                // branch must have a visible node, else skip it
                if (!pChild->IsSet(CDispFlags::s_visibleBranch))
                    continue;

                // translate hit point between different layer types
                int childLayer = (int) pChild->GetLayerType();
                if (childLayer != lastLayer)
                {
                    Assert(lastLayer > childLayer);
                    switch (childLayer)
                    {
                    case DISPNODELAYER_NEGATIVEZ:
                        if (lastLayer == DISPNODELAYER_FLOW)
                            hitContext._ptHitTest += *di._pInsetOffset;
                        break;
                    case DISPNODELAYER_FLOW:
                        hitContext._ptHitTest -= *di._pInsetOffset;
                        break;
                    }
                    lastLayer = childLayer;
                }

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
                
                fHit =
                    hitContext.FuzzyRectIsHit(rcChild, IsFatHitTest() ) &&
                    pChild->HitTestPoint(&hitContext);

                if (fHit)
                    break;
            }

            //
            // no children were hit, check to see if this container was hit
            // but HitTextContent can only be called if the point is over the
            // element. e.g. (-500, -500) should just be !fHit
            //
            if (!fHit && GetBounds().Contains(ptOriginal))
            {
                CDispClient* pClient = GetDispClient();
                if (pClient != NULL)
                {
                    fHit = pClient->HitTestContent(
                        &hitContext._ptHitTest,
                        this,
                        hitContext._pClientData);
                    AssertSz(fHit, "Expected to hit content background");
                }
            }
        }
    }

    if (fHit)
        *pptHit = hitContext._ptHitTest;

    return fHit;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetTransformOffset
//
//  Synopsis:   Return an offset that can be added to points in the source
//              coordinate system, producing values for the destination
//              coordinate system.
//
//  Arguments:  pOffset         returns offset value
//              source          source coordinate system
//              destination     destination coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetTransformOffset(
        CSize* pOffset,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination) const
{
    if (destination < source)
        GetNodeTransform(pOffset, source, destination);

    else if (destination > source)
    {
        // reverse the transform and then negate the offset
        GetNodeTransform(pOffset, destination, source);

        *pOffset = -*pOffset;
    }

    else
        *pOffset = g_Zero.size;

}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetTransformContext
//
//  Synopsis:   Return a context that can be used to transform valules in the
//              source coordinate system, producing values for the destination
//              coordinate system.
//
//  Arguments:  pOffset         returns offset value
//              source          source coordinate system
//              destination     destination coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetTransformContext(
        CDispContext* pContext,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination) const
{
    if (destination < source)
        GetNodeTransform(pContext, source, destination);

    else if (destination > source)
    {
        GetNodeTransform(pContext, destination, source);
        pContext->_rcClip.OffsetRect(pContext->_offset);
        pContext->_offset = -pContext->_offset;
    }

    else
        pContext->SetToIdentity();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::TransformRect
//
//  Synopsis:   Transform a rect from the source coordinate system to the
//              destination coordinate system with optional clipping.
//
//  Arguments:  prc             rect to transform
//              source          source coordinate system
//              destination     destination coordinate system
//              fClip           TRUE to clip the rectangle
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::TransformRect(
        CRect* prc,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination,
        BOOL fClip) const
{
    if (fClip)
    {
        CDispContext context;
        GetTransformContext(&context, source, destination);
        context.Transform(prc);
    }

    // special case here for !fClip, because GetTransformOffset is significantly
    // faster than GetTransformContext
    else
    {
        CSize offset;
        GetTransformOffset(&offset, source, destination);
        prc->OffsetRect(offset);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::TransformRegion
//
//  Synopsis:   Transform a region from the source coordinate system to the
//              destination coordinate system with optional clipping.
//
//  Arguments:  prgn            region to transform
//              source          source coordinate system
//              destination     destination coordinate system
//              pChild          optional child pointer for child/parent transform
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::TransformRegion(
        CRegion* prgn,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination,
        BOOL fClip) const
{
    if (fClip)
    {
        CDispContext context;
        GetTransformContext(&context, source, destination);
        context.Transform(prgn);
    }

    // special case here for !fClip, because GetTransformOffset is significantly
    // faster than GetTransformContext
    else
    {
        CSize offset;
        GetTransformOffset(&offset, source, destination);
        prgn->Offset(offset);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetNodeTransform
//
//  Synopsis:   Return an offset that can be added to points in the source
//              coordinate system, producing values for the destination
//              coordinate system.
//
//  Arguments:  pOffset         returns offset value
//              source          source coordinate system
//              destination     destination coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetNodeTransform(
        CSize* pOffset,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination) const
{
    Assert(destination < source);
    Assert(source != COORDSYS_GLOBAL);

    // for vanilla CDispNode, there are only two relevant coordinate systems:
    // COORDSYS_GLOBAL and everything else.
    if (destination != COORDSYS_GLOBAL || _pParentNode == NULL)
        *pOffset = g_Zero.size;

    else
        _pParentNode->GetNodeTransform(
            pOffset,
            GetContentCoordinateSystem(),
            COORDSYS_GLOBAL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::GetNodeTransform
//
//  Synopsis:   Return a context that can be used to transform values from
//              the source coordinate system to the destination coordinate
//              system.
//
//  Arguments:  pContext        returns context
//              source          source coordinate system
//              destination     destination coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::GetNodeTransform(
        CDispContext* pContext,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination) const
{
    Assert(destination < source);
    Assert(source != COORDSYS_GLOBAL);

    // for vanilla CDispNode, there are only two relevant coordinate systems:
    // COORDSYS_GLOBAL and everything else.
    if (destination != COORDSYS_GLOBAL || _pParentNode == NULL)
        pContext->SetToIdentity();

    else
        _pParentNode->GetNodeTransform(
            pContext,
            GetContentCoordinateSystem(),
            COORDSYS_GLOBAL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ScrollRectIntoView
//
//  Synopsis:   Scroll the given rect in CONTENT coordinates into
//              view, with various pinning (alignment) options.
//
//  Arguments:  rc              rect to scroll into view
//              spVert          scroll pin request, vertical axis
//              spHorz          scroll pin request, horizontal axis
//              coordSystem     coordinate system for rc (COORDSYS_CONTENT
//                              or COORDSYS_NONFLOWCONTENT only)
//
//  Returns:    TRUE if any scrolling was necessary.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::ScrollRectIntoView(
        const CRect& rc,
        CRect::SCROLLPIN spVert,
        CRect::SCROLLPIN spHorz,
        COORDINATE_SYSTEM coordSystem,
        BOOL fScrollBits)
{
    Assert(coordSystem == COORDSYS_CONTENT ||
           coordSystem == COORDSYS_NONFLOWCONTENT);

    if (_pParentNode == NULL)
        return FALSE;

    CRect rcParent(rc);
    CSize offset;
    GetTransformOffset(&offset, coordSystem, COORDSYS_PARENT);
    rcParent.OffsetRect(offset);

    return
        _pParentNode->ScrollRectIntoView(
            rcParent,
            spVert,
            spHorz,
            GetContentCoordinateSystem(),
            fScrollBits);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ScrollIntoView
//
//  Synopsis:   Scroll the given display node into view.
//
//  Arguments:  spVert      vertical scroll pin option
//              spHorz      horizontal scroll pin option
//
//  Returns:    TRUE if scrolling was necessary
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::ScrollIntoView(CRect::SCROLLPIN spVert, CRect::SCROLLPIN spHorz)
{
    return
        _pParentNode != NULL &&
        _pParentNode->ScrollRectIntoView(
            _rcVisBounds, spVert, spHorz, GetContentCoordinateSystem(), TRUE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SubtractOpaqueSiblings
//
//  Synopsis:   Subtract opaque areas corresponding to overlapping sibling
//              nodes.
//
//  Arguments:  pRegion     region to subtract from
//
//  Returns:    TRUE if pRegion is not empty after this call
//
//----------------------------------------------------------------------------

BOOL
CDispNode::SubtractOpaqueSiblings(CRegion* pRegion)
{
    CRect rcRegionBounds;
    pRegion->GetBounds(&rcRegionBounds);

    CDispContext context;
    context._rcClip = rcRegionBounds;

    for (CDispNode* pSibling = _pNextSiblingNode;
         pSibling != NULL && !pRegion->IsEmpty();
         pSibling = pSibling->_pNextSiblingNode)
    {
        // only visible, inview, opaque branches are of interest
        if (pSibling->AllSet(CDispFlags::s_subtractOpaqueSelector))
        {
            if (pSibling->IsLeafNode() ||
                pSibling->IsSet(CDispFlags::s_opaqueNode))
            {
                pRegion->Subtract(pSibling->GetBounds());
                pRegion->GetBounds(&rcRegionBounds);
            }
            else if (rcRegionBounds.Intersects(pSibling->_rcVisBounds))
            {
                CDispInteriorNode* pInterior =
                    DYNCAST(CDispInteriorNode,pSibling);
                if (!pInterior->SubtractOpaqueChildren(pRegion, &context))
                    return FALSE;
            }
        }
    }

    return !pRegion->IsEmpty();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::Recalc
//
//  Synopsis:   After a node is inserted in the tree, we have to calculate
//              its visibility, whether it is in view, and we have to update
//              flags and bounding rectangles in parent nodes.
//
//  Arguments:  fForceRecalc    TRUE to force recalc of this subtree
//              fSuppressInval  if TRUE, suppress invalidation
//              pContext        draw context
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::Recalc(
        BOOL fForceRecalc,
        BOOL fSuppressInval,
        CDispDrawContext* pContext)
{
    Assert(fForceRecalc || IsSet(CDispFlags::s_recalc));

    BOOL fInval = IsSet(CDispFlags::s_inval);

    // apply optional user clip
    CApplyUserClip applyUC(this, pContext);
    
    // set visible flag and invalidate if this node is a visible item
    if (IsLeafNode())
    {
        // is this item in view?
        BOOL fInView = IsSet(CDispFlags::s_visibleNode) && _rcVisBounds.Intersects(pContext->_rcClip);
        BOOL fWasInView = IsSet(CDispFlags::s_inView);
        ClearFlag(CDispFlags::s_clearInRecalc);
        SetBoolean(CDispFlags::s_inView, fInView);
        
        // notify client of visibility changes if requested
        if ( IsSet(CDispFlags::s_inViewChangeOrNeedsInsertedNotify) )
        {
            // if our parent moved and forced recalc of its children, the
            // children may have moved
            if (fForceRecalc)
                SetFlag(CDispFlags::s_positionHasChanged);

            if (fInView != fWasInView ||
                (fInView && IsSet(CDispFlags::s_positionHasChanged)) ||
                IsSet(CDispFlags::s_justInserted))
            {
                CDispLeafNode* pLeaf = DYNCAST(CDispLeafNode, this);
                pLeaf->NotifyInViewChange(
                    pContext,
                    fInView && IsVisible(),
                    fWasInView,
                    FALSE);
            }
            ClearFlag(CDispFlags::s_positionHasChanged);
            ClearFlag(CDispFlags::s_justInserted);
        }
    }

    else
    {
        CDispInteriorNode* pInterior = DYNCAST(CDispInteriorNode,this);

        // get rid of destroyed children
        if (IsSet(CDispFlags::s_destructChildren))
        {
            pInterior->DestroyChildren();
        }

        // check for tree balance
        if (pInterior->_cChildren > MAX_CHILDREN_BEFORE_BALANCE)
        {
            pInterior->BalanceTree();
        }

        // recalc children
        pInterior->RecalcChildren(
            fForceRecalc || IsSet(CDispFlags::s_recalcChildren),
            fSuppressInval || fInval,
            pContext);

        // set in-view status
        if (!IsSet(CDispFlags::s_inView) &&
            pContext->_rcClip.Intersects(_rcVisBounds))
        {
            SetFlag(CDispFlags::s_inView);
        }
    }

    // add to invalid area if necessary
    if (fInval && !fSuppressInval && _flags.AllSet(CDispFlags::s_visible))
    {
        pContext->AddToRedrawRegion(_rcVisBounds);
    }

    // set opaque branch flag
    if (GetFlag(CDispFlags::s_opaqueNodeAndBranch) == CDispFlags::s_opaqueNode &&
        GetBounds().Area() >= MINIMUM_OPAQUE_PIXELS)
    {
        SetFlag(CDispFlags::s_opaqueBranch);
    }

    // notify filter of possible size changes
    // BUGBUG (donmarsh) - in order to filter positioned children, we really
    // want to return the size of _rcVisBounds here, but that doesn't match
    // the container coordinate system we ask filters to render themselves in.
    // Eventually, we should notify the filter of the size of each layer.
    if (IsFiltered())
    {
        GetFilter()->SetSize(GetBounds().Size());
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::MustInvalidate
//
//  Synopsis:   A parent node may have already invalidated the area within
//              its clipping region, so children may not have to do invalidation
//              calculations.
//
//  Arguments:  none
//
//  Returns:    TRUE if this node must perform invalidation calculations.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispNode::MustInvalidate() const
{
    for (const CDispNode* pNode = this;
         pNode != NULL;
         pNode = pNode->_pParentNode)
    {
        if (pNode->IsSet(CDispFlags::s_inval))
            return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::InvalidateEdges
//
//  Synopsis:   Invalidate edges of a node that is changing size.
//
//  Arguments:  rcOld           old bounds
//              rcNew           new bounds
//              rcBorderWidths  width of borders
//              fRightToLeft    TRUE if right to left coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::InvalidateEdges(
        const CRect& rcOld,
        const CRect& rcNew,
        const CRect& rcBorderWidths,
        BOOL fRightToLeft)
{
    CRect rcInval;

    // invalidate edges that are changing
    if (rcOld.right != rcNew.right)
    {
        rcInval.SetRect(
            min(rcOld.right, rcNew.right) - rcBorderWidths.right,
            rcOld.top,
            max(rcOld.right, rcNew.right),
            max(rcOld.bottom, rcNew.bottom) - rcBorderWidths.bottom);
        Invalidate(rcInval, COORDSYS_PARENT);
    }
    else if (rcOld.left != rcNew.left)
    {
        rcInval.SetRect(
            min(rcOld.left, rcNew.left),
            rcOld.top,
            max(rcOld.left, rcNew.left) + rcBorderWidths.left,
            max(rcOld.bottom, rcNew.bottom) - rcBorderWidths.bottom);
        Invalidate(rcInval, COORDSYS_PARENT);
    }
    else if (!fRightToLeft)
    {
        if (rcBorderWidths.right != 0)
        {
            rcInval.SetRect(
                rcNew.right - rcBorderWidths.right,
                rcNew.top,
                rcNew.right,
                rcNew.bottom);
            Invalidate(rcInval, COORDSYS_PARENT);
        }
    }
    else
    {
        if (rcBorderWidths.left != 0)
        {
#if 1
// BUGBUG (donmarsh) -- this is bogus!
            rcInval.SetRect(
                rcNew.left + rcBorderWidths.left,
                rcNew.top,
                rcNew.left,
                rcNew.bottom);
#else
// BUGBUG (donmarsh) -- this is the way it really should be, but
// checkin is delayed until IE 5.x to prevent regressions on 5.0
            rcInval.SetRect(
                rcNew.left,
                rcNew.top,
                rcNew.left + rcBorderWidths.left,
                rcNew.bottom);
#endif
            Invalidate(rcInval, COORDSYS_PARENT);
        }
    }

    if (rcOld.bottom != rcNew.bottom)
    {
        rcInval.SetRect(
            rcOld.left,
            min(rcOld.bottom, rcNew.bottom) - rcBorderWidths.bottom,
            rcOld.right,
            max(rcOld.bottom, rcNew.bottom));
        Invalidate(rcInval, COORDSYS_PARENT);
    }
    else if (rcBorderWidths.bottom > 0)
    {
        rcInval.SetRect(
            rcNew.left,
            rcNew.bottom - rcBorderWidths.bottom,
            rcNew.right,
            rcNew.bottom);
        Invalidate(rcInval, COORDSYS_PARENT);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetUserClip
//
//  Synopsis:   Set user clip rect for this node.
//
//  Arguments:  rcUserClip      rectangle in this node's local coordinates
//              pExtras         disp node extra information
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::SetUserClip(const RECT& rcUserClip, CDispExtras* pExtras)
{
    Assert(pExtras != NULL);

    if (!IsSet(CDispFlags::s_inval) && IsSet(CDispFlags::s_visibleBranchAndInView))
    {
        // invalidate area that was exposed before (in case it will be clipped
        // now and the background needs to be repainted)
        Invalidate((const CRect&)(pExtras->GetUserClip()), COORDSYS_CONTAINER);

        // invalidate new clipped area
        SetFlag(CDispFlags::s_inval);
    }

    RequestRecalc();
    SetFlag(CDispFlags::s_recalcChildren);

    pExtras->SetUserClip(rcUserClip);
    SetFlag(CDispFlags::s_hasUserClip);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::SetFiltered
//
//  Synopsis:   Turn on filtering for this node
//
//  Arguments:  fFiltered
//
//  Notes:
//
//----------------------------------------------------------------------------
void
CDispNode::SetFiltered(BOOL fFiltered)
{
    Assert(GetNodeType() != DISPNODETYPE_ROOT);
    
    if (fFiltered != IsFiltered())
    {
        SetBoolean(CDispFlags::s_filtered, fFiltered);
        SetFlag(CDispFlags::s_inval);
        RequestRecalc();
    }
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpDisplayTree
//
//  Synopsis:   Dump the display tree containing this node.
//
//  Arguments:  none (so it can be easily called from the debugger)
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpDisplayTree()
{
    HANDLE              hfile    = CreateFile(_T("c:\\displaytree.htm"),
                                        GENERIC_WRITE | GENERIC_READ,
                                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                                        NULL,
                                        OPEN_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);

    if (hfile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    SetFilePointer( hfile, GetFileSize( hfile, 0 ), 0, 0 );

    CDispNode* pRoot = GetRootNode();
    if (pRoot != NULL)
    {
        pRoot->DumpStart(hfile);
        if (pRoot->IsDispRoot())
        {
            WriteString(hfile, _T("<h5>"));
            WriteString(hfile, (LPTSTR) *((CStr*)(DYNCAST(CDispRoot,pRoot)->_debugUrl)));
            WriteString(hfile, _T("</h5>\r\n"));
        }
        pRoot->Dump(hfile, 0, MAXLONG, 0);
        pRoot->DumpEnd(hfile);
    }

    CloseHandle(hfile);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpDisplayNode
//
//  Synopsis:   Dump debugging information for this node and its immediate
//              children.
//
//  Arguments:  none (so it can be easily called from the debugger)
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpDisplayNode()
{
    HANDLE              hfile    = CreateFile(_T("c:\\displaytree.htm"),
                                        GENERIC_WRITE | GENERIC_READ,
                                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                                        NULL,
                                        OPEN_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);

    if (hfile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    SetFilePointer( hfile, GetFileSize( hfile, 0 ), 0, 0 );

    DumpStart(hfile);
    Dump(hfile, 0, 1, 0);
    DumpEnd(hfile);

    CloseHandle(hfile);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpStart
//
//  Synopsis:   Start dump of display tree debug information.
//
//  Arguments:  hFile       handle to output file
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpStart(HANDLE hFile)
{
    WriteString(hFile, _T("\
<html>\r\n\
<head>\r\n\
<style>\r\n\
body       { font-family:verdana; font-size:11 }\r\n\
div        { padding-left:10; padding-right:3; padding-top:2; padding-bottom:3; cursor:default}\r\n\
div.root   { background-color:e0e0e0 }\r\n\
div.c0     { background-color:ffe0e0 }\r\n\
div.c1     { background-color:e0ffff }\r\n\
div.c2     { background-color:ffe0ff }\r\n\
div.c3     { background-color:e0ffe0 }\r\n\
div.c4     { background-color:e0e0ff }\r\n\
div.c5     { background-color:ffffe0 }\r\n\
font.s0    { background-color:ffff40; font-weight:bold }\r\n\
font.background    { background-color:e0e0c0; font-style:italic }\r\n\
font.border        { background-color:e0e0c0; font-style:italic }\r\n\
font.childwindow   { background-color:e0e0c0; font-style:italic }\r\n\
font.flow          { background-color:e0e0c0; font-style:italic }\r\n\
font.tag           { background-color:e0e0c0; font-style:italic; font-weight:bold }\r\n\
xmp        { margin-top:0; margin-bottom:0; background-color:f0f0d0 }\r\n\
</style>\r\n\
"));
    WriteString(hFile, _T("\
<script>\r\n\
function OnClick()\r\n\
{\r\n\
\r\n\
    if (!event.ctrlKey)\r\n\
    {\r\n\
        var srcElem = event.srcElement;\r\n\
\r\n\
        while (srcElem && srcElem.tagName != 'DIV')\r\n\
        {\r\n\
            srcElem = srcElem.parentElement;\r\n\
        }\r\n\
\r\n\
        if (srcElem)\r\n\
        {\r\n\
            var elem = srcElem.children.tags('SPAN');\r\n\
            if (elem.length > 0)\r\n\
            {\r\n\
                var className = elem[0].className;\r\n\
                if (className == 'c')\r\n\
                {\r\n\
                    elem[0].style.display = elem[0].style.display == '' ? 'none' : '';\r\n\
                }\r\n\
            }\r\n\
        }\r\n\
    }\r\n\
    else\r\n\
    {\r\n\
        document.all.body.innerText = '';\r\n\
    }\r\n\
}\r\n\
</script>\r\n\
"));
    WriteString(hFile, _T("\
</head>\r\n\
<body onclick=OnClick()>\r\n\
"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpEnd
//
//  Synopsis:   Finish dumping display tree debugging information.
//
//  Arguments:  hFile       handle to output file
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpEnd(HANDLE hFile)
{
    WriteString(hFile, _T("\
<hr>\r\n\
</body>\r\n\
</html>\r\n\
"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::Dump
//
//  Synopsis:   Dump debugging information about the display tree
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              maxLevel    max tree depth to dump
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::Dump(HANDLE hFile, long level, long maxLevel, long childNumber)
{
    if (level == 0)
    {
        WriteString(hFile, _T("<div class=root"));
    }
    else
    {
        WriteHelp(hFile, _T("<<div class=c<0d>"), level % 6);
    }

    // change cursor for node with children
    if (IsInteriorNode() && DYNCAST(CDispInteriorNode,this)->_cChildren > 0)
    {
        WriteString(hFile, _T(" style='cursor:hand'"));
    }
    WriteString(hFile, _T(">\r\n"));

    DumpClassName(hFile, level, childNumber);
    DumpInfo(hFile, level, childNumber);
    WriteString(hFile, _T("\r\n"));
    DumpBounds(hFile, level, childNumber);
    DumpFlags(hFile, level, childNumber);
    CDispExtras* pExtras = GetExtras();
    if (pExtras != NULL)
    {
        CDispInfo di(pExtras);

        if (HasUserClip())
        {
            WriteString(hFile, _T("<i>user clip:</i>"));
            DumpRect(hFile, GetUserClip(pExtras));
        }
        if (GetBorderType(pExtras) == DISPNODEBORDER_SIMPLE)
        {
            WriteHelp(hFile, _T("<<i>uniform border:<</i> <0d>"),
                      di._prcBorderWidths->left);
            DumpEndLine(hFile);
        }
        else if (GetBorderType(pExtras) == DISPNODEBORDER_COMPLEX)
        {
            WriteString(hFile, _T("<i>border widths:</i>"));
            DumpRect(hFile, *di._prcBorderWidths);
        }
        if (HasInset(pExtras))
        {
            WriteHelp(hFile, _T("<<i>inset:<</i> x:<0d>, y:<1d>"),
                      di._pInsetOffset->cx, di._pInsetOffset->cy);
            DumpEndLine(hFile);
        }
        if (HasExtraCookie(pExtras))
        {
            WriteHelp(hFile, _T("<<i>extra cookie:<</i> <0d> (0x<1d>)"),
                      (LONG)(LONG_PTR)GetExtraCookie(pExtras),
                      (LONG)(LONG_PTR)GetExtraCookie(pExtras));
            DumpEndLine(hFile);
        }
    }
    WriteHelp(hFile, _T("(<0d> bytes)<<br>\r\n"), GetMemorySize());
    DumpChildren(hFile, level, maxLevel, childNumber);

    WriteString(hFile, _T("</div>\r\n"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpClassName
//
//  Synopsis:   Display this node's class name and child number
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpClassName(HANDLE hFile, long level, long childNumber)
{
    // print child number
    WriteHelp(hFile, _T("<<b><0d>. "), childNumber+1);

    if (IsInteriorNode() && DYNCAST(CDispInteriorNode,this)->_cChildren > 0)
    {
        WriteString(hFile, _T("<u>"));
    }

    // print node type
    switch (GetNodeType())
    {
    case DISPNODETYPE_BALANCE:
        WriteString(hFile, _T("<i>balance</i>"));
        break;
    case DISPNODETYPE_ROOT:
        WriteString(hFile, _T("CDispRoot"));
        break;
    case DISPNODETYPE_ITEMPLUS:
        WriteString(hFile, _T("CDispItemPlus"));
        break;
    case DISPNODETYPE_GROUP:
        WriteString(hFile, _T("CDispGroup"));
        break;
    case DISPNODETYPE_CONTAINER:
        WriteString(hFile, _T("CDispContainer"));
        break;
    case DISPNODETYPE_CONTAINERPLUS:
        WriteString(hFile, _T("CDispContainerPlus"));
        break;
    case DISPNODETYPE_SCROLLER:
        WriteString(hFile, _T("CDispScroller"));
        break;
    case DISPNODETYPE_SCROLLERPLUS:
        WriteString(hFile, _T("CDispScrollerPlus"));
        break;
    default:
        WriteString(hFile, _T("UNKNOWN NODE TYPE"));
        break;
    }
    if (IsInteriorNode())
    {
        CDispInteriorNode* pInterior = DYNCAST(CDispInteriorNode,this);
        if (pInterior->_cChildren > 0)
        {
            WriteString(hFile, _T("</u>"));
            WriteHelp(hFile, _T("<</b>  0x<0x>  (children: <1d>)"), this, pInterior->_cChildren);
        }
        else
        {
            WriteHelp(hFile, _T("<</b>  0x<0x>"), this);
        }
    }
    else
    {
        WriteHelp(hFile, _T("<</b>  0x<0x>"), this);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpInfo
//
//  Synopsis:   Dump custom information for this node.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//  Notes:      Nodes with extra information to display override this method.
//
//----------------------------------------------------------------------------

void
CDispNode::DumpInfo(HANDLE hFile, long level, long childNumber)
{
    DumpEndLine(hFile);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpBounds
//
//  Synopsis:   Dump this node's bounding rect.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpCoord(HANDLE hFile, TCHAR* pszLabel, LONG coord, BOOL fHighlight)
{
    WriteHelp(hFile, _T(" <0s>:"), pszLabel);
    if (fHighlight)
    {
        WriteString(hFile, _T("<font class=s0>"));
    }
    WriteHelp(hFile, _T("<0d>"), coord);
    if (fHighlight)
    {
        WriteString(hFile, _T("</font>"));
    }
}

void
CDispNode::DumpRect(HANDLE hFile, const RECT& rc)
{
    // print bounds
    WriteHelp(hFile, _T(" l:<0d> t:<1d> r:<2d> b:<3d><<br>\r\n"),
              rc.left, rc.top, rc.right, rc.bottom);
}

void
CDispNode::DumpRect(HANDLE hFile, const RECT& rc, const CDispFlags& flags)
{
    // print bounds
    DumpCoord(hFile, _T("l"), rc.left, FALSE);
    DumpCoord(hFile, _T("t"), rc.top, FALSE);
    DumpCoord(hFile, _T("r"), rc.right, FALSE);
    DumpCoord(hFile, _T("b"), rc.bottom, FALSE);
    DumpEndLine(hFile);
}

void
CDispNode::DumpBounds(HANDLE hFile, long level, long childNumber)
{
    // print bounds
    WriteString(hFile, _T("<i>rcVis:</i>"));
    DumpRect(hFile, _rcVisBounds, _flags);
}

void
CDispNode::DumpEndLine(HANDLE hFile)
{
    WriteString(hFile, _T("<br>\r\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpFlags
//
//  Synopsis:   Dump this node's flags.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpFlags(HANDLE hFile, long level, long childNumber)
{
    // print flags
    WriteString(hFile, _T("<i>flags:</i>"));

    // general flags
    switch (GetLayerType())
    {
    case DISPNODELAYER_NEGATIVEZ:
        WriteString(hFile, _T(" -z"));
        break;
    case DISPNODELAYER_FLOW:
        WriteString(hFile, _T(" flow"));
        break;
    case DISPNODELAYER_POSITIVEZ:
        WriteString(hFile, _T(" +z"));
        break;
    default:
        WriteHelp(hFile, _T("<<b>ILLEGAL LAYER: <0d><</b>"), (int)GetLayerType());
        break;
    }

    if (!AffectsScrollBounds())
    {
        WriteString(hFile, _T(" !AFFECTSSCROLLBOUNDS"));
    }
    if (!IsVisible())
    {
        WriteString(hFile, _T(" INVISIBLE"));
    }
    if (IsRightToLeft())
    {
        WriteString(hFile, _T(" RTL"));
    }
    if (IsSet(CDispFlags::s_owned))
    {
        WriteString(hFile, _T(" OWNED"));
    }
    if (IsSet(CDispFlags::s_destruct))
    {
        WriteString(hFile, _T(" <b>DESTRUCT</b>"));
    }
    if (IsSet(CDispFlags::s_hasBackground))
    {
        WriteString(hFile, _T(" HASBACKGROUND"));
    }
    if (IsSet(CDispFlags::s_hasLookaside))
    {
        WriteString(hFile, _T(" LOOKASIDE"));
    }
    if (IsSet(CDispFlags::s_bufferInvalid))
    {
        WriteString(hFile, _T(" BUFFERINVALID"));
    }
    if (IsSet(CDispFlags::s_savedRedrawRegion) && IsSet(CDispFlags::s_opaqueNode))
    {
        WriteString(hFile, _T(" SAVEDREDRAWREGION"));
    }
    if (IsSet(CDispFlags::s_inval))
    {
        WriteString(hFile, _T(" INVAL"));
    }
    if (IsSet(CDispFlags::s_recalcChildren))
    {
        WriteString(hFile, _T(" RECALCCHILDREN"));
    }
    if (IsSet(CDispFlags::s_opaqueNode))
    {
        WriteString(hFile, _T(" OPAQUENODE"));
    }
    if (IsSet(CDispFlags::s_filtered))
    {
        WriteString(hFile, _T("<b><i> FILTERED</i></b>"));
    }

    // leaf flags
    if (IsLeafNode())
    {
        // no leaf node flags right now...
    }

    // interior flags
    else
    {
        if (IsSet(CDispFlags::s_destructChildren))
        {
            WriteString(hFile, _T(" <b>DESTRUCTCHILDREN</b>"));
        }
        if (IsSet(CDispFlags::s_fixedBackground))
        {
            WriteString(hFile, _T(" FIXEDBKGD"));
        }
    }

    // propagated flags
    if (IsSet(CDispFlags::s_recalc))
    {
        WriteString(hFile, _T(" RECALC"));
    }
    if (!IsSet(CDispFlags::s_inView))
    {
        WriteString(hFile, _T(" !INVIEW"));
    }
    if (!IsSet(CDispFlags::s_visibleBranch))
    {
        WriteString(hFile, _T(" INVISIBLEBRANCH"));
    }
    if (IsSet(CDispFlags::s_opaqueBranch))
    {
        WriteString(hFile, _T(" OPAQUEBRANCH"));
    }
    if (IsSet(CDispFlags::s_buffered))
    {
        WriteString(hFile, _T(" BUFFERED"));
    }
    if (IsSet(CDispFlags::s_positionChange))
    {
        WriteString(hFile, _T(" POSCHANGE"));
    }
    if (IsSet(CDispFlags::s_inViewChange))
    {
        WriteString(hFile, _T(" INVIEWCHANGE"));
    }

    DumpEndLine(hFile);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::DumpChildren
//
//  Synopsis:   Dump this node's children.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              maxLevel    max tree depth to dump
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::DumpChildren(HANDLE hFile, long level, long maxLevel, long childNumber)
{
    // dump children
    if (level < maxLevel &&
        IsInteriorNode() && DYNCAST(CDispInteriorNode,this)->_cChildren > 0)
    {
        WriteString(hFile, _T("<span class=c style='display:none'>\r\n"));
        long i = 0;
        for (CDispNode* pChild = DYNCAST(CDispInteriorNode,this)->_pFirstChildNode;
             pChild != NULL;
             pChild = pChild->_pNextSiblingNode)
        {
            pChild->Dump(hFile, level+1, maxLevel, i);
            i++;
        }
        WriteString(hFile, _T("</span>\r\n"));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::VerifyTreeCorrectness
//
//  Synopsis:   Verify that a newly inserted node is not a parent of itself.
//              This is important, because otherwise the tree could have a
//              cycle that will cause infinite loops during traversal.
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::VerifyTreeCorrectness() const
{
    const CDispNode * pChild  = this;
    const CDispNode * pParent = _pParentNode;
    for (;
         pParent != NULL;
         pChild = pParent, pParent = pParent->_pParentNode)
    {
        AssertSz(pParent != pChild, "FATAL ERROR: Cycle in Display Tree will cause infinite loop");
        AssertSz(pParent != this, "FATAL ERROR: Cycle in Display Tree will cause infinite loop");
        Assert(pParent->IsBalanceNode() == (pParent->GetNodeType() == DISPNODETYPE_BALANCE));
    }

    if (_pPreviousSiblingNode != NULL)
    {
        AssertSz(_pPreviousSiblingNode->GetLayerType() <= GetLayerType(), "Invalid layer ordering");
    }

    if (_pNextSiblingNode != NULL)
    {
        AssertSz(_pNextSiblingNode->GetLayerType() >= GetLayerType(), "Invalid layer ordering");
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::VerifyRecalc
//
//  Synopsis:   Verify that the recalc flag is set all the way from
//              this node to the root.
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispNode::VerifyRecalc() const
{
    for (const CDispNode* pNode = this;
         pNode != NULL;
         pNode = pNode->_pParentNode)
    {
        Assert(pNode->IsSet(CDispFlags::s_recalc));
    }
}
#endif

