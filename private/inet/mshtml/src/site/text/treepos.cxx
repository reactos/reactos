#include "headers.hxx"

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_NOTIFY_HXX_
#define X_NOTIFY_HXX_
#include "notify.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_CGLYPH_HXX_
#define X_CGLYPH_HXX_
#include "cglyph.hxx"
#endif

#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

DeclareTag(tagTreePosOps,"TreePos","TreePos operations");
DeclareTag(tagTreePosAccess, "TreePos", "TreePos access");
DeclareTag(tagTreePosSplay, "TreePos", "Rebalancing");
DeclareTag(tagTreePosValidate, "TreePos", "Aggressive validation");

DeclareTag(tagNotify,     "Notify",     "Trace notifications");
DeclareTag(tagNotifyPath, "NotifyPath", "Trace notification send");

MtDefine(CTreePos, Tree, "CTreePos");
MtDefine(CTreeDataPos, Tree, "CTreeDataPos");
MtDefine(CMarkup_pvPool, CMarkup, "CMarkup::_pvPool")
MtDefine(CChildIterator, Tree, "CChildIterator");

#pragma warning(disable:4706) /* assignment within conditional expression */

MtDefine(CAryNotify_aryANotification_pv, CMarkup, "CMarkup::_aryANotification::_pv")

// allocate this many CTreePos objects at a time
static const size_t s_cTreePosPoolSize = 64;

// splay when accessing something deeper than this
static const long s_cSplayDepth = 10;

// The tree pos SN pool
#if DBG == 1 || defined(DUMPTREE)
int CTreePos::s_NextSerialNumber = 0;
#endif

inline BOOL
CMarkup::ShouldSplay(long cDepth) const
{
    return cDepth > 4 && (cDepth > 30 || (0x1<<cDepth) > NumElems());
}


/////////////////////////////////////////////////////////////////////////
//          CTreePos
/////////////////////////////////////////////////////////////////////////

inline void
CTreePos::ClearCounts()
{
    SetElemLeft( 0 );
    _cchLeft = 0;
}

inline void
CTreePos::IncreaseCounts(const CTreePos *ptp, unsigned fFlags)
{
    if (fFlags & TP_LEFT)
    {
        AdjElemLeft(ptp->GetElemLeft());
        _cchLeft += ptp->_cchLeft;
    }
    if (fFlags & TP_DIRECT)
    {
        if (ptp->IsNode())
        {
            if (ptp->IsBeginElementScope())
            {
                AdjElemLeft(1);
            }
            _cchLeft += 1;
        }
        else if (ptp->IsText())
        {
            _cchLeft += ptp->Cch();
        }
    }
}

inline void
CTreePos::IncreaseCounts(const SCounts *pCounts )
{
    AdjElemLeft( pCounts->_cElem );
    _cchLeft += pCounts->_cch;
}

inline void
CTreePos::DecreaseCounts(const CTreePos *ptp, unsigned fFlags)
{
    if (fFlags & TP_LEFT)
    {
        AdjElemLeft(-ptp->GetElemLeft());
        _cchLeft -= ptp->_cchLeft;
    }
    if (fFlags & TP_DIRECT)
    {
        if (ptp->IsNode())
        {
            if (ptp->IsBeginElementScope())
            {
                AdjElemLeft(-1);
            }
            _cchLeft -= 1;
        }
        else if (ptp->IsText())
        {
            _cchLeft -= ptp->Cch();
        }
    }
}


// functions for CTreePos::SCounts
inline void
CTreePos::SCounts::Clear()
{
    _cch = 0;
    _cElem = 0;
}


inline void
CTreePos::SCounts::Increase( const CTreePos * ptp )
{
    if (ptp->IsNode())
    {
        if (ptp->IsBeginElementScope())
        {
            _cElem++;
        }
        _cch++;;
    }
    else if (ptp->IsText())
    {
        _cch += ptp->Cch();
    }
}

inline BOOL
CTreePos::SCounts::IsNonzero()
{
    return _cElem  || _cch;
}


void
CTreePos::InitSublist()
{
    SetFirstChild(NULL);
    SetNext(NULL);
    MarkLast();
    MarkRight();        // this distinguishes a sublist from a splay tree root
    ClearCounts();
#if defined(MAINTAIN_SPLAYTREE_THREADS)
    SetLeftThread(NULL);
    SetRightThread(NULL);
#endif
}


inline void
CTreePos::ReplaceOrRemoveChild(CTreePos *pOld, CTreePos *pNew)
{
    if (pNew)
        ReplaceChild(pOld, pNew);
    else
        RemoveChild(pOld);
}


BOOL
CTreePos::HasNonzeroCounts(unsigned fFlags)
{
    BOOL result = FALSE;

    if (fFlags & TP_DIRECT)
    {
        Assert( !IsUninit() );
        result = result || !IsPointer();
    }

    if (fFlags & TP_LEFT)
    {
        result = result || GetElemLeft() > 0 || _cchLeft > 0;
    }

    return result;
}


#if DBG==1

inline BOOL
CTreePos::EqualCounts(const CTreePos *ptp) const
{
    return  GetElemLeft() == ptp->GetElemLeft() &&
            _cchLeft == ptp->_cchLeft;
}


BOOL
CTreePos::IsSplayValid(CTreePos *ptpTotal) const
{
    BOOL fIsValid = TRUE;
    CTreePos *ptpLeft, *ptpRight;
    const CTreePos *ptpFail = NULL;   // marks where we ran out of stack or memory
    const CTreePos *ptpCurr = this;
    enum { VISIT_FIRST, VISIT_MIDDLE, VISIT_LAST} state = VISIT_FIRST;
    CStackPtrAry<CTreePos*, 32> aryTreePosStack(Mt(Mem));
#define PUSH(stack, item)   stack.Append(item)
#define POP(stack)          stack[stack.Size()-1]; stack.Delete(stack.Size()-1)

    ptpTotal->ClearCounts();

    while (fIsValid && ptpCurr)
    {
        switch (state)
        {
        case VISIT_FIRST:
            ptpCurr->GetChildren(&ptpLeft, &ptpRight);

            // verify structure
            if (ptpLeft)
            {
                
                fIsValid = fIsValid && ptpLeft->IsLeftChild() &&
                                    ptpLeft->Parent() == ptpCurr;
                Assert(fIsValid);
            }
            if (ptpRight)
            {
                fIsValid = fIsValid && !ptpRight->IsLeftChild() &&
                                    ptpRight->Parent() == ptpCurr;
                Assert(fIsValid);
            }

            // push accumulated total onto the stack
            if (ptpFail == NULL)
            {
                if (S_OK == PUSH(aryTreePosStack, ptpTotal))
                {
                    ptpTotal = new CTreePos(TRUE);
                    if (ptpTotal == NULL)
                    {
                        ptpTotal = POP(aryTreePosStack);
                        ptpFail = ptpCurr;
                    }
                }
                else
                {
                    ptpFail = ptpCurr;
                }
            }

            // advance into left subtree
            if (ptpLeft)
                ptpCurr = ptpLeft;
            else
                state = VISIT_MIDDLE;
            break;

        case VISIT_MIDDLE:
            // compare true count from left subtree with count in current node,
            // then pop the stack
            if (ptpFail == NULL)
            {
                fIsValid = fIsValid && ptpCurr->EqualCounts(ptpTotal);
                Assert(fIsValid);
                delete ptpTotal;
                ptpTotal = POP(aryTreePosStack);
            }

            // accumulate counts
            ptpTotal->IncreaseCounts(ptpCurr, ptpFail ? TP_DIRECT : TP_BOTH);

#if defined(MAINTAIN_SPLAYTREE_THREADS)
            // check the threads
            fIsValid = fIsValid &&
                        (ptpCurr->LeftThread() == NULL ||
                         ptpCurr->LeftThread()->RightThread() == ptpCurr);
            fIsValid = fIsValid &&
                        (ptpCurr->RightThread() == NULL ||
                         ptpCurr->RightThread()->LeftThread() == ptpCurr);
            Assert(fIsValid);
#endif

            // advance into right subtree
            ptpRight = ptpCurr->RightChild();
            if (ptpRight)
                ptpCurr = ptpRight, state = VISIT_FIRST;
            else
                state = VISIT_LAST;
            break;

        case VISIT_LAST:
            // advance to parent
            if (ptpCurr == ptpFail)
            {
                ptpFail = NULL;
            }
            state = ptpCurr->IsLeftChild() ? VISIT_MIDDLE : VISIT_LAST;
            ptpCurr = (ptpCurr == this) ? NULL : ptpCurr->Parent();
            break;
        }
    }

    Assert(!fIsValid || aryTreePosStack.Size() == 0);
    fIsValid = fIsValid && aryTreePosStack.Size() == 0;

    aryTreePosStack.DeleteAll();

    return fIsValid;
}

#endif


#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

void
CTreePos::GetChildren(CTreePos **ppLeft, CTreePos **ppRight) const
{
    if (FirstChild())
    {
        if (FirstChild()->IsLeftChild())
        {
            *ppLeft = FirstChild();
            *ppRight = (FirstChild()->IsLastChild()) ? NULL : FirstChild()->Next();
        }
        else
        {
            *ppLeft = NULL;
            *ppRight = FirstChild();
        }
    }
    else
    {
        *ppLeft = *ppRight = NULL;
    }
}


HRESULT
CTreePos::Remove()
{
    Assert(!HasNonzeroCounts(TP_DIRECT));    // otherwise we need to adjust counts to the root
    CTreePos *pLeft, *pRight, *pParent=Parent();

    GetChildren(&pLeft, &pRight);

    if (pLeft == NULL)
    {
        pParent->ReplaceOrRemoveChild(this, pRight);
    }
    else
    {
        while (pRight)
        {
            pRight->RotateUp(this, pParent);
            pRight = RightChild();
        }
        pParent->ReplaceChild(this, pLeft);
    }

#if defined(MAINTAIN_SPLAYTREE_THREADS)
    // adjust threads
    if (LeftThread())
        LeftThread()->SetRightThread(RightThread());
    if (RightThread())
        RightThread()->SetLeftThread(LeftThread());
    SetLeftThread(NULL);
    SetRightThread(NULL);
#endif

    return S_OK;
}


CTreePos *
CTreePos::LeftmostDescendant() const
{
    const CTreePos *ptp = this;
    const CTreePos *ptpLeft = FirstChild();

    while (ptpLeft && ptpLeft->IsLeftChild())
    {
        ptp = ptpLeft;
        ptpLeft = ptp->FirstChild();
    }

    return const_cast<CTreePos *>(ptp);
}


CTreePos *
CTreePos::RightmostDescendant() const
{
    CTreePos *ptp = const_cast<CTreePos*>(this);
    CTreePos *ptpRight = RightChild();

    while (ptpRight)
    {
        ptp = ptpRight;
        ptpRight = ptp->RightChild();
    }

    return ptp;
}


#if DBG==1
long
CTreePos::Depth() const
{
    long cDepth=0;
    const CTreePos *ptp;

    for (ptp=this; ptp; ptp=ptp->Parent())
    {
        ++ cDepth;
    }

    return cDepth-2;
}
#endif


CMarkup *
CTreePos::GetMarkup()
{
    CTreePos *ptp=this, *ptpParent=Parent();

    while (ptpParent)
    {
        ptp = ptpParent;
        ptpParent = ptp->Parent();
    }

    AssertSz(!ptp->IsLeftChild(), "GetList called when not in a CMarkup");
    return ptp->IsLeftChild() ? NULL
                              : CONTAINING_RECORD(ptp, CMarkup, _tpRoot);
}

CTreePos *
CTreePos::PreviousTreePos()
{
    CTreePos *ptp = this;
    
    // Do we have a left child?
    if (ptp->FirstChild() && ptp->FirstChild()->IsLeftChild())
    {
        // Rightmost descendent for ptp->LeftChild()
    Loop:
        ptp = ptp->FirstChild();
        
        while (ptp->FirstChild())
        {
            if (!ptp->FirstChild()->IsLeftChild())
                goto Loop;
            
            if (ptp->FirstChild()->IsLastChild())
            {
                Assert(ptp == _ptpThreadLeft); // MAINTAIN_SPLAYTREE_THREADS
                return ptp;
            }
                
            ptp = ptp->FirstChild()->Next();
        }

        Assert(ptp == _ptpThreadLeft); // MAINTAIN_SPLAYTREE_THREADS
        return ptp;
    }
    
    // No left child
    while (ptp->IsLeftChild())
    {
        // ptp = ptp->Parent() -- inlined here
        if (ptp->IsLastChild())
            ptp = ptp->Next();
        else
            ptp = ptp->Next()->Next();

        // Root pos (marked as right child) protects us from NULL
    }
            
    // ptp = ptp->Parent() (we know ptp is a right (and last) child)
    ptp = ptp->Next();

    Assert(ptp == _ptpThreadLeft); // MAINTAIN_SPLAYTREE_THREADS
    return ptp;
}

CTreePos *
CTreePos::NextTreePos()
{
    CTreePos *ptp;
    
    if (!FirstChild())
        goto Up;

    if (!FirstChild()->IsLeftChild())
    {
        ptp = FirstChild();
        goto Right;
    }

    if (FirstChild()->IsLastChild())
        goto Up;
    
    ptp = FirstChild()->Next();

Right:

    // Leftmost descendent

    while (ptp->FirstChild() && ptp->FirstChild()->IsLeftChild())
        ptp = ptp->FirstChild();

    Assert(ptp == _ptpThreadRight); // MAINTAIN_SPLAYTREE_THREADS
    return ptp;
    
Up:

    for (ptp = this; !ptp->IsLeftChild(); ptp = ptp->Next());

    ptp = ptp->Parent();
    
    if (ptp->Next() == NULL)
    {
        Assert(NULL == _ptpThreadRight); // MAINTAIN_SPLAYTREE_THREADS
        return NULL;
    }

    Assert(ptp == _ptpThreadRight); // MAINTAIN_SPLAYTREE_THREADS
    return ptp;
}
        

CTreePos *
CTreePos::NextValidNonPtrInterLPos()
{
    CTreePos *ptp = this;

    do
    {
        ptp = ptp->NextValidInterLPos();
    }
    while (ptp && ptp->IsPointer());

    return ptp;
}

CTreePos *
CTreePos::PreviousValidNonPtrInterLPos()
{
    CTreePos *ptp = this;

    do
    {
        ptp = ptp->PreviousValidInterLPos();
    }
    while (ptp && ptp->IsPointer());

    return ptp;
}

CTreePos *
CTreePos::NextNonPtrTreePos()
{
    CTreePos * ptp = this;

    do
    {
        ptp = ptp->NextTreePos();
    }
    while ( ptp && ptp->IsPointer() );

    return ptp;
}

CTreePos *
CTreePos::PreviousNonPtrTreePos()
{
    CTreePos * ptp = this;

    do
    {
        ptp = ptp->PreviousTreePos();
    }
    while ( ptp && ptp->IsPointer() );

    return ptp;
}

