//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       e1dlyt.cxx
//
//  Contents:   Implementation of C1DLayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_E1D_HXX_
#define X_E1D_HXX_
#include "e1d.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif


MtDefine(C1DLayout, Layout, "C1DLayout");

IMPLEMENT_LAYOUT_FNS(CFlowSite, C1DLayout)

const CLayout::LAYOUTDESC C1DLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,              // _dwFlags
};


//+---------------------------------------------------------------------------
//
//  Member:     C1DLayout::Init()
//
//  Synopsis:   Called when the element enters the tree. Super initializes
//              CDisplay.
//
//----------------------------------------------------------------------------

HRESULT
C1DLayout::Init()
{
    HRESULT hr = super::Init();
    if(hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     C1DLayout::CalcSize
//
//----------------------------------------------------------------------------
DWORD
C1DLayout::CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault)
{
    CSaveCalcInfo   sci(pci, this);
    CScopeFlag      csfCalcing(this);
    CSize           sizeOriginal;
    BOOL        fNormalMode = (pci->_smMode == SIZEMODE_NATURAL ||
                               pci->_smMode == SIZEMODE_SET     ||
                               pci->_smMode == SIZEMODE_FULLSIZE);
    BOOL        fSizeChanged;
    DWORD       grfReturn;

    CDisplay * pdp = GetDisplay();
    Assert(pdp);

    Listen();

    Assert(pci);
    Assert(psize);

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    // Set default return values and initial state
    //
    grfReturn = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        _fSizeThis         = TRUE;
        _fAutoBelow        = FALSE;
        _fPositionSet      = FALSE;
        _fContainsRelative = FALSE;
    }

    fSizeChanged   = (fNormalMode
                            ? (     (psize->cx != sizeOriginal.cx)
                                ||  (psize->cy != sizeOriginal.cy))
                            : FALSE);

    // Text recalc is required if:
    //  a) The site is marked for sizing
    //  b) The available width has changed (during normal calculations)
    //  c) The calculation is forced
    //  d) Changes against the text are outstanding
    //  e) The requested size is not already cached
    //
    if(   (fNormalMode && _fSizeThis)
       || (fNormalMode && fSizeChanged)
       || (pci->_grfLayout & LAYOUT_FORCE)
       || !IsCommitted()
       || (pci->_smMode == SIZEMODE_MMWIDTH && !_fMinMaxValid)
       || (pci->_smMode == SIZEMODE_MINWIDTH &&
                  (!_fMinMaxValid || _xMin < 0)))
    {

        // This is a helper routine specific to CFlowSite
        //
        grfReturn = RecalcText(pci, psize, psizeDefault);
    }

    // Propagate SIZEMODE_NATURAL requests to contained sites
    //
    if (    pci->_smMode == SIZEMODE_NATURAL
        &&  HasRequestQueue())
    {
        long xParentWidth;
        long yParentHeight;

        pdp->GetViewWidthAndHeightForChild(
            pci,
            &xParentWidth,
            &yParentHeight,
            pci->_smMode == SIZEMODE_MMWIDTH);

        ProcessRequests(pci, CSize(xParentWidth, yParentHeight));
    }

    // Lastly, return the requested size
    //
    switch (pci->_smMode)
    {
    case SIZEMODE_SET:
    case SIZEMODE_NATURAL:
    case SIZEMODE_FULLSIZE:
        Assert(!_fSizeThis);
        GetSize(psize);
        Reset(FALSE);
        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
        break;

    case SIZEMODE_MMWIDTH:
        Assert(_fMinMaxValid);
        psize->cx = _xMax;
        psize->cy = _xMin;
        break;

    case SIZEMODE_MINWIDTH:
        psize->cx = _xMin;
        psize->cy = 0;
        break;
    }

    return grfReturn;
}

