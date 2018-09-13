//+----------------------------------------------------------------------------
// File: qdispscrl.cxx
//
// Description: All scrolling related functions on CDisplay
//
// This is the version customized for hosting Quill.
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef _X_SELECTN_HXX_
#define _X_SELECTN_HXX_
#include "selectn.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifdef QUILL

#ifndef X_QUILGLUE_HXX_
#define X_QUILGLUE_HXX_
#include "quilglue.hxx"
#endif

#endif  // QUILL

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#if DBG!=1
#define CheckView()
#define CheckLineArray()
#endif

#ifndef DISPLAY_TREE
// Auto scroll constants
#define dwAutoScrollUp      1
#define dwAutoScrollDown    2
#define dwAutoScrollLeft    3
#define dwAutoScrollRight   4
#endif

MtDefine(CDisplayScrollAction_aryInvalRects_pv, Locals, "CDisplay::ScrollAction aryInvalRects::_pv")

ExternTag(tagLayout);

#ifndef DISPLAY_TREE
/*
 *  CDisplay::AdjustScroll()
 *
 *  @mfunc
 *      Adjust the scroll position if we are scrolled past the
 *      bottom or right edge of the document
 */
void CDisplay::AdjustScroll(CPositionInfo * ppi)
{
    CFlowLayout * pFL = GetFlowLayout();
    LONG          xScroll   = GetXScroll();
    LONG          yScroll   = GetYScroll();

    _fRTL = !!pFL->GetFirstBranch()->GetParaFormat()->HasRTL(TRUE);

    // If print doc, always AdjustScroll(), otherwise, if normal doc, then don't
    // try to adjust scrolling till we are inplace.
    if (!pFL->Doc()->IsPrintDoc() && pFL->Doc()->_state < OS_INPLACE)
        return;

    // If my contents affect my size then I will not have any scroll, so
    // no point in adjusting scroll values.
    if (pFL->_fContentsAffectSize)
        return;

    if (  !_fRecalcDone
        && GetYScroll() > pFL->GetMaxYScroll()
       )
    {
        WaitForRecalc(-1, GetYScroll() + GetViewHeight(), NULL);
    }

    if (GetXScroll() > pFL->GetMaxXScroll() && !_fRTL)
    {
        xScroll = max(0L, pFL->GetMaxXScroll());
        if (xScroll != GetXScroll())
            ScrollView(ppi, xScroll, -1, FALSE, FALSE, FALSE, TRUE);
    }
    // adjust the HScrollbar if the document is RTL
    else if(_fRTL)
    {
        // If the scrollbar has not been initialized or window is resized leaving xScroll larger
        // than max, we want to initalize the scroll to max (thumb button to the right).
        if((pFL->GetMaxXScroll() == 0  ||
            pFL->GetMaxXScroll() <= xScroll ||
            pFL->GetMaxXScroll() > 0 && xScroll == 0) && !_fxScrollMax)
        {
            xScroll = pFL->GetMaxXScroll();
            ScrollView(ppi, xScroll, GetYScroll(), FALSE, FALSE, FALSE, TRUE);
            _fxScrollMax = TRUE;
        }
        // if we are already maxed out and a resize occurs we want to keep the scroll maxed out
        else if(_fxScrollMax)
        {
            xScroll = max(0L, pFL->GetMaxXScroll());
            if (xScroll != GetXScroll())
                ScrollView(ppi, xScroll, GetYScroll(), FALSE, FALSE, FALSE, TRUE);
        }
    }

    if (GetYScroll() > pFL->GetMaxYScroll())
    {
        yScroll = max(0L, pFL->GetMaxYScroll());
        if (yScroll != GetYScroll())
            ScrollView(ppi, -1, yScroll, FALSE, FALSE, FALSE, TRUE);
    }
}
#endif


//===================================  Scrolling  =============================

#ifndef DISPLAY_TREE
/*
 *  CDisplay::HScrollN(wCode, iliCount)
 *
 *  Purpose:
 *      Scroll the view horizontally by iliCount number of characters
 *      >>> Should be called when in-place active only <<<
 *
 *  Arguments:
 *      wCode    scrollbar event code (up or down)
 *      iliCount number of characters to scroll by
 */
VOID CDisplay::HScrollN (WORD wCode, LONG iliCount)
{
    CFlowLayout * pFL = GetFlowLayout();
    LONG xScroll = GetXScroll() ;

    AssertSz(pFL->IsInPlace(), "CDisplay::HScrollN() called when not in place") ;

    xScroll += _xWidthSysChar * iliCount * (wCode == SB_LINEDOWN ? 1 : -1) ;
    xScroll = max (0, (int)xScroll) ;
    xScroll = min ((int)xScroll, (int)pFL->GetMaxXScroll()) ;
    ScrollView (NULL, xScroll, -1, FALSE, FALSE) ;
}
#endif

#ifndef DISPLAY_TREE
/*
 *  CDisplay::HScroll(wCode, xPos)
 *
 *  Purpose:
 *      Scroll the view horizontally in response to a scrollbar event
 *      >>> Should be called when in-place active only <<<
 *
 *  Arguments:
 *      wCode           scrollbar event code
 *      yPos            thumb position
 *      fRepeat         TRUE if the previous scrolling action is repeated
 */
VOID CDisplay::HScroll(WORD wCode, LONG xPos, BOOL fRepeat)
{
    CFlowLayout * pFL = GetFlowLayout();
    BOOL          fTracking = FALSE;
    LONG          xScroll = GetXScroll();

    _fxScrollMax = FALSE;

    AssertSz(pFL->IsInPlace(), "CDisplay::HScroll() called when not in place");

    switch(wCode)
    {
    case SB_BOTTOM:
#if 0
//$ REVIEW: this actually makes a little bit of sense, do we want it?
        WaitForRecalc(GetLastCp(), -1, FALSE);
#endif
        xScroll = pFL->GetMaxXScroll();
        break;

    case SB_LINEDOWN:
//$ REVIEW: any better way to do this?
        xScroll += _xWidthSysChar;
        break;

    case SB_LINEUP:
//$ REVIEW: any better way to do this?
        xScroll -= _xWidthSysChar;
        break;

    case SB_PAGEDOWN:
        xScroll += _xWidthView;
//$ REVIEW: backup one character ?
        break;

    case SB_PAGEUP:
        xScroll -= _xWidthView;
//$ REVIEW: move forward one character
        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        if(xPos < 0)
            return;
        xScroll = xPos;
        fTracking = TRUE;
        break;

    case SB_TOP:
        xScroll = 0;
        break;

    case SB_ENDSCROLL:
        pFL->OnScrollChange(fmScrollBarsHorizontal);
        return;

    default:
        return;
    }

    if (xScroll < 0)
    {
        // xScroll is the new proposed scrolling position and
        // therefore cannot be less than 0.
        xScroll = 0;
    }
    else if(xScroll >= pFL->GetMaxXScroll())
    {
        xScroll = pFL->GetMaxXScroll();
        _fxScrollMax = TRUE;
    }

    ScrollView(NULL, xScroll, -1, fTracking, FALSE);
}
#endif

#ifndef DISPLAY_TREE
/*
 *  CDisplay::VScrollPercent (wCode, yFrac)
 *
 *  @mfunc
 *      Scroll the view vertically by a fraction of the screen height
 *
 *  @devnote
 *      --- Use when in-place active only ---
 *
 */