// BUGBUG deprecated.  use CTreePosGap::IsValid()
BOOL
CTreePos::IsLegalPosition(CTreePos *ptpLeft, CTreePos *ptpRight)
{
    // use the marks to determine if content is allowed between the inputs
    return (!ptpLeft->IsNode() || !ptpRight->IsNode() ||
             !( (ptpLeft->IsEndNode() && !ptpLeft->IsEdgeScope()) ||
                (ptpRight->IsBeginNode() && !ptpRight->IsEdgeScope()) ) );
}

CTreePos *
CTreePos::FindLegalPosition(BOOL fBefore)
{
    CTreePos *ptpCur, *ptpAdvance;
    CTreePos * (CTreePos::*pAdvanceFn)(void);

    // select the member function we use to advance
    pAdvanceFn = fBefore ? &CTreePos::PreviousTreePos : &CTreePos::NextTreePos;

    // advance to a legal position, or the end of the list
    ptpAdvance = this;
    do
    {
        ptpCur = ptpAdvance;
        ptpAdvance = (ptpAdvance->*pAdvanceFn)();
    }
    while (ptpAdvance &&
            ( fBefore ? !IsLegalPosition(ptpAdvance, ptpCur)
                      : !IsLegalPosition(ptpCur, ptpAdvance) ));

    return ptpAdvance ? ptpCur : NULL;
}

BOOL
CTreePos::ShowTreePos(CGlyphRenderInfoType *pRenderInfo)
{
    BOOL fRet;

    // 1) We can only show edge scopes
    // 2) We can only show explicit end tags
    if (   IsEdgeScope()
        &&  (  !IsEndNode()
             || Branch()->Element()->_fExplicitEndTag
            )
       )
    {
        CGlyphRenderInfoType renderInfo;
        if (pRenderInfo == NULL)
            pRenderInfo = &renderInfo;
        pRenderInfo->pImageContext = NULL;
        fRet = Branch()->Doc()->GetTagInfo(this, GAT_COMPUTE, GPT_COMPUTE, GOT_COMPUTE, NULL, pRenderInfo) == S_OK;
        fRet = fRet && pRenderInfo->HasInfo();
    }
    else
        fRet = FALSE;
    return fRet;
}

CTreeNode *
CTreePos::SearchBranchForElement(CElement *pElement, BOOL fLeft)
{
    Assert(pElement);
    CTreePos *ptpLeft, *ptpRight;
    CTreeNode *pNode;

    // start at the requested gap
    if (fLeft)
    {
        ptpLeft = PreviousTreePos(),  ptpRight = this;
    }
    else
    {
        ptpLeft = this,  ptpRight = NextTreePos();
    }

    // move right to a valid gap (invalid gaps may give too high a scope)
    while (!IsLegalPosition(ptpLeft, ptpRight))
    {
        // but stop if we stumble across the element we're looking for
        if (ptpRight->IsNode() && ptpRight->Branch()->Element() == pElement)
        {
            return ptpRight->Branch();
        }

        ptpLeft = ptpRight;
        ptpRight = ptpLeft->NextTreePos();
    }

    // now search for a nearby node
    if (ptpLeft->IsNode())
    {
        pNode = ptpLeft->IsBeginNode() ? ptpLeft->Branch()
                                       : ptpLeft->Branch()->Parent();
    }
    else
    {
        while (!ptpRight->IsNode())
        {
            ptpRight = ptpRight->NextTreePos();
        }
        pNode = ptpRight->IsBeginNode() ? ptpRight->Branch()->Parent()
                                        : ptpRight->Branch();
    }

    // finally, look for the element along this node's branch
    for ( ; pNode; pNode = pNode->Parent())
    {
        if (pNode->Element() == pElement)
        {
            return pNode;
        }
    }

    return NULL;
}


long
CTreePos::SourceIndex()
{
    CTreePos *ptp;
    long cSourceIndex = GetElemLeft();
    BOOL fLeftChild = IsLeftChild();
    long cDepth = -1;
    CMarkup *pMarkup;
    CTreePos *pRoot = NULL;

    for (ptp = Parent();  ptp;  ptp = ptp->Parent())
    {
        if (!fLeftChild)
        {
            cSourceIndex += ptp->GetElemLeft() + (ptp->IsBeginElementScope()? 1: 0);
        }
        fLeftChild = ptp->IsLeftChild();

        ++cDepth;
        pRoot = ptp;
    }

    pMarkup = CONTAINING_RECORD(pRoot, CMarkup, _tpRoot);

    TraceTag((tagTreePosAccess, "%p: SourceIndex %ld  depth %ld",
            pMarkup, cSourceIndex, cDepth));

    if (pMarkup->ShouldSplay(cDepth))
    {
        Splay();
    }

    return cSourceIndex;
}

//
// Retunrs TRUE if this and ptpRight are separated only by
// pointers or empty text positions.  ptpRight must already
// be to the right of this.
//

BOOL
CTreePos::LogicallyEqual ( CTreePos * ptpRight )
{
    Assert( InternalCompare( ptpRight ) <= 0 );

    for ( CTreePos * ptp = this ;
          ptp->IsPointer() || (ptp->IsText() && ptp->Cch() == 0) ;
          ptp = ptp->NextTreePos() )
    {
        if (ptp == ptpRight)
            return TRUE;
    }

    return FALSE;
}
            
//
// Returns (not logically)
//
//   -1: this <  that
//    0: this == that
//   +1: this >  that
//

int
CTreePos::InternalCompare ( CTreePos * ptpThat )
{
    Assert( GetMarkup() && ptpThat->GetMarkup() == GetMarkup() );

    if (this == ptpThat)
        return 0;

    static long cSplayThis = 0;
    
    if (cSplayThis++ & 1)
    {
        CTreePos * ptpThis = this;

        ptpThis->Splay();

        for ( ; ; )
        {
            CTreePos * ptpChild = ptpThat;

            ptpThat = ptpThat->Parent();

            if (ptpThat == ptpThis)
                return ptpChild->IsLeftChild() ? +1 : -1;
        }
    }
    else
    {
        CTreePos * ptpThis = this;

        ptpThat->Splay();

        for ( ; ; )
        {
            CTreePos * ptpChild = ptpThis;

            ptpThis = ptpThis->Parent();

            if (ptpThis == ptpThat)
                return ptpChild->IsLeftChild() ? -1 : +1;
        }
    }
}

CTreeNode *
CTreePos::GetBranch ( ) const
{
    CTreePos * ptp = const_cast < CTreePos * > ( this );

    if (ptp->IsNode())
        return ptp->Branch();

    while ( ! ptp->IsNode() )
        ptp = ptp->PreviousTreePos();

    Assert( ptp && (ptp->IsBeginNode() || ptp->IsEndNode()) );

    return ptp->IsBeginNode() ? ptp->Branch() : ptp->Branch()->Parent();
}

CTreeNode *
CTreePos::GetInterNode ( ) const
{
    CTreePos * ptp = const_cast < CTreePos * > ( this );

    while ( ! ptp->IsNode() )
        ptp = ptp->PreviousTreePos();

    Assert( ptp && (ptp->IsBeginNode() || ptp->IsEndNode()) );

    return ptp->IsBeginNode() ? ptp->Branch() : ptp->Branch()->Parent();
}

long
CTreePos::GetCp( WHEN_DBG(BOOL fNoTrace) )
{
    CTreePos *ptp;
    long cch = _cchLeft;
    BOOL fLeftChild = IsLeftChild();
    long cDepth = -1;
    CMarkup *pMarkup;
    CTreePos *pRoot = NULL;

    for (ptp = Parent();  ptp;  ptp = ptp->Parent())
    {
        if (!fLeftChild)
            cch += ptp->_cchLeft + ptp->GetCch();
        
        fLeftChild = ptp->IsLeftChild();

        ++cDepth;
        pRoot = ptp;
    }

    pMarkup = CONTAINING_RECORD(pRoot, CMarkup, _tpRoot);
    
#if DBG==1
    if (!fNoTrace)
    {
        TraceTag((tagTreePosAccess, "%p: GetCp cp %ld  depth %ld",
                pMarkup, cch, cDepth));
    }
#endif

    if ( WHEN_DBG( !fNoTrace && ) pMarkup->ShouldSplay(cDepth))
        Splay();

    return cch;
}

int
CTreePos::Gravity() const
{
    Assert( IsPointer() );
    return DataThis()->p._dwPointerAndGravityAndCling & 0x1;
}

void
CTreePos::SetGravity ( BOOL fRight )
{
    Assert( IsPointer() );

    DataThis()->p._dwPointerAndGravityAndCling =
        (DataThis()->p._dwPointerAndGravityAndCling & ~1) | (!!fRight);
}

int
CTreePos::Cling() const
{
    Assert( IsPointer() );
    return !!(DataThis()->p._dwPointerAndGravityAndCling & 0x2);
}

void
CTreePos::SetCling ( BOOL fStick )
{
    Assert( IsPointer() );

    DataThis()->p._dwPointerAndGravityAndCling =
        (DataThis()->p._dwPointerAndGravityAndCling & ~2) | (fStick ? 2 : 0);
}

void
CTreePos::SetScopeFlags(BOOL fEdge)
{
    long cElemDelta = IsBeginElementScope()? -1 : 0;

    SetFlags((GetFlags() & ~TPF_EDGE) | BOOLFLAG(fEdge, TPF_EDGE));

    if (IsBeginElementScope())
    {
        cElemDelta += 1;
    }

    if (cElemDelta != 0)
    {
        CTreePos *ptp;
        BOOL fLeftChild = IsLeftChild();

        for (ptp=Parent(); ptp; ptp = ptp->Parent())
        {
            if (fLeftChild)
            {
                ptp->AdjElemLeft(cElemDelta);
            }
            fLeftChild = ptp->IsLeftChild();
        }
    }
}


void
CTreePos::ChangeCch(long cchDelta)
{
    BOOL fLeftChild = IsLeftChild();
    CTreePos *ptp;
#if DBG==1
    CTreePos *pRoot = NULL;
    CMarkup *pMarkup;
    long cDepth = -1;
#endif

    Assert( IsText() );

    DataThis()->t._cch += cchDelta;

    for (ptp=Parent(); ptp; ptp = ptp->Parent())
    {
        if (fLeftChild)
            ptp->_cchLeft += cchDelta;
        
        fLeftChild = ptp->IsLeftChild();

        WHEN_DBG( ++cDepth; )
        WHEN_DBG( pRoot = ptp; )
    }

    WHEN_DBG( pMarkup = CONTAINING_RECORD(pRoot, CMarkup, _tpRoot); )
    TraceTag((tagTreePosOps, "%p: ChangeCch by %ld to %ld", pMarkup, cchDelta, Cch()));
    TraceTag((tagTreePosAccess, "%p: ChangeCch depth %ld", pMarkup, cDepth));
}

void
CTreePos::Splay()
{
    CTreePos *p=Parent(), *g=p->Parent(), *gg;   // parent, grandparent, great-grandparent
#if DBG==1
    CTreePos tpValid(TRUE);
    long nZigZig=0, nZigZag=0, nZig=0;
    CMarkup *pMarkup;
#endif

    for (; g; p=Parent(), g=p->Parent())
    {
        gg = g->Parent();

        if (gg)
        {
            if (IsLeftChild() == p->IsLeftChild())      // zig-zig
            {
                p->RotateUp(g, gg);
                RotateUp(p, gg);
                WHEN_DBG( ++ nZigZig; )
            }
            else                                    // zig-zag
            {
                RotateUp(p, g);
                RotateUp(g, gg);
                WHEN_DBG( ++ nZigZag; )
            }
        }
        else                                        // zig
        {
            RotateUp(p, g);
            WHEN_DBG( ++ nZig; )
        }
    }

    WHEN_DBG( pMarkup = CONTAINING_RECORD(p, CMarkup, _tpRoot); )
    TraceTag((tagTreePosSplay, "%p: Splay  depth=%ld (%ld, %ld, %ld)",
                pMarkup, 2*(nZigZig+nZigZag) + nZig, nZigZig, nZigZag, nZig));

#if DBG==1
    if (IsTagEnabled(tagTreePosValidate))
        Assert(IsSplayValid(&tpValid));
#endif
}


void
CTreePos::RotateUp(CTreePos *p, CTreePos *g)
{
    WHEN_DBG( CTreePos tpValid(TRUE); )
    CTreePos *ptp1, *ptp2, *ptp3;

    if (IsLeftChild())                    // rotate right
    {
        GetChildren(&ptp1, &ptp2);
        ptp3 = p->RightChild();
        g->ReplaceChild(p, this);

        // recreate my family
        if (ptp1)
        {
            ptp1->MarkFirst();
            ptp1->SetNext(p);
        }
        else
        {
            SetFirstChild(p);
        }

        // recreate p's family
        if (ptp2)
        {
            p->SetFirstChild(ptp2);
            ptp2->MarkLeft();
            if (ptp3)
            {
                ptp2->MarkFirst();
                ptp2->SetNext(ptp3);
            }
            else
            {
                ptp2->MarkLast();
                ptp2->SetNext(p);
            }
        }
        else
        {
            p->SetFirstChild(ptp3);
        }
        p->MarkRight();
        p->MarkLast();
        p->SetNext(this);

        // adjust cumulative counts
        p->DecreaseCounts(this, TP_BOTH);
    }

    else                                    // rotate left
    {
        ptp1 = p->LeftChild();
        GetChildren(&ptp2, &ptp3);
        g->ReplaceChild(p, this);

        // recreate my family
        SetFirstChild(p);
        p->MarkLeft();
        if (ptp3)
        {
            p->MarkFirst();
            p->SetNext(ptp3);
        }
        else
        {
            p->MarkLast();
            p->SetNext(this);
        }

        // recreate p's family
        if (ptp1)
        {
            if (ptp2)
            {
                ptp1->MarkFirst();  // BUGBUG not needed?
                ptp1->SetNext(ptp2);
            }
            else
            {
                ptp1->MarkLast();
                ptp1->SetNext(p);
            }
        }
        else
        {
            p->SetFirstChild(ptp2);
        }
        if (ptp2)
        {
            ptp2->MarkRight();
            ptp2->MarkLast();
            ptp2->SetNext(p);
        }

        // adjust cumulative counts
        IncreaseCounts(p, TP_BOTH);
    }

#if DBG==1
    if (IsTagEnabled(tagTreePosValidate))
        Assert(IsSplayValid(&tpValid));
#endif
}


void
CTreePos::ReplaceChild(CTreePos *pOld, CTreePos *pNew)
{
    pNew->MarkLeft(pOld->IsLeftChild());
    pNew->MarkLast(pOld->IsLastChild());
    pNew->SetNext(pOld->Next());

    if (FirstChild() == pOld)
        SetFirstChild(pNew);
    else
        FirstChild()->SetNext(pNew);
}


void
CTreePos::RemoveChild(CTreePos *pOld)
{
    if (FirstChild() == pOld)
    {
        SetFirstChild(pOld->IsLastChild() ? NULL : pOld->Next());
    }
    else
    {
        FirstChild()->MarkLast();
        FirstChild()->SetNext(this);
    }
}


/////////////////////////////////////////////////////////////////////////
//          CMarkup stuff
/////////////////////////////////////////////////////////////////////////

