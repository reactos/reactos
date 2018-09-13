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

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_EPARA_HXX_
#define X_EPARA_HXX_
#include "epara.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif


// Just to save a little typing. _doc.h doesn't include the definition
// for CTxtEdit, so this can't be inline.
#define DEFBLOCKTAG (Doc()->GetDefaultBlockTag())

#pragma warning(disable:4706) /* assignment within conditional expression */

#ifdef WIN16
#pragma code_seg ("DOC22_TEXT")
#endif

//+------------------------------------------------------------------------
//
//  Memeber:    GetRunOwnerBranch
//
//  Synopsis:   Locates the highest CSite which owns runs but also
//              occurs beneath pSitecontext
//
//  Arguments:  brElement    - The node from which to begin searching
//              pSiteContext - The context of the owner; that is,
//                             when not NULL, return the highest run
//                             owner in the tree beneath the passed
//                             site. Passing NULL returns the lowest
//                             site in the tree which owns the run
//
//  Returns:    branch which owns the passed iRun or the passed brElement
//              if pSiteContext is the only owner in the chain
//
//-------------------------------------------------------------------------

CTreeNode *
CMarkup::GetRunOwnerBranch(
    CTreeNode * pNode,
    CLayout *   pLayoutContext )
{
    CTreeNode * pNodeLayoutOwnerBranch;
    CTreeNode * pNodeLayoutBranch;
    CElement  * pElementLytCtx = pLayoutContext ? pLayoutContext->ElementOwner() : NULL;

    Assert( pNode );
    Assert( pNode->GetUpdatedNearestLayout() );
    Assert( !pElementLytCtx || pElementLytCtx->IsRunOwner() );
    
    pNodeLayoutBranch      = pNode->GetUpdatedNearestLayoutNode();
    pNodeLayoutOwnerBranch = NULL;

    do
    {
        CLayout * pLayout;
        
        // set this to the new branch every step
        
        pLayout = pNodeLayoutBranch->GetUpdatedLayout();

        // if we hit the context get out
        
        if (SameScope( pNodeLayoutBranch, pElementLytCtx ))
        {
            // if we don't have an owner yet the context
            // branch is returned

            if(!pNodeLayoutOwnerBranch)
                pNodeLayoutOwnerBranch = pNodeLayoutBranch;

            break;
        }

        if (pNodeLayoutBranch->Element()->IsRunOwner())
        {
            pNodeLayoutOwnerBranch = pNodeLayoutBranch;

            if (!pElementLytCtx)
                break;
        }

        pNodeLayoutBranch = pNodeLayoutBranch->GetUpdatedParentLayoutNode();
    }
    while ( pNodeLayoutBranch );

#if DBG == 1
    if (pNodeLayoutBranch)
    {
        Assert( pNodeLayoutOwnerBranch );
        Assert( pNodeLayoutOwnerBranch->Element()->IsRunOwner() );
    }
#endif

    return (pNodeLayoutBranch
                ? pNodeLayoutOwnerBranch
                : NULL);
}

//+----------------------------------------------------------------------------
//
//  Member:     ClearRunCaches
//
//  Synopsis:   This method invalidates cached information associated with a
//              range of runs.
//
//-----------------------------------------------------------------------------

//
// This function is not optimized at all.  It just runs around
// and does the same thing as the function above
//

HRESULT
CMarkup::ClearCaches ( CTreePos * ptpStart, CTreePos * ptpFinish )
{
    CTreePos * ptpCurr, *ptpAfterFinish = ptpFinish->NextTreePos();

    for(ptpCurr = ptpStart;
        ptpCurr != ptpAfterFinish;
        ptpCurr = ptpCurr->NextTreePos())
    {
        if(ptpCurr->IsBeginNode())
        {
            ptpCurr->Branch()->VoidCachedInfo();

            if (ptpCurr->IsEdgeScope())
            {
                CElement * pElementCur = ptpCurr->Branch()->Element();

                // Clear caches on the slave markup
                if (pElementCur->HasSlaveMarkupPtr())
                {
                    CTreePos    *ptpStartSlave, *ptpFinishSlave;
                    CMarkup     *pMarkupSlave = pElementCur->GetSlaveMarkupPtr();

                    pMarkupSlave->GetContentTreeExtent(&ptpStartSlave, &ptpFinishSlave);
                    pMarkupSlave->ClearCaches(ptpStartSlave, ptpFinishSlave);
                }
            }
        }
    }

    return S_OK;
}

