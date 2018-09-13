//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispscroller.cxx
//
//  Contents:   Simple scrolling container.
//
//  Classes:    CDispScroller
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_SAVEDISPCONTEXT_HXX_
#define X_SAVEDISPCONTEXT_HXX_
#include "savedispcontext.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

MtDefine(CDispScroller, DisplayTree, "CDispScroller")

void
CDispScroller::GetVScrollbarRect(CRect* prcVScrollbar, const CDispInfo& di) const
{
    if(!IsRightToLeft())
    {
        prcVScrollbar->right =
            _rcContainer.Width() - di._prcBorderWidths->right;
        prcVScrollbar->left =
            max(di._prcBorderWidths->left,
                prcVScrollbar->right - _sizeScrollbars.cx);
    }
    else
    {
        prcVScrollbar->left = di._prcBorderWidths->left;
        prcVScrollbar->right =
            min(_rcContainer.Width() - di._prcBorderWidths->right,
                prcVScrollbar->left + _sizeScrollbars.cx);
    }
    prcVScrollbar->top = di._prcBorderWidths->top;
    prcVScrollbar->bottom =
        _rcContainer.Height() - di._prcBorderWidths->bottom;
    if (_fHasHScrollbar)
        prcVScrollbar->bottom -= _sizeScrollbars.cy;
}

