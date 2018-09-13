//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       rel.cxx
//
//  Contents:   Implementation of some code for relatively positioned lines.
//
//  Classes:    CDisplay
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_RCLCLPTR_HXX_
#define X_RCLCLPTR_HXX_
#include "rclclptr.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif


#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
#endif

DeclareTag(tagRelPositioning, "Relative Positioning", "Relative Positioning");

MtDefine(CDisplayDrawRelElemBgAndBorder_aryNodesWithBgOrBorder_pv, Locals, "CDisplay::DrawRelElemBgAndBorder aryNodesWithBgOrBorder::_pv")
MtDefine(CDisplayDrawRelElemBgAndBorder_aryRects_pv, Locals, "CDisplay::DrawRelElemBgAndBorder aryRects::_pv")
MtDefine(CDisplayAddRelDispNodes_aryRelDispNodeCtxs_pv, Locals, "CDisplay::AddRelDispNodes aryRelDispNodeCtxs::_pv")
MtDefine(CDisplayUpdateRelDispNodeCache_rdnc_pv, Locals, "CDisplay::UpdateRelDispNodeCache rdnc::_pv")

extern CDispNode * EnsureContentNode(CDispNode * pDispNode);

void CDisplay::RcFromLine(RECT *prc, LONG top, LONG xLead, CLine *pli)
{
    // If it is the first fragment in a line with relative chunks include
    // the margin space otherwise use the text left.
    if(!_fRTL)
    {
        prc->left   = xLead + ((pli->_fFirstFragInLine || pli->_fHasBulletOrNum)
                                ? pli->_xLeftMargin
                                : pli->GetTextLeft());

        prc->right  = xLead + pli->_xLeftMargin + pli->_xLineWidth
                        + (pli->_fForceNewLine ? pli->_xWhite : 0);
    }
    else
    {
        prc->right  = xLead - ((pli->_fFirstFragInLine || pli->_fHasBulletOrNum)
                                ? pli->_xRightMargin
                                : pli->GetRTLTextRight());

        prc->left   = xLead - (pli->_xRightMargin + pli->_xLineWidth
                        + (pli->_fForceNewLine ? pli->_xWhite : 0));
    }

    // If the line has negative margin, we need to use the larger
    // rect because text goes into the negative margin space.
    prc->top    = top + pli->GetYTop();
    prc->bottom = top + max(pli->GetYHeight(), pli->GetYBottom());
}

void
CDisplay::VoidRelDispNodeCache()
{
    if(HasRelDispNodeCache())
    {
        CRelDispNodeCache * prdnc = GetRelDispNodeCache();

        // BUGBUG (donmarsh) - use this condition after MichaelW's filter work to enable CDispRoot::DestroyTreeWithPrejudice
        //if (GetFlowLayout()->GetView()->IsInState(CView::VS_OPEN))
        {
            prdnc->DestroyDispNodes();
        }

        delete DelRelDispNodeCache();
    }
}

CRelDispNodeCache *
CDisplay::GetRelDispNodeCache() const
{
    CDoc * pDoc = GetFlowLayout()->Doc();
#if DBG == 1
    if(HasRelDispNodeCache())
    {
        void * pLookasidePtr =  pDoc->GetLookasidePtr((DWORD *)this);

        Assert(pLookasidePtr == _pRelDispNodeCache);

        return (CRelDispNodeCache *)pLookasidePtr;
    }
    else
        return NULL;
#else
    return (CRelDispNodeCache *)(HasRelDispNodeCache() ? pDoc->GetLookasidePtr((DWORD *)this) : NULL);
#endif
}

HRESULT
CDisplay::SetRelDispNodeCache(void * pvVal)
{
    HRESULT hr = THR(GetFlowLayout()->Doc()->SetLookasidePtr((DWORD *)this, pvVal));

    if (hr == S_OK)
    {
        _fHasRelDispNodeCache = 1;
#if DBG == 1
        Assert(!_pRelDispNodeCache);

        _pRelDispNodeCache = (CRelDispNodeCache *)pvVal;
#endif
    }

    RRETURN(hr);
}

CRelDispNodeCache *
CDisplay::DelRelDispNodeCache()
{
    if (HasRelDispNodeCache())
    {
        void * pvVal = GetFlowLayout()->Doc()->DelLookasidePtr((DWORD *)this);
        _fHasRelDispNodeCache = 0;
#if DBG == 1
        Assert(_pRelDispNodeCache == pvVal);

        _pRelDispNodeCache = NULL;

        TraceTag((tagRelPositioning, "Deleting RelDispNodeCache - Element(Tag: %ls, SN:%ld)",
                                        GetFlowLayoutElement()->TagName(),
                                        GetFlowLayoutElement()->SN()));
#endif
        return(CRelDispNodeCache *)pvVal;
    }

    return(NULL);
}