HRESULT
CMarkup::DestroySplayTree( BOOL fReinit )
{
    HRESULT     hr = S_OK;
    CTreePos *  ptp, *ptpNext;
    CTreeNode * pDelayReleaseList = NULL;
    CTreeNode * pExitTreeScList = NULL;
    CNotification   nfExit;

    // Prime the exittree notification
    nfExit.ElementExittree1(NULL);
    Assert( nfExit.IsSecondChanceAvailable() );

    //
    // Unposition any unembedded markup pointers.  Because there is
    // no pointer pos for these pointers, they will not get notified.
    //

    while ( _pmpFirst )
    {
        hr = THR( _pmpFirst->Unposition() );

        if (hr)
            goto Cleanup;
    }
            
    //
    // The walk used here is destructive.  This is needed
    // because once a CTreePos is released (via ReleaseContents)
    // we can't use it any more.  So before we release a pos,
    // we short circuit other pointers in the tree to not point
    // to it any more.  This way, we only visit (and release) each
    // node only once.
    //

    // set a sentinel value to make the traversal end properly
    if (_tpRoot.FirstChild())
    {
        _tpRoot.FirstChild()->SetNext( NULL );
    }

    // release the tree
    for (ptp=_tpRoot.FirstChild(); ptp; )
    {
        // figure out the next to be deleted
        if (ptp->FirstChild())
        {
            ptpNext = ptp->FirstChild();

            // this is the short circuit
            if( ptpNext->IsLastChild() )
            {
                ptpNext->SetNext( ptp->Next() );
            }
            else
            {
                Assert( ptpNext->Next()->IsLastChild() );
                ptpNext->Next()->SetNext( ptp->Next() );
            }
        }
        else
        {
            ptpNext = ptp->Next();
        }

        if( ptp->IsNode() )
        {
            CTreeNode *     pNode = ptp->Branch();

            ReleaseTreePos( ptp, TRUE );

            if (!pNode->_fInMarkupDestruction)
            {
                pNode->_fInMarkupDestruction = TRUE;
            }
            else
            {
                BOOL    fDelayRelease = FALSE;
                BOOL    fExitTreeSc = FALSE;

                if (pNode->IsFirstBranch())
                {
                    // Prepare and send the exit tree notification
                    CElement *      pElement = pNode->Element();
                    DWORD           dwData = EXITTREE_DESTROY;
                    WHEN_DBG( BOOL fPassivatePending = FALSE );

                    if( pElement->GetObjectRefs() == 1 )
                    {
                        dwData |= EXITTREE_PASSIVATEPENDING;
                        Assert( !pElement->_fPassivatePending );
                        WHEN_DBG( pElement->_fPassivatePending = TRUE );
                        WHEN_DBG( fPassivatePending = TRUE );
                    }

                    pElement->_fExittreePending = TRUE;

                    // NOTE: we may want to bump up the notification SN here
                    nfExit.SetElement(pElement);
                    nfExit.SetData( dwData );
                    pElement->Notify(&nfExit);

                    pElement->_fExittreePending = FALSE;

                    // Check to see if we have to delay release
                    fDelayRelease = nfExit.DataAsDWORD() & EXITTREE_DELAYRELEASENEEDED;

                    // Check to see if we have to send ElementExittreeSc
                    fExitTreeSc = nfExit.IsSecondChanceRequested();

                    if (!fDelayRelease && !fExitTreeSc)
                    {
                        // The node is now no longer in the tree so release
                        // the tree's ref on the node
                        pNode->PrivateExitTree();
                    }
                    else
                    {
                        // We will use the node as an item on our delay release
                        // linked list.  Make it dead and use useless fields
                        pNode->PrivateMakeDead();
                        pNode->GetBeginPos()->_pNext = (CTreePos*)pDelayReleaseList;
                        pDelayReleaseList = pNode;
                        pElement->AddRef();

                        WHEN_DBG( pElement->_fDelayRelease = TRUE );
                        WHEN_DBG( pElement->_fPassivatePending = FALSE );
                        WHEN_DBG( fPassivatePending = FALSE );

                        // If we need an exit tree second chance, link it into that list.
                        if( fExitTreeSc )
                        {
                            pNode->GetBeginPos()->_pFirstChild = (CTreePos*)pExitTreeScList;
                            pExitTreeScList = pNode;

                            nfExit.ClearSecondChanceRequested();
                        }
                    }

                    // Remove the element from the tree
                    pElement->__pNodeFirstBranch = NULL;
                    pElement->DelMarkupPtr();
                    pElement->PrivateExitTree(this);

#if DBG==1
                    if (fDelayRelease)
                    {
                        pElement->_fPassivatePending = fPassivatePending;
                    }
#endif

                }
                else
                {
                    // The node is now no longer in the tree so release
                    // the tree's ref on the node
                    pNode->PrivateExitTree();
                }
            }
        }
        else
        {
            ReleaseTreePos( ptp, TRUE );
        }

        ptp = ptpNext;
    }

    // release all the TreePos pools
    while (_pvPool)
    {
        void *pvNextPool = *((void **)_pvPool);

        if (_pvPool != & (_abPoolInitial[0]))
        {
            MemFree(_pvPool);
        }
        _pvPool = pvNextPool;
    }

    memset( & _tpRoot, 0, sizeof( CTreePos ) );
    _ptpFirst = NULL;
    Assert( _pvPool == NULL );
    _ptdpFree = NULL;
    memset( &_abPoolInitial, 0, sizeof( _abPoolInitial ) );

    _tpRoot.MarkLast();
    _tpRoot.MarkRight();

    WHEN_DBG( _cchTotalDbg = 0 );
    WHEN_DBG( _cElementsTotalDbg = 0 );


    if (fReinit)
    {
        hr = CreateInitialMarkup( _pElementRoot );
        if (hr)
        {
            _pElementRoot = NULL;
            goto Cleanup;
        }
    }
    else
    {
        _pElementRoot = NULL;
    }

    // Release and delete the text frag context
    delete DelTextFragContext();
    delete DelTopElemCache();

    UpdateMarkupTreeVersion();

    // Send all of the exit tree second chance notifications that we need to
    if (pExitTreeScList)
    {
        nfExit.ElementExittree2( NULL );
    }

    while( pExitTreeScList )
    {
        CElement *      pElement = pExitTreeScList->Element();

        // Get the next link in the list
        pExitTreeScList = (CTreeNode*)pExitTreeScList->GetBeginPos()->_pFirstChild;

        // Send the after exit tree notifications
        // NOTE: we may want to bump up the notification SN here
        nfExit.SetElement( pElement );
        pElement->Notify(&nfExit);
    }

    // Release all of the elements (and nodes) on our delay release list
    while( pDelayReleaseList )
    {
        CElement * pElement = pDelayReleaseList->Element();
        CTreeNode *pNode = pDelayReleaseList;

        // Get the next link in the list
        pDelayReleaseList = (CTreeNode*)pDelayReleaseList->GetBeginPos()->_pNext;

        // Release the element
        Assert( pElement->_fDelayRelease );
        WHEN_DBG( pElement->_fDelayRelease = FALSE );
        Assert( !pElement->_fPassivatePending || pElement->GetObjectRefs() == 1 );
        pElement->Release();

        // Release any hold the markup has on the node
        pNode->PrivateMarkupRelease();
    }


Cleanup:
    RRETURN( hr );
}

#pragma optimize("", on)

#if DBG==1

BOOL
CMarkup::IsSplayValid() const
{
    CTreePos tpValid(TRUE);
    BOOL result = _tpRoot.IsSplayValid(&tpValid);

    if (result)
    {
        Assert(result = (_ptpFirst == _tpRoot.LeftmostDescendant()));
    }

    return result;
}

#endif

//-----------------------------------------------------------------------------
//
//  Notification Methods
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Send a notification through the tree
//
//  Arguments:  pnf - The notification to send
//
//-----------------------------------------------------------------------------
void
CMarkup::Notify(
    CNotification * pnf )
{
    Assert(pnf);

#if DBG == 1
    {
        BOOL    fQueue = ( _fInSendAncestor && pnf->SendTo(NFLAGS_ANCESTORS));

        TraceTagEx((tagNotify, TAG_NONAME,
               "Notify    : (%d, %S) Element(0x%x,%S) Node(0x%x) cp(%d) cch(%d) %S",
               pnf->_sn,
               pnf->Name(),
               pnf->_pElement,
               (pnf->_pElement
                    ? pnf->_pElement->TagName()
                    : _T("")),
               pnf->_pNode,
               pnf->_cp,
               pnf->_cch,
               (fQueue
                    ? _T("QUEUED")
                    : _T(""))));
    }
#endif

    //
    //  If the branch was not supplied, infer it from the affected element
    //

    if (    !pnf->_pNode
        &&  pnf->_pElement)
    {
        pnf->_pNode = pnf->_pElement->GetFirstBranch();
    }

    Assert( pnf->_pNode
        ||  pnf->_pElement
        ||  pnf->IsTreeChange()
        ||  (   pnf->SendTo(NFLAGS_DESCENDENTS)
            &&  pnf->IsRangeValid()));
/*
BUGBUG: These asserts fire when clearing the undo-stack. We need to fix that and
        enable these asserts. (brendand)
    Assert( !pnf->Node()
        ||  pnf->Node()->IsInMarkup());
    Assert( !pnf->Node()
        ||  pnf->Node()->GetMarkup() == this);
    Assert( !pnf->Element()
        ||  pnf->Element()->IsInMarkup());
    Assert( !pnf->Element()
        ||  pnf->Element()->GetMarkup() == this);
*/
    Assert( !pnf->SendTo(NFLAGS_ANCESTORS)
        ||  !pnf->SendTo(NFLAGS_DESCENDENTS));

    //
    //  Mark the document as dirty is requested
    //

    if (!pnf->IsFlagSet(NFLAGS_CLEANCHANGE))
    {
        SetModified();
    }

#if DBG==1
    //
    // Need to validate the change synchronously when the change is fired
    // (That is, the check cannot be queued since ValidateChange code can only handle
    //  one change at a time)
    //
    
    if (    pnf->IsTextChange()
        ||  pnf->IsTreeChange())
    {
        ValidateChange(pnf);
    }
#endif

    //
    //  If a notification is blocking the ancestor direction, queue the incoming notification
    //

    if (_fInSendAncestor && pnf->SendTo(NFLAGS_ANCESTORS))
    {
        if (!pnf->IsFlagSet(NFLAGS_SYNCHRONOUSONLY))
        {
            if (pnf->Element())
            {
                pnf->Element()->AddRef();
                pnf->SetFlag(NFLAGS_NEEDS_RELEASE);
            }

            _aryANotification.AppendIndirect(pnf);
        }
    }
    else
    {
        CDataAry<CNotification> *   paryNotification = pnf->SendTo(NFLAGS_ANCESTORS)
                                                            ? (CDataAry<CNotification> *)&_aryANotification
                                                            : NULL;

        if (!pnf->IsFlagSet(NFLAGS_DONOTBLOCK))
        {
            if (pnf->SendTo(NFLAGS_ANCESTORS))
            {
                _fInSendAncestor = TRUE;
            }
        }

        SendNotification(pnf, paryNotification);

        Assert( !_fInSendAncestor   || (paryNotification != &_aryANotification));
    }
    return;
}

//-----------------------------------------------------------------------------
//
//  Member:     ElementWantsNotification
//
//  Synopsis:   Check if an element may want to receive a notification
//
//  Arguments:  pElement - Element to notify
//              pnf      - Notification to send
//
//-----------------------------------------------------------------------------
inline BOOL
CMarkup::ElementWantsNotification(
    CElement *      pElement,
    CNotification * pnf)
{
    return  !pElement->_fExittreePending
        &&  (   pnf->IsForAllElements()
            ||  (    pElement->HasLayout()
                &&  (   pnf->IsTextChange()
                    ||  pnf->IsTreeChange()
                    ||  pnf->IsLayoutChange()
                    ||  pnf->IsForLayouts()
                    ||  (   pnf->IsForPositioned()
                        &&  (   pElement->IsZParent()
                            ||  pElement->GetLayoutPtr()->_fContainsRelative))
                    ||  (   pnf->IsForWindowed()
                        &&  pElement->GetHwnd())))
            ||  (   (   IsPositionNotification(pnf)
// BUGBUG: Rework the categories to include the following:
//         1) text change
//         2) tree change
//         3) layout change
//         4) display change
//         Then, instead of testing for NTYPE_VISIBILITY_CHANGE etc., test for
//         display changes
                    ||  pnf->IsType(NTYPE_VISIBILITY_CHANGE))
                &&  !pElement->IsPositionStatic())
            ||  (   pnf->IsForActiveX()
                &&  pElement->TestClassFlag(CElement::ELEMENTDESC_OLESITE)));
}


//-----------------------------------------------------------------------------
//
//  Member:     NotifyElement
//
//  Synopsis:   Notify a element of a notification
//
//  Arguments:  pElement - Element to notify
//              pnf      - Notification to send
//
//-----------------------------------------------------------------------------
inline void
CMarkup::NotifyElement(
    CElement *      pElement,
    CNotification * pnf)
{
    Assert(pElement);
    Assert(ElementWantsNotification(pElement, pnf));
    Assert(!pnf->IsFlagSet(NFLAGS_SENDENDED));

    //
    //  If a layout exists, pass it all notifications except those meant for ActiveX elements
    //  NOTE: Notifications passed to a layout may not also sent to the element
    //

    if (    pElement->HasLayout()
        &&  !pnf->IsForActiveX())
    {
        CLayout * pLayout = pElement->GetCurLayout();

        Assert(pLayout);

        TraceTagEx((tagNotifyPath, TAG_NONAME,
                   "NotifyPath: (%d) sent to pLayout(0x%x, %S)",
                   pnf->_sn,
                   pLayout,
                   pElement->TagName()));

        pLayout->Notify(pnf);

        if (    pnf->IsFlagSet(NFLAGS_SENDUNTILHANDLED)
            &&  pnf->IsHandled())
        {
            pnf->SetFlag(NFLAGS_SENDENDED);
        }
    }

    //
    //  If not handled, hand appropriate notifications to the element
    //
    // (KTam) These look like the same conditions that ElementWantsNotification()
    // checks.  Why are we checking them again when we've already asserted that
    // ElementWantsNotification is true?

    if (    !pnf->IsFlagSet(NFLAGS_SENDENDED)
        &&  (   pnf->IsForAllElements()
            ||  (   pnf->IsForActiveX()
                &&  pElement->TestClassFlag(CElement::ELEMENTDESC_OLESITE))
            ||  (   (   IsPositionNotification(pnf)
// BUGBUG: Rework the categories to include the follownig:
//         1) text change
//         2) tree change
//         3) layout change
//         4) display change
//         Then, instead of testing for NTYPE_VISIBILITY_CHANGE etc., test for
//         display changes
                    ||  (   pnf->IsType(NTYPE_VISIBILITY_CHANGE)
                        &&  !pElement->NeedsLayout()))
                &&  !pElement->IsPositionStatic())
            )
       )
    {
        TraceTagEx((tagNotifyPath, TAG_NONAME,
                   "NotifyPath: (%d) sent to pElement(0x%x, %S)",
                   pnf->_sn,
                   pElement,
                   pElement->TagName()));

        pElement->Notify(pnf);

        if (    pnf->IsFlagSet(NFLAGS_SENDUNTILHANDLED)
            &&  pnf->IsHandled())
        {
            pnf->SetFlag(NFLAGS_SENDENDED);
        }
    }
}


