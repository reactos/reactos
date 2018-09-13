//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       qrel.cxx
//
//  Contents:   Implementation of some code for relatively positioned lines.
//
//  Classes:    CDisplay
//
// This is the version customized for hosting Quill.
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

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPITEM_HXX_
#define X_DISPITEM_HXX_
#include "dispitem.hxx"
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

MtDefine(CDisplayDrawRelativeElement_aryRects_pv, Locals, "CDisplay::DrawRelativeElement aryRects::_pv")
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
        prc->left   = xLead + (pli->_fFirstFragInLine
                                ? pli->_xLeftMargin
                                : pli->GetTextLeft());

        prc->right  = xLead + pli->_xLeftMargin + pli->_xLineWidth
                        + (pli->_fForceNewLine ? pli->_xWhite : 0);
    }
    else
    {
        prc->right  = xLead - (pli->_fFirstFragInLine
                                ? pli->_xRightMargin
                                : pli->GetRTLTextRight());

        prc->left   = xLead - (pli->_xRightMargin + pli->_xLineWidth
                        + (pli->_fForceNewLine ? pli->_xWhite : 0));
    }

    // If the line has negative margin, we need to use the larger
    // rect because text goes into the negative margin space.
    prc->top    = top + min(0L, pli->GetYTop());
    prc->bottom = top + max(pli->GetYHeight(), pli->GetYBottom());
}