CRelDispNodeCache *
CDisplay::EnsureRelDispNodeCache()
{
    Assert(!HasRelDispNodeCache());

    CRelDispNodeCache * prdnc = new CRelDispNodeCache(this);

    if(prdnc)
    {
        TraceTag((tagRelPositioning, "Creating RelDispNodeCache - %x", prdnc));
        SetRelDispNodeCache(prdnc);
    }

    return prdnc;
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::UpdateDispNodeCache
//
// Synopsis:    Update the relative line cache smartly using the line edit
//              descriptor
//
//-----------------------------------------------------------------------------

void
CDisplay::UpdateRelDispNodeCache(CLed * pled)
{
    CRelDispNodeCache * prdnc = GetRelDispNodeCache();
    CStackDataAry<CRelDispNode, 4>  rdnc(Mt(CDisplayUpdateRelDispNodeCache_rdnc_pv));
    CLed                led;
    long                iliMatchNew;
    long                dy;
    long                dili;

    TraceTag((tagRelPositioning, "Entering: CDisplay::UpdateRelDispNodeCache - Element(Tag: %ls, SN:%ld)",
                                    GetFlowLayoutElement()->TagName(),
                                    GetFlowLayoutElement()->SN()));

    if(!pled)
    {
        pled = &led;
        pled->_yFirst = pled->_iliFirst = 0;
        pled->_cpFirst = GetFlowLayout()->GetContentFirstCp();
        pled->SetNoMatch();
    }

    dy   = pled->_yMatchNew - pled->_yMatchOld;
    dili = pled->_iliMatchNew - pled->_iliMatchOld;

    iliMatchNew = pled->_iliMatchNew == MAXLONG ? LineCount() : pled->_iliMatchNew;

    {
        CRelDispNode      * prdn = NULL;
        long                lSize;
        long                iEntry, iEntryStart, iEntryFinish;
        long                iliFirst   = pled->_iliFirst;
        long                yFirst     = pled->_yFirst;
        long                cpFirst    = pled->_cpFirst;
        long                cNewEntries;

        lSize = prdnc ? prdnc->Size() : 0;

        if(prdnc)
        {
            // find the node that corresponds to iliFirst
            for ( iEntryStart = 0, prdn = (*prdnc)[0];
                  iEntryStart < lSize && prdn->_ili + prdn->_cLines <= iliFirst;
                  iEntryStart++, prdn++);
        }
        else
        {
            iEntryStart = 0;
        }

        iEntryFinish = -1;

        // find the last entry affected by the lines changed
        if ( iEntryStart < lSize)
        {
            // if the region affected starts in the middle of the
            // disp node, then update the affected range to include
            // the entire dispnode.
            if(prdn->_ili < iliFirst)
            {
                iliFirst = prdn->_ili;
                yFirst  = prdn->_yli;
                cpFirst = prdn->GetElement()->GetFirstCp() - 1;
            }

            if (iliMatchNew != LineCount())
            {
                for(iEntryFinish = iEntryStart;
                    iEntryFinish < lSize && prdn->_ili < pled->_iliMatchOld;
                    iEntryFinish++, prdn++);
            }
            else
            {
                iEntryFinish = lSize;
            }

        }

        //
        // Add the new entries to temporary stack
        //
        AddRelNodesToCache(cpFirst, yFirst, iliFirst, iliMatchNew, &rdnc);
        cNewEntries = rdnc.Size();


        //
        // Destroy the display nodes that correspond to the entries
        // in the dirty range
        //
        if(iEntryStart < lSize)
        {
            long i, iNewEntry = 0;
            long diEntries;

            prdn = (*prdnc)[iEntryStart];
            // now remove all the entries that are affected
            for ( iEntry = iEntryStart; iEntry < iEntryFinish; iEntry++, prdn++)
            {
                i = iNewEntry;

                if(cNewEntries && prdn->_pDispNode->IsContainer())
                {
                    //
                    // Replace old disp containers with the new ones, to ensure that all
                    // children are properly parented to the new container.
                    //
                    for( ; i < cNewEntries; i++)
                    {
                        CRelDispNode * prdnNewEntry = &rdnc.Item(i);

                        //
                        // Do not dereference, prdn->_pElement, it might have been destroyed.
                        //
                        if(     prdn->GetElement()    
                            &&  prdnNewEntry->GetElement() == prdn->GetElement()
                            &&  prdnNewEntry->_pDispNode->IsContainer())
                        {
                            CDispNode * pDispNode;
                            CSize       size;

                            // start at the next entry, so that we can start
                            // the search for the next container after the
                            // current entry. Both the caches are in source order
                            iNewEntry = i + 1;

                            prdn->_pDispNode->ExtractFromTree();

                            TraceTag((tagRelPositioning, "\tReplacing dispnode for element(Tag: %ls, SN:%ld) from %ld",
                                        prdnNewEntry->GetElement()->TagName(),
                                        prdnNewEntry->GetElement()->SN(),
                                        iEntry));

                            prdnNewEntry->_pDispNode->GetSize(&size);
                            prdnNewEntry->_pDispNode->ReplaceNode(prdn->_pDispNode);
                            prdnNewEntry->_pDispNode->SetSize(size, FALSE);

                            for (pDispNode = DYNCAST(CDispInteriorNode, prdnNewEntry->_pDispNode)->GetFirstChildNodeInLayer(DISPNODELAYER_FLOW);
                                pDispNode;
                                pDispNode = pDispNode->GetNextSiblingNode(TRUE))
                            {
                                if (pDispNode->IsOwned())
                                {
                                    CElement *  pElement;

                                    pDispNode->GetDispClient()->GetOwner(pDispNode, (void **)&pElement);

                                    if (    pElement
                                        &&  pElement->NeedsLayout())
                                    {
                                        pElement->ZChangeElement();
                                    }
                                }
                            }
                            break;
                        }
                    }

                    if( i == cNewEntries)
                    {
                        prdn->ClearContents();
                    }
                }
                else
                {
                    prdn->ClearContents();
                }
            }

            diEntries = cNewEntries - iEntryFinish + iEntryStart;

            // move all the disp nodes that follow the
            // affected entries
            if( iEntryFinish != lSize && (dy || dili || diEntries))
            {
                TraceTag((tagRelPositioning, "\tMoving Entries %ld - %ld by %ld",
                    iEntryFinish,
                    lSize,
                    diEntries));

                long iLastLine = -1;

                for (iEntry = iEntryFinish, prdn = (*prdnc)[iEntryFinish];
                     iEntry < lSize;
                     iEntry++, prdn++)
                {
                    prdn->_ili += dili;
                    prdn->_yli += dy;

                    if (iLastLine <= prdn->_ili)
                    {
                        prdn->_pDispNode->SetPosition(prdn->_pDispNode->GetPosition() + CSize(0, dy));
                        iLastLine = prdn->_ili + prdn->_cLines;
                    }

                    prdn->_pDispNode->SetExtraCookie((void *)(LONG_PTR)(iEntry + diEntries));
                }
            }

            if(iEntryStart < iEntryFinish)
            {
                // delete all the old entries in the dirty range
                prdnc->Delete(iEntryStart, iEntryFinish - 1);
            }
        }

        //
        // Insert the new entries in the dirty range, back to the cache
        //
        if(cNewEntries)
        {
            long iEntryInsert = iEntryStart;
            prdnc = GetRelDispNodeCache();

            Assert(prdnc);

            prdn = &rdnc.Item(0);
            for(iEntry = 0; iEntry < cNewEntries; iEntry++, prdn++, iEntryInsert++)
            {
                long xPos = prdn->_ptOffset.x;
                
                CPoint ptAuto(xPos, prdn->_ptOffset.y + prdn->_yli);

                prdnc->InsertAt(iEntryInsert, *prdn);

                prdn->_pDispNode->SetExtraCookie((void *)(LONG_PTR)(iEntryInsert));

                //
                // Ensure flow node for each of the newly created container
                //
                if(prdn->_pDispNode->IsContainer())
                {
                    CDispNode * pDispContent = EnsureContentNode(prdn->_pDispNode);

                    if(pDispContent)
                    {
                        CSize size;
                        prdn->_pDispNode->GetSize(&size);
                        pDispContent->SetSize(size, FALSE);
                    }
                }

                TraceTag((tagRelPositioning, "\tAdding Element(Tag: %ls, SN:%ld) at %ld",
                                prdn->GetElement()->TagName(),
                                prdn->GetElement()->SN(),
                                iEntryInsert));
                //
                // Fire off a ZChange notification to insert it in the appropriate
                // ZLayer and ZParent.
                //
                prdn->GetElement()->ZChangeElement(NULL, &ptAuto);

                Assert(prdnc->Size() > (iEntryInsert));
            }
        }
    }

    prdnc = GetRelDispNodeCache();

    if(prdnc && !prdnc->Size())
        delete DelRelDispNodeCache();

    GetFlowLayout()->_fContainsRelative = HasRelDispNodeCache();
    TraceTag((tagRelPositioning, "\tContainsRelative: %ls", HasRelDispNodeCache ? "TRUE" : "FALSE"));
    TraceTag((tagRelPositioning, "Leaving: CDisplay::UpdateRelDispNodeCache"));
}

struct CRelDispNodeCtx
{
    CElement *  _pElement;
    CRect       _rc;
    long        _cpEnd;
    long        _cLines;
    long        _ili;
    long        _yli;
    long        _iEntry;
    BOOL        _fHasChildren;
};

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::AddDispNodesToCache
//
// Synopsis:    Add new display nodes to the cache in the range of lines changed
//
//-----------------------------------------------------------------------------
void
CDisplay::AddRelNodesToCache(
    long cpFirst,
    long yli,
    long iliStart,
    long iliMatchNew,
    CDataAry<CRelDispNode> * prdnc)
{
    CStackDataAry<CRelDispNodeCtx, 4> aryRelDispNodeCtx(Mt(CDisplayAddRelDispNodes_aryRelDispNodeCtxs_pv));
    CRelDispNodeCtx   * prdnCtx = NULL;
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CMarkup     * pMarkup = pFlowLayout->GetContentMarkup();
    CLine       * pli;
    long          ili;
    long          ich;
    long          iTop = -1;
    long          cpTopEnd = MINLONG;
    long          cpLayoutMax = pFlowLayout->GetContentLastCp();
    long          iEntryInsert = 0;
    long          lCount = LineCount();
    BOOL          fDesignMode = pFlowLayout->ElementOwner()->IsEditable(TRUE);

    //
    // Note: here we are trying to walk lines beyond iliMatchNew to update the
    // rc of the elements that came into scope in the dirty region
    //

    for (ili = iliStart;
         ili < lCount && (ili < iliMatchNew || cpFirst <= cpTopEnd);
         ili++)
    {
        pli = Elem(ili);

        // create a new entry only for elements in the dirty range
        // (ili < iliMatchNew)
        if (pli->_fRelative && pli->_cch && ili < iliMatchNew)
        {
            CTreePos *ptp = pMarkup->TreePosAtCp(cpFirst, &ich);
            CElement *pElementRel = ptp->GetBranch()->Element();

            // if the current line is relative and a new element
            // is comming into scope, then push a new reldispnode
            // context on to the stack.
            if(     ptp->IsBeginElementScope()
                &&  pElementRel->IsRelative()
                &&  !pElementRel->NeedsLayout())
            {
                CRelDispNode rdn;

                if(!GetRelDispNodeCache())
                {
                    if(!EnsureRelDispNodeCache())
                        return;
                }

                prdnc->InsertIndirect(iEntryInsert, &rdn);

                prdnCtx = aryRelDispNodeCtx.Append();

                if(!prdnCtx)
                    return;

                cpTopEnd          = min(pElementRel->GetLastCp(), cpLayoutMax);
                prdnCtx->_pElement = pElementRel;
                prdnCtx->_ili      = ili;
                prdnCtx->_yli      = yli;
                prdnCtx->_cpEnd    = cpTopEnd;
                prdnCtx->_rc       = g_Zero.rc;
                prdnCtx->_iEntry    = iEntryInsert++;

                iTop++;

                Assert(aryRelDispNodeCtx.Size() == iTop + 1);
            }

        }

        // if we have a relative element in context, add the
        // current line to it
        if(iTop >= 0)
        {
            CRect rcLine;

            Assert(prdnCtx);

            prdnCtx->_cLines++;

            RcFromLine(&rcLine, yli, 0, pli);

            if(!IsRectEmpty(&rcLine))
            {
                UnionRect((RECT *)&prdnCtx->_rc, (RECT *)&prdnCtx->_rc, (RECT *)&rcLine);
            }
            else if ( prdnCtx->_cLines == 1 )
            {
                prdnCtx->_rc = rcLine;
            }

            if(pli->IsFrame() || pli->_fHasEmbedOrWbr || pli->_fRelative)
                prdnCtx->_fHasChildren = TRUE;

        }

        cpFirst  += pli->_cch;

        if(pli->_fForceNewLine)
            yli += pli->_yHeight;


        // if the current relative element is going out of scope,
        // then create a disp node for the element and send a zchange
        // notification.
        while(((cpFirst > cpTopEnd) || (ili == lCount - 1)) && iTop >= 0)
        {
            VISIBILITYMODE vis = VISIBILITYMODE_INHERIT;

            CRelDispNode * prdn = &prdnc->Item(prdnCtx->_iEntry);

            // pop the top rel disp node context and create a disp item for it
            // and update the parent disp node context
            prdn->SetElement( prdnCtx->_pElement);
            prdn->_ili        = prdnCtx->_ili;
            prdn->_yli        = prdnCtx->_yli;
            prdn->_cLines     = prdnCtx->_cLines;
            if(!_fRTL)
                prdn->_ptOffset.x = prdnCtx->_rc.left;
            else
                prdn->_ptOffset.x = prdnCtx->_rc.right;
            prdn->_ptOffset.y = prdnCtx->_rc.top - prdnCtx->_yli;

            // create the display node
            if(prdnCtx->_fHasChildren)
            {
                prdn->_pDispNode = CDispRoot::CreateDispContainer(
                                                GetRelDispNodeCache(),
                                                TRUE,                   // has extra cookie
                                                FALSE,                  // dni.HasUserClip(),
                                                FALSE,                  // dni.HasInset(),
                                                DISPNODEBORDER_NONE,    // dni.GetBorderType(),
                                                pli->_fRTL);                 // dni.IsRTL()

                //
                // Some of the children may be -ve'ly positioned, so we need to set
                // the background bit on the dispnode to get called with a DrawClientBackground
                // before we draw any -ve'ly positioned children
                //
                prdn->_pDispNode->SetBackground(TRUE);
            }
            else
            {
                prdn->_pDispNode = (CDispNode *)CDispRoot::CreateDispItemPlus(
                                                GetRelDispNodeCache(),
                                                TRUE,                   // has extra cookie
                                                FALSE,                  // dni.HasUserClip(),
                                                FALSE,                  // dni.HasInset(),
                                                DISPNODEBORDER_NONE,    // dni.GetBorderType(),
                                                pli->_fRTL);                 // dni.IsRTL()
            }

            if(!prdn->_pDispNode)
                goto Error;

            prdn->_pDispNode->SetSize(prdnCtx->_rc.Size() , FALSE);
            prdn->_pDispNode->SetOwned(TRUE);
            prdn->_pDispNode->SetAffectsScrollBounds(fDesignMode);

            switch (prdn->GetElement()->GetFirstBranch()->GetCascadedvisibility())
            {
            case styleVisibilityVisible:
                vis = VISIBILITYMODE_VISIBLE;
                break;

            case styleVisibilityHidden:
                vis = VISIBILITYMODE_INVISIBLE;
                break;

            case styleVisibilityInherit:
            default:
                vis = prdn->GetElement()->GetFirstBranch()->GetCharFormat()->_fVisibilityHidden
                                        ? VISIBILITYMODE_INVISIBLE
                                        : VISIBILITYMODE_VISIBLE;
            }

            Assert(vis == VISIBILITYMODE_VISIBLE || vis == VISIBILITYMODE_INVISIBLE);
            prdn->_pDispNode->SetVisible(vis == VISIBILITYMODE_VISIBLE);

            //
            // Append the current cLines & rc to the parent's
            // cache entry, if the current element is nested.
            //
            if(iTop > 0)
            {
                CRelDispNodeCtx * prdnCtxT = prdnCtx;

                prdnCtx = &aryRelDispNodeCtx.Item(iTop - 1);
                prdnCtx->_fHasChildren = TRUE;
                prdnCtx->_cLines += prdnCtxT->_cLines;
                cpTopEnd = prdnCtx->_cpEnd;

                // update the parent's context
                UnionRect( (RECT *)&prdnCtx->_rc,
                           (RECT *)&prdnCtx->_rc,
                           (RECT *)&prdnCtxT->_rc);

            }
            else
            {
                prdnCtx  = NULL;
                cpTopEnd = MINLONG;
            }

            aryRelDispNodeCtx.Delete(iTop--);
        }
    }
Cleanup:
    return;
Error:
    goto Cleanup;
}

//+----------------------------------------------------------------------------
//
// Function:    CRelDispNodeCache::GetOwner
//
// Synopsis:    Get the owning element of the given disp node.
//
//-----------------------------------------------------------------------------
void
CRelDispNodeCache::GetOwner(
    CDispNode * pDispNode,
    void ** ppv)
{
    CRelDispNode *  prdn;
    long            lEntry;

    Assert(pDispNode);
    Assert(pDispNode->GetDispClient() == this);
    Assert(ppv);
    Assert(Size());

    // we could be passed in a dispNode corresponding to the
    // content node of a container, in which case return the
    // element owner of the container.
    if(!pDispNode->IsOwned())
    {
        CDispNode * pDispNodeParent = pDispNode->GetParentNode();

        Assert(pDispNodeParent->GetDispClient() == this);

        lEntry = (LONG)(LONG_PTR)pDispNodeParent->GetExtraCookie();
    }
    else
    {
        lEntry = (LONG)(LONG_PTR)pDispNode->GetExtraCookie();
    }

    Assert(lEntry >= 0 && lEntry < Size());

    prdn = (*this)[lEntry];

    Assert (prdn->_pDispNode == pDispNode ||
            prdn->_pDispNode == pDispNode->GetParentNode());

    *ppv = prdn->GetElement();
}


long GetRelCacheEntry(CDispNode * pDispNode)
{
    long    lEntry;

    //
    // Find the index of the the cache entry that corresponds to the given node.
    // Extra cookie on the disp node stores the index into the cache.
    //
    // For container nodes, their flow node is passed in as a display node
    // to be rendered, so we need to get the parent's cookie instead.
    //
    if(!pDispNode->IsOwned())
    {
        CDispNode * pDispNodeParent = pDispNode->GetParentNode();

        lEntry = (LONG)(LONG_PTR)pDispNodeParent->GetExtraCookie();
    }
    else
    {
        lEntry = (LONG)(LONG_PTR)pDispNode->GetExtraCookie();
    }

    return lEntry;
}

BOOL
CRelDispNodeCache::HitTestContent(
    const POINT *pptHit,
    CDispNode *pDispNode,
    void *pClientData)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);

    CFlowLayout   * pFL = _pdp->GetFlowLayout();
    CElement      * pElementDispNode;
    CRelDispNode *  prdn;
    CHitTestInfo *  phti        = (CHitTestInfo *)pClientData;
    BOOL            fBrowseMode = !pFL->ElementContent()->IsEditable();
    long            lEntry      = GetRelCacheEntry(pDispNode);
    long            lSize       = Size();
    long            ili;
    long            iliLast;
    long            yli;
    long            cp;
    CPoint          pt;

    Assert(lSize && lEntry >= 0 && lEntry < lSize);

    prdn    = (*this)[lEntry];
    cp      = prdn->GetElement()->GetFirstCp() - 1;
    ili     = prdn->_ili;
    iliLast = ili + prdn->_cLines;
    yli     = prdn->_yli;

    //
    // convert the point relative to the layout parent
    //

    pt.x = pptHit->x + prdn->_ptOffset.x;
    pt.y = pptHit->y + prdn->_ptOffset.y + prdn->_yli;

    pElementDispNode = prdn->GetElement();

    //
    // HitTest lines that are owned by the given relative node.
    //
    while(ili < iliLast)
    {
        CLine * pli = _pdp->Elem(ili);

         // Hit test the current line here.
        if (!pli->_fHidden && (!pli->_fDummyLine || fBrowseMode))
        {
            // if the point is in the vertical bounds of the line
            if ( pt.y >= yli + pli->GetYTop() &&
                 pt.y <  yli + pli->GetYLineBottom())
            {
                // check if the point lies in the horzontal bounds of the
                // line
                if (pt.x >= pli->_xLeftMargin &&
                    pt.x < (pli->_fForceNewLine
                                    ? pli->_xLeftMargin + pli->_xLineWidth
                                    : pli->GetTextRight(ili == _pdp->LineCount() - 1)))
                {
                    break;
                }
            }

        }

        if(pli->_fForceNewLine)
            yli += pli->_yHeight;

        cp += pli->_cch;
        ili++;
    }

    //
    // If a line is hit, find the branch hit.
    //
    if ( ili < iliLast)
    {
        HITTESTRESULTS * presultsHitTest = phti->_phtr;
        HTC              htc = HTC_YES;
        BOOL             fPseudoHit;

        //
        // If the line hit belongs to a nested relative line then it is
        // a pseudo hit.
        //
        fPseudoHit = FALSE;

        for (++lEntry; lEntry < lSize; lEntry++)
        {
            prdn++;

            if(ili >= prdn->_ili && ili < prdn->_ili + prdn->_cLines)
            {
                fPseudoHit = TRUE;
                break;
            }
        }

        if(!fPseudoHit)
        {
            CLinePtr    rp(_pdp);
            CTreePos *  ptp = NULL;
            DWORD       dwFlags = 0;

            dwFlags |= !!(phti->_grfFlags & HT_ALLOWEOL)
                            ? CDisplay::CFP_ALLOWEOL : 0;
            dwFlags |=  !(phti->_grfFlags & HT_DONTIGNOREBEFOREAFTER)
                            ? CDisplay::CFP_IGNOREBEFOREAFTERSPACE : 0;
            dwFlags |=  !(phti->_grfFlags & HT_NOEXACTFIT)
                            ? CDisplay::CFP_EXACTFIT : 0;

            cp = _pdp->CpFromPointEx(
                            ili, yli,
                            cp, pt,
                            &rp, &ptp, NULL, dwFlags,
                            &phti->_phtr->_fRightOfCp,
                            &fPseudoHit, &presultsHitTest->_cchPreChars, NULL);

            if (   cp < pElementDispNode->GetFirstCp() - 1
                || cp > pElementDispNode->GetLastCp() + 1
               )
            {
                return FALSE;
            }

            presultsHitTest->_iliHit = rp;
            presultsHitTest->_ichHit = rp.RpGetIch();

            
            if (!fBrowseMode && ptp->IsNode() && ptp->ShowTreePos()
                             && (cp + 1 == ptp->GetBranch()->Element()->GetFirstCp()))
            {
                htc = HTC_YES;
                phti->_pNodeElement = ptp->GetBranch();
                presultsHitTest->_fWantArrow = TRUE;
            }
            else
            {
                htc = pFL->BranchFromPointEx(pt, rp, ptp,
                                    pElementDispNode->GetFirstBranch(),
                                    &phti->_pNodeElement,
                                    fPseudoHit, &presultsHitTest->_fWantArrow,
                                    dwFlags & CDisplay::CFP_IGNOREBEFOREAFTERSPACE);
            }
        }
        else
        {
            phti->_pNodeElement      = pElementDispNode->GetFirstBranch();
            presultsHitTest->_iliHit = ili;
            presultsHitTest->_ichHit = 0;
        }

        presultsHitTest->_cpHit  = cp;

        if (htc != HTC_YES)
        {
            presultsHitTest->_fWantArrow = TRUE;
        }
        else
        {
            phti->_ptContent = pt;
            phti->_pDispNode = pDispNode;
        }

        phti->_htc = htc;

        return htc == HTC_YES;
    }
    else
        return FALSE;
}