//-----------------------------------------------------------------------------
//
//  Member:     NotifyAncestors
//
//  Synopsis:   Notify all ancestors of a CTreeNode
//
//  Arguments:  pnf - The notification to send
//
//-----------------------------------------------------------------------------
#if DBG==1
void
#else
inline void
#endif
CMarkup::NotifyAncestors(
    CNotification * pnf)
{
    CTreeNode * pNodeBranch;
    CTreeNode * pNode;

    Assert(pnf);
    Assert(pnf->_pNode);

    for (pNodeBranch = pnf->_pNode;
        pNodeBranch;
//  BUGBUG: We may eventually want this routine should distribute the notification to all parents of the element.
//          Unfortunately, doing so now is too problematic (since layouts cannot handle receiving the same
//          notification multiple times). (brendand)
//        pNodeBranch = pNodeBranch->NextBranch())
        pNodeBranch = NULL)
    {
        pNode = pNodeBranch->Parent();

        while ( pNode
            &&  !pnf->IsFlagSet(NFLAGS_SENDENDED))
        {
            CElement *  pElement = pNode->Element();

            if (ElementWantsNotification(pElement, pnf))
            {
                pElement->AddRef();

                NotifyElement(pElement, pnf);

                if (!pElement->IsInMarkup())
                {
                    pNode = NULL;
                }
                
                pElement->Release();
            }

            if (pNode)
            {
                pNode = pNode->Parent();
            }
        }
    }
}


//-----------------------------------------------------------------------------
//
//  Member:     NotifyDescendents
//
//  Synopsis:   Notify decendents of a CTreeNode or range
//
//  Arguments:  pnf - The notification to send
//
//-----------------------------------------------------------------------------

MtDefine(BroadcastNotify, Metrics, "Full Tree Notifications");

#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

void
CMarkup::NotifyDescendents( CNotification * pnf )
{
    CStackPtrAry < CElement *, 32 > aryElements( Mt( Mem ) );

    Assert( pnf->SendTo(NFLAGS_DESCENDENTS) );

    CTreePos *  ptp;
    CTreePos *  ptpEnd;
    WHEN_DBG( CTreeNode * pNodeEnd );

#if DBG==1
    // Descendent notifications are order exempt so set their sn to 0xFFFFFFFF
    DWORD   snBefore = pnf->SerialNumber();
    pnf->_sn = (DWORD)-1;
#endif

    BOOL        fZParentsOnly = pnf->IsFlagSet( NFLAGS_ZPARENTSONLY );
    BOOL        fSCAvail = pnf->IsSecondChanceAvailable();

    Assert(pnf);
    Assert(  pnf->Element() || pnf->IsRangeValid() );
    Assert( !pnf->Element() || pnf->Element()->IsInMarkup() );
    Assert( !pnf->Element() || pnf->Element()->GetMarkup() == this );

    //
    //  Determine the range
    //

    if (pnf->Element())
    {
        pnf->Element()->GetTreeExtent( & ptp, & ptpEnd );

        Assert( ptp );
        
        ptp = ptp->NextTreePos();

        if (pnf->Element() == Root() && !fSCAvail)
            aryElements.EnsureSize( NumElems() );
    }
    else
    {
        long cpStart, cpEnd, ich;

        Assert( pnf->IsRangeValid() );

        cpStart = pnf->Cp(0);
        cpEnd   = cpStart + pnf->Cch(LONG_MAX);

        ptp     = TreePosAtCp( cpStart, & ich );
        ptpEnd  = TreePosAtCp( cpEnd, & ich );
    }

    Assert( ptp );
    Assert( ptpEnd );
    Assert( ptp->InternalCompare( ptpEnd ) <= 0 );

    WHEN_DBG( pNodeEnd = CTreePosGap( ptpEnd, TPG_LEFT ).Branch(); )

    //
    //  Build a list of target elements
    //  (This allows the tree to change shape as the notification is delivered to each target)
    //

    while ( ptp && 
            ptp != ptpEnd )
    {
        if (ptp->IsBeginElementScope())
        {
            CTreeNode * pNode    = ptp->Branch();
            CElement *  pElement = pNode->Element();

            //
            //  Remember the element if it may want the notification
            //

            if (ElementWantsNotification( pElement, pnf ))
            {
                if (fSCAvail)
                {
                    if (!pnf->IsFlagSet( NFLAGS_SENDENDED ))
                        NotifyElement(pElement, pnf);

                    if (pnf->IsSecondChanceRequested())
                    {
                        HRESULT hr;

                        hr = aryElements.Append( pElement );
                        if (hr)
                            goto Cleanup;

                        pElement->AddRef();

                        pnf->ClearSecondChanceRequested();
                    }
                }
                else
                {
                    HRESULT hr;

                    hr = aryElements.Append( pElement );
                    if (hr)
                        goto Cleanup;

                    pElement->AddRef();
                }
            }

            //
            //  Skip over z-parents (if requested)
            //

            if (fZParentsOnly)
            {
                BOOL fSkipElement;

                fSkipElement =  pNode->IsZParent()
                            ||  (   pnf->IsFlagSet(NFLAGS_AUTOONLY)
                                &&  pElement->HasLayout()
                                &&  !pElement->GetCurLayout()->_fAutoBelow);

                //
                // If the element should be skipped, then advance past it
                // (If the element has layout that overlaps with another layout that was not
                //  skipped, then this routine will only skip the portion contained in the
                //  non-skipped layout)
                //

                if (fSkipElement)
                {
                    Assert( !pNodeEnd->SearchBranchToRootForScope( pElement ) );

                    for ( ; ; )
                    {
                        CElement * pElementInner;

                        //
                        //  Get the ending treepos,
                        //  stop if it is for the end of the element
                        //

                        ptp = pNode->GetEndPos();
                        
                        Assert( ptp->IsEndNode() );

                        if (ptp->IsEdgeScope())
                            break;

                        //
                        //  There is overlap, locate the beginning of the next section
                        //  (Tunnel through the inclusions stopping at either the next begin
                        //   node for the current element, the end of the range, or if an
                        //   element with layout ends)
                        //

                        do
                        {
                            ptp           = ptp->NextTreePos();
                            pElementInner = ptp->Branch()->Element();
                            Assert(!ptp->IsEdgeScope()
                                ||  ptp->IsEndNode());
                        }
                        while ( pElementInner != pElement
                            &&  ptp != ptpEnd
                            &&  !(  ptp->IsEdgeScope()
                                &&  pElementInner->HasLayout()));

                        //
                        //  If range ended, stop all searching
                        //

                        if (ptp == ptpEnd)
                            goto Cleanup;

                        //
                        //  If the end of an element with layout was encountered,
                        //  treat that as the end of the skipped over element
                        //

                        Assert( !ptp->IsEdgeScope()
                            ||  (   ptp->IsEndNode()
                                &&  pElementInner->HasLayout()));

                        if (ptp->IsEdgeScope())
                            break;

                        //
                        //  Otherwise, reset the treenode and skip over the section of the element
                        //

                        Assert( ptp->IsBeginNode() && !ptp->IsEdgeScope() );

                        pNode = ptp->Branch();

                        Assert( pNode->Element() == pElement );
                    }
                }
            }
        }

        ptp = ptp->NextTreePos();
    }

    //
    //  Deliver the notification
    //

Cleanup:
    
    if (aryElements.Size())
    {
        CElement **     ppElement;
        int             c;
        CNotification   nfSc;
        CNotification * pnfSend;

        if (fSCAvail)
        {
            nfSc.InitializeSc( pnf );
            pnfSend = &nfSc;
        }
        else
        {
            pnfSend = pnf;
        }

        for (   c = aryElements.Size(), ppElement = & (aryElements[0] );
                c > 0;
                c--, ppElement++ )
        {
            Assert( ppElement && *ppElement );

            if (!pnf->IsFlagSet( NFLAGS_SENDENDED ))
                NotifyElement(*ppElement, pnfSend);

            (*ppElement)->Release();
        }
    }

#if DBG==1
    // Reset the serial number on the notification
    pnf->_sn = snBefore;
#endif

#ifdef PERFMETER
    if (pnf->Element() == Root())
        MtAdd( Mt(BroadcastNotify), 1, aryElements.Size() );
#endif

}

#if DBG != 1
#pragma optimize("", on)
#endif


//-----------------------------------------------------------------------------
//
//  Member:     NotifyTreeLevel
//
//  Synopsis:   Notify objects listening at the tree level
//
//  Arguments:  pnf - Text change notification
//
//-----------------------------------------------------------------------------
void
CMarkup::NotifyTreeLevel(
    CNotification * pnf)
{
    Assert(!pnf->IsFlagSet(NFLAGS_SENDENDED));

    if (pnf->Type() == NTYPE_DISPLAY_CHANGE)
    {
        CTreeNode *pNode = pnf->Node();
        while (pNode)
        {
            CElement *pElement = pNode->Element();
            if (pElement->HasFlag(TAGDESC_LIST))
            {
                DYNCAST(CListElement, pElement)->UpdateVersion();
                break;
            }
            pNode = pNode->Parent();
        }
    }
    else
    {
        //
        //  Notify the view of all layout changes
        //

        if (    !pnf->IsTextChange()
            &&  (   pnf->IsLayoutChange()
                ||  pnf->IsForLayouts()))
        {
            Doc()->GetView()->Notify(pnf);
        }

        //
        //  Forward the notification to the master markup (if it exists)
        //

//  BUGBUG: Two changes are needed here. First, the notification should be altered to contain
//          the "master" element prior to forwarding. Second, category flags should be added
//          to efficiently filter notifications that need forwarding. (It's also reasonable
//          to imagine conversion flags that say how some forwarded notifications should change
//          prior to forwarding - e.g., some changes become a resize.) Unfortunately, these
//          changes are too de-stabilizing to do at this time. (brendand)
        if (    Master()
            &&  Master()->IsInMarkup()
            &&  !pnf->IsType(NTYPE_VIEW_ATTACHELEMENT)
            &&  !pnf->IsType(NTYPE_VIEW_DETACHELEMENT))
        {
            Master()->GetUpdatedLayout()->Notify(pnf);
        }
    }
    
#if MARKUP_DIRTYRANGE
    //
    //  Update the dirty range for any listeners
    //

    if (    pnf->IsTextChange()
        &&  HasDirtyRangeContext())
    {
        CMarkupDirtyRangeContext *  pdrc = GetDirtyRangeContext();
        MarkupDirtyRange *          pdr  = pdrc->_aryMarkupDirtyRange;
        int                         cdr  = pdrc->_aryMarkupDirtyRange.Size();
        
        for( ; cdr; cdr--, pdr++)
        {
// BUGBUG: Should this use GetContentFirstCp? (brendand)
            pdr->_dtr.Accumulate(pnf, 0, GetContentLastCp(), FALSE);
        }

        if (    pdrc->_aryMarkupDirtyRange.Size()
            &&  !_fOnDirtyRangeChangePosted )
        {
            _fOnDirtyRangeChangePosted = TRUE;
            IGNORE_HR(GWPostMethodCall(this, ONCALL_METHOD(CMarkup, OnDirtyRangeChange, ondirtyrangechange), 0, TRUE, "CMarkup::OnDirtyRangeChange"));
        }
    }
#endif
}


//-----------------------------------------------------------------------------
//
//  Member:     SendNotification
//
//  Synopsis:   Send the current notification and any subsequent notifications
//              which might result
//
//  Arguments:  pnf              - The notification to send
//              paryNotification - Queue of pending notifications (may be NULL)
//
//-----------------------------------------------------------------------------
void
CMarkup::SendNotification(
    CNotification *             pnf,
    CDataAry<CNotification> *   paryNotification)
{
    CNotification   nf;
    int             iRequest = 0;

    for (;;)
    {
        Assert(pnf);

        BOOL        fNeedsRelease = pnf->IsFlagSet(NFLAGS_NEEDS_RELEASE);
        CElement *  pElement      = pnf->_pElement;

        //
        //  Send the notification to "self"
        //

        if (pnf->SendTo(NFLAGS_SELF))
        {
            CElement *  pElementSelf = pElement
                                            ? pElement
                                            : pnf->_pNode->Element();

            Assert(pElementSelf);
            if (ElementWantsNotification(pElementSelf, pnf))
            {
                NotifyElement(pElementSelf, pnf);
            }
        }

        //
        //  If sending to ancestors or descendents and a starting node or range exists,
        //  Send the notification to "ancestors" and "descendents"
        //

        if (    !pnf->IsFlagSet(NFLAGS_SENDENDED)
            &&  (   pnf->_pNode
                ||  pnf->IsRangeValid()))
        {
            if (pnf->SendTo(NFLAGS_ANCESTORS))
            {
                Assert(pnf->_pNode);
                NotifyAncestors(pnf);
            }
            else if (pnf->SendTo(NFLAGS_DESCENDENTS))
            {
                NotifyDescendents(pnf);
            }
        }

        //
        //  Send the notification to listeners at the "tree" level
        //

        if (    !pnf->IsFlagSet(NFLAGS_SENDENDED)
            &&  pnf->SendTo(NFLAGS_TREELEVEL))
        {
            NotifyTreeLevel(pnf);
        }

        //
        //  Release the element (if necessary)
        //  (Elements are AddRef'd only when the associated notification is delayed)
        //

        if (fNeedsRelease)
        {
            Assert(pElement);
            Assert(paryNotification);
            Assert(iRequest <= (*paryNotification).Size());
            WHEN_DBG((*paryNotification)[iRequest-1].ClearFlag(NFLAGS_NEEDS_RELEASE));
            pElement->Release();
        }

        //
        //  Leave or fetch the next notification to send
        //  (Copy the notification into a local in case others should arrive
        //   and cause re-allocation of the array)
        //

        if (    !paryNotification
            ||  iRequest >= (*paryNotification).Size())
            goto Cleanup;

        nf  = (*paryNotification)[iRequest++];
        pnf = &nf;

        //
        //  Ensure notifications no longer part of this markup are sent only to self
        //  (Previously sent notifications can initiate changes such that the elements
        //   of pending notifications are no longer part of this markup)
        //

        if (pnf->Element())
        {
            CMarkup *   pMarkup = pnf->Element()->GetMarkupPtr();

//  BUGBUG: Unfortunately, notifications forwarded from nested markups contain the element
//          from the nested markup, hence the more complex check. What should be is, when
//          a notification from a nested markup is forwarded (by CMarkup::NotifyTreeLevel)
//          the contained element et. al. should be changed to the "master" element.
//          This change cannot be made right now because it is potentially de-stabilizing. (brendand)

            if (    !pMarkup
                ||  (   pMarkup != this
                    &&  !pMarkup->Master())
                ||  (   pMarkup != this
                    &&  pMarkup->Master()
                    &&  pMarkup->Master()->GetMarkupPtr() != this))
            {
                pnf->_grfFlags &= ~NFLAGS_TARGETMASK | NFLAGS_SELF;
            }
        }
    }

Cleanup:
    if (paryNotification)
    {
#if DBG==1
        for (int i=0; i < (*paryNotification).Size(); i++)
            Assert(!(*paryNotification)[i].IsFlagSet(NFLAGS_NEEDS_RELEASE));
#endif

        (*paryNotification).DeleteAll();

        if (paryNotification == &_aryANotification)
        {
            _fInSendAncestor = FALSE;
        }
    }
}