VOID CDisplay::VScrollPercent (WORD wCode, LONG yPercent, BOOL fRepeat)
{
    AssertSz(GetFlowLayout()->IsInPlace(), "CDisplay::VScrollPercent() called when not in-place");
    LONG dy;
    LONG yScroll;

    dy = (_yHeightView * yPercent + 500) / 1000;
    dy = max (1l, (long)dy);
    yScroll = GetYScroll();
    switch (wCode)
    {
        case SB_LINEDOWN:
        case SB_PAGEDOWN:
            yScroll += dy;
            break;
        case SB_LINEUP:
        case SB_PAGEUP:
            yScroll -= dy;
            break;
    }

    yScroll = max (0l, (long)yScroll);

    ScrollView (NULL, -1, yScroll, FALSE, TRUE, fRepeat);
}
#endif

#ifndef DISPLAY_TREE
/*
 *  CDisplay::VScrollN(wCode, iliCount)
 *
 *  @mfunc
 *      Scroll the view vertically by iliCount number of lines
 *
 *  @devnote
 *      --- Use when in-place active only ---
 *
 */
VOID CDisplay::VScrollN(
    WORD wCode,     //@parm Scrollbar event code
    LONG iliCount)  //@parm Number of lines to scroll the display by
{
    CFlowLayout * pFL = GetFlowLayout();
    LONG cliVisible ;
    LONG dy = 0 ;
    LONG yScroll = GetYScroll() ;
    LONG iliNail ;
    LONG iliFirst ;
    LONG i ;
    BOOL fDown = FALSE ;
    CLine * pli;

    AssertSz(pFL->IsInPlace(), "CDisplay::VScrollN() called when not in-place");
    Assert (iliCount > 0) ;

    // Decide which is the first line visible. We cannot use GetFirstVisibleLine(),
    // because for images whose tops are not rendered (as would happen if we
    // are going to scroll by multiple lines at a time), have their line heights
    // set as SHORT_MAX, thereby totally throwing off GetFirstVisibleLine() computation.
    // BUGBUG sujalp: Replace call to LineFromPos() with GetFirstVisibleLine() when
    //                that variable is updated appropriately.
    iliFirst = LineFromPos (0, GetYScroll(), FALSE, TRUE, NULL, NULL) ;
    iliNail  = iliFirst ;

    cliVisible = GetCliVisible() ;

    switch (wCode)
    {
    case SB_LINEDOWN:

        for (i = 0; i < iliCount; i++)
        {
            // Are there more lines to scroll into the view ?
            if(iliFirst + cliVisible + i < (LONG)Count())
            {
                // If the _yHeight parameter was SHORT_MAX in the line
                // below, we are still safe because the code below
                // cuts-short any scrolls which are greater than one
                // screen full.
                pli = Elem(iliFirst + cliVisible + i);
                if(pli->_fForceNewLine)
                    dy += pli->_yHeight ;
                iliNail = iliFirst + cliVisible + i ;
            }
            else if (cliVisible > 1)
            {
                // No more lines to scroll into the view. However, number of lines
                // visible in the screen is > 1. That means that the last line
                // is tall and only part of it is being seen presently (as can happen
                // when using an exceptionally large font or a big image on the last line).
                // In this case, we scroll off the line on the *top* of the screen.
                // The line at the top of the screen is identified by iliNail.
                pli = Elem(iliNail);
                if(pli->_fForceNewLine)
                    dy += pli->_yHeight ;

                // If we still have more lines to scroll, then the line after present
                // iliNail is the one by whose height we scroll the next time, if such
                // a line exists.
                if (iliNail < (LONG)Count() - 1)
                {
                    iliNail++ ;
                }
            }
            else
            {
                // Only one line is visible on the screen and that is the last line
                // in the document. Hence, scroll down as much as possible. Remember,
                // a later condition verfies that the scroll is *never* more than
                // the screen height.
                dy += _yHeightMax - GetYScroll() ;
                break ;
            }
        }

        // Tell the scrollview method not to align the first visible line
        // with the top of the document.
        fDown = TRUE ;
        break ;

    case SB_LINEUP:
        for (i = 0; i < iliCount; i++)
        {
            // Are there are lines which we can scroll down?
            if(iliNail > 0)
            {
                // Yes, also adjust the line to scroll down next
                pli = Elem(iliNail - 1);
                if(pli->_fForceNewLine)
                    dy += pli->_yHeight ;
                iliNail-- ;
            }
            else if (yScroll > 0)
            {
                // No more lines which can scroll down, however,
                // we still have the top line which is still partially
                // hidden. We want to scroll it into view. But we have
                // already setup dy to some scroll amount (0 possibly),
                // so adjust for it! Now if the first line is very tall
                // (which will be reflected in the yScroll value),
                // then we want to scroll only iliCount number of standard
                // sized lines.
                dy += min (int(yScroll - dy), int(_yHeightSysChar * (iliCount - i))) ;
                Assert (dy >= 0) ;
                break ;
            }
        }
        break ;
    }

    // Make sure we never scroll more than one screen full
    if (dy >= _yHeightView)
    {
        dy = _yHeightSysChar * iliCount ;

        // Is the scroll amount still greater than one screen full?
        if (dy >= _yHeightView)
        {
            // OK, then scroll by as many standard-sized lines as will
            // fit in the screen
            dy = (_yHeightView / _yHeightSysChar) * _yHeightSysChar ;
        }
    }
    Assert (dy <= _yHeightView) ;

    // nothing to scroll, early exit
    if ( !dy )
        return ;

    yScroll += wCode == SB_LINEDOWN ? dy : (-dy) ;

    ScrollView (NULL, -1, yScroll, FALSE, fDown) ;
}
#endif

#ifndef DISPLAY_TREE
/*
 *  CDisplay::VScroll(wCode, yPos)
 *
 *  @mfunc
 *      Scroll the view vertically in response to a scrollbar event
 *
 *  @devnote
 *      --- Use when in-place active only ---
 *
 *  @rdesc
 *      LRESULT formatted for WM_VSCROLL message
 */