//+---------------------------------------------------------------------------
//
//  Member:     C1DLayout::RecalcText
//
//  Synopsis:   Helper function for CalcSize
//
//----------------------------------------------------------------------------
DWORD
C1DLayout::RecalcText(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault)
{
    CSaveCalcInfo  sci(pci, this);
    BOOL           fNormalMode = (  pci->_smMode == SIZEMODE_NATURAL
                                 || pci->_smMode == SIZEMODE_SET
                                 || pci->_smMode == SIZEMODE_FULLSIZE);
    BOOL           fHeightChanged;
    CTreeNode     *pContext     = GetFirstBranch();
    const CFancyFormat * pFF    = pContext->GetFancyFormat();
    CUnitValue     uvWidth      = pFF->_cuvWidth;
    CUnitValue     uvHeight     = pFF->_cuvHeight;
    CUnitValue     uvBottom     = pFF->_cuvBottom;
    BOOL           fPercentWidth  = pFF->_fWidthPercent;
    BOOL           fPercentHeight = pFF->_fHeightPercent;
    styleOverflow  bOverflowX   = (styleOverflow)pFF->_bOverflowX;
    styleOverflow  bOverflowY   = (styleOverflow)pFF->_bOverflowY;
    styleStyleFloat bFloat      = (styleStyleFloat)pFF->_bStyleFloat;
    BOOL           fClipContentX = ((bOverflowX != styleOverflowVisible) &&
                                          (bOverflowX != styleOverflowNotSet));
    BOOL           fClipContentY = ((bOverflowY != styleOverflowVisible) &&
                                          (bOverflowY != styleOverflowNotSet));
    long           cxUser, cyUser, cxInit = 0;
    long           xLeft, xRight;
    long           yTop, yBottom;
    CSize          sizeOriginal;
    DWORD          grfReturn;
    long           lPadding[PADDING_MAX];

    Assert(GetDisplay());

    grfReturn = (pci->_grfLayout & LAYOUT_FORCE);

    GetSize(&sizeOriginal);

    //
    // If dirty, ensure display tree nodes exist
    //

    if (    _fSizeThis
        &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
    {
        grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
    }

    // We need to get padding into the mixture for calculating
    // percentWidth and percentHeight to get the correct numbers
    long lcxPadding = 0, lcyPadding = 0;
    if(ElementOwner()->IsAbsolute() && (fPercentWidth || fPercentHeight))
    {
        CFlowLayout *pLayoutParent = pContext->Parent()->GetFlowLayout();
        CDisplay    *pdpParent     = pLayoutParent->GetDisplay();

        pdpParent->GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);

        if(fPercentWidth)
            lcxPadding = lPadding[PADDING_LEFT] + lPadding[PADDING_RIGHT];

        if(fPercentHeight)
            lcyPadding = lPadding[PADDING_TOP] + lPadding[PADDING_BOTTOM];
    }

    //
    // aligned & absolute layouts are sized based on parent's available width,
    // and inline elements are sized based on the current line wrapping width.
    // psize->cx holds the wrapping width.
    //
    cxUser = (fPercentWidth && !fNormalMode)
           ? 0
           : uvWidth.XGetPixelValue(
                   pci,
                   pFF->_fAlignedLayout ? pci->_sizeParent.cx : psize->cx,
                   GetFirstBranch()->GetFontHeightInTwips(&uvWidth));
    cyUser = (fPercentHeight && !fNormalMode)
           ? 0
           : uvHeight.YGetPixelValue(
                   pci,
                   pci->_sizeParent.cy + lcyPadding,
                   GetFirstBranch()->GetFontHeightInTwips(&uvHeight));

    xLeft = 0;
    xRight = psize->cx;

    yTop = 0;
    yBottom = psize->cy;

// BUGBUG (a-pauln) Does this flag need to be set further down and with different cause?
//                  What about fClipContent? Rename to _fContentsAffectHeight?

    // set the width and height for our text layout
    //
    if (ElementOwner()->IsAbsolute() && (uvWidth.IsNullOrEnum() || uvHeight.IsNullOrEnum()))
    {
        CalcAbsoluteSize(pci, psize, &cxUser, &cyUser, &xLeft, &xRight, &yTop, &yBottom, fClipContentX, fClipContentY);
    }
    else
    {
        _fContentsAffectSize =  (   uvHeight.IsNullOrEnum()
                                ||  uvWidth.IsNullOrEnum()
                                ||  !fClipContentX
                                ||  !fClipContentY);
    }

    // if contents affect size, start with 0 height and expand.
    //
    cxInit    =
    psize->cx = uvWidth.IsNullOrEnum()
                    ? max(0L, xRight - xLeft)
                    : max(0L, cxUser);
    psize->cy = uvHeight.IsNullOrEnum()
                    ? 0
                    : max(0L, cyUser);

    // Decide if our size changed
    // We used to have a flag for both width and height but
    // it is too fragile to predict if the width has changed
    // or not.  It is also too hard to know whether our
    // height has changed or not when _fContentsAffectSize.
    //
    fHeightChanged = FALSE;

    if(fNormalMode)
    {
        fHeightChanged = _fContentsAffectSize
                       ? TRUE
                       : (cyUser != sizeOriginal.cy);
    }

    // Mark percentage sized children as in need of sizing
    // CONSIDER: as an optimization, if we knew for sure the height
    //   did indeed change, we could not do this here and be sure
    //   that the full recalc would take care of all children.
    //
    if(fNormalMode && !ContainsVertPercentAttr() && fHeightChanged)
    {
        long fContentsAffectSize = _fContentsAffectSize;

        _fContentsAffectSize = FALSE;
        ResizePercentHeightSites();
        _fContentsAffectSize = fContentsAffectSize;
    }

    // Cache sizes and recalculate the text (if required)
    // ** with lock **
    //
    {
        CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

        //
        // Auto-size to content if we are width auto and either float
        // left/right or absolutely positioned.
        //
        if (   uvWidth.IsNullOrEnum()
            && (   bFloat == styleStyleFloatLeft
                || bFloat == styleStyleFloatRight
                || !ElementOwner()->IsBlockElement()
                || ElementOwner()->IsAbsolute()))
        {
            // If we're in home publisher, we don't want to size to content.
            if ( (Doc()->_fInHomePublisherDoc || g_fInHomePublisher98) &&
                ElementOwner()->IsAbsolute())
                _fSizeToContent = FALSE;
            else
                _fSizeToContent = TRUE;
        }

        //
        // Calculate the text
        //

        CalcTextSize(pci, psize);

        //
        // For normal modes, cache values and request layout
        //

        if (fNormalMode)
        {
            //
            // For each direction,
            // If a explicit size exists and clipping, use the explicit size
            // Otherwise, adjust size to content
            //

            if (fClipContentX)
            {
                psize->cx = uvWidth.IsNullOrEnum() ? psize->cx : cxUser;
            }
            else
            {
                if (_fSizeToContent)
                {
                    CRect rc(*psize);
                    CRect rcBorders;

                    rc.right = psize->cx = _dp.GetWidth();

                    _dp.SetViewSize(rc);

                    SizeContentDispNode(*psize);

                    _dp.RecalcLineShift(pci, 0);

                    rcBorders.top    = 0;
                    rcBorders.left   = 0;
                    rcBorders.right  = 0x7FFFFFFF;
                    rcBorders.bottom = 0x7FFFFFFF;

                    GetClientRect(&rcBorders, CLIENTRECT_USERECT, pci);

                    psize->cx += 0x7FFFFFFF - (rcBorders.right - rcBorders.left);
                }
                else if (!uvWidth.IsNullOrEnum() && psize->cx < cxUser)
                {
                    psize->cx = max(0L, cxUser);
                }

            }

            if (fClipContentY)
            {
                psize->cy = uvHeight.IsNullOrEnum() ? psize->cy : cyUser;
            }
            else
            {
                if (!uvHeight.IsNullOrEnum() && (psize->cy < cyUser))
                {
                    psize->cy = max(0L, cyUser);
                }
            }

        }

        _fSizeToContent = FALSE;
    }

    // Resize the child height percent sites again.  Do another
    // CalcTextSize to make sure they are actually sized.
    // ** No Lock **
    //
    if (fNormalMode && _fContentsAffectSize)
    {
        if(ContainsVertPercentAttr() || _fChildHeightPercent)
        {
            SIZE    size;

            size.cx = cxInit;
            size.cy = psize->cy;
            {
                long fContentsAffectSize = _fContentsAffectSize;

                _fContentsAffectSize = FALSE;
                ResizePercentHeightSites();
                _fContentsAffectSize = fContentsAffectSize;
            }

            CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

            CalcTextSize(pci, &size);

            psize->cy = max(psize->cy, size.cy);

            if (fClipContentY)
            {
                psize->cy = uvHeight.IsNullOrEnum() ? psize->cy : cyUser;
            }

            // Intent is to insure that both calculations return the same width.
            // This is (validly) not the case when we are absolutely positioned and have
            //  children dependent on our height and we are not clipping  and the normal
            //  width of content is less than the width of the percent sized child.
            //
            Assert(    (psize->cx >= size.cx)
                    || (   ElementOwner()->IsAbsolute() 
                        && _fChildHeightPercent
                        && fNormalMode
                        && !fClipContentX));
        }
    }

    // Finish up the recalc ** with lock **
    //
    {
        CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

        if(fNormalMode)
        {
            grfReturn |= LAYOUT_THIS
                      | (psize->cx != sizeOriginal.cx
                                ? LAYOUT_HRESIZE
                                : 0)
                      | (psize->cy != sizeOriginal.cy
                                ? LAYOUT_VRESIZE
                                : 0);

            //
            // If size changes occurred, size the display nodes
            //

            if (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
            {
                SizeDispNode(pci, *psize);
                SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
            }

            // Mark the site clean
            // (This is done last so that other code, such as GetClientRect,
            // will "do the right thing" while the size is being determined)
            //
            _fSizeThis = FALSE;
        }

        // For min/max mode, cache the values and note that they are now valid
        //
        else if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            // If the user doesn't specify a width, or the width
            // is percentage size, we return min/max info
            // as calculated for any other regular CTxtSite (by
            // CalcTextSize)
            //
            if (!uvWidth.IsNullOrEnum() && !fPercentWidth)
            {
                // We have two cases here:
                // 1) we are clipping - just use the size specified
                //      for both min/max
                // 2) we are growing but know our width so use
                //      max(psize->cy, cxUser) for both min/max
                //
                psize->cy =
                psize->cx = fClipContentX
                                ? max(0L, cxUser)
                                : max(psize->cy, cxUser);
             }
            _xMax = psize->cx;
            _xMin = psize->cy;

            _fMinMaxValid = TRUE;
        }

        else if (pci->_smMode == SIZEMODE_MINWIDTH)
        {
            if (!uvWidth.IsNullOrEnum() && !fPercentWidth)
            {
                psize->cx = fClipContentX
                                ? max(0L, cxUser)
                                : max(psize->cx, cxUser);
            }
            _xMin = psize->cx;
        }
    }
    return grfReturn;
}

