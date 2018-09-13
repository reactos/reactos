//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       scrollbar.cxx
//
//  Contents:   Class to render default horizontal and vertical scrollbars.
//
//  Classes:    CScrollbar
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SCROLLBAR_HXX_
#define X_SCROLLBAR_HXX_
#include "scrollbar.hxx"
#endif

#ifndef X_BUTTUTIL_HXX_
#define X_BUTTUTIL_HXX_
#include "buttutil.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifdef UNIX
#ifndef X_UNIXCTLS_HXX_
#define X_UNIXCTLS_HXX_
#include "unixctls.hxx"
#endif
#endif // UNIX

#ifdef _MAC
#ifndef X_MACCONTROLS_HXX_
#define X_MACCONTROLS_HXX_
#include "maccontrols.h"
#endif
#endif
//+---------------------------------------------------------------------------
//
//  Class:      CScrollButton
//
//  Synopsis:   Utility class to draw scroll bar buttons.
//
//----------------------------------------------------------------------------

class CScrollButton :
    public CUtilityButton
{
public:
                            CScrollButton(
                                ThreeDColors* pColors,
                                BOOL fFlat)
                                    {Assert(pColors != NULL);
                                    _pColors = pColors;
                                    _fFlat = fFlat;
#ifdef UNIX // This is ok for now.
                                    _bMotifScrollBarBtn = TRUE;
#endif
                                    }
                            ~CScrollButton() {}
                            
    virtual ThreeDColors &  GetColors()
                                    {return *_pColors;}
        
private:
    ThreeDColors*   _pColors;
};


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::Draw
//              
//  Synopsis:   Draw the scroll bar in the given direction.
//              
//  Arguments:  direction           0 for horizontal, 1 for vertical
//              rcScrollbar         bounds of entire scroll bar
//              rcRedraw            bounds to be redrawn
//              contentSize         size of content controlled by scroll bar
//              containerSize       size of area to scroll within
//              scrollAmount        amount that the content is scrolled
//              partPressed         which part, if any, is pressed
//              hdc                 DC to draw into
//              params              customizable scroll bar parameters
//              pDI                 draw info
//              dwFlags             rendering flags
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::Draw(
        int direction,
        const CRect& rcScrollbar,
        const CRect& rcRedraw,
        long contentSize,
        long containerSize,
        long scrollAmount,
        CScrollbar::SCROLLBARPART partPressed,
        HDC hdc,
        const CScrollbarParams& params,
        CDrawInfo *pDI,
        DWORD dwFlags)
{
    Assert(hdc != NULL);
    // for now, we're using CDrawInfo, which should have the same hdc
    Assert(pDI->_hdc == hdc);

    // trivial rejection if nothing to draw
    if (!rcScrollbar.Intersects(rcRedraw))
        return;
    
#ifdef _MAC
    DrawMacScrollbar(pDI, direction, (RECT*) &rcScrollbar,
                    contentSize, containerSize, scrollAmount);
    return;
#endif

    BOOL fDisabled = (params._fForceDisabled) || (containerSize >= contentSize);
    long scaledButtonWidth =
        GetScaledButtonWidth(direction, rcScrollbar, params._buttonWidth);
    
    // compute rects for buttons and track
    CRect rcTrack(rcScrollbar);
    rcTrack[direction] += scaledButtonWidth;
    rcTrack[direction+2] -= scaledButtonWidth;
    
    // draw buttons unless requested not to (it's expensive to draw these!)
    if ((dwFlags & DISPSCROLLBARHINT_NOBUTTONDRAW) == 0)
    {
        CRect rcButton[2];
        rcButton[0] = rcScrollbar;
        rcButton[0][direction+2] = rcTrack[direction];
        rcButton[1] = rcScrollbar;
        rcButton[1][direction] = rcTrack[direction+2];
        
        // draw buttons
        CSize sizeButton;
        pDI->DocumentFromWindow(
            &sizeButton, rcButton[0].Width(), rcButton[0].Height());
        for (int i = 0; i < 2; i++)
        {
            if (rcRedraw.Intersects(rcButton[i]))
            {
                BOOL fButtonPressed =
                    (i == 0 && partPressed == SB_PREVBUTTON) ||
                    (i == 1 && partPressed == SB_NEXTBUTTON);
                CScrollButton scrollButton(params._pColors, params._fFlat);
                scrollButton.DrawButton(
                    pDI,
                    NULL,   // no hwnd, we don't want to invalidate
                    (direction == 0
                        ? (i == 0 ? BG_LEFT : BG_RIGHT)
                        : (i == 0 ? BG_UP : BG_DOWN)),
                    fButtonPressed,
                    !fDisabled,
                    FALSE,  // never focused
                    rcButton[i],
                    sizeButton,
                    0);     // assume both button glyphs are the same size
            }
        }
    }
    
    // draw track
    if (rcRedraw.Intersects(rcTrack))
    {
        if (fDisabled)
        {
            // no thumb, so draw non-pressed track
            DrawTrack(rcTrack, FALSE, fDisabled, hdc, params);
        }
        
        else
        {
            // calculate thumb rect
            CRect rcThumb;
            GetPartRect(
                &rcThumb,
                SB_THUMB,
                direction,
                rcScrollbar,
                contentSize,
                containerSize,
                scrollAmount,
                params._buttonWidth,
                pDI,
                FALSE);
                
            // can track contain the thumb?
            if (!rcTrack.Contains(rcThumb))
            {
                DrawTrack(rcTrack, FALSE, fDisabled, hdc, params);
            }
            
            else
            {
                // draw previous track
                CRect rcTrackPart(rcTrack);
                rcTrackPart[direction+2] = rcThumb[direction];
                if (rcRedraw.Intersects(rcTrackPart))
                {
                    DrawTrack(rcTrackPart, partPressed == SB_PREVTRACK,
                              fDisabled, hdc, params);
                }
                
                // draw thumb
                if (rcRedraw.Intersects(rcThumb))
                {
                    DrawThumb(rcThumb, partPressed == SB_THUMB, hdc, params, pDI );
                }
                
                // draw next track
                rcTrackPart = rcTrack;
                rcTrackPart[direction] = rcThumb[direction+2];
                if (rcRedraw.Intersects(rcTrackPart))
                {
                    DrawTrack(rcTrackPart, partPressed == SB_NEXTTRACK,
                              fDisabled, hdc, params);
                }
            }
        }
    }
#ifdef UNIX
    //
    // Kind of a copout, but it works
    // We draw the 3D border around the whole scroll bar anytime
    // any part of the scroll bar is updated
    //
    
    IGNORE_HR(BRDrawBorder (
        pDI, (RECT *) &rcScrollbar, fmBorderStyleSunken,
        0, params._pColors, BRFLAGS_MONO ));
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::DrawTrack
//              
//  Synopsis:   Draw the scroll bar track.
//              
//  Arguments:  rcTrack     bounds of track
//              fPressed    TRUE if this portion of track is pressed
//              fDisabled   TRUE if scroll bar is disabled
//              hdc         HDC to draw into
//              params      customizable scroll bar parameters
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::DrawTrack(
        const CRect& rcTrack,
        BOOL fPressed,
        BOOL fDisabled,
        HDC hdc,
        const CScrollbarParams& params)
{
#ifdef _MAC
    return;
#endif
#ifdef UNIX
    {
        ScrollBarInfo info;
        info.lStyle = params._bDirection ? SB_VERT : SB_HORZ;
        info.bDisabled = FALSE;
        MwPaintMotifScrollRect(hdc,
                                LeftTopThumbRect, // It draws both sides.
                                (LPRECT)&rcTrack,
                                FALSE,
                                &info);
        return;
    }
#endif
    
    ThreeDColors& colors = *params._pColors;
    HBRUSH hbr;
    BOOL fDither = TRUE;
    
    if (params._fFlat)
    {
        hbr = GetCachedBmpBrush(IDB_DITHER);
        SetTextColor(hdc, colors.BtnFace());
        SetBkColor(hdc, (fPressed) ? 
                   colors.BtnShadow() :
                   colors.BtnHighLight());
    }
    else
    {
        //
        // Check to see if we have to dither
        //
    
        //  LaszloG: The condition is directly from the NT GDI implementation
        //           of DefWindowProc's handling of WM_CTLCOLORSCROLLBAR.
        //           The codeis on \\rastaman\ntwinie\src\ntuser\kernel\dwp.c
    
        fDither =
            GetDeviceCaps(hdc, BITSPIXEL) < 8 ||
            colors.BtnHighLight() == GetSysColorQuick(COLOR_WINDOW) ||
            colors.BtnHighLight() != GetSysColorQuick(COLOR_SCROLLBAR);
        
        if (fDither)
        {
            hbr = ::GetCachedBmpBrush(IDB_DITHER);
            COLORREF dither1 = colors.BtnFace();
            COLORREF dither2 = colors.BtnHighLight();
            if (fPressed)
            {
                dither1 ^= 0x00ffffff;
                dither2 ^= 0x00ffffff;
            }
            SetTextColor(hdc, dither1);
            SetBkColor(hdc, dither2);
        }
        else
        {
            COLORREF color = colors.BtnHighLight();
            if (fPressed)
            {
                color ^= 0x00ffffff;
            }
            hbr = ::GetCachedBrush(color);
        }
    }
    
    if (fDither)
    {
        CPoint pt;
        ::GetViewportOrgEx(hdc, &pt);
        pt += rcTrack.TopLeft().AsSize();
#ifndef WINCE
        // not supported on WINCE
        ::UnrealizeObject(hbr);
#endif
        ::SetBrushOrgEx(hdc, POSITIVE_MOD(pt.x,8), POSITIVE_MOD(pt.y,8), NULL);
    }
    
    HBRUSH hbrOld = (HBRUSH)::SelectObject(hdc, hbr);
    
    ::PatBlt(
           hdc,
           rcTrack.left, rcTrack.top,
           rcTrack.Width(), rcTrack.Height(),
           PATCOPY);
    
    ::SelectObject(hdc, hbrOld);

    // Release only the non-bitmap brushes
    if (!fDither)
        ::ReleaseCachedBrush(hbr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::DrawThumb
//              
//  Synopsis:   Draw scroll bar thumb.
//              
//  Arguments:  rcThumb         bounds of thumb
//              fThumbPressed   TRUE if thumb is pressed
//              hdc             HDC to draw into
//              params          customizable scroll bar parameters
//              pDI             draw info
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::DrawThumb(
        const CRect& rcThumb,
        BOOL fThumbPressed,
        HDC hdc,
        const CScrollbarParams& params,
        CDrawInfo* pDI)
{
#ifdef _MAC
    return;
#endif

#ifdef UNIX
    {
        ScrollBarInfo info;
        info.lStyle = params._bDirection ? SB_VERT : SB_HORZ;
        info.bDisabled = FALSE;
        MwPaintMotifScrollRect( hdc,
                                ThumbButton,
                                (LPRECT)&rcThumb,
                                FALSE,
                                &info);
        return;
    }
#endif
    
    CRect rcInterior(rcThumb);

    //
    // Draw the border of the thumb
    //

    IGNORE_HR(BRDrawBorder (
        pDI, (RECT *) &rcThumb, fmBorderStyleRaised,
        0, params._pColors,
        (params._fFlat ? BRFLAGS_MONO : 0 )));

    //
    // Calculate the interior of the thumb
    //

    IGNORE_HR(BRAdjustRectForBorder(
        pDI, &rcInterior,
        (params._fFlat ? fmBorderStyleSingle : fmBorderStyleRaised )));

    //
    // Here we draw the interior border of the scrollbar thumb.
    // We assume that the edge is two pixels wide.
    //

    HBRUSH hbr = params._pColors->BrushBtnFace();
    HBRUSH hbrOld = (HBRUSH)::SelectObject(hdc, hbr);
    ::PatBlt(hdc, rcInterior.left, rcInterior.top,
             rcInterior.Width(), rcInterior.Height(), PATCOPY );
    ::SelectObject(hdc, hbrOld);
    ::ReleaseCachedBrush(hbr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::GetPart
//              
//  Synopsis:   Return the scroll bar part hit by the given test point.
//              
//  Arguments:  direction       0 for horizontal scroll bar, 1 for vertical
//              rcScrollbar     scroll bar bounds
//              ptHit           test point
//              contentSize     size of content controlled by scroll bar
//              containerSize   size of container
//              scrollAmount    current scroll amount
//              buttonWidth     width of scroll bar buttons
//              fRightToLeft    The text flow is RTL...0,0 is at top right
//              
//  Returns:    The scroll bar part hit, or SB_NONE if nothing was hit.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CScrollbar::SCROLLBARPART
CScrollbar::GetPart(
        int direction,
        const CRect& rcScrollbar,
        const CPoint& ptHit,
        long contentSize,
        long containerSize,
        long scrollAmount,
        long buttonWidth,
        CDrawInfo* pDI,
        BOOL fRightToLeft)
{
    if (!rcScrollbar.Contains(ptHit))
        return SB_NONE;
    
#ifdef _MAC
    return GetMacScrollPart(pDI, direction, ptHit);
#endif
   
    // adjust button width if there isn't room for both buttons at full size
    long scaledButtonWidth =
        GetScaledButtonWidth(direction, rcScrollbar, buttonWidth);
    
    // now test just the axis that matters
    long x = ptHit[direction];
    
    if (x < rcScrollbar.TopLeft()[direction] + scaledButtonWidth)
        return SB_PREVBUTTON;
    
    if (x >= rcScrollbar.BottomRight()[direction] - scaledButtonWidth)
        return SB_NEXTBUTTON;
    
    // NOTE: if there is no thumb, return SB_TRACK
    CRect rcThumb;
    GetPartRect(
        &rcThumb,
        SB_THUMB,
        direction,
        rcScrollbar,
        contentSize,
        containerSize,
        scrollAmount,
        buttonWidth,
        pDI,
        fRightToLeft);
    if (rcThumb.IsEmpty())
        return SB_TRACK;
     
    if (x < rcThumb.TopLeft()[direction])
        return SB_PREVTRACK;
    if (x >= rcThumb.BottomRight()[direction])
        return SB_NEXTTRACK;
    
    return SB_THUMB;
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::GetPartRect
//              
//  Synopsis:   Return the rect bounding the given scroll bar part.
//              
//  Arguments:  prcPart         returns part rect
//              part            which scroll bar part
//              direction       0 for horizontal scroll bar, 1 for vertical
//              rcScrollbar     scroll bar bounds
//              contentSize     size of content controlled by scroll bar
//              containerSize   size of container
//              scrollAmount    current scroll amount
//              buttonWidth     width of scroll bar buttons
//              fRightToLeft    The text flow is RTL...0,0 is at top right
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::GetPartRect(
        CRect* prcPart,
        CScrollbar::SCROLLBARPART part,
        int direction,
        const CRect& rcScrollbar,
        long contentSize,
        long containerSize,
        long scrollAmount,
        long buttonWidth,
        CDrawInfo* pDI,
        BOOL fRightToLeft)
{
    // adjust button width if there isn't room for both buttons at full size
    long scaledButtonWidth =
        GetScaledButtonWidth(direction, rcScrollbar, buttonWidth);
    
    switch (part)
    {
    case SB_NONE:
        AssertSz(FALSE, "CScrollbar::GetPartRect called with no part");
        prcPart->SetRectEmpty();
        break;
        
    case SB_PREVBUTTON:
        *prcPart = rcScrollbar;
        (*prcPart)[direction+2] = rcScrollbar[direction] + scaledButtonWidth;
        break;
        
    case SB_NEXTBUTTON:
        *prcPart = rcScrollbar;
        (*prcPart)[direction] = rcScrollbar[direction+2] - scaledButtonWidth;
        break;
        
    case SB_TRACK:
    case SB_PREVTRACK:
    case SB_NEXTTRACK:
    case SB_THUMB:
        {
            if (contentSize <= containerSize && part != SB_TRACK)
            {
                prcPart->SetRectEmpty();
                break;
            }
            
            *prcPart = rcScrollbar;
            (*prcPart)[direction] += scaledButtonWidth;
            (*prcPart)[direction+2] -= scaledButtonWidth;
            if (part == SB_TRACK)
                break;
            
            // calculate thumb size
            long trackSize = prcPart->Size(direction);
            long thumbSize = GetThumbSize(
                direction, rcScrollbar, contentSize, containerSize, buttonWidth, pDI);
            long thumbOffset = GetThumbOffset(
                contentSize, containerSize, scrollAmount, trackSize, thumbSize);
            
            if (part == SB_THUMB)
            {
                // We need to special case RTL HSCROLL
                if(direction == 0 && fRightToLeft)
                {
                    prcPart->right += thumbOffset;
                    prcPart->left = prcPart->right - thumbSize;
                }
                else
                {
                    (*prcPart)[direction] += thumbOffset;
                    (*prcPart)[direction+2] = (*prcPart)[direction] + thumbSize;
                }
            }
            else if (part == SB_PREVTRACK)
            {
                if(direction == 0 && fRightToLeft)
                    prcPart->right += thumbOffset - thumbSize;
                else
                    (*prcPart)[direction+2] = (*prcPart)[direction] + thumbOffset;
            }
            else
            {
                if(direction == 0 && fRightToLeft)
                    prcPart->left = prcPart->right + thumbOffset;
                else
                    (*prcPart)[direction] += thumbOffset + thumbSize;
            }
        }
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::InvalidatePart
//              
//  Synopsis:   Invalidate and immediately redraw the indicated scrollbar part.
//              
//  Arguments:  part                part to redraw
//              direction           0 for horizontal scroll bar, 1 for vertical
//              rcScrollbar         scroll bar bounds
//              contentSize         size of content controlled by scroll bar
//              containerSize       size of container
//              scrollAmount        current scroll amount
//              buttonWidth         width of scroll bar buttons
//              pDispNodeToInval    display node to invalidate
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CScrollbar::InvalidatePart(
        CScrollbar::SCROLLBARPART part,
        int direction,
        const CRect& rcScrollbar,
        long contentSize,
        long containerSize,
        long scrollAmount,
        long buttonWidth,
        CDispNode* pDispNodeToInval,
        CDrawInfo* pDI)
{
    // find bounds of part
    CRect rcPart;
    GetPartRect(
        &rcPart,
        part,
        direction,
        rcScrollbar,
        contentSize,
        containerSize,
        scrollAmount,
        buttonWidth,
        pDI,
        pDispNodeToInval->IsRightToLeft());

    pDispNodeToInval->Invalidate(rcPart, COORDSYS_CONTAINER, TRUE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CScrollbar::GetThumbSize
//              
//  Synopsis:   Calculate the thumb size given the adjusted button width.
//              
//  Arguments:  direction           0 for horizontal scroll bar, 1 for vertical
//              rcScrollbar         scroll bar bounds
//              contentSize         size of content controlled by scroll bar
//              containerSize       size of container
//              buttonWidth         width of scroll bar buttons
//              
//  Returns:    width of thumb in pixels
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

long
CScrollbar::GetThumbSize(
        int direction,
        const CRect& rcScrollbar,
        long contentSize,
        long containerSize,
        long buttonWidth,
        CDrawInfo* pDI)
{
#ifdef _MAC
    return 16; // fixed size thumb
#endif
    long thumbSize = 0;

    long trackSize = GetTrackSize(direction, rcScrollbar, buttonWidth);
    // minimum thumb size is 8 points for
    // compatibility with IE4.
    
    // [alanau] For some reason, I can't put two ternery expressions in a "max()", so it's coded this clumsy way:
    //
    if (contentSize)
    {
        thumbSize = max(
            (direction == 0
             ? pDI->WindowFromDocumentCX(HIMETRIC_PER_INCH*8/72)
             : pDI->WindowFromDocumentCY(HIMETRIC_PER_INCH*8/72)),
             trackSize * containerSize / contentSize);
    }
    else
    {
        // Avoid divide-by-zero fault.
        thumbSize = direction == 0
             ? pDI->WindowFromDocumentCX(HIMETRIC_PER_INCH*8/72)
             : pDI->WindowFromDocumentCY(HIMETRIC_PER_INCH*8/72);
    }
    return (thumbSize <= trackSize ? thumbSize : 0);
}