BOOL
CRelDispNodeCache::HitTestFuzzy(
    const POINT *pptHitInParentCoords,
    CDispNode *pDispNode,
    void *pClientData)
{
    // no fuzzy hit test
    return FALSE;
}

LONG
CRelDispNodeCache::CompareZOrder(
    CDispNode * pDispNode1,
    CDispNode * pDispNode2)
{
    CElement *  pElement1 = ::GetDispNodeElement(pDispNode1);
    CElement *  pElement2 = ::GetDispNodeElement(pDispNode2);

    return pElement1 != pElement2
                ? pElement1->CompareZOrder(pElement2)
                : 0;
}


//+----------------------------------------------------------------------------
//
// Function:    CRelDispNodeCache::DrawClient
//
// Synopsis:    Draw the background of the given disp node
//
//-----------------------------------------------------------------------------
void
CRelDispNodeCache::DrawClientBackground(
    const RECT* prcBounds,
    const RECT* prcRedraw,
    CDispSurface *pSurface,
    CDispNode *pDispNode,
    void *pClientData,
    DWORD dwFlags)
{
    long            lEntry = GetRelCacheEntry(pDispNode);

    Assert(Size() && lEntry >= 0 && lEntry < Size());

    if (lEntry >= 0)
    {
        Assert(pClientData);

        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pSurface);
        CRelDispNode *  prdn;

        prdn = (*this)[lEntry];

        Assert(!prdn->GetElement()->NeedsLayout());

        CTreePos *  ptp;
        CDisplay *  pdp = GetDisplay();
        long        cp;

        prdn = (*this)[lEntry];

        ((CRect &)(pDI->_rcClip)).IntersectRect(*prcBounds);

        prdn->GetElement()->GetTreeExtent(&ptp, NULL);

        cp = ptp->GetCp();

        // Draw background and border of the current relative
        // element or any of it's descendents.
        pdp->DrawRelElemBgAndBorder(cp, ptp, prdn, prcBounds, prcRedraw, pDI);
    }
}

