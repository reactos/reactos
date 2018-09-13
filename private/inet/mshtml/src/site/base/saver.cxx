//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       Saver.cxx
//
//  Contents:   Object to save to a stream
//
//  Class:      CTreeSaver
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_SAVER_HXX_
#define X_SAVER_HXX_
#include "saver.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_COMMENT_HXX_
#define X_COMMENT_HXX_
#include "comment.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

MtDefine(CTreeSaver, Locals, "CTreeSaver")

//+---------------------------------------------------------------
//
//  Member:     CTreeSaver constructor from element
//
//  Synopsis:   Construct a range saver object for saving everything
//              under a specified element.  This is the normal case.
//
//---------------------------------------------------------------

CTreeSaver::CTreeSaver(CElement* pelToSave, CStreamWriteBuff * pswb, CElement * pelContainer /* = NULL */)
{
    CTreePos * ptpStart, * ptpEnd;

    _pMarkup        = pelToSave->GetMarkup();

    pelToSave->GetTreeExtent( & ptpStart, & ptpEnd );

    Assert( ptpStart && ptpEnd );

    Verify( ! _tpgStart.MoveTo( ptpStart, TPG_RIGHT ) );
    Verify( ! _tpgEnd.MoveTo( ptpEnd, TPG_LEFT ) );
    Verify( ! _tpgEnd.MoveLeft( TPG_VALIDGAP | TPG_OKNOTTOMOVE ) );

    _pelFragment  = pelToSave;
    _pswb         = pswb;
    _fSymmetrical = FALSE;
    _fLBStartLeft = FALSE;
    _fSaveTextFrag= FALSE;
    _fLBEndLeft   = !!LineBreakChar( &_tpgEnd );
    _pelContainer = pelContainer;

}

//+---------------------------------------------------------------
//
//  Member:     CTreeSaver::Save
//
//  Synopsis:   Dump our guts to the stream _pswb.
//
//---------------------------------------------------------------