LRESULT CDisplay::VScroll(
    WORD wCode,     //@parm Scrollbar event code
    LONG yPos,      //@parm Thumb position (yPos <lt> 0 for EM_SCROLL behavior)
    BOOL fFixed,    //@parm Fixed amount scrolling?
    BOOL fRepeat)   //@parm TRUE if the previous scrolling action is repeated
{
    CFlowLayout *   pFL = GetFlowLayout();
    const LONG      ili = GetFirstVisibleLine();
    LONG            cliVisible;
    LONG            yScroll = GetYScroll();
    BOOL            fTracking = FALSE;
    LONG            dy = 0;
    BOOL            fDown=FALSE;
    LRESULT         lRetVal;
    CLine *         pli;

    AssertSz(pFL->IsInPlace(), "CDisplay::VScroll() called when not in-place");

    //yPos = min(yPos, _yHeightMax);
    yPos = min(yPos, pFL->GetMaxYScroll() + GetViewHeight());

    switch(wCode)
    {
    case SB_BOTTOM:
        if(yPos < 0)
            return FALSE;
        WaitForRecalc(GetLastCp(), -1);
        yScroll = pFL->GetMaxYScroll();
        break;

    case SB_LINEDOWN:
        if (fFixed)
        {
            // BUGBUG: What is this "magic" number? (brendand)
            VScrollPercent (wCode, 125, fRepeat);
            lRetVal = MAKELRESULT ((WORD)0, TRUE);
            goto Cleanup;
        }
        else
        {
            cliVisible = GetCliVisible();
            if(GetFirstVisibleLine() + cliVisible < LineCount())
            {
                pli = Elem(GetFirstVisibleLine() + cliVisible);
                if(pli->_fForceNewLine)
                    dy = pli->_yHeight;
            }
            else if(cliVisible > 1)
            {
                pli = Elem(GetFirstVisibleLine());
                if(pli->_fForceNewLine)
                    dy = pli->_yHeight;
            }
            else
                dy = _yHeightMax - GetYScroll();
            if(dy >= _yHeightView)
                dy = _yHeightSysChar;

            // nothing to scroll, early exit
            if ( !dy )
                return MAKELRESULT(0, TRUE);

            yScroll += dy;

            // Tell the scrollview method not to align the first visible line
            // with the top of the document.
            fDown = TRUE;
        }
        break;

    case SB_LINEUP:
        if (fFixed)
        {
            // BUGBUG: What is this "magic" number? (brendand)
            VScrollPercent (wCode, 125, fRepeat);
            lRetVal = MAKELRESULT ((WORD)0, TRUE);
            goto Cleanup;
        }
        else
        {
            if(GetFirstVisibleLine() > 0)
                dy = Elem(GetFirstVisibleLine() - 1)->_yHeight;
            else if(yScroll > 0)
                dy = min(int(yScroll), _yHeightSysChar);
            if(dy > _yHeightView)
                dy = _yHeightSysChar;
            yScroll -= dy;
        }
        break;

    case SB_PAGEDOWN:
        if (fFixed)
        {
            // BUGBUG: What is this "magic" number? (brendand)
            VScrollPercent (wCode, 875, fRepeat);
            lRetVal = MAKELRESULT ((WORD)0, TRUE);
            goto Cleanup;
        }
        else
        {
            cliVisible = GetCliVisible();
            yScroll += _yHeightView;
            if(yScroll < _yHeightMax && cliVisible > 0)
            {
                pli = Elem(GetFirstVisibleLine() + cliVisible - 1);
                if(pli->_fForceNewLine)
                    dy = pli->_yHeight;
                if(dy >= _yHeightView)
                {
                    dy = _yHeightSysChar;
                }
                else if (dy > _yHeightView - dy)
                {
                    // Go at least a line if the line
                    // is very big.
                    dy = _yHeightView - dy;
                }

                yScroll -= dy;
            }
        }
        break;

    case SB_PAGEUP:
        if (fFixed)
        {
            // BUGBUG: What is this "magic" number? (brendand)
            VScrollPercent (wCode, 875, fRepeat);
            lRetVal = MAKELRESULT ((WORD)0, TRUE);
            goto Cleanup;
        }
        else
        {
            cliVisible = GetCliVisible();
            yScroll -= _yHeightView;
            if(cliVisible > 0)
            {
                pli = Elem(GetFirstVisibleLine());
                if(pli->_fForceNewLine)
                    dy = pli->_yHeight;
                if(dy >= _yHeightView)
                {
                    dy = _yHeightSysChar;
                }
                else if (dy > _yHeightView - dy)
                {
                    // Go at least a line if the line
                    // is very big.
                    dy = _yHeightView - dy;
                }
                yScroll += dy;
            }
        }
        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:

        if(yPos < 0)
            return FALSE;
        if(pFL->_fVertical)
            yScroll = GetScrollRange(SB_VERT) - yPos;
        else
            yScroll = yPos;
        fTracking = TRUE;
        break;

    case SB_TOP:
        if(yPos < 0)
            return FALSE;
        yScroll = 0;
        break;

    case SB_ENDSCROLL:
        pFL->OnScrollChange(fmScrollBarsVertical);
        return MAKELRESULT(0, TRUE);;

    default:
        return FALSE;
    }

    ScrollView(NULL, -1, yScroll, fTracking, fDown);

    // return how many lines we scrolled
    lRetVal = MAKELRESULT((WORD) (GetFirstVisibleLine() - ili), TRUE);

Cleanup:
    return lRetVal;
}
#endif

#ifndef DISPLAY_TREE
/*
 *  CDisplay::ScrollToLineStart()
 *
 *  @mfunc
 *      If the view is scrolled so that only a partial line is at the
 *      top, then scroll the view so that the entire view is at the top.
 */
VOID CDisplay::ScrollToLineStart()
{
    if ( GetFirstVisibleDY())
        ScrollView(NULL, -1, GetYScroll() + GetFirstVisibleDY(), FALSE, TRUE);
}
#endif

#ifndef DISPLAY_TREE
/*
 *  CDisplay::CalcYLineScrollDelta ( LONG cli )
 *
 *  @mfunc
 *      Given a count of lines, positive or negative, calc the number
 *      of vertical units necessary to scroll the view to the start of
 *      the current line + the given count of lines.
 */
LONG CDisplay::CalcYLineScrollDelta ( LONG cli, BOOL fFractionalFirst )
{
    LONG yScroll = 0;

    if ( fFractionalFirst && GetFirstVisibleDY() )      // Scroll partial for 1st.
    {
        Assert ( GetFirstVisibleDY() <= 0 ); // get jonmat

        if ( cli < 0 )
        {
            cli++;
            yScroll = GetFirstVisibleDY();
        }
        else
        {
            cli--;
            Assert(!Elem(GetFirstVisibleLine())->IsFrame());

            yScroll = GetFirstVisibleDY();
            if(Elem(GetFirstVisibleLine()))
                yScroll += Elem(GetFirstVisibleLine())->_yHeight;
        }
    }

    if(cli > 0)
        cli = min(cli, long(LineCount() - GetFirstVisibleLine() - 1));
    else if(cli < 0)
        cli = max(cli, -GetFirstVisibleLine());

    if(cli)
    {
        yScroll += YposFromLine(GetFirstVisibleLine() + cli, NULL) -
                    YposFromLine(GetFirstVisibleLine(), NULL);
    }

    return yScroll;
}
#endif

#ifndef DISPLAY_TREE
/*
 *  CDisplay::ScrollView(xScroll, yScroll, fTracking, fFractionalScroll,
 *                       fRepeat, fDisableSmoothScrolling)
 *
 *  @mfunc
 *      Scroll view to new x and y position
 *
 *  @devnote
 *      This method tries to adjust the y scroll pos before
 *      scrolling to display complete line at top. x scroll
 *      pos is adjusted to avoid scrolling all text off the
 *      view rectangle.
 *
 *      Must be able to handle yScroll <gt> pdp->yHeight and yScroll <lt> 0
 *
 *  @rdesc
 *      TRUE if actual scrolling occurred,
 *      FALSE if no change
 */
