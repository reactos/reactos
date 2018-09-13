//+----------------------------------------------------------------------------
//
// File:        Rgncol.cxx
//
// Contents:    Contains class CDispRegion and region collection related member
//              functions.
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#if 0

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

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

#ifndef X_DISPRGN_HXX_
#define X_DISPRGN_HXX_
#include "disprgn.hxx"
#endif

#ifndef X_DISPITEM_HXX_
#define X_DISPITEM_HXX_
#include "dispitem.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

extern ZERO_STRUCTS g_Zero;

//+----------------------------------------------------------------------------
//
// Member:      CDispRegion()
//
// Synopsis:    constructor
//
//-----------------------------------------------------------------------------

CDispRegion::CDispRegion()
{
    memset(this, 0, sizeof CDispRegion);

    _iLine = -1;
}

void
CDispRegion::Init(long iLine, long cp, long yOffset, RECT * prc)
{
    _iLine      = iLine;
    _cp         = cp;
    _yOffset    = yOffset;
    _nLines     = 0;
    _rc         = *prc;
    _pDispNode  = NULL;
}

//+----------------------------------------------------------------------------
//
// Member:      VoidRegionCollection
//
// Synopsis:    Remove all the regions and release the associated display node
//              from the display tree
//
//-----------------------------------------------------------------------------
void
CDisplay::VoidRegionCollection()
{
    CDispRegion * pRegion   = _aryRegionCollection;
    int       i             = _aryRegionCollection.Size();

    OpenDisplayTree();

    for(; i > 0; i--, pRegion++)
    {
        CDispNode* pDispNode = pRegion->_pDispNode;
        if (pDispNode != NULL)
        {
            pDispNode->Destroy();
        }
    }
    CloseDisplayTree();

    _aryRegionCollection.SetSize(0);
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::CreateRegionCollection
//
// Synopsis:    Build a new region collection
//
//-----------------------------------------------------------------------------

HRESULT
CDisplay::CreateRegionCollection()
{
    CLed led;

    OpenDisplayTree();

    // delete the old region collection
    VoidRegionCollection();

    led._cpFirst  = 0;
    led._iliFirst = 0;
    led._yFirst   = 0;

    led.SetNoMatch();

    HRESULT hr = UpdateRegionCollection(&led);

    CloseDisplayTree();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDisplay::NeedNewRegion, public
//
//  Synopsis:   Indicates whether the current line needs a new region and/or
//              ends the current one.
//
//  Arguments:  [pli]                 -- Current line pointer
//              [pNodeRelRgn]         -- Pointer to relative node for the current
//                                         region. Can be NULL
//              [pNodeRelLine]        -- Pointer to relative node for current
//                                         line. Can be NULL
//              [prcRgn]              -- Rect for the current region
//              [prcLine]             -- Rect for the current line
//              [pfEndsCurrentRegion] -- Pointer to BOOL. See notes below
//
//  Returns:    TRUE if the current line should be added to a new region. FALSE
//              if not.
//
//  Notes:      [pfEndsCurrentRegion] equal to TRUE means that this line should
//              be added to the region and then a new region should be started.
//              If the return value is also TRUE then this line should have
//              its own region.
//
//----------------------------------------------------------------------------

BOOL
CDisplay::NeedNewRegion(CLine     * pli,
                        CTreeNode * pNodeRelRgn,
                        CTreeNode * pNodeRelLine,
                        RECT      * prcRgn,
                        RECT      * prcLine,
                        BOOL      * pfEndsCurrentRegion)
{
    BOOL fReturn = FALSE;

    *pfEndsCurrentRegion = FALSE;

    //
    // Lines that must have their own region
    //
    if (   pli->IsFrame()                     // line has aligned element
        || pli->_fSingleSite)                 // single site line
    {
        *pfEndsCurrentRegion = TRUE;
        return TRUE;
    }

    //
    // Lines where we want to start a new region.
    //
    if (   pNodeRelRgn->SafeElement() !=
              pNodeRelLine->SafeElement()     // relative pos transition
        ||  prcLine->left  != prcRgn->left    // left margin changed
        ||  prcLine->right != prcRgn->right)  // right margin changed
    {
        fReturn = TRUE;
    }

#ifdef NEVER
// BUGBUG (donmarsh) - paragraphs do not end a region now
    //
    // Lines which terminate an existing region
    //
    if (pli->_fHasEOP)                        // has end-of-paragraph
    {
        *pfEndsCurrentRegion = TRUE;
    }
#endif

    return fReturn;
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::UpdateRegionCollection
//
// Synopsis:    Update region collection for a txtchange
//
// Arguments:   pled -  line edit descriptor that contains information
//                      pertaining to changes to the line array caused by a
//                      txtchange transaction.
//
//-----------------------------------------------------------------------------

HRESULT
CDisplay::UpdateRegionCollection(CLed * pled)
{
    Assert(pled);

    HRESULT     hr = S_OK;
    RECT        rcLine;
    CLine *     pli;
    CDispRegion rgn;
    CTreeNode * pNodeRelRgn = NULL;
    CTreeNode * pNodeRelLine = NULL;
    CRchTxtPtr  rtp(GetFlowLayoutElement());
    CCalcInfo   CI(GetFlowLayout());
    BOOL        fEndsCurrentRegion      = TRUE;

    long    i;
    long    cp              = pled->_cpFirst;
    long    iliStart        = pled->_iliFirst;
    long    iliFinish       = 0;
    long    yOffset         = 0;
    long    xWidthDisplay   = GetMaxWidth();
    long    iRegionStart    = 0;

    if(pled->_cpMatchOld == MAXLONG)
    {
        pled->_cpMatchOld   = GetLastCp();
        pled->_iliMatchOld  = LineCount();
    }

    if(pled->_cpMatchNew == MAXLONG)
    {
        pled->_cpMatchNew   = GetLastCp();
        pled->_iliMatchNew  = LineCount();
    }

    OpenDisplayTree();

    // first remove the regions effected
    if(_aryRegionCollection.Size())
    {
        long iRegionFinish;

        iliFinish = pled->_iliMatchOld - 1;

        Assert(iliStart >= 0);
        Assert(iliFinish <= LineCount());

        // find the region corresponding to iliStart
        iRegionStart  = FindRegionIndexForLine(iliStart);
        if(iliFinish < iliStart)
            iRegionFinish = -1;
        else
            iRegionFinish = FindRegionIndexForLine(iliFinish);


        if(iRegionStart == -1)
        {
            iRegionStart = _aryRegionCollection.Size() - 1;
        }
        else
        {
            long    iliDiff = pled->_iliMatchOld - pled->_iliMatchNew;
            long    cchDiff = pled->_cpMatchOld - pled->_cpMatchNew;

            if(iRegionFinish == -1)
                iRegionFinish = _aryRegionCollection.Size() - 1;

            cp        = _aryRegionCollection[iRegionStart]._cp + GetFirstCp();
            yOffset   = _aryRegionCollection[iRegionStart]._yOffset;
            iliStart  = _aryRegionCollection[iRegionStart]._iLine;
            iliFinish = max(pled->_iliMatchNew,
                            _aryRegionCollection[iRegionFinish]._iLine +
                            long(_aryRegionCollection[iRegionFinish]._nLines));
            iliFinish = min(iliFinish, long(LineCount()));

            for (i = iRegionFinish + 1; i < _aryRegionCollection.Size(); i++)
            {
                _aryRegionCollection[i]._iLine -= iliDiff;
                _aryRegionCollection[i]._cp    -= cchDiff;
            }

            for(i = iRegionStart; i <= iRegionFinish; i++)
            {
                CDispNode* pDispNode = _aryRegionCollection[i]._pDispNode;
                if (pDispNode != NULL)
                {
                    pDispNode->Destroy();
                }
            }
            
            // remove the effected regions
            _aryRegionCollection.DeleteMultiple(iRegionStart, iRegionFinish);
        }
    }
    else
    {
        iliFinish = pled->_iliMatchNew;
    }

    rtp = cp;

    // now add new regions for the effected lines
    for(i = iliStart; i < iliFinish; i++)
    {
        BOOL fNewRegion;

        rtp.AdvanceToNonEmpty();

        pli = Elem(i);

        RcFromLine(&rcLine, yOffset, 0, pli);

        if(pli->_fRelative)
        {
            const CCharFormat * pCF;

            pNodeRelLine = rtp.GetCurrentRelativeNode(GetFlowLayout(),
                                                      pli->IsFrame()
                                                        ? pli->_pNodeLayout
                                                        : NULL);

            Assert(pNodeRelLine);

            pCF = pNodeRelLine->GetCharFormat();

            OffsetRect(&rcLine, pCF->GetRelLeft(&CI), pCF->GetRelTop(&CI));
        }
        else
        {
            pNodeRelLine = NULL;
        }

        if(pli->_fForceNewLine && !pli->_xRightMargin)
        {
            rcLine.right = xWidthDisplay;
        }

        //
        // Did the previous line indicate that the next line (the current
        // line) should be in a new region?
        //
        fNewRegion = fEndsCurrentRegion;

        //
        // Does this line need to be in a new region? (fEndsCurrentRegion
        // will be set for the next line).
        //
        fNewRegion = NeedNewRegion(pli,
                                   pNodeRelRgn,
                                   pNodeRelLine,
                                   &rgn._rc,
                                   &rcLine,
                                   &fEndsCurrentRegion) || fNewRegion;
        if (fNewRegion)
        {
            if (rgn._iLine != -1)
            {
                AddRegionToCollection(iRegionStart++, &rgn);
            }

            rgn.Init(i, rtp.GetCp() - GetFirstCp(), yOffset, &rcLine);

            pNodeRelRgn = pNodeRelLine;
        }
        else
        {
            UnionRect(&rgn._rc, &rgn._rc, &rcLine);
        }

        rgn._nLines++;

        if(pli->_fForceNewLine)
            yOffset += pli->_yHeight;

        rtp.Advance(pli->_cch);
    }

    // Add the last region
    if(iliStart < iliFinish)
        AddRegionToCollection(iRegionStart++, &rgn);

    CloseDisplayTree();

    RRETURN(hr);
}

long
CDisplay::FindRegionIndexForLine(long iLine)
{
    CDispRegion * pRegion = NULL;
    long      lSize = _aryRegionCollection.Size();
    long      lLow, lMid, lHigh;

    Assert (lSize > 0);
    Assert (iLine <= _aryRegionCollection[lSize - 1]._iLine +
                        _aryRegionCollection[lSize - 1]._nLines);

    // binary search for iLine
    lLow = 0;
    lHigh = lSize - 1;

    while(lLow <= lHigh)
    {
        lMid = (lLow + lHigh)/2;

        if(iLine < _aryRegionCollection[lMid]._iLine)
        {
            lHigh = lMid - 1;
        }
        else if(iLine >= (_aryRegionCollection[lMid]._iLine +
                         _aryRegionCollection[lMid]._nLines))
        {
            lLow = lMid + 1;
        }
        else
        {
            return lMid;
        }
    }

    return -1;
}


HRESULT
CDisplay::AddRegionToCollection(long iPos, CDispRegion * pRegion)
{
    HRESULT hr = S_OK;
    LONG    ili = pRegion->_iLine;
    BOOL    fNewNode = !Elem(ili)->_fDummyLine;

#if 1
    if(fNewNode && pRegion->_nLines == 1)
    {
        if(Elem(ili)->_fSingleSite)
        {
            // BUGBUG - (srinib) We need to move this code out of here,
            // single sites should be handled outside. AddRegionToCollection
            // should not be called for regions that contain only a nested
            // run owner
            CRchTxtPtr rtp(GetFlowLayoutElement(), pRegion->_cp + GetFirstCp());

            rtp.AdvanceToNonEmpty();

            if(rtp.CurrBranch()->Tag() != ETAG_HR)
            {
#if 1
                Assert(pRegion->_pDispNode == NULL);
#else
                CLayout * pLayoutOwnLine = rtp.GetRunOwner(GetFlowLayout());

                Assert(pLayoutOwnLine != GetFlowLayout());

                pRegion->_pDispNode = pLayoutOwnLine->_pDispNode;
                AssertSz(pRegion->_pDispNode != NULL, "A Layout subclass didn't call CheckDisplayNode in its CalcSize equivalent");
#endif
                fNewNode = FALSE;
            }
        }
        else
        {
            LONG width = Elem(ili)->_xWidth;
            if (width == 0)
                fNewNode = FALSE;
        }
    }
#endif
    if(fNewNode)
    {
        // create display item for display tree (use internal interfaces for speed)
        // BUGBUG (donmarsh) - for now, copy redundant rectangle stored in region
        CDispItem* pItem = CDispRoot::CreateDispItem(
            &(pRegion->_rc),(void*)iPos,FALSE);
        Assert(pRegion->_pDispNode == NULL);
        pRegion->_pDispNode = pItem;

        // BUGBUG (donmarsh) - for now, ignore z index; just add this display item
        // to container node
        pItem->SetLayerType(DISPNODELAYER_FLOW);
//        _pFlowLayout->AddChildDisplayNode(pItem);
    }

    hr = THR(_aryRegionCollection.InsertIndirect(iPos, pRegion));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

HRESULT
CDisplay::Draw(
        long iRegion,
        HDC hdc,
        const RECT* prcBounds,
        const RECT* prcClip)
{
    HRESULT             hr = S_OK;
    CDispRegion *       pRegion;
    CFormDrawInfo       DI;
    CFormDrawInfo *     pDI = &DI;
    pDI->Init(_pFlowLayout,hdc,NULL);
    CLSRenderer         lsre(this, pDI);
    RECT                rcView;
    CCalcInfo           CI(GetFlowLayout());

    if (!lsre._pLS)
        goto endrender;

    Assert(iRegion < _aryRegionCollection.Size());

    pRegion = &_aryRegionCollection[iRegion];

    rcView = *prcBounds;
    pDI->_rcClip = *prcClip;

    CALLREFUNC(SetCp, (pRegion->_cp + GetFirstCp()));

    // Adjust the renderer
    while(CALLREFUNC(AdjustForward, ()));

    // Prepare renderer
    if(CALLREFUNC(StartRender, (rcView, *pDI->ClipRect())))
    {
        int i;
        POINT   pt = {0, pRegion->_yOffset + CALLREFUNC(GetCF, (TRUE))->GetRelTop(&CI)};

        pt.y += rcView.top - GetYScroll();

        // Init renderer at the start of the first line to render
        CALLREFUNC(SetCurPoint, (pt));

        // Perpare the List Index cache.
        PrepareListIndexForRender();

        // BUGBUG: relative elements that have no content will
        //         not have an entry in the line array. Get rid
        //         of this assert for now.
        // we better have some lines in the relative line cache
        // Assert(_aryRelLines.Size());

        for(i = pRegion->_iLine; i < pRegion->_iLine + pRegion->_nLines; i++)
        {
            CLine *pli = Elem(i);

            // Compute the list index if appropriate.
            if (pli->_fHasBulletOrNum)
            {
                SetListIndex( lsre, i );
            }

            // Draw the line.
            lsre.RenderLine(*pli);
        }
    }
endrender:
    RRETURN(hr);
}

void
CDisplay::AdjustRegionCollectionForTextChange(long ili, long cchDelta)
{
    long iRgn = FindRegionIndexForLine(ili);

    for(long i = iRgn + 1; i < _aryRegionCollection.Size(); i++)
    {
        _aryRegionCollection[i]._cp += cchDelta;
    }
}

void
CDisplay::OpenDisplayTree()
{
    if (_pDispRoot == NULL)
    {
        _pDispRoot = GetFlowLayoutElement()->Doc()->_pDispRoot;
    }

    Assert(_pDispRoot != NULL);
    _pDispRoot->OpenDisplayTree();
}

void
CDisplay::CloseDisplayTree()
{
    Assert(_pDispRoot != NULL);
    _pDispRoot->CloseDisplayTree();
}

HTC
CDisplay::HitTestRegion(
            LONG iRgn,    
            CMessage *pMessage,
            CTreeNode ** ppNodeElement,
            DWORD dwFlags)
{
    Assert(iRgn >= 0 && iRgn < _aryRegionCollection.Size());

    CDispRegion * pRgn = &_aryRegionCollection[iRgn];
    CLinePtr      rp(this);
    CRchTxtPtr    rtp(GetFlowLayoutElement());
    POINT         pt = pMessage->pt;
    long          iCurLine = pRgn->_iLine;
    long          yTop = pRgn->_rc.top;
    long          cp   = pRgn->_cp + GetFirstCp();
    RECT          rcLine;
    HTC           htc = HTC_NO;

    Assert(PtInRect(&pRgn->_rc, pt));

    while(iCurLine < pRgn->_iLine + pRgn->_nLines)
    {
        CLine * pli = Elem(iCurLine);

        if(!_fRTL)
            RcFromLine(&rcLine, yTop, pRgn->_rc.left, pli);
        else
            RcFromLine(&rcLine, yTop, pRgn->_rc.right, pli);

        if (PtInRect(&rcLine, pt))
        {
            BOOL fPseudoHit;

            rp  = iCurLine;
            rtp.SetCp(cp);

            if(CpFromPointEx(iCurLine, yTop,
                    cp, pt,
                    &rtp, &rp, NULL,
                    FALSE, TRUE, TRUE,
                    &pMessage->resultsHitTest.fRightOfCp,
                    &fPseudoHit, NULL) != -1)
            {
                *ppNodeElement = rtp.CurrBranch();
                htc = HTC_YES;
                break;
            }
        }
        iCurLine++;
        if(pli->_fForceNewLine)
            yTop += pli->_yHeight;
        cp += pli->_cch;
    }

    return htc;
}
#endif
