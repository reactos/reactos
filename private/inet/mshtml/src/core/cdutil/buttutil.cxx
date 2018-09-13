//+------------------------------------------------------------------------
//
//  File:       dbbase.cxx
//
//  Contents:   Button Drawing routines common to scrollbar and dropbutton.
//
//  History:    15-Aug-95   t-vuil  Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_BUTTUTIL_HXX_
#define X_BUTTUTIL_HXX_
#include "buttutil.hxx"
#endif

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

#ifdef UNIX
#ifndef X_UNIXCTLS_HXX_
#define X_UNIXCTLS_HXX_
#include "unixctls.hxx"
#endif
#endif
//+------------------------------------------------------------------------
//
//  Function:   DrawRect
//
//  Synopsis:   Draws a rectangle in the current BG color
//
//  Notes:      Uses ExtTextOut because it has generally been highly
//              optimized for text rendering
//
//-------------------------------------------------------------------------

void
DrawRect (
    HDC hdc, HBRUSH hbr, int x1, int y1, int x2, int y2,
    BOOL fReleaseBrush = TRUE )
{
    if (x2 > x1 && y2 > y1)
    {
        HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, hbr);
        PatBlt( hdc, x1, y1, x2 - x1, y2 - y1, PATCOPY );
        SelectObject(hdc, hbrOld);
    }

    if (fReleaseBrush)
        ReleaseCachedBrush( hbr );
}

