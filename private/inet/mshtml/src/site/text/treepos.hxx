#ifndef I_TREEPOS_HXX_
#define I_TREEPOS_HXX_
#pragma INCMSG("--- Beg 'treepos.hxx'")

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

MtExtern(CChildIterator);

//+---------------------------------------------------------------------------
//
//  Class:      CTreePosGap
//
//  This is a lightweight object that points to a gap between two CTreePos
//  objects.  Here are the rules for its use:
//  o You specify the desired gap by giving a CTreePos and a bit that indicates
//    whether the gap to its left or its right is the one you want.
//  o There's a bunch of methods for getting information about the gap and
//    its neighborhood.
//  o Once positioned, a CTreePosGap can move left or right.  Special cases
//    are provided for moving to "valid" gaps (ones where it's permissible
//    to insert text), moving to gaps adjacent to text, or skipping over
//    tree pointers.
//  o You can restrict a CTreePosGap to the scope of a particular element.
//    It will fail to navigate anywhere to the left of the element's first
//    Node treepos, or to the right of the element's last Node treepos.
//  o The CTreePosGap "attaches" to a CTreePos adjacent to its gap.  There are
//    always two choices.  By default it picks whichever CTreePos is more
//    convenient for the implementation, but you can ask the CTreePosGap to
//    always attach to the CTreePos on its left or on its right.
//  o The CTreePosGap relies heavily on the CTreePos it attaches to.  It is
//    your responsibility to ensure that you don't move or delete a CTreePos
//    while a CTreePosGap is attached.  There's some debug-only code to help
//    catch violations of this rule.  If you have to move or delete a CTreePos,
//    call UnPosition on the CTreePosGap's first to detach them
//    (it's a no-op in ship build).
//  o A CTreePosGap is designed to be used only as a local variable to help
//    navigation through the HTML tree.  It is not part of the tree itself.
//    Changes to the tree that affect its CTreePos may (and probably will)
//    lead to chaos and bugs.  It's perfectly safe to navigate through a
//    static tree (as the measurer does), or to insert new content into a gap,
//    as long as you understand that the CTreePosGap will remain attached to
//    the same CTreePos.  To enforce the ephemeral intent, there's no operator
//    new constructor; you can't allocate a CTreePosGap from the heap.
//    Though we have a copy constructor, we *SHOULD NOT* pass it by value
//    and should only be used to create clones.
//----------------------------------------------------------------------------

enum TPG_DIRECTION
{
    TPG_EITHER = 0,
    TPG_LEFT   = 1,
    TPG_RIGHT  = 2
};

enum TPG_MOVE_FLAGS
{
    TPG_VALIDGAP      = 0x1,
    TPG_SKIPPOINTERS  = 0x2,
    TPG_FINDTEXT      = 0x4,
    TPG_FINDEDGESCOPE = 0x8,
    TPG_OKNOTTOMOVE   = 0x10,
};

class CTreePosGap
{
public:
    
    // constructors / destructor
    
    CTreePosGap(
        TPG_DIRECTION eAttach          = TPG_EITHER,
        CElement *    pElementRestrict = NULL );
    
    CTreePosGap(
        CTreePos *    ptp,
        TPG_DIRECTION eDir,
        TPG_DIRECTION eAttach          = TPG_EITHER,
        CElement *    pElementRestrict = NULL );

    CTreePosGap ( const CTreePosGap & that );

    WHEN_DBG( ~ CTreePosGap() { UnPosition(); } )

    // information

    long GetCp ( );

    CMarkup *   GetAttachedMarkup ( ) { Assert( IsPositioned() ); return AttachedTreePos()->GetMarkup(); }
    CElement *  RestrictingElement() const { return _pElemRestrict; }
    TPG_DIRECTION   AttachDirection() const { return _fLeft ? TPG_LEFT : TPG_RIGHT; }
    CTreePos *  AttachedTreePos() const { return _ptpAttach; }
    CTreePos *  AdjacentTreePos(TPG_DIRECTION eDir) const;
    CTreeNode * Branch() const;
    BOOL        IsValid() const;
    CTreeNode * SearchBranchForElement(CElement *pElement) const;
    int         operator==(const CTreePosGap& tpg) const;    // true if same gap
    int         operator!=(const CTreePosGap& tpg) const { return !operator==(tpg); }


    // state change
    void    SetMoveDirection(TPG_DIRECTION);
    void    SetAttachPreference(TPG_DIRECTION);
    void    SetRestrictingElement(CElement *pElemRestrict);

