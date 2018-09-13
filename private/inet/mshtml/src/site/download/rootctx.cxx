//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       rootctx.cxx
//
//  Contents:   CHtmRootParseCtx adds text and nodes to the tree on
//              behalf of the parser.
//
//              CHtmTopParseCtx is also defined here.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_ROOTCTX_HXX_
#define X_ROOTCTX_HXX_
#include "rootctx.hxx"
#endif

#ifndef X_UNISID_H
#define X_UNISID_H
#include <unisid.h>
#endif

#ifndef X_TXTDEFS_H
#define X_TXTDEFS_H
#include "txtdefs.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

ExternTag(tagParse);

DeclareTag(tagRootParseCtx,     "Tree",     "Trace RootParseCtx");

MtDefine(CHtmRootParseCtx, CHtmParseCtx, "CHtmRootParseCtx");
MtDefine(CHtmTopParseCtx, CHtmParseCtx, "CHtmTopParseCtx");

MtDefine(RootParseCtx, Metrics, "Tree Building");
MtDefine(ParseTextNotifications, RootParseCtx, "Text Notifications sent");
MtDefine(ParseElementNotifications, RootParseCtx, "Element Notifications sent");
MtDefine(ParseNailDownChain, RootParseCtx, "Chain nailed down");
MtDefine(ParseInclusions, RootParseCtx, "Inclusions built");
MtDefine(ParsePrepare, RootParseCtx, "Prepare called");



extern ELEMENT_TAG s_atagNull[];

//+------------------------------------------------------------------------
//
//  CHtmRootParseCtx
//
//  The root parse context.
//
//  The root parse context is responsible for:
//
//  1. Creating nodes for elements
//  2. Putting nodes and text into the tree verbatim
//  3. Ending elements, creating "proxy" nodes as needed
//
//-------------------------------------------------------------------------

ELEMENT_TAG s_atagRootReject[] = {ETAG_NULL};

//+----------------------------------------------------------------------------
//
//  Members:    Factory, constructor, destructor
//
//  Synopsis:   hold on to root site
//
//-----------------------------------------------------------------------------
HRESULT
CreateHtmRootParseCtx(CHtmParseCtx **pphpx, CMarkup *pMarkup)
{
    *pphpx = new CHtmRootParseCtx(pMarkup);
    if (!*pphpx)
        RRETURN(E_OUTOFMEMORY);

    return S_OK;
}

CHtmRootParseCtx::CHtmRootParseCtx(CMarkup *pMarkup)
  : CHtmParseCtx(0.0)
{
    pMarkup->AddRef();
    _pMarkup       = pMarkup;
    _pDoc          = pMarkup->Doc();
    _atagReject    = s_atagNull;
}

CHtmRootParseCtx::~CHtmRootParseCtx()
{
    // Defensive deletion and release of all pointers
    // should already be null except in out of memory or other catastophic error
    
    _pMarkup->SetRootParseCtx( NULL );
    
    _pMarkup->Release();
}

CHtmParseCtx *
CHtmRootParseCtx::GetHpxEmbed()
{
    return this;
}

BOOL
CHtmRootParseCtx::SetGapToFrontier( CTreePosGap * ptpg )
{
    if( _ptpAfterFrontier )
    {
        IGNORE_HR( ptpg->MoveTo( _ptpAfterFrontier, TPG_RIGHT ) );
        return TRUE;
    }

    return FALSE;
}


//+----------------------------------------------------------------------------
//
//  Members:    Prepare, Commit, Finish
//
//-----------------------------------------------------------------------------

HRESULT
CHtmRootParseCtx::Init()
{
    HRESULT hr = S_OK;
    
    _pMarkup->SetRootParseCtx( this );
    
    Assert(!_pMarkup->IsStreaming());
    _pMarkup->SetStreaming(TRUE);

    _sidLast = sidAsciiLatin;

    Assert( ! _pMarkup->_fNoUndoInfo );
    _pMarkup->_fNoUndoInfo = TRUE;

    // Set up the dummy text node in my insertion chain
    _tdpTextDummy.SetType( CTreePos::Text );
    _tdpTextDummy.SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD | CTreePos::TPF_DATA_POS );
    WHEN_DBG( _tdpTextDummy._pOwner = _pMarkup; );

    _ptpChainTail = & _tdpTextDummy;
    _ptpChainCurr = & _tdpTextDummy;

    RRETURN(hr);
}


HRESULT
CHtmRootParseCtx::Prepare()
{
    HRESULT hr = S_OK;

    _fLazyPrepareNeeded = TRUE;

    MtAdd(Mt(ParsePrepare), 1, 0);
    
    RRETURN(hr);
}

HRESULT
CHtmRootParseCtx::Commit()
{
    HRESULT         hr;

    hr = THR(FlushNotifications());

    AssertSz( _pMarkup->IsNodeValid(), "Markup not valid after root parse ctx, talk to JBeda");

    RRETURN(hr);
}