//+---------------------------------------------------------------------------
//
//  Member:     C1DLayout::CalcAbsoluteSize
//
//  Synopsis:   Computes width and height of absolutely positioned element
//
//----------------------------------------------------------------------------
void
C1DLayout::CalcAbsoluteSize(CCalcInfo * pci,
                            SIZE* psize,
                            long* pcxWidth,
                            long* pcyHeight,
                            long* pxLeft,
                            long* pxRight,
                            long* pyTop,
                            long* pyBottom,
                            BOOL  fClipContentX,
                            BOOL  fClipContentY)
{
    CTreeNode   *pContext      = GetFirstBranch();
    styleDir     bDirection    = pContext->GetCascadedBlockDirection();
    CUnitValue   uvWidth       = pContext->GetCascadedwidth();
    CUnitValue   uvHeight      = pContext->GetCascadedheight();
    CUnitValue   uvLeft        = pContext->GetCascadedleft();
    CUnitValue   uvRight       = pContext->GetCascadedright();
    CUnitValue   uvTop         = pContext->GetCascadedtop();
    CUnitValue   uvBottom      = pContext->GetCascadedbottom();
    BOOL         fWidthAuto    = uvWidth.IsNullOrEnum();
    BOOL         fHeightAuto   = uvHeight.IsNullOrEnum();
    BOOL         fLeftAuto     = uvLeft.IsNullOrEnum();
    BOOL         fRightAuto    = uvRight.IsNullOrEnum();
    BOOL         fTopAuto      = uvTop.IsNullOrEnum();
    BOOL         fBottomAuto   = uvBottom.IsNullOrEnum();
    CFlowLayout *pLayoutParent = pContext->Parent()->GetFlowLayout();
    CDisplay    *pdpParent     = pLayoutParent->GetDisplay();
    long         lPadding[PADDING_MAX];
    POINT        pt;
    
    pt = g_Zero.pt;

    _fContentsAffectSize =  (   fHeightAuto
                            &&  fBottomAuto)
                        ||  (   fWidthAuto
                            &&  (   (   bDirection == htmlDirLeftToRight
                                    &&  fRightAuto)
                                ||  (   bDirection == htmlDirRightToLeft
                                    &&  fLeftAuto)))
                        ||  !fClipContentX
                        ||  !fClipContentY;

// BUGBUG: The pci should pass the yOffset as well! (brendand)
    if (    (   fHeightAuto
            &&  fBottomAuto)
        ||  (   fLeftAuto
            &&  bDirection == htmlDirLeftToRight)
        ||  (   fRightAuto
            &&  bDirection == htmlDirRightToLeft))
    {
         CLinePtr   rp(pdpParent);
         CTreePos * ptpStart;
         CTreePos * ptpEnd;

         ElementOwner()->GetTreeExtent(&ptpStart, &ptpEnd);

         if (pdpParent->PointFromTp(ptpStart->GetCp(), ptpStart, FALSE, FALSE, pt, &rp, TA_TOP) < 0)
         {
            pt = g_Zero.pt;
         }
         else if (bDirection == htmlDirRightToLeft)
         {
            pt.x = psize->cx - pt.x;
         }
    }

    if(fWidthAuto)
    {
        if (pci->_fUseOffset)
        {
            // measurer sets up _xLayoutOffsetInLine, use it instead of computing the
            // the xOffset. The xOffset computed through PointFromTp below, does not
            // take line shift (used for alignment) into account when the line containing
            // the current layout is being measured.
            pt.x = pci->_xOffsetInLine;
        }

        //
        // pt.x includes the leading padding of the layout owner, but the available
        // size does not include the padding. So, subtract the leadding padding for
        // pt.x to make it relative to the leading padding(that is the zero of where
        // content actualy starts.
        //
        pdpParent->GetPadding(pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH);

        pt.x -= (bDirection == htmlDirLeftToRight)
                    ? lPadding[PADDING_LEFT]
                    : lPadding[PADDING_RIGHT];

        *pxLeft = !fLeftAuto
                        ? uvLeft.XGetPixelValue(
                                pci,
                                psize->cx,
                                GetFirstBranch()->GetFontHeightInTwips(&uvLeft))
                        : 0;

        *pxRight = !fRightAuto
                        ? psize->cx - 
                          uvRight.XGetPixelValue(
                                pci,
                                psize->cx,
                                GetFirstBranch()->GetFontHeightInTwips(&uvRight))
                        : psize->cx;
    }

    if(fHeightAuto)
    {
        *pyTop = !fTopAuto
                        ? uvTop.XGetPixelValue(
                                pci,
                                psize->cy,
                                GetFirstBranch()->GetFontHeightInTwips(&uvTop))
                        : pt.y;

        *pyBottom = !fBottomAuto
                        ? psize->cy - 
                          uvBottom.XGetPixelValue(
                                pci,
                                psize->cy,
                                GetFirstBranch()->GetFontHeightInTwips(&uvBottom))
                        : psize->cy;
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Handle notification
//
//----------------------------------------------------------------------------

void
C1DLayout::Notify(CNotification *pNF)
{
    HRESULT     hr = S_OK;
    IStream *   pStream = NULL;

    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_SAVE_HISTORY_1:
        pNF->SetSecondChanceRequested();
        break;

    case NTYPE_SAVE_HISTORY_2:
        {
            CDataStream ds;
            CHistorySaveCtx *phsc;

            pNF->Data((void **)&phsc);
            hr = THR(phsc->BeginSaveStream(ElementOwner()->GetSourceIndex(), 
                                           ElementOwner()->HistoryCode(), 
                                           &pStream));
            if (hr)
                goto Cleanup;

            ds.Init(pStream);

            // save scroll pos
            hr = THR(ds.SaveDword(GetYScroll()));
            if (hr)
                goto Cleanup;

            hr = THR(phsc->EndSaveStream());
            if (hr)
                goto Cleanup;
        }
        break;

    case NTYPE_DELAY_LOAD_HISTORY:
        {
            IGNORE_HR(Doc()->GetLoadHistoryStream(
                                ElementOwner()->GetSourceIndex(),
                                ElementOwner()->HistoryCode(), 
                                &pStream));

            if (pStream && !Doc()->_fUserInteracted)
            {
                CDataStream ds(pStream);
                DWORD       dwScrollPos;

                // load scroll pos
                hr = THR(ds.LoadDword(&dwScrollPos));
                if (hr)
                    goto Cleanup;
                if (    _pDispNode
                    &&  GetElementDispNode()->IsScroller())
                {
                    ScrollToY(dwScrollPos);
                }
            }
        }
        break;
    }

Cleanup:
    ReleaseInterface(pStream);
    return;
}



void
C1DLayout::ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected,  BOOL fLayoutCompletelyEnclosed, BOOL fFireOM )
{
    Assert(ptpStart && ptpEnd && ptpStart->GetMarkup() == ptpStart->GetMarkup());
    CElement* pElement = ElementOwner();
    //
    // For IE 5 we have decided to not draw the dithered selection at browse time for DIVs
    // people thought it weird.
    //
    // We do however draw it for XML Sprinkles.
    //
    if (( Doc()->_fDesignMode || pElement->_etag == ETAG_GENERIC ) && 
        ( fSelected && fLayoutCompletelyEnclosed ) ||
        ( !fSelected && ! fLayoutCompletelyEnclosed ) )
        SetSelected( fSelected, TRUE );
    else
    {
        _dp.ShowSelected( ptpStart, ptpEnd, fSelected);
    }
}