void
CUtilityButton::Invalidate(HWND hWnd, const RECT &rc, DWORD dwFlags)
{
    if (hWnd)
    {
        RedrawWindow(hWnd, &rc, NULL, RDW_INVALIDATE);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     DrawNull
//
//  Synopsis:   Draws nothing
//
//-------------------------------------------------------------------------

void
CUtilityButton::DrawNull (
    HDC hdc, HBRUSH, BUTTON_GLYPH glyph,
    const RECT & rcBounds, const SIZEL & sizel )
{
}

//+------------------------------------------------------------------------
//
//  Member:     DrawDotDotDot
//
//  Synopsis:   Draws the Ellipses Glyph (dotdotdot) (used in dropbutton)
//
//-------------------------------------------------------------------------

void
CUtilityButton::DrawDotDotDot (
    HDC hdc, HBRUSH hbr, BUTTON_GLYPH glyph,
    const RECT & rcBounds, const SIZEL & sizel )
{
    long xStart, yStart, width, height;
    
    Assert( rcBounds.right  > rcBounds.left );
    Assert( rcBounds.bottom > rcBounds.top );

        //
        // 106 is about 3 points, which is how far from the bottom
        // the dots starts.
        //
        // 53 is about 1.5 points, which the height/width of the dots
        //

    yStart = rcBounds.bottom -
        MulDiv( rcBounds.bottom - rcBounds.top, 106, sizel.cy );
    
    height = MulDiv( rcBounds.bottom - rcBounds.top, 53, sizel.cy );
    width  = MulDiv( rcBounds.right - rcBounds.left, 54, sizel.cx );

    xStart = rcBounds.left + (rcBounds.right - rcBounds.left) * 2 / 13;
    DrawRect(hdc, hbr, xStart, yStart, xStart + width, yStart + height, FALSE);
    
    xStart = rcBounds.left + (rcBounds.right - rcBounds.left) * 5 / 13;
    DrawRect(hdc, hbr, xStart, yStart, xStart + width, yStart + height, FALSE);
    
    xStart = rcBounds.left + (rcBounds.right - rcBounds.left) * 8 / 13;
    DrawRect(hdc, hbr, xStart, yStart, xStart + width, yStart + height, FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     DrawReduce
//
//  Synopsis:   Draws the under bar
//
//-------------------------------------------------------------------------

void
CUtilityButton::DrawReduce (
    HDC hdc, HBRUSH hbr, BUTTON_GLYPH glyph,
    const RECT & rcBounds, const SIZEL & sizel )
{
    long xStart, yStart, width, height;
    
    Assert( rcBounds.right  > rcBounds.left );
    Assert( rcBounds.bottom > rcBounds.top );

        //
        // 106 is about 3 points, which is how far from the bottom
        // the bar starts.
        //
        // 53 is about 1.5 points, which the height of the bar
        //

    yStart = rcBounds.bottom -
        MulDiv( rcBounds.bottom - rcBounds.top, 108, sizel.cy );
    
    height = MulDiv( rcBounds.bottom - rcBounds.top, 54, sizel.cy );
    
    xStart = rcBounds.left + (rcBounds.right - rcBounds.left) * 3 / 13;
    width  = (rcBounds.right - rcBounds.left) * 6 / 13;

    DrawRect(hdc, hbr, xStart, yStart, xStart + width, yStart + height, FALSE);
}

//+------------------------------------------------------------------------
//
//  Function:   DrawArrow
//
//  Synopsis:   Draws a scrollbar type arrow
//
//-------------------------------------------------------------------------

// BUGBUG: Use PolyLine when the arrow gets big.  This speeds up drawing,
//         and lets printing dither

void
CUtilityButton::DrawArrow (
    HDC hdc, HBRUSH hbr, BUTTON_GLYPH dir,
    const RECT & rcBounds, const SIZEL & sizel )
{
    long i;

    Assert( rcBounds.right  > rcBounds.left );
    Assert( rcBounds.bottom > rcBounds.top );
    
    Assert(
        dir == BG_UP || dir == BG_DOWN || dir == BG_LEFT || dir == BG_RIGHT );

    if (dir == BG_UP || dir == BG_DOWN)
    {
            //
            // Determine the height of the arrow by computing the largest
            // arrows we allow for the given width and height, and then taking
            // the smaller of the two.  Also make sure it is non zero.
            //

        long arrow_height =
            max(
                (long)1,
                (long)min(
                    (rcBounds.bottom - rcBounds.top + 2) / 3,
                    ((rcBounds.right - rcBounds.left) * 5 / 8 + 1) / 2 ) );
        
            //
            // Locate where the top of the arrow starts and where it is
            // centered horizontally
            //
        
        long sy =
            rcBounds.top +
                (rcBounds.bottom - rcBounds.top + 1 - arrow_height) / 2;

        long cx =
            rcBounds.left + (rcBounds.right - rcBounds.left - 1) / 2;

            //
            // Draw the arrow from top to bottom in successive strips
            //
        
        for ( i = 0 ; i < arrow_height ; i++ )
        {
            long y = dir == BG_UP ? sy + i : sy + arrow_height - i - 1;

            DrawRect( hdc, hbr, cx - i, y, cx - i + 1 + i * 2, y + 1, FALSE );
        }
    }
    else    
    {
            //
            // Determine the width of the arrow by computing the largest
            // arrows we allow for the given width and height, and then taking
            // the smaller of the two.  Also make sure it iz non zero.
            //
        
        long arrow_width =
            max(
                (long)1,
                (long)min(
                    (rcBounds.right - rcBounds.left + 2) / 3,
                    ((rcBounds.bottom - rcBounds.top) * 5 / 8 + 1) / 2 ) );
        
            //
            // Locate where the left of the arrow starts and where it is
            // centered vertically
            //
        
        long sx =
            rcBounds.left +
                (rcBounds.right - rcBounds.left + 1 - arrow_width) / 2;
        
        long cy =
            rcBounds.top + (rcBounds.bottom - rcBounds.top) / 2;

            //
            // Draw the arrow from top to bottom in successive strips
            //

        for ( long i = 0 ; i < arrow_width ; i++ )
        {
            long x = dir == BG_LEFT ? sx + i : sx + arrow_width - i - 1;
            
            DrawRect( hdc, hbr, x, cy - i, x + 1, cy + i + 1, FALSE );
        }
    }
}

//+------------------------------------------------------------------------
//
//  Function:   DrawButton
//
//  Synopsis:   Draws a scrollbar type pushbutton
//
//-------------------------------------------------------------------------


void
CUtilityButton::DrawButton (
    CDrawInfo * pDI, HWND hWnd, BUTTON_GLYPH glyph,
    BOOL fPressed, BOOL fEnabled, BOOL focused,
    const RECT & rcBounds, const SIZEL & sizelExtent,
    unsigned long padding )
{
    if (hWnd)
    {
        Invalidate(hWnd, rcBounds);
    }
    else
    {
        void (CUtilityButton ::* pmfDraw) (
                                           HDC, HBRUSH, BUTTON_GLYPH, const RECT &, const SIZEL & );

        ThreeDColors &  colors = GetColors();
        RECT            rcGlyph = rcBounds;
        SIZEL           sizelGlyph = sizelExtent;
        long            dx = 0, dy = 0, xOffset = 0, yOffset = 0;


        // must have at least hdc
        Assert(pDI->_hdc);
        // should come in initialized
        Assert(pDI->_sizeSrc.cx); 


        //
        // First, draw the border around the glyph and the background.
        //

        if (fPressed && !_fFlat)
        {
            RECT rcFill = rcBounds;

            //
            // Draw a "single line" border then reduce rect by that size and
            // Fill the rest with the button background
            //
#ifdef UNIX // If it's a motif button, see below #ifdef UNIX codes
            if (!_bMotifScrollBarBtn)
            {
#endif
            IGNORE_HR(BRDrawBorder(
                                   pDI, 
                                   LPRECT(& rcBounds),
                                   fmBorderStyleSingle, 
                                   colors.BtnShadow(), 
                                   0, 
                                   0));

            IGNORE_HR(BRAdjustRectForBorder(
                                            pDI, 
                                            & rcFill, 
                                            fmBorderStyleSingle));

            DrawRect(
                     pDI->_hdc, colors.BrushBtnFace(),
                     rcFill.left, rcFill.top, rcFill.right, rcFill.bottom );

#ifdef UNIX // Draw motif button background
            }
            else
            {
                ScrollBarInfo info;
                info.lStyle=(glyph==BG_LEFT || glyph==BG_RIGHT)?SB_HORZ:SB_VERT;
                info.bDisabled = FALSE;
                switch (glyph)
                {
                    case BG_LEFT:
                        rcFill.right+=1; 
                        break;
                    case BG_RIGHT:
                        rcFill.left-=1;
                        break;
                    case BG_UP:
                        rcFill.bottom+=1;
                        break;
                    case BG_DOWN:
                        rcFill.top-=1;
                        break;
                }
                MwPaintMotifScrollRect( pDI->_hdc,
                                    (glyph==BG_LEFT || glyph==BG_UP) ?
                                    LeftTopThumbRect : RightBottomThumbRect,
                                    &rcFill,
                                    FALSE,
                                    &info);
                switch (glyph)
                {
                    case BG_LEFT:
                        rcFill.right-=1; 
                        break;
                    case BG_RIGHT:
                        rcFill.left+=1;
                        break;
                    case BG_UP:
                        rcFill.bottom-=1;
                        break;
                    case BG_DOWN:
                        rcFill.top+=1;
                        break;
                }
            }
#endif
            //
            // Now, compute the reduced rect as if a sunken border were drawn.
            // This leaves the glyph rect in rcGlyph
            //

#ifdef UNIX
            if (!_bMotifScrollBarBtn)
#endif
            IGNORE_HR(BRAdjustRectForBorder(
                                            pDI,
                                            & rcGlyph,
                                            fmBorderStyleRaised));
        }
        else
        {
#ifdef UNIX
            if (!_bMotifScrollBarBtn)
            {
#endif
            //
            // Draw the sunken border and fill the rest with the button bg.
            // This leaves the arrow rect in rcGlyph

            // If we come here with fPressed = TRUE, this must be a flat scrollbar

            IGNORE_HR( BRDrawBorder(
                                    pDI, LPRECT(& rcBounds), 
                                    (fPressed) ? fmBorderStyleSunken : fmBorderStyleRaised,
                                    0, 
                                    & colors, 
                                    (_fFlat) ? BRFLAGS_MONO : 0 ) );

            IGNORE_HR( BRAdjustRectForBorder(
                                             pDI, 
                                             & rcGlyph, 
                                             (_fFlat) ? fmBorderStyleSingle : fmBorderStyleRaised));

            DrawRect(
                     pDI->_hdc, colors.BrushBtnFace(),
                     rcGlyph.left, rcGlyph.top, rcGlyph.right, rcGlyph.bottom );
#ifdef UNIX
            }
            else
            {
                ScrollBarInfo info;
                info.lStyle=(glyph==BG_LEFT || glyph==BG_RIGHT)?SB_HORZ:SB_VERT;
                info.bDisabled = FALSE;
                switch (glyph)
                {
                    case BG_LEFT:
                        rcGlyph.right+=1; 
                        break;
                    case BG_RIGHT:
                        rcGlyph.left-=1;
                        break;
                    case BG_UP:
                        rcGlyph.bottom+=1;
                        break;
                    case BG_DOWN:
                        rcGlyph.top-=1;
                        break;
                }
                MwPaintMotifScrollRect( pDI->_hdc,
                                    (glyph==BG_LEFT || glyph==BG_UP) ?
                                    LeftTopThumbRect : RightBottomThumbRect,
                                    &rcGlyph,
                                    FALSE,
                                    &info);
                switch (glyph)
                {
                    case BG_LEFT:
                        rcGlyph.right-=1; 
                        break;
                    case BG_RIGHT:
                        rcGlyph.left+=1;
                        break;
                    case BG_UP:
                        rcGlyph.bottom-=1;
                        break;
                    case BG_DOWN:
                        rcGlyph.top+=1;
                        break;
                }
            }
#endif
        }

        //
        // See if we have a null rect.
        //

        if (rcGlyph.right <= rcGlyph.left || rcGlyph.bottom <= rcGlyph.top)
            goto Cleanup;

        //
        // Adjust the extent to reflect the border
        //

        BRAdjustSizelForBorder(
                               & sizelGlyph, fmBorderStyleSunken );

        //
        // A combo glyph looks like a down arrow
        //

        if (glyph == BG_COMBO)
            glyph = BG_DOWN;

        //
        // Select the draw member.  Default to arrow.
        //

        pmfDraw = & CUtilityButton::DrawArrow;

        switch ( glyph )
        {
            case BG_PLAIN :
                pmfDraw = & CUtilityButton::DrawNull;
                break;

                //
                // Adjust the rect for padding.
                //
                // BUGBUG: the padding has not been zoomed....
                //

            case BG_DOWN  : rcGlyph.bottom -= padding; break;
            case BG_UP    : rcGlyph.top    += padding; break;
            case BG_LEFT  : rcGlyph.left   += padding; break;
            case BG_RIGHT : rcGlyph.right  -= padding; break;

            case BG_REFEDIT:
                pmfDraw = & CUtilityButton::DrawDotDotDot;
                break;

            case BG_REDUCE:
                pmfDraw = & CUtilityButton::DrawReduce;
                break;

            default:
                Assert( 0 && "Unknown glyph" );
                goto Cleanup;
        }

        // In the rest of the code, if the scrollbar is flat, ignore
        // fPressed (no offset even if the button is pressed)
        if (_fFlat)
            fPressed = FALSE;

        //
        // Now that the glyph draw member has been selected, use it to draw.
        //
        //
        // If we are pressed or disabled, we need to compute the offset
        // for pressing or the highlight offset version of the glyph.
        //
        // Also, if we have to draw a focus, we need to know how much to 
        // offset the focus rect from the border.
        //

        if (!fEnabled || fPressed || focused)
        {

            //
            // The offset is 27 HIMETRICS (~ 3/4 points).
            //


            xOffset = MulDiv( rcGlyph.right - rcGlyph.left, 27, sizelGlyph.cx );
            yOffset = MulDiv( rcGlyph.bottom - rcGlyph.top, 27, sizelGlyph.cy );

            if (!fEnabled || fPressed)
            {
                dx = xOffset;
                dy = yOffset;
            }
        }

        if (fEnabled)
        {
            RECT rc = rcGlyph;
            HBRUSH hbr = colors.BrushBtnText();

            if (fPressed)
            {
                rc.left   += dx;
                rc.right  += dx;
                rc.top    += dy;
                rc.bottom += dy;
            }

#ifdef UNIX
            if (_bMotifScrollBarBtn)
            {
                ScrollBarInfo info;
                info.lStyle=(glyph==BG_LEFT || glyph==BG_RIGHT)?SB_HORZ:SB_VERT;
                info.bDisabled = FALSE;
                MwPaintMotifScrollRect( pDI->_hdc,
                                    (glyph==BG_RIGHT || glyph==BG_DOWN) ?
                                    RightBottomButton : LeftTopButton,
                                    &rc,
                                    fPressed,
                                    &info);
            }
            else 
#endif                           
            CALL_METHOD(this,pmfDraw,( pDI->_hdc, hbr, glyph, rc, sizelGlyph ));

            ReleaseCachedBrush( hbr );
        }
        else
        {
            HBRUSH hbr;

            //
            // Here we draw the disabled glyph.  First, draw the lighter
            // back ground version, the the darker foreground version.
            //

            RECT rcBack = {
                rcGlyph.left + dx, rcGlyph.top + dy,
                rcGlyph.right + dx, rcGlyph.bottom + dy };

#ifdef UNIX
            if (_bMotifScrollBarBtn)
            {
                ScrollBarInfo info;
                info.lStyle=(glyph==BG_LEFT || glyph==BG_RIGHT)?SB_HORZ:SB_VERT;
                info.bDisabled = TRUE;
                MwPaintMotifScrollRect( pDI->_hdc,
                                    (glyph==BG_DOWN || glyph==BG_RIGHT) ?
                                    RightBottomButton : LeftTopButton,
                                    &rcGlyph,
                                    FALSE, //fPressed,
                                    &info);
            }
            else 
            {
#endif                           
                hbr = colors.BrushBtnHighLight();
                CALL_METHOD(this,pmfDraw,( pDI->_hdc, hbr, glyph, rcBack, sizelGlyph ));
                ReleaseCachedBrush( hbr );

                hbr = colors.BrushBtnShadow();
                CALL_METHOD(this,pmfDraw,( pDI->_hdc, hbr, glyph, rcGlyph, sizelGlyph ));
                ReleaseCachedBrush( hbr );
#ifdef UNIX
            }
#endif
        }

        //
        // Draw any focus rect
        //

        // BUGBUG: This relies heavily on the assumption that the arrow which is
        //         drawn leaves alot of space around the edge so that the focus
        //         can be drawn there.  Also, the focus rect is not zoomed here.

        if (focused)
        {
#ifdef WIN16
            GDIRECT rcFocus = {rcGlyph.left, rcGlyph.top, rcGlyph.right, rcGlyph.bottom};
#else
            GDIRECT rcFocus = rcGlyph;
#endif

            rcFocus.left   += xOffset + dx;
            rcFocus.right  -= xOffset - dx;
            rcFocus.top    += yOffset + dy;
            rcFocus.bottom -= yOffset - dy;

            rcFocus.right  = max( rcFocus.right, rcFocus.right );
            rcFocus.bottom = max( rcFocus.bottom, rcFocus.top );

            if (rcFocus.right <= rcGlyph.right && rcFocus.bottom <= rcGlyph.bottom)
            {
                ::DrawFocusRect( pDI->_hdc, & rcFocus );
            }
        }
    }
Cleanup:

    return;
}

//+------------------------------------------------------------------------
//
//  Function:   GetColors
//
//-------------------------------------------------------------------------

ThreeDColors &
CUtilityButton::GetColors ( void )
{
    static ThreeDColors colorsDefault( DEFAULT_BASE_COLOR );
    return colorsDefault;
}

