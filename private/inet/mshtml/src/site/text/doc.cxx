/*
 *  @doc INTERNAL
 *
 *  @module DOC.C   CTxtStory and CTxtArray implementation |
 *
 *  Original Authors: <nl>
 *      Original RichEdit code: David R. Fulmer <nl>
 *      Christian Fortini   <nl>
 *      Murray Sargent <nl>
 *
 *  History: <nl>
 *      6/25/95 alexgo  Cleanup and reorganization
 *
 */

#include "headers.hxx"

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_EFONT_HXX_
#define X_EFONT_HXX_
#include "efont.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_UNISID_H_
#define X_UNISID_H_
#include <unisid.h>
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"

#endif

MtDefine(CTxtBlk, Tree, "CTxtBlk")
MtDefine(CTxtBlkData, CTxtBlk, "CTxtBlk data")
MtDefine(CTxtArray, Tree, "CTxtArray")
MtDefine(CTxtArray_pv, CTxtArray, "CTxtArray::_pv")
MtDefine(CRunArray, Tree, "CRunArray")
MtDefine(CRunArray_pv, CRunArray, "CRunArray::_pv")
MtDefine(CLineArray, Tree, "CLineArray")
MtDefine(CLineArray_pv, CLineArray, "CLineArray::_pv")
MtDefine(CListCache, Tree, "CListCache")
MtDefine(CListCacheInst, Tree, "CListCacheInst")
MtDefine(CListIndexArray, Tree, "CListIndexArray")
MtDefine(CListIndexArray_pv, CListIndexArray, "CListIndexArray::_pv")
MtDefine(CCharFormat, Tree, "CCharFormat")
MtDefine(CParaFormat, Tree, "CParaFormat")
MtDefine(CFancyFormat, Tree, "CFancyFormat")

// ===========================  Invariant stuff  ======================

#define DEBUG_CLASSNAME CTxtArray
#include "_invar.h"

// ========================  CTxtArray class  =========================


#if DBG==1

/*
 *  CTxtArray::Invariant
 *
 *  @mfunc  Tests CTxtArray's state
 *
 *  @rdesc  returns TRUE always; failures are indicated by Asserts
 */

BOOL
CTxtArray::Invariant(  ) const
{
    static LONG numTests = 0;
    numTests++;             // how many times we've been called.

    if ( Count() > 0 )
    {

        // make sure total characters stored in the blocks match the length
        // that is stored in _cchText.
        long  cch = 0;
        DWORD i, iMax;

        iMax = Count();

        for ( i = 0; i < iMax; i++ )
        {
            CTxtBlk *ptb = Elem(i);

            // ptb shouldn't be NULL since we're within Count elements
            Assert(ptb);

            long currCch = ptb->_cch;

            cch += currCch;

            Assert ( currCch >= 0 );
            Assert ( currCch <= long(CchOfCb(ptb->_cbBlock)) );

            // while we're here, check the range of the interblock gaps.
            Assert (ptb->_ibGap >= 0);
            Assert (ptb->_ibGap <= ptb->_cbBlock);


            DWORD cchGap = CchOfCb(ptb->_ibGap);

            Assert ( cchGap >= 0 );

            Assert ( long(cchGap) <= currCch );

        }
        Assert ( cch == GetCch() );
    }

    return TRUE;
}

#endif

/*
 *  CTxtArray::CTxtArray
 *
 *  @mfunc      Text array constructor
 *
 */
CTxtArray::CTxtArray()
    : CArray<CTxtBlk>(Mt(CTxtArray_pv))
{
    AssertSz(CchOfCb(cbBlockMost) - cchGapInitial >= cchBlkInitmGapI * 2,
        "cchBlockMax - cchGapInitial must be at least (cchBlockInitial - cchGapInitial) * 2");

    _cchText = 0;
}

/*
 *  CTxtArray::~CTxtArray
 *
 *  @mfunc      Text array destructor
 */
CTxtArray::~CTxtArray()
{
    DWORD itb = Count();

    while(itb--)
    {
        Assert(Elem(itb) != NULL);
        Elem(itb)->FreeBlock();
    }
}

/*
 *  CTxtArray::RemoveAll
 *
 *  @mfunc      Removes all characters in the array
 */
void
CTxtArray::RemoveAll()
{
    DWORD itb = Count();

    while(itb--)
    {
        Assert(Elem(itb) != NULL);
        Elem(itb)->FreeBlock();
    }

    Clear( AF_DELETEMEM );

    _cchText = 0;
}


/*
 *  CTxtArray::GetCch()
 *
 *  @mfunc      Computes and return length of text in this text array
 *
 *  @rdesc      The number of character in this text array
 *
 *  @devnote    This call may be computationally expensive; we have to
 *              sum up the character sizes of all of the text blocks in
 *              the array.
 */

long
CTxtArray::GetCch ( ) const
{
//    _TEST_INVARIANT_

    DWORD itb = Count();
    long  cch = 0;

    while (itb--)
    {
        Assert(Elem(itb) != NULL);
        cch += Elem(itb)->_cch;
    }

    return cch;
}

/*
 *  CTxtArray::AddBlock(itbNew, cb)
 *
 *  @mfunc      create new text block
 *
 *  @rdesc
 *      FALSE if block could not be added
 *      non-FALSE otherwise
 *
 *  @comm
 *  Side Effects:
 *      moves text block array
 */
BOOL CTxtArray::AddBlock(
    DWORD   itbNew,     //@parm index of the new block
    LONG    cb)         //@parm size of new block; if <lt>= 0, default is used
{
    _TEST_INVARIANT_

    CTxtBlk *ptb;

    if(cb <= 0)
        cb = cbBlockInitial;

    AssertSz(cb > 0, "CTxtArray::AddBlock() - adding block of size zero");
    AssertSz(cb <= cbBlockMost, "CTxtArray::AddBlock() - block too big");

    ptb = Insert(itbNew, 1);

    if( !ptb || !ptb->InitBlock(cb))
    {
        TraceTag((tagError, "TXTARRAT::AddBlock() - unable to allocate new block"));
        return FALSE;
    }

    return TRUE;
}

/*
 *  CTxtArray::SplitBlock(itb, ichSplit, cchFirst, cchLast, fStreaming)
 *
 *  @mfunc      split a text block into two
 *
 *  @rdesc
 *      FALSE if the block could not be split <nl>
 *      non-FALSE otherwise
 *
 *  @comm
 *  Side Effects: <nl>
 *      moves text block array
 */
BOOL CTxtArray::SplitBlock(
    DWORD itb,          //@parm index of the block to split
    DWORD ichSplit,     //@parm character index within block at which to split
    DWORD cchFirst,     //@parm desired extra space in first block
    DWORD cchLast,      //@parm desired extra space in new block
    BOOL fStreaming)    //@parm TRUE if streaming in new text
{
    _TEST_INVARIANT_

    LPBYTE pbSrc;
    LPBYTE pbDst;
    CTxtBlk *ptb, *ptb1;

    AssertSz(ichSplit > 0 || cchFirst > 0, "CTxtArray::SplitBlock(): splitting at beginning, but not adding anything");

    AssertSz(itb >= 0, "CTxtArray::SplitBlock(): negative itb");
    ptb = Elem(itb);

    // compute size for first half

    AssertSz(cchFirst + ichSplit <= CchOfCb(cbBlockMost),
        "CTxtArray::SplitBlock(): first size too large");
    cchFirst += ichSplit + cchGapInitial;
    // Does not work!, need fail to occur. jonmat cchFirst = min(cchFirst, CchOfCb(cbBlockMost));
    // because our client expects cchFirst chars.

    // BUGBUG (cthrash) I *think* this should work but I also *know* this
    // code needs to be revisited.  Basically, there are Asserts sprinkled
    // about the code requiring the _cbBlock < cbBlockMost.  We can of course
    // exceed that if we insist on tacking on cchGapInitial (See AssertSz
    // above).  My modifications make certain you don't.  It would seem fine
    // except for the comment by jonmat.

    // (cthrash) jonmat's comment makes no sense to me.  The new buffer size
    // *wants* to cchFirst + ichSplit + cchGapInitial; it seems to me that
    // it'll be ok as long as we give it a gap >= 0.
    cchFirst = min(cchFirst, (DWORD)CchOfCb(cbBlockMost));

    // compute size for second half

    AssertSz(cchLast + ptb->_cch - ichSplit <= CchOfCb(cbBlockMost),
        "CTxtArray::SplitBlock(): second size too large");
    cchLast += ptb->_cch - ichSplit + cchGapInitial;
    // Does not work!, need fail to occur. jonmat cchLast = min(cchLast, CchOfCb(cbBlockMost));
    // because our client expects cchLast chars.

    // (cthrash) see comment above with cchFirst.
    cchLast = min(cchLast, (DWORD)CchOfCb(cbBlockMost));

    // allocate second block and move text to it

    // ***** moves rgtb ***** //
    // if streaming in, allocate a block that's as big as possible so that
    // subsequent additions of text are faster
    // we always fall back to smaller allocations so this won't cause
    // unneccesary errors
    // when we're done streaming we compress blocks, so this won't leave
    // a big empty gap
    if(fStreaming)
    {
        DWORD cb = cbBlockMost;
        const DWORD cbMin = CbOfCch(cchLast);

        while(cb >= cbMin && !AddBlock(itb + 1, cb))
            cb -= cbBlockCombine;
        if(cb >= cbMin)
            goto got_block;
    }
    if(!AddBlock(itb + 1, CbOfCch(cchLast)))
    {
        TraceTag((tagError, "CTxtArray::SplitBlock(): unabled to add new block"));
        return FALSE;
    }

got_block:
    ptb1 = Elem(itb+1); // recompute ptb after rgtb moves
    ptb = Elem(itb);    // recompute ptb after rgtb moves
    ptb1->_cch = ptb->_cch - ichSplit;
    ptb1->_ibGap = 0;
    pbDst = (LPBYTE) (ptb1->_pch - ptb1->_cch) + ptb1->_cbBlock;
    ptb->MoveGap(ptb->_cch); // make sure pch points to a continuous block of all text in ptb.
    pbSrc = (LPBYTE) (ptb->_pch + ichSplit);
    CopyMemory(pbDst, pbSrc, CbOfCch(ptb1->_cch));
    ptb->_cch = ichSplit;
    ptb->_ibGap = CbOfCch(ichSplit);

    // resize the first block
    if(CbOfCch(cchFirst) != ptb->_cbBlock)
    {
//$ FUTURE: don't resize unless growing or shrinking considerably
        if(!ptb->ResizeBlock(CbOfCch(cchFirst)))
        {
            // Review, if this fails then we need to delete all of the Added blocks, right? jonmat
            TraceTag((tagError, "TXTARRA::SplitBlock(): unabled to resize block"));
            return FALSE;
        }
    }

    return TRUE;
}