BOOL CDisplay::ScrollView (
    CPositionInfo * ppi,
    LONG xScroll,       //@parm New x scroll position
    LONG yScroll,       //@parm New y scroll position
    BOOL fTracking,     //@parm TRUE indicates we are tracking scrollbar thumb
    BOOL fFractionalScroll,
    BOOL fRepeat,       //@parm TRUE if the previous scrolling action is repeated
    BOOL fDisableSmoothScrolling)
{
    CFlowLayout       * pFL = GetFlowLayout();
    CElement          * pElementFL = pFL->ElementOwner();
    CDoc              * pDoc = pFL->Doc();
    CPositionInfo       PI(pFL);
    CPositionInfo     * lppi;
    BOOL                fTryAgain = TRUE;
    SMOOTHSCROLLINFO    sinfo;
    BOOL                fSmoothScrolling = !fDisableSmoothScrolling &&
                                           pDoc->_pOptionSettings->fSmoothScrolling &&
                                           !_fPrinting;
    BOOL                fInBrowseMode   = !GetPed()->IsEditable();
    CElement *          pElemCurrent    = pDoc->_pElemCurrent;
    BOOL                fScrolled       = FALSE;

    lppi = ppi ? ppi : &PI;

    // Passing -1 in means no change in scroll pos
    if(xScroll == -1)
        xScroll = GetXScroll();
    if(yScroll == -1)
        yScroll = GetYScroll();

    // If the new position is the same as our current position, just return
    // (This is safer than letting it pass through with the hope that the following calculations, which
    //  could involve round-off errors, will result in no movement)
    if (xScroll == GetXScroll() &&
        yScroll == GetYScroll())
        goto Cleanup;

    // If we are a table cell derivative then we should not scroll at all
    if (pElementFL->TestClassFlag(CElement::ELEMENTDESC_TABLECELL)
        || pFL->IsDisplayNone())
        goto Cleanup;

    sinfo._dx = 0;
    sinfo._fTracking = fTracking;
    sinfo._psel = NULL;
    sinfo._pFlowLayoutSelOwner = NULL;
    sinfo._fRepeat = fRepeat;

    // If we get here, we are updating some general characteristic of the
    // display and so we want the cursor updated as well as the general
    // change otherwise the cursor will land up in the wrong place.
    _fUpdateCaret = TRUE;

    AssertSz(pFL->IsInPlace(), "CDisplay::ScrollView() called when not in-place");

    pFL->GetVisibleClientRect(&sinfo._rcView, lppi);

    // Get selection record for the current site
    if (pElemCurrent)
    {
        sinfo._pFlowLayoutSelOwner = (pElemCurrent->IsMaster() ?
                                        pElemCurrent->GetSlave() :
                                        pElemCurrent                )->HasFlowLayout();
        if (sinfo._pFlowLayoutSelOwner)
        {
            IGNORE_HR(sinfo._pFlowLayoutSelOwner->GetSel(&sinfo._psel, FALSE));
        }
    }

    if(pElementFL->Tag() == ETAG_BODY)
    {
        DYNCAST(CBodyElement, pElementFL)->Layout()->SetFocusRect(FALSE);
    }

    if (fSmoothScrolling && !fTracking)
    {
        fScrolled = SmoothScrolling(lppi, xScroll, yScroll, &sinfo);
    }
    else
    {
        RECT rcInvalid = {0,0,0,0};
        fScrolled = ScrollAction(lppi, xScroll, yScroll, &sinfo, &rcInvalid);
    }

#ifdef DBCS
    UpdateIMEWindow();
#endif

    // if we are not printing the fire the onscroll event
    if (!pFL->Doc()->IsPrintDoc())
        pFL->Fire_onscroll();

Cleanup:
    return fScrolled;
}
#endif



#ifndef DISPLAY_TREE
//+---------------------------------------------------------------------------
//
//  Member:     CDisplay::SmoothScrolling
//
//  Synopsis:   Smoothly scroll view to the new x and y position
//
//  Arguments:  [xScroll]       new horizontal scroll position
//              [yScroll]       new vertical scroll position
//              [psinfo]        pointer to smooth scroll info struct
//
//  Returns:    TRUE if actual scrolling occurred,
//              FALSE if no change
//
//----------------------------------------------------------------------------

// the maximum number os scrolls that we allow during 1 scroll opertaion
// (IE3.0 parity).
const int MAX_NUMBER_OF_SUBSCROLLS = 50;