HRESULT
CMarkup::SetTextPosID( CTreePos ** pptpText, long lTextID )
{
    HRESULT     hr = S_OK;

    Assert( pptpText && *pptpText );

    if( ! (*pptpText)->IsData2Pos() )
    {
        CTreePos *  ptpRet;

        ptpRet = AllocData2Pos();
        if( !ptpRet )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        ptpRet->SetType( CTreePos::Text );
        *(LONG *)&ptpRet->DataThis()->t = *(LONG *)&(*pptpText)->DataThis()->t;

        hr = THR( ReplaceTreePos( *pptpText, ptpRet ) );
        if (hr)
            goto Cleanup;

        *pptpText = ptpRet;
    }

    (*pptpText)->DataThis()->t._lTextID = lTextID;

Cleanup:
    RRETURN( hr );
}

CTreePos *
CMarkup::AllocData1Pos()
{
    CTreeDataPos *ptdpNew;

    if (!_ptdpFree)
    {
        void *          pvPoolNew;
        size_t          cPoolSize;

        if (!_pvPool)
        {
            pvPoolNew = (void*)(_abPoolInitial);
            cPoolSize = INITIAL_TREEPOS_POOL_SIZE;
        }
        else
        {
            cPoolSize = s_cTreePosPoolSize;
            pvPoolNew = MemAllocClear(
                            Mt(CMarkup_pvPool), 
                            sizeof(void*) + TREEDATA1SIZE * s_cTreePosPoolSize);
        }

        if (pvPoolNew)
        {
            CTreeDataPos *ptdp;
            int i;

            for (ptdp = (CTreeDataPos*)((void**)pvPoolNew + 1), i = cPoolSize - 1; 
                 i > 0; 
                 ptdp = (CTreeDataPos*)((BYTE*)ptdp + TREEDATA1SIZE), --i)
            {
                ptdp->SetFlag(CTreePos::TPF_DATA_POS);
                ptdp->SetNext((CTreeDataPos*)((BYTE*)ptdp + TREEDATA1SIZE));
            }

            ptdp->SetFlag(CTreePos::TPF_DATA_POS);

            _ptdpFree = (CTreeDataPos*)((void**)pvPoolNew + 1);
            *((void**)pvPoolNew) = _pvPool;
            _pvPool = pvPoolNew;
        }
    }

    ptdpNew = _ptdpFree;
    if (ptdpNew)
    {
        _ptdpFree = (CTreeDataPos*)(ptdpNew->Next());
        ptdpNew->SetNext(NULL);
    }

#if DBG == 1 || defined(DUMPTREE)
    ptdpNew->SetSN();
#endif

    return ptdpNew;
}

CTreePos *
CMarkup::AllocData2Pos()
{
    CTreeDataPos *ptdpNew;
    ptdpNew = (CTreeDataPos*)MemAllocClear( Mt(CTreeDataPos), TREEDATA2SIZE );

    if( ptdpNew )
    {
        ptdpNew->SetFlag(CTreePos::TPF_DATA_POS|CTreePos::TPF_DATA2_POS);
#if DBG == 1 || defined(DUMPTREE)
        ptdpNew->SetSN();
#endif
    }

    return ptdpNew;
}


inline void
CMarkup::ReleaseTreePos(CTreePos *ptp, BOOL fLastRelease /*= FALSE*/ )
{
    AssertSz(ptp->_cGapsAttached==0, "Destroying a TreePos with TreePosGaps attached");

    switch (ptp->Type())
    {
    case CTreePos::Pointer:
        if (ptp->MarkupPointer())
        {
            ptp->MarkupPointer()->OnPositionReleased();
            ptp->DataThis()->p._dwPointerAndGravityAndCling = 0;
        }

    // fall through
    case CTreePos::Text:
        Assert( ptp->IsDataPos() );
        if( ptp->IsData2Pos() )
        {
            MemFree( ptp );
        }
        else if( ! fLastRelease )
        {
            memset( ptp, 0, TREEDATA1SIZE );

            ptp->SetFlag( CTreePos::TPF_DATA_POS );
            ptp->SetNext( _ptdpFree );
            _ptdpFree = ptp->DataThis();
        }
        break;

    case CTreePos::NodeBeg:
    case CTreePos::NodeEnd:
        // Make sure that we crash if someone
        // tries to use a dead node pos...
        ptp->_pFirstChild = NULL;
        ptp->_pNext = NULL;
        break;
    }
}


void
CMarkup::FreeTreePos(CTreePos *ptp)
{
    Assert(ptp);

    // set a sentinel to make the traversal terminate
    ptp->MarkFirst();
    ptp->SetNext(NULL);

    // release the subtree, adding its nodes to the free list
    while (ptp)
    {
        if (ptp->FirstChild())
        {
            ptp = ptp->FirstChild();
        }
        else
        {
            CTreePos *ptpNext;
            BOOL fRelease = TRUE;
            while (fRelease)
            {
                fRelease = ptp->IsLastChild();
                ptpNext = ptp->Next();
                ReleaseTreePos(ptp);
                ptp = ptpNext;
            }
        }
    }
}


CTreePos *
CMarkup::FirstTreePos() const
{
    return _ptpFirst;
}


CTreePos *
CMarkup::LastTreePos() const
{
    CTreePos *ptpLeft = _tpRoot.FirstChild();

    return (ptpLeft) ? ptpLeft->RightmostDescendant() : NULL;
}



#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

CTreePos *
CMarkup::TreePosAtSourceIndex(long iSourceIndex)
{
    Assert(0<=iSourceIndex);
    WHEN_DBG( long iSIOrig = iSourceIndex; )
    long cElemLeft;
    long cDepth=0;

    CTreePos *ptp = _tpRoot.FirstChild();

    for (;; ++cDepth )
    {
        cElemLeft = ptp->GetElemLeft();

        if (iSourceIndex < cElemLeft)
        {
            ptp = ptp->FirstChild();
            Assert(ptp->IsLeftChild());
        }
        else if (iSourceIndex > cElemLeft || !ptp->IsBeginElementScope())
        {
            iSourceIndex -= cElemLeft + (ptp->IsBeginElementScope()? 1 : 0);
            ptp = ptp->RightChild();
        }
        else
        {
            break;
        }
    }

    TraceTag((tagTreePosAccess, "%p: TreePos at sourceindex %ld  depth %ld",
                this, iSIOrig, cDepth));

    if (ShouldSplay(cDepth))
    {
        ptp->Splay();
    }

    return ptp;
}

CTreePos *
#if DBG == 1
CMarkup::TreePosAtCp ( long cp, long * pcchOffset, BOOL fNoTrace ) const
#else
CMarkup::TreePosAtCp ( long cp, long * pcchOffset ) const
#endif
{
    long cDepth=0;
    WHEN_DBG( long cpOrig=cp; )

    // Make sure we got a valid cp.
            
    AssertSz( cp >= 1 && cp < Cch(), "Invalid cp - out of document" );

    CTreePos * ptp = _tpRoot.FirstChild();

    for ( ; ; ++cDepth )
    {
        if (cp < long( ptp->_cchLeft ))
        {
            ptp = ptp->FirstChild();

            if (!ptp || !ptp->IsLeftChild())
            {
                ptp = FirstTreePos();
                break;          // we fell off the left end
            }
        }
        else
        {
            cp -= ptp->_cchLeft;
            
            if (ptp->IsPointer() || cp && (!ptp->IsText() || cp >= ptp->Cch()))
            {
                cp -= ptp->GetCch();
                ptp = ptp->RightChild();
            }
            else
                break;
        }
    }

    if (pcchOffset)
        *pcchOffset = cp;

    WHEN_DBG( if (!fNoTrace) )
            
    TraceTag(
        (tagTreePosAccess,
         "%p: TreePos at cp %ld offset %ld depth %ld",
         this, cpOrig, cp, cDepth));

    if (WHEN_DBG( !fNoTrace && ) ShouldSplay(cDepth))
        ptp->Splay();

    return ptp;
}

#pragma optimize("", on)

HRESULT
CMarkup::Append(CTreePos *ptpNew)
{
    HRESULT hr = (ptpNew)? S_OK : E_OUTOFMEMORY;
    CTreePos *ptpOldRoot = _tpRoot.FirstChild();

    if (hr)
        goto Cleanup;
    Assert(!ptpNew->Owner() || ptpNew->Owner() == this);

    ptpNew->SetFirstChild(ptpOldRoot);
    ptpNew->MarkLeft();
    ptpNew->MarkLast();
    ptpNew->SetNext(&_tpRoot);

    ptpNew->ClearCounts();
    ptpNew->IncreaseCounts(&_tpRoot, CTreePos::TP_LEFT);

    _tpRoot.SetFirstChild(ptpNew);
    _tpRoot.IncreaseCounts(ptpNew, CTreePos::TP_DIRECT);

    if (ptpOldRoot)
    {
        Assert(ptpOldRoot->IsLastChild());
#if defined(MAINTAIN_SPLAYTREE_THREADS)
        // adjust threads
        CTreePos *ptpRightmost = ptpOldRoot->RightmostDescendant();
        Assert(ptpRightmost->RightThread() == NULL);
        ptpRightmost->SetRightThread(ptpNew);
        ptpNew->SetLeftThread(ptpRightmost);
        ptpNew->SetRightThread(NULL);
#endif
        ptpOldRoot->SetNext(ptpNew);
    }
    else
    {
        _ptpFirst = ptpNew;
    }

#if DBG==1
    if (IsTagEnabled(tagTreePosValidate))
        Assert(IsSplayValid());
#endif

    TraceTag((tagTreePosOps, "%p: Append TreePos cch=%ld",
        this, ptpNew->GetCch()));

Cleanup:
    RRETURN(hr);
}

HRESULT
CMarkup::Insert(CTreePos *ptpNew, CTreePosGap *ptpgInsert)
{
    Assert( ptpgInsert->IsPositioned() );

    return Insert( ptpNew,
                   ptpgInsert->AttachedTreePos(),
                   ptpgInsert->AttachDirection() == TPG_RIGHT );
}

HRESULT
CMarkup::Insert(CTreePos *ptpNew, CTreePos *ptpInsert, BOOL fBefore)
{
    CTreePos *pLeftChild, *pRightChild;
    CTreePos *ptp;
    BOOL fLeftChild;
    HRESULT hr = (ptpNew)? S_OK : E_OUTOFMEMORY;

    if (hr)
        goto Cleanup;

    Assert(!ptpNew->Owner() || ptpNew->Owner() == this);
    Assert(ptpInsert);

    if (!fBefore)
    {
        CTreePos *ptpInsertBefore = ptpInsert->NextTreePos();

        if( ptpInsertBefore )
        {
            ptpInsert = ptpInsertBefore;
        }
        else
        {
            hr = Append( ptpNew );
            goto Cleanup;
        }
    }

    ptpNew->ClearCounts();
    ptpNew->IncreaseCounts(ptpInsert, CTreePos::TP_LEFT);

    ptpInsert->GetChildren(&pLeftChild, &pRightChild);

    ptpInsert->SetFirstChild(ptpNew);

    ptpNew->SetFirstChild(pLeftChild);
    ptpNew->MarkLeft();

    if (pLeftChild)
    {
        ptpNew->MarkLast(pLeftChild->IsLastChild());
        ptpNew->SetNext(pLeftChild->Next());
        pLeftChild->MarkLast();
        pLeftChild->SetNext(ptpNew);
    }
    else if (pRightChild)
    {
        ptpNew->MarkFirst();
        ptpNew->SetNext(pRightChild);
    }
    else
    {
        ptpNew->MarkLast();
        ptpNew->SetNext(ptpInsert);
    }

#if defined(MAINTAIN_SPLAYTREE_THREADS)
    // adjust threads
    ptpNew->SetLeftThread(ptpInsert->LeftThread());
    ptpNew->SetRightThread(ptpInsert);
    if (ptpNew->LeftThread())
        ptpNew->LeftThread()->SetRightThread(ptpNew);
    ptpInsert->SetLeftThread(ptpNew);
#endif

    if (ptpNew->HasNonzeroCounts(CTreePos::TP_DIRECT))
    {
        fLeftChild = TRUE;
        for (ptp = ptpInsert; ptp; ptp = ptp->Parent())
        {
            if (fLeftChild)
            {
                ptp->IncreaseCounts(ptpNew, CTreePos::TP_DIRECT);
            }
            fLeftChild = ptp->IsLeftChild();
        }
    }

    if (ptpInsert == _ptpFirst)
    {
        _ptpFirst = ptpNew;
    }

    TraceTag((tagTreePosOps, "%p: Insert TreePos cch=%ld",
        this, ptpNew->GetCch() ));

Cleanup:
#if DBG==1
    if (IsTagEnabled(tagTreePosValidate))
        Assert(IsSplayValid());
#endif

    RRETURN(hr);
}

HRESULT 
CMarkup::Move(CTreePos *ptpNew, CTreePosGap *ptpgDest)
{
    Assert( ptpgDest->IsPositioned() );

    return Move( ptpNew,
                 ptpgDest->AttachedTreePos(),
                 ptpgDest->AttachDirection() == TPG_RIGHT );
}

HRESULT
CMarkup::Move(CTreePos *ptpMove, CTreePos *ptpDest, BOOL fBefore)
{
    Assert(ptpMove != ptpDest);
    SUBLIST sublist;
    HRESULT hr;

    hr = SpliceOut(ptpMove, ptpMove, &sublist);
    if (!hr)
    {
        Assert(sublist.FirstChild() == ptpMove);
        hr = Insert(ptpMove, ptpDest, fBefore);
    }

    return hr;
}


HRESULT
CMarkup::Split(CTreePos *ptpSplit, long cchLeft, SCRIPT_ID sidNew /*= sidNil */)
{
    WHEN_DBG( CTreePos tpValid(TRUE); )
    CTreePos * ptpNew;
    CTreePos * pLeftChild, * pRightChild;
    HRESULT    hr = S_OK;
    
    Assert( ptpSplit->IsText() && ptpSplit->Cch() >= cchLeft );

    if (sidNew == sidNil)
        sidNew = ptpSplit->Sid();

    ptpNew = NewTextPos( ptpSplit->Cch() - cchLeft, sidNew, ptpSplit->TextID() );

    if (!ptpNew)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    ptpSplit->GetChildren(&pLeftChild, &pRightChild);

    ptpNew->MarkLast();
    ptpNew->SetNext(ptpSplit);
    ptpNew->SetFirstChild(pRightChild);
    if (pRightChild)
    {
        pRightChild->SetNext(ptpNew);
    }

    ptpSplit->DataThis()->t._cch = cchLeft;
    if (pLeftChild)
    {
        pLeftChild->MarkFirst();
        pLeftChild->SetNext(ptpNew);
    }
    else
    {
        ptpSplit->SetFirstChild(ptpNew);
    }

#if defined(MAINTAIN_SPLAYTREE_THREADS)
    // adjust threads
    ptpNew->SetLeftThread(ptpSplit);
    ptpNew->SetRightThread(ptpSplit->RightThread());
    ptpSplit->SetRightThread(ptpNew);
    if (ptpNew->RightThread())
        ptpNew->RightThread()->SetLeftThread(ptpNew);
#endif

#if DBG==1
    if (IsTagEnabled(tagTreePosValidate))
        Assert(ptpSplit->IsSplayValid(&tpValid));
#endif

    TraceTag((tagTreePosOps, "%p: Split TreePos into cch %ld/%ld",
                this, ptpSplit->Cch(), ptpNew->Cch()));

Cleanup:
    RRETURN(hr);
}