void
CDispScroller::GetHScrollbarRect(CRect* prcHScrollbar, const CDispInfo& di) const
{
    prcHScrollbar->bottom =
        _rcContainer.Height() - di._prcBorderWidths->bottom;
    prcHScrollbar->top =
        max(di._prcBorderWidths->top,
            prcHScrollbar->bottom - _sizeScrollbars.cy);
    prcHScrollbar->left = di._prcBorderWidths->left;
    prcHScrollbar->right =
        _rcContainer.Width() - di._prcBorderWidths->right;

    if (_fHasVScrollbar)
    {
        if(!IsRightToLeft())
            prcHScrollbar->right -= _sizeScrollbars.cx;
        else
            prcHScrollbar->left += _sizeScrollbars.cx;

        // don't do a negative scroll
        if(prcHScrollbar->right < prcHScrollbar->left)
            prcHScrollbar->right = prcHScrollbar->left;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::SetVerticalScrollbarWidth
//
//  Synopsis:   Set the width of the vertical scroll bar, and whether it is
//              forced to display.
//
//  Arguments:  width       new width
//              fForce      TRUE to force scroll bar to be displayed
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::SetVerticalScrollbarWidth(LONG width, BOOL fForce)
{
    Assert(width >= 0);

    if (width != _sizeScrollbars.cx || fForce != _fForceVScrollbar)
    {
        _sizeScrollbars.cx = width;
        _fForceVScrollbar = fForce;

        RecalcScroller();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::SetHorizontalScrollbarHeight
//
//  Synopsis:   Set the height of the horizontal scroll bar, and whether it is
//              forced to display.
//
//  Arguments:  height      new height
//              fForce      TRUE to force scroll bar to be displayed
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::SetHorizontalScrollbarHeight(LONG height, BOOL fForce)
{
    Assert(height >= 0);

    if (height != _sizeScrollbars.cy || fForce != _fForceHScrollbar)
    {
        _sizeScrollbars.cy = height;
        _fForceHScrollbar = fForce;

        RecalcScroller();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::VerticalScrollbarIsActive
//
//  Synopsis:   Determine whether the vertical scroll bar is visible and
//              active (can scroll the content).
//
//  Arguments:  none
//
//  Returns:    TRUE if the vertical scroll bar is visible and active
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::VerticalScrollbarIsActive() const
{
    if (!_fHasVScrollbar)
        return FALSE;

    CDispInfo di(GetExtras());
    return
        _rcContainer.Height()
        - di._prcBorderWidths->top - di._prcBorderWidths->bottom
        - ((_fHasHScrollbar) ? _sizeScrollbars.cy : 0)
        < _sizeContent.cy;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::HorizontalScrollbarIsActive
//
//  Synopsis:   Determine whether the horizontal scroll bar is visible and
//              active (can scroll the content).
//
//  Arguments:  none
//
//  Returns:    TRUE if the horizontal scroll bar is visible and active
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::HorizontalScrollbarIsActive() const
{
    if (!_fHasHScrollbar)
        return FALSE;

    CDispInfo di(GetExtras());
    return
        _rcContainer.Width()
        - di._prcBorderWidths->left - di._prcBorderWidths->right
        - ((_fHasVScrollbar) ? _sizeScrollbars.cx : 0)
        < _sizeContent.cx;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::CalcScrollbars
//
//  Synopsis:   Determine scroll bar visibility and adjust scroll offsets.
//
//  Arguments:  none
//
//  Returns:    FALSE if the whole scroller must be invalidated due to
//              scroll offset change.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::CalcScrollbars()
{
    // sometimes we're called before SetSize!
    if (_rcContainer.IsEmpty())
    {
        _fHasVScrollbar = _fHasHScrollbar = FALSE;
        return TRUE;
    }

    BOOL fOldHasVScrollbar = _fHasVScrollbar;
    BOOL fOldHasHScrollbar = _fHasHScrollbar;

    CDispInfo di(GetExtras());

    // calculate container size inside border
    CSize sizeInsideBorder(
        _rcContainer.Width()
            - di._prcBorderWidths->left - di._prcBorderWidths->right,
        _rcContainer.Height()
            - di._prcBorderWidths->top - di._prcBorderWidths->bottom);

    // determine whether vertical scroll bar is needed
    _fHasVScrollbar =
        _sizeScrollbars.cx > 0 &&
        (_fForceVScrollbar || _sizeContent.cy > sizeInsideBorder.cy);

    if (_fHasVScrollbar)
    {
        sizeInsideBorder.cx -= _sizeScrollbars.cx;

        // determine whether horizontal scroll bar is needed
        _fHasHScrollbar =
            _sizeScrollbars.cy > 0 &&
            (_fForceHScrollbar || _sizeContent.cx > sizeInsideBorder.cx);

        if (_fHasHScrollbar)
        {
            sizeInsideBorder.cy -= _sizeScrollbars.cy;
        }
    }

    else
    {
        // determine whether horizontal scroll bar is needed
        _fHasHScrollbar =
            _sizeScrollbars.cy > 0 &&
            (_fForceHScrollbar ||
             _sizeContent.cx > sizeInsideBorder.cx);

        if (_fHasHScrollbar)
        {
            sizeInsideBorder.cy -= _sizeScrollbars.cy;

            // but now vertical scroll bar might be needed
            _fHasVScrollbar =
                _sizeScrollbars.cx > 0 &&
                _sizeContent.cy > sizeInsideBorder.cy;

            if (_fHasVScrollbar)
            {
                sizeInsideBorder.cx -= _sizeScrollbars.cx;
            }
        }
    }

    // fix scroll offsets
    BOOL fScrollOffsetChanged = FALSE;
    CSize contentBottomRight;
    if(!IsRightToLeft())
        contentBottomRight = _sizeContent + _sizeScrollOffset;
    else
    {
        contentBottomRight.cx = _sizeContent.cx - _sizeScrollOffset.cx;
        contentBottomRight.cy = _sizeContent.cy + _sizeScrollOffset.cy;
    }
    if (contentBottomRight.cx < sizeInsideBorder.cx)
    {
        long newOffset;
        if (!IsRightToLeft())
            newOffset = min(0L, sizeInsideBorder.cx - _sizeContent.cx);
        else
            newOffset = max(0L, _sizeContent.cx - sizeInsideBorder.cx);

        if (newOffset != _sizeScrollOffset.cx)
        {
            _sizeScrollOffset.cx = newOffset;
            fScrollOffsetChanged = TRUE;
        }
    }
    if (contentBottomRight.cy < sizeInsideBorder.cy)
    {
        long newOffset = min(0L, sizeInsideBorder.cy - _sizeContent.cy);
        if (newOffset != _sizeScrollOffset.cy)
        {
            _sizeScrollOffset.cy = newOffset;
            fScrollOffsetChanged = TRUE;
        }
    }

    // TRICKY... invalidate scroll bars if they've come
    // or gone, but their coming and going also invalidates
    // the other scroll bar, because it has to adjust for
    // the presence or absence of the scroll bar filler
    if (_fHasVScrollbar != fOldHasVScrollbar)
    {
        _fInvalidVScrollbar = TRUE;
        if (_fHasHScrollbar || fOldHasHScrollbar)
            _fInvalidHScrollbar = TRUE;
    }
    if (_fHasHScrollbar != fOldHasHScrollbar)
    {
        _fInvalidHScrollbar = TRUE;
        if (_fHasVScrollbar || fOldHasVScrollbar)
            _fInvalidVScrollbar = TRUE;
    }

    return !fScrollOffsetChanged;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::InvalidateScrollbars
//
//  Synopsis:   Invalidate the scroll bars according to their invalid flags.
//
//  Arguments:  none
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::InvalidateScrollbars()
{
    CDispInfo di(GetExtras());
    CRegion rgnInvalid;
    InvalidateScrollbars(&rgnInvalid, &di);
    Invalidate(rgnInvalid, COORDSYS_PARENT);
}


// return invalid region in COORDSYS_PARENT coordinates
void
CDispScroller::InvalidateScrollbars(CRegion* prgnInvalid, CDispInfo* pdi)
{
    if (_fInvalidVScrollbar)
    {
        _fInvalidVScrollbar = FALSE;
        if (_fInvalidHScrollbar)
        {
            // form invalid region by subtracting the scrolling area from
            // the area inside borders so that the scrollbar filler will
            // also be invalid
            _fInvalidHScrollbar = FALSE;
            CRect rcInsideBorder(
                _rcContainer.TopLeft() +
                    pdi->_prcBorderWidths->TopLeft().AsSize(),
                _rcContainer.BottomRight() -
                    pdi->_prcBorderWidths->BottomRight().AsSize());
            CRect rcContent(
                rcInsideBorder.TopLeft(),
                rcInsideBorder.BottomRight() - _sizeScrollbars);
            *prgnInvalid = rcInsideBorder;
            prgnInvalid->Subtract(rcContent);
        }
        else
        {
            CRect rcVScrollbar;
            GetVScrollbarRect(&rcVScrollbar, *pdi);
            rcVScrollbar.OffsetRect(_rcContainer.TopLeft().AsSize());
            *prgnInvalid = rcVScrollbar;
        }
    }
    else if (_fInvalidHScrollbar)
    {
        _fInvalidHScrollbar = FALSE;
        CRect rcHScrollbar;
        GetHScrollbarRect(&rcHScrollbar, *pdi);
        rcHScrollbar.OffsetRect(_rcContainer.TopLeft().AsSize());
        *prgnInvalid = rcHScrollbar;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::SetSize
//
//  Synopsis:   Set the size of this container and invalidate the scroll bars.
//
//  Arguments:  size            new size
//              fInvalidateAll  TRUE if entire contents should be invalidated
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::SetSize(const SIZE& size, BOOL fInvalidateAll)
{
    if (_rcContainer.Size() == size)
        return;

    CRect rcBorderWidths(g_Zero.rc);
    
    // factor size of scroll bars into border width
    if (!fInvalidateAll)
    {
        CDispInfo di(GetExtras());
        rcBorderWidths = *di._prcBorderWidths;

        if (_fHasVScrollbar)
        {
            // BUGBUG (donmarsh) - change for left-hand scrollbar for RTL
            rcBorderWidths.right += _sizeScrollbars.cx;
        }
        if (_fHasHScrollbar)
        {
            rcBorderWidths.bottom += _sizeScrollbars.cy;
        }
    }
    
    super::SetSize(size, rcBorderWidths, fInvalidateAll);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::ScrollRectIntoView
//
//  Synopsis:   Scroll the given rect in the indicated coordinate system into
//              view, with various pinning (alignment) options.
//
//  Arguments:  rc              rect to scroll into view
//              spVert          scroll pin request, vertical axis
//              spHorz          scroll pin request, horizontal axis
//              coordSystem     coordinate system for rc
//
//  Returns:    TRUE if any scrolling was necessary.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::ScrollRectIntoView(
        const CRect& rc,
        CRect::SCROLLPIN spVert,
        CRect::SCROLLPIN spHorz,
        COORDINATE_SYSTEM coordSystem,
        BOOL fScrollBits)
{
    Assert(coordSystem == COORDSYS_CONTENT ||
           coordSystem == COORDSYS_NONFLOWCONTENT);
    BOOL fRightToLeft = IsRightToLeft();

    // translate rc into scroller's parent coordinate system and clip to our
    // content size (can't scroll farther than our contents!)
    CRect rcParent(rc);
    if (!fRightToLeft)
    {
        if (rcParent.left < 0) rcParent.left = 0;
        if (rcParent.right > _sizeContent.cx) rcParent.right = _sizeContent.cx;
    }
    else
    {
        if (rcParent.left < -_sizeContent.cx) rcParent.left = -_sizeContent.cx;
        if (rcParent.right > 0) rcParent.right = 0;
    }
    if (rcParent.top < 0) rcParent.top = 0;
    if (rcParent.bottom > _sizeContent.cy) rcParent.bottom = _sizeContent.cy;
    
    CDispInfo di(GetExtras());
    if (coordSystem == COORDSYS_CONTENT)
    {
        rcParent.OffsetRect(*di._pInsetOffset);
    }
    CSize contentOffset;
    if (!fRightToLeft)
    {
        contentOffset.SetSize(_rcContainer.left + di._prcBorderWidths->left,
                              _rcContainer.top + di._prcBorderWidths->top);
    }
    else
    {
        CPoint pt;
        _rcContainer.GetTopRight(&pt);
        
        contentOffset.SetSize(pt.x - di._prcBorderWidths->left,
                              pt.y + di._prcBorderWidths->top);
    }
    rcParent.OffsetRect(contentOffset);


    // calculate scroll offset required to bring rcParent into view
    CRect rcView;
    if(!fRightToLeft)
    {
        rcView.SetRect(contentOffset.AsPoint()-_sizeScrollOffset,
                       _rcContainer.BottomRight() 
                           - di._prcBorderWidths->BottomRight().AsSize()
                           - _sizeScrollOffset);
    }
    else
    {
        rcView.SetRect(_rcContainer.TopLeft() 
                           + di._prcBorderWidths->TopLeft().AsSize()
                           - _sizeScrollOffset,
                       _rcContainer.BottomRight() 
                           - di._prcBorderWidths->BottomRight().AsSize()
                           - _sizeScrollOffset);
    }

    if (_fHasVScrollbar)
    {
        if(!fRightToLeft)
            rcView.right -= _sizeScrollbars.cx;
        else
            rcView.left += _sizeScrollbars.cx;
    }
    if (_fHasHScrollbar)
    {
        rcView.bottom -= _sizeScrollbars.cy;
    }
    CSize scrollOffset;
    BOOL fScrolled =
        rcView.CalcScrollDelta(rcParent, &scrollOffset, spVert, spHorz);
    if (fScrolled)
    {
        fScrolled = (scrollOffset != -_sizeScrollOffset);
        if (fScrolled)
        {
            scrollOffset -= _sizeScrollOffset;
            SetScrollOffset(scrollOffset, fScrollBits);
        }
    }

    if (_pParentNode == NULL )
    {
        return fScrolled;
    }
    
    rcParent.OffsetRect(_sizeScrollOffset);
    
    return
        _pParentNode->ScrollRectIntoView(
            rcParent,
            spVert,
            spHorz,
            GetContentCoordinateSystem(),
            fScrollBits)
          || fScrolled;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller:PreDraw
//
//  Synopsis:   Before drawing starts, PreDraw processes the redraw region,
//              subtracting areas that are blocked by opaque or buffered items.
//              PreDraw is finished when the redraw region becomes empty
//              (i.e., an opaque item completely obscures all content below it)
//
//  Arguments:  pContext    draw context
//
//  Returns:    TRUE if first opaque node to draw has been found
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::PreDraw(CDispDrawContext* pContext)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(AllSet(CDispFlags::s_preDrawSelector));
    Assert(pContext->IntersectsRedrawRegion(_rcVisBounds));
    Assert(!IsSet(CDispFlags::s_generalFlagsNotSetInDraw));
    Assert(!IsSet(CDispFlags::s_interiorFlagsNotSetInDraw));

    // BUGBUG (donmarsh) - for now, we do not delve inside a filtered node.
    // Someday, this will be up to the filter to determine whether PreDraw
    // can safely look at its children and come up with the correct answers.
    if (IsFiltered())
        return FALSE;
    
    // check for redraw region intersection with scroll bars
    CRegion rgnScrollbars;
    if ((_fHasVScrollbar || _fHasHScrollbar) && IsVisible())
    {
        CDispInfo di(GetExtras());
        CRect rcVScrollbar(CRect::CRECT_EMPTY);
        CRect rcHScrollbar(CRect::CRECT_EMPTY);

        if (_fHasVScrollbar)
        {
            GetVScrollbarRect(&rcVScrollbar, di);
            rcVScrollbar.OffsetRect(_rcContainer.TopLeft().AsSize());
            pContext->Transform(&rcVScrollbar);
            if (rcVScrollbar.Contains(pContext->GetRedrawRegion()->GetBounds()))
            {
                // add this node to the redraw region stack
                Verify(!pContext->PushRedrawRegion(rcVScrollbar,this));
                return TRUE;
            }
        }
        if (_fHasHScrollbar)
        {
            GetHScrollbarRect(&rcHScrollbar, di);
            rcHScrollbar.OffsetRect(_rcContainer.TopLeft().AsSize());
            pContext->Transform(&rcHScrollbar);
            if (rcHScrollbar.Contains(pContext->GetRedrawRegion()->GetBounds()))
            {
                // add this node to the redraw region stack
                Verify(!pContext->PushRedrawRegion(rcHScrollbar,this));
                return TRUE;
            }
        }

        // subtract the scroll bar rect(s) from the redraw region
        if (_fHasVScrollbar)
        {
            if (pContext->GetRedrawRegion()->Intersects(rcVScrollbar))
            {
                rgnScrollbars = rcVScrollbar;
            }
        }
        if (_fHasHScrollbar)
        {
            if (pContext->GetRedrawRegion()->Intersects(rcHScrollbar))
            {
                rgnScrollbars.Union(rcHScrollbar);
            }
        }
    }
    
    // process border and children
    if (pContext->IntersectsRedrawRegion(_rcVisBounds) &&
        super::PreDraw(pContext))
    {
        return TRUE;
    }
    
    // if we need to draw scrollbars, subtract them from the redraw region
    if (!rgnScrollbars.IsEmpty() && pContext->GetRedrawRegionStack()->GetKey() != this)
    {
        SetBranchFlag(CDispFlags::s_savedRedrawRegion);
        if (!pContext->PushRedrawRegion(rgnScrollbars,this))
            return TRUE;
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::DrawSelf
//
//  Synopsis:   Draw this node's children, plus optional background.
//
//  Arguments:  pContext        draw context
//              pChild          start drawing at this child
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::DrawSelf(CDispDrawContext* pContext, CDispNode* pChild)
{
    super::DrawSelf(pContext, pChild);

    // draw scroll bars.  This must be done after our super method is called
    // so that the redraw region is opened up to allow drawing in the scrollbar
    // area.
    if (IsVisible() && (_fHasVScrollbar || _fHasHScrollbar))
    {
        DrawScrollbars(pContext);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::DrawBorderAndScrollbars
//
//  Synopsis:   Draw border and scroll bars if they intersect the redraw region.
//
//  Arguments:  pContext        display context
//              prcContent      returns the content rect inside the border
//
//  Notes:      This is an accelerator for the top-level container node only!
//
//----------------------------------------------------------------------------

void
CDispScroller::DrawBorderAndScrollbars(
    CDispDrawContext* pContext,
    CRect* prcContent)
{
    super::DrawBorderAndScrollbars(pContext, prcContent);

    // draw scroll bars
    if (_fHasVScrollbar || _fHasHScrollbar)
    {
        DrawScrollbars(pContext, 0);

        if (_fHasVScrollbar)
        {
            if (!IsRightToLeft())
                prcContent->right -= _sizeScrollbars.cx;
            else
                prcContent->left += _sizeScrollbars.cx;
        }

        if (_fHasHScrollbar)
        {
            prcContent->bottom -= _sizeScrollbars.cy;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::DrawScrollbars
//
//  Synopsis:   Draw the portions of the scroll bars that intersect the
//              redraw region.
//
//  Arguments:  pContext        display context
//              dwFlags         rendering flags
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::DrawScrollbars(CDispDrawContext* pContext, DWORD dwFlags)
{
    CDispInfo di(GetExtras());
    CRect rcVScrollbar(CRect::CRECT_EMPTY);
    CRect rcHScrollbar(CRect::CRECT_EMPTY);

    // draw vertical scroll bar
    if (_fHasVScrollbar)
    {
        // calculate intersection with redraw region
        GetVScrollbarRect(&rcVScrollbar, di);
        rcVScrollbar.OffsetRect(_rcContainer.TopLeft().AsSize());
        CRect rcRedraw(rcVScrollbar);
        pContext->IntersectRedrawRegion(&rcRedraw);
        if (!rcRedraw.IsEmpty())
        {
            _pDispClient->DrawClientScrollbar(
                1,
                &rcVScrollbar,
                &rcRedraw,
                _sizeContent.cy,         // content size
                rcVScrollbar.Height(),   // container size
                -_sizeScrollOffset.cy,   // amount scrolled
                pContext->GetDispSurface(),
                this,
                pContext->GetClientData(),
                dwFlags);
        }
    }

    // draw horizontal scroll bar
    if (_fHasHScrollbar)
    {
        // calculate intersection with redraw region
        GetHScrollbarRect(&rcHScrollbar, di);
        rcHScrollbar.OffsetRect(_rcContainer.TopLeft().AsSize());
        CRect rcRedraw(rcHScrollbar);
        pContext->IntersectRedrawRegion(&rcRedraw);
        if (!rcRedraw.IsEmpty())
        {
            long xScroll = (!IsRightToLeft() 
                            ? -_sizeScrollOffset.cx
                            : max(0L, _sizeContent.cx - rcHScrollbar.Width() - _sizeScrollOffset.cx));

            _pDispClient->DrawClientScrollbar(
                0,
                &rcHScrollbar,
                &rcRedraw,
                _sizeContent.cx,         // content size
                rcHScrollbar.Width(),    // container size
                xScroll,                 // amount scrolled
                pContext->GetDispSurface(),
                this,
                pContext->GetClientData(),
                dwFlags);
        }

        // draw scroll bar filler if necessary
        if (_fHasVScrollbar)
        {
            // calculate intersection with redraw region
            CRect rcScrollbarFiller(
                rcVScrollbar.left,
                rcHScrollbar.top,
                rcVScrollbar.right,
                rcHScrollbar.bottom);
            rcRedraw = rcScrollbarFiller;
            pContext->IntersectRedrawRegion(&rcRedraw);
            if (!rcRedraw.IsEmpty())
            {
                _pDispClient->DrawClientScrollbarFiller(
                    &rcScrollbarFiller,
                    &rcRedraw,
                    pContext->GetDispSurface(),
                    this,
                    pContext->GetClientData(),
                    dwFlags);
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::HitTestPoint
//
//  Synopsis:   Determine whether any of our children, OR THIS CONTAINER,
//              intersects the hit test point.
//
//  Arguments:  pContext        hit context
//
//  Returns:    TRUE if any child or this container intersects the hit test pt.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::HitTestPoint(CDispHitContext* pContext) const
{
    Assert(IsSet(CDispFlags::s_visibleBranch));
    
    if (IsVisible() && (_fHasVScrollbar || _fHasHScrollbar))
    {
        CPoint ptSave(pContext->_ptHitTest);

        // get border info
        CDispInfo di(GetExtras());

        // translate hit point to local coordinates
        pContext->_ptHitTest -=
                _rcContainer.TopLeft().AsSize();
        CRect rcVScrollbar(CRect::CRECT_EMPTY);
        CRect rcHScrollbar(CRect::CRECT_EMPTY);

        // does point hit vertical scroll bar?
        if (_fHasVScrollbar)
        {
            GetVScrollbarRect(&rcVScrollbar, di);
            if (rcVScrollbar.Contains(pContext->_ptHitTest) &&
                _pDispClient->HitTestScrollbar(
                    1,
                    &pContext->_ptHitTest,
                    (CDispContainer*)this,
                    pContext->_pClientData))
            {
                return TRUE;
                // NOTE: don't bother to restore _ptHitTest for speed
            }
        }

        // does point hit horizontal scroll bar?
        if (_fHasHScrollbar)
        {
            GetHScrollbarRect(&rcHScrollbar, di);
            if (rcHScrollbar.Contains(pContext->_ptHitTest) &&
                _pDispClient->HitTestScrollbar(
                    0,
                    &pContext->_ptHitTest,
                    (CDispContainer*)this,
                    pContext->_pClientData))
            {
                return TRUE;
                // NOTE: don't bother to restore _ptHitTest for speed
            }

            // does point hit scroll bar filler?
            if (_fHasVScrollbar)
            {
                CRect rcScrollbarFiller(
                    rcVScrollbar.left,
                    rcHScrollbar.top,
                    rcVScrollbar.right,
                    rcHScrollbar.bottom);
                if (rcScrollbarFiller.Contains(pContext->_ptHitTest) &&
                    _pDispClient->HitTestScrollbarFiller(
                        &pContext->_ptHitTest,
                        (CDispContainer*)this,
                        pContext->_pClientData))
                {
                    return TRUE;
                    // NOTE: don't bother to restore _ptHitTest for speed
                }
            }
        }

        // restore hit test point
        pContext->_ptHitTest = ptSave;
    }

    return super::HitTestPoint(pContext);
}


CDispScroller *
CDispScroller::HitScrollInset(CPoint *pptHit, DWORD *pdwScrollDir)
{
    CPoint ptSave(*pptHit);
    // get border info
    CDispInfo di(GetExtras());
    CSize sizeInsideBorder(
        _rcContainer.Width()
            - di._prcBorderWidths->left - di._prcBorderWidths->right,
        _rcContainer.Height()
            - di._prcBorderWidths->top - di._prcBorderWidths->bottom);

    if (_fHasVScrollbar)
        sizeInsideBorder.cx -= _sizeScrollbars.cx;
    if (_fHasHScrollbar)
        sizeInsideBorder.cy -= _sizeScrollbars.cy;

    // translate hit point to local coordinates
    *pptHit -= _rcContainer.TopLeft().AsSize();

    *pdwScrollDir = 0;

    if (sizeInsideBorder.cx > 2 * g_sizeDragScrollInset.cx)
    {
        if (    (pptHit->x <= g_sizeDragScrollInset.cx)
            &&  (pptHit->x >= 0)
            &&  (_sizeScrollOffset.cx < 0))
        {
            *pdwScrollDir |= SCROLL_LEFT;
        }
        else if (   (pptHit->x >= sizeInsideBorder.cx - g_sizeDragScrollInset.cx)
                 && (pptHit->x <= sizeInsideBorder.cx)
                 && (_sizeContent.cx + _sizeScrollOffset.cx > sizeInsideBorder.cx))
        {
            *pdwScrollDir |= SCROLL_RIGHT;
        }
    }

    if (sizeInsideBorder.cy > 2 * g_sizeDragScrollInset.cy)
    {
        if (    (pptHit->y <= g_sizeDragScrollInset.cy)
            &&  (pptHit->y >= 0)
            &&  (_sizeScrollOffset.cy < 0))
        {
            *pdwScrollDir |= SCROLL_UP;
        }
        else if (   (pptHit->y >= sizeInsideBorder.cy - g_sizeDragScrollInset.cy)
                 && (pptHit->y <= sizeInsideBorder.cy)
                 && (_sizeContent.cy + _sizeScrollOffset.cy > sizeInsideBorder.cy))
        {
            *pdwScrollDir |= SCROLL_DOWN;
        }
    }

    if (*pdwScrollDir)
        return this;

    // restore hit test point
    *pptHit = ptSave;

    return super::HitScrollInset(pptHit, pdwScrollDir);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::GetClientRect
//
//  Synopsis:   Return rectangles for various interesting parts of a display
//              node.
//
//  Arguments:  prc         rect which is returned
//              type        type of rect wanted
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::GetClientRect(RECT* prc, CLIENTRECT type) const
{
    CDispInfo di(GetExtras());
    BOOL fRightToLeft = IsRightToLeft();

    switch (type)
    {
    case CLIENTRECT_BACKGROUND:
        ((CRect*)prc)->SetRect(
            _rcContainer.Size()
            - di._prcBorderWidths->TopLeft().AsSize()
            - di._prcBorderWidths->BottomRight().AsSize());
        if (!IsSet(CDispFlags::s_fixedBackground))
        {
            if(!fRightToLeft)
                ((CRect*)prc)->OffsetRect(-_sizeScrollOffset);
            else
            {
                ((CRect*)prc)->OffsetRect(_sizeScrollOffset.cx, -_sizeScrollOffset.cy);
            }

        }
        if(fRightToLeft)
        {
            ((CRect*)prc)->MirrorX();
        }
        if (_fHasVScrollbar)
        {
            if(!fRightToLeft)
                prc->right -= _sizeScrollbars.cx;
            else
                prc->left += _sizeScrollbars.cx;
        }
        if (_fHasHScrollbar)
        {
            prc->bottom -= _sizeScrollbars.cy;
        }
        break;

    case CLIENTRECT_CONTENT:
    case CLIENTRECT_VISIBLECONTENT:
        {
            CSize sizeContained(
                _rcContainer.Size()
                - di._prcBorderWidths->TopLeft().AsSize()
                - di._prcBorderWidths->BottomRight().AsSize());
            if (type == CLIENTRECT_CONTENT || _fHasVScrollbar)
            {
                sizeContained.cx -= _sizeScrollbars.cx;
            }
            if (_fHasHScrollbar)
            {
                sizeContained.cy -= _sizeScrollbars.cy;
            }
            ((CRect*)prc)->SetRect(
                -_sizeScrollOffset.AsPoint(),
                sizeContained);
            if (fRightToLeft)
            {
                ((CRect*)prc)->OffsetX(-(sizeContained.cx-_sizeScrollOffset.cx));
            }
        }
        break;

    case CLIENTRECT_VSCROLLBAR:
        if (_fHasVScrollbar)
        {
            GetVScrollbarRect((CRect*)prc, di);
        }
        else
            *prc = g_Zero.rc;
        break;

    case CLIENTRECT_HSCROLLBAR:
        if (_fHasHScrollbar)
        {
            GetHScrollbarRect((CRect*)prc, di);
        }
        else
            *prc = g_Zero.rc;
        break;

    case CLIENTRECT_SCROLLBARFILLER:
        if (_fHasHScrollbar && _fHasVScrollbar)
        {
            prc->right = _rcContainer.Width() - di._prcBorderWidths->right;
            prc->bottom = _rcContainer.Height() - di._prcBorderWidths->bottom;
            prc->left = max(
                di._prcBorderWidths->left, prc->right - _sizeScrollbars.cx);
            prc->top = max(
                di._prcBorderWidths->top, prc->bottom - _sizeScrollbars.cy);
        }
        else
            *prc = g_Zero.rc;
        break;
    }

    if (prc->left >= prc->right || prc->top >= prc->bottom)
        *prc = g_Zero.rc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::SetScrollOffset
//
//  Synopsis:   Set new scroll offset.
//
//  Arguments:  offset          new scroll offset
//              fScrollBits     TRUE if we should try to scroll bits on screen
//
//  Returns:    TRUE if a scroll update occurred
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::SetScrollOffset(
        const SIZE& offset,
        BOOL fScrollBits)
{
    CDispInfo di(GetExtras());
    CDispContext context;
    BOOL fRightToLeft = IsRightToLeft();

    // adjust for size of border and scroll bars
    CSize sizeContainer(
        _rcContainer.Size()
        - di._prcBorderWidths->TopLeft().AsSize()
        - di._prcBorderWidths->BottomRight().AsSize());

    if (_fHasVScrollbar)
    {
        sizeContainer.cx -= _sizeScrollbars.cx;
    }
    if (_fHasHScrollbar)
    {
        sizeContainer.cy -= _sizeScrollbars.cy;
    }

    // calculate new scroll offset
    CSize peggedOffset(offset);
    CSize sizeDiff;
    if (!fRightToLeft)
    {
        if (peggedOffset.cx < 0)    // don't scroll into negative coords.
            peggedOffset.cx = 0;
        sizeDiff.cx =
            (sizeContainer.cx >= _sizeContent.cx
                ? 0
                : max(-peggedOffset.cx, sizeContainer.cx - _sizeContent.cx))
            - _sizeScrollOffset.cx;
    }
    else
    {
        if (peggedOffset.cx > 0)    // don't scroll into positive coords.
            peggedOffset.cx = 0;
        sizeDiff.cx =
            (sizeContainer.cx >= _sizeContent.cx
                ? 0
                : min(-peggedOffset.cx, _sizeContent.cx - sizeContainer.cx))
            - _sizeScrollOffset.cx;
    }
    if (peggedOffset.cy < 0)
        peggedOffset.cy = 0;
    sizeDiff.cy =
        (sizeContainer.cy >= _sizeContent.cy
            ? 0
            : max(-peggedOffset.cy, sizeContainer.cy - _sizeContent.cy))
        - _sizeScrollOffset.cy;

    // scroll only if we need to
    if (sizeDiff.IsZero())
        return FALSE;

    _sizeScrollOffset += sizeDiff;

    // no more work needed if this node isn't rooted under a CDispRoot node
    // Along the way we need to check for any filtered parents (this is why
    // we don't call GetRootNode).

    BOOL fFiltered = IsFiltered();
    CDispNode* pRootCandidate = this;

    while (pRootCandidate->_pParentNode)
    {
        pRootCandidate = pRootCandidate->_pParentNode;
        fFiltered |= pRootCandidate->IsFiltered();
    }

    if (!pRootCandidate->IsDispRoot())
        return FALSE;

    CDispRoot* pRoot = DYNCAST(CDispRoot, pRootCandidate);

    if (fFiltered)
    {
        GetNodeTransform(&context, COORDSYS_PARENT, COORDSYS_GLOBAL);
        Verify(CalculateInView(&context, TRUE, TRUE));
        Invalidate(_rcContainer, COORDSYS_PARENT);

        goto Cleanup;
    }


    // check to make sure display tree is open
    // BUGBUG (donmarsh) -- normally we could assert this, but SetScrollOffsetSmoothly calls
    // SetScrollOffset multiple times, and the display tree will only be open for the first
    // call.  After we call CDispRoot::ScrollRect below, the tree may not be open anymore.
    //Assert(pRoot->DisplayTreeIsOpen());

    // no more work if this scroller isn't in view and visible
    if (!AllSet(CDispFlags::s_visibleBranchAndInView))
        goto Cleanup;

    // if the root is marked for recalc, just recalc this scroller
    // if we aren't being asked to scroll bits, simply request recalc
    if (!fScrollBits || pRoot->IsSet(CDispFlags::s_recalc))
    {
        SetFlag(CDispFlags::s_inval);
        RecalcScroller();
        if (IsSet(CDispFlags::s_positionChange))
        {
            SetPositionHasChanged();
        }
    }

    else
    {
        GetNodeTransform(&context, COORDSYS_PARENT, COORDSYS_GLOBAL);

        // do scroll bars need to be redrawn?
        Assert(!_fInvalidVScrollbar && !_fInvalidHScrollbar);
        _fInvalidVScrollbar = _fHasVScrollbar && sizeDiff.cy != 0;
        _fInvalidHScrollbar = _fHasHScrollbar && sizeDiff.cx != 0;

        CRegion rgnInvalid;
        if (_fInvalidVScrollbar || _fInvalidHScrollbar)
        {
            InvalidateScrollbars(&rgnInvalid, &di);
            rgnInvalid.Offset(context._offset);
        }

        // get rect area to scroll
        CRect rcScroll(
            _rcContainer.TopLeft() + di._prcBorderWidths->TopLeft().AsSize(),
            sizeContainer);
        if (fRightToLeft && _fHasVScrollbar)
            rcScroll.OffsetX(_sizeScrollbars.cx);
        rcScroll.IntersectRect(context._rcClip);
        CRect rcScrollGlobal(rcScroll);
        rcScrollGlobal.OffsetRect(context._offset);
        CDispInteriorNode* pNode;

        // try to scroll bits only if it was requested, and only one
        // axis is being scrolled, and it's being scrolled less than the
        // width of the container
        fScrollBits = FALSE;
        if ((abs(sizeDiff.cx) >= sizeContainer.cx) ||   // must be smaller than rect
            (abs(sizeDiff.cy) >= sizeContainer.cy) ||
            !IsSet(CDispFlags::s_opaqueNode)       ||   // must be opaque
            IsSet(CDispFlags::s_fixedBackground))       // no fixed background
        {
            goto Invalidate;
        }

        // children must completely and opaquely fill this container
        // BUGBUG (donmarsh) - this could be expensive!
        if (!IsSet(CDispFlags::s_opaqueNode))
        {
            CDispContext opaqueContext;
            CRegion rgnScroll(rcScroll);
            opaqueContext._rcClip = rcScroll;
            if (SubtractOpaqueChildren(&rgnScroll, &opaqueContext))
            {
                goto Invalidate;
            }
        }

        // Now determine if there are any items layered on top of this
        // scroll container.  We could do partial scroll bits in this
        // scenario, but for now we completely disqualify bit scrolling
        // if there is anything overlapping us.
        pNode = this;
        for (;;)
        {
            for (CDispNode* pSibling = pNode->_pNextSiblingNode;
                 pSibling != NULL;
                 pSibling = pSibling->_pNextSiblingNode)
            {
                // does sibling intersect scroll area?
                if (pSibling->AllSet(CDispFlags::s_visibleBranchAndInView) &&
                    rcScroll.Intersects(pSibling->_rcVisBounds))
                {
                    goto Invalidate;
                }
            }

            // no intersections among this node's siblings, now check
            // our parent's siblings
            CDispInteriorNode* pParent = pNode->_pParentNode;
            if (pParent == NULL)
                break;

            pParent->TransformRect(
                &rcScroll,
                pNode->GetContentCoordinateSystem(),
                COORDSYS_PARENT,
                TRUE);
            Assert(!rcScroll.IsEmpty());
            pNode = pParent;
        }

        // we made it to the root
        Assert(pNode == pRoot);
        fScrollBits = TRUE;

Invalidate:
        // determine which children are in view, and do change notification
        Verify(CalculateInView(&context, TRUE, TRUE));

        // scroll content
        pRoot->ScrollRect(
            rcScrollGlobal,
            sizeDiff,
            this,
            rgnInvalid,
            fScrollBits);
    }

Cleanup:
    _pDispClient->NotifyScrollEvent(NULL, 0);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::SetScrollOffsetSmoothly
//
//  Synopsis:   Set new scroll offset smoothly if appropriate.  Wraps
//              SetScrollOffset().
//
//  Arguments:  offset          new scroll offset
//              fScrollBits     TRUE if we should try to scroll bits on screen
//
//  Notes:
//
//----------------------------------------------------------------------------

#define MAX_SCROLLS 50

BOOL
CDispScroller::SetScrollOffsetSmoothly(
        const SIZE& offset,
        BOOL fScrollBits,
        LONG lScrollTime)
{
    CDispNode * pNode;
    CDispRoot * pRoot;

    // determine if smooth scrolling is enabled
    if (!fScrollBits ||
        lScrollTime <= 0 ||
        !AllSet(CDispFlags::s_visibleBranchAndInView))
        goto NoSmoothScroll;
    pNode = GetRootNode();
    if (!pNode->IsDispRoot())
        return FALSE;
    pRoot = DYNCAST(CDispRoot, pNode);
    if (!pRoot->CanSmoothScroll())
        goto NoSmoothScroll;

    {
        CSize scrollDelta(offset + _sizeScrollOffset);
        int axis = (scrollDelta.cx != 0 ? 0 : 1);
        LONG cScrolls = min((long)MAX_SCROLLS, (long)abs(scrollDelta[axis]));
        LONG cScrollsDone = 0;
        CSize perScroll(g_Zero.size);
        CSize sizeScrollRemainder = scrollDelta;
        DWORD dwStart = GetTickCount(), dwTimer;

        while (cScrolls > 0)
        {
            perScroll[axis] = sizeScrollRemainder[axis] / (cScrolls--);
            sizeScrollRemainder[axis] -= perScroll[axis];

            if (!SetScrollOffset(perScroll - _sizeScrollOffset, fScrollBits))
                return FALSE;

            // Obtain new cScrolls.
            dwTimer = GetTickCount();
            cScrollsDone++;
            if (cScrolls && dwTimer != dwStart)
            {
                // See how many cScrolls we have time for by dividing the remaining time
                // by duration of last scroll, but don't increase the number (only speed up).
                LONG cSuggestedScrolls = MulDivQuick(cScrollsDone, lScrollTime - (dwTimer - dwStart), dwTimer - dwStart);
                if (cSuggestedScrolls <= 1)
                    cScrolls = 1;
                else if (cSuggestedScrolls < cScrolls)
                    cScrolls = cSuggestedScrolls;
            }
        }
        Assert(sizeScrollRemainder[axis] == 0);
    }

    return TRUE;

NoSmoothScroll:
    return SetScrollOffset(offset, fScrollBits);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::ComputeVisibleBounds
//
//  Synopsis:   Compute visible bounds for an interior node, marking children
//              that determine the edges of these bounds
//
//  Arguments:  none
//
//----------------------------------------------------------------------------

BOOL
CDispScroller::ComputeVisibleBounds()
{
    if (_rcVisBounds == _rcContainer)
        return FALSE;
    
    // visible bounds for a scrolling container is simply the container bounds
    _rcVisBounds = _rcContainer;
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::RecalcChildren
//
//  Synopsis:   Recalculate children.
//
//  Arguments:  fForceRecalc        TRUE to force recalc of this subtree
//              fSuppressInval      TRUE to suppress child invalidation
//              pContext            draw context
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::RecalcChildren(
        BOOL fForceRecalc,
        BOOL fSuppressInval,
        CDispDrawContext* pContext)
{
    // recalc children's in view flag with probable scroll bar existence
    if (_fForceVScrollbar)
    {
        _fHasVScrollbar = TRUE;
    }
    if (_fForceHScrollbar)
    {
        _fHasHScrollbar = TRUE;
    }

    BOOL fHadVScrollbar = _fHasVScrollbar;
    BOOL fHadHScrollbar = _fHasHScrollbar;

    super::RecalcChildren(fForceRecalc, fSuppressInval, pContext);

    // compute the size of this container's content
    CSize sizeContentOld = _sizeContent;
    GetScrollableContentSize(&_sizeContent);

    CDispInfo di(GetExtras());
    CRect rcVSB(g_Zero.rc);
    CRect rcHSB(g_Zero.rc);

    if (_fHasVScrollbar)
    {
        GetVScrollbarRect(&rcVSB, di);
    }

    if (_fHasHScrollbar)
    {
        GetHScrollbarRect(&rcHSB, di);
    }
    
    BOOL fScrollOffsetChanged = !CalcScrollbars();
    if (!fSuppressInval && IsVisible())
    {
        if (!fScrollOffsetChanged)
        {
            CRect rc;
            CalcDispInfo(g_Zero.rc, &di);
            if (_fInvalidVScrollbar ||
                (_fHasVScrollbar && _sizeContent.cy != sizeContentOld.cy))
            {
                GetVScrollbarRect(&rc, di);
                rcVSB.Union(rc);
                rcVSB.OffsetRect(di._borderOffset);
                pContext->AddToRedrawRegion(rcVSB);
            }
            if (_fInvalidHScrollbar ||
                (_fHasHScrollbar && _sizeContent.cx != sizeContentOld.cx))
            {
                GetHScrollbarRect(&rc, di);
                rcHSB.Union(rc);
                rcHSB.OffsetRect(di._borderOffset);
                pContext->AddToRedrawRegion(rcHSB);

                // invalidate scrollbar filler if we invalidated both
                // scroll bars.
                if (rcVSB != g_Zero.rc)
                {
                    rc.SetRect(
                        rcVSB.left,
                        rcHSB.top,
                        rcVSB.right,
                        rcHSB.bottom);
                    pContext->AddToRedrawRegion(rc);
                }
            }
        }
        else
        {
            pContext->AddToRedrawRegion(_rcContainer);
        }
    }
    
    _fInvalidVScrollbar = _fInvalidHScrollbar = FALSE;

    // if the scroll bar status or scrolloffset changed, we need to correct the
    // in-view status of children
    if ((fScrollOffsetChanged ||
         _fHasVScrollbar != fHadVScrollbar ||
         _fHasHScrollbar != fHadHScrollbar) &&
        (IsSet(CDispFlags::s_inView) ||
         pContext->_rcClip.Intersects(_rcContainer)))
    {
        CalculateInView(pContext, fForceRecalc, FALSE);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::CalcDispInfo
//
//  Synopsis:   Calculate clipping and positioning info for this node.
//
//  Arguments:  rcClip          current clip rect
//              pdi             display info structure
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::CalcDispInfo(
        const CRect& rcClip,
        CDispInfo* pdi) const
{
    //
    // user clip = current clip INTERSECT container bounds,
    //   in content coordinates with scrolling
    //
    // border clip = current clip INTERSECT container bounds,
    //   in container coordinates (no scrolling)
    //
    // flow clip = current clip INTERSECT container bounds,
    //   in content coordinates with scrolling
    //
    CDispInfo& di = *pdi;   // notational convenience

    // set scrolling offset
    di._scrollOffset = _sizeScrollOffset;
    di._pInsetOffset = &((CSize&)g_Zero.size);

    // content size
    di._sizeContent = _sizeContent;

    // offset to local coordinates
    _rcContainer.GetTopLeft(&(di._borderOffset.AsPoint()));

    // calc container clip intersect with container bounds
    di._rcContainerClip = rcClip;
    di._rcContainerClip.IntersectRect(_rcContainer);
    di._rcContainerClip.OffsetRect(-di._borderOffset);

    // calc rect inside border and scroll bars
    _rcContainer.GetSize(&di._sizeBackground);
    if (_fHasVScrollbar)
    {
        di._sizeBackground.cx -= _sizeScrollbars.cx;
    }
    if (_fHasHScrollbar)
    {
        di._sizeBackground.cy -= _sizeScrollbars.cy;
    }
    di._rcPositionedClip.SetRect(di._sizeBackground);
    di._rcPositionedClip.IntersectRect(di._rcContainerClip);
    di._contentOffset = g_Zero.size;

    // contents scroll
    if (!IsSet(CDispFlags::s_fixedBackground))
    {
        di._rcPositionedClip.OffsetRect(-_sizeScrollOffset);
        di._rcBackgroundClip = di._rcPositionedClip;
    }
    else
    {
        di._rcBackgroundClip = di._rcPositionedClip;
        di._rcPositionedClip.OffsetRect(-_sizeScrollOffset);
    }

    // adjust for right to left coordinate system
    if (IsRightToLeft())
    {
        di._contentOffset.cx = di._sizeBackground.cx;
        if(_fHasVScrollbar)
        {
            di._contentOffset.cx += _sizeScrollbars.cx;
        }
        di._rcPositionedClip.OffsetX(-di._sizeBackground.cx);
        di._rcBackgroundClip.OffsetX(-di._sizeBackground.cx);
    }
    di._rcFlowClip = di._rcPositionedClip;
    
    // size of background is big enough to fill background and content
    di._sizeBackground.Max(di._sizeContent);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::GetNodeTransform
//
//  Synopsis:   Return an offset that can be added to points in the source
//              coordinate system, producing values for the destination
//              coordinate system.
//
//  Arguments:  pOffset         returns offset value
//              source          source coordinate system
//              destination     destination coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::GetNodeTransform(
        CSize* pOffset,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination) const
{
    Assert(destination < source);
    BOOL fRightToLeft = IsRightToLeft();

    *pOffset = g_Zero.size;

    switch (source)
    {
    case COORDSYS_NONFLOWCONTENT:
    case COORDSYS_CONTENT:
        // COORDSYS_CONTENT --> COORDSYS_CONTAINER
        if(!fRightToLeft)
            *pOffset = _sizeScrollOffset;
        else
        {
            pOffset->cx = _rcContainer.Width() + _sizeScrollOffset.cx;
            pOffset->cy = _sizeScrollOffset.cy;
        }
        if (destination == COORDSYS_CONTAINER)
            break;
        // fall thru to continue transformation

    case COORDSYS_CONTAINER:
        // COORDSYS_CONTAINER --> COORDSYS_PARENT
        *pOffset += _rcContainer.TopLeft().AsSize();
        if (destination == COORDSYS_PARENT)
            break;
        // fall thru to continue transformation

    default:
        Assert(source != COORDSYS_GLOBAL);
        // COORDSYS_PARENT --> COORDSYS_GLOBAL
        if (_pParentNode != NULL)
        {
            CSize globalOffset;
            _pParentNode->GetNodeTransform(
                &globalOffset,
                GetContentCoordinateSystem(),
                COORDSYS_GLOBAL);
            *pOffset += globalOffset;
        }
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::GetNodeTransform
//
//  Synopsis:   Return a context that can be used to transform values
//              in the source coordinate system, producing values for the
//              destination coordinate system.
//
//  Arguments:  pContext        returns context
//              source          source coordinate system
//              destination     destination coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispScroller::GetNodeTransform(
        CDispContext* pContext,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination) const
{
    Assert(destination < source);
    BOOL fRightToLeft = IsRightToLeft();

    CSize& offset = pContext->_offset;
    CRect& rcClip = pContext->_rcClip;

    switch (source)
    {
    case COORDSYS_NONFLOWCONTENT:
    case COORDSYS_CONTENT:
        {
            // COORDSYS_CONTENT --> COORDSYS_CONTAINER
            // all layers clip to container regardless of layer type
            CSize sizeContained(_rcContainer.Size());

            // clip to scroll bars
            if (_fHasVScrollbar)
            {
                sizeContained.cx -= _sizeScrollbars.cx;
            }
            if (_fHasHScrollbar)
            {
                sizeContained.cy -= _sizeScrollbars.cy;
            }

            if(!fRightToLeft)
            {
                offset = _sizeScrollOffset;
                rcClip.SetRect(-_sizeScrollOffset.AsPoint(), sizeContained);
            }
            else
            {
                offset.cx = _rcContainer.Width() + _sizeScrollOffset.cx;
                offset.cy = _sizeScrollOffset.cy;

                CPoint pt(_sizeScrollOffset.cx, -_sizeScrollOffset.cy);
                rcClip.SetRect(pt, sizeContained);
                rcClip.MirrorX();
            }
        }

        if (destination == COORDSYS_CONTAINER)
            break;
        // fall thru to continue transformation

    case COORDSYS_CONTAINER:
        // COORDSYS_CONTAINER --> COORDSYS_PARENT
        if (source == COORDSYS_CONTAINER)
        {
            pContext->SetNoClip();
            offset = _rcContainer.TopLeft().AsSize();
        }
        else
        {
            offset += _rcContainer.TopLeft().AsSize();
        }
        if (destination == COORDSYS_PARENT)
            break;
        // fall thru to continue transformation

    default:
        Assert(source != COORDSYS_GLOBAL);
        // COORDSYS_PARENT --> COORDSYS_GLOBAL
        if (source == COORDSYS_PARENT)
        {
            if (_pParentNode != NULL)
            {
                _pParentNode->GetNodeTransform(
                    pContext,
                    GetContentCoordinateSystem(),
                    COORDSYS_GLOBAL);
            }
            else
                pContext->SetToIdentity();
        }
        else
        {
            if (_pParentNode != NULL)
            {
                CDispContext globalContext;
                _pParentNode->GetNodeTransform(
                    &globalContext,
                    GetContentCoordinateSystem(),
                    COORDSYS_GLOBAL);
                globalContext._rcClip.OffsetRect(-offset);  // to source coords.
                rcClip.IntersectRect(globalContext._rcClip);
                offset += globalContext._offset;
            }
        }
        break;
    }
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispScroller::DumpBounds
//
//  Synopsis:   Dump custom information for this node.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//----------------------------------------------------------------------------

void
CDispScroller::DumpBounds(HANDLE hFile, long level, long childNumber)
{
    super::DumpBounds(hFile, level, childNumber);

    // print scroll offset
    WriteHelp(hFile, _T("<<i>scroll:<</i> <0d>,<1d><<br>\r\n"),
        _sizeScrollOffset.cx, _sizeScrollOffset.cy);

    // dump scroll bar info
    WriteHelp(hFile, _T("<<i>V scrollbar:<</i> width=<0d>"), _sizeScrollbars.cx);
    if (_fHasVScrollbar)
    {
        WriteString(hFile, _T(" visible"));
    }
    else
    {
        WriteString(hFile, _T(" !visible"));
    }
    if (_fForceVScrollbar)
    {
        WriteString(hFile, _T(" FORCED"));
    }
    DumpEndLine(hFile);

    WriteHelp(hFile, _T("<<i>H scrollbar:<</i> height=<0d>"), _sizeScrollbars.cy);
    if (_fHasHScrollbar)
    {
        WriteString(hFile, _T(" visible"));
    }
    else
    {
        WriteString(hFile, _T(" !visible"));
    }
    if (_fForceHScrollbar)
    {
        WriteString(hFile, _T(" FORCED"));
    }
    DumpEndLine(hFile);
}
#endif