BOOL
CDisplay::SmoothScrolling(CPositionInfo * ppi,
                          LONG  xScroll,
                          LONG yScroll,
                          SMOOTHSCROLLINFO *psinfo)
{
    CFlowLayout * pFL = GetFlowLayout();
    DWORD       dwSpendTime, dwStartTime, dwCurrentTime, dwTargetTime; //ticks
    DWORD       dwOptimalTargetTime, dwInBetweenTime;
    LONG        dStep, dScroll, delta, dMinStep, dx, dy;             // pixels
    int         ix, iy, iDirection;                              // muliplyers
    int         cMoreScrolls, cDoneScrolls, cAnticipateScrolls;    // counters
    BOOL        fMoreToScroll;    // flag, set to TRUE if we need more scrolls
    BOOL        fScrolled = FALSE;// return value of this function
    THREADSTATE * pts = GetThreadState();
    RECT        rcInvalid;

    Assert (pts);
    if (!pts->scrollTimeInfo.lRepeatDelay)
    {
        // set the scrollbar timeing values only once.
        InitScrollbarTiming();
        Assert(pts->scrollTimeInfo.lRepeatDelay);
    }

    dy = GetYScroll() - yScroll;        // delta pixels to scroll
    dx = GetXScroll() - xScroll;
    if (dx || dy)                   // if we need to scroll
    {
        Assert(dx == 0 || dy == 0); // make sure only one of them is 0
        if (dx)
        {
            delta = dx;             // the target delta to scroll
            ix = 1;                 // dx multiplyer
            iy = 0;                 // dy multiplyer
        }
        else
        {
            delta = dy;
            iy = 1;
            ix = 0;
        }


        // Initialize the time of scroll operation and the number of
        // performed scroll actions.
        dwCurrentTime = dwStartTime = GetTickCount();
        cDoneScrolls = 0;

#if 0
        // if this code is parsed, it will result in a much smoother scrolling
        dwTargetTime = pts->scrollTimeInfo.lRepeatDelay;     // (250 ticks)
#else
        // this code, will cause smoothing to be not perfect, unless we optimize ScrollAction
        // this is IE30 100% compatibility.
        dwTargetTime = pts->scrollTimeInfo.lRepeatDelay/2;   // (125 ticks)
#endif
        cAnticipateScrolls = MAX_NUMBER_OF_SUBSCROLLS;

        if (psinfo->_fRepeat)
        {
            // the _fRepeat flag is set when we are holding mouse down on the
            // scrollbar which will initate the timer who will fire scroll
            // events once in _RepeatRate (== 50) ticks.

            // Calculate the new target time: should be equal to (50ticks)
            // minus the time spend between the end of last scroll and the
            // this repeated request to scroll
            dwTargetTime = pts->scrollTimeInfo.lRepeatRate;
            dwInBetweenTime = dwCurrentTime - pts->scrollTimeInfo.dwTimeEndLastScroll;
            if (dwInBetweenTime >= dwTargetTime)
            {
                // the time between the scrolls takes longer then the actual
                // scroll, whatch out for the jerk effect
                dwTargetTime = 1;               // 1 tick == no smoothscrolling
            }
            else
            {
                dwTargetTime -= dwInBetweenTime;
            }

            dwOptimalTargetTime = pts->scrollTimeInfo.dwTargetTime;

            // should be at least 1 scroll action performed
            Assert (pts->scrollTimeInfo.iTargetScrolls);

            if (dwTargetTime < dwOptimalTargetTime)
            {
                cAnticipateScrolls = ( pts->scrollTimeInfo.iTargetScrolls * dwTargetTime +
                                       dwOptimalTargetTime - 1 )/dwOptimalTargetTime;
            }
            else
            {
                cAnticipateScrolls = pts->scrollTimeInfo.iTargetScrolls;
            }
        }

        // Calculate the minimum delta to scroll at a time
        dMinStep = (abs(delta) + cAnticipateScrolls - 1)/cAnticipateScrolls;
        if (delta < 0)
        {
            iDirection = -1;    // iDirection has the same sign as the delta
            dStep = -dMinStep;  // Set the delta step to the minimum delta
        }
        else
        {
            iDirection = 1;
            dStep = dMinStep;
        }

        // Main smooth scrolling loop
        do
        {
            // calculate the time already spent in the loop
            dwSpendTime = dwCurrentTime - dwStartTime;

            if (dwSpendTime && !psinfo->_fRepeat)
            {
                // if it is no the first time and we are not repeating the
                // SmoothScrolling operation.
                Assert(cDoneScrolls);

                // Calculate the number of pixels to scroll (dScroll)
                dScroll = delta;
                if (dwSpendTime < dwTargetTime)
                {
                    cMoreScrolls = MulDivQuick(cDoneScrolls, dwTargetTime - dwSpendTime, dwSpendTime);
                    if (cMoreScrolls)
                    {
                        dScroll = delta/cMoreScrolls;
                        if (abs(dScroll) < dMinStep)
                        {
                            dScroll = dStep;
                        }
                    }   // else   dScroll = delta;
                }       // else   dScroll = delta;
            }
            else
            {
                dScroll = dStep;
            }

            if (abs(dScroll) > abs(delta))
            {
                if (!psinfo->_fRepeat)
                {
                    // normal case (1st time smooth scrolling requested)
                    dScroll = delta;
                }
                else
                {
                    // when repeating SmoothScrolling operation always scroll
                    // the same amount of pixels to avoid jerking effect.
                    delta = dScroll;
                }
            }

            fMoreToScroll = delta != dScroll;

            xScroll = GetXScroll() - dScroll*ix;    // calc new x scroll position
            yScroll = GetYScroll() - dScroll*iy;    // calc new y ...
            if (ScrollAction(ppi, xScroll, yScroll, psinfo, &rcInvalid))
            {
                // BUGBUG: Arye: Why the hell are print docs firing OnScroll messages?
                // This is necessary or printing will crash every time.
                if (!CDebugPaint::UseDisplayTree())
                {
                    if (!pFL->Doc()->IsPrintDoc())
                        pFL->Doc()->UpdateForm();
                }
                fScrolled = TRUE;
            }
            // Calculate the amount was actualy scrolled (dScroll)
            dScroll = psinfo->_dx + psinfo->_dy;  // dx or dy is garanteed to be 0;
            delta -= dScroll;
            dwCurrentTime = GetTickCount();
            cDoneScrolls++;
        } while (fMoreToScroll &&       // continue if we need to do more scrolls
                 dScroll != 0 &&        // and we did actually scrolled
                 delta*iDirection > 0); // and we didn't exeed over the original limit.

        if (!psinfo->_fRepeat)
        {
            // remember the number of scrolls and the actual time it took.
            pts->scrollTimeInfo.iTargetScrolls = cDoneScrolls;
            pts->scrollTimeInfo.dwTargetTime = dwCurrentTime - dwStartTime;
        }
        pts->scrollTimeInfo.dwTimeEndLastScroll = dwCurrentTime;
    }
    return fScrolled;
}
#endif


#ifndef DISPLAY_TREE
//+---------------------------------------------------------------------------
//
//  Member:     CDisplay::ScrollAction(xScroll, yScroll, psinfo, prcInvalid)
//
//  Synopsis:   scroll view to the new x and y position
//  Devnote:    helper function is called from ScrollView and from
//              SmoothScrolling, this function does actual scrolling
//
//  Arguments:  [xScroll]       new horizontal scroll position
//              [yScroll]       new vertical scroll position
//              [psinfo]        pointer to smooth scroll info struct
//              [prcInvalid]    rect which accumulates invalid area
//
//  Returns:    TRUE if actual scrolling occurred,
//              FALSE if no change
//
//----------------------------------------------------------------------------