MtDefine(CTreeSaver_aryElements_pv, Locals, "CTreeSaver::Save aryElements::_pv");
HRESULT
CTreeSaver::Save()
{
    HRESULT     hr = S_OK;
    CTreePos *  ptpWalk, * ptpAfterEnd, *ptpEnd, *ptpNext;
    CElement *  pelPendingForce = NULL;
    BOOL        fSelectionEndSaved = FALSE;
    BOOL        fSuppressingPrettyCRLF = FALSE;
    BOOL        fSeenContent = FALSE;
    MarkupTextFrag *ptf = NULL;
    long            ctf = 0;

    Assert( _tpgStart.Branch()->SearchBranchToRootForScope( _pelFragment ) );
    Assert( _tpgEnd.Branch()->SearchBranchToRootForScope( _pelFragment ) );

    // If we are saving out text frags, then we must be starting at the beginning of the 
    // markup.  To change this later, we would need to search in the text frag list to
    // find the correct place to start.
    Assert( !_fSaveTextFrag || _tpgStart.AdjacentTreePos( TPG_LEFT ) == _pMarkup->FirstTreePos() );

    //
    // Initialize saver state
    //

    _pelLastBlockScope = NULL;
    _fPendingNBSP      = FALSE;

    if(     _fSaveTextFrag 
        &&  _pMarkup->HasTextFragContext() 
        &&  ! _pswb->TestFlag(WBF_SAVE_PLAINTEXT)
        &&  ! _pswb->TestFlag(WBF_FOR_RTF_CONV) )
    {
        CMarkupTextFragContext * ptfc = _pMarkup->GetTextFragContext();

        Assert( ptfc );

        ctf = ptfc->_aryMarkupTextFrag.Size();
        ptf = ptfc->_aryMarkupTextFrag;
    }

    //
    // If we are symmetrical then we want to save begin tags
    // from under the _pelFragment down to the node above _ptpStart
    //

    if( _fSymmetrical )
    {
        CTreeNode * pNodeCur = _tpgStart.Branch();
        CStackPtrAry<CElement *, 16> aryBeginElements(Mt(CTreeSaver_aryElements_pv));

        for( ; pNodeCur->Element() != _pelFragment; pNodeCur = pNodeCur->Parent() )
        {
            hr = THR( aryBeginElements.Append( pNodeCur->Element() ) );
            if (hr)
                goto Cleanup;
        }
        
        {
            int iElement = aryBeginElements.Size() - 1;

            while( iElement >= 0 )
            {
                hr = THR( SaveElement( aryBeginElements[iElement--], FALSE) );
                if (hr)
                    goto Cleanup;
            }
        }
    }

    //
    // Tell any derived classes that the selection is started
    //
    hr = THR( SaveSelection( FALSE ) );
    if (hr)
        goto Cleanup;

    // Start supressing all pretty CRLF until
    // we output real content (BUG 3844)
    if (!_pswb->TestFlag(WBF_NO_PRETTY_CRLF))
    {
        _pswb->SetFlags(WBF_NO_PRETTY_CRLF);
        fSuppressingPrettyCRLF = TRUE;
    }

    //
    // Handle any leading block breaks
    //
    if(     ! _fLBStartLeft 
        &&  _pswb->TestFlag(WBF_SAVE_PLAINTEXT)
        &&  LineBreakChar( &_tpgStart ) & BREAK_BLOCK_BREAK )
    {
        hr = THR(_pswb->NewLine());
        if (hr)
            goto Cleanup;
    }

    //
    // Walk the tree and save out tags and text
    //
    ptpEnd      = _tpgEnd.AdjacentTreePos( TPG_LEFT );
    ptpAfterEnd = _tpgEnd.AdjacentTreePos( TPG_RIGHT );

    for( ptpWalk = _tpgStart.AdjacentTreePos( TPG_RIGHT );
         ptpWalk != ptpAfterEnd;
         ptpWalk = ptpNext )
    {
        // Make sure we haven't somehow passed the top of our text frag list
        Assert( ctf <= 0 || ptpWalk->InternalCompare(ptf->_ptpTextFrag) != 1 );

        ptpNext = ptpWalk->NextTreePos();

        switch( ptpWalk->Type() )
        {
        case CTreePos::Pointer:
            if( ctf > 0 && ptpWalk == ptf->_ptpTextFrag )
            {
                // Write out the text frag...
                DWORD dwOldFlags = _pswb->ClearFlags(WBF_ENTITYREF);
                _pswb->SetFlags(WBF_SAVE_VERBATIM | WBF_NO_WRAP);
                _pswb->BeginPre();

                hr = THR(_pswb->Write(ptf->_pchTextFrag));

                _pswb->EndPre();
                _pswb->RestoreFlags(dwOldFlags);

                if (hr)
                    goto Cleanup;

                ctf--;
                ptf++;

                fSeenContent = TRUE;
            }
            break;
        case CTreePos::Text:
            if( ptpWalk->Cch() && pelPendingForce )
            {
                hr = THR( ForceClose( pelPendingForce ) );
                if (hr)
                    goto Cleanup;

                pelPendingForce = NULL;
            }

            hr = THR( SaveTextPos( ptpWalk ) );
            if (hr)
                goto Cleanup;

            fSeenContent = TRUE;

            break;
        case CTreePos::NodeBeg:
            if (!ptpWalk->IsEdgeScope())
                break;

            if(     pelPendingForce 
                &&  !TagProhibitedContainer( ptpWalk->Branch()->Tag(), 
                                             pelPendingForce->Tag() ) )
            {
                hr = THR( ForceClose( pelPendingForce ) );
                if (hr)
                    goto Cleanup;
            }

            pelPendingForce = NULL;

            hr = THR( SaveElement( ptpWalk->Branch()->Element(), FALSE ) );
            if (hr)
                goto Cleanup;

            fSeenContent = TRUE;

            break;
        case CTreePos::NodeEnd:
            if (!ptpWalk->IsEdgeScope())
                break;

            if(     pelPendingForce
                &&  !TagEndContainer( ptpWalk->Branch()->Tag(),
                                      pelPendingForce->Tag() ) )
            {
                hr = THR( ForceClose( pelPendingForce ) );
                if (hr)
                    goto Cleanup;
            }

            pelPendingForce = ptpWalk->Branch()->Element();

            hr = THR( SaveElement( ptpWalk->Branch()->Element(), TRUE ) );
            if (hr)
                goto Cleanup;

            fSeenContent = TRUE;

            break;
        }

        if ( fSuppressingPrettyCRLF && fSeenContent )
        {
            _pswb->ClearFlags(WBF_NO_PRETTY_CRLF);
            fSuppressingPrettyCRLF = FALSE;
        }


        // Handle any LB at this gap, but not if we are at the end
        // and the LB is not to the left
        if( _fLBEndLeft || ptpWalk != ptpEnd )
        {
            CTreePosGap tpgLB( ptpNext, TPG_LEFT );

            DWORD dwBreaks = LineBreakChar( &tpgLB );

            if( dwBreaks )
            {
                // To IE4, this looks like text so
                // write out the end tag
                if( pelPendingForce )
                {
                    hr = THR( ForceClose( pelPendingForce ) );
                    if (hr)
                        goto Cleanup;

                    pelPendingForce = NULL;
                }

                if(     _pswb->TestFlag(WBF_SAVE_PLAINTEXT)
                    &&  dwBreaks & BREAK_BLOCK_BREAK )
                {

                    hr = THR(_pswb->NewLine());
                    if (hr)
                        goto Cleanup;
                }

                // In IE4, anything besides BB and LB would
                // cause us to clear _fPendingNBSP
                if (dwBreaks & ~( BREAK_BLOCK_BREAK | BREAK_LINE_BREAK ))
                    _fPendingNBSP = FALSE;
            }
        } // LB Check

    }

    if ( fSuppressingPrettyCRLF )
    {
        _pswb->ClearFlags(WBF_NO_PRETTY_CRLF);
        fSuppressingPrettyCRLF = FALSE;
    }

    if (!pelPendingForce)
    {
        hr = THR( SaveSelection( TRUE ) );
        if (hr)
            goto Cleanup;

        fSelectionEndSaved = TRUE;
    }

    //
    // If we are symmetrical, then save out all the end tags starting
    // from above the end gap to _pelFragment
    //
     
    if( _fSymmetrical )
    {
        CTreeNode * pNodeCur = _tpgEnd.Branch();

        for( ; pNodeCur->Element() != _pelFragment ; pNodeCur = pNodeCur->Parent() )
        {
            CElement * pElementCur = pNodeCur->Element();

            if(     pelPendingForce
                &&  !TagEndContainer( pElementCur->Tag(),
                                      pelPendingForce->Tag() ) )
            {
                hr = THR( ForceClose( pelPendingForce ) );
                if (hr)
                    goto Cleanup;
            }

            if (!fSelectionEndSaved)
            {
                hr = THR( SaveSelection( TRUE ) );
                if (hr)
                    goto Cleanup;

                fSelectionEndSaved = TRUE;
            }

            pelPendingForce = pElementCur;

            hr = THR( SaveElement( pElementCur, TRUE ) );
            if (hr)
                goto Cleanup;
        }
    }

    //
    // Force the save of any pending close tag.
    //

    if (pelPendingForce)
    {
        hr = THR( ForceClose( pelPendingForce ) );
        if (hr)
            goto Cleanup;

        if (!fSelectionEndSaved)
        {
            hr = THR( SaveSelection( TRUE ) );
            if (hr)
                goto Cleanup;

            fSelectionEndSaved = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CTreeSaver::SaveElement
//
//  Synopsis:   Open and close tags.
//
//---------------------------------------------------------------

HRESULT
CTreeSaver::SaveElement(CElement * pel, BOOL fEnd)
{
    HRESULT    hr;

    if (fEnd)
    {
        if (_fPendingNBSP && pel == _pelLastBlockScope)
        {
            if (!_pswb->TestFlag(WBF_SAVE_PLAINTEXT))
            {
                TCHAR ch = TCHAR(160);          // &nbsp;
                hr = _pswb->Write(&ch, 1);
                if (hr)
                    goto Cleanup;
            }

            _fPendingNBSP = FALSE;
        }
    }
    else
    {
        // BUGBUG: perhaps we should is IsBlockElement()
        if (pel->HasFlag(TAGDESC_BLOCKELEMENT))
        {
            // Remember the last block element which came into scope
            _pelLastBlockScope = pel;

            // We may have to write an nbsp if this block element has
            // break on empty set (the bit will get cleared in the event
            // that we see a non-empty run before closing).
            _fPendingNBSP = _pelLastBlockScope->_fBreakOnEmpty &&
                _pelLastBlockScope->HasFlag(TAGDESC_SAVENBSPIFEMPTY);
        }
    }
    
    hr = pel->Save(_pswb, fEnd);

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CTreeSaver::SaveTextPos
//
//  Synopsis:   Write out the entire text pos.
//
//---------------------------------------------------------------
HRESULT
CTreeSaver::SaveTextPos(CTreePos *ptp)
{
    Assert( ptp->IsText() );

    HRESULT hr = S_OK;
    LONG    cpStart  = ptp->GetCp();
    LONG    cchTotal = ptp->Cch();

    Assert( cpStart >= 0 );
    Assert( cchTotal >= 0 );

    if( cchTotal > 0 )
    {
        CTxtPtr tp(_pMarkup, cpStart );

        DWORD dwOldFlags;
        dwOldFlags = _pswb->SetFlags(WBF_NO_DQ_ENTITY); // IE5 bug 26812
        
        while (cchTotal > 0)
        {
            long            cch;
            const TCHAR *   pch = tp.GetPch(cch);  // sets cch

            Assert( cch > 0 );

            cch = min(cchTotal, cch);

            hr = _pswb->Write(pch, cch);
            if (hr)
                goto Cleanup;

            cchTotal -= cch;
            tp.AdvanceCp(cch);
            Assert( cch );

            Assert( cchTotal >= 0 );
        }

        _fPendingNBSP = FALSE;
        
Cleanup:

        _pswb->RestoreFlags(dwOldFlags);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Helper:     CTreeSaver::ForceClose
//
//  Synopsis:   Writes a close tag for the specified element if
//              the tag can have an end and it hasn't already been
//              written.
//
//---------------------------------------------------------------

HRESULT
CTreeSaver::ForceClose ( CElement * pel )
{
    HRESULT hr = S_OK;

    if (!pel->_fExplicitEndTag)
    {
        BOOL fHasNoEndTag = TagHasNoEndTag( pel->Tag() );

        if (pel->Tag() == ETAG_COMMENT && DYNCAST( CCommentElement, pel )->_fAtomic)
            fHasNoEndTag = TRUE;

        if (!fHasNoEndTag && !pel->HasFlag( TAGDESC_SAVEALWAYSEND ))
            hr = pel->WriteTag(_pswb, TRUE, TRUE);
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  IE4 Compat helpers
//
//---------------------------------------------------------------

// simulates the IE4 CTxtEdit::IsElementBlockInContext
BOOL 
CTreeSaver::IsElementBlockInContainer( CElement * pElement )
{
    Assert( pElement );

    if (pElement->IsBlockElement())
        return TRUE;

    if (pElement->Tag() == ETAG_ROOT)
        return TRUE;

    if (    pElement->HasFlowLayout()
        &&  pElement->GetFlowLayout()->GetContentMarkup() == _pMarkup )
        return TRUE;

    // BUGBUG: what about text slaves?  Should we see if pElementContainer
    // has a flow layout and compare against it's content element?
    if( pElement == _pelContainer )
        return TRUE;
    
    return FALSE;
}

DWORD
CTreeSaver::LineBreakChar( CTreePosGap * ptpg )
{
    DWORD      dwBreaks = BREAK_NONE;
    CTreePos * ptpRight = ptpg->AdjacentTreePos( TPG_RIGHT );
    CTreePos * ptpLeft = ptpg->AdjacentTreePos( TPG_LEFT );
    CElement * pElement;

    // If we have an end node to the right and 
    // we don't have an end non edge to the left, then
    // we are at the beginning of an inclusion and
    // we go with it...
    if (    ptpRight->IsEndNode() 
        &&  !(  ptpLeft->IsEndNode() 
            &&  !ptpLeft->IsEdgeScope() ))
    {
        while( ! ptpRight->IsEndElementScope() )
            ptpRight = ptpRight->NextTreePos();
    }
    else if (!ptpRight->IsBeginElementScope())
        return dwBreaks;

    pElement = ptpRight->Branch()->Element();

    if( ptpRight->IsEndElementScope() )
    {
        // [TSE] at the end of text sites
        // [BB] at the end of block elements
        IGNORE_HR( _breaker.QueryBreaks( ptpg, & dwBreaks ) );

        // no scope elements (BR, IMG, etc.)
        if(     pElement->IsNoScope()
            ||  pElement->Tag() == ETAG_SELECT
            ||  pElement->Tag() == ETAG_OPTION)
        {
            switch( pElement->Tag() )
            {
            case ETAG_BR:
                dwBreaks |= BREAK_LINE_BREAK;
                break;
            case ETAG_WBR:
                dwBreaks |= BREAK_WORD_BREAK;
                break;
            default:
                // BUGBUG: (jbeda) WCH_EMBEDDING/WCH_NOSCOPE?
                dwBreaks |= BREAK_NOSCOPE;
                break;
            }
        }
    }
    else if( ptpRight->IsBeginElementScope() )
    {
        // [TSB] when a new text site comes into scope
        // [BB] when a new block element comes into scope
        IGNORE_HR( _breaker.QueryBreaks( ptpg, & dwBreaks ) );
    }

    return dwBreaks;
}

// simulates the IE4 CElementRuns::ScopesLeft
BOOL 
CTreeSaver::ScopesLeftOfStart( CElement * pel )
{
    CTreePosGap     tpgCur( TPG_LEFT );

    Verify( ! tpgCur.MoveTo( &_tpgStart ) );

    Assert( tpgCur.Branch()->SearchBranchToRootForScope( pel ) );

    // If there is a LB to the left, then it
    // must scope
    if (_fLBStartLeft)
        return FALSE;

    while(  !(  tpgCur.AttachedTreePos()->IsText() 
            &&  tpgCur.AttachedTreePos()->Cch() ) )
    {
        // Check if we are about to cross that elements boundry
        if (tpgCur.AttachedTreePos()->IsBeginElementScope(pel))
            return FALSE;

        // never cross container boundries
        if(     tpgCur.AttachedTreePos()->IsNode()
            &&  tpgCur.AttachedTreePos()->IsEdgeScope()
            &&  tpgCur.AttachedTreePos()->Branch()->IsContainer() )
            return TRUE;

        // Move the gap left
        Verify( ! tpgCur.MoveLeft() );

        // Check for line break here
        if (LineBreakChar( &tpgCur ))
            break;
    }

    return TRUE;
}

// simulates the IE4 CElementRuns::ScopesRight
BOOL 
CTreeSaver::ScopesRightOfEnd( CElement * pel )
{
    CTreePosGap     tpgCur( TPG_RIGHT );
    BOOL            fLBLeft = _fLBEndLeft;

    Verify( ! tpgCur.MoveTo( &_tpgEnd ) );

    Assert( tpgCur.Branch()->SearchBranchToRootForScope( pel ) );

    while(  !(  tpgCur.AttachedTreePos()->IsText() 
            &&  tpgCur.AttachedTreePos()->Cch() ) )
    {
        // Check LB to our right
        if(     !fLBLeft 
            &&  LineBreakChar( &tpgCur ) )
            return TRUE;

        // Check edge of pel to our right
        if( tpgCur.AttachedTreePos()->IsEndElementScope(pel) )
            return FALSE;

        // never cross container boundries
        if(     tpgCur.AttachedTreePos()->IsNode()
            &&  tpgCur.AttachedTreePos()->IsEdgeScope()
            &&  tpgCur.AttachedTreePos()->Branch()->IsContainer() )
            return TRUE;

        fLBLeft = FALSE;

        Verify( ! tpgCur.MoveRight() );
    }

    return TRUE;
}