/*
 *  CTxtArray::ShrinkBlocks()
 *
 *  @mfunc      Shrink all blocks to their minimal size
 *
 *  @rdesc
 *      nothing
 *
 */
void CTxtArray::ShrinkBlocks()
{
    _TEST_INVARIANT_

    DWORD itb = Count();
    CTxtBlk *ptb;

    while(itb--)
    {
        ptb = Elem(itb);
        Assert(ptb);
        ptb->ResizeBlock(CbOfCch(ptb->_cch));
    }
}


/*
 *  CTxtArray::RemoveBlocks(itbFirst, ctbDel)
 *
 *  @mfunc      remove a range of text blocks
 *
 *  @rdesc
 *      nothing
 *
 *  @comm Side Effects: <nl>
 *      moves text block array
 */
VOID CTxtArray::RemoveBlocks(
    DWORD itbFirst,         //@parm index of first block to remove
    DWORD ctbDel)           //@parm number of blocks to remove
{
    _TEST_INVARIANT_

    DWORD itb = itbFirst;
    DWORD ctb = ctbDel;

    AssertSz(itb + ctb <= Count(), "CTxtArray::RemoveBlocks(): not enough blocks");

    while(ctb--)
    {
        Assert(Elem(itb) != NULL);
        Elem(itb++)->FreeBlock();
    }

    Remove(itbFirst, ctbDel, AF_KEEPMEM);
}


/*
 *  CTxtArray::CombineBlocks(itb)
 *
 *  @mfunc      combine adjacent text blocks
 *
 *  @rdesc
 *      TRUE if blocks were combined, otherwise false
 *
 *  @comm
 *  Side Effects: <nl>
 *      moves text block array
 *
 *  @devnote
 *      scans blocks from itb - 1 through itb + 1 trying to combine
 *      adjacent blocks
 */
BOOL CTxtArray::CombineBlocks(
    DWORD itb)      //@parm index of the first block modified
{
    _TEST_INVARIANT_

    DWORD ctb;
    DWORD cbT;
    CTxtBlk *ptb, *ptb1;
    BOOL  fRet = FALSE;

    if(itb > 0)
        itb--;

    ctb = min(3, int(Count() - itb));
    if(ctb <= 1)
        return FALSE;

    for(; ctb > 1; ctb--)
    {
        ptb  = Elem(itb);                       // Can we combine current
        ptb1 = Elem(itb+1);                     //  and next blocks ?
        cbT = CbOfCch(ptb->_cch + ptb1->_cch + cchGapInitial);
        if(cbT <= cbBlockInitial)
        {                                           // Yes
            if(cbT != ptb->_cbBlock && !ptb->ResizeBlock(cbT))
                continue;
            ptb ->MoveGap(ptb->_cch);               // Move gaps at ends of
            ptb1->MoveGap(ptb1->_cch);              //  both blocks
            CopyMemory(ptb->_pch + ptb->_cch,       // Copy next block text
                ptb1->_pch, CbOfCch(ptb1->_cch));   //  into current block
            ptb->_cch += ptb1->_cch;
            ptb->_ibGap += CbOfCch(ptb1->_cch);
            RemoveBlocks(itb+1, 1);                 // Remove next block
            fRet = TRUE;
        }
        else
            itb++;
    }

    return fRet;
}

/*
 *  CTxtArray::GetChunk(ppch, cch, pchChunk, cchCopy)
 *
 *  @mfunc
 *      Get content of text chunk in this text array into a string
 *
 *  @rdesc
 *      remaining count of characters to get
 */
LONG CTxtArray::GetChunk(
    TCHAR **ppch,           //@parm ptr to ptr to buffer to copy text chunk into
    DWORD cch,              //@parm length of pch buffer
    TCHAR *pchChunk,        //@parm ptr to text chunk
    DWORD cchCopy) const    //@parm count of characters in chunk
{
    _TEST_INVARIANT_

    if(cch > 0 && cchCopy > 0)
    {
        if(cch < cchCopy)
            cchCopy = cch;                      // Copy less than full chunk
        CopyMemory(*ppch, pchChunk, cchCopy*sizeof(TCHAR));
        *ppch   += cchCopy;                     // Adjust target buffer ptr
        cch     -= cchCopy;                     // Fewer chars to copy
    }
    return cch;                                 // Remaining count to copy
}


// ========================  CTxtBlk class  =================================


/*
 *  CTxtBlk::InitBlock(cb)
 *
 *  @mfunc
 *      Initialize this text block
 *
 *  @rdesc
 *      TRUE if success, FALSE if allocation failed
 */
BOOL CTxtBlk::InitBlock(
    DWORD cb)           //@parm initial size of the text block
{
    _pch    = NULL;
    _cch    = 0;
    _ibGap  = 0;

    if (cb)
        _pch = (TCHAR*)MemAllocClear(Mt(CTxtBlkData), cb);

    MemSetName((_pch, "CTxtBlk data"));

    if (_pch)
    {
        _cbBlock = cb;
        return TRUE;
    }
    else
    {
        _cbBlock = 0;
        return FALSE;
    }
}

/*
 *  CTxtBlk::FreeBlock()
 *
 *  @mfunc
 *      Free this text block
 *
 *  @rdesc
 *      nothing
 */
VOID CTxtBlk::FreeBlock()
{
    MemFree(_pch);
    _pch    = NULL;
    _cch    = 0;
    _ibGap  = 0;
    _cbBlock= 0;
}

/*
 *  CTxtBlk::MoveGap(ichGap)
 *
 *  @mfunc
 *      move gap in this text block
 *
 *  @rdesc
 *      nothing
 */
void CTxtBlk::MoveGap(
    DWORD ichGap)           //@parm new position for the gap
{
    DWORD cbMove;
    DWORD ibGapNew = CbOfCch(ichGap);
    LPBYTE pbFrom = (LPBYTE) _pch;
    LPBYTE pbTo;

    if(ibGapNew == _ibGap)
        return;

    if(ibGapNew < _ibGap)
    {
        cbMove = _ibGap - ibGapNew;
        pbFrom += ibGapNew;
        pbTo = pbFrom + _cbBlock - CbOfCch(_cch);
    }
    else
    {
        cbMove = ibGapNew - _ibGap;
        pbTo = pbFrom + _ibGap;
        pbFrom = pbTo + _cbBlock - CbOfCch(_cch);
    }

    MoveMemory(pbTo, pbFrom, cbMove);
    _ibGap = ibGapNew;
}


/*
 *  CTxtBlk::ResizeBlock(cbNew)
 *
 *  @mfunc
 *      resize this text block
 *
 *  @rdesc
 *      FALSE if block could not be resized <nl>
 *      non-FALSE otherwise
 *
 *  @comm
 *  Side Effects: <nl>
 *      moves text block
 */
BOOL CTxtBlk::ResizeBlock(
    DWORD cbNew)        //@parm the new size
{
    TCHAR *pch;
    DWORD cbMove;
    HRESULT hr;

    AssertSz(cbNew > 0, "resizing block to size <= 0");

    if(cbNew < _cbBlock)
    {
        if(_ibGap != CbOfCch(_cch))
        {
            // move text after gap down so that it doesn't get dropped

            cbMove = CbOfCch(_cch) - _ibGap;
            pch = _pch + CchOfCb(_cbBlock - cbMove);
            MoveMemory(pch - CchOfCb(_cbBlock - cbNew), pch, cbMove);
        }
        _cbBlock = cbNew;
    }
    pch = _pch;
    hr = MemRealloc(Mt(CTxtBlkData), (void **) & pch, cbNew);
    if(hr)
        return _cbBlock == cbNew;   // FALSE if grow, TRUE if shrink

    _pch = pch;
    if(cbNew > _cbBlock)
    {
        if(_ibGap != CbOfCch(_cch))     // Move text after gap to end so that
        {                               // we don't end up with two gaps
            cbMove = CbOfCch(_cch) - _ibGap;
            pch += CchOfCb(_cbBlock - cbMove);
            MoveMemory(pch + CchOfCb(cbNew - _cbBlock), pch, cbMove);
        }
        _cbBlock = cbNew;
    }

    return TRUE;
}

HRESULT
CMarkup::CreateInitialMarkup( CRootElement * pElementRoot )
{
    HRESULT hr = S_OK;
    CTreeNode * pNodeRoot;
    CTreePos *  ptpNew;

    // Assert that there is nothing in the splay tree currently
    Assert( FirstTreePos() == NULL );

    pNodeRoot = new CTreeNode( NULL, pElementRoot );
    if ( !pNodeRoot )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // The initial ref on the node will transfer to the tree

    ptpNew = pNodeRoot->InitBeginPos( TRUE );
    Verify( ! Append(ptpNew) );

    ptpNew = pNodeRoot->InitEndPos( TRUE );
    Verify( ! Append(ptpNew) );

    pNodeRoot->PrivateEnterTree();

    pElementRoot->SetMarkupPtr( this );
    pElementRoot->__pNodeFirstBranch = pNodeRoot;
    pElementRoot->PrivateEnterTree();

    // Insert the chars for the node poses
    Verify(
        CTxtPtr( this, 0 ).
            InsertRepeatingChar( 2, WCH_NODE ) == 2 );

    {
        CNotification nf;
        nf.ElementEntertree( pElementRoot );
        pElementRoot->Notify( &nf );
    }

    UpdateMarkupTreeVersion();

    WHEN_DBG( _cchTotalDbg = 2 );
    WHEN_DBG( _cElementsTotalDbg = 1 );

Cleanup:
    RRETURN( hr );
}


#if DBG == 1
//+----------------------------------------------------------------------------
//
//  Member:     ValidateChange
//
//  Synopsis:   The markup maintains a redundant count of characters and
//              elements so that notifications can be validated.
//
//-----------------------------------------------------------------------------