BOOL
CDisplay::ScrollAction(
    CPositionInfo *ppi,
    LONG xScroll,
    LONG yScroll,
    SMOOTHSCROLLINFO *psinfo,
    RECT* prcInvalid)
{
    LONG dx = 0;
    LONG dy = 0;
    CFlowLayout * pFL = GetFlowLayout();
    CElement    * pElementFL = pFL->ElementOwner();
    CDoc        * pDoc = pFL->Doc();
    DWORD         dwScrollBars = 0;
    LONG          yFirst;
    LONG          cpFirst;
    LONG          iliFirst;
    CScrollInfo * pScrollInfo = ScrollInfo();

    if( ! pScrollInfo || pFL->IsDisplayNone())
        return FALSE;

    //Assert(pScrollInfo);

    // Step 1: Calculate the new scrolling position

    CSavePositionInfo   spi(ppi);

    const       LONG iliOldFirstVisible = GetFirstVisibleLine();
    BOOL        fScroll = FALSE;
    RECT        rcScroll;
    RECT        rcInval;
    CStackDataAry < RECT, 2 > aryInvalRects(Mt(CDisplayScrollAction_aryInvalRects_pv));

    // Determine vertical scrolling pos
    for (;;)
    {
        LONG dyFirst;
#ifdef NOT_USED_ANY_MORE
        BOOL fNothingBig = TRUE;
        LONG yHeight;
        LONG iliT;
#endif

        // Ensure all visible lines are recalced
        if (!pFL->FExternalLayout())
        {
            WaitForRecalcView();
        }

        // NB (cthrash) If we have a PrintDoc, we want dead space at the bottom.
        // So don't clip yScroll by GetMaxYScroll() when printing.

        if (!_fPrinting)
        {
            yScroll = min(yScroll, pFL->GetMaxYScroll());
        }
        yScroll = max(0, int(yScroll));


        // Compute new first visible line
        if (!pFL->FExternalLayout())
        {
            iliFirst = LineFromPos((!_fRTL ? xScroll : (pFL->GetMaxXScroll() - xScroll)),
                                   yScroll, FALSE, FALSE, &yFirst, &cpFirst,
                                   _xWidthView, ISF_DONTIGNORE_DONTLOOKFURTHER, NULL, FALSE);
            if(iliFirst < 0)
            {
                // No line at GetYScroll(), use last line instead
                iliFirst = max(0, LineCount() - 1);
                Assert(!Elem(iliFirst)->IsFrame());
                cpFirst = GetLastCp() - Elem(iliFirst)->_cch;
                yScroll = _yHeightMax - Elem(iliFirst)->_yHeight;
                yFirst = GetYScroll();
            }
        }
        else
        {
            if (pFL->GetQuillGlue())
                pFL->GetQuillGlue()->GetLineInfo(xScroll, yScroll, FALSE, _xWidthView, FALSE, &yFirst, &cpFirst);
        }
        dyFirst = yFirst - yScroll;

        {
            if(yScroll != GetYScroll())
            {
                pScrollInfo->_iliFirstVisible = iliFirst;
                pScrollInfo->_dyFirstVisible  = dyFirst;
                pScrollInfo->_dcpFirstVisible = cpFirst - GetFirstCp();
                dy                            = GetYScroll() - yScroll;
                pScrollInfo->_yScroll         = yScroll;

                AssertSz(GetYScroll() >= 0, "CDisplay::ScrollView GetYScroll() < 0");
                Assert(pFL->FExternalLayout() || VerifyFirstVisible());
            }
            break;
        }
    }

    if (!pFL->FExternalLayout())
    {
        // If GetFirstVisibleLine() changed, we'd better flush the line index cache.
        ResyncListIndex( iliOldFirstVisible );

        CheckView();
    }

    // Determine horizontal scrolling pos.

    // LONG xWidthMax;
    // xWidthMax = _xWidth;
    xScroll = min(xScroll, pFL->GetMaxXScroll());
    xScroll = max(0, int(xScroll));

    dx = GetXScroll() - xScroll;
    if (dx)
    {
        Assert(pScrollInfo);
        pScrollInfo->_xScroll = xScroll;
    }

    // Step 2: do actual act of scrolling

    // BUGBUG:  During a smooth scroll sites might move (due to a pending layout).
    //          We should investigate why the layout is pending...really all layout
    //          should be complete prior to initiating a scroll (brendand)

    //
    // BUGBUG EricVas:
    //
    // Case in point:  scrolling after pasting a table into a table cell causes
    // us to scroll before the doc is recalced.  This causes the clip rect to bet
    // out of sync with with the rc's in the site tree (because recalc needs to
    // happen).
    //

    // For normal documents, scroll if there is a scroll amount

    // ATTENTION:  If you fix a bug within this if-branch for non-printing, please
    // take a minute to decide whether this bug would also apply to the printing
    // cases handled in the else-branch.  Thank you.  (Oliver)
    if(!_fPrinting && (dy || dx))
    {
        // Scroll only if scrolling < view dimensions and we are in-place,
        // don't have a watermark, and are opaque (or the special case of
        // us being the client site of the root site).

        //$ BUGBUG (dinartem) Can't scroll if there are sites hovering above

        if (    (dy < _yHeightView && dx < _xWidthView)
            &&  !pFL->HasWatermark()
            &&  pDoc->_LoadStatus >= LOADSTATUS_INTERACTIVE
            &&  !pDoc->TestLock(FORMLOCK_SCROLL)
            &&  (   pFL->_fOpaque
                ||  pDoc->GetElementClient() == pElementFL))
        {
            if(pFL->_fVertical)
            {
                long    lTemp;

                fScroll  = TRUE;
                rcScroll = psinfo->_rcView;

                ConvSetRect(&rcScroll);

                lTemp = dx;
                dx    = -dy;
                dy    = lTemp;
            }
            else
            {
                RECT rcInset;

                rcScroll = psinfo->_rcView;

                // Scroll the gutters around the edge, too.
                pFL->GetViewInsetDisp (&rcInset, ppi);
                if (dy != 0 && dx == 0)
                {
                    fScroll         = TRUE;
                    rcScroll.left  -= rcInset.left;
                    rcScroll.right += rcInset.right;


                    // Update the inset at the top and bottom.
                    if (rcInset.top != 0)
                    {
                        rcInval        = rcScroll;
                        rcInval.top   -= rcInset.top;
                        rcInval.bottom = psinfo->_rcView.top;
                        IGNORE_HR(aryInvalRects.AppendIndirect(&rcInval));
                    }
                    if (rcInset.bottom != 0)
                    {
                        rcInval         = rcScroll;
                        rcInval.bottom += rcInset.bottom;
                        rcInval.top     = psinfo->_rcView.bottom;
                        IGNORE_HR(aryInvalRects.AppendIndirect(&rcInval));
                    }
                }
                else if (dy == 0 && dx != 0)
                {
                    fScroll          = TRUE;
                    rcScroll.top    -= rcInset.top;
                    rcScroll.bottom += rcInset.bottom;

                    // Update the inset at the left and right.
                    if (rcInset.left != 0)
                    {
                        rcInval       = rcScroll;
                        rcInval.left -= rcInset.left;
                        rcInval.right = psinfo->_rcView.left;
                        IGNORE_HR(aryInvalRects.AppendIndirect(&rcInval));
                    }
                    if (rcInset.right != 0)
                    {
                        rcInval        = rcScroll;
                        rcInval.right += rcInset.right;
                        rcInval.left   = psinfo->_rcView.right;
                        IGNORE_HR(aryInvalRects.AppendIndirect(&rcInval));
                    }
                }
                else
                {
                    // We can't scroll diagonally.
                    Assert (dy != 0 && dx != 0);
                }
            }
        }
        else
        {
            IGNORE_HR(aryInvalRects.AppendIndirect(&psinfo->_rcView));
        }

        //
        // Update the scrollbars
        // (This is done prior to positioning so that the positioning code
        //  can depend upon the scrollbars to contain the correct scroll
        //  positions)
        //

        if(!psinfo->_fTracking && dy)
        {
            dwScrollBars = fmScrollBarsVertical;
        }

        if(!psinfo->_fTracking && dx)
        {
            dwScrollBars |= fmScrollBarsHorizontal;
        }

        if (dwScrollBars)
        {
            pFL->OnScrollChange(dwScrollBars, ppi);
        }

        //
        // Next, position all sites/elements dependent upon scroll position
        //

        pFL->_fPositionDirectlyBelow = TRUE;

        if (fScroll)
        {
            //
            // Don't set the scrolling data if we have positioned things.
            //
            if (    pElementFL->Tag() == ETAG_BODY
                || !pDoc->NeedRegionCollection())
            {
                ppi->SetScroll(&rcScroll);
            }
        }

        if (!pFL->FExternalLayout())
        {
            IGNORE_HR(PositionAndLayoutObjects(ppi, dx, dy));
        }
        else
        {
            if (pFL->GetQuillGlue())
                IGNORE_HR(pFL->GetQuillGlue()->PositionAndLayoutObjectsForScroll(ppi, dx, dy));
        }

        //
        // Scroll the appropriate area
        //

        if (fScroll)
        {
            if(psinfo->_psel)
                psinfo->_psel->ShowCaret(FALSE, psinfo->_pFlowLayoutSelOwner);

            pFL->ScrollRect(ppi, &rcScroll, dx, dy, RDW_INVALIDATE | RDW_ALLCHILDREN);

            if (psinfo->_psel)
                psinfo->_psel->ShowCaret(TRUE, psinfo->_pFlowLayoutSelOwner);
        }

        //
        // Move child windows and invalidate
        //

        ppi->EndDeferSetWindowPos();
        ppi->EndDeferSetObjectRects();

        for (int nIndex = 0; nIndex < aryInvalRects.Size(); nIndex++)
        {
            pFL->Invalidate(&aryInvalRects[nIndex], 0);
            // do not do INVAL_CHILDWINDOWS here - this will cause flickering (alexz) (brendand)
        }

        if (psinfo->_psel)
        {
            pDoc->UpdateCaret(FALSE, FALSE, ppi);
        }

        pFL->ViewChange(pFL->Doc()->_state >= OS_INPLACE ? TRUE : FALSE);
    }

    // For printing documents, simply position the contained sites
    else if (_fPrinting && (dx || dy))
    {

        //
        // Position all children
        //

        pFL->_fPositionDirectlyBelow = TRUE;

        IGNORE_HR(PositionAndLayoutObjects(ppi, dx, dy));

        //
        // And move child windows
        //

        ppi->EndDeferTransitionTo();
        ppi->EndDeferSetWindowPos();
        ppi->EndDeferSetObjectRects();
    }

    psinfo->_dx = dx;
    psinfo->_dy = dy;

    if (dy || dx)
    {
        pFL->Doc()->DeferSendMouseMessage();
    }
    return dy || dx;
}
#endif