//+----------------------------------------------------------------------------
//
// Function:    CRelDispNodeCache::DrawClient
//
// Synopsis:    Draw the given disp node
//
//-----------------------------------------------------------------------------
void
CRelDispNodeCache::DrawClient(
    const RECT* prcBounds,
    const RECT* prcRedraw,
    CDispSurface *pSurface,
    CDispNode *pDispNode,
    void *cookie,
    void *pClientData,
    DWORD dwFlags)
{
    CRelDispNode *  prdn;
    long            lEntry = GetRelCacheEntry(pDispNode);

    Assert(Size() && lEntry >= 0 && lEntry < Size());

    if (lEntry >= 0)
    {
        Assert(pClientData);

        // draw the lines here
        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pSurface);

        prdn = (*this)[lEntry];

        Assert(!prdn->GetElement()->NeedsLayout());

        //
        // Draw the lines that correspond to the given disp node
        //
        HDC         hdc = pDI->GetDC();
        long        ili;
        CTreePos *  ptp;
        CDisplay *  pdp = GetDisplay();
        CElement *  pElementFL = pdp->GetFlowLayoutElement();
        CLine    *  pli;
        CLSRenderer lsre(pdp, pDI);
        long        cp;

        if (!lsre.GetLS())
            return;

        prdn = (*this)[lEntry];

        ((CRect &)(pDI->_rcClip)).IntersectRect(*prcBounds);

        lsre.StartRender(*prcBounds, pDI->_rcClip);

        prdn->GetElement()->GetTreeExtent(&ptp, NULL);

        cp = ptp->GetCp();

        if ( !prdn->_pDispNode->IsContainer())
        {
            // If the current relative element is not a container,
            // draw its background and border (or those of its
            // descendents).  Containers have their background
            // drawn via DrawClientBackground().
            pdp->DrawRelElemBgAndBorder(cp, ptp, prdn, prcBounds, prcRedraw, pDI);
        }

        lsre.SetCurPoint(CPoint(0, prcBounds->top - prdn->_ptOffset.y));
        lsre.SetCp(cp, ptp);

        for (ili = prdn->_ili; ili < prdn->_ili + prdn->_cLines; ili++)
        {
            //
            // Find the current relative node that owns the current line
            //
            CTreeNode * pNodeRelative =
                            ptp->GetBranch()->GetCurrentRelativeNode(pElementFL);

            pli = pdp->Elem(ili);

            //
            // Skip the current line if the owner is not the same element
            // that owns the current display node.
            //
            if( pNodeRelative && pNodeRelative->Element() != prdn->GetElement())
            {
                lsre.SkipCurLine (pli);
            }
            else
            {
                lsre.RenderLine(*pli, prcBounds->left - prdn->_ptOffset.x);
            }

            Assert(pli == pdp->Elem(ili));

            ptp = lsre.GetPtp();
            // It's possible for the renderer to return a pointer treepos; this is no good
            // to us, since we need the next content related node.  Keep walking till we
            // find one. (Bug #72264).
            while ( ptp->Type() == CTreePos::EType::Pointer )
            {
                ptp = ptp->NextTreePos();
            }
        }

        //
        // restore the original text align, the renderer might have modified the
        // text align. This caused some pretty bad rendering problems(specially with
        // radio buttons).
        //
        if (lsre._lastTextOutBy != CLSRenderer::DB_LINESERV)
        {
            lsre._lastTextOutBy = CLSRenderer::DB_NONE;
            SetTextAlign(hdc, TA_TOP | TA_LEFT);
        }
    }
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::DrawRelElemBgAndBorder
//
// Synopsis:    CDisplay::DrawRelElemBgAndBorder draws the backround or borders
//              on itself and any child elements that are not relative.
//
//-----------------------------------------------------------------------------
void
CDisplay::DrawRelElemBgAndBorder(
    long            cp,
    CTreePos      * ptp,
    CRelDispNode  * prdn,
    const RECT    * prcView,
    const RECT    * prcClip,
    CFormDrawInfo * pDI)
{
    CDataAry <RECT> aryRects(Mt(CDisplayDrawRelElemBgAndBorder_aryRects_pv));
    CLine       *   pli;
    CFlowLayout *   pFlowLayout     = GetFlowLayout();
    CMarkup     *   pMarkup         = pFlowLayout->GetContentMarkup();
    BOOL            fPaintBackground= pFlowLayout->Doc()->PaintBackground();
    long            iliStop = min(LineCount(), prdn->_ili + prdn->_cLines);
    long            yli     = - prdn->_ptOffset.y;
    long            cpLine;
    long            cpNextLine;
    long            cpPtp;
    long            ili, ich;
    CPoint          ptOffset = prdn->_ptOffset;

    ptOffset.x = -ptOffset.x;
    ptOffset.y = -ptOffset.y - prdn->_yli;

    cpPtp = cpLine = cpNextLine = cp;
    for (ili = prdn->_ili; ili < iliStop && yli < prcClip->bottom; ili++)
    {
        pli = Elem(ili);

        cpNextLine += pli->_cch;

        if (pli->_fForceNewLine)
            yli += pli->_yHeight;

        if (pli->_cch)
        {
            if (    (fPaintBackground && pli->_fHasBackground)
                ||  pli->_fHasParaBorder)
            {
                if(cpPtp < cpLine)
                {
                    ptp = pMarkup->TreePosAtCp(cpLine, &ich);

                    cpPtp = cpLine;
                    if(ich)
                    {
                        cpPtp += ich;
                        ptp = ptp->NextTreePos();
                    }
                }

                while(cpNextLine > cpPtp)
                {
                    if(ptp->IsBeginElementScope())
                    {
                        CTreeNode * pNode = ptp->GetBranch();
                        CElement  * pElement = pNode->Element();
                        const CCharFormat * pCF = pNode->GetCharFormat();
                        const CFancyFormat* pFF = pNode->GetFancyFormat();

                        if(     pCF->IsVisibilityHidden() || pCF->IsDisplayNone()
                            ||  (cp != cpPtp && (pFF->_fRelative || pNode->NeedsLayout())))
                        {
                            pElement->GetTreeExtent(NULL, &ptp);
                            ptp = ptp->NextTreePos();
                            cpPtp = ptp->GetCp();
                            continue;
                        }
                        else
                        {
                            BOOL fDrawBackground = fPaintBackground &&
                                                    (pFF->_lImgCtxCookie ||
                                                     pFF->_ccvBackColor.IsDefined());
                            BOOL fDrawBorder     = pNode->GetParaFormat()->_fPadBord &&
                                                    pFF->_fBlockNess;

                            // Draw the background if the element comming into scope
                            // has background or border.
                            if ( fDrawBackground || fDrawBorder)
                            {
                                DrawElemBgAndBorder(
                                    pElement, &aryRects,
                                    prcView, prcClip,
                                    pDI, &ptOffset,
                                    fDrawBackground, fDrawBorder, -1, -1, TRUE);
                            }
                        }

                    }
                    cpPtp += ptp->GetCch();
                    ptp    = ptp->NextTreePos();
                }
            }
        }

        cpLine = cpNextLine;
    }
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::DrawElemBgAndBorder
//
// Synopsis:    Draw the border and background on an element if any. cpStart
//              and cpFinish define the clip region. iRunStart and iRunFinish
//              are for performance reasons so that we dont have run around in
//              the tree for the element.
//
//-----------------------------------------------------------------------------

void
CDisplay::DrawElemBgAndBorder(
                     CElement        *  pElement,
                     CDataAry <RECT> *  paryRects,
                     const RECT      *  prcView,
                     const RECT      *  prcClip,
                     CFormDrawInfo   *  pDI,
                     const CPoint    *  pptOffset,
                     BOOL               fDrawBackground,
                     BOOL               fDrawBorder,
                     LONG               cpStart,       // default -1
                     LONG               cpFinish,      // default -1
                     BOOL               fNonRelative)
{
    Assert (pElement && pDI && prcView && prcClip && paryRects);
    Assert (!pElement->NeedsLayout());

    // If a clip range has been specified (e.g. either cpStart or cpFinish
    // is not -1), then we want a bounding rect.
    // BUGBUG: (KTam) clean this up! we may want RFE to specifically take
    // a flag that says to clip or not. remove dependancy on prcBound
    // to determine clipping.
    RECT        rcBound;
    RECT *      prcBound = ((pElement->IsBlockElement() || cpStart != -1 || cpFinish != -1)
                                    ? &rcBound
                                    : NULL);

    Assert ( fDrawBorder || fDrawBackground );

    CPoint  ptOffset(((CRect *)prcView)->TopLeft());
    if (pptOffset)
    {
        ptOffset += (SIZE &)*pptOffset;
    }

    // If the display is RTL we need to adjust by the width to
    // get it into positive coordinates.
    if(_fRTL)
        ptOffset.x += _xWidth;
    
    DWORD dwRFEFlags = fNonRelative ? RFE_IGNORE_RELATIVE : 0;

    dwRFEFlags |= RFE_BACKGROUND;

    RegionFromElement(pElement, paryRects, &ptOffset, pDI,
        dwRFEFlags, cpStart, cpFinish, prcBound);

    if(paryRects->Size())
    {

        // now that we have its region render its background or border
        if(fDrawBackground)    // if we have background color
        {
            // draw element background
            DrawElementBackground(pElement->GetFirstBranch(), paryRects, prcBound,
                                    prcView, prcClip,
                                    pDI);
        }

        if (fDrawBorder)
        {
            // Draw a border if necessary
            DrawElementBorder(pElement->GetFirstBranch(), paryRects, prcBound,
                                prcView, prcClip,
                                pDI);
        }
    }
}

CDispNode *
CRelDispNodeCache::FindElementDispNode(CElement * pElement)
{
    int lSize = _aryRelDispNodes.Size();

    if (lSize)
    {
        CRelDispNode * prdn = _aryRelDispNodes;
        for (; lSize ; lSize--, prdn++)
        {
            if(prdn->GetElement() == pElement)
            {
                return prdn->_pDispNode;
            }
        }
    }
    return NULL;
}

void 
CRelDispNodeCache::Delete(long iPosFrom, long iPosTo)
{
    // we need to clearContents on each item
    if (iPosTo >= iPosFrom)
    {
        int            i = iPosFrom;
        CRelDispNode * prdn = (*this)[i];

        for (; i<=iPosTo; i++, prdn++)
        {
            prdn->SetElement( NULL );
        }
    }

    _aryRelDispNodes.DeleteMultiple(iPosFrom, iPosTo);
}

void
CRelDispNodeCache::SetElementDispNode(
    CElement *  pElement,
    CDispNode * pDispNode)
{
    Assert(pElement);
    Assert(pDispNode);

    int lSize = _aryRelDispNodes.Size();

    if (lSize)
    {
        CRelDispNode * prdn = _aryRelDispNodes;
        for (; lSize ; lSize--, prdn++)
        {
            if(prdn->GetElement() == pElement)
            {
                prdn->_pDispNode = pDispNode;
            }
        }
    }
}

void
CRelDispNodeCache::EnsureDispNodeVisibility(
    CElement * pElement)
{
    CLayout *       pLayout = _pdp->GetFlowLayout();
    CRelDispNode *  prdn    = _aryRelDispNodes;
    long            cSize   = _aryRelDispNodes.Size();

    for (; cSize; prdn++, cSize--)
    {
        if (    !pElement
            ||  prdn->GetElement() == pElement)
        {
            pLayout->EnsureDispNodeVisibility(prdn->GetElement(), prdn->_pDispNode);
        }
    }
}

void
CRelDispNodeCache::HandleDisplayChange()
{
    CRelDispNode *  prdn    = _aryRelDispNodes;
    long            cSize   = _aryRelDispNodes.Size();
    BOOL            fDisplayNone = _pdp->GetFlowLayoutElement()->IsDisplayNone();

    for (; cSize; prdn++, cSize--)
    {
        if (fDisplayNone || prdn->GetElement()->IsDisplayNone())
        {
            prdn->_pDispNode->ExtractFromTree();
        }
        else
        {
            CPoint ptAuto(prdn->_ptOffset.x, prdn->_ptOffset.y + prdn->_yli);

            prdn->GetElement()->ZChangeElement(NULL, &ptAuto);
        }
    }
}

void
CRelDispNodeCache::DestroyDispNodes()
{
    long lSize = _aryRelDispNodes.Size();

    if (lSize)
    {
        CRelDispNode *  prdn;
        for (prdn = (*this)[0]; lSize; lSize--, prdn++)
        {
            prdn->ClearContents();
        }
    }
}

void
CRelDispNodeCache::Invalidate(
    CElement *pElement,         // Relative element to invalidate
    const RECT * prc /*=NULL*/, // array of rects describing region to inval
    int nRects /*=1*/ )         // count of rects in array
{
    // The element must itself actually be relative; it is insufficient for
    // it to have "inherited" relativeness (see element.hxx discussion of
    // IsRelative vs. IsInheritingRelativeness), since the RDNC tracks only
    // only relative elements and not their children.
    // We could use GetCurrentRelativeNode() instead of relying on the caller
    // to do that, but it would require an additional param (pElementFL)
    // that might obfuscate.
    Assert( pElement->IsRelative() );

    CDispNode *pDispNode = FindElementDispNode( pElement );

    // If callers pass a relative element that we aren't responsible for, then
    // bail!  This shouldn't happen.
    if ( !pDispNode )
    {
        // BUGBUG: this assert is valid.  See CFlowLayout::Notify() comments
        // on handling of invalidation.
        //Assert( FALSE && "Caller passed a relative element that doesn't belong to this RDNC!" );
        return;
    }
    
    if ( _pdp->GetFlowLayout()->Doc()->_state >= OS_INPLACE )
    {   
        if ( _pdp->GetFlowLayout()->OpenView())
        {
            // Incoming rects are in the coordinate system of the
            // flow layout.  We need to convert them into the system
            // of the dispnode, so we need the flow offset of the
            // dispnode..
            CPoint pt;
            _pdp->GetRelNodeFlowOffset( pDispNode, &pt );

            // Conversion involves subtracting flow offset of dispnode
            // from each rect:
            pt.x = -pt.x;
            pt.y = -pt.y;
            
            Assert( prc );
            for (int i=0; i < nRects; i++, prc++)
            {
                ::OffsetRect( (RECT *)prc, pt.x, pt.y );
                pDispNode->Invalidate((CRect &)*prc, COORDSYS_CONTENT);
            }
        }
    }
}


void
CDisplay::GetRelNodeFlowOffset(CDispNode * pDispNode, CPoint *ppt)
{
    CRelDispNode      * prdn;
    CRelDispNodeCache * prdnc = GetRelDispNodeCache();
    long                lEntry = (LONG)(LONG_PTR)pDispNode->GetExtraCookie();

    Assert(prdnc);
    Assert(prdnc == pDispNode->GetDispClient());
    Assert(lEntry >= 0 && lEntry < prdnc->Size());


    prdn = (*prdnc)[lEntry];

    Assert(prdn->_pDispNode == pDispNode);

    ppt->x = prdn->_ptOffset.x;
    ppt->y = prdn->_ptOffset.y + prdn->_yli;
}

void
CDisplay::GetRelElementFlowOffset(CElement * pElement, CPoint *ppt)
{
    CRelDispNode      * prdn;
    CRelDispNodeCache * prdnc = GetRelDispNodeCache();
    long                lEntry, lSize;

    *ppt = g_Zero.pt;

    if(prdnc)
    {
        lSize = prdnc->Size();
        prdn  = (*prdnc)[0];

        for(lEntry = 0; lEntry < lSize; lEntry++, prdn++)
        {
            if (prdn->GetElement() == pElement)
            {
                ppt->x = prdn->_ptOffset.x;
                ppt->y = prdn->_ptOffset.y + prdn->_yli;
            }
        }
    }
}


void
CDisplay::TranslateRelDispNodes(const CSize & size, long lStart)
{
    CDispNode         * pDispNode;
    CRelDispNodeCache * prdnc = GetRelDispNodeCache();
    CRelDispNode      * prdn;
    long                lSize = prdnc->Size();
    long                lEntry;
    long                iLastLine = -1;

    Assert(lSize && lStart < lSize);

    prdn = (*prdnc)[lStart];

    for(lEntry = lStart; lEntry < lSize; lEntry++, prdn++)
    {
        if (iLastLine <= prdn->_ili)
        {
            pDispNode = prdn->_pDispNode;
            pDispNode->SetPosition(pDispNode->GetPosition() + size);
            iLastLine = prdn->_ili + prdn->_cLines;
        }
    }
}


void
CDisplay::ZChangeRelDispNodes()
{
    CRelDispNodeCache * prdnc = GetRelDispNodeCache();
    CRelDispNode      * prdn;
    long                cEntries = prdnc->Size();
    long                iEntry   = 0;
    long                iLastLine = -1;

    Assert( cEntries
        &&  iEntry < cEntries);

    prdn = (*prdnc)[iEntry];

    for(; iEntry < cEntries; iEntry++, prdn++)
    {
        if (iLastLine < prdn->_ili)
        {
            prdn->GetElement()->ZChangeElement();
            iLastLine = prdn->_ili + prdn->_cLines;
        }
    }
}