    // navigation
    BOOL    IsPositioned() const { return !!_ptpAttach; }
    void    UnPosition();
    HRESULT MoveTo(const CTreePosGap *ptpg);
    HRESULT MoveTo(CTreePos *ptp, TPG_DIRECTION eDir);
    HRESULT MoveToCp(CMarkup * pMarkup, long cp);
    HRESULT Move(DWORD dwMoveFlags=0, CTreePos **pptpEndCrossed=NULL);
    HRESULT Move(TPG_DIRECTION eDir, DWORD dwMoveFlags=0, CTreePos **pptpEndCrossed=NULL);
    HRESULT MoveRight(DWORD dwMoveFlags=0, CTreePos **pptpEndCrossed=NULL)
            { return Move(TPG_RIGHT, dwMoveFlags, pptpEndCrossed); }
    HRESULT MoveLeft(DWORD dwMoveFlags=0, CTreePos **pptpEndCrossed=NULL)
            { return Move(TPG_LEFT, dwMoveFlags, pptpEndCrossed); }

    // tree modification
    void    PartitionPointers( CMarkup * pMarkup, BOOL fDOMPartition );
    void    CleanCling( CMarkup * pMarkup, TPG_DIRECTION eDir, BOOL fDOMPartition );

private:
    // helper functions
    HRESULT MoveImpl(BOOL fLeft, DWORD dwMoveFlags, CTreePos **pptpEndCrossed);

    // representation
    CElement    *_pElemRestrict; // don't navigate outside scope of this element
    CTreePos    *_ptpAttach;    // treepos I'm attached to (or NULL)
    unsigned    _fLeft:1;       // true if my treepos is on my left
    unsigned    _eAttach:2;     // how my user wants me to attach
    unsigned    _fMoveLeft:1;   // true if I'm moving left

    // these methods aren't implemented.  They enforce prohibited use.
#ifndef UNIX
    void *          operator new(size_t cb);        // no operator new
    CTreePosGap&    operator=(const CTreePosGap&);  // no assignment
#endif
};

inline
CTreePosGap::CTreePosGap ( TPG_DIRECTION eAttach, CElement * pElementRestrict )
  : _ptpAttach( NULL ), _eAttach( eAttach ), _pElemRestrict( pElementRestrict )
{
}

inline
CTreePosGap::CTreePosGap (
    CTreePos * ptp, TPG_DIRECTION eDir, TPG_DIRECTION eAttach, CElement * pElementRestrict )
  : _ptpAttach( NULL ), _eAttach( eAttach ), _pElemRestrict( pElementRestrict )
{
    Verify( MoveTo( ptp, eDir ) == S_OK );
}

inline long
CTreePosGap::GetCp ( )
{
    Assert( IsPositioned() );
    return _ptpAttach->GetCp() + (_fLeft ? _ptpAttach->GetCch() : 0);
}

inline
CTreePosGap::CTreePosGap ( const CTreePosGap & that )
{
    Assert( & that != this );
    memcpy( this, & that, sizeof( CTreePosGap ) );
#if DBG==1
    if (_ptpAttach)
    {
        _ptpAttach->AttachGap();
    }
#endif
}

inline void
CTreePosGap::UnPosition()
{
#if DBG==1
    if (_ptpAttach)
    {
        _ptpAttach->DetachGap();
    }
#endif
    _ptpAttach = NULL;
}

inline HRESULT
CTreePosGap::MoveTo(const CTreePosGap *ptpg)
{
    Assert(ptpg);
    if(!ptpg->IsPositioned())
    {
        UnPosition();
        return S_OK;
    }
    else
    {
        return MoveTo( ptpg->_ptpAttach, ptpg->_fLeft ? TPG_RIGHT : TPG_LEFT );
    }
}


inline CTreeNode *
CTreePosGap::SearchBranchForElement(CElement *pElement) const
{
    AssertSz(_ptpAttach, "Gap not positioned");
    return _ptpAttach->SearchBranchForElement(pElement, !_fLeft);
}


inline int
CTreePosGap::operator==(const CTreePosGap& tpg) const    // true if same gap
{
    AssertSz(_ptpAttach && tpg._ptpAttach, "Gap is not positioned");
    CTreePos *ptp = (_fLeft == tpg._fLeft) ? _ptpAttach :
                _fLeft ? _ptpAttach->NextTreePos() : _ptpAttach->PreviousTreePos();
    return (ptp == tpg._ptpAttach);
}