#ifndef DISPLAY_TREE
//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DeferSendMouseMessage
//
//  Synopsis:   After scrolling we want to post a setcursor message to our
//              window so that the cursor shape will get updated. However,
//              because of nested scrolls, we might end up with multiple
//              setcursor calls. To avoid this, we will post a method call
//              to the function which actually does the postmessage. During
//              the setup of the method call we will delete any existing
//              callbacks, and hence this will delete any existing callbaks.
//
//----------------------------------------------------------------------------
void
CDoc::DeferSendMouseMessage()
{
    // Kill pending calls if any
    GWKillMethodCall (this, ONCALL_METHOD(CDoc, SendMouseMessage, sendmousemessage), 0);
    // Defer the mouse message call
    IGNORE_HR(GWPostMethodCall (this,
                                ONCALL_METHOD(CDoc, SendMouseMessage, sendmousemessage),
                                0, FALSE, "CDoc::SendMouseMessage"));
}
#endif

#ifndef DISPLAY_TREE
//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SendMouseMessage
//
//  Synopsis:   This function actually posts the message to our window.
//
//----------------------------------------------------------------------------
void
CDoc::SendMouseMessage(DWORD dwParam)
{
    // First be sure that we are all OK
    // Do this only if we have focus. We don't want to generate mouse events
    // when we do not have focus (bug 9144)
    if (_pInPlace && _pInPlace->_hwnd && HasFocus())
    {
        POINT pt;
        RECT  rc;

        ::GetCursorPos(&pt);
        ::GetClientRect(_pInPlace->_hwnd, &rc);

        // Next be sure that the mouse is in our client rect and only then
        // post ourselves the message.
        if (::PtInRect(&rc, pt))
            ::PostMessage(_pInPlace->_hwnd, WM_SETCURSOR, (WORD)_pInPlace->_hwnd, HTCLIENT);
    }
}
#endif

#ifndef DISPLAY_TREE
//+---------------------------------------------------------------------------
//
//  Member:     CDisplay::PositionAndLayoutObjects(xScroll, yScroll, psinfo)
//
//  Synopsis:   Positions all objects and lays out absolutely positioned elements
//  Devnote:    helper function is called from ScrollAction from
//              both the printing and the browsing case.
//
//  Arguments:  [ppi]           position info
//              [dx]            horizontal scroll amount
//              [dy]            vertical scroll amount
//
//  Returns:    S_OK
//
//----------------------------------------------------------------------------

HRESULT
CDisplay::PositionAndLayoutObjects(CPositionInfo *  ppi,
                                   LONG             dx,
                                   LONG             dy)
{
    CFlowLayout *   pFL = GetFlowLayout();
    CElement::CLock Lock(pFL->ElementOwner(), CElement::ELEMENTLOCK_POSITIONING);
    DWORD           grfLayout = ppi->_grfLayout;

    ppi->ApplyClipRects(pFL);

    TraceTagEx((tagLayout, TAG_NONAME,
                "Scroll : (%ls, %d) positioning STATIC descendents",
                pFL->ElementOwner()->TagName(), pFL->ElementOwner()->SN()));

    ppi->_grfLayout |= LAYOUT_NOINVAL;
    PositionObjects(ppi, 0L);
    ppi->_grfLayout  = grfLayout;

    TraceTagEx((tagLayout, TAG_NONAME,
                "Scroll : (%ls, %d) positioning RELATIVE elements",
                pFL->ElementOwner()->TagName(), pFL->ElementOwner()->SN()));

    PositionElements(ppi, 0, dx, dy);

    pFL->RequestLayoutAbsolute(ppi, TRUE, TRUE);

    if (pFL->_fLayoutPositioned)
    {
        pFL->LayoutPositionedElements(ppi, FALSE);
    }

    return S_OK;
}
#endif

#ifndef DISPLAY_TREE
/*
 *  CDisplay::GetScrollRange(nBar)
 *
 *  @mfunc
 *      Returns the max part of a scrollbar range for scrollbar <p nBar>
 *
 *  @rdesc
 *      LONG max part of scrollbar range
 */
LONG CDisplay::GetScrollRange(
    INT nBar) const     //@parm Scroll bar to interrogate (SB_VERT or SB_HORZ)
{
    CFlowLayout * pFL = GetFlowLayout();
    LONG          lRange = 0;

    Assert( !_fPrinting );

    if (nBar == SB_VERT)
    {
        if(pFL->GetScrollBarFlag(fmScrollBarsVertical, SBF_ALLOWED))
        {
            lRange = pFL->GetMaxYScroll();
        }

        return lRange;
    }


    if(pFL->GetScrollBarFlag(fmScrollBarsHorizontal, SBF_ALLOWED))
    {
        // Scroll range is maxium width plus room for the caret.
        lRange = GetWidth();
        if (lRange < 0)
            lRange = 0;
    }

    return lRange;
}
#endif

#ifndef DISPLAY_TREE
/*
 *  CDisplay::UpdateScrollBars()
 *
 *  @mfunc
 *      Update either the horizontal or vertial scroll bar
 *      Also figure whether the scroll bar should be visible or not
 */
void
CDisplay::UpdateScrollBars(CDocInfo * pdci)
{
#ifndef NO_SCROLLBAR
    CFlowLayout * pFL = GetFlowLayout();

    // If not in need of sizing, update what scrollbars are showing and their state
    // (If in need of sizing, the correct determination cannot be made regarding scrollbars.)
    if (!pFL->_fSizeThis)
    {
        // Do nothing if the site has nno scrollbars
        // Note : The call to DetermineScrollbars(0 later in this function catches this
        // case, but we want to avoid the call to GetClientRect() which ends up calling
        // the expensive GetClientRectSlow() even for trivial edits.
        if (!pFL->GetScrollBarFlag(fmScrollBarsVertical, SBF_ALLOWED) &&
            !pFL->GetScrollBarFlag(fmScrollBarsHorizontal, SBF_ALLOWED))
        {
            return;
        }

        CElement::CLock    Lock(pFL->ElementOwner(), CElement::ELEMENTLOCK_SIZING);
        RECT            rc;

        Assert(!pFL->_fSizeThis);

        // Determine which scrollbars are necessary
        if (pFL->DetermineScrollBars(pdci, &pFL->_rc))
        {
            BOOL    fViewChanged = _fViewChanged;

            pFL->CacheClientRect(pdci);
            pFL->GetClientRect((CRect *)&rc, 0, pdci);

            Assert((rc.right -XVScrollBarWidth()) - rc.left  == _xWidthView);

            Verify (!SetViewSize(rc));

            _fViewChanged = fViewChanged;

            pFL->ResizePercentHeightSites();
        }
        pFL->OnScrollChange(pFL->GetAllScrollBarFlags(SBF_VISIBLE),
                                  pdci);
    }
#endif // NO_SCROLLBAR
}
#endif