HRESULT
CMarkup::Join(CTreePos *ptpJoin)
{
    WHEN_DBG( CTreePos tpValid(TRUE); )
    CTreePos *ptpNext, *ptpParent;
    CTreePos *pLeftChild;

    // put the two TreePos together at the top of the tree, next TreePos on top
    ptpJoin->Splay();
    ptpNext = ptpJoin->NextTreePos();
    ptpNext->Splay();

    Assert( ptpJoin->IsText() && ptpNext->IsText() );

    // take over the next TreePos (my right subtree is empty)
    ptpJoin->DataThis()->t._cch += ptpNext->Cch();
    if (!ptpJoin->IsLastChild())
    {
        ptpJoin->Next()->SetNext(ptpJoin);
        pLeftChild = ptpJoin->LeftChild();
        if (pLeftChild)
        {
            pLeftChild->MarkFirst();
            pLeftChild->SetNext(ptpJoin->Next());
        }
        else
        {
            ptpJoin->SetFirstChild(ptpJoin->Next());
        }
    }

#if defined(MAINTAIN_SPLAYTREE_THREADS)
    // adjust threads
    ptpJoin->SetRightThread(ptpNext->RightThread());
    if (ptpNext->RightThread())
        ptpNext->RightThread()->SetLeftThread(ptpJoin);
#endif

    // discard the next TreePos
    ptpParent = ptpNext->Parent();
    Assert(ptpParent->Next() == NULL);
    ptpParent->ReplaceChild(ptpNext, ptpJoin);
    ptpNext->SetFirstChild(NULL);
    FreeTreePos(ptpNext);

#if DBG==1
    if (IsTagEnabled(tagTreePosValidate))
        Assert(ptpJoin->IsSplayValid(&tpValid));
#endif

    TraceTag((tagTreePosOps, "%p: Join cch=%ld", this, ptpJoin->Cch()));

    return S_OK;
}