inline void
CTreePosGap::SetMoveDirection(TPG_DIRECTION eDir)
{
    Assert(eDir==TPG_LEFT || eDir==TPG_RIGHT);

    _fMoveLeft = (eDir == TPG_LEFT);
}


inline void
CTreePosGap::SetRestrictingElement(CElement *pElemRestrict)
{
    AssertSz(!_ptpAttach || !pElemRestrict,
            "Can't change restriction while positioned");
    _pElemRestrict = pElemRestrict;
}


inline HRESULT
CTreePosGap::Move(DWORD dwMoveFlags, CTreePos **pptpEndCrossed)
{
    return MoveImpl(_fMoveLeft, dwMoveFlags, pptpEndCrossed);
}

inline HRESULT
CTreePosGap::Move(TPG_DIRECTION eDir, DWORD dwMoveFlags, CTreePos **pptpEndCrossed)
{
    AssertSz(_ptpAttach, "Gap is not positioned");
    Assert(eDir == TPG_LEFT || eDir == TPG_RIGHT);
    BOOL fLeft = (eDir == TPG_LEFT);

    return MoveImpl(fLeft, dwMoveFlags, pptpEndCrossed);
}

inline HRESULT
CMarkup::Remove(CTreePosGap *ptpgStart, CTreePosGap *ptpgFinish)
{
    return Remove( ptpgStart->AdjacentTreePos( TPG_RIGHT ), 
                   ptpgFinish->AdjacentTreePos( TPG_LEFT ) );
}

///////////////////////////////////////////////////////////////////////////////
//
//  Class:      CChildIterator
//
//  * This class can be derived from and used to walk all of the children 
//    of an element, node or part of an element.  
//  * The derived class can decide to skip over children (for a direct 
//    child walk) or to look just at direct children.  
//  * Any derived class can also decide which nodes of the the parent 
//    are interesting.
//  * All walks are in source order.
//  * Default behavior walks all direct children
//
//  Possible Uses:
//  * Child Layouts: the parent is only interested in children that are
//    Layouts.  It also only wants direct child layouts.  Also, if the 
//    layout for the parent does not extend over the entire scope of the
//    parent, the iterator can stop early.
//  * Select Elements: This can be used to find all of the option elements
//    under a select.
//
//  Notes:
//  * The array that you pass into the constructor must be good for the
//    life of the child iterator.  The ci just caches the pointer.
//
///////////////////////////////////////////////////////////////////////////////

#define CHILDITERATOR_USEDEFAULT    0x0
#define CHILDITERATOR_USELAYOUT     0x1
#define CHILDITERATOR_USETAGS       0x2
#define CHILDITERATOR_DEEP          0x4
#define CHILDITERATOR_PUBLICMASK    0x7

// These are private and should not be passed
// into the constructor
#define CHILDITERATOR_BEFOREBEGIN   0x8
#define CHILDITERATOR_AFTEREND     0x10


class CChildIterator
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CChildIterator))
    
    //
    // Constructors
    //
    CChildIterator(CElement * pElementParent,
                   CElement * pElementChild = NULL,
                   DWORD dwFlags = CHILDITERATOR_USEDEFAULT,
                   ELEMENT_TAG  *pStopTags = NULL,
                   long     cStopTags = 0,
                   ELEMENT_TAG *pChildTags = NULL,
                   long cChildTags = 0);

    // NOTE: Don't use this unless you understand overlapping
    //       very well.
    //
    // This constructor is provided as an optimization.
    // If nodes are not provided, the element version
    // constructor has to do some hunting around to find
    // the right nodes
    CChildIterator( CTreeNode *pNodeParent,
                    CTreeNode *pNodeChild );

    //
    // Move functions
    //
    CTreeNode * NextChild();
    CTreeNode * PreviousChild();

    //
    // Set functions
    //
    void        SetChild( CElement *pChild );
    void        SetBeforeBegin();
    void        SetAfterEnd();

    //
    // Data Accessors
    //

    CTreeNode * ChildNode() { WHEN_DBG( Invariant() ); return _pNodeChild; };
    CElement *  Child() { WHEN_DBG( Invariant() ); return _pNodeChild->SafeElement(); }

    CTreeNode * ParentNode() { WHEN_DBG( Invariant() ); return _pNodeParent; }
    CElement *  Parent() { Assert( _pNodeParent ); WHEN_DBG( Invariant() ); return _pNodeParent->Element(); }

    BOOL        IsBeforeBegin() { WHEN_DBG( Invariant() ); return IsBeforeBeginBit(); }
    BOOL        IsAfterEnd() { WHEN_DBG( Invariant() ); return IsAfterEndBit(); }

    BOOL        IsLookDeep() { WHEN_DBG( Invariant() ); return IsDeepBit(); }
    void        SetDeep() { WHEN_DBG( Invariant() ); SetDeepBit(); }
    void        ClearDeep() { WHEN_DBG( Invariant() ); ClearDeepBit(); }