void
CDisplay::VoidRelDispNodeCache()
{
    if(HasRelDispNodeCache())
    {
        CRelDispNodeCache * prdnc = GetRelDispNodeCache();

        prdnc->DestroyDispNodes();

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
                cpFirst = prdn->_pElement->GetFirstCp() - 1;
            }

            if (iliMatchNew != LineCount())
            {
                for(iEntryFinish = iEntryStart;
                    prdn->_ili < pled->_iliMatchOld && iEntryFinish < lSize;
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

                        if(     prdnNewEntry->_pElement == prdn->_pElement
                            &&  prdnNewEntry->_pDispNode->IsContainer())
                        {
                            // start at the next entry, so that we can start
                            // the search for the next container after the
                            // current entry. Both the caches are in source order
                            iNewEntry = i + 1;

                            prdn->_pDispNode->ExtractFromTree();

                            TraceTag((tagRelPositioning, "\tReplacing dispnode for element(Tag: %ls, SN:%ld) from %ld",
                                        prdn->_pElement->TagName(),
                                        prdn->_pElement->SN(),
                                        iEntry));

                            prdnNewEntry->_pDispNode->ReplaceNode(prdn->_pDispNode);
                            break;
                        }
                    }

                    if( i == cNewEntries)
                    {
                        TraceTag((tagRelPositioning, "\tDestroying dispnode for element(Tag: %ls, SN:%ld) from %ld",
                                    prdn->_pElement->TagName(),
                                    prdn->_pElement->SN(),
                                    iEntry));
                        prdn->_pDispNode->Destroy();
                    }
                }
                else
                {
                    TraceTag((tagRelPositioning, "\tDestroying dispnode for element(Tag: %ls, SN:%ld) from %ld",
                                prdn->_pElement->TagName(),
                                prdn->_pElement->SN(),
                                iEntry));
                    prdn->_pDispNode->Destroy();
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

                for (iEntry = iEntryFinish, prdn = (*prdnc)[iEntryFinish];
                     iEntry < lSize;
                     iEntry++, prdn++)
                {
                    prdn->_ili += dili;
                    prdn->_yli += dy;
                    prdn->_pDispNode->SetPosition(prdn->_pDispNode->GetPosition() + CSize(0, dy));
                    prdn->_pDispNode->SetExtraCookie((void *)(iEntry + diEntries));
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
                CPoint ptAuto(prdn->_ptOffset.x, prdn->_ptOffset.y + prdn->_yli);

                prdnc->InsertAt(iEntryInsert, *prdn);

                prdn->_pDispNode->SetExtraCookie((void *)(iEntryInsert));

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
                                prdn->_pElement->TagName(),
                                prdn->_pElement->SN(),
                                iEntryInsert));
                //
                // Fire off a ZChange notification to insert it in the appropriate
                // ZLayer and ZParent.
                //
                prdn->_pElement->ZChangeElement(NULL, &ptAuto);

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

    //
    // Note: here we are trying to walk lines beyond iliMatchNew to update the
    // rc of the elements that came into scope in the dirty region
    //

    for (ili = iliStart;
         ili < lCount && (ili < iliMatchNew || cpFirst < cpTopEnd);
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
                &&  !pElementRel->HasLayout())
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

            if(pli->IsFrame() || pli->_fHasEmbedOrWbr)
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
            prdn->_pElement   = prdnCtx->_pElement;
            prdn->_ili        = prdnCtx->_ili;
            prdn->_yli        = prdnCtx->_yli;
            prdn->_cLines     = prdnCtx->_cLines;
            prdn->_ptOffset.x = prdnCtx->_rc.left;
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
                                                _fRTL);                 // dni.IsRTL()

            }
            else
            {
                prdn->_pDispNode = (CDispNode *)CDispRoot::CreateDispItemPlus(
                                                GetRelDispNodeCache(),
                                                TRUE,                   // has extra cookie
                                                FALSE,                  // dni.HasUserClip(),
                                                FALSE,                  // dni.HasInset(),
                                                DISPNODEBORDER_NONE,    // dni.GetBorderType(),
                                                _fRTL);                 // dni.IsRTL()
            }

            if(!prdn->_pDispNode)
                goto Error;

            prdn->_pDispNode->SetSize(prdnCtx->_rc.Size() , FALSE);
            prdn->_pDispNode->SetOwned(TRUE);

            switch (prdn->_pElement->GetFirstBranch()->GetCascadedvisibility())
            {
            case styleVisibilityVisible:
                vis = VISIBILITYMODE_VISIBLE;
                break;

            case styleVisibilityHidden:
                vis = VISIBILITYMODE_INVISIBLE;
                break;
            }

            prdn->_pDispNode->SetVisibilityMode(vis);

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
    if(pDispNode->GetNodeType() == DISPNODETYPE_ITEM)
    {
        CDispNode * pDispNodeParent = pDispNode->GetParentNode();

        Assert(pDispNodeParent->GetDispClient() == this);

        lEntry = (long)pDispNodeParent->GetExtraCookie();
    }
    else
    {
        lEntry = (long)pDispNode->GetExtraCookie();
    }

    Assert(lEntry >= 0 && lEntry < Size());

    prdn = (*this)[lEntry];

    Assert (prdn->_pDispNode == pDispNode ||
            prdn->_pDispNode == pDispNode->GetParentNode());

    *ppv = prdn->_pElement;
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
    if(pDispNode->GetNodeType() == DISPNODETYPE_ITEM)
    {
        CDispNode * pDispNodeParent = pDispNode->GetParentNode();

        lEntry = (long)pDispNodeParent->GetExtraCookie();
    }
    else
    {
        lEntry = (long)pDispNode->GetExtraCookie();
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
    cp      = prdn->_pElement->GetFirstCp() - 1;
    ili     = prdn->_ili;
    iliLast = ili + prdn->_cLines;
    yli     = prdn->_yli;

    //
    // convert the point relative to the layout parent
    //

    pt.x = pptHit->x + prdn->_ptOffset.x;
    pt.y = pptHit->y - prdn->_ptOffset.y + prdn->_yli;

    pElementDispNode = prdn->_pElement;

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
                                    : pli->GetTextRight()))
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
            CLinePtr         rp(_pdp);
            CTreePos  *      ptp = NULL;
            BOOL fAllowEOL          = !!(phti->_grfFlags & HT_ALLOWEOL);
            BOOL fIgnoreBeforeAfter =  !(phti->_grfFlags & HT_DONTIGNOREBEFOREAFTER);
            BOOL fExactFit          =  !(phti->_grfFlags & HT_NOEXACTFIT);
            BOOL fVirtual           = !!(phti->_grfFlags & HT_VIRTUALHITTEST);

            cp = _pdp->CpFromPointEx(
                            ili, yli,
                            cp, pt,
                            &rp, &ptp, NULL,
                            fAllowEOL, fIgnoreBeforeAfter,
                            fExactFit, &phti->_phtr->fRightOfCp,
                            &fPseudoHit, NULL);

            if (   cp < pElementDispNode->GetFirstCp() - 1
                || cp > pElementDispNode->GetLastCp() + 1
               )
            {
                return FALSE;
            }

            presultsHitTest->iliHit = rp;
            presultsHitTest->ichHit = rp.RpGetIch();

            htc = pFL->BranchFromPointEx(pt, rp, ptp,
                                    pElementDispNode->GetFirstBranch(),
                                    &phti->_pNodeElement,
                                    fPseudoHit, &presultsHitTest->fWantArrow,
                                    fIgnoreBeforeAfter, fVirtual);
        }
        else
        {
            phti->_pNodeElement = pElementDispNode->GetFirstBranch();
            presultsHitTest->iliHit = ili;
            presultsHitTest->ichHit = 0;
        }

        presultsHitTest->cpHit  = cp;

        if (htc != HTC_YES)
        {
            presultsHitTest->fWantArrow = TRUE;
        }
        else
            phti->_pDispNode = pDispNode;

        phti->_htc = htc;

        return htc == HTC_YES;
    }
    else
        return FALSE;
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
    IDispSurface *pSurface,
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
        // draw the lines here
        CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
        CDispSurface *  pDispSurface = DYNCAST(CDispSurface, pSurface);

        Assert(pDI);

        prdn = (*this)[lEntry];

        Assert(!prdn->_pElement->HasLayout());

        //
        // Draw the lines that correspond to the given disp node
        //
        if (SUCCEEDED(pDispSurface->_GetDC(&pDI->_hdc, *prcRedraw, FALSE)))
        {
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

            pDI->_rc     = *prcBounds;
            pDI->_rcClip = *prcRedraw;
            ((CRect &)(pDI->_rcClip)).IntersectRect(*prcBounds);

            prdn->_pElement->GetTreeExtent(&ptp, NULL);

            cp = ptp->GetCp();

            // Draw background and border of the current relative
            // element or any of it's descendents.
            pdp->DrawRelElemBgAndBorder(cp, ptp, prdn, prcBounds, prcRedraw, pDI);

            CALLREFUNC(SetCurPoint, (CPoint(0, prdn->_ptOffset.y)));
            CALLREFUNC(SetCp, (cp, ptp));

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
                if( pNodeRelative && pNodeRelative->Element() != prdn->_pElement)
                {
                    CALLREFUNC(SkipCurLine, (pli));
                }
                else
                    lsre.RenderLine(*pli, -prdn->_ptOffset.x);

                Assert(pli == pdp->Elem(ili));

                ptp = lsre.GetPtp();
            }

            //
            // restore the original text align, the renderer might have modified the
            // text align. This caused some pretty bad rendering problems(specially with
            // radio buttons).
            //
            if (lsre._lastTextOutBy != CLSRenderer::DB_LINESERV)
            {
                lsre._lastTextOutBy = CLSRenderer::DB_NONE;
                SetTextAlign(pDI->_hdc, TA_TOP | TA_LEFT);
            }
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
                            ||  (cp != cpPtp && (pFF->_fRelative || pNode->HasLayout())))
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
                     CPoint          *  pptOffset,
                     BOOL               fDrawBackground,
                     BOOL               fDrawBorder,
                     LONG               cpStart,       // default -1
                     LONG               cpFinish,      // default -1
                     BOOL               fNonRelative)
{
    Assert (pElement && pDI && prcView && prcClip && paryRects);
    Assert (!pElement->HasLayout());

    RECT        rcBound;
    RECT *      prcBound = ((cpStart != -1 || cpFinish != -1)
                                    ? &rcBound
                                    : NULL);

    Assert ( fDrawBorder || fDrawBackground );

    RegionFromElement(pElement, paryRects, pptOffset, pDI,
        fNonRelative ? RFE_NONRELATIVE : 0, cpStart, cpFinish, prcBound);

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
            if(prdn->_pElement == pElement)
            {
                return prdn->_pDispNode;
            }
        }
    }
    return NULL;
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
            prdn->_pDispNode->Destroy();
        }
    }
}

void
CDisplay::GetRelNodeFlowOffset(CDispNode * pDispNode, CPoint *ppt)
{
    CRelDispNode      * prdn;
    CRelDispNodeCache * prdnc = GetRelDispNodeCache();
    long                lEntry = long(pDispNode->GetExtraCookie());

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
            if (prdn->_pElement == pElement)
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
        if (iLastLine < prdn->_ili)
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
            prdn->_pElement->ZChangeElement();
            iLastLine = prdn->_ili + prdn->_cLines;
        }
    }
}
