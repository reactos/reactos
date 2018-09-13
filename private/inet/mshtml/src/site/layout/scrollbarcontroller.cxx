//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       scrollbarcontroller.cxx
//
//  Contents:   Transient object to control scroll bar during user interaction.
//
//  Classes:    CScrollbarController
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SCROLLBARCONTROLLER_HXX_
#define X_SCROLLBARCONTROLLER_HXX_
#include "scrollbarcontroller.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_CDBASE_HXX_
#define X_CDBASE_HXX_
#include "cdbase.hxx"
#endif

#ifdef _MAC
#ifndef X_MACCONTROLS_HXX_
#define X_MACCONTROLS_HXX_
#include "maccontrols.h"
#endif
#endif

//
// This is the "timer ID" which will be used when the user has pressed
// either one of the scrollbar buttons, or the paging areas, to time
// the repeat rate for scrolling.
//

#define SB_REPEAT_TIMER 1


MtDefine(THREADSTATE_pSBC, THREADSTATE, "THREADSTATE::_pSBC")


//+----------------------------------------------------------------------------
//
//  Function:   InitScrollbar
//
//  Synopsis:   Allocate scrollbar helper
//
//  Arguments:  pts - THREADSTATE for current thread
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------
HRESULT
InitScrollbar(
    THREADSTATE *   pts)
{
    Assert(pts);

    pts->pSBC = new (Mt(THREADSTATE_pSBC)) CScrollbarController();
    if (!pts->pSBC)
        RRETURN(E_OUTOFMEMORY);
    RRETURN(S_OK);
}


//+----------------------------------------------------------------------------
//
//  Function:   DeinitScrollbar
//
//  Synopsis:   Delete scrollbar helper
//
//  Arguments:  pts - THREADSTATE for current thread
//
//-----------------------------------------------------------------------------
void
DeinitScrollbar(
    THREADSTATE *   pts)
{
    Assert(pts);
    delete pts->pSBC;
}


//+---------------------------------------------------------------------------
//
//  Function:   InitScrollbarTiming
//
//  Synopsis:   Get scrollbar's timing info into a thread local storage
//              scrollTimeInfo structure
//
//----------------------------------------------------------------------------

void
InitScrollbarTiming()
{
    THREADSTATE * pts = GetThreadState();

    Assert (pts);
    Assert (pts->pSBC);

    pts->scrollTimeInfo.lRepeatDelay = pts->pSBC->GetRepeatDelay ();
    pts->scrollTimeInfo.lRepeatRate  = pts->pSBC->GetRepeatRate ();
    pts->scrollTimeInfo.lFocusRate   = pts->pSBC->GetFocusRate ();
}