HRESULT
CMarkup::ClearRunCaches (DWORD dwFlags, CElement *pElement)
{
    CTreePos * ptpStart = NULL;
    CTreePos * ptpEnd;
    BOOL       fClearAllFormats = dwFlags & ELEMCHNG_CLEARCACHES;

    Assert(pElement);
    pElement->GetTreeExtent( & ptpStart, & ptpEnd );

    if (ptpStart)
    {
        CTreePos *ptpAfterFinish = ptpEnd->NextTreePos();

        for( ; ptpStart != ptpAfterFinish; ptpStart = ptpStart->NextTreePos())
        {
            if(ptpStart->IsBeginNode())
            {
                CTreeNode * pNodeCur     = ptpStart->Branch();
                CElement  * pElementCur  = pNodeCur->Element();
                BOOL        fNotifyFormatChange = FALSE;

                if(fClearAllFormats)
                {
                    // clear the formats on the node
                    pNodeCur->VoidCachedInfo();
                    fNotifyFormatChange = TRUE;
                }
                else if (pElement == pElementCur || pElementCur->_fInheritFF)
                {
                    pNodeCur->VoidFancyFormat();
                    fNotifyFormatChange = pElement == pElementCur;
                }

                // if the node comming into scope is a new element
                // notify the element of a format cache change.
                if (fNotifyFormatChange && ptpStart->IsEdgeScope())
                {
                    CLayout * pLayout = pElementCur->GetLayoutPtr();

                    if (pLayout)
                    {
                        pLayout->OnFormatsChange(dwFlags);
                    }

                    // Clear caches on the slave markup
                    if (pElementCur->HasSlaveMarkupPtr())
                    {
                        CMarkup * pMarkupSlave = pElementCur->GetSlaveMarkupPtr();
                        pMarkupSlave->ClearRunCaches(dwFlags, pMarkupSlave->Root());
                    }
                }
            }
        }
    }
    else if (pElement->GetFirstBranch())
    {
        // this could happen, when element's are temporarily parented to the
        // rootsite.
        pElement->GetFirstBranch()->VoidCachedInfo();
    }
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     RunsAffected
//
//  Synopsis:   Adds a change event to the CTextChanges object which
//              describes the fact that the chars between two runs have
//              been affected (their tree context changed).
//
//-------------------------------------------------------------------------


HRESULT
CMarkup::RangeAffected ( CTreePos *ptpLeft, CTreePos *ptpRight )
{
    HRESULT         hr;
    CNotification   nf;
    long            cp, cch;
    CTreeNode *     pNodeNotify = NULL;
    CTreePos *      ptpStart = ptpLeft, *ptpAfterLast;

    // clear all of the caches
    hr = THR( ClearCaches( ptpLeft, ptpRight ) );
    if (hr)
        goto Cleanup;

    Assert( ptpLeft && ptpRight );
    ptpAfterLast = ptpRight->NextTreePos();

    // Send the CharsResize notification
    cp  = ptpStart->GetCp();
    cch = 0;

    Assert( cp >= 0 );

    // Note: notifications for the WCH_NODE characters go
    // to intersting places.  For WCH_NODE characters inside
    // of an inclusion, the notifations go to nodes at the
    // bottom of the inclusion.  Also, for edges, notifications
    // go to the parent of the element, not the element itself.
    // This way, noscope elements to not get any notifications
    // in this loop.

    while( ptpStart != ptpAfterLast )
    {
        if( ptpStart->IsNode() )
        {
            // if we are entering an inclusion
            // then remember the first node as the
            // one to send the left half of 
            // the notification to
            if(     ! pNodeNotify 
                &&  ptpStart->IsEndNode() 
                &&  ! ptpStart->IsEdgeScope() )
            {
                pNodeNotify = ptpStart->Branch();
            }

            if( ptpStart->IsBeginNode() )
            {
                cch++;
            }

            // send a notification if we hit the edge
            // of a layout
            if(     cch 
                &&  ptpStart->IsEdgeScope() 
                &&  ptpStart->Branch()->HasLayout() )
            {
                if( ! pNodeNotify )
                    pNodeNotify = ptpStart->IsBeginNode() 
                        ? ptpStart->Branch()->Parent()
                        : ptpStart->Branch();

                Assert( !pNodeNotify->Element()->IsNoScope() );

                nf.CharsResize( cp, cch, pNodeNotify );
                Notify( nf );

                cp += cch;
                cch = 0;
            }
            
            // If we hit an edge, clear pNodeNotify.  Either we sent
            // a notification or we didn't. Either way, we are done
            // with this inclusion so we don't have to remember pNodeNotify
            if( ptpStart->IsEdgeScope() )
            {
                pNodeNotify = NULL;
            }

            if( ptpStart->IsEndNode() )
                cch++;
        }
        else if( ptpStart->IsText() )
        {
            cch += ptpStart->Cch();
        }

        ptpStart = ptpStart->NextTreePos();
    }

    // Finish off any notification left over
    if( cch )
    {
        if( !pNodeNotify )
        {
            CTreePosGap tpg( ptpStart, TPG_LEFT );
            pNodeNotify = tpg.Branch();
        }

        nf.CharsResize( cp, cch, pNodeNotify );
        Notify( nf );
    }

Cleanup:
    RRETURN( hr );
}

#if 0
This routine is not currently needed and may not even be correct!
//+------------------------------------------------------------------------
//
//  Member:     EnsureSidAfterCharsInserted
//
//  Arguments:  pNodeNotify     - the node to send changes to
//              ptpText         - the text pos to examine
//              ich             - the character to start with
//              cch             - the count of characters added
//
//  Synopsis:   Runs through the chracters added and splits off
//              new runs for new script IDs as needed
//
//  Returns:    S_OK if successful
//
//-------------------------------------------------------------------------
HRESULT CMarkup::EnsureSidAfterCharsInserted(
    CTreeNode * pNodeNotify,
    CTreePos *  ptpText,
    long        ichStart,
    long        cch )
{
    Assert( ptpText && ptpText->IsText() );
    Assert( cch > 0 && cch <= ptpText->Cch() );
    Assert( ichStart < ptpText->Cch() );

    HRESULT         hr = S_OK;
    CMarkupUndo     mu( this );
    CTxtPtr         tp( this, ptpText->GetCp() + ichStart );
    long            ichCurr = ichStart;
    long            ichLastAdded = ichStart + cch - 1;
    long            ichLastInRun = ptpText->Cch() - 1;
    CTreePos *      ptpCurr = ptpText;
    long            sidCurr = ptpText->Sid();
    long            sidOrig = sidCurr;
    TCHAR           chCurr = tp.GetChar();
    BOOL            fFirst = TRUE;

    //
    // Iterate through all of the characters added
    //

    for( ; ichCurr <= ichLastAdded; ichCurr++ )
    {
        long    sidNew;

        if( ! fFirst )
            chCurr = tp.NextChar();

        fFirst = FALSE;

        sidNew = ScriptIDFromCh( chCurr );

        // BUGBUG (t-johnh): This can be a LOT smarter about when
        // to split, as right now, we get excessive fragmentation.
        // This should be using AreDifferentSids, which currently
        // sits in doc.cxx to see when different sids can coexist.
        // HOWEVER, it also needs to be smarter about sidCurr - 
        // sidCurr should represent the sid of the combination of
        // all characters previous in the text pos.

        sidNew = FoldScriptIDs( sidCurr, sidNew );

        if( sidCurr != sidNew )
        {
            if( ichCurr )
            {
                //
                // Split the run at this ich
                //

                mu.TextPosSplit( ptpCurr );

                hr = THR( Split( ptpCurr, ichCurr, sidNew ) );
                if (hr)
                    goto Cleanup;

                Doc()->InvalidateTreeCache();

                // I'm turning this off as it can break linebreaking.
#if NEVER
                // BUGBUG: this is dangerous as it plays with the run
                // before the run being examined.  The calling code must
                // be aware that the run that it passed in may be invalid
                // after the call! -- Do we really want to do this merging?

                // If this is the fist time we've split, and
                // the new characters start at the beginning of the run, 
                // try to join the left run (with the new characters) with
                // the previous run
                if( nRunsAdded == 0 && ichStart == 0)
                {
                    CTreePos *ptpPrev = ptpCurr->PreviousTreePos();

                    if( ptpPrev->IsText() && ptpPrev->Sid() == ptpCurr->Sid() )
                    {
                        Assert( long(tp.GetCp()) - ichCurr == ptpPrev->GetCp() + ptpPrev->Cch() );
                        nf.RunsJoined( tp.GetCp() - ichCurr, ptpPrev->Index(), ptpPrev->Cch(), pNodeNotify );
                        mu.TextPosJoined( ptpPrev );

                        hr = THR( Join( ptpPrev ) );
                        if (hr)
                            goto Cleanup;

                        nRunsAdded --;

                        Doc()->InvalidateTreeCache();

                        Notify( nf );

                        ptpCurr = ptpPrev;
                    }

                }
#endif

                // We are now examining the added pos
                ptpCurr = ptpCurr->NextTreePos();
                Assert( ptpCurr && ptpCurr->IsText() );

                // Update all of ich's
                ichLastAdded -= ichCurr;
                ichLastInRun -= ichCurr;
                ichCurr = 0;

            }
            else
            {
                // Set the sid for the current run since
                // the run wasn't split, this didn't get
                // set that way.
                mu.TextPosSidChanged( ptpCurr );
                ptpCurr->DataThis()->_sid = sidNew;
            }
        }
        
        //
        // Set up for the next loop
        //

        sidCurr = sidNew;
    }

    //
    // The characters were inserted into the middle
    // of the run.  Split after those characters and give
    // the run the original sid.
    // NOTE: we may want to reexamine this later as if everything
    // left is neutral, we don't want to split again.
    //
    Assert( sidCurr == ptpCurr->Sid() );
    if( sidOrig != sidCurr && ichCurr && ichCurr <= ichLastInRun )
    {
        // Advance to catch up with the ich
        tp.AdvanceCp(1);

        mu.TextPosSplit( ptpCurr );

        hr = THR( Split( ptpCurr, ichCurr, sidOrig ) );
        if (hr)
            goto Cleanup;

        Doc()->InvalidateTreeCache();

#if NEVER
        // Same comment as above
        if( nRunsAdded == 0 && ichStart == 0)
        {
            CTreePos *ptpPrev = ptpCurr->PreviousTreePos();

            if( ptpPrev->IsText() && ptpPrev->Sid() == ptpCurr->Sid() )
            {
                Assert( long(tp.GetCp()) - ichCurr == ptpPrev->GetCp() + ptpPrev->Cch() );
                nf.RunsJoined( tp.GetCp() - ichCurr, ptpPrev->Index(), ptpPrev->Cch(), pNodeNotify );
                mu.TextPosJoined( ptpPrev );

                hr = THR( Join( ptpPrev ) );
                if (hr)
                    goto Cleanup;

                nRunsAdded --;

                Doc()->InvalidateTreeCache();

                Notify( nf );

                ptpCurr = ptpPrev;
            }
        }
#endif
    }

Cleanup:
    RRETURN(hr);
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     SetTextID
//
//  Arguments:  ptpgStart   - where to start setting
//              ptpgEnd     - where to stop setting
//
//  Synopsis:   Gives a unique textID to every chunk of text in the given
//              range.
//
//  Returns:    S_OK if successful
//
//-------------------------------------------------------------------------
HRESULT 
CMarkup::SetTextID(
    CTreePosGap *   ptpgStart,
    CTreePosGap *   ptpgEnd,
    long *plNewTextID )
{
    HRESULT hr = S_OK;
    long lTxtID;

    Assert( ! HasUnembeddedPointers() );

    EnsureTotalOrder( ptpgStart, ptpgEnd );

    Assert( ptpgStart && ptpgStart->IsPositioned() && ptpgStart->GetAttachedMarkup() == this );
    Assert( ptpgEnd && ptpgEnd->IsPositioned() && ptpgEnd->GetAttachedMarkup() == this );

    CTreePos *  ptpFirst, * ptpCurr, *ptpStop;
    CDoc *      pDoc = Doc();

    if ( !plNewTextID )
        plNewTextID = &lTxtID;

    *plNewTextID = 0;

    ptpFirst = ptpgStart->AdjacentTreePos( TPG_LEFT );
    ptpCurr = ptpFirst;
    ptpStop = ptpgEnd->AdjacentTreePos( TPG_RIGHT );

    ptpgStart->UnPosition();
    ptpgEnd->UnPosition();

    SplitTextID( ptpCurr, ptpStop );

    ptpCurr = ptpCurr->NextTreePos();

    while( ptpCurr != ptpStop )
    {
        Assert( ptpCurr );

        if( ptpCurr->IsNode() )
        {
            *plNewTextID = 0;
        }

        if( ptpCurr->IsText() )
        {
            if( *plNewTextID == 0 )
            {
                *plNewTextID = ++(pDoc->_lLastTextID);
            }

            hr = THR( SetTextPosID( &ptpCurr, *plNewTextID ) );
            if (hr)
                goto Cleanup;

        }

        ptpCurr = ptpCurr->NextTreePos();
    }

    if( !*plNewTextID )
    {
        CTreePos * ptpNew;
        CTreePosGap tpgInsert( ptpCurr, TPG_RIGHT );

        ptpNew = NewTextPos(0, sidDefault, *plNewTextID = ++(pDoc->_lLastTextID));

        hr = THR(Insert(ptpNew, &tpgInsert));
        if(hr)
            goto Cleanup;
    }

    Verify( ! ptpgStart->MoveTo( ptpFirst, TPG_RIGHT ) );
    Verify( ! ptpgEnd->MoveTo( ptpStop, TPG_LEFT ) );

Cleanup:
    RRETURN( hr );
}


//+------------------------------------------------------------------------
//
//  Member:     GetTextID
//
//  Arguments:  ptpg        - where
//
//  Synopsis:   Finds the TextID for any text to the right of the gap
//              passed in.
//
//  Returns:    -1 if no text is to the right
//              0  if text to right has no ID assigned
//              otherwise, the textID to the right
//
//-------------------------------------------------------------------------
long 
CMarkup::GetTextID( 
    CTreePosGap * ptpg )
{
    Assert( ptpg && ptpg->IsPositioned() && ptpg->GetAttachedMarkup() == this );

    CTreePos * ptp = ptpg->AdjacentTreePos( TPG_RIGHT );

    while( ! ptp->IsNode() )
    {
        if( ptp->IsText() )
            return ptp->TextID();

        ptp = ptp->NextTreePos();
        Assert( ptp );
    }

    return -1;
}

//+------------------------------------------------------------------------
//
//  Member:     FindTextID
//
//  Arguments:  lTextID     - IN the id to scan for
//              ptpgStart   - IN/OUT where to start scanning
//              ptpgEnd     - OUT the end of the extent
//
//  Synopsis:   Find the extent of lTextID.  Start searching at ptpgStart.
//              Set ptpgStart to the beginning and ptpgEnd to the end of
//              the extent
//
//  Returns:    S_OK if text ID found
//              S_FALSE if text ID not found
//              error otherwise
//
//-------------------------------------------------------------------------
HRESULT 
CMarkup::FindTextID(
    long            lTextID,
    CTreePosGap *   ptpgStart,
    CTreePosGap *   ptpgEnd )
{
    Assert( ptpgStart && ptpgStart->IsPositioned() && ptpgStart->GetAttachedMarkup() == this );
    Assert( ptpgEnd );

    CTreePos * ptpLeft, *ptpRight, *ptpFound = NULL;

    ptpLeft = ptpgStart->AdjacentTreePos( TPG_LEFT );
    ptpRight = ptpgStart->AdjacentTreePos( TPG_RIGHT );

    //
    // Start from ptpgStart and search both directions at the same time.
    //

    while( ptpLeft || ptpRight )
    {
        if( ptpLeft )
        {
            if( ptpLeft->IsText() && ptpLeft->TextID() == lTextID )
            {
                ptpRight = ptpLeft;

                // Starting at ptpLeft, loop to the left
                // looking for all of consecutive text poses 
                // with TextID of lTextID
                do
                {
                    if( ptpLeft->IsText() )
                    {
                        if( ptpLeft->TextID() == lTextID )
                        {
                            ptpFound = ptpLeft;
                        }
                        else
                        {
                            break;
                        }
                    }
                    ptpLeft = ptpLeft->PreviousTreePos();
                }
                while( !ptpLeft->IsNode() );

                ptpLeft = ptpFound;

                break;
            }

            ptpLeft = ptpLeft->PreviousTreePos();
        }

        if( ptpRight )
        {
            if( ptpRight->IsText() && ptpRight->TextID() == lTextID )
            {
                ptpLeft = ptpRight;

                // Starting at ptpRight, loop to the right
                // looking for all of consecutive text poses 
                // with TextID of lTextID
                do
                {
                    if( ptpRight->IsText() )
                    {
                        if( ptpRight->TextID() == lTextID )
                        {
                            ptpFound = ptpRight;
                        }
                        else
                        {
                            break;
                        }
                    }
                    ptpRight = ptpRight->NextTreePos();
                }
                while( !ptpRight->IsNode() );

                ptpRight = ptpFound;

                break;
            }
            ptpRight = ptpRight->NextTreePos();
        }
    }

    if( ptpFound )
    {
        Verify( !ptpgStart->MoveTo( ptpLeft, TPG_LEFT ) );
        Verify( !ptpgEnd->MoveTo( ptpRight, TPG_RIGHT ) );

        return S_OK;
    }

    return S_FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     SplitTextID
//
//  Arguments:  ptpLeft     - The left side of the split
//              ptpRight    - The right side to split
//
//  Synopsis:   If text to the left of ptpLeft has the same ID
//              as the text to the right of ptpRight, give the
//              fragment after ptpRight a new ID
//
//-------------------------------------------------------------------------
void
CMarkup::SplitTextID(
    CTreePos *   ptpLeft,
    CTreePos *   ptpRight )
{
    Assert( ptpLeft && ptpRight );

    Assert( ! HasUnembeddedPointers() );

    //
    // Find a text pos to the left if any
    //
    
    while ( ptpLeft )
    {
        if( ptpLeft->IsNode() )
        {
            ptpLeft = NULL;
            break;
        }

        if( ptpLeft->IsText() )
        {
            break;
        }

        ptpLeft = ptpLeft->PreviousTreePos();
    }

    //
    // Find one to the right
    //
    
    while ( ptpRight )
    {
        if( ptpRight->IsNode() )
        {
            ptpRight = NULL;
            break;
        }

        if( ptpRight->IsText() )
        {
            break;
        }

        ptpRight = ptpRight->NextTreePos();
    }

    //
    // If we have one to the left and right and they
    // both have the same ID (that isn't 0) we want
    // to give the fragment to the right a new ID.
    //
    
    if(     ptpLeft 
        &&  ptpRight 
        &&  ptpRight->TextID()
        &&  ptpLeft->TextID() == ptpRight->TextID() )
    {
        long lCurrTextID = ptpRight->TextID();
        long lNewTextID = ++(Doc()->_lLastTextID);

        while( ptpRight && !ptpRight->IsNode() )
        {
            if( ptpRight->IsText() )
            {
                if( ptpRight->TextID() == lCurrTextID )
                {
                    WHEN_DBG( CTreePos * ptpOld = ptpRight );
                    Verify( ! SetTextPosID( &ptpRight, lNewTextID ) );
                    Assert( ptpOld == ptpRight );
                }
                else
                {
                    break;
                }
            }

            ptpRight = ptpRight->NextTreePos();
        }
    }
}