private:
    //
    // Behavior defining functions
    //

    // If this function returns TRUE, no children of this node are
    // examined
    BOOL IsRecursionStopChildNode(CTreeNode * pNode);

    // If this function returns FALSE, the iterator does not stop
    // at this node
    BOOL IsInterestingChildNode(CTreeNode * pNode);


    BOOL IsInterestingNode(ELEMENT_TAG * pary, long c, CTreeNode *pNode);
    

    //
    // Bit twiddling and access
    //
    BOOL        IsBeforeBeginBit() { return _dwFlags & CHILDITERATOR_BEFOREBEGIN; }
    void        SetBeforeBeginBit() { _dwFlags |= CHILDITERATOR_BEFOREBEGIN; }
    void        ClearBeforeBeginBit() { _dwFlags &= ~CHILDITERATOR_BEFOREBEGIN; }

    BOOL        IsAfterEndBit() { return _dwFlags & CHILDITERATOR_AFTEREND; }
    void        SetAfterEndBit() { _dwFlags |= CHILDITERATOR_AFTEREND; }
    void        ClearAfterEndBit() { _dwFlags &= ~CHILDITERATOR_AFTEREND; }

    BOOL        IsDeepBit() { return _dwFlags & CHILDITERATOR_DEEP; }
    void        SetDeepBit() { _dwFlags |= CHILDITERATOR_DEEP; }
    void        ClearDeepBit() { _dwFlags &= ~CHILDITERATOR_DEEP; }

    void        ClearOutOfBoundsBits() { _dwFlags &= ~( CHILDITERATOR_BEFOREBEGIN & CHILDITERATOR_AFTEREND ); }

    BOOL        UseLayout() { return _dwFlags & CHILDITERATOR_USELAYOUT; }
    BOOL        UseTags() { return _dwFlags & CHILDITERATOR_USETAGS; }

#if DBG == 1
    //
    // Version Number Checking
    //
    long _lTreeVersion;
    void Invariant();
#endif

    //
    // Class Data
    //

    CTreeNode *             _pNodeChild;
    CTreeNode *             _pNodeParent;

    ELEMENT_TAG  *          _pStopTags;
    long                    _cStopTags;
    ELEMENT_TAG *           _pChildTags;
    long                    _cChildTags;
    
    DWORD       _dwFlags;
};


inline
CChildIterator::CChildIterator( 
    CTreeNode *pNodeParent,
    CTreeNode *pNodeChild )
    :   _pNodeParent( pNodeParent ),
        _pNodeChild( pNodeChild ),
        _dwFlags( 0 )
{
    Assert( pNodeParent );
    Assert( pNodeChild );

    // Make sure the parent passed in is on the same branch as the child
    Assert( pNodeParent == pNodeChild->SearchBranchToRootForScope( pNodeParent->Element() ) );

#if DBG == 1
    _lTreeVersion = pNodeParent->Doc()->GetDocTreeVersion();
#endif
}

inline BOOL
CChildIterator::IsInterestingChildNode(CTreeNode *pNode)
{
    return IsInterestingNode(_pChildTags, _cChildTags, pNode);
}

//

inline BOOL
CChildIterator::IsRecursionStopChildNode(CTreeNode *pNode)
{
    return (!IsDeepBit() && IsInterestingNode(_pStopTags, _cStopTags, pNode));
}

///////////////////////////////////////////////////////////////////////////////
//
//  CTreePos inlines
//
///////////////////////////////////////////////////////////////////////////////

inline CTreePos *
CTreePos::Parent() const
{
    return IsLastChild()? Next() : Next()->Next();
}

inline CTreePos *
CTreePos::LeftChild() const
{
    return (FirstChild() && FirstChild()->IsLeftChild()) ? FirstChild() : NULL;
}

inline CTreePos *
CTreePos::RightChild() const
{
    if (!FirstChild())
        return NULL;
    if (!FirstChild()->IsLeftChild())
        return FirstChild();
    if (!FirstChild()->IsLastChild())
        return FirstChild()->Next();
    return NULL;
}

#pragma INCMSG("--- End 'treepos.hxx'")
#else
#pragma INCMSG("*** Dup 'treepos.hxx'")
#endif