static UINT
TranslateSBAction(CScrollbar::SCROLLBARPART part)
{
    switch (part)
    {
    case CScrollbar::SB_PREVBUTTON: return SB_LINEUP;
    case CScrollbar::SB_NEXTBUTTON: return SB_LINEDOWN;
    case CScrollbar::SB_PREVTRACK:  return SB_PAGEUP;
    case CScrollbar::SB_NEXTTRACK:  return SB_PAGEDOWN;
    case CScrollbar::SB_THUMB:      return SB_THUMBPOSITION;
    default:                        Assert(FALSE); break;
    }
    
    // here only on error
    return SB_LINEUP;
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbarController::StartScrollbarController
//              
//  Synopsis:   Start a scroll bar controller if necessary.
//              
//  Arguments:  pLayout         layout object to be called on scroll changes
//              pDispScroller   display scroller node
//              pServerHost     server host
//              buttonWidth     custom scroll bar button width
//              pMessage        message that caused creation of controller
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbarController::StartScrollbarController(
        CLayout* pLayout,
        CDispScroller* pDispScroller,
        CServer* pServerHost,
        long buttonWidth,
        CMessage* pMessage)
{
    Assert(pLayout != NULL);
    Assert(pDispScroller != NULL);
    Assert(pServerHost != NULL);
    Assert(pMessage != NULL);
    BOOL fRightToLeft;
    
    CScrollbarController* pSBC = TLS(pSBC);
    Assert(pSBC != NULL);
   
    // just to make sure previous controller is stopped
    if (pSBC->_pLayout != NULL)
    {
        StopScrollbarController();
    }
   
#ifdef UNIX
    if (pMessage->dwKeyState & (MK_CONTROL | MK_SHIFT | MK_ALT))
    {
        if (pMessage->message == WM_MBUTTONDOWN ||
            (pMessage->dwKeyState & MK_CONTROL) != MK_CONTROL)
            return;
    }
#endif
 
    pSBC->_direction = (pMessage->htc == HTC_HSCROLLBAR ? 0 : 1);
    pSBC->_pDispScroller = pDispScroller;
    pSBC->_drawInfo.Init(pLayout->ElementOwner());
    fRightToLeft = (pSBC->_direction == 0 && pDispScroller->IsRightToLeft());
    
    // calculate scroll bar rect
    pDispScroller->GetClientRect(
        &pSBC->_rcScrollbar,
        (pSBC->_direction == 0
            ? CLIENTRECT_HSCROLLBAR
            : CLIENTRECT_VSCROLLBAR));
    Assert(pSBC->_rcScrollbar.Contains(pMessage->ptContent));
    
    LONG contentSize, containerSize, scrollAmount;
    pSBC->GetScrollInfo(&contentSize, &containerSize, &scrollAmount);
    
    // if the scrollbar is inactive, it doesn't matter what was pressed
    if (contentSize <= containerSize)
        return;

    // what was pressed?
    pSBC->_partPressed = GetPart(
        pSBC->_direction,
        pSBC->_rcScrollbar,
        pMessage->ptContent,
        contentSize,
        containerSize,
        scrollAmount,
        buttonWidth,
        pSBC->GetDrawInfo(),
        pDispScroller->IsRightToLeft());
    Assert(pSBC->_partPressed != SB_NONE);
    
    // if inactive track was pressed, no more work to do
    if (pSBC->_partPressed == SB_TRACK)
        return;
    
    // make scroll bar controller active
    pSBC->_partPressedStart = pSBC->_partPressed;
    pSBC->_pLayout = pLayout;
    pSBC->_pDispScroller = pDispScroller;
    pSBC->_pServerHost = pServerHost;
    pSBC->_buttonWidth = buttonWidth;
    pSBC->_ptMouse = pMessage->ptContent;
   
    LONG lScrollTime = MAX_SCROLLTIME;

#ifdef UNIX // Handle UNIX motif scrollbar's special actions. 
    // Is MK_CONTROL pressed?
    if (pMessage->dwKeyState & MK_CONTROL)
    {
        switch (pSBC->_partPressed)
        {
        case CScrollbar::SB_PREVBUTTON:
        case CScrollbar::SB_PREVTRACK:
            pLayout->OnScroll(pSBC->_direction, SB_TOP, 0);
            break;
        case CScrollbar::SB_NEXTBUTTON:
        case CScrollbar::SB_NEXTTRACK:
            pLayout->OnScroll(pSBC->_direction, SB_BOTTOM, 0);
            break;
        }
        return;
    }
    if (pMessage->message == WM_MBUTTONDOWN)
    {
        pSBC->MoveThumb(pMessage->pt);
        pSBC->GetScrollInfo(&contentSize, &containerSize, &scrollAmount);
        pSBC->_partPressedStart = pSBC->_partPressed = SB_THUMB;
    }
#endif // UNIX

    // if thumbing, compute hit point offset from top of thumb
    if (pSBC->_partPressed == SB_THUMB)
    {
        long trackSize = GetTrackSize(
            pSBC->_direction, pSBC->_rcScrollbar, pSBC->_buttonWidth);
        long thumbSize = GetThumbSize(
            pSBC->_direction, pSBC->_rcScrollbar, contentSize, containerSize,
            pSBC->_buttonWidth, pSBC->GetDrawInfo());
        // _mouseInThumb is the xPos of the mouse in from the left edge of the thumb in LTR cases
        // and xPos of the mouse in from the right edge of the thumb in RTL HSCROLL cases
        if(!fRightToLeft)
        {
            pSBC->_mouseInThumb = 
                pSBC->_ptMouse[pSBC->_direction] -
                pSBC->_rcScrollbar[pSBC->_direction] -
                GetScaledButtonWidth(pSBC->_direction, pSBC->_rcScrollbar, pSBC->_buttonWidth) -
                GetThumbOffset(contentSize, containerSize, scrollAmount, trackSize, thumbSize);
        }
        else
        {
            pSBC->_mouseInThumb = 
                pSBC->_rcScrollbar.right -
                GetScaledButtonWidth(pSBC->_direction, pSBC->_rcScrollbar, pSBC->_buttonWidth) +
                GetThumbOffset(contentSize, containerSize, scrollAmount, trackSize, thumbSize) -
                pSBC->_ptMouse.x;
        }
        Assert(pSBC->_mouseInThumb >= 0);
        // no smooth scrolling
        lScrollTime = 0;
    }

    // capture the mouse
    Assert(pServerHost->State() >= OS_INPLACE);
    HWND hwnd = pServerHost->_pInPlace->_hwnd;
    if (!hwnd)
    {
        IGNORE_HR(pServerHost->_pInPlace->_pInPlaceSite->GetWindow(&hwnd));
    }
#ifdef _MAC
    TrackMacScroll(pSBC, pServerHost, pMessage->ptContent, pSBC->_direction, pSBC->_partPressed);
#else
    if (FAILED(GWSetCapture(
            pSBC,
            ONMESSAGE_METHOD(CScrollbarController, OnMessage, onmessage),
            hwnd)))
    {
        pSBC->_pLayout = NULL;
        return;
    }
 
    // set timer for repeating actions
    if (pSBC->_partPressed != SB_THUMB)
    {
        // perform first action
        pLayout->OnScroll(
            pSBC->_direction, TranslateSBAction(pSBC->_partPressed), 0, FALSE, lScrollTime);

        CSize scrollOffset;
        pSBC->_pDispScroller->GetScrollOffset(&scrollOffset);
        scrollAmount = scrollOffset[pSBC->_direction];

        // set timer for subsequent action
        FormsSetTimer(
            pSBC,
            ONTICK_METHOD(CScrollbarController, OnTick, ontick),
            SB_REPEAT_TIMER,
            GetRepeatDelay() );
    }
    
    // invalidate the part we hit, if necessary
    pLayout->OpenView();
    InvalidatePart(
        pSBC->_partPressed,
        pSBC->_direction,
        pSBC->_rcScrollbar,
        contentSize,
        containerSize,
        scrollAmount,
        buttonWidth,
        pDispScroller,
        pSBC->GetDrawInfo());
#endif // _MAC
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbarController::StopScrollbarController
//              
//  Synopsis:   Stop an existing scroll bar controller.
//              
//  Arguments:  none
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbarController::StopScrollbarController()
{
    CScrollbarController* pSBC = TLS(pSBC);
    Assert(pSBC != NULL);
    
    if (pSBC->_pLayout != NULL)
    {
        // report scroll change to layout, which will take care of
        // invalidation

        pSBC->_pLayout->OnScroll(pSBC->_direction, SB_ENDSCROLL, 0);

        // do this before GWReleaseCapture, or StopScrollbarController
        // will be called recursively
        pSBC->_pLayout = NULL;

        if (pSBC->_partPressed != SB_THUMB)
        {
            FormsKillTimer(pSBC, SB_REPEAT_TIMER);
        }
#ifndef _MAC
        GWReleaseCapture(pSBC);
#endif
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbarController::OnMessage
//              
//  Synopsis:   Handle messages sent to this scroll bar controller.
//              
//  Arguments:  
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

LRESULT
CScrollbarController::OnMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    //
    // If the serverhost somehow went into the running state
    // just terminate capture and bail out.  This could happen
    // because of asynchronous activation/deactivation (as in IE)
    //
    if (_pServerHost->State() < OS_INPLACE)
        goto Terminate;
    
    switch (msg)
    {
    case WM_MOUSEMOVE:
        MouseMove( CPoint( MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y ) );
        break;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        AssertSz(FALSE, "CScrollbarController got unexpected message");
        break;

#ifdef UNIX
    case WM_MBUTTONUP:
#endif
    case WM_LBUTTONUP:
        if (_partPressed != SB_NONE)
        {
            // invalidate just the part that was pressed
            _partPressed = SB_NONE;
            CSize scrollOffset;
            _pDispScroller->GetScrollOffset(&scrollOffset);
            LONG containerSize = _rcScrollbar.Size(_direction);
            Verify(_pLayout->OpenView());
            InvalidatePart(
                _partPressedStart,
                _direction,
                _rcScrollbar,
                _pDispScroller->GetContentSize()[_direction],
                containerSize,
                scrollOffset[_direction],
                _buttonWidth,
                _pDispScroller,
                &_drawInfo);
        }
        
        // fall thru to Terminate...

    case WM_CAPTURECHANGED:
        goto Terminate;
    }
    
    return 0;
    
Terminate:
    StopScrollbarController();
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbarController::MouseMove
//              
//  Synopsis:   Handle mouse move events.
//              
//  Arguments:  pt      new mouse location
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbarController::MouseMove(const CPoint& pt)
{
    _ptMouse = pt;
    _pDispScroller->TransformPoint(
        &_ptMouse,
        COORDSYS_GLOBAL,
        COORDSYS_CONTAINER);
    
    switch (_partPressedStart)
    {
    case SB_NONE:
    case SB_TRACK:
        AssertSz(FALSE, "unexpected call to CScrollbarController::MouseMoved");
        break;
        
    case SB_THUMB:
        {
            LONG contentSize = _pDispScroller->GetContentSize()[_direction];
            Assert(contentSize >= 0);
            LONG trackSize =
                GetTrackSize(_direction, _rcScrollbar, _buttonWidth) -
                GetThumbSize(_direction, _rcScrollbar, contentSize,
                             _rcScrollbar.Size(_direction), _buttonWidth,
                             &_drawInfo);
            if (trackSize <= 0)
                break;  // can't move thumb

            // NOTE: we're not currently checking to see if the mouse point
            // is out of range perpendicular to the scroll bar axis
            BOOL fRightToLeft = (_direction == 0 && _pDispScroller->IsRightToLeft());
            LONG trackPos;
            if(!fRightToLeft)
            {
                trackPos = _ptMouse[_direction] -
                           _rcScrollbar[_direction] -
                           GetScaledButtonWidth(_direction, _rcScrollbar, _buttonWidth) -
                           _mouseInThumb;
            }
            else
            {
                trackPos = _rcScrollbar.right -
                           GetScaledButtonWidth(_direction, _rcScrollbar, _buttonWidth) -
                           _mouseInThumb -
                           _ptMouse.x;
            }
            LONG scrollOffset;
            if (trackPos <= 0)
            {
                scrollOffset = 0;
            }
            else
            {
                contentSize -= _rcScrollbar.Size(_direction);
                scrollOffset = MulDivQuick(trackPos, contentSize, trackSize);
                if(fRightToLeft)
                    scrollOffset = -scrollOffset;
            }
            
            _pLayout->OnScroll(_direction, SB_THUMBPOSITION, scrollOffset);
        }
        break;
        
    default:
        {
            // find out what the mouse would be pressing in its new location.
            // If it's not the same as it used to be, invalidate the part.
            SCROLLBARPART partPressedOld = _partPressed;
            LONG contentSize, containerSize, scrollAmount;
            GetScrollInfo(&contentSize, &containerSize, &scrollAmount);
            _partPressed = GetPart(
                _direction,
                _rcScrollbar,
                _ptMouse,
                contentSize,
                containerSize,
                scrollAmount,
                _buttonWidth,
                &_drawInfo,
                _pDispScroller->IsRightToLeft());
            if (_partPressed != _partPressedStart)
            {
                _partPressed = SB_NONE;
            }
            if (_partPressed != partPressedOld)
            {
                SCROLLBARPART invalidPart = _partPressed;
                if (_partPressed != SB_NONE)
                {
                    // perform scroll action and set timer
                    OnTick(SB_REPEAT_TIMER);
                }
                else
                {
                    invalidPart = partPressedOld;
                }
                Verify(_pLayout->OpenView());
                InvalidatePart(
                    invalidPart,
                    _direction,
                    _rcScrollbar,
                    contentSize,
                    containerSize,
                    scrollAmount,
                    _buttonWidth,
                    _pDispScroller,
                    &_drawInfo);
            }
        }
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbarController::OnTick
//              
//  Synopsis:   Handle mouse timer ticks to implement repeated scroll actions
//              and focus blinking.
//              
//  Arguments:  id      timer event type
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

HRESULT
CScrollbarController::OnTick(UINT id)
{
    // for now, SB_REPEAT_TIMER is the only id we use
    Assert(id == SB_REPEAT_TIMER);
    
    // timer tick snuck through right before we disabled it
    if (_pLayout == NULL)
        return S_OK;

    if (id == SB_REPEAT_TIMER)
    {
        if (_partPressed != SB_NONE)
        {
            // while paging, thumb may have moved underneath the mouse
            if (_partPressed == SB_PREVTRACK || _partPressed == SB_NEXTTRACK)
            {
                LONG contentSize, containerSize, scrollAmount;
                BOOL fRightToLeft = (_direction == 0 && _pDispScroller->IsRightToLeft());

                GetScrollInfo(&contentSize, &containerSize, &scrollAmount);
#if !defined(UNIX) && !defined(_MAC) //Under Unix where the thumb goes during a repetition
             // of page down makes no difference.

                _partPressed = GetPart(
                    _direction,
                    _rcScrollbar,
                    _ptMouse,
                    contentSize,
                    containerSize,
                    scrollAmount,
                    _buttonWidth,
                    &_drawInfo,
                    fRightToLeft);
                if (_partPressed != _partPressedStart)
                {
                    _partPressed = SB_NONE;
                    Verify(_pLayout->OpenView());
                    InvalidatePart(
                        _partPressedStart,
                        _direction,
                        _rcScrollbar,
                        contentSize,
                        containerSize,
                        scrollAmount,
                        _buttonWidth,
                        _pDispScroller,
                        &_drawInfo);
                    return S_OK;
                }
#endif
            }
            
#ifndef _MAC            
            //
            // Now that at least one click has come in (to be processed right afterwards)
            // we set the timer to be the repeat rate.
            //

            FormsSetTimer(
                    this,
                    ONTICK_METHOD(CScrollbarController, OnTick, ontick),
                    SB_REPEAT_TIMER,
                    GetRepeatRate() );
#endif

            // repeat this scroll action
            _pLayout->OnScroll(_direction, TranslateSBAction(_partPressed), 0, FALSE, GetRepeatRate());
        }
    }
    
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbarController::GetScrollInfo
//              
//  Synopsis:   Get basic size information needed to perform scrolling calcs.
//              
//  Arguments:  pContentSize        size of content to be scrolled
//              pContainerSize      size of container
//              pScrollAmount       current scroll amount
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbarController::GetScrollInfo(
        LONG* pContentSize,
        LONG* pContainerSize,
        LONG* pScrollAmount) const
{
    Assert(_pDispScroller != NULL);
    
    // get content size
    *pContentSize = _pDispScroller->GetContentSize()[_direction];
    
    // get container size
    *pContainerSize = _rcScrollbar.Size(_direction);
    
    // get current scroll offset
    CSize scrollOffset;
    _pDispScroller->GetScrollOffset(&scrollOffset);
    *pScrollAmount = scrollOffset[_direction];
}

#ifdef UNIX // Grap codes from MouseMove
void 
CScrollbarController::MoveThumb(const CPoint& pt)
{
    LONG contentSize = _pDispScroller->GetContentSize()[_direction];
    Assert(contentSize >= 0);
    LONG thumbSize =
        GetThumbSize(_direction, _rcScrollbar, contentSize,
                     _rcScrollbar.Size(_direction), _buttonWidth,
                     &_drawInfo);
    LONG trackSize =
        GetTrackSize(_direction, _rcScrollbar, _buttonWidth) - thumbSize;
    if (trackSize <= 0)
        return;  // can't move thumb

    // NOTE: we're not currently checking to see if the mouse point
    // is out of range perpendicular to the scroll bar axis
    LONG trackPos =
        pt[_direction] - _rcScrollbar[_direction] -
        GetScaledButtonWidth(_direction, _rcScrollbar, _buttonWidth) -
        thumbSize/2; //_mouseInThumb;
    LONG scrollOffset;
    if (trackPos <= 0)
    {
        scrollOffset = 0;
    }
    else
    {
        contentSize -= _rcScrollbar.Size(_direction);
        scrollOffset = trackPos * contentSize;
        if (scrollOffset < trackPos || scrollOffset < contentSize)
        {
            // oops, it overflowed.  Do a less accurate calculation in
            // a safer order.
            scrollOffset = contentSize / trackSize * trackPos;
        }
        else
        {
            scrollOffset /= trackSize;
        }
    }
            
    // report scroll change to layout, which will take care of
    // invalidation
    if (_direction == 0 && _pDispScroller->IsRightToLeft())
        scrollOffset -= contentSize;
    _pLayout->OnScroll(_direction, SB_THUMBPOSITION, scrollOffset);
}
#endif // UNIX