// BUGBUG: this is just a braindead implementation.  If this is slow
// we can improve it later
HRESULT 
CMarkup::ReplaceTreePos(CTreePos * ptpOld, CTreePos * ptpNew)
{
    HRESULT hr = S_OK;

    Assert( ! HasUnembeddedPointers() );

    Assert( ptpOld && ptpNew );

    hr = THR( Insert( ptpNew, ptpOld, TRUE ) );
    if (hr)
        goto Cleanup;

    hr = THR( Remove( ptpOld ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

//
// This MergeText cannot be used for merging text after pointer pos removal
//

HRESULT
CMarkup::MergeText( CTreePos * ptpMerge )
{
    HRESULT hr = S_OK;
    CTreePos * ptp;

    Assert( ! HasUnembeddedPointers() );

    if (!ptpMerge->IsText())
        goto Cleanup;

    ptp = ptpMerge->PreviousTreePos();

    if (    ptp->IsText() 
        &&  ptp->Sid() == ptpMerge->Sid()
        &&  ptp->TextID() == ptpMerge->TextID() )
    {
        hr = THR( MergeTextHelper( ptp ) );

        if (hr)
            goto Cleanup;

        ptpMerge = ptp;
    }

    ptp = ptpMerge->NextTreePos();

    if (    ptp->IsText() 
        &&  ptp->Sid() == ptpMerge->Sid()
        &&  ptp->TextID() == ptpMerge->TextID() )
    {
        hr = THR( MergeTextHelper( ptpMerge ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

HRESULT
CMarkup::MergeTextHelper ( CTreePos * ptpMerge )
{
    HRESULT  hr = S_OK;

    Assert( ptpMerge->IsText() );
    Assert( ptpMerge->NextTreePos()->IsText() );

    hr = THR( Join( ptpMerge ) );

    if (hr)
        goto Cleanup;
Cleanup:

    RRETURN( hr );
}

///////////////////////////////////////////////////////////////////////////////
//
// RemovePointerPos
//
// Removes a pointer pos, making sure unembedded pointers are updated
// properly.  Once can also pass in a ptp/ich pair to make sure it gets
// updated as well.
//
///////////////////////////////////////////////////////////////////////////////

HRESULT
CMarkup::RemovePointerPos ( CTreePos * ptp, CTreePos * * pptpUpdate, long * pichUpdate )
{
    HRESULT    hr = S_OK;
    CTreePos * ptpBefore;
    CTreePos * ptpAfter;

    Assert( ptp->IsPointer() );
    Assert( ! pptpUpdate || *pptpUpdate != ptp );

    //
    // Remove the pos from the list, making sure to record the pos before the one
    // being removed
    //

    ptpBefore = ptp->PreviousTreePos();

    hr = THR( Remove( ptp ) );

    if (hr)
        goto Cleanup;

    //
    // Now, see if there are two adjacent text pos's we can merge
    //

    if (ptpBefore->IsText() && (ptpAfter = ptpBefore->NextTreePos())->IsText() &&
        ptpBefore->Sid() == ptpAfter->Sid() && ptpBefore->TextID() == ptpAfter->TextID())
    {
        CMarkupPointer * pmp;

        //
        // Update the incomming ref and unembedded markup pointers
        //
        
        if (pptpUpdate && *pptpUpdate == ptpAfter)
        {
            *pptpUpdate = ptpBefore;
            *pichUpdate += ptpBefore->Cch();
        }

        for ( pmp = _pmpFirst ; pmp ; pmp = pmp->_pmpNext )
        {
            Assert( ! pmp->_fEmbedded );

            if (pmp->_ptpRef == ptpAfter)
            {
                pmp->_ptpRef = ptpBefore;
                
                Assert( ! pmp->_ptpRef->IsPointer() );
                
                Assert(
                    ! pmp->_ptpRef->IsBeginElementScope() ||
                        ! pmp->_ptpRef->Branch()->Element()->IsNoScope() );
                
                pmp->_ichRef += ptpBefore->Cch();
            }
        }

        //
        // Then joing the adjacent text pos's
        //

        hr = THR( Join( ptpBefore ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

HRESULT
CMarkup::Remove(CTreePos *ptpStart, CTreePos *ptpFinish)
{
    HRESULT hr;
    SUBLIST sublist;

    hr = SpliceOut(ptpStart, ptpFinish, &sublist);
    if (!hr && sublist.FirstChild())
    {
        FreeTreePos(sublist.FirstChild());

        TraceTag((tagTreePosOps, "%p: Remove cch=%ld", this, sublist._cchLeft));
    }

    RRETURN(hr);
}


HRESULT
CMarkup::SpliceOut(CTreePos *ptpStart, CTreePos *ptpFinish,
                        SUBLIST *pSublistSplice)
{
    CTreePos *ptpSplice;
    CTreePos *ptp;
    BOOL fLeftChild;
    CTreePos *ptpLeftEdge = ptpStart->PreviousTreePos();
    CTreePos *ptpRightEdge = ptpFinish->NextTreePos();
//    CTreePos *ptpPointerHead=NULL, *ptpPointerTail=NULL;
#if defined(MAINTAIN_SPLAYTREE_THREADS)
    CTreePos *ptpLeftmost, *ptpRightmost;
#endif

    pSublistSplice->InitSublist();

    // isolate the splice region near the top of the tree
    if (ptpRightEdge)
    {
        ptpRightEdge->Splay();
    }
    if (ptpLeftEdge)
    {
        ptpLeftEdge->Splay();
    }

    // locate the splice region
    ptpSplice = _tpRoot.FirstChild();
    if (ptpLeftEdge)
    {
        ptpSplice = ptpLeftEdge->RightChild();
        // call RotateUp to isolate if last step was zig-zig
        if (ptpRightEdge && ptpRightEdge->Parent() == ptpSplice)
        {
            ptpRightEdge->RotateUp(ptpSplice, ptpSplice->Parent());
        }
    }
    if (ptpRightEdge)
    {
        ptpSplice = ptpRightEdge->LeftChild();
    }

    ptp = ptpSplice->Parent();
    fLeftChild = ptpSplice->IsLeftChild();

    // splice it out
    if (fLeftChild)
    {
        pSublistSplice->IncreaseCounts(ptp, CTreePos::TP_LEFT);
    }
    else
    {
        Assert(ptp->Parent() == &_tpRoot);
        pSublistSplice->IncreaseCounts(&_tpRoot, CTreePos::TP_LEFT);
        pSublistSplice->DecreaseCounts(ptp, CTreePos::TP_BOTH);
    }
    ptp->RemoveChild(ptpSplice);
    pSublistSplice->SetFirstChild(ptpSplice);
    ptpSplice->SetNext(pSublistSplice);
    ptpSplice->MarkLast();
    ptpSplice->MarkLeft();

#if defined(MAINTAIN_SPLAYTREE_THREADS)
    // adjust threads
    ptpLeftmost = ptpSplice->LeftmostDescendant();
    ptpRightmost = ptpSplice->RightmostDescendant();
    if (ptpLeftmost->LeftThread())
        ptpLeftmost->LeftThread()->SetRightThread(ptpRightmost->RightThread());
    if (ptpRightmost->RightThread())
        ptpRightmost->RightThread()->SetLeftThread(ptpLeftmost->LeftThread());
    ptpLeftmost->SetLeftThread(NULL);
    ptpRightmost->SetRightThread(NULL);
#endif

#if 0 // Don't need this right now
    if (fIgnorePointers)
    {
        CTreePos *ptpSource;

        // find any pointers in the splice region
        for (ptpSource = pSublistSplice->LeftmostDescendant();
             ptpSource;
             /* advance done in body */ )
        {
            CTreePos *ptpNext = ptpSource->NextTreePos();

            if (ptpSource->IsPointer())
            {
                ptpSource->Remove();
                if (ptpPointerTail)
                {
                    ptpPointerTail->SetFirstChild(ptpSource);
                    ptpSource->MarkRight();
                    ptpSource->MarkLast();
                    ptpSource->SetNext(ptpPointerTail);
#if defined(MAINTAIN_SPLAYTREE_THREADS)
                    ptpPointerTail->SetRightThread(ptpSource);
                    ptpSource->SetLeftThread(ptpPointerTail);
#endif
                }
                else
                {
                    ptpPointerHead = ptpSource;
                }
                ptpSource->SetFirstChild(NULL);
                ptpPointerTail = ptpSource;
            }

            ptpSource = ptpNext;
        }

        // restore them to the main list
        if (ptpPointerHead)
        {
            if (fLeftChild)
            {
                ptpPointerHead->MarkLeft();
                if (ptp->FirstChild())
                {
                    ptpPointerHead->MarkFirst();
                    ptpPointerHead->SetNext(ptp->FirstChild());
                }
                else
                {
                    ptpPointerHead->MarkLast();
                    ptpPointerHead->SetNext(ptp);
                }
                ptp->SetFirstChild(ptpPointerHead);
#if defined(MAINTAIN_SPLAYTREE_THREADS)
                // adjust threads
                if (ptp->LeftThread())
                {
                    ptp->LeftThread()->SetRightThread(ptpPointerHead);
                    ptpPointerHead->SetLeftThread(ptp->LeftThread());
                }
                ptpPointerTail->SetRightThread(ptp);
                ptp->SetLeftThread(ptpPointerTail);
#endif
            }
            else
            {
                CTreePos *ptpLeft = ptp->FirstChild();

                ptpPointerHead->MarkRight();
                ptpPointerHead->MarkLast();
                ptpPointerHead->SetNext(ptp);
                if (ptpLeft)
                {
                    ptpLeft->SetNext(ptpPointerHead);
                    ptpLeft->MarkFirst();
                }
                else
                {
                    ptp->SetFirstChild(ptpPointerHead);
                }
#if defined(MAINTAIN_SPLAYTREE_THREADS)
                // adjust threads
                if (ptp->RightThread())
                {
                    ptp->RightThread()->SetLeftThread(ptpPointerTail);
                    ptpPointerTail->SetRightThread(ptp->RightThread());
                }
                ptpPointerHead->SetLeftThread(ptp);
                ptp->SetRightThread(ptpPointerHead);
#endif
            }
        }
    }
#endif // Don't need this right now

    // adjust cumulative counts
    if( pSublistSplice->HasNonzeroCounts( CTreePos::TP_LEFT ) )
    {
        for (; ptp; ptp=ptp->Parent())
        {
            if (fLeftChild)
            {
                ptp->DecreaseCounts(pSublistSplice, CTreePos::TP_LEFT);
            }
            fLeftChild = ptp->IsLeftChild();
        }
    }

    // return answers
    TraceTag((tagTreePosOps, "%p: SpliceOut cch=%ld", this, pSublistSplice->_cchLeft));

    if (ptpLeftEdge == NULL)
    {
        _ptpFirst = _tpRoot.LeftmostDescendant();
        if (_ptpFirst == &_tpRoot)
        {
            _ptpFirst = NULL;
        }
    }

#if DBG==1
    if (IsTagEnabled(tagTreePosValidate))
        Assert(IsSplayValid());
#endif

    return S_OK;
}

HRESULT
CMarkup::SpliceIn(SUBLIST *pSublistSplice, CTreePos *ptp)
{
    CTreePos *ptpSplice = pSublistSplice->FirstChild();
    CTreePos *pRightChild;
    CTreePos *ptpPrev;
    BOOL fLeftChild;
    BOOL fNewFirst = (ptp == _ptpFirst);

    Assert(ptpSplice);
    Assert(!ptpSplice->Owner() || ptpSplice->Owner() == this);

#if defined(MAINTAIN_SPLAYTREE_THREADS)
    CTreePos *ptpLeftmost = ptpSplice->LeftmostDescendant();
    CTreePos *ptpRightmost = ptpSplice->RightmostDescendant();
#endif

    if (ptp)
    {
        ptp->Splay();

        ptpPrev = ptp->PreviousTreePos();
        if (ptpPrev)
        {
            ptpPrev->Splay();
        }

        pRightChild = ptp->RightChild();
        if (pRightChild)
        {
            ptpSplice->MarkFirst();
            ptpSplice->SetNext(pRightChild);
        }
        else
        {
            ptpSplice->MarkLast();
            ptpSplice->SetNext(ptp);
        }

        ptp->SetFirstChild(ptpSplice);
        ptpSplice->MarkLeft();

        fLeftChild = TRUE;

#if defined(MAINTAIN_SPLAYTREE_THREADS)
        // adjust threads
        if (ptp->LeftThread())
        {
            ptp->LeftThread()->SetRightThread(ptpLeftmost);
            ptpLeftmost->SetLeftThread(ptp->LeftThread());
        }
        ptpRightmost->SetRightThread(ptp);
        ptp->SetLeftThread(ptpRightmost);
#endif
    }
    else
    {
        ptp = LastTreePos();
        Assert(ptp);
        CTreePos *ptpLeft = ptp->LeftChild();

        if (ptpLeft)
        {
            ptpLeft->MarkFirst();
            ptpLeft->SetNext(ptpSplice);
        }
        else
        {
            ptp->SetFirstChild(ptpSplice);
        }
        ptpSplice->MarkLast();
        ptpSplice->MarkRight();
        ptpSplice->SetNext(ptp);

        fLeftChild = FALSE;

#if defined(MAINTAIN_SPLAYTREE_THREADS)
        // adjust threads
        ptp->SetRightThread(ptpLeftmost);
        ptpLeftmost->SetLeftThread(ptp);
#endif
    }

    for (; ptp; ptp=ptp->Parent())
    {
        if (fLeftChild)
        {
            ptp->IncreaseCounts(pSublistSplice, CTreePos::TP_LEFT);
        }
        fLeftChild = ptp->IsLeftChild();
    }

    if (fNewFirst)
    {
        _ptpFirst = _ptpFirst->LeftmostDescendant();
    }

#if DBG==1
    if (IsTagEnabled(tagTreePosValidate))
        Assert(IsSplayValid());
#endif
    TraceTag((tagTreePosOps, "%p: SpliceIn cch=%ld", this, pSublistSplice->_cchLeft));
    return S_OK;
}

#if 0
// BUGBUG This routine is useless in the new architecture  We have no reason
// to just clone text poses
HRESULT
CMarkup::CloneOut(CTreePos *ptpStart, CTreePos *ptpFinish,
                        SUBLIST *pSublistClone, CMarkup *pMarkupOwner)
{
    WHEN_DBG( CTreePos tpValid(TRUE); )
    CTreePos *ptpParent, *ptpClone=NULL;
    CTreePos *ptpSource, *ptpTarget;

    pSublistClone->InitSublist();

    // BUGBUG we should advance using NextTreePos instead of NextRun.
    // This code only copies text runs, and assumes the ending position is a text run.
    for (ptpSource = ptpStart; ; ptpSource = ptpSource->NextRun())
    {
        if (!ptpSource->IsPointer())
        {
            ptpTarget = pMarkupOwner->NewTreePos(ptpSource);
            pSublistClone->IncreaseCounts(ptpTarget, CTreePos::TP_DIRECT);
            if (ptpClone)
            {
                ptpTarget->MarkRight();
                ptpParent = ptpClone;
#if defined(MAINTAIN_SPLAYTREE_THREADS)
                // adjust threads
                ptpParent->SetRightThread(ptpTarget);
                ptpTarget->SetLeftThread(ptpParent);
#endif
            }
            else
            {
                ptpTarget->MarkLeft();
                ptpParent = pSublistClone;
            }
            ptpTarget->MarkLast();
            ptpTarget->SetNext(ptpParent);
            ptpParent->SetFirstChild(ptpTarget);
            ptpClone = ptpTarget;
        }

        if (ptpSource == ptpFinish)
            break;
    }

    TraceTag((tagTreePosOps, "%p: CloneOut cch=%ld", this, pSublistClone->_cchLeft));
    Assert(pSublistClone->IsSplayValid(&tpValid));
    return S_OK;
}


HRESULT
CMarkup::CopyFrom(CMarkup *pMarkupSource, CTreePos *ptpStart, CTreePos *ptpFinish,
                    CTreePos *ptpInsert)
{
    SUBLIST sublist;
    HRESULT hr;

    hr = pMarkupSource->CloneOut(ptpStart, ptpFinish, &sublist, this);
    if (hr)
        goto Cleanup;

    hr = SpliceIn(&sublist, ptpInsert);

Cleanup:
    RRETURN(hr);
}

#endif

//
// CMarkup::InsertPosChain
//
// This method inserts a chain of CTreePoses into the tree.  The
// poses should be chained together in an appropriate way to make
// insertion very cheap.
//
// The first child pointer of every pos should point to the previous
// pos in the chain.  The next pointer should point to the next
// pos in the chain.  The ends should point to NULL.  The last child
// and left child bits should be set on every element.
//
// Eventually, the counts should probably be set to something also so
// that this method doesn't have to run the chain, but I haven't figured
// that out yet.
//

HRESULT
CMarkup::InsertPosChain( 
                CTreePos * ptpChainHead, CTreePos * ptpChainTail, 
                CTreePos * ptpInsertBefore )
{
    CTreePos *pLeftChild, *pRightChild;

    // We can't insert at the beginning or end of the splay tree
    Assert( ptpInsertBefore != _ptpFirst );
    Assert( ptpInsertBefore );

    ptpInsertBefore->GetChildren(&pLeftChild, &pRightChild);

    ptpInsertBefore->SetFirstChild(ptpChainTail);
    ptpChainTail->MarkLeft();

    ptpChainHead->SetFirstChild(pLeftChild);

    if (pLeftChild)
    {
        ptpChainTail->MarkLast(pLeftChild->IsLastChild());
        ptpChainTail->SetNext(pLeftChild->Next());

        pLeftChild->MarkLast();
        pLeftChild->SetNext(ptpChainHead);
    }
    else if (pRightChild)
    {
        ptpChainTail->MarkFirst();
        ptpChainTail->SetNext(pRightChild);
    }
    else
    {
        ptpChainTail->MarkLast();
        ptpChainTail->SetNext(ptpInsertBefore);
    }

#if defined(MAINTAIN_SPLAYTREE_THREADS)
    // adjust threads
    ptpChainHead->SetLeftThread(ptpInsertBefore->LeftThread());
    ptpChainTail->SetRightThread(ptpInsertBefore);
    if (ptpChainHead->LeftThread())
        ptpChainHead->LeftThread()->SetRightThread(ptpChainHead);
    ptpInsertBefore->SetLeftThread(ptpChainTail);
#endif

    // update the order statistics
    {
        CTreePos *  ptp, *ptpPrev = NULL;
        BOOL        fLeftChild = TRUE;
        CTreePos::SCounts   counts;

        // accumulate/set the chain just inserted
        ptpChainHead->ClearCounts();
        ptpChainHead->IncreaseCounts( ptpInsertBefore, CTreePos::TP_LEFT );
        
        counts.Clear();
        counts.Increase( ptpChainHead );

        for ( ptpPrev = ptpChainHead, ptp = ptpChainHead->Parent(); 
              ptpPrev != ptpChainTail; 
              ptpPrev = ptp, ptp = ptp->Parent() )
        {
            ptp->ClearCounts();
            ptp->IncreaseCounts( ptpPrev, CTreePos::TP_BOTH );

            counts.Increase( ptp );
        }

        // This method isn't used to insert pointers so
        // we must have some sort of count
        Assert( counts.IsNonzero() );
        
        fLeftChild = TRUE;
        for (ptp = ptpInsertBefore; ptp; ptp = ptp->Parent())
        {
            if (fLeftChild)
            {
                ptp->IncreaseCounts( &counts );
            }
            fLeftChild = ptp->IsLeftChild();
        }
    }

#if DBG==1
    if (IsTagEnabled(tagTreePosValidate))
        Assert(IsSplayValid());
#endif

    RRETURN(S_OK);
}


//+----------------------------------------------------------------------------
//
//  Member:     FastElemTextSet
//
//  Synopsis:   Fast way to set the entire text of an element (e.g. INPUT).
//              Assumes that there is no overlapping.
//
//-----------------------------------------------------------------------------

HRESULT
CMarkup::FastElemTextSet(CElement * pElem, const TCHAR * psz, int c, BOOL fAsciiOnly)
{
    HRESULT         hr = S_OK;
    CTreePos *      ptpBegin;
    CTreePos *      ptpEnd;
    long            cpBegin, cpEnd;

    // make sure pointers are embedded

    hr = THR( EmbedPointers() );
    if (hr)
        goto Cleanup;
    
    // Assert that there is no overlapping into this element
    Assert( ! pElem->GetFirstBranch()->NextBranch() );

    pElem->GetTreeExtent( &ptpBegin, &ptpEnd );
    Assert( ptpBegin && ptpEnd );

    cpBegin = ptpBegin->GetCp() + 1;
    cpEnd = ptpEnd->GetCp();

    if( cpBegin != cpEnd || !fAsciiOnly)
    {
        CTreePosGap tpgBegin( TPG_LEFT ), tpgEnd( TPG_RIGHT );
        Verify( ! tpgBegin.MoveTo( ptpBegin, TPG_RIGHT ) );
        Verify( ! tpgEnd.MoveTo( ptpEnd, TPG_LEFT ) );

        hr = THR( SpliceTreeInternal( & tpgBegin, & tpgEnd ) );
        if (hr)
            goto Cleanup;

    #if DBG == 1
        {
            CTreePos * ptpEndAfter;
            pElem->GetTreeExtent( NULL, &ptpEndAfter );
            Assert( ptpEndAfter == ptpEnd );
        }
    #endif

        hr = THR( InsertTextInternal( ptpEnd, psz, c, NULL ) );
        if (hr)
            goto Cleanup;
    }
    else if (c)
    {
        CTreePos *      ptpText = ptpEnd;
        CNotification   nf;
        CInsertSpliceUndo Undo( Doc() );

        Undo.Init( this, NULL );

        UpdateMarkupContentsVersion();

        Undo.SetData( cpBegin, cpBegin + c );

        // Build a notification (do we HAVE to do this?!?)
        nf.CharsAdded( cpBegin, c, pElem->GetFirstBranch() );

        ptpText = NewTextPos( c );
        hr = THR( Insert( ptpText, ptpBegin, FALSE ) );
        if (hr)
            goto Cleanup;

        // Insert the text
        Verify(
            CTxtPtr( this, cpBegin ).
                InsertRange( c, psz ) == c );

        Notify( &nf );

        Undo.CreateAndSubmit();
    }

Cleanup:
    RRETURN(hr);
}


/////////////////////////////////////////////////////////////////////////////////
//      CTreePosGap
/////////////////////////////////////////////////////////////////////////////////

CTreePos *
CTreePosGap::AdjacentTreePos(TPG_DIRECTION eDir) const
{
    Assert(eDir==TPG_LEFT || eDir==TPG_RIGHT);
    AssertSz(_ptpAttach, "Gap is not positioned");
    BOOL fLeft = (eDir == TPG_LEFT);

    return (fLeft==!!_fLeft) ? _ptpAttach
                             : _fLeft ? _ptpAttach->NextTreePos()
                                      : _ptpAttach->PreviousTreePos();
}

#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

CTreeNode *
CTreePosGap::Branch() const
{
    AssertSz(_ptpAttach, "Gap is not positioned");
    BOOL fLeft = _fLeft;
    CTreePos *ptp= _ptpAttach;

    if( !_ptpAttach->IsNode() ) 
    {
        fLeft = FALSE;
        do 
            ptp = ptp->NextTreePos();
        while ( !ptp->IsNode() );  
    }

    return (fLeft == ptp->IsEndNode()) ? ptp->Branch()->Parent()
                                       : ptp->Branch();
}

#if DBG != 1
#pragma optimize("", on)
#endif

BOOL
CTreePosGap::IsValid() const
{
    AssertSz(_ptpAttach, "Gap is not positioned");

    BOOL result = !_ptpAttach->IsNode();

    if (!result)
    {
        CTreePos *ptpLeft = _fLeft ? _ptpAttach : _ptpAttach->PreviousTreePos();
        CTreePos *ptpRight = _fLeft ? _ptpAttach->NextTreePos() : _ptpAttach;

        result = !(
            (ptpLeft->IsEndNode() && !ptpLeft->IsEdgeScope())
         || (ptpRight->IsBeginNode() && !ptpRight->IsEdgeScope())
            );
    }

    return result;
}


void
CTreePosGap::SetAttachPreference(TPG_DIRECTION eDir)
{
    _eAttach = eDir;

    if (eDir!=TPG_EITHER && (eDir==TPG_LEFT)!=!!_fLeft && _ptpAttach)
    {
        if (_fLeft)
        {
            MoveTo(_ptpAttach->NextTreePos(), TPG_LEFT);
        }
        else
        {
            MoveTo(_ptpAttach->PreviousTreePos(), TPG_RIGHT);
        }
    }
}


HRESULT
CTreePosGap::MoveTo(CTreePos *ptp, TPG_DIRECTION eDir)
{
    Assert(ptp && (eDir==TPG_LEFT || eDir==TPG_RIGHT));
    BOOL fLeft = (eDir != TPG_LEFT);        // now from the gap's point of view
    HRESULT hr;

    // adjust for my attach preference
    if (_eAttach!=TPG_EITHER && fLeft!=(_eAttach==TPG_LEFT))
    {
        ptp = fLeft ? ptp->NextTreePos() : ptp->PreviousTreePos();
        fLeft = !fLeft;
    }

    // make sure the requested gap is in the scope of the restricting element
    if (_pElemRestrict)
    {
        if (ptp->SearchBranchForElement(_pElemRestrict, !fLeft) == NULL)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }
    }

    UnPosition();
    _ptpAttach = ptp;
    _fLeft = fLeft;
    WHEN_DBG( _ptpAttach->AttachGap(); )
    hr = S_OK;

Cleanup:
    return hr;
}

HRESULT
CTreePosGap::MoveToCp(CMarkup * pMarkup, long cp)
{
    HRESULT     hr;
    long        ich;
    CTreePos *  ptp;

    ptp = pMarkup->TreePosAtCp( cp, &ich );
    Assert( ptp );
    
    if (ich)
    {
        Assert( ptp->IsText() );

        hr = THR( pMarkup->Split( ptp, ich ) );
        if (hr)
            goto Cleanup;

        hr = THR( MoveTo( ptp, TPG_RIGHT ) );
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR( MoveTo( ptp, TPG_LEFT ) );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN( hr );
}

HRESULT
CTreePosGap::MoveImpl(BOOL fLeft,
                      DWORD dwMoveFlags,
                      CTreePos **pptpEdgeCrossed
                      )
{
    HRESULT hr;
    BOOL fFirstTime = TRUE;
    const BOOL fAttachToCurr = (fLeft == !_fLeft);
    CTreePos *ptpCurr = fAttachToCurr ? _ptpAttach :
            _fLeft ? _ptpAttach->NextTreePos() : _ptpAttach->PreviousTreePos();
    CTreePos *ptpAdv;
    CTreePos *ptpAttach = NULL;

    Assert(_eAttach!=TPG_EITHER ||
            0 == (dwMoveFlags & (TPG_SKIPPOINTERS | TPG_FINDTEXT | TPG_FINDEDGESCOPE)) );

    if (pptpEdgeCrossed)
        *pptpEdgeCrossed = NULL;

    for (ptpAdv = fLeft ? ptpCurr->PreviousTreePos() : ptpCurr->NextTreePos();
         ptpAdv;
         ptpCurr = ptpAdv,  ptpAdv = fLeft ? ptpCurr->PreviousTreePos() : ptpCurr->NextTreePos() )
    {
        ptpAttach = fAttachToCurr ? ptpCurr : ptpAdv;

        if (!fFirstTime)
        {
            // if we've left the scope of the restricting element, fail
            if (_pElemRestrict)
            {
                if (ptpCurr->IsNode() && ptpCurr->IsEdgeScope() &&
                    ptpCurr->Branch()->Element() == _pElemRestrict)
                {
                    ptpAdv = NULL;
                    break;
                }
            }

            // track any begin-scope or end-scope nodes that we cross over
            if (pptpEdgeCrossed && ptpCurr->IsNode() && ptpCurr->IsEdgeScope() )
            {
                AssertSz(*pptpEdgeCrossed == NULL, "Crossed multiple begin or endscope TreePos");
                *pptpEdgeCrossed = ptpCurr;
            }
        }
        else
        {
            // if it's not OK to stay still, force the first advance
            fFirstTime = FALSE;
            if (!(dwMoveFlags & TPG_OKNOTTOMOVE))
                continue;
        }

        // if caller wants a valid gap, advance if we're not in one
        if ((dwMoveFlags & TPG_VALIDGAP))
        {
            CTreePos *ptpLeft = fLeft ? ptpAdv : ptpCurr;
            CTreePos *ptpRight = fLeft ? ptpCurr : ptpAdv;
            if (!CTreePos::IsLegalPosition(ptpLeft, ptpRight))
                continue;
        }

        // if caller wants to skip pointers, advance if we're at one
        if (ptpAttach->IsPointer() && (dwMoveFlags & TPG_SKIPPOINTERS))
            continue;

        // if caller wants a text position, advance if we're not at one
        if (!ptpAttach->IsText() && (dwMoveFlags & TPG_FINDTEXT))
            continue;

        // if caller wants an edge-scope position, advance if we're not at one
        if ((dwMoveFlags & TPG_FINDEDGESCOPE) &&
            !(ptpAttach->IsNode() && ptpAttach->IsEdgeScope()) )
            continue;

        // if we've survived the gauntlet above, we're in a good gap
        break;
    }

    // if we found a good gap, move there
    if (ptpAdv)
    {
        UnPosition();
        _ptpAttach = ptpAttach;
        WHEN_DBG( _ptpAttach->AttachGap(); )
        hr = S_OK;
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}


/////////////////////////////////////////////////////////
// Method:      PartitionPointers
//
// Synopsis:    Permute any Pointer treepos around this gap so that the
//              ones with left-gravity come before the ones with right-
//              gravity, and the gap is positioned between the two groups.
//              Within each group, the pointers should be stable - i.e. they
//              maintain their relative positions.
//
//              Normal operation treats empty text poses as pointers
//              with right gravity.  However, if fDOMPartition is TRUE,
//              then empty text poses are treated as content.

// BUGBUG: this might be faster if we didn't use gaps internally

void
CTreePosGap::PartitionPointers(CMarkup * pMarkup, BOOL fDOMPartition)
{
    TPG_DIRECTION eSaveAttach = (TPG_DIRECTION)_eAttach;
    CTreePosGap tpgMiddle( TPG_RIGHT );

    Assert( ! AttachedTreePos()->GetMarkup()->HasUnembeddedPointers() );
    Assert( pMarkup );

    // move left to the beginning of the block of pointers
    SetAttachPreference(TPG_LEFT);
    while ( AttachedTreePos()->IsPointer()
        ||      AttachedTreePos()->IsText() && AttachedTreePos()->Cch() == 0 
            &&  (!fDOMPartition || AttachedTreePos()->TextID() == 0 ) )
    {
        MoveLeft();
    }

    // now move right, looking for left-gravity pointers
    SetAttachPreference(TPG_RIGHT);
    tpgMiddle.MoveTo( this );
    while ( AttachedTreePos()->IsPointer()
        ||      AttachedTreePos()->IsText() && AttachedTreePos()->Cch() == 0 
            &&  (!fDOMPartition || AttachedTreePos()->TextID() == 0 ) )
    {
        if (    AttachedTreePos()->IsPointer() 
            &&  AttachedTreePos()->Gravity() == POINTER_GRAVITY_Left)
        {
            // If tpg is in the same place, no need to move the pointer.
            if(AttachedTreePos() != tpgMiddle.AttachedTreePos())
            {
                CTreePos * ptpToMove = AttachedTreePos();

                // Don't move us along with the pointer.
                SetAttachPreference(TPG_LEFT);

                Verify( ! pMarkup->Move(ptpToMove, &tpgMiddle) );

                // So now we're attached to the next pointer.
                SetAttachPreference(TPG_RIGHT);
            }
            else
            {
                // Nothing changed, so move on ahead.
                MoveRight();
                tpgMiddle.MoveTo( this );
            }
        }
        else
        {
            MoveRight();
        }
    }

    // Position myself in the middle
    MoveTo( &tpgMiddle );

    // restore my state
    SetAttachPreference(eSaveAttach);
}

/////////////////////////////////////////////////////////
// Method:      CleanCling
//
// Synopsis:    Assuming that the current gap is in 
//              the middle of a group of pointers that
//              has been positioned

void    
CTreePosGap::CleanCling( CMarkup *pMarkup, TPG_DIRECTION eDir, BOOL fDOMPartition )
{
    Assert( eDir == TPG_LEFT || eDir == TPG_RIGHT );
    BOOL fLeft = eDir == TPG_LEFT;
    CTreePos * ptp = AdjacentTreePos( eDir );
    CTreePos * ptpNext;

    while ( ptp->IsPointer()
        ||      ptp->IsText() && ptp->Cch() == 0 
            &&  (!fDOMPartition || ptp->TextID() == 0 ) )
    {
        ptpNext = fLeft ? ptp->PreviousTreePos() : ptp->NextTreePos();

        if (    ptp->IsPointer() && ptp->Cling()
            ||      ptp->IsText() && ptp->Cch() == 0 
                &&  (!fDOMPartition || ptp->TextID() == 0 ) )
        {
            if (AttachedTreePos() == ptp)
            {
                Assert( AttachDirection() == eDir );
                Move( eDir );
            }

            Verify( ! pMarkup->Remove( ptp ) );
        }

        ptp = ptpNext;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Class:      CChildIterator
//
///////////////////////////////////////////////////////////////////////////////

CChildIterator::CChildIterator( 
    CElement * pElementParent,
    CElement * pElementChild,
    DWORD dwFlags,
    ELEMENT_TAG *pStopTags,
    long cStopTags,
    ELEMENT_TAG *pChildTags,
    long cChildTags)
{
    AssertSz( !(dwFlags & ~CHILDITERATOR_PUBLICMASK), "Invalid flags passed to CChildIterator constructor" );
    _dwFlags = dwFlags;

    Assert( pElementParent );

    if( pElementChild )
    {
        _pNodeChild = pElementChild->GetFirstBranch();
        Assert( _pNodeChild );
    
        _pNodeParent = _pNodeChild->SearchBranchToRootForScope( pElementParent );
        Assert( _pNodeParent );
    }
    else
    {
        SetBeforeBeginBit();
        _pNodeChild = NULL;
        _pNodeParent = pElementParent->GetFirstBranch();
    }

    AssertSz( _pNodeParent, "CChildIterator used with parent not in tree -- not a tree bug");

    if (UseTags())
    {
        _pStopTags = pStopTags;
        _cStopTags = cStopTags;

        _pChildTags = pChildTags;
        _cChildTags = cChildTags;
    }
    
#if DBG == 1
    if(_pNodeParent)
        _lTreeVersion = _pNodeParent->Doc()->GetDocTreeVersion();
#endif
}


void
CChildIterator::SetChild( CElement * pElementChild )
{
    WHEN_DBG( Invariant() );

    Assert( pElementChild && pElementChild->GetFirstBranch() );

    _pNodeChild = pElementChild->GetFirstBranch();
    _pNodeParent = _pNodeChild->SearchBranchToRootForScope( _pNodeParent->Element() );

    Assert( _pNodeParent );
}

void
CChildIterator::SetBeforeBegin( )
{
    WHEN_DBG( Invariant() );

    _pNodeChild = NULL;
    _pNodeParent = _pNodeParent->Element()->GetFirstBranch();

    Assert( _pNodeParent );

    SetBeforeBeginBit();
    ClearAfterEndBit();
}

void
CChildIterator::SetAfterEnd( )
{
    WHEN_DBG( Invariant() );

    _pNodeChild = NULL;
    _pNodeParent = _pNodeParent->Element()->GetFirstBranch();

    while (_pNodeParent->NextBranch())
        _pNodeParent = _pNodeParent->NextBranch();

    Assert( _pNodeParent );

    ClearBeforeBeginBit();
    SetAfterEndBit();
}

CTreeNode * 
CChildIterator::NextChild()
{
    WHEN_DBG( Invariant() );

    // If we are after the end of the child list,
    // we can't go any further
    if( IsAfterEnd() )
        return NULL;

    CTreePos * ptpCurr;
    CTreeNode *pNodeParentCurr = _pNodeParent;
    BOOL       fFirst = TRUE;

    //
    // Decide where to start.  
    // * If we already have a child either start after that 
    //   child or just inside of it, dependent on the virtual 
    //   IsRecursionStopChildNode call.
    // * If we are before begin, start just inside of the
    //   parent node.
    //
    if( _pNodeChild )
    {
        ptpCurr = IsRecursionStopChildNode( _pNodeChild )
            ? _pNodeChild->GetEndPos()
            : _pNodeChild->GetBeginPos();
    }
    else
    {
        Assert( IsBeforeBeginBit() );
        ClearBeforeBeginBit();

        ptpCurr = _pNodeParent->GetBeginPos();
    }

    Assert( ptpCurr && !ptpCurr->IsUninit() );

    //
    // Do this loop while we have a parent that we are interested in
    //
    while (pNodeParentCurr)
    {
        CTreePos *  ptpParentEnd = pNodeParentCurr->GetEndPos();

        fFirst = FALSE;

        // Since this is a good parent, set the member variable
        _pNodeParent = pNodeParentCurr;

        //
        // Loop through everything under this parent.
        //
        for( ptpCurr = ptpCurr->NextTreePos();
             ptpCurr != ptpParentEnd;
             ptpCurr = ptpCurr->NextTreePos() )
        {
            // If this is a begin node, investigate further
            if( ptpCurr->IsBeginNode() )
            {
                CTreeNode * pNodeChild = ptpCurr->Branch();

                // return this node if we have an edge scope and
                // the child is interesting.
                if( ptpCurr->IsEdgeScope() && 
                    IsInterestingChildNode( pNodeChild ) )
                {
                    _pNodeChild = pNodeChild;
                    return _pNodeChild;
                }
                
                // Decide if we want to skip over this node's
                // subtree
                if( IsRecursionStopChildNode( pNodeChild ) )
                {
                    ptpCurr = pNodeChild->GetEndPos();
                }
            }
        }

        // Move on to the next parent node
        pNodeParentCurr = pNodeParentCurr->NextBranch();
        if( pNodeParentCurr )
        {
            ptpCurr = pNodeParentCurr->GetBeginPos();
        }
    }

    // If we got here, we ran out of parent nodes, so we must be
    // after the end.
    SetAfterEndBit();
    _pNodeChild = NULL;
    return NULL;
}

CTreeNode * 
CChildIterator::PreviousChild()
{
    WHEN_DBG( Invariant() );

    // If we are after the end of the child list,
    // we can't go any further
    if( IsBeforeBegin() )
        return NULL;

    CTreePos * ptpCurr;
    CTreeNode *pNodeParentCurr = _pNodeParent;

    //
    // Decide where to start.  
    // * If we already have a child either start before that 
    //   child or just inside of it, dependent on the virtual 
    //   IsRecursionStopChildNode call.
    // * If we are after end, start just inside of the
    //   parent node.
    //
    if( _pNodeChild )
    {
        ptpCurr = IsRecursionStopChildNode( _pNodeChild )
            ? _pNodeChild->GetBeginPos()
            : _pNodeChild->GetEndPos();
    }
    else
    {
        Assert( IsAfterEndBit() );
        ClearAfterEndBit();

        ptpCurr = _pNodeParent->GetEndPos();
    }

    while( pNodeParentCurr )
    {
        CTreePos * ptpStop = pNodeParentCurr->GetBeginPos();

        // Since this is a good parent, set the member variable
        _pNodeParent = pNodeParentCurr;

        //
        // Loop through everything under this parent.
        //
        for( ptpCurr = ptpCurr->PreviousTreePos();
             ptpCurr != ptpStop;
             ptpCurr = ptpCurr->PreviousTreePos() )
        {
            Assert(     ! ptpCurr->IsNode() 
                    ||  ptpCurr->Branch()->SearchBranchToRootForNode( pNodeParentCurr ) );

            // If this is a begin node, investigate further
            if( ptpCurr->IsEndNode() )
            {
                CTreeNode * pNodeChild = ptpCurr->Branch();
                CTreePos * ptpBegin = pNodeChild->GetBeginPos();

                // return this node if we have an edge scope and
                // the child is interesting.
                if( ptpBegin->IsEdgeScope() && 
                    IsInterestingChildNode( pNodeChild ) )
                {
                    _pNodeChild = pNodeChild;
                    return _pNodeChild;
                }
                
                // Decide if we want to skip over this nodes
                // subtree
                if( IsRecursionStopChildNode( pNodeChild ) )
                {
                    ptpCurr = ptpBegin;
                }
            }
        }

        // Find the previous node in the context chain
        if( pNodeParentCurr == pNodeParentCurr->Element()->GetFirstBranch() )
        {
            pNodeParentCurr = NULL;
        }
        else
        {
            CTreeNode * pNodeTemp = pNodeParentCurr->Element()->GetFirstBranch();
            while( pNodeTemp->NextBranch() != pNodeParentCurr )
            {
                pNodeTemp = pNodeTemp->NextBranch();
            }
            Assert( pNodeTemp );
            pNodeParentCurr = pNodeTemp;
        }

        if( pNodeParentCurr )
        {
            ptpCurr = pNodeParentCurr->GetEndPos();
        }
    }

    // If we got here, we ran out of parent nodes, so we must be
    // before begin.
    SetBeforeBeginBit();
    _pNodeChild = NULL;
    return NULL;
}

#if DBG==1
void
CChildIterator::Invariant()
{
    AssertSz( _pNodeParent->Doc()->GetDocTreeVersion() == _lTreeVersion,
              "CChildIterator used while tree is changing" );
    Assert( !!IsBeforeBeginBit() + !!IsAfterEndBit() + !!_pNodeChild == 1 );
}
#endif


BOOL
CChildIterator::IsInterestingNode(ELEMENT_TAG * pary, long c, CTreeNode * pNode)
{
    if (UseLayout())
    {
        return pNode->NeedsLayout();
    }

    if (UseTags() && c > 0)
    {
        long   i;

        for (i = 0; i < c; i++, pary++)
        {
            if (*pary == pNode->Tag())
                return TRUE;
        }

        return FALSE;
    }

    return TRUE;
}
