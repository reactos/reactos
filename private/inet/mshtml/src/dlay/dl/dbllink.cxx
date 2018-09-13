//-----------------------------------------------------------------------------
//
// Handy-Dandy doubly linked list package.
//
// Created by tedsmith
//
// See comments in dbllink.hxx  (this stuff is boring anyway)

#include <dlaypch.hxx>

#ifndef X_DBLLINK_HXX_
#define X_DBLLINK_HXX_
#include "dbllink.hxx"
#endif

void
CDblLinkedListBase::Remove(Node *pNode)
{
    Assert("Node must exist in Linked list" &&
           (pNode && !IsAnchor(pNode) && Contains(pNode)) );

    pNode->_pNext->_pPrev = pNode->_pPrev;
    pNode->_pPrev->_pNext = pNode->_pNext;
#if DBG == 1
    pNode->_pPrev = pNode->_pNext = NULL;
#endif
}


void
CDblLinkedListBase::InsertAfter(Node *pPosition, Node *pNewNode)
{
    Assert("A node can only be in one TIntrusive... at a time" &&
           (!pNewNode->_pPrev && !pNewNode->_pNext) );
    Assert("Node must exist in Linked list" &&
           (pPosition && Contains(pPosition)) );

    pNewNode->_pNext = pPosition->_pNext;
    pNewNode->_pPrev = pPosition;
    pPosition->_pNext->_pPrev = pNewNode;
    pPosition->_pNext = pNewNode;
}


void
CDblLinkedListBase::InsertBefore(Node *pPosition, Node *pNewNode)
{
    Assert("A node can only be in one TIntrusive... at a time" &&
           (!pNewNode->_pPrev && !pNewNode->_pNext) );
    Assert("Node must exist in Linked list" &&
           (pPosition && Contains(pPosition)) );

    pNewNode->_pNext = pPosition;
    pNewNode->_pPrev = pPosition->_pPrev;
    pPosition->_pPrev->_pNext = pNewNode;
    pPosition->_pPrev = pNewNode;
}


#if DBG == 1
BOOL
CDblLinkedListBase::Contains(const Node *pNode) const
{
    CALL_ISVALIDOBJECT(this);

    // We must implement this routine without using our abstract access
    //   routines since it is used to validate the arguments of those routines.
    const Node *pCur;
    for (pCur = &_Anchor; pCur != pNode; pCur = pCur->_pNext)
    {
        if (pCur == _Anchor._pPrev)
        {
            pCur = NULL;
            break;
        }
    }
    return !!pCur;
}


void
CDblLinkedListBase::IsValidObject() const
{
    const Node *pCur = &_Anchor;
    do
    {
        Assert(pCur->_pPrev);
        Assert(pCur->_pNext);
        Assert(pCur->_pPrev->_pNext == pCur);
        Assert(pCur->_pNext->_pPrev == pCur);
        pCur = pCur->_pNext;
    }
    while (!IsAnchor(pCur));
}


void
CDblLinkedListBaseIterator::IsValidObject() const
{
    Assert("A list must be associated with this Iterator" &&
           (_pList) );
    _pList->IsValidObject();
    Assert("Node must exist in Linked list" &&
           (!_pNode || _pList->Contains(_pNode)) );
}
#endif