void
CMarkup::ValidateChange(CNotification * pnf)
{
    switch ( pnf->Type() )
    {
    case NTYPE_CHARS_ADDED :
    {
        _cchTotalDbg += pnf->Cch(LONG_MAX);

        break;
    }
    case NTYPE_CHARS_DELETED :
    {
        _cchTotalDbg -= pnf->Cch(LONG_MAX);

        break;
    }

    case NTYPE_CHARS_RESIZE :
    case NTYPE_CHARS_INVALIDATE :
        break;

    case NTYPE_ELEMENTS_ADDED :
    {
        _cElementsTotalDbg += pnf->CElements();

        break;
    }

    case NTYPE_ELEMENTS_DELETED :
    {
        _cElementsTotalDbg -= pnf->CElements();

        break;
    }

    default :
        AssertSz( 0, "Unknown change kind" );
        break;
    }

    //
    // Make sure the debug count and the real doc are the same
    //

    Assert( !pnf->IsTextChange() 
        ||  pnf->_fNoTextValidate 
        || (    _cchTotalDbg == GetTextLength() 
            &&  _cchTotalDbg == Cch() ) );
    Assert( !pnf->IsTreeChange() 
        ||  pnf->_fNoElementsValidate 
        ||  _cElementsTotalDbg == NumElems() );
}

//+----------------------------------------------------------------------------
//
//  Member:     AreChangesValid
//
//  Synopsis:   Return if the counds match the real situation
//
//-----------------------------------------------------------------------------

BOOL
CMarkup::AreChangesValid()
{
    return  _cchTotalDbg == GetTextLength()
        &&  _cchTotalDbg == Cch()
        &&  _cElementsTotalDbg == NumElems();
}
#endif

//+----------------------------------------------------------------------------
//
//  Member:     FindMyListContainer
//
//  Synopsis:   Searches the given branch (potentially starting above the base
//              of the branch) for the first list element above the start of
//              the search.
//
//   Returns:   Returns the element along the branch which has the same scope
//              as the given element.
//
//     Notes:   Stops searching after the CTxtSite goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::FindMyListContainer ( CTreeNode * pNodeStartHere )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode = pNodeStartHere;

    //
    // If we get to the CTxtSite immediately, stop searching!
    //

    if (pNode->HasFlowLayout())
        return NULL;

    for ( ; ; )
    {
        CElement * pElementScope;

        pNode = pNode->Parent();
        
        if (!pNode)
            return NULL;

        pElementScope = pNode->Element();

        if (pNode->HasFlowLayout())
            return NULL;
        
        if (pElementScope->IsFlagAndBlock(TAGDESC_LIST))
            return pNode;
        
        if (pElementScope->IsFlagAndBlock(TAGDESC_LISTITEM))
            return NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForCriteria
//
//  Synopsis:   Searches the given branch for an element which meets
//              certain criteria starting from a given element.
//
//     Notes:   Stops searching after the CTxtSite goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForCriteria (
    CTreeNode * pNodeStartHere,
    BOOL (* pfnSearchCriteria) ( CTreeNode * ) )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode;

    for ( pNode = pNodeStartHere ; ; pNode = pNode->Parent() )
    {
        if (!pNode)
            return NULL;
        
        if (pfnSearchCriteria( pNode ))
            return pNode;

        if (pNode->HasFlowLayout())
            return NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForCriteriaInStory
//
//  Synopsis:   Searches the given branch for an element which meets
//              certain criteria starting from a given element.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForCriteriaInStory (
    CTreeNode * pNodeStartHere,
    BOOL (* pfnSearchCriteria) ( CTreeNode * ) )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode;

    for ( pNode = pNodeStartHere ; pNode; pNode = pNode->Parent() )
    {
        if (pfnSearchCriteria( pNode ))
            return pNode;
    }
    
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForAnchor
//
//  Synopsis:   Searches the given branch for an anchor element
//              starting from a given element.
//
//     Notes:   Stops searching after the CTxtSite goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForAnchor ( CTreeNode * pNodeStartHere )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode = pNodeStartHere;

    for ( ; ; )
    {
        if (!pNode || pNode->HasFlowLayout())
            return NULL;

        if (pNode->Tag() == ETAG_A)
            break;

        pNode = pNode->Parent();
    }

    return pNode;
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForAnchorLink
//
//  Synopsis:   Searches the given branch for an anchor element which
//              has a HREF starting from a given element.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForAnchorLink ( CTreeNode * pNodeStartHere )
{
    CTreeNode * pNodeAnchor = pNodeStartHere;

    for ( ; ; )
    {
        CAnchorElement * pAnchor;

        if (!pNodeAnchor)
            break;

        pNodeAnchor = SearchBranchForAnchor( pNodeAnchor );

        if (!pNodeAnchor)
            break;

        pAnchor = DYNCAST( CAnchorElement, pNodeAnchor->Element() );

        if (pAnchor->GetAAhref())
            break;

        pNodeAnchor = pNodeAnchor->Parent();
    }

    return pNodeAnchor;
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForTag
//
//  Synopsis:   Searches the given branch for an element with a given tag.
//              starting from a given element.
//
//   Returns:   Returns the element along the branch which has the tag.
//
//     Notes:   Stops searching after the CTxtSite goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForTag ( CTreeNode * pNodeStartHere, ELEMENT_TAG etag )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode;

    for ( pNode = pNodeStartHere ;
          pNode && etag != pNode->Tag() ;
          pNode = pNode->Parent() )
    {
        if (pNode->HasFlowLayout())
            return NULL;
    }

    return pNode;
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForTagInStory
//
//  Synopsis:   Searches the given branch for an element with a given tag.
//              starting from a given element.
//
//   Returns:   Returns the element along the branch which has the tag.
//
//     Notes:   Stops searching after the CTxtEdit goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForTagInStory (
    CTreeNode * pNodeStartHere, ELEMENT_TAG etag )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode;

    for ( pNode = pNodeStartHere ;
          pNode ;
          pNode = pNode->Parent() )
    {
        if ( etag == pNode->Tag() )
            return pNode;
    }

    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForScope
//
//  Synopsis:   Searches the given branch for the scope of a given element,
//              starting from a given element.
//
//   Returns:   Returns the element along the branch which has the same scope
//              as the given element.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForScope (
    CTreeNode * pNodeStartHere,
    CElement *  pElementFindMyScope )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode;

    for ( pNode = pNodeStartHere ; pNode ; pNode = pNode->Parent() )
    {
        CElement * pElementScope = pNode->Element();

        if (pElementFindMyScope == pElementScope)
            return pNode;

        if (pNode->HasFlowLayout())
            return NULL;
    }

    Assert( ! pNode );

    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForScopeInStory
//
//  Synopsis:   Searches the given branch for the scope of a given element,
//              starting from a given element.
//
//   Returns:   Returns the element along the branch which has the same scope
//              as the given element.
//
//-----------------------------------------------------------------------------

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

CTreeNode *
CMarkup::SearchBranchForScopeInStory (
    CTreeNode * pNodeStartHere,
    CElement *  pElementFindMyScope )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode;

    for ( pNode = pNodeStartHere ; pNode ; pNode = pNode->Parent() )
    {
        CElement * pElementScope = pNode->Element();

        if (pElementFindMyScope == pElementScope)
            return pNode;
    }

    return NULL;
}

#pragma optimize("", on)

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForNode
//
//  Synopsis:   Searches the given branch for a given element (non scope),
//              starting from a given element.
//
//     Notes:   Stops searching after the CTxtSite goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForNode (
    CTreeNode * pNodeStartHere, CTreeNode * pNodeFindMe )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode;

    for ( pNode = pNodeStartHere ; ; pNode = pNode->Parent() )
    {
        if (!pNode)
            return NULL;
        
        if (pNodeFindMe == pNode)
            return pNode;

        if (pNode->HasFlowLayout())
            return NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForNodeInStory
//
//  Synopsis:   Searches the given branch for a given element (non-scope),
//              starting from a given element.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForNodeInStory (
    CTreeNode * pNodeStartHere, CTreeNode * pNodeFindMe )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode;

    for ( pNode = pNodeStartHere ; pNode ; pNode = pNode->Parent() )
    {
        if (pNodeFindMe == pNode)
            return pNode;
    }
    
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForChildOfScope
//
//  Synopsis:   Searches the given branch for the scope of a given element,
//              starting from a given node.
//
//   Returns:   Returns the child node of the node found.
//
//     Notes:   Stops searching after the CTxtSite goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForChildOfScope (
    CTreeNode * pNodeStartHere, CElement * pElementFindChildOfMyScope )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode, * pNodeChild = NULL;

    for ( pNode = pNodeStartHere ; ; pNode = pNode->Parent() )
    {
        if (!pNode)
            return NULL;
        
        CElement * pElementScope = pNode->Element();

        if (pElementFindChildOfMyScope == pElementScope)
            return pNodeChild;

        if (pNode->HasFlowLayout())
            return NULL;

        pNodeChild = pNode;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForChildOfScopeInStory
//
//  Synopsis:   Searches the given branch for the scope of a given element,
//              starting from a given node.
//
//   Returns:   Returns the child node of the node found.
//
//     Notes:   Stops searching after the CTxtEdit goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForChildOfScopeInStory (
    CTreeNode * pNodeStartHere, CElement * pElementFindChildOfMyScope )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode, * pNodeChild = NULL;

    for ( pNode = pNodeStartHere ; pNode ; pNode = pNode->Parent() )
    {
        CElement * pElementScope = pNode->Element();

        if (pElementFindChildOfMyScope == pElementScope)
            return pNodeChild;

        pNodeChild = pNode;
    }

    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForBlockElement
//
//  Synopsis:   Searches the given branch for the first block element,
//              starting from a given element.
//
//     Notes:   Stops searching after the CTxtSite goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForBlockElement (
    CTreeNode * pNodeStartHere,
    CFlowLayout * pFLContext )
{
    CTreeNode * pNode;

    Assert( pNodeStartHere );

    if (!pFLContext && GetElementClient())
        pFLContext = GetElementClient()->GetFlowLayout();

    if (!pFLContext)
        return NULL;

    for ( pNode = pNodeStartHere ; ; pNode = pNode->Parent() )
    {
        if (!pNode)
            return NULL;
        
        CElement * pElementScope = pNode->Element();

        if (pFLContext->IsElementBlockInContext( pElementScope ))
            return pNode;

        if (pElementScope == pFLContext->ElementContent())
            return NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForNonBlockElement
//
//  Synopsis:   Searches the given branch for the first non block element,
//              starting from a given element.
//
//     Notes:   Stops searching after the CTxtSite goes out of scope.
//
//-----------------------------------------------------------------------------

CTreeNode *
CMarkup::SearchBranchForNonBlockElement (
    CTreeNode * pNodeStartHere,
    CFlowLayout * pFLContext )
{
    Assert( pNodeStartHere );

    CTreeNode * pNode;

    if (!pFLContext)
        pFLContext = GetElementClient()->GetFlowLayout();

    for ( pNode = pNodeStartHere ; ; pNode = pNode->Parent() )
    {
        if (!pNode)
            return NULL;
        
        CElement * pElementScope = pNode->Element();

        if (!pFLContext->IsElementBlockInContext( pElementScope ))
        {
            return pNode;
        }

        if (pElementScope == pFLContext->ElementOwner())
            return NULL;
    }
}

#ifdef WIN16
#pragma code_seg ("DOC_2_TEXT")
#endif

//+----------------------------------------------------------------------------
//
//    Member:   CreateInclusion
//
//  Synopsis:   Splits the branch up to but not including pNodeStop.
//              The inclusion will occur at ptpgLocation.  ptpgInclusion will
//              be set to the middle of the inclusion.
//
// Arguments:
//      pNodeStop       -   the node to split up to but not including
//      ptpgLocation    -   create the split here. IN ONLY
//      ptpgInclusion   -   return the CTreePos just before the split. OUT ONLY
//      pcchNeeded      -   return the number of WCH_NODE characters needed.
//                          If NULL, then insert them here
//      pNodeAboveLocation - the node directly above ptpgLocation, if NULL,
//                           it will be computed
//      fFullReparent   -   if TRUE, everything will be reparented correctly
//                          if FALSE, only the new nodes will
//      ppNodeLastAdded -   return the top of the new element chain
//
//-----------------------------------------------------------------------------

HRESULT
CMarkup::CreateInclusion(
    CTreeNode *     pNodeStop,
    CTreePosGap *   ptpgLocation,
    CTreePosGap *   ptpgInclusion,
    long *          pcchNeeded,
    CTreeNode *     pNodeAboveLocation /*= NULL*/,
    BOOL            fFullReparent /*= TRUE*/,
    CTreeNode **    ppNodeLastAdded /*= NULL*/)
{
    HRESULT     hr = S_OK;
    CTreePosGap tpgInsert ( TPG_LEFT );
    CTreeNode * pNodeCurr, * pNodeNew = NULL;
    CTreeNode * pNodeLastNew = NULL;
    long        cpInsertNodeChars = -1;
    BOOL        fInsertChars = ! pcchNeeded;
    long        cchNeededAlternate;

    //
    // The passed in gap must be specified and positioned at a valid point
    // in this tree.
    //

    Assert( ptpgLocation );
    Assert( ptpgLocation->IsValid() );
    Assert( ptpgLocation->GetAttachedMarkup() == this );

    //
    // The node up to which we should split must be specified and in this tree.
    // Note that this node may not actually curently influence the gap in the
    // tree specified by ptpgLocation.  This is so because the tree may be in
    // an unstable state.
    //
    
    Assert( pNodeStop );
    Assert( pNodeStop->GetMarkup() == this );

    //
    // If caller wants us to put in the node characters, then point the
    // char counter at a local variable.
    //

    if (!pcchNeeded)
        pcchNeeded = & cchNeededAlternate;

    //
    // Initially, there are no node chars needed
    //

    *pcchNeeded = 0;
    
    //
    // cpInsertNodeChars is where we may need to insert node chars
    //

    if (fInsertChars)
        cpInsertNodeChars = ptpgLocation->GetCp();

    //
    // The pNodeAboveLocation, if specified, should be assumed to be the first node
    // which influences the gap specified by ptpgLocation.  Because the tree may be
    // in an unstable state, this node may not be visible to ptpgLocation.
    //
    // In any case, here we compute the node where we start the creation of the
    // inclusion.
    //
    
    pNodeCurr = pNodeAboveLocation ? pNodeAboveLocation : ptpgLocation->Branch();

    Assert( pNodeCurr );

    //
    // Now that we have a place to start with in the tree, make sure the stop node
    // is somewhere along the parent chain from the start node (now pNodeCurr).
    //
    
    Assert( pNodeCurr->SearchBranchToRootForScope( pNodeStop->Element() ) == pNodeStop );

    //
    // Set up the spot to insert
    //
    
    Verify( ! tpgInsert.MoveTo( ptpgLocation ) );

    while (pNodeCurr != pNodeStop)
    {
        CTreePos * ptpNew;
        
        //
        // Create the new node (proxy to the current element)
        //

        pNodeNew = new CTreeNode( pNodeCurr->Parent(), pNodeCurr->Element() );
        
        if (!pNodeNew)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        //
        // Insert the end pos of the new node to be just before the end pos of
        // the node we cloned it from.  It inherits edge scopeness from the end
        // of the current node.
        //

        ptpNew = pNodeNew->InitEndPos( pNodeCurr->GetEndPos()->IsEdgeScope() );
        
        Verify( ! Insert( ptpNew, pNodeCurr->GetEndPos(), TRUE ) );

        //
        // Based on the attach preference of the tree gap which originally
        // specified where to create the inclusion, push that gap around.
        //
        
        if (ptpgLocation->AttachedTreePos() == pNodeCurr->GetEndPos())
        {
            Assert( ptpgLocation->AttachDirection() == TPG_RIGHT );
            
            ptpgLocation->MoveTo( ptpNew, TPG_LEFT );
        }

        //
        // Insert the begin pos to the right of the point of the inclusion.  It is
        // automatically not an edge.
        //

        ptpNew = pNodeNew->InitBeginPos( FALSE );

        Verify( ! Insert( ptpNew, tpgInsert.AttachedTreePos(), FALSE ) );

        //
        // Mark this node as being in the markup
        //
        pNodeNew->PrivateEnterTree();

        //
        // Move the end pos of the current node to be to the left of the
        // location of the inclusion.
        //

        Verify( ! Move( pNodeCurr->GetEndPos(), tpgInsert.AttachedTreePos(), FALSE ) );

        //
        // The end pos of the current node is no longer an edge
        //

        pNodeCurr->GetEndPos()->SetScopeFlags( FALSE );

        //
        // Move over the end of end of the current node to get back into the
        // location of the inclusion.
        //

        Verify( ! tpgInsert.Move( TPG_RIGHT ) );

        Assert( tpgInsert.AdjacentTreePos( TPG_LEFT ) == pNodeCurr->GetEndPos() );

        //
        // Reparent the direct children in the range affected
        //
        
        if (fFullReparent)
        {
            hr = THR( ReparentDirectChildren( pNodeNew ) );
            
            if (hr)
                goto Cleanup;
        }
        else if (pNodeLastNew)
        {
            pNodeLastNew->SetParent( pNodeNew );
        }

        //
        // Record the number of WCH_NODE characters needed at
        // the inclusion point
        //
        
        *pcchNeeded += 2;

        //
        // Finally, set up for the next time around
        //
        
        pNodeLastNew = pNodeNew;
        pNodeCurr = pNodeCurr->Parent();
    }


    if (ppNodeLastAdded)
        *ppNodeLastAdded = pNodeLastNew;

    if (ptpgInclusion)
        Verify( ! ptpgInclusion->MoveTo( & tpgInsert ) );

    //
    // If requested, insert need characters here
    //

    if (fInsertChars && *pcchNeeded)
    {
        CNotification nf;

        Assert( *pcchNeeded % 2 == 0 );

        nf.CharsAdded( cpInsertNodeChars, *pcchNeeded, tpgInsert.Branch() );

        CTxtPtr( this, cpInsertNodeChars ).InsertRepeatingChar( *pcchNeeded, WCH_NODE );
        
        Notify( & nf );
    }

Cleanup:
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//    Member:   CloseInclusion
//
//  Synopsis:   Close a possible empty inclusion.  It is possible that there
//              will be no inclusion at ptpgMiddle, in which case
//              CloseInclusion does nothing.  Otherwise, it removes as many
//              redundant nodes as possible.
//
// Arguments:
//      ptpgMiddle -   The middle of the inclusion to close.  Will be
//                     set to the gap where the inclusion was on exit
//
//      pcchRemove -   The number of WCH_NODE characters which need to be
//                     removed because of the removal of node pos's.  If NULL
//                     chars are removed automatically.
//
//-----------------------------------------------------------------------------

HRESULT
CMarkup::CloseInclusion (
    CTreePosGap * ptpgMiddle,
    long *        pcchRemove )
{
    HRESULT     hr = S_OK;
    BOOL        fRemoveChars = ! pcchRemove;
    long        cchRemoveAlternate;
    CTreePosGap tpgMiddle ( TPG_LEFT );

    //
    // Note: tpgMiddle is bound to the pos which is to the left of the
    // gap because it will move to the left as we delete nodes.  This
    // way, it stays out of the way (to the left) of the removals.
    //

    //
    // If caller wants us to remove the node characters, then point the
    // char counter at a local variable.
    //

    if (!pcchRemove)
        pcchRemove = & cchRemoveAlternate;

    //
    // No chars to delete ... yet
    //

    *pcchRemove = 0;

    //
    // The middle gap argument must be specified and positioned in this
    // tree.
    //

    Assert( ptpgMiddle );
    Assert( ptpgMiddle->IsPositioned() );
    Assert( ptpgMiddle->GetAttachedMarkup() == this );

    //
    // Copy the incomming argument to our local gap, nd unposition the argument.
    // We'll reposition it later, after we have closed the inclusion.
    //

    Verify( ! tpgMiddle.MoveTo( ptpgMiddle ) );

    ptpgMiddle->UnPosition();

    for ( ; ; )
    {
        CTreePos * ptpRightMiddle, * ptpLeftMiddle;
        CTreeNode * pNodeBefore, * pNodeAfter;

        //
        // Get the pos to the left of the gap which travels accross
        // the splays.
        //

        ptpLeftMiddle = tpgMiddle.AttachedTreePos();

        //
        // We know we're not in an inclusion anymore when the pos to the
        // left is not a ending edge.
        //
        // Note, we don't have to check to tree pointers or text because
        // they must never be seen in inclusions.
        //

        if (!ptpLeftMiddle->IsEndNode() || ptpLeftMiddle->IsEdgeScope())
            break;

        //
        // Get the tree pos to the right of the one which is to the
        // left of the gap.  This will, then, be to the right of the gap.
        //
        
        ptpRightMiddle = ptpLeftMiddle->NextTreePos();

        //
        // Check the right edge for non matching ptp's.  We can
        // have a non edge end node pos on the left but not a non edge begin
        // node ont he right during certain circumstances in splice.
        //

        if (!ptpRightMiddle->IsBeginNode() || ptpRightMiddle->IsEdgeScope())
            break;

        //
        // Move the gap to the left (<--) by one (not right).
        //
        
        Verify( ! tpgMiddle.MoveLeft() );

        //
        // If we've gotten this far, then there damn well better be
        // an inclusion here.
        //
        // Thus, we must have a non edge beginning to our right.
        //

        Assert(
              ptpRightMiddle->IsBeginNode() &&
            ! ptpRightMiddle->IsEdgeScope() );

        //
        // Get the nodes associated with either side of the gap in question.
        //

        pNodeBefore = ptpLeftMiddle->Branch();
        pNodeAfter  = ptpRightMiddle->Branch();

        //
        // To be in an inclusion, the two adjacent nodes must refer to
        // the same element.  
        //

        Assert( pNodeBefore->Element() == pNodeAfter->Element() );
        
        //
        // Make sure the inclusion does not refer to the root!
        //
        
        Assert( !pNodeBefore->Element()->IsRoot() );


        //
        // Move the end pos of the node left of the gap to be just before
        // the end pos of the node right of the gap.
        //
        // This way, the node left of the gap will subsume "ownership"
        // of all the stuff which is under the node right of the gap.
        //

        Verify( ! Move( pNodeBefore->GetEndPos(), pNodeAfter->GetEndPos(), TRUE ) );

        //
        // Now that the node right of the gap is going away, the end pos
        // of the node left of the gap needs to reflect the same edge status
        // of the end pos going away.  Because there may be yet another
        // proxy for this element to the right, we can't assume that it
        // is an edge.
        //

        pNodeBefore->GetEndPos()->SetScopeFlags(
            pNodeAfter->GetEndPos()->IsEdgeScope() );

        //
        // Here we remove the node right of the gap (and its pos's)
        //

        Verify( ! Remove( pNodeAfter->GetBeginPos() ) );

        Verify( ! Remove( pNodeAfter->GetEndPos() ) );

        //
        // Two pos's gone .. two chars must go
        //
        
        *pcchRemove += 2;

        //
        // Reparent the children that were pointing to the node going away.
        //
        // Now that the node to the right of the gap is gone, we must
        // make sure that all the nodes which pointed to the node which
        // we just removed now point to the node which remains.
        //
        // We start the reparent starting at the current location of the gap
        // and extending until the remaining node goes out of scope.
        //
        
        hr = THR( ReparentDirectChildren( pNodeBefore, & tpgMiddle ) );
        
        if (hr)
            goto Cleanup;

        //
        // Now, kill the node we just removed.  No one in this tree better be
        // pointing to it after the reparent operation.
        //

        // pNodeAfter is not usable after this
        pNodeAfter->PrivateExitTree();
        pNodeAfter = NULL;
    }

    //
    // Put the incomming argument back into the tree
    //

    Verify( ! ptpgMiddle->MoveTo( & tpgMiddle ) );

    //
    // If requested, remove WCH_NODE chars here
    //

    if (fRemoveChars && *pcchRemove)
    {
        CNotification nf;
        long cp = ptpgMiddle->GetCp();

        Assert( *pcchRemove % 2 == 0 );
        
        nf.CharsDeleted( cp, *pcchRemove, tpgMiddle.Branch() );

#if DBG == 1
        {
            CTxtPtr tp ( this, cp );
            
            for ( int i = 0 ; i < *pcchRemove; i++, tp.AdvanceCp( 1 ) )
                Assert( tp.GetChar() == WCH_NODE );
        }
#endif

        CTxtPtr( this, cp ).DeleteRange( *pcchRemove );
        
        Notify( & nf );
    }

Cleanup:
    
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//    Member:   ReparentDirectChildren
//
//  Synopsis:   This routine assumes that the begin and end node pos's for
//              pNodeParentNew are already in place.  What it does is
//              make sure all direct children (according to the runs)
//              point to pNodeParentNew.  In other words, this routine makes
//              sure the parent info implied by the runs is reflected by
//              the tree itself.
//
//              If ptpgStart and ptpgEnd are passed in, they must be under the
//              direct scope of the parent.  This allows the routine to skip
//              over all the runs of direct children.
//
// Arguments:
//      pNodeParentNew  -   the new parent
//      ptpgStart       -   where to start parenting, IN only
//      ptpgEnd         -   where to stop parenting, IN only
//
//-----------------------------------------------------------------------------

HRESULT
CMarkup::ReparentDirectChildren (
    CTreeNode *   pNodeParentNew,
    CTreePosGap * ptpgStart,
    CTreePosGap * ptpgEnd )
{
    CTreePos *  ptpCurr, * ptpEnd;

    //
    // If the start/end is specified, make sure they're sane
    //

    Assert( ! ptpgStart || ptpgStart->GetAttachedMarkup() == this );
    Assert( ! ptpgEnd   || ptpgEnd->GetAttachedMarkup()   == this );

    //
    // If the position where we should start and end the reparent operation
    // are specified, then go there, otherwise start/end at where the new
    // parent starts/ends.
    //

    ptpCurr =
        ptpgStart
            ? ptpgStart->AdjacentTreePos( TPG_RIGHT )
            : pNodeParentNew->GetBeginPos()->NextTreePos();

    ptpEnd =
        ptpgEnd
            ? ptpgEnd->AdjacentTreePos( TPG_RIGHT )
            : pNodeParentNew->GetEndPos();

    //
    // Make sure the start is to the left of the end
    //
    
    Assert( ptpCurr->InternalCompare( ptpEnd ) <= 0 );

    //
    // Loop and reparent.  Get to just direct children by skipping
    // over direct children.
    //
    
    while ( ptpCurr != ptpEnd )
    {
        switch ( ptpCurr->Type() )
        {
        case CTreePos::NodeEnd :

            //
            // We will (most) never find an end pos because we'll skip over
            // them when we encounter the begin pos for that node.
            //

            AssertSz( 0, "Found an end pos during reparent children" );

            break;

        case CTreePos::NodeBeg :
                
            ptpCurr->Branch()->SetParent( pNodeParentNew );

            //
            // Skip over everything under this node
            //
            ptpCurr = ptpCurr->Branch()->GetEndPos();

            break;
            
        default:
            
            Assert( ! ptpCurr->IsUninit() );
            
            break;
        }

        ptpCurr = ptpCurr->NextTreePos();
    }

    RRETURN( S_OK );
}

//+----------------------------------------------------------------------------
//
//    Member:   RemoveElement
//
//  Synopsis:   Removes the influence of an element
//
//-----------------------------------------------------------------------------

static void
RemoveNodeChars ( CMarkup * pMarkup, long cp, long cch, CTreeNode * pNode )
{
    CNotification nf;
    
    Assert( cp >= 0 && cp < pMarkup->GetTextLength() );
    Assert( cch > 0 && cp + cch <= pMarkup->GetTextLength() );

    nf.CharsDeleted( cp, cch, pNode );
    
#if DBG == 1
    {
        CTxtPtr tp2 ( pMarkup, cp );

        for ( int i = 0 ; i < cch; i++, tp2.AdvanceCp( 1 ) )
            Assert( tp2.GetChar() == WCH_NODE );
    }
#endif
    
    CTxtPtr( pMarkup, cp ).DeleteRange( cch );

    pMarkup->Notify( & nf );
}

HRESULT
CMarkup::RemoveElementInternal ( 
    CElement * pElementRemove,
    DWORD      dwFlags )
{
    HRESULT            hr = S_OK;
    BOOL               fDOMOperation = dwFlags & MUS_DOMOPERATION;
    CTreePosGap        tpgElementBegin ( TPG_LEFT );
    CTreePosGap        tpgEnd ( TPG_RIGHT );
    CTreeNode *        pNodeCurr, * pNodeNext;
    long               siStart;
    BOOL               fDelayRelease = FALSE, fExitTreeSc = FALSE;
    CMarkup::CLock     MarkupLock( this );
    CRemoveElementUndo Undo( this, pElementRemove, dwFlags );

    // BUGBUG (EricVas): Specialcase the removal of an empty element so that
    //                   multiple notifications can be amortized

    //
    // The element to be removed must be specified and in this tree.  Also,
    // We better not be trying to remove the tree element itself!
    //
    
    Assert( pElementRemove );
    Assert( pElementRemove->GetMarkup() );
    Assert( pElementRemove->GetMarkup() == this );
    Assert( !pElementRemove->IsRoot() );

    //
    // Notify element it is about to exit the tree
    //
    
    {
        CNotification nf;

        Assert( !pElementRemove->_fExittreePending );
        pElementRemove->_fExittreePending = TRUE;

        nf.ElementExittree1( pElementRemove );

        Assert( nf.IsSecondChanceAvailable() );

        // If we are in the undo queue, we will have _ulRefs>1 by now.
        if( pElementRemove->GetObjectRefs() == 1 )
        {
            nf.SetData( EXITTREE_PASSIVATEPENDING );
            Assert( !pElementRemove->_fPassivatePending );
            WHEN_DBG( pElementRemove->_fPassivatePending = TRUE );
        }

        pElementRemove->Notify( & nf );

        if (nf.IsSecondChanceRequested())
        {
            fDelayRelease = TRUE;
            fExitTreeSc = TRUE;
            WHEN_DBG( pElementRemove->_fDelayRelease = TRUE );
        }
        else if (nf.DataAsDWORD() & EXITTREE_DELAYRELEASENEEDED)
        {
            fDelayRelease = TRUE;
            WHEN_DBG( pElementRemove->_fDelayRelease = TRUE );
        }

        pElementRemove->_fExittreePending = FALSE;
    }

    hr = THR( EmbedPointers() );

    if (hr)
        goto Cleanup;

    //
    // Remove tree pointers with cling around the edges of the element being removed
    //
    {
        CTreePosGap tpgCling;
        CTreePos * ptpBegin, * ptpEnd;

        pElementRemove->GetTreeExtent( &ptpBegin, &ptpEnd );

        Assert( ptpBegin && ptpEnd );

        // Around the beginning
        Verify( ! tpgCling.MoveTo( ptpBegin, TPG_LEFT ) );
        tpgCling.PartitionPointers(this, fDOMOperation);
        tpgCling.CleanCling( this, TPG_RIGHT, fDOMOperation );

        Verify( ! tpgCling.MoveTo( ptpBegin, TPG_RIGHT ) );
        tpgCling.PartitionPointers(this, fDOMOperation);
        tpgCling.CleanCling( this, TPG_LEFT, fDOMOperation );

        // Around the end
        Verify( ! tpgCling.MoveTo( ptpEnd, TPG_LEFT ) );
        Verify( ! tpgCling.MoveLeft( TPG_VALIDGAP | TPG_OKNOTTOMOVE ) );
        tpgCling.PartitionPointers(this, fDOMOperation);
        tpgCling.CleanCling( this, TPG_RIGHT, fDOMOperation );

        Verify( ! tpgCling.MoveTo( ptpEnd, TPG_RIGHT ) );
        Verify( ! tpgCling.MoveRight( TPG_VALIDGAP | TPG_OKNOTTOMOVE ) );
        tpgCling.PartitionPointers(this, fDOMOperation);
        tpgCling.CleanCling( this, TPG_LEFT, fDOMOperation );
    }

    //
    // Record where the element begins, making sure the gap used to record this
    // points to the tree pos to the left so that stuff removed from the tree
    // does not include that.
    //

    tpgElementBegin.MoveTo( pElementRemove->GetFirstBranch()->GetBeginPos(), TPG_LEFT );

    //
    // Remember the source index of the element that we are removing
    //
    
    siStart = pElementRemove->GetFirstBranch()->GetBeginPos()->SourceIndex(); 
    
    //
    // Run through the context chain removing contexts in their entirety
    //
    
    for ( pNodeCurr = pElementRemove->GetFirstBranch() ;
          pNodeCurr ;
          pNodeCurr = pNodeNext )
    {
        CTreeNode * pNodeParent;
        CTreePosGap tpgNodeBegin ( TPG_LEFT );
        BOOL        fBeginEdge;

        //
        // Before we nuke this node, record its parent and next context
        //

        pNodeParent = pNodeCurr->Parent();
        pNodeNext = pNodeCurr->NextBranch();

        //
        // Record the gaps where the node is right now
        //

        tpgNodeBegin.MoveTo( pNodeCurr->GetBeginPos(), TPG_RIGHT );
        tpgEnd.MoveTo( pNodeCurr->GetEndPos(), TPG_LEFT );

        //
        // Make all the immediate children which were beneath this node point
        // to its parent.
        //

        hr = THR(
            ReparentDirectChildren(
                pNodeParent, & tpgNodeBegin, & tpgEnd ) );

        if (hr)
            goto Cleanup;

        //
        // Swing the pointers to the outside
        //

        tpgNodeBegin.MoveTo( pNodeCurr->GetBeginPos(), TPG_LEFT );
        tpgEnd.MoveTo( pNodeCurr->GetEndPos(), TPG_RIGHT );

        //
        // Remove the begin pos
        //

        fBeginEdge = pNodeCurr->GetBeginPos()->IsEdgeScope();

        hr = THR( Remove( pNodeCurr->GetBeginPos() ) );

        if (hr)
            goto Cleanup;

        //
        // Delete the character and send the notification
        //

        {
            CTreeNode * pNodeNotifyBegin = pNodeParent;
            long cpNotifyBegin = tpgNodeBegin.GetCp();

            // If we removed something from inside of an inclusion, find the bottom
            // of that inclusion to send the notification to
            if( !fBeginEdge )
            {
                CTreePos * ptpBeginTest = tpgNodeBegin.AdjacentTreePos( TPG_RIGHT );

                while( ptpBeginTest->IsBeginNode() && !ptpBeginTest->IsEdgeScope() )
                {
                    pNodeNotifyBegin = ptpBeginTest->Branch();
                    ptpBeginTest = ptpBeginTest->NextTreePos();
                }
            }

            RemoveNodeChars(this, cpNotifyBegin, 1, pNodeNotifyBegin );
        }

//...        hr = THR( MergeRightText( tpgNodeBegin.AttachedTreePos() );


        //
        // Remove the end pos
        //

        {
            CTreeNode * pNodeNotifyEnd;
            long cchNotifyEnd;
            long cpNotifyEnd;

            hr = THR( Remove( pNodeCurr->GetEndPos() ) );

            if (hr)
                goto Cleanup;
        
            pNodeNotifyEnd = pNodeParent;
            cpNotifyEnd = tpgEnd.GetCp();
            cchNotifyEnd = 1;

            //
            // If this is the last node in the content chain, there must be
            // an inclusion here.  Remove it.
            //

            if (!pNodeNext)
            {
                long cchRemove;

                hr = THR( CloseInclusion( & tpgEnd, &cchRemove ) );
                if (hr)
                    goto Cleanup;

                cpNotifyEnd -= cchRemove/2;
                cchNotifyEnd += cchRemove;

                pNodeNotifyEnd = tpgEnd.Branch();
            }

            //
            // Delete the chracters and send the  notification
            //

            // If we removed something from inside of an inclusion, find the bottom
            // of that inclusion to send the notification to
            if( pNodeNext )
            {
                CTreePos * ptpEndTest = tpgEnd.AdjacentTreePos( TPG_LEFT );

                while( ptpEndTest->IsEndNode() && !ptpEndTest->IsEdgeScope() )
                {
                    pNodeNotifyEnd = ptpEndTest->Branch();
                    ptpEndTest = ptpEndTest->PreviousTreePos();
                }
            }

            RemoveNodeChars(this, cpNotifyEnd, cchNotifyEnd, pNodeNotifyEnd );

        }

        //
        // Kill the node
        //

        pNodeCurr->PrivateExitTree();
        pNodeCurr = NULL;
    }

    //
    // Set the Undo data
    //

    Undo.SetData( tpgElementBegin.GetCp(), tpgEnd.GetCp() );

    //
    // Release element from the tree
    //

    {
        WHEN_DBG(BOOL fPassivatePending = pElementRemove->_fPassivatePending);

        if (fDelayRelease)
        {
            pElementRemove->AddRef();
            WHEN_DBG( pElementRemove->_fPassivatePending = FALSE );
        }

        pElementRemove->__pNodeFirstBranch = NULL;

        pElementRemove->DelMarkupPtr();
        pElementRemove->PrivateExitTree( this );

#if DBG==1
        if (fDelayRelease)
            pElementRemove->_fPassivatePending = fPassivatePending;
#endif
    }

    //
    // Clear caches, etc
    //
    
    UpdateMarkupTreeVersion();

    if (tpgElementBegin != tpgEnd)
    {
        CTreePos * ptpLeft = tpgElementBegin.AdjacentTreePos( TPG_RIGHT );
        CTreePos * ptpRight = tpgEnd.AdjacentTreePos( TPG_LEFT );

        hr = THR( RangeAffected( ptpLeft, ptpRight ) );
    }

    tpgEnd.UnPosition();
    tpgElementBegin.UnPosition();

    //
    //  Send the ElementsDeleted notification
    //

    {
        CNotification nf;

        nf.ElementsDeleted( siStart, 1 );

        Notify( nf );
    }

    Assert( IsNodeValid() );

    Undo.CreateAndSubmit();

    //
    // Send the ExitTreeSc notification and do the delay release
    //

    if( fExitTreeSc )
    {
        Assert( fDelayRelease );
        CNotification nf;

        nf.ElementExittree2( pElementRemove );
        pElementRemove->Notify( &nf );
    }

    if( fDelayRelease )
    {
        // Release the element
        Assert( pElementRemove->_fDelayRelease );
        WHEN_DBG( pElementRemove->_fDelayRelease = FALSE );
        Assert( !pElementRemove->_fPassivatePending || pElementRemove->GetObjectRefs() == 1 );
        pElementRemove->Release();
    }

Cleanup:

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//    Member:   InsertElement
//
//  Synopsis:   Inserts an element into the tree.  The element must not already
//              be in the tree when this is called.
//
// Arguments:
//      pElementInsertThis  -   The element to insert
//      ptpgBegin           -   Element will begin here.  Must be a valid gap.
//                              IN only
//      ptpgEnd             -   Element will end here. Must be a valid gap.
//                              IN only
//-----------------------------------------------------------------------------

static void
InsertNodeChars ( CMarkup * pMarkup, long cp, long cch, CTreeNode * pNode )
{
    CNotification nf;
    
    Assert( cp >= 0 && cp < pMarkup->GetTextLength() );

    nf.CharsAdded( cp, cch, pNode );
    
    CTxtPtr( pMarkup, cp ).InsertRepeatingChar( cch, WCH_NODE );

    pMarkup->Notify( & nf );
}

HRESULT
CMarkup::InsertElementInternal (
    CElement *    pElementInsertThis,
    CTreePosGap * ptpgBegin,
    CTreePosGap * ptpgEnd,
    DWORD         dwFlags )
{
    HRESULT      hr = S_OK;
    CDoc *       pDoc = Doc();
    BOOL         fDOMOperation = dwFlags & MUS_DOMOPERATION;
    CTreePosGap  tpgCurr( TPG_LEFT );
    CTreeNode *  pNodeNew = NULL;
    CTreeNode *  pNodeLast = NULL;
    CTreeNode *  pNodeParent = NULL;
    CTreeNode *  pNodeAboveEnd;
    BOOL         fLast = FALSE;
    CTreePosGap  tpgBegin( TPG_LEFT ), tpgEnd( TPG_RIGHT );
    CInsertElementUndo Undo( this, dwFlags );

    Assert( ! HasUnembeddedPointers() );

    EnsureTotalOrder( ptpgBegin, ptpgEnd );

    Undo.SetData( pElementInsertThis );

    // BUGBUG (EricVas) : Make insertion of unscoped element amortize notifications

    //
    // Element to insert must be specified and not in any tree
    //
    
    Assert( pElementInsertThis && !pElementInsertThis->GetFirstBranch() );

    //
    // The gaps specifying where to put the element must be specified and
    // in valid locations in the splay tree.  They must also both be in
    // the same tree (this tree).
    //
    
    Assert( ptpgBegin && ptpgBegin->IsValid() );
    Assert( ptpgEnd && ptpgEnd->IsValid() );
    Assert( ptpgBegin->AttachedTreePos()->IsInMarkup( this ) );
    Assert( ptpgEnd->AttachedTreePos()->IsInMarkup( this ) );

    //
    // Push any tree pointers at the insertion points in the correct directions
    //

    ptpgBegin->PartitionPointers(this, fDOMOperation);
    ptpgEnd->PartitionPointers(this, fDOMOperation);

    //
    // Make sure to split any text IDs
    //
    
    if (pDoc->_lLastTextID)
    {
        SplitTextID(
            ptpgBegin->AdjacentTreePos( TPG_LEFT ),
            ptpgBegin->AdjacentTreePos( TPG_RIGHT) );
        
        if (*ptpgBegin != *ptpgEnd)
        {
            SplitTextID(
                ptpgEnd->AdjacentTreePos( TPG_LEFT ),
                ptpgEnd->AdjacentTreePos( TPG_RIGHT) );
        }
    }

    //
    // Copy the endpoints to local gaps.  This way we don't muck with the
    // arguments.
    //

    Verify( ! tpgBegin.MoveTo( ptpgBegin ) );
    Verify( ! tpgEnd.MoveTo( ptpgEnd ) );

    //
    // The var tpgCurr will walk through the insertionm left to right
    //

    tpgCurr.MoveTo( ptpgBegin );

    pNodeAboveEnd = ptpgEnd->Branch();

    //
    // Get the parent for the new node
    //
    
    pNodeParent = tpgCurr.Branch();

    while (!fLast)
    {
        //
        // Check to see if this is the last node we will have to insert
        //
        
        if (SearchBranchForNodeInStory( pNodeAboveEnd, pNodeParent ))
            fLast = TRUE;

        //
        // Create the new node and hook it into the context list
        //

        pNodeNew = new CTreeNode( pNodeParent, pElementInsertThis );
        
        if ( !pNodeNew )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }


        if (! pNodeLast)
        {
            pElementInsertThis->__pNodeFirstBranch = pNodeNew;

            pElementInsertThis->SetMarkupPtr( this );
            pElementInsertThis->PrivateEnterTree();
        }

        //
        // Insert the start node
        //

        {
            CTreePos * ptpNew;
            CTreeNode *pNodeNotify = pNodeParent;

            Assert( pNodeNew->GetBeginPos()->IsUninit() );

            ptpNew = pNodeNew->InitBeginPos( !pNodeLast );

            Verify( ! Insert( ptpNew, & tpgCurr ) );

            // move tpgCurr to the right looking for the bottom of 
            // the inclusion, so that we know which node to send
            // the notification to
            if( pNodeLast )
            {
                tpgCurr.MoveRight();
                Assert( tpgCurr.AttachedTreePos() == ptpNew );

                tpgCurr.MoveRight();

                while(  tpgCurr.AttachedTreePos()->IsBeginNode() 
                    &&  !tpgCurr.AttachedTreePos()->IsEdgeScope() )
                {
                    pNodeNotify = tpgCurr.AttachedTreePos()->Branch();
                    tpgCurr.MoveRight();
                }
            }

            // The element is now in the tree so add a ref for the
            // tree
            pNodeNew->PrivateEnterTree();

            InsertNodeChars( this, ptpNew->GetCp(), 1, pNodeNotify );
        }

        //
        // Insert the end node
        //
        {
            CTreePos *  ptpNew;
            long        cpInsert;
            long        cchInsert = 0;
            CTreeNode * pNodeNotify = NULL;

            //
            // Create or set the insertion point
            //
        
            if (fLast)
            {
                //
                // Split the branch to create a spot for our end node pos
                //
                pNodeNotify = pNodeAboveEnd;

                cpInsert = tpgEnd.GetCp();

                hr = THR( CreateInclusion(  pNodeParent, 
                                            & tpgEnd, 
                                            & tpgCurr, 
                                            & cchInsert,
                                            pNodeAboveEnd ) );

                if (hr)
                    goto Cleanup;
            }
            else
            {
                CTreePosGap tpgNotify( TPG_RIGHT );

                Verify( !tpgCurr.MoveTo( pNodeParent->GetEndPos(), TPG_LEFT ) );

                cpInsert = tpgCurr.GetCp();

                // Move tpgNotify to the left to find the lowest 
                // node in the inclusion.
                Verify( !tpgNotify.MoveTo( &tpgCurr ) );
                Assert( tpgNotify.AttachedTreePos() == pNodeParent->GetEndPos() );

                pNodeNotify = pNodeParent;
                tpgNotify.MoveLeft();

                while(  tpgNotify.AttachedTreePos()->IsEndNode()
                    &&  !tpgNotify.AttachedTreePos()->IsEdgeScope() )
                {
                    pNodeNotify = tpgNotify.AttachedTreePos()->Branch();
                    tpgNotify.MoveLeft();
                }
            }


            Assert( pNodeNew->GetEndPos()->IsUninit() );

            ptpNew = pNodeNew->InitEndPos( fLast );

            Verify( ! Insert( ptpNew, & tpgCurr ) );

            cchInsert++;

            Verify( ! tpgCurr.MoveRight() );
            Assert( tpgCurr.AdjacentTreePos( TPG_LEFT ) == ptpNew );

            InsertNodeChars( this, cpInsert, cchInsert, pNodeNotify);
        }

        //
        // Reparent direct children to the new node
        //

        hr = THR( ReparentDirectChildren( pNodeNew ) );
        
        if (hr)
            goto Cleanup;

        if (!fLast)
        {
            // if we are here, it implies that we are
            // inserting the end after the end of pNodeParent
            // This _can't_ happen if pNodeParent is a ped.
            
            Assert( !pNodeParent->IsRoot() );

            //
            // Set up for the next new node
            //
            
            pNodeLast = pNodeNew;

            //
            // This sets our beginning insertion point
            // for the next node.  What we are doing here
            // is essentially threading the new element
            // through any inclusions.
            //

            Verify( ! tpgCurr.MoveRight() );

            Assert( tpgCurr.AttachedTreePos()->IsEndNode() &&
                    tpgCurr.AttachedTreePos()->Branch() == pNodeParent );

            //
            // pNodeParent is done so its parent is the new pNodeParent
            //

            if( tpgCurr.AttachedTreePos()->IsEdgeScope() )
            {
                pNodeParent = pNodeParent->Parent();
            }
            else
            {
                //
                // Find the next branch of pNodeParent
                //

                CElement *pElementParent = pNodeParent->Element();
                CTreeNode *pNodeCurr;

                do
                {
                    Verify( ! tpgCurr.MoveRight() );

                    Assert( tpgCurr.AttachedTreePos()->IsNode() );

                    pNodeCurr = tpgCurr.AttachedTreePos()->Branch();
                }
                while( pNodeCurr->Element() != pElementParent );

                Assert( tpgCurr.AttachedTreePos()->IsBeginNode() );

                pNodeParent = pNodeCurr;
            }

        }
    }

    //
    // Tell the element that it is now in the tree
    //

    {
        CNotification nf;

        nf.ElementEntertree( pElementInsertThis );
        pElementInsertThis->Notify( & nf );
    }

    if (tpgBegin != tpgEnd)
    {
        CTreePos * ptpLeft = tpgBegin.AdjacentTreePos( TPG_RIGHT );
        CTreePos * ptpRight = tpgEnd.AdjacentTreePos( TPG_LEFT );

        hr = THR( RangeAffected( ptpLeft, ptpRight ) );
    }

    // Send the element added notification
    {
        CTreePos *      ptpBegin;
        CNotification   nf;

        pElementInsertThis->GetTreeExtent( &ptpBegin, NULL );
        Assert( ptpBegin );

        nf.ElementsAdded( ptpBegin->SourceIndex(), 1 );
        Notify( nf );
    }

    //
    // Things should be valid here
    //
    
    Assert( IsNodeValid() );

    UpdateMarkupTreeVersion();

    Undo.CreateAndSubmit();

Cleanup:
    
    RRETURN( hr );
}

BOOL
AreDifferentScriptIDs(SCRIPT_ID * psidFirst, SCRIPT_ID sidSecond)
{
    if(     *psidFirst == sidSecond
       ||   sidSecond == sidMerge
       ||   *psidFirst == sidAsciiLatin && sidSecond == sidAsciiSym)
    {
        return FALSE;
    }

    if (    *psidFirst == sidMerge
        ||  *psidFirst == sidAsciiSym && sidSecond == sidAsciiLatin)
    {
        *psidFirst = sidSecond;
        return FALSE;
    }

    return TRUE;
}
//+----------------------------------------------------------------------------
//
//    Member:   InsertText
//
//  Synopsis:   Inserts a chunk of text into the tree before the given TreePos
//              This does not send notifications.
//
//-----------------------------------------------------------------------------

// BUGBUG: we can probably share code here between this function and
// the slow loop of CHtmRootParseCtx::AddText

HRESULT
CMarkup::InsertTextInternal (
    CTreePos *    ptpAfterInsert, 
    const TCHAR * pch,
    long          cch,
    DWORD         dwFlags )
{
    HRESULT         hr = S_OK;
    BOOL            fDOMOperation = dwFlags & MUS_DOMOPERATION;
    CTreePosGap     tpg( TPG_RIGHT );
    CTreePos *      ptpLeft;
    const TCHAR *   pchCurr;
    const TCHAR *   pchStart;
    long            cchLeft;
    SCRIPT_ID       sidLast = sidDefault;
    long            lTextIDCurr = 0;
    CNotification   nf;
    long            cpStart = ptpAfterInsert->GetCp();
    CInsertSpliceUndo Undo( Doc() );
    WHEN_DBG(long   ichAfterInsert = 0);

    Undo.Init( this, dwFlags );

    Assert( ! HasUnembeddedPointers() );

    Assert( ptpAfterInsert );
    Assert( cch >= 0 );

    if (cch <= 0)
        goto Cleanup;

    UpdateMarkupContentsVersion();

    Assert( ptpAfterInsert );
    Assert( cch >= 0 );

    // Partition the pointers to ensure gravity
    
    tpg.MoveTo( ptpAfterInsert, TPG_LEFT );
    tpg.PartitionPointers(this, fDOMOperation);
    ptpAfterInsert = tpg.AdjacentTreePos( TPG_RIGHT );

    Undo.SetData( cpStart, cpStart + cch );

    nf.CharsAdded( cpStart, cch, tpg.Branch() );

    //
    // Collect some information that we'll need later
    //

    // Get info about the previous pos

    ptpLeft = tpg.AdjacentTreePos( TPG_LEFT );
    Assert( ptpLeft );

    if (ptpLeft->IsText())
        sidLast = ptpLeft->Sid();

    pchCurr = pchStart = pch;
    cchLeft = cch;

    // Look around for a TextID to merge with

    {
        CTreePos * ptp;

        for ( ptp = ptpLeft ; ! ptp->IsNode() ;  ptp = ptp->PreviousTreePos() )
        {
            if (ptp->IsText())
            {
                lTextIDCurr = ptp->TextID();
                break;
            }
        }

        if (!lTextIDCurr)
        {
            for ( ptp = ptpAfterInsert ; ! ptp->IsNode() ; ptp = ptp->NextTreePos() )
            {
                if (ptp->IsText())
                {
                    lTextIDCurr = ptp->TextID();
                    break;
                }
            }
        }
    }

    //
    // Break up the text into chunks of compatible SIDs.
    //

    while (cchLeft)
    {
        SCRIPT_ID   sidChunk, sidCurr;
        ULONG       cchChunk;
        TCHAR       chCurr = *pchCurr;

        sidChunk = ScriptIDFromCh(chCurr);

        // Find the end of this chunk of characters with the same sid

        while (cchLeft)
        {
            chCurr = *pchCurr;

            //
            // If we find an illeagal character, then simply compute a new,
            // correct, buffer and call recursively.
            //

            if (chCurr == 0 || !IsValidWideChar( chCurr ))
            {
                TCHAR * pch2;
                long i;

                AssertSz( 0, "Bad char during insert" );

                pch2 = new TCHAR [ cch ];

                if (!pch2)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                for ( i = 0 ; i < cch ; i++ )
                {
                    TCHAR ch = pch[ i ];
                    
                    if (ch == 0 || !IsValidWideChar( ch ))
                        ch = _T('?');

                    pch2[i] = ch;
                }

                hr = THR( InsertTextInternal( ptpAfterInsert, pch2, cch, dwFlags ) );

                delete pch2;

                goto Cleanup;
            }

            sidCurr = ScriptIDFromCh(chCurr);

            if (AreDifferentScriptIDs( & sidChunk, sidCurr ))
                break;

            pchCurr++;
            cchLeft--;
        }

        cchChunk = pchCurr - pchStart;

        // If this is the first chunk, attempt to merge with a text pos to the left
        if(ptpLeft->IsText() && !AreDifferentScriptIDs(&sidChunk, sidLast))
        {

            Assert(pchStart == pch);
            ptpLeft->ChangeCch(cchChunk);
            ptpLeft->DataThis()->t._sid = sidChunk;
        }
        else
        {
            // If we can merge with the text pos after insertion, do so
            if(cchLeft == 0 && 
               ptpAfterInsert->IsText() &&
                !AreDifferentScriptIDs(&sidChunk, ptpAfterInsert->Sid()))
            {
                WHEN_DBG( ichAfterInsert = cchChunk );
                ptpAfterInsert->ChangeCch(cchChunk);
                ptpAfterInsert->DataThis()->t._sid = sidChunk;
            }
            else
            {
                if( sidChunk == sidMerge )
                    sidChunk = sidDefault;

                // No merging - we actually have to create a new text pos
                ptpLeft = NewTextPos(cchChunk, sidChunk, lTextIDCurr);
                if(!ptpLeft)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                hr = THR(Insert(ptpLeft, &tpg));
                if(hr)
                    goto Cleanup;

                Assert(tpg.AttachedTreePos() == ptpAfterInsert);
                Assert(tpg.AdjacentTreePos( TPG_LEFT ) == ptpLeft);
            }
        }

        pchStart  = pchCurr;
        sidLast = sidChunk;
    }

    Assert( cch == pchCurr - pch );
    Assert( cch == ptpAfterInsert->GetCp() + ichAfterInsert - cpStart );

    //
    // Now actually stuff the characters into the story
    //

    CTxtPtr( this, cpStart).InsertRange( cch, pch );

    //
    // Send the notification
    //
    
    Notify( & nf );

    Undo.CreateAndSubmit();

    Assert( IsNodeValid() );

Cleanup:

    RRETURN(hr);
}

DeclareTag(tagTreePosNoIsNodeValid,"TreePoslist","Don't Validate Tree (in nodal sense)");

#if DBG==1
// Verify that the runs match the state of the tree
BOOL
CMarkup::IsNodeValid()
{
    BOOL        fValid = FALSE;
    CTreePos *  ptpCurr, *ptpStored;
    CTreeNode * pNodeCurr = NULL, *pNodeStored;
    CStackPtrAry<CTreeNode*, 8> aryNodes(Mt(Mem));
    CStackPtrAry<CTreePos*, 8> aryTreePosOverlap(Mt(Mem));
    CStackPtrAry<LONG_PTR, 8> aryTextIDSeen(Mt(Mem));
    long        lCurrTextID = 0;
    BOOL        fAddingOverlap = TRUE;
    CTxtPtr     tp( this, 0 );

    if (IsTagEnabled( tagTreePosNoIsNodeValid ))
        return TRUE;

    ptpCurr = FirstTreePos();

    while(ptpCurr)
    {
        if(ptpCurr->IsText() || ptpCurr->IsPointer())
        {
            if(!fAddingOverlap)
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - Text/Pointer pos inside inclusion"));
                goto Error;
            }

            if(aryTreePosOverlap.Size())
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - Text/Pointer pos inside inclusion"));
                goto Error;
            }


            if(aryNodes.Size() == 0)
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - Text/Pointer pos outside of all nodes"));
                goto Error;
            }

            if(ptpCurr->IsText())
            {
                tp.AdvanceCp( ptpCurr->Cch() );

                if( ptpCurr->TextID() != lCurrTextID )
                {
                    lCurrTextID = ptpCurr->TextID();
                    if( lCurrTextID != 0 )
                    {
                        if( aryTextIDSeen.Find( ptpCurr->TextID() ) != -1 )
                        {
                            TraceTag((tagError, "CMarkup::IsNodeValid - non contiguous TextID"));
                            goto Error;
                        }

                        aryTextIDSeen.Append( lCurrTextID );
                    }
                }
                    
            }

            goto NextLoop;
        }

        if(!ptpCurr->IsNode())
        {
            TraceTag((tagError, "CMarkup::IsNodeValid - CTreePos that isn't Text, Pointer or Node"));
            goto Error;
        }

        lCurrTextID = 0;

        if( tp.GetChar() != WCH_NODE )
        {
            TraceTag((tagError, "CMarkup::IsNodeValid - NodePos without WCH_NODE"));
            goto Error;
        }

        tp.AdvanceCp(1);

        if(ptpCurr->IsBeginNode())
        {
            if(ptpCurr->Branch()->IsDead())
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - dead node in tree"));
                goto Error;
            }

            if( ptpCurr->Branch()->Parent() != pNodeCurr )
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - invalid parent chain"));
                goto Error;
            }
        }


        if(ptpCurr->IsBeginNode() && ptpCurr->IsEdgeScope())
        {
            if(aryTreePosOverlap.Size())
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - BeginEdge Node pos inside of inclusion"));
                goto Error;
            }

            pNodeCurr = ptpCurr->Branch();

            Assert(pNodeCurr);
            aryNodes.Append(pNodeCurr);

            if(pNodeCurr->Element()->__pNodeFirstBranch != pNodeCurr)
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - wrong first branch pointer"));
                goto Error;
            }

            if(!pNodeCurr->Element()->IsInMarkup())
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - element not marked as in markup"));
                goto Error;
            }

        }
        else if(ptpCurr->IsBeginNode())
        {
            Assert(!ptpCurr->IsEdgeScope());

            if(aryTreePosOverlap.Size() == 0)
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - BeginNonEdge Node pos without corresponding end"));
                goto Error;
            }

            if(fAddingOverlap)
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - Empty inclusion, non optimal tree"));
                goto Error;
            }

            fAddingOverlap = FALSE;

            ptpStored = aryTreePosOverlap[aryTreePosOverlap.Size()-1];
            aryTreePosOverlap.Delete(aryTreePosOverlap.Size()-1);

            if(aryTreePosOverlap.Size() == 0)
                fAddingOverlap = TRUE;

            if(ptpStored->Branch()->Element() != ptpCurr->Branch()->Element())
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - Unmatched/Unbalanced node pos' inside of inclusion"));
                goto Error;
            }

            Assert(ptpCurr->Branch());
            aryNodes.Append(ptpCurr->Branch());

            pNodeCurr = ptpCurr->Branch();
        }
        else if(ptpCurr->IsEdgeScope())
        {
            Assert(ptpCurr->IsEndNode());

            if(!fAddingOverlap)
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - EndEdge Node pos in wrong place in inclusion"));
                goto Error;
            }

            if(!aryNodes.Size())
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - EndEdge Node pos without corresponding begin"));
                goto Error;
            }


            pNodeStored = aryNodes[aryNodes.Size()-1];
            aryNodes.Delete(aryNodes.Size()-1);

            pNodeCurr = ptpCurr->Branch();

            if(pNodeStored != pNodeCurr)
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - EndEdge Node pos does not match begin"));
                goto Error;
            }

            if( aryNodes.Size() )
                pNodeCurr = aryNodes[aryNodes.Size()-1];

            if(aryTreePosOverlap.Size())
                fAddingOverlap = FALSE;
        }
        else
        {
            Assert(ptpCurr->IsEndNode() && !ptpCurr->IsEdgeScope());

            if(!fAddingOverlap)
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - EndNonEdge Node pos in wrong place in inclusion"));
                goto Error;
            }


            pNodeStored = aryNodes[aryNodes.Size()-1];
            aryNodes.Delete(aryNodes.Size()-1);

            pNodeCurr = ptpCurr->Branch();

            if(pNodeStored != pNodeCurr)
            {
                TraceTag((tagError, "CMarkup::IsNodeValid - EndNonEdge Node pos does not match begin"));
                goto Error;
            }

            if( aryNodes.Size() )
                pNodeCurr = aryNodes[aryNodes.Size()-1];

            aryTreePosOverlap.Append(ptpCurr);
        }

NextLoop:
        ptpCurr = ptpCurr->NextTreePos();
    }

    if( !AreChangesValid() )
    {
        TraceTag((tagError, "CMarkup::IsNodeValid - Changes out of sync with markup"));
        goto Error;
    }

    if(aryNodes.Size() == 0 && aryTreePosOverlap.Size() == 0)
    {
        fValid = TRUE;
    }
    else
    {
        TraceTag((tagError, "CMarkup::IsNodeValid - unclosed nodes in CTreePosList"));
    }

Error:
    return fValid;
}
#endif