HRESULT
CHtmRootParseCtx::Finish()
{
    HRESULT hr = S_OK;
        

    // Step 1: Commit
    
    hr = THR(Commit());
    if (hr)
        goto Cleanup;

    // Step 2: Update the IsStreaming flag
    Assert(_pMarkup->IsStreaming());
    _pMarkup->SetStreaming(FALSE);

    _pMarkup->CompactStory();

    _pMarkup->SetRootParseCtx( NULL );

    Assert( _pMarkup->_fNoUndoInfo );
    _pMarkup->_fNoUndoInfo = FALSE;

Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmRootParseCtx::InsertLPointer ( CTreePos * * pptp, CTreeNode * pNodeCur)
{
    RRETURN(InsertPointer(pptp, pNodeCur, TRUE));
}

HRESULT
CHtmRootParseCtx::InsertRPointer ( CTreePos * * pptp, CTreeNode * pNodeCur)
{
    RRETURN(InsertPointer(pptp, pNodeCur, TRUE));
}

HRESULT
CHtmRootParseCtx::InsertPointer ( CTreePos * * pptp, CTreeNode * pNodeCur, BOOL fRightGravity )
{
    HRESULT hr = S_OK;

    Assert( pptp );

    if (_fLazyPrepareNeeded)
    {
        LazyPrepare( pNodeCur );
    }
    VALIDATE( pNodeCur );

    // Quick and dirty way to get a pos into the tree
    NailDownChain();

    *pptp = _pMarkup->NewPointerPos( NULL, fRightGravity, FALSE );
    if( ! *pptp )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( _pMarkup->Insert( *pptp, _ptpAfterFrontier, TRUE ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

HRESULT
CHtmRootParseCtx::InsertTextFrag ( TCHAR * pch, ULONG cch, CTreeNode * pNodeCur )
{
    HRESULT     hr = S_OK;
    CTreePos *  ptpTextFrag = NULL;
    CMarkupTextFragContext * ptfc;

    Assert( pch && cch );

    if (!_pDoc->_fDesignMode)
        goto Cleanup;

    if (_fLazyPrepareNeeded)
    {
        LazyPrepare( pNodeCur );
    }
    VALIDATE( pNodeCur );

    // Insert the text frag into the list
    ptfc = _pMarkup->EnsureTextFragContext();
    if( !ptfc )
        goto OutOfMemory;

    hr = THR( InsertPointer( &ptpTextFrag , pNodeCur, FALSE ) );
    if (hr) 
        goto Cleanup;
    
#if DBG==1
    if( ptfc->_aryMarkupTextFrag.Size() )
    {
        CTreePos * ptpLast = ptfc->_aryMarkupTextFrag[ptfc->_aryMarkupTextFrag.Size()-1]._ptpTextFrag;
        Assert( ptpLast );
        Assert( ptpLast->InternalCompare( ptpTextFrag ) == -1 );
    }
#endif

    hr = THR( ptfc->AddTextFrag( ptpTextFrag, pch, cch, ptfc->_aryMarkupTextFrag.Size() ) );
    if (hr)
    {
        IGNORE_HR( _pMarkup->Remove( ptpTextFrag ) );
        goto Cleanup;
    }

    WHEN_DBG( ptfc->TextFragAssertOrder() );

Cleanup:

    RRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  CHtmRootParseCtx::BeginElement
//
//  1. Create the new node
//  2. Add it to the tree
//  3. Call hack code
//
//-------------------------------------------------------------------------
HRESULT
CHtmRootParseCtx::BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty)
{
    HRESULT         hr = S_OK;
    CTreeNode *     pNode;

    TraceTagEx((tagRootParseCtx, TAG_NONAME,
        "RootParse      : BeginElement %S.E%d",
        pel->TagName(), pel->SN()));

    // Step 1: set our insertion point if needed
    if (_fLazyPrepareNeeded)
    {
        LazyPrepare( pNodeCur );
    }
    VALIDATE( pNodeCur );
   
    // Step 2: create the node

    pNode = *ppNodeNew = new CTreeNode(pNodeCur, pel);
    if (!*ppNodeNew)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

#if NOPARSEADDREF
    // Addref for the parser -- get rid of this!
    (*ppNodeNew)->NodeAddRef();
#endif

    pel->__pNodeFirstBranch = pNode;

    {
        CTreePos *  ptpBegin, * ptpEnd;
        CTreePos *  ptpAfterCurr = _ptpChainCurr->Next();

        //
        // Step 3: append the node into the pending chain
        //

        // Initialize/create the tree poses
        Assert( pNode->GetBeginPos()->IsUninit() );
        ptpBegin = pNode->InitBeginPos( TRUE );
        Assert( ptpBegin );

        Assert( pNode->GetEndPos()->IsUninit() );
        ptpEnd = pNode->InitEndPos( TRUE );
        Assert( ptpEnd );

        // Add them to the chain
        ptpBegin->SetFirstChild( _ptpChainCurr );
        ptpBegin->SetNext( ptpEnd );
        ptpBegin->SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
        ptpBegin->SetLeftThread( _ptpChainCurr );
        ptpBegin->SetRightThread( ptpEnd );
#endif

        ptpEnd->SetFirstChild( ptpBegin );
        ptpEnd->SetNext( ptpAfterCurr );
        ptpEnd->SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
        ptpEnd->SetLeftThread( ptpBegin );
        ptpEnd->SetRightThread( ptpAfterCurr );
#endif
        _ptpChainCurr->SetNext( ptpBegin );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
        _ptpChainCurr->SetRightThread( ptpBegin );
#endif
        Assert( _ptpChainCurr->IsLeftChild() );
        Assert( _ptpChainCurr->IsLastChild() );

        if( ptpAfterCurr )
        {
            ptpAfterCurr->SetFirstChild( ptpEnd );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
            ptpAfterCurr->SetLeftThread( ptpEnd );
#endif
            Assert( ptpAfterCurr->IsLeftChild() );
            Assert( ptpAfterCurr->IsLastChild() );
        }
        else
            _ptpChainTail = ptpEnd;

        // BUGBUG: update the counts on the
        // insertion list as we add these.

        // The node is now "in" the tree so
        // addref it for the tree
        pNode->PrivateEnterTree();


        // Step 4: remember info for notifications
        if (!_pNodeForNotify)
            _pNodeForNotify = pNodeCur;
        _nElementsAdded++;
        if( !_ptpElementAdded )
            _ptpElementAdded = ptpBegin;

        // "Add" the characters
        _cchNodeBefore++;
        _cchNodeAfter++;
        AddCharsToNotification( _cpChainCurr, 2 );

        pel->SetMarkupPtr( _pMarkup);
        pel->PrivateEnterTree();

        // Step 5: Advance the frontier
        _ptpChainCurr = ptpBegin;
        _pNodeChainCurr = pNode;
        _cpChainCurr++;
    }


    // Step 6: compatibility hacks
    hr = THR(HookBeginElement(pNode));
    if (hr)
        goto Cleanup;
    
Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  CHtmRootParseCtx::EndElement
//
//  1. Call hack code
//  2. Create proxy nodes and add them all to the tree if needed
//  3. Compute and return new current node
//
//-------------------------------------------------------------------------
HRESULT
CHtmRootParseCtx::EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd)
{
    HRESULT hr = S_OK;
    CElement * pElementEnd = pNodeEnd->Element();
    BOOL    fFlushNotification = FALSE;

    TraceTagEx((tagRootParseCtx, TAG_NONAME,
        "RootParse      : EndElement %S.E%d",
        pNodeEnd->Element()->TagName(), pNodeEnd->Element()->SN()));

    // Make sure the node ending passed in is actually in 
    // the pNodeCur's branch
    Assert( pNodeCur->SearchBranchToRootForNode( pNodeEnd ) );

    // Step 1: set our insertion point if needed
    if (_fLazyPrepareNeeded)
    {
        LazyPrepare( pNodeCur );
    }
    VALIDATE( pNodeCur );
       
    // Step 2: compatibility hacks - this is kind of
    // a nasty place to put this, but we need to make
    // sure that the element is in the tree before
    // we do the hacks.

    hr = THR(HookEndElement(pNodeEnd, pNodeCur));
    if (hr)
        goto Cleanup;

    if (!_pNodeForNotify)
    {
        _pNodeForNotify = pNodeCur;
    }

    // Step 3: decide if we are going to flush a notification
    // because the element is ending
    {
        // This asserts that elements that we have just put in
        // the tree (below the notification) don't want text
        // change notifications.
        Assert(     ! pElementEnd->WantTextChangeNotifications()
                ||  ! _fTextPendingValid
                ||  _nfTextPending.Node()->SearchBranchToRootForScope( pElementEnd ) );

        if(   pElementEnd->WantTextChangeNotifications()
           || pElementEnd->WantEndParseNotification() )
        {
            fFlushNotification = TRUE;
        }
    }

    // step 4: optimization: bottom node is ending

    if (pNodeEnd == pNodeCur)
    {
        *ppNodeNew = pNodeEnd->Parent();

        if( *ppNodeNew )
        {
#ifdef NOPARSEADDREF
            (*ppNodeNew)->NodeAddRef();
#endif

            // If we are at the end of the chain, nail
            // down the chain and advance the real frontier
            if( _ptpChainCurr == _ptpChainTail )
            {
                WHEN_DBG( CTreePos * ptpContent );

                NailDownChain();

                WHEN_DBG( ptpContent = ) AdvanceFrontier();

                Assert(     ptpContent->IsEndNode()
                       &&   ptpContent->Branch() == pNodeEnd );

            }
            // otherwise, advance within the chain
            else
            {
                _ptpChainCurr = _ptpChainCurr->Next();

                Assert(     _ptpChainCurr
                       &&   _ptpChainCurr->IsEndNode()
                       &&   _ptpChainCurr->Branch() == pNodeEnd );

                Assert( _cchNodeAfter );
            }

            _cpChainCurr++;
            _pNodeChainCurr = *ppNodeNew;

            // If we have WCH_NODE chars pending after the current cp
            // move them behind the current cp.  If this isn't the case,
            // we just advance the cp and assume the character we are moving
            // over is a WCH_NODE
            if( _cchNodeAfter)
            {
                _cchNodeAfter--;
                _cchNodeBefore++;
            }
#if DBG==1
            else
                Assert( CTxtPtr( _pMarkup, _cpChainCurr - 1 ).GetChar() == WCH_NODE );
#endif
        }
        else
        {
            Assert( pNodeEnd->Tag() == ETAG_ROOT );
            goto Cleanup;
        }
    }

    // step 5: create an inclusion and move the end pos
    // for pNodeEnd into it.  Get the new node for ppNodeNew

    else
    {
        hr = THR( OverlappedEndElement( ppNodeNew, pNodeCur, pNodeEnd, fFlushNotification ) );
        if (hr)
            goto Cleanup;
    }

    // step 6: Push off the notificication or update it if necessary

    {
        if( fFlushNotification && _fTextPendingValid )
        {
            FlushTextNotification();
        }

        // If we didn't send the notification above and we
        // think we want to send it to the element that is ending
        // we should send it to that element's parent instead
        if(     _fTextPendingValid
           &&   _nfTextPending.Node()->Element() == pElementEnd )
        {
            _nfTextPending.SetNode( pNodeEnd->Parent() );
        }
        else if( _pNodeForNotify && _pNodeForNotify->Element() == pElementEnd )
        {
            _pNodeForNotify = pNodeEnd->Parent();
            Assert( _pNodeForNotify );
        }
    }

Cleanup:
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     OverlappedEndElement
//
//  Synopsis:   Handles the complex case of overlapping end elements
//
//-----------------------------------------------------------------------------
HRESULT 
CHtmRootParseCtx::OverlappedEndElement( CTreeNode **ppNodeNew, CTreeNode* pNodeCur, CTreeNode *pNodeEnd, BOOL fFlushNotification )
{
    HRESULT         hr = S_OK;
    CTreeNode *     pNodeReparent;
    CTreePos *      ptpWalkLeft, * ptpInsRight, * ptpInsLeft;
    long            iLeft;
    long            cIncl = 0;
    CTreeNode *     pNodeNotifyRight;

    // The basic idea here is that we want to create an inclusion
    // for the element while sending the right notifications.  I'm going to
    // use the fact that there can be no real content after the frontier
    // besides end edges.  If we encounter pointers here, we will move
    // them.

    // BUGBUG: do this without nailing down the chain!
    hr = THR( NailDownChain() );
    if (hr)
        goto Cleanup;

    Assert( _cchNodeBefore == 0 );

    // Walk up the tree to count the size of the inclusion.  Change
    // the end edges to non edges to form the left side of the inclusion
    {
        CTreeNode * pNodeWalk;

        for( pNodeWalk = pNodeCur; pNodeWalk != pNodeEnd; pNodeWalk = pNodeWalk->Parent() )
        {
            CTreePos * ptpEnd = pNodeWalk->GetEndPos();

            ptpEnd->SetScopeFlags( FALSE );

            cIncl++;
        }
    }

    // Make sure that _pNdoeForNotify is in sync with the notification
    if( _fTextPendingValid )
        _pNodeForNotify = _nfTextPending.Node();

    // Create nodes on the right side of the inclusion.  Move any pointers
    // that may be inside of the inclusion to the appropriate places on the right
    pNodeReparent = pNodeEnd->Parent();
    ptpWalkLeft = pNodeEnd->GetEndPos()->PreviousTreePos();
    ptpInsRight = pNodeReparent->GetEndPos();
    ptpInsLeft = pNodeEnd->GetEndPos();
    pNodeNotifyRight = _pNodeForNotify;

    for( iLeft = cIncl; iLeft > 0; iLeft-- )
    {
        CElement *  pElementCur;
        CTreeNode * pNodeNew;
        CTreePos *  ptpNew;

        // Move ptpWalkLeft until we get to an end nonedge.  Everything
        // else we see in here must be a pointer and we should move to the
        // right side of the inclusion
        while( ptpWalkLeft->IsPointer() )
        {
            CTreePos * ptpWalkLeftNext = ptpWalkLeft->PreviousTreePos();
            _pMarkup->Move( ptpWalkLeft, ptpInsRight, TRUE );
            ptpInsRight = ptpWalkLeft;
            ptpWalkLeft = ptpWalkLeftNext;
        }

        Assert( ptpWalkLeft->IsEndNode() && ! ptpWalkLeft->IsEdgeScope() );
        pElementCur = ptpWalkLeft->Branch()->Element();

        pNodeNew = new CTreeNode( pNodeReparent, pElementCur );
        if( !pNodeNew )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }


        // initialize the begin pos and insert it after the begin of our parent
        ptpNew = pNodeNew->InitBeginPos( FALSE );
        Verify( ! _pMarkup->Insert( ptpNew, ptpInsLeft, FALSE ) );
        ptpInsLeft = ptpNew;

        // initialize the end pos and insert it on the right
        ptpNew = pNodeNew->InitEndPos( TRUE );
        Verify( ! _pMarkup->Insert( ptpNew, ptpInsRight, TRUE ) );
        ptpInsRight = ptpNew;

        // tell the node it is in the tree
        pNodeNew->PrivateEnterTree();

        if( _pNodeForNotify == ptpWalkLeft->Branch() )
            pNodeNotifyRight = pNodeNew;

        // set up for next time
        pNodeReparent = pNodeNew;
        ptpWalkLeft = ptpWalkLeft->PreviousTreePos();
    }

    if( _pNodeForNotify == pNodeEnd )
        pNodeNotifyRight = pNodeEnd->Parent();

    *ppNodeNew = pNodeReparent;

    // There is really no efficient way to do notifications here unless
    // we lie a little bit.  So that is what we are going to do.

    // Assume that there are no pending notifications and we have an I overlapping
    // outside of a B -- <P><B><I>{/I}</B>{I}</I></P>
    // Before this call we have this: <P><B><I></I></B><P> and we flushed all notifications.
    // now we want to create an inclusion to represent the overlap.

    // Here is a way to describe this change to the tree.  We don't tell the B
    // anything.  We would tell the I that it got one more character under it for the {I}
    // on the right side of the inclusion.  This would bubble up to the P.  We would then
    // tell the P that it got another character for the </I>.  Net change to the B is 0, to
    // the I is 1, and to the P is 2.  

    // For the P, this is correct. But the I now has three more characters under it ({/I}</B>{I})
    // then it did before we started all of this.  We only reported a change of +1.  
    // eg. The </B> is not new to the tree but is new to the I.  The {/I} was converted from the 
    // </I> that wasn't new to the tree but new to the I.
    
    // The net result is that the I needs to know that 3 characters were added under it but
    // the P needs to know that only 2 characters were added under it.  The B doesn't need to
    // hear a thing!

    // Here is what I'm going to do.  I'm going to remove the </B> completely and send a chars deleted
    // notification to just the P.  Next, I'm going to send a notification for the entire inclusion
    // to the second context of the I.  This way it will get to the I and the P but not the B.  By
    // doing this the net change to the P is +2 and the net change to the I is +3.

    // Trying to use any pending notifications and trying to leave a pending notification complicates
    // this process.  I can't leave a notification that needs to be broken up.  So any notification that
    // I leave around must be completely within the lowest node (i.e. the second context of the I).  So
    // this means that the notification must cover the entire inclusion.

    // So here is the general outline of how things should go:
    // 1. Make the change but don't tell anyone
    // 2. Send a notification to the P for the removal of the </B> character.
    // 3. Send one notification for the entire inclusion (3 chars starting at {/I} to the second
    //    context of the I.

    // This lets the B know about anything under that may be pending.
    // We only need to do this if the B wants its notifiaction.

    {
        BOOL fKernelInNotification;

        // See if the kernel is in any notification that we already have
        // pending.  If so, we don't have to send the remove notification
        // for the kernel.
        fKernelInNotification = !fFlushNotification
            &&  _fTextPendingValid
            &&  (_cpChainCurr + cIncl < _nfTextPending.Cp(0) + _nfTextPending.Cch(LONG_MAX) );

        if( fFlushNotification || !fKernelInNotification )
        {
            WHEN_DBG( _nfTextPending._fNoTextValidate = TRUE );
            FlushTextNotification();
            WHEN_DBG( _nfTextPending._fNoTextValidate = FALSE );
        }

        // Send the remove notification to the P (pNodeEnd->Parent())
        if( !fKernelInNotification )
        {
            CNotification nfRemove;
            nfRemove.CharsDeleted( _cpChainCurr + cIncl, 1, pNodeEnd->Parent() );
            WHEN_DBG( nfRemove._fNoTextValidate = TRUE );
            _pMarkup->Notify( nfRemove );
            WHEN_DBG( nfRemove._fNoTextValidate = FALSE );

            MtAdd(Mt(ParseTextNotifications), 1, 0);
        }

        // Add the inclusion characters (plus the kernel) to the pending
        // notification.  Start this at the beginning of the inclusion and
        // send it to the notify node on the right side (second context of the I).  
        {
            _pNodeForNotify = pNodeNotifyRight;
            AddCharsToNotification( _cpChainCurr, 2 * cIncl + (fKernelInNotification ? 0 : 1) );
        }

        // Mark node chars for insertion and advance the frontier
        _cchNodeBefore += cIncl;
        _cchNodeAfter += cIncl;
        _cpChainCurr += 2 * cIncl + 1;
        _pNodeChainCurr = pNodeReparent;
        _ptpAfterFrontier = pNodeReparent->GetBeginPos();
        AdvanceFrontier();
    }

    MtAdd(Mt(ParseInclusions), 1, 0);

Cleanup:
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     AddText
//
//  Synopsis:   Inserts text directly into the tree
//
//-----------------------------------------------------------------------------

HRESULT
CHtmRootParseCtx::AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii)
{
    HRESULT         hr = S_OK;

    // No more null runs
    Assert( cch != 0 );

    TraceTagEx((tagRootParseCtx, TAG_NONAME,
        "RootParse      : AddText cch=%d",
        cch));

    // Step 1: set our insertion point if needed
    if (_fLazyPrepareNeeded)
    {
        LazyPrepare( pNode );
    }
    VALIDATE( pNode );

    if (! _pNodeForNotify)
    {
        _pNodeForNotify = pNode;
    }

    // Step 2: Put down any pending WCH_NODE characters

    if( _cchNodeBefore )
    {
        ULONG    cpInsert = _cpChainCurr - _cchNodeBefore;

        Verify( CTxtPtr( _pMarkup, cpInsert ).
                    InsertRepeatingChar( _cchNodeBefore, WCH_NODE ) == _cchNodeBefore );
        _cchNodeBefore = 0;
    }

    // Step 3: Insert a run or add chars to a current run
    AddCharsToNotification( _cpChainCurr, cch );
 
    // First case handles the all ASCII case
    if ( !cch || _sidLast == sidAsciiLatin && fAscii )
    {
        if ( ! _ptpChainCurr->IsText() )
        {
            CTreePos    *ptpTextNew;
            // insert the new text pos

            ptpTextNew = InsertNewTextPosInChain( cch, _sidLast, _ptpChainCurr );
            if (!ptpTextNew)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            _ptpChainCurr = ptpTextNew;
        }
        else
        {
            _ptpChainCurr->DataThis()->t._cch += cch;
        }
    }
    else
    {
        // Slow loop to take care of non ascii characters
        TCHAR *pchStart = pch;
        TCHAR *pchScan = pch;
        ULONG cchScan = cch;
        LONG sid = sidDefault;

        for (;;)
        {
            // BUGBUG: this might do something strange when _sidLast == sidDefault
            // or _sidLast == sidMerge (jbeda)

            // Break string into uniform sid's, merging from left
            while (cchScan)
            {
                sid = ScriptIDFromCh(*pchScan);
                sid = FoldScriptIDs(_sidLast, sid);
                
                if (sid != _sidLast)
                    break;

                cchScan--;
                pchScan++;
            }

            // Add to the current run or create a new run
            if (pchScan > pchStart)
            {
                long cchChunk = pchScan - pchStart;

                if ( ! _ptpChainCurr->IsText() || _ptpChainCurr->Sid() != _sidLast )
                {
                    CTreePos * ptpTextNew;

                    ptpTextNew = InsertNewTextPosInChain( cchChunk, _sidLast, _ptpChainCurr );
                    if (!ptpTextNew)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }

                    _ptpChainCurr = ptpTextNew;
                }
                else
                {
                    _ptpChainCurr->DataThis()->t._cch += cchChunk;
                }
            }

            pchStart = pchScan;

            if (!cchScan)
                break;
                
            Assert(sid != sidMerge);

            _sidLast = sid;
        }

    }


    // Step 2: Add the actual text to the story
    if (cch > 0)
    {
        Verify(
            ULONG(
                CTxtPtr( _pMarkup, _cpChainCurr ).
                    InsertRange( cch, pch ) ) == cch );
    }

    _cpChainCurr += cch;

Cleanup:

    RRETURN( hr );
}


//+------------------------------------------------------------------------
//
//  Member:     HookBeginElement
//
//  Synopsis:   Compatibility hacks for begin element
//
//-------------------------------------------------------------------------
HRESULT
CHtmRootParseCtx::HookBeginElement(CTreeNode * pNode)
{
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     HookEndElement
//
//  Synopsis:   Compatibility hacks for end element
//
//-------------------------------------------------------------------------
HRESULT
CHtmRootParseCtx::HookEndElement(CTreeNode * pNode, CTreeNode * pNodeCur)
{
    //
    // <p>'s which have </p>'s render differently.  Here, when a <p> goes
    // out of scope, we check to see if a </p> was detected, and invalidate
    // the para graph to render correctly.  We only have to do this if the
    // paragraph has gotten an EnterTree notification.
    //

    if (    pNode->Tag() == ETAG_P 
        &&  pNode->Element()->_fExplicitEndTag
        &&  pNode->Element()->IsInMarkup() )
    {
        // This is a hack. Basically, the regular code is WAY too slow.
        // In fact, it forces rerendering of the paragraph AND runs the
        // monster walk.
        //
        // The bug it is trying to solve is that the fancy format for the
        // P tag has already been calculated, and the </P> tag changes the
        // fancy format's _cuvSpaceAfter. Fortunately, we haven't actually used
        // the space after yet (because we haven't even parsed anything following
        // this </P> tag), and because fancy formats aren't inherited,
        // we can just recalc the format for this one tag (actually, we
        // could do even less, just reset the post spacing, but we lack
        // primitives for that).
        // - Arye
        BYTE ab[sizeof(CFormatInfo)];
        ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
        if (pNode->IsFancyFormatValid())
        {
            pNode->VoidCachedInfo();
            pNode->Element()->ComputeFormats((CFormatInfo *)ab, pNode);
        }
    }

    return S_OK;
}

void    
CHtmRootParseCtx::AddCharsToNotification( long cpStart, long cch  )
{
    if( ! _fTextPendingValid )
    {
        Assert( _pNodeForNotify );

        // We are adding chars to an existing text pos
        _nfTextPending.CharsAdded(  cpStart,
                                    cch,
                                    _pNodeForNotify );

        _nfTextPending.SetFlag( NFLAGS_CLEANCHANGE );

        _fTextPendingValid = TRUE;
    }
    else
    {
        // Add to the current notification
        _nfTextPending.AddChars( cch );
    }
}

HRESULT
CHtmRootParseCtx::NailDownChain()
{
    HRESULT     hr = S_OK;
    BOOL        fResetChain = FALSE;

    // Create/Modify text pos as necessary
    if( _tdpTextDummy.Cch() != 0 )
    {
        CTreePos   *ptpBeforeFrontier;

        ptpBeforeFrontier = _ptpAfterFrontier->PreviousTreePos();
        if(     ptpBeforeFrontier->IsText()
           &&   ptpBeforeFrontier->Sid() == _tdpTextDummy.Sid() )
        {
            ptpBeforeFrontier->ChangeCch( _tdpTextDummy.Cch() );
        }
        else
        {
            CTreePos * ptpTextNew;

            ptpTextNew = InsertNewTextPosInChain( 
                            _tdpTextDummy.Cch(),
                            _tdpTextDummy.Sid(),
                            &_tdpTextDummy );
            if( !ptpTextNew )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            Assert( _tdpTextDummy.Next() == ptpTextNew );

            if( _ptpChainCurr == &_tdpTextDummy )
                _ptpChainCurr = ptpTextNew;
        }

        fResetChain = TRUE;
    }

    // Add chain to the tree
    if( _ptpChainTail != &_tdpTextDummy )
    {
        CTreePos * ptpChainHead = _tdpTextDummy.Next();

        Assert( ptpChainHead );

        ptpChainHead->SetFirstChild( NULL );

        hr = THR( _pMarkup->InsertPosChain( ptpChainHead, _ptpChainTail, _ptpAfterFrontier ) );
        if (hr)
            goto Cleanup;

        fResetChain = TRUE;
    }

    if( fResetChain )
    {
        // update the frontier
        if( _ptpChainCurr != &_tdpTextDummy )
            _ptpAfterFrontier = _ptpChainCurr->NextTreePos();
#if DBG == 1
        // The current pos on the chain should only be the
        // dummy text pos when the chain is empty
        else
            Assert( ! _tdpTextDummy.Next() );
#endif

        // reset the chain
        _tdpTextDummy.SetNext( NULL );
        _tdpTextDummy.DataThis()->t._cch = 0;
        _tdpTextDummy.DataThis()->t._sid = _ptpChainCurr->IsText() ? _ptpChainCurr->Sid() : sidAsciiLatin;
        _ptpChainCurr = &_tdpTextDummy;
        _ptpChainTail = &_tdpTextDummy;

        MtAdd(Mt(ParseNailDownChain), 1, 0);
    }

    // insert any WCH_NODE characters
    if( _cchNodeBefore || _cchNodeAfter )
    {
        ULONG cpInsert = _cpChainCurr - _cchNodeBefore;
        ULONG cchInsert = _cchNodeBefore + _cchNodeAfter;

        Verify(
            ULONG(
                CTxtPtr( _pMarkup, cpInsert ).
                    InsertRepeatingChar( cchInsert, WCH_NODE ) ) == cchInsert );

        _cchNodeBefore = 0;
        _cchNodeAfter = 0;
    }

Cleanup:
    RRETURN( hr );
}

void 
CHtmRootParseCtx::FlushTextNotification()
{
    // Step 1: nail down the chain
    IGNORE_HR( NailDownChain() );

    // Step 2: up the documents content version
    _pMarkup->UpdateMarkupTreeVersion();

    // Step 3: send the pending text notification
    if (_fTextPendingValid)
    {
        // If we've made this notification 0 length, don't do anything with it!
        if( _nfTextPending.Cch(LONG_MAX) )
        {
            WHEN_DBG(_nfTextPending.ResetSN());

            TraceTagEx((tagRootParseCtx, TAG_NONAME,
               "RootParse       : Notification sent (%d, %S) Node(N%d.%S) cp(%d) cch(%d)",
               _nfTextPending.SerialNumber(),
               _nfTextPending.Name(),
               _nfTextPending.Node()->SN(),
               _nfTextPending.Node()->Element()->TagName(),
               _nfTextPending.Cp(0),
               _nfTextPending.Cch(LONG_MAX)));

            _nfTextPending.SetFlag(NFLAGS_PARSER_TEXTCHANGE);

            _pMarkup->Notify( _nfTextPending );

            MtAdd(Mt(ParseTextNotifications), 1, 0);
        }

        _fTextPendingValid = FALSE;
    }

    _pNodeForNotify = NULL;
}

HRESULT
CHtmRootParseCtx::FlushNotifications()
{
    HRESULT hr;
    long lVer;

    hr = S_OK;

    // Step 1: send the pending text notification

    FlushTextNotification();

    lVer = _pMarkup->GetMarkupTreeVersion();

#if DBG == 1
    _pMarkup->DbgLockTree(TRUE);
#endif

    // Step 2: send any pending ElementEnter and ElementAdded notifications

    if (_nElementsAdded)
    {
        CTreePos * ptpCurr = _ptpElementAdded;

        Assert( ptpCurr );

        {
            CNotification nf;

            nf.ElementsAdded( _ptpElementAdded->SourceIndex(), _nElementsAdded );

            TraceTagEx((tagRootParseCtx, TAG_NONAME,
               "RootParse       : Notification sent (%d, %S) si(%d) cElements(%d)",
               nf.SerialNumber(),
               nf.Name(),
               nf.SI(),
               nf.CElements()));

            _pMarkup->Notify( nf );

            // 49396: duck out if markup has been modified to avoid crash (dbau).
            if (lVer != _pMarkup->GetMarkupTreeVersion())
            {
                hr = E_ABORT;
                goto Cleanup;
            }

            MtAdd(Mt(ParseElementNotifications), 1, 0);
        }

        while (_nElementsAdded)
        {
            if( ptpCurr->IsBeginElementScope() )
            {
                CNotification   nf;
                CElement *      pElement = ptpCurr->Branch()->Element();

                nf.ElementEntertree( pElement );
                nf.SetData( ENTERTREE_PARSE );
                pElement->Notify( &nf );

                // 49396: duck out if markup has been modified to avoid crash (dbau).
                if (lVer != _pMarkup->GetMarkupTreeVersion())
                {
                    hr = E_ABORT;
                    goto Cleanup;
                }

                _nElementsAdded--;
            }

            ptpCurr = ptpCurr->NextTreePos();

            Assert( ptpCurr );
        }

        _ptpElementAdded = NULL;
    }

Cleanup:

#if DBG == 1
    _pMarkup->DbgLockTree(FALSE);
#endif

    RRETURN(hr);
}

CTreePos *
CHtmRootParseCtx::InsertNewTextPosInChain( 
    LONG cch, 
    SCRIPT_ID sid,
    CTreePos *ptpBeforeOnChain)
{
    CTreePos *  ptpTextNew;
    CTreePos *  ptpAfterCurr = ptpBeforeOnChain->Next();

    Assert( cch != 0 );
    ptpTextNew = _pMarkup->NewTextPos(cch, sid);
    if (!ptpTextNew)
    {
        goto Cleanup;
    }

    ptpTextNew->SetFirstChild( ptpBeforeOnChain );
    ptpTextNew->SetNext( ptpAfterCurr );
    ptpTextNew->SetFlag( CTreePos::TPF_LEFT_CHILD | CTreePos::TPF_LAST_CHILD );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
    ptpTextNew->SetLeftThread( ptpBeforeOnChain );
    ptpTextNew->SetRightThread( ptpAfterCurr );
#endif

    ptpBeforeOnChain->SetNext( ptpTextNew );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
    ptpBeforeOnChain->SetRightThread( ptpTextNew );
#endif
    Assert( ptpBeforeOnChain->IsLeftChild() );
    Assert( ptpBeforeOnChain->IsLastChild() );

    if( ptpAfterCurr )
    {
        ptpAfterCurr->SetFirstChild( ptpTextNew );
#if defined(MAINTAIN_SPLAYTREE_THREADS)
        ptpAfterCurr->SetLeftThread( ptpTextNew );
#endif
        Assert( ptpAfterCurr->IsLeftChild() );
        Assert( ptpAfterCurr->IsLastChild() );
    }
    else
        _ptpChainTail = ptpTextNew;

Cleanup:
    return ptpTextNew;
}

#if DBG==1
CTreePos *
#else
void
#endif
CHtmRootParseCtx::AdvanceFrontier()
{
    WHEN_DBG( CTreePos * ptpContent );

    while( _ptpAfterFrontier->IsPointer() )
        _ptpAfterFrontier = _ptpAfterFrontier->NextTreePos();

    WHEN_DBG( ptpContent = _ptpAfterFrontier );
    _ptpAfterFrontier = _ptpAfterFrontier->NextTreePos();

    if( _ptpAfterFrontier->IsPointer() )
    {
        CTreePosGap tpg( _ptpAfterFrontier, TPG_LEFT );

        tpg.PartitionPointers( _pMarkup, FALSE );

        _ptpAfterFrontier = tpg.AdjacentTreePos( TPG_RIGHT );
    }

    WHEN_DBG( return ptpContent );
}

void    
CHtmRootParseCtx::LazyPrepare( CTreeNode * pNodeUnder )
{
    CTreePos * ptpBeforeFrontier;

    Assert( _fLazyPrepareNeeded );

    // BUGBUG - should return hr here

    IGNORE_HR( _pMarkup->EmbedPointers() );

    // Set up the real frontier
    _ptpAfterFrontier = pNodeUnder->GetEndPos();

    // Set up the script ID for the accumulation TextPos
    ptpBeforeFrontier = _ptpAfterFrontier->PreviousTreePos();
    Assert( _ptpChainCurr == &_tdpTextDummy ); 
    Assert( _ptpChainTail == &_tdpTextDummy );
    if( ptpBeforeFrontier->IsText() )
    {
        _sidLast = ptpBeforeFrontier->Sid();
    }
    else
    {
        _sidLast = sidAsciiLatin;

    }

    if( ptpBeforeFrontier->IsPointer() )
    {
        CTreePosGap tpg( _ptpAfterFrontier, TPG_LEFT );

        tpg.PartitionPointers( _pMarkup, FALSE );
        
        _ptpAfterFrontier = tpg.AdjacentTreePos( TPG_RIGHT );
    }

    _ptpChainCurr->DataThis()->t._sid = _sidLast;

    // Set up the frontier inside of the chain
    _pNodeChainCurr = pNodeUnder;
    _cpChainCurr = _ptpAfterFrontier->GetCp();

    // Assert a bunch of stuff
    Assert( _cchNodeBefore == 0 );
    Assert( _cchNodeAfter == 0 );
    Assert( _pNodeForNotify == NULL );
    Assert( _nElementsAdded == 0 );
    Assert( _ptpElementAdded == NULL );
    Assert( !_fTextPendingValid );

    _fLazyPrepareNeeded = FALSE;
}

#if DBG == 1
void 
CHtmRootParseCtx::Validate( CTreeNode * pNodeUnder)
{
    Assert( ! _pMarkup->HasUnembeddedPointers() );
    
    // Make sure that we are just under the end of pNodeUnder
    if( _ptpChainCurr->Next() )
        Assert( _ptpChainCurr->Next() == pNodeUnder->GetEndPos() );
    else
    {
        CTreePos * ptpVerify = _ptpAfterFrontier;

        while( ptpVerify->IsPointer() )
            ptpVerify = ptpVerify->NextTreePos();

        Assert( ptpVerify == pNodeUnder->GetEndPos() );
    }

    // Make sure that the element we are going to send a notfication to
    // above the current insertion point
    if( _fTextPendingValid )
    {
        Assert( pNodeUnder->SearchBranchToRootForScope( _nfTextPending.Node()->Element() ) );
    }

    Assert( pNodeUnder == _pNodeChainCurr );
}
#endif


//+------------------------------------------------------------------------
//
//  CHtmTopParseCtx
//
//  The top parse context.
//
//  The top parse context is responsible for:
//
//  Throwing out text (and asserting on nonspace)
//
//  Recognizing that an input type=hidden at the beginning of the document
//  is not textlike
//
//-------------------------------------------------------------------------

class CHtmTopParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmTopParseCtx))
    
    CHtmTopParseCtx(CHtmParseCtx *phpxParent);
    
    virtual BOOL QueryTextlike(ELEMENT_TAG etag, CHtmTag *pht);

#if DBG==1
    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii) { Assert(IsAllSpaces(pch, cch)); return S_OK; }
#endif
};