#ifndef DISPLAY_TREE
inline long GetAmountToScroll(long distance)
{
    distance /= 2;
    return max(distance, 1l);
}
#endif

#ifndef DISPLAY_TREE
//+---------------------------------------------------------------------------
//
//  Member:     CDisplay::ScrollMoreHorizontal
//
//  Synopsis:   Check if any horizontal scrolling can be done, and if so by how
//              much. Note that it will scroll both the client rect AND the
//              contents. This gives a much better feedback to the user.
//
//----------------------------------------------------------------------------
void
CDisplay::ScrollMoreHorizontal(
    POINT *ppt,
    RECT  *prc,
    LONG  *pxScroll,
    BOOL  *pfScrollClientRect,
    BOOL  *pfScrollContents)
{
    CFlowLayout * pFL = GetFlowLayout();
    RECT rcVisible;
    RECT rcClient;
    LONG delta;

    Assert(ppt && prc && pxScroll && pfScrollClientRect && pfScrollContents);

    // If we've already scrolled in vertical direction then do not bother
    if (*pfScrollClientRect || *pfScrollContents)
        return;

    pFL->GetVisibleClientRect(&rcVisible);
    pFL->GetClientRect((CRect *)&rcClient);

    Assert(!PtInRect(&rcVisible, *ppt));

    *prc = rcVisible;

    if (ppt->x < rcVisible.left)
    {
        delta = GetAmountToScroll(rcVisible.left - ppt->x);
        if (rcVisible.left > rcClient.left)
        {
            prc->right = rcVisible.left;
            prc->left = prc->right - delta;
            *pfScrollClientRect = TRUE;
        }
        if (*pxScroll)
        {
            *pxScroll -= delta;
            *pfScrollContents = TRUE;
        }
    }
    else if (ppt->x >= rcVisible.right)
    {
        delta = GetAmountToScroll(ppt->x - rcVisible.right);
        if (rcVisible.right < rcClient.right)
        {
            prc->left = rcVisible.right;
            prc->right = prc->left + delta;
            *pfScrollClientRect = TRUE;
        }
        if (*pxScroll < pFL->GetMaxXScroll())
        {
            *pxScroll += delta;
            *pfScrollContents = TRUE;
        }
    }
}
#endif


#ifndef DISPLAY_TREE
//+---------------------------------------------------------------------------
//
//  Member:     CDisplay::ScrollMoreVertical
//
//  Synopsis:   Check if any vertical scrolling can be done, and if so by how
//              much. Note that it will scroll both the client rect AND the
//              contents. This gives a much better feedback to the user.
//
//----------------------------------------------------------------------------
void
CDisplay::ScrollMoreVertical(
    POINT *ppt,
    RECT  *prc,
    LONG  *pyScroll,
    BOOL  *pfScrollClientRect,
    BOOL  *pfScrollContents)
{
    CFlowLayout * pFL = GetFlowLayout();
    RECT rcVisible;
    RECT rcClient;
    LONG delta;

    Assert(ppt && prc && pyScroll && pfScrollClientRect && pfScrollContents);

    // If we've already scrolled in horiz direction then do not bother
    if (*pfScrollClientRect || *pfScrollContents)
        return;

    pFL->GetVisibleClientRect(&rcVisible);
    pFL->GetClientRect((CRect *)&rcClient);

    Assert(!PtInRect(&rcVisible, *ppt));
    *prc = rcVisible;

    // Mouse above the visible portion of the site
    if (ppt->y < rcVisible.top)
    {
        delta = GetAmountToScroll(rcVisible.top - ppt->y);
        if (rcVisible.top > rcClient.top)
        {
            prc->bottom = rcVisible.top;
            prc->top = prc->bottom - delta;
            *pfScrollClientRect = TRUE;
        }
        if (*pyScroll)
        {
            *pyScroll -= delta;
            *pfScrollContents = TRUE;
        }
    }
    else if (ppt->y >= rcVisible.bottom)
    {
        delta = GetAmountToScroll(ppt->y - rcVisible.bottom);
        if (rcVisible.bottom < rcClient.bottom)
        {
            prc->top = rcVisible.bottom;
            prc->bottom = prc->top + delta;
            *pfScrollClientRect = TRUE;
        }
        if (*pyScroll < pFL->GetMaxYScroll())
        {
            *pyScroll += delta;
            *pfScrollContents = TRUE;
        }
    }
}
#endif

#ifndef DISPLAY_TREE
//+---------------------------------------------------------------------------
//
//  Member:     CDisplay::ScrollMore
//
//  Synopsis:   Scroll in the direction indicated by pt. pptDelta tells us
//              where the current point is relative to the start point. This
//              helps us make the decision to go left-right first or up-down
//              first.
//----------------------------------------------------------------------------
BOOL
CDisplay::ScrollMore(POINT *ppt, POINT *pptDelta)
{
    RECT rc;
    LONG xScroll, yScroll;
    BOOL fScrollClientRect = FALSE;
    BOOL fScrollContents = FALSE;
    CFlowLayout * pFL = GetFlowLayout();

    if (pFL->_fParked)
       goto Cleanup;

    xScroll = GetXScroll();
    yScroll = GetYScroll();

    if (pptDelta->y < 0)
    {
        if (pptDelta->x < 0)
        {
            ScrollMoreHorizontal(ppt, &rc, &xScroll, &fScrollClientRect, &fScrollContents);
            ScrollMoreVertical  (ppt, &rc, &yScroll, &fScrollClientRect, &fScrollContents);
        }
        else
        {
            ScrollMoreVertical  (ppt, &rc, &yScroll, &fScrollClientRect, &fScrollContents);
            ScrollMoreHorizontal(ppt, &rc, &xScroll, &fScrollClientRect, &fScrollContents);
        }
    }
    else
    {
        if (pptDelta->x < 0)
        {
            ScrollMoreVertical  (ppt, &rc, &yScroll, &fScrollClientRect, &fScrollContents);
            ScrollMoreHorizontal(ppt, &rc, &xScroll, &fScrollClientRect, &fScrollContents);
        }
        else
        {
            ScrollMoreHorizontal(ppt, &rc, &xScroll, &fScrollClientRect, &fScrollContents);
            ScrollMoreVertical  (ppt, &rc, &yScroll, &fScrollClientRect, &fScrollContents);
        }
    }

    if (fScrollClientRect)
    {
        fScrollClientRect = FALSE;
        OffsetRect(&rc, -pFL->_rc.left, -pFL->_rc.top);
        pFL->GetParentLayout()->ScrollSiteIntoView(
            pFL, &rc, SP_MINIMAL, SP_MINIMAL, &fScrollClientRect);
    }

    if (fScrollContents)
    {
        fScrollContents = ScrollView(NULL, xScroll, yScroll, FALSE, TRUE);
    }

Cleanup:
    return fScrollClientRect || fScrollContents;
}
#endif