HRESULT
CreateHtmTopParseCtx(CHtmParseCtx **pphpx, CHtmParseCtx *phpxParent)
{
    CHtmParseCtx *phpx;

    phpx = new CHtmTopParseCtx(phpxParent);
    if (!phpx)
        return E_OUTOFMEMORY;

    *pphpx = phpx;

    return S_OK;
}

ELEMENT_TAG
s_atagTopReject[] =
{
    ETAG_NULL
};

ELEMENT_TAG s_atagTopIgnoreEnd[] = {ETAG_HTML, ETAG_HEAD, ETAG_BODY, ETAG_NULL};

CHtmTopParseCtx::CHtmTopParseCtx(CHtmParseCtx *phpxParent)
    : CHtmParseCtx(phpxParent)
{
    _atagReject    = s_atagTopReject;
    _atagIgnoreEnd = s_atagTopIgnoreEnd;
}


BOOL
CHtmTopParseCtx::QueryTextlike(ELEMENT_TAG etag, CHtmTag *pht)
{
    Assert(!pht || pht->Is(etag));
        
    // For Netscape comptibility:
    // An INPUT in the HEAD is not textlike if the input is type=hidden.
    // Also, For IE4 compat during paste, if the head was not explicit, then all
    // inputs, including hidden are text like.
    // Also, forms tags must force a body in the paste scenario

    switch ( etag )
    {
        case ETAG_MAP :
        case ETAG_GENERIC :
        case ETAG_GENERIC_LITERAL :
        case ETAG_GENERIC_BUILTIN :
        case ETAG_BASEFONT :
        case ETAG_AREA :
        case ETAG_FORM :
        
            // Some tags should be text-like when parsing (pasting, usually)
            
            if (DYNCAST( CHtmRootParseCtx, GetHpxRoot() )->_pDoc->_fMarkupServicesParsing)
                return TRUE;
            else
                return FALSE;
                
        case ETAG_INPUT:
        
            // If not parsing from markup services (pasting, usually) then the hidden input
            // is not text-like (it can begin before the body.

            if (DYNCAST( CHtmRootParseCtx, GetHpxRoot() )->_pDoc->_fMarkupServicesParsing)
                return TRUE;
                
            {
                TCHAR * pchType;

                if (pht->ValFromName(_T("TYPE"), &pchType) && !StrCmpIC(pchType, _T("HIDDEN")))
                    return FALSE;
            }

        case ETAG_OBJECT:
        case ETAG_APPLET:
        
            // Objects and applets that appear bare at the top (not in head) are textlike

            return TRUE;

        case ETAG_A:
        default:
        
            return FALSE;
    }
}
