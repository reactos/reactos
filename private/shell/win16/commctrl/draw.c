// ---------------------------------------------------------------------------
//
//  DRAW.C
//
//  Init Routines which are also used by Control Panel
//
//  -- Scalable Window Frame Support
//
//  exports from this module:
//   > DrawFrameControl  -- API called internally by CreateFrameControl
//
// ---------------------------------------------------------------------------

#include "ctlspriv.h"
// module stolen from user win 4.0 to get drawing functions for
// win31 comctl31.dll

#define DIM_ARROWMIN    6
#define DIM_CAPTIONMIN  8

#define DA_MAX  1
#define DA_MIN  2
#define DA_VERT 4
#define DA_HORZ 8


// WIN31 note...removed all MARLETT stuff from original draw.c
// #define MARLETT 1

// ----------------------------------------------------------------------------
//
//  DrawDiagonalLine
//
// ----------------------------------------------------------------------------
DWORD NEAR DrawDiagonalLine(HDC hdc, LPRECT lprc, int iDirection, int iThickness)
{
    RECT    rc;
    LPINT   py;
    int     cx;
    int     cy;
    int     dx;
    int     dy;
    LPINT   pc;

    if (IsRectEmpty(lprc))
        return(0L);

    CopyRect(&rc, lprc);

    //
    // We draw slopes < 1 by varying y instead of x.
    //
    --iThickness;

    // HACK HACK HACK. REMOVE THIS ONCE MARLETT IS AROUND
    cy = rc.bottom - rc.top;
    cx = rc.right - rc.left;

    if (cy != cx)
        cy -= iThickness*CYBORDER;

    if (cy >= cx)
    {
        // "slope" is >= 1, so vary x by 1
        cy /= cx;
        pc = &cy;

        cx = CXBORDER;
    }
    else
    {
        // "slope" is < 1, so vary y by 1
        cx /= cy;
        pc = &cx;

        cy = CYBORDER;
    }

    dx = cx;
    dy = iDirection * cy;

    if (*pc > 1)
        pc++;
    *pc = (*pc + iThickness) * CYBORDER;

    rc.right -= cx;
    rc.bottom -= cy;

    // For negative slopes, start from opposite side.
    if (iDirection < 0)
        py = &rc.top;
    else
        py = &rc.bottom;

    while ((rc.left <= rc.right) && (rc.top <= rc.bottom))
    {
        PatBlt(hdc, rc.left, *py, cx, cy, PATCOPY);
        
        rc.left += dx;
        *py -= dy;
    }

    return(MAKELONG(cx, cy));
}


// -------------------------------------------------------------------------
//
//  DrawDiagonal()
//
//  Called by DrawEdge() for BF_DIAGONAL edges.
//
//  Draws line of slope 1, one of 4 different ones.  The difference is
//  where the line starts and where the end point is.  The BF_ flags for
//  BF_DIAGONAL specify where the end point is.  For example, BF_DIAGONAL |
//  BF_TOP | BF_LEFT means to draw a line ending up at the top left corner.
//  So the origin must be bottom right, and the angle must be 3pi/4, or
//  135 degrees.
//
// --------------------------------------------------------------------------
void FAR DrawDiagonal(HDC hdc, LPRECT lprc, HBRUSH hbrTL, HBRUSH hbrBR, UINT flags)
{
    HBRUSH  hbrT;
    int     nDirection;
    DWORD   dAdjust;

    //
    // BOGUS
    // With a little more work, we can support arbitrary diagonals.
    // DrawDiagonalLine() just needs to figure out whether abs(slope) 
    // is >= 1 or <= 1, and vary x/y accordingly.
    //

    // Away from light source
    if (flags & BF_BOTTOM)
        hbrT = hbrBR;
    else
        hbrT = hbrTL;

    switch (flags & (BF_RECT | BF_DIAGONAL))
    {
        case BF_DIAGONAL_ENDTOPLEFT:
        case BF_DIAGONAL_ENDBOTTOMRIGHT:
            nDirection = -1;
            break;

        default:
            nDirection = 1;
            break;
    }

    hbrT = SelectBrush(hdc, hbrT);
    dAdjust = DrawDiagonalLine(hdc, lprc, nDirection, 1);
    SelectBrush(hdc, hbrT);

    // Adjust rectangle for next border

    if (flags & BF_TOP)
        lprc->left += LOWORD(dAdjust);
    else
        lprc->right -= LOWORD(dAdjust);

    if (flags & BF_RIGHT)
        lprc->top += HIWORD(dAdjust);
    else
        lprc->bottom -= HIWORD(dAdjust);
}


// ---------------------------------------------------------------------------
//
//  DrawClose
//
// ---------------------------------------------------------------------------
BOOL NEAR DrawClose(HDC hdc, LPRECT lprc, WORD wControlState)
{
    HBRUSH  hOldBrush;
    int     cx, cy;
    int     cDraw;
    int dMin;
    BOOL    fDrawDisabled = wControlState & DFCS_INACTIVE;
    
    cy = lprc->bottom - lprc->top;
    cx = lprc->right - lprc->left;

    dMin = min(cy, cx);

    if (dMin < DIM_CAPTIONMIN)
        return(FALSE);
    
    cDraw = dMin - ((((dMin - DIM_CAPTIONMIN) / 4) + 1) * 2);

    lprc->left += (cx - cDraw + 1) / 2;
    lprc->top  += (cy - cDraw + 1) / 2;
    
    if (wControlState & (DFCS_INACTIVE | DFCS_PUSHED))
    {
        lprc->left++;
        lprc->top++;
    }
    
    lprc->bottom = lprc->top + cDraw;
    lprc->right = lprc->left + cDraw;

    hOldBrush = SelectBrush(hdc, fDrawDisabled ? HBR_3DHILIGHT : HBR_BTNTEXT);
    
DrawX:
    DrawDiagonalLine(hdc, lprc, 1, 2);
    DrawDiagonalLine(hdc, lprc, -1, 2);

    if (fDrawDisabled)
    {
        SelectBrush(hdc, HBR_3DSHADOW);
        OffsetRect(lprc, -1, -1);
        fDrawDisabled = FALSE;
        goto DrawX;
    }

    SelectBrush(hdc, hOldBrush);
    return(TRUE);
}


#ifndef WIN31
// ---------------------------------------------------------------------------
//
//  DrawHelp
//
// ---------------------------------------------------------------------------
BOOL NEAR DrawHelp(HDC hdc, LPRECT lprc, WORD wControlState)
{
    HBITMAP hBmpHelp = OwnerLoadBitmap(hInstanceWin, MAKEINTRESOURCE(OBM_HELP), hInstanceWin);
    HBITMAP hBmpOld;
    int     cx, cy;
    int     cDraw;
    int     dMin;
    
    cy = lprc->bottom - lprc->top;
    cx = lprc->right - lprc->left;

    dMin = min(cy, cx);

    if (dMin < DIM_CAPTIONMIN)
        return(FALSE);
    
    cDraw = 10;

    lprc->left += (cx - cDraw + 1) / 2;
    lprc->top += (cy - cDraw + 1) / 2;
    
    if (wControlState & DFCS_PUSHED)
    {
        lprc->left++;
        lprc->top++;
    }
    
    hBmpOld = SelectObject(hdcGray, hBmpHelp);
    BltColor(hdc, HBR_BTNTEXT, hdcGray, lprc->left, lprc->top, cDraw, cDraw, 0, 0, TRUE);
    SelectObject(hdcGray, hBmpOld);
    DeleteObject(hBmpHelp);
    return(TRUE);
}
#endif

// --------------------------------------------------------------------------
//
//  DrawMiniWindow
//
// --------------------------------------------------------------------------
BOOL NEAR DrawMiniWindow(HDC hdc, int x, int y, int cWidth, int cHeight)
{
    PatBlt(hdc, x, y, cWidth + 1, 2, PATCOPY);
    if (cHeight == 2)
        return(TRUE);
    PatBlt(hdc, x, y, 1, cHeight, PATCOPY);
    PatBlt(hdc, x + cWidth, y, 1, cHeight, PATCOPY);
    PatBlt(hdc, x, y + cHeight, cWidth + 1, 1, PATCOPY);
}

// ---------------------------------------------------------------------------
//
//  DrawWindowSize
//
// ---------------------------------------------------------------------------

BOOL NEAR DrawWindowSize(HDC hdc, LPRECT lprc, WORD wControlState)
{
    HBRUSH hOldBrush;
    int cx, cy, cDraw;
    int dMin, x, y;
    BOOL    fDrawDisabled = wControlState & DFCS_INACTIVE;

    cx = lprc->right - lprc->left;
    cy = lprc->bottom - lprc->top;
    dMin = min(cx, cy);

    if (dMin < DIM_CAPTIONMIN)
        return(FALSE);

    cDraw = dMin - ((((dMin - DIM_CAPTIONMIN) / 4) + 1) * 2);

    if (((dMin & 0x0001) && !(cDraw & 0x0001)) || (!(dMin & 0x0001) && (cDraw & 0x0001)))
        cDraw++;
    
    x = lprc->left + (cx - cDraw - 1) / 2;
    y = lprc->top  + (cy - cDraw - 1) / 2;

    cx = cDraw;
    cy = cDraw;
   
    if (wControlState & DFCS_CAPTIONMIN)
    {
        cDraw /= 3;
        cx -= cDraw;
        if (wControlState & DFCS_CAPTIONMAX)
        {
            x += cDraw;
            cy -= cDraw;
        }
        else
        {
            // hack -- should be -2 instead of -1
            y += cy - 1;
            cy = 2;
        }
    }
    
    if (wControlState & DFCS_PUSHED)
    {
        x++;
        y++;
    }

    hOldBrush = SelectBrush(hdc, fDrawDisabled ? HBR_3DHILIGHT : HBR_BTNTEXT);
    
DrawSize:
    DrawMiniWindow(hdc, x, y, cx, cy);

    if ((wControlState & DFCS_CAPTIONRESTORE) == DFCS_CAPTIONRESTORE)
    {
        RECT rc;
        rc.left = x - cDraw;
        rc.right = rc.left + cx;
        rc.top = y + cDraw;
        rc.bottom = rc.top + cy;
        FillRect(hdc, &rc, HBR_3DFACE);
        DrawMiniWindow(hdc, x - cDraw, y + cDraw, cx, cy);
    }
    
    if (fDrawDisabled)
    {
        SelectBrush(hdc, HBR_3DSHADOW);
        x--;
        y--;
        fDrawDisabled = FALSE;
        goto DrawSize;
    }

    SelectBrush(hdc, hOldBrush);
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
//  DrawArrow
//
// ----------------------------------------------------------------------------
BOOL NEAR DrawArrow(HDC hdc, int x, int y, int cWidth, WORD wStyle)
{
    int i;
    int iCount = (cWidth + 1) / 2;
    int sign = (wStyle & DA_MAX) ? 1 : -1;
    int inc = 2 * sign;

    if (sign == -1)
        cWidth = (cWidth % 2) ? 1 : 2;

    if (wStyle & DA_VERT)
    {
        if (sign == -1)
            x += iCount - 1;

        for (i = 0; i < iCount; i++, cWidth -= inc, x += sign)
            PatBlt(hdc, x, y++, cWidth, 1, PATCOPY); 
    }
    else
    {
        if (sign == -1)
            y += iCount - 1;
        
        for (i = 0; i < iCount; i++, cWidth -= inc, y += sign)
            PatBlt(hdc, x++, y, 1, cWidth, PATCOPY);
    }
    return(TRUE);
}



// ---------------------------------------------------------------------------
//
//  DrawScrollArrow
//
// ---------------------------------------------------------------------------
BOOL NEAR DrawScrollArrow(HDC hdc, LPRECT lprc, WORD wControlState)
{
    HBRUSH hOldBrush;
    int dMin, x, y, cx, cy;
    int cWidth, cLength, cxArrow, cyArrow, cWidthSave;
    int iLine = 0;
    BOOL fDrawDisabled = FALSE;
    int iShift = 0;
                        
#if 0
ComputeArrowSize:
#endif
    cx = lprc->right - lprc->left;
    cy = lprc->bottom - lprc->top;

    dMin = min(cx, cy);

    if (dMin < (DIM_ARROWMIN - 3))  
        return(FALSE);
    
    cLength = (dMin <= DIM_ARROWMIN) ? 2 : ((dMin - (DIM_ARROWMIN + 1)) / 4) + 3;

    cWidth = cWidthSave = (cLength * 2) - 1;
    
    if (wControlState & DFCS_SCROLLHORZ)
    {
        cxArrow = cLength;
        cyArrow = cWidth;
    }
    else
    {
        cxArrow = cWidth;
        cyArrow = cLength;
    }
    
    x = ((cx - cxArrow + 1) / 2) + lprc->left;
    y = ((cy - cyArrow + 1) / 2) + lprc->top;

#if 0
    if (wControlState & DFCS_SCROLLLINE)
    {
        iLine = lprc->bottom - ((cy - cyArrow) / 2);
        lprc->bottom -= (cy - cyArrow + 1) / 4;
        wControlState &= ~DFCS_SCROLLLINE;
        goto ComputeArrowSize;
    }
#endif
    
    fDrawDisabled = (wControlState & DFCS_INACTIVE);
    
    if (wControlState & (DFCS_INACTIVE | DFCS_PUSHED))
        iShift++;

    hOldBrush = SelectBrush(hdc,
        fDrawDisabled ? HBR_3DHILIGHT : HBR_BTNTEXT);
        
    wControlState = ((wControlState & DFCS_SCROLLMAX)  ? DA_MAX  : DA_MIN ) |
                    ((wControlState & DFCS_SCROLLHORZ) ? DA_HORZ : DA_VERT);
    
DrawArrow:
    DrawArrow(hdc, x + iShift, y + iShift, cWidth, wControlState);
    
    if (iLine)
        PatBlt(hdc, x + iShift, iLine + iShift, cWidth, 1, PATCOPY);

    if (fDrawDisabled)
    {
        SelectBrush(hdc, fDrawDisabled ? HBR_3DSHADOW : HBR_BTNTEXT);
        iShift = 0;
        fDrawDisabled = FALSE;
        goto DrawArrow;
    }

    SelectBrush(hdc, hOldBrush);
    return(TRUE);
}


// ----------------------------------------------------------------------------
//
//  DrawCheckMark
//
// ----------------------------------------------------------------------------
BOOL NEAR DrawCheckMark(HDC hdc, LPRECT lprc, int iThickness)
{
    int i = (lprc->right - lprc->left + 2) / 3;
    int saveRight = lprc->right;

    lprc->top += ((lprc->bottom - lprc->top - (i * 2)) / 2) + i - 1;
    lprc->bottom = lprc->top + i + (iThickness - 1);

    lprc->left += (lprc->right - lprc->left - ((i * 3) - 1)) / 2;

    lprc->right = lprc->left + i - 1;
    DrawDiagonalLine(hdc, lprc, -1, iThickness);
    lprc->top -= i;
    lprc->left = lprc->right;
    lprc->right = saveRight;
    DrawDiagonalLine(hdc, lprc, 1, iThickness);
    return(TRUE);
}



// ----------------------------------------------------------------------------
//
//  DrawBullet
//
//  Our bullet has to be the same size as the checkmark.  But we leave a 
//  border on all sides since it's solid and would have too much "presence"
//  in the same amount of space as the relatively empty checkmark.
//
// ----------------------------------------------------------------------------
BOOL NEAR DrawBullet(HDC hdc, LPRECT lprc)
{
    RECT    rcT;
    int     cx, cy;

#ifndef WIN31
    CopyInflateRect(&rcT, lprc, -CXBORDER, -CYBORDER);
#else
    rcT.left    = lprc->left    - CXBORDER;
    rcT.top     = lprc->top     - CYBORDER;
    rcT.right   = lprc->right   + CXBORDER;
    rcT.bottom  = lprc->bottom  + CYBORDER;
#endif

    cx = rcT.right - rcT.left;
    cy = rcT.bottom - rcT.top;

    DrawArrow(hdc, rcT.left, rcT.top, cx, DA_MIN | DA_VERT);
    DrawArrow(hdc, rcT.left, rcT.top + (cy / 2), cx, DA_MAX | DA_VERT);

    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  DrawGrip
//
//  Draws the sizing grip bitmap, which replaces the HTSIZEBOX (the area in 
//  the lower right corner between a vertical and a horizontal scrollbar) for
//  sizeable windows.  It acts just like HTBOTTOMRIGHT.
//
// ----------------------------------------------------------------------------
BOOL NEAR DrawGrip(register HDC hdc, LPRECT lprc, WORD wState)
{
    int x, y;
    int xMax, yMax;
    int dMin;
    HBRUSH hbrOld;
    HPEN hpen, hpenOld;
    DWORD rgbHilight, rgbShadow;

    //
    // The grip is really a pattern of 4 repeating diagonal lines:
    //      One glare
    //      Two raised
    //      One empty
    // These lines run from bottom left to top right, in the bottom right
    // corner of the square given by (lprc->left, lprc->top, dMin by dMin.
    //

    dMin = min(lprc->right-lprc->left, lprc->bottom-lprc->top);
    xMax = lprc->left + dMin;
    yMax = lprc->top + dMin;
  
    //
    // Setup colors
    //
    if (wState & (DFCS_FLAT | DFCS_MONO))
    {
        hbrOld = HBR_WINDOW; 
        rgbHilight = RGB_WINDOWFRAME;
        rgbShadow = RGB_WINDOWFRAME;
    }
    else
    {
        hbrOld = HBR_3DFACE;
        rgbHilight = RGB_3DHILIGHT;
        rgbShadow = RGB_3DSHADOW;
    }

    //
    // Fill in background of ENTIRE rect
    //

    hbrOld = SelectBrush(hdc, hbrOld);
    PatBlt(hdc, lprc->left, lprc->top, lprc->right-lprc->left,
            lprc->bottom-lprc->top, PATCOPY);
    SelectBrush(hdc, hbrOld);

    //
    // Draw glare with COLOR_3DHILIGHT:
    //      Create proper pen
    //      Select into hdc
    //      Starting at lprc->left, draw a diagonal line then skip the
    //          next 3
    //      Select out of hdc
    //

    hpen = CreatePen(PS_SOLID, 1, rgbHilight);
    if (hpen == NULL)
        return FALSE;
    hpenOld = SelectPen(hdc, hpen);
 
    x = lprc->left;
    y = lprc->top;
    while (x < xMax)
    {
        //
        // Since dMin is the same horz and vert, x < xMax and y < yMax
        // are interchangeable...
        //

        //
        // Draw the diagonal line.  Since lines do NOT include the endpoint,
        // we have to draw it one past the endpoint.
        //

        MoveTo(hdc, x, yMax);
        LineTo(hdc, xMax+1, y-1);

        // Skip 3 lines in between
        x += 4;
        y += 4;
    }

    SelectPen(hdc, hpenOld);
    DeletePen(hpen);

    //
    // Draw raised part with COLOR_3DSHADOW:
    //      Create proper pen
    //      Select into hdc
    //      Starting at lprc->left+1, draw 2 diagonal lines, then skip
    //          the next 2
    //      Select outof hdc
    //

    hpen = CreatePen(PS_SOLID, 1, rgbShadow);
    if (hpen == NULL)
        return FALSE;
    hpenOld = SelectPen(hdc, hpen);

    x = lprc->left+1;
    y = lprc->top+1;
    while (x < xMax)
    {
        //
        // Draw two diagonal lines touching each other.  Again, we need
        // to draw past the endpoint since LineTo() is [), including the
        // start point but not the endpoint.
        //

        MoveTo(hdc, x, yMax);
        LineTo(hdc, xMax+1, y-1);

        x++;
        y++;

        MoveTo(hdc, x, yMax);
        LineTo(hdc, xMax+1, y-1);

        //
        // Skip 2 lines inbetween
        //
        x += 3;
        y += 3;
    }

    SelectPen(hdc, hpenOld);
    DeletePen(hpen);

    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  DrawBox
//
// ----------------------------------------------------------------------------
BOOL NEAR DrawBox(HDC hdc, LPRECT lprc, WORD wControlState)
{
    HBRUSH hOldBrush;
    
    int x = lprc->left;
    int y = lprc->top;
    int cx = lprc->right  - lprc->left;
    int cy = lprc->bottom - lprc->top;
    RECT rc;
    int cxRect = CXBORDER * ((wControlState & DFCS_PUSHED) ? 2 : 1);
    int cyRect = CXBORDER * ((wControlState & DFCS_PUSHED) ? 2 : 1);
    
    CopyRect(&rc, lprc);
    if (wControlState & DFCS_BUTTONRADIO)
    {
        HDC     hdcCompat;
        HBITMAP hbmpCompat;
        DWORD   rgbBk, rgbTx;

        //
        // Create one big mono bitmap, twice the width.  Then
        // DrawFrameControl into each the right way, and blt the darned 
        // things into the destination the right way.
        //
        wControlState &= ~DFCS_BUTTONRADIO;

        hdcCompat = CreateCompatibleDC(hdc);
        hbmpCompat = CreateCompatibleBitmap(hdc, 2*cx, cy);
        hbmpCompat = SelectBitmap(hdcCompat, hbmpCompat);
        PatBlt(hdcCompat, 0, 0, 2*cx, cy, WHITENESS);

        // Get the mask & image
        OffsetRect(&rc, -x, -y);
        DrawFrameControl(hdcCompat, &rc, DFC_BUTTON, wControlState | DFCS_BUTTONRADIOMASK);
        OffsetRect(&rc, cx, 0);
        DrawFrameControl(hdcCompat, &rc, DFC_BUTTON, wControlState | DFCS_BUTTONRADIOIMAGE);

        // Blt them right
        rgbBk = SetBkColor(hdc, 0x00FFFFFFL);
        rgbTx = SetTextColor(hdc, 0L);

        BitBlt(hdc, x, y, cx, cy, hdcCompat, 0, 0, SRCAND);
        BitBlt(hdc, x, y, cx, cy, hdcCompat, cx, 0, SRCINVERT);

        SetTextColor(hdc, rgbTx);
        SetBkColor(hdc, rgbBk);

        hbmpCompat = SelectBitmap(hdcCompat, hbmpCompat);
        DeleteBitmap(hbmpCompat);
        DeleteDC(hdcCompat);
    }
    else if (wControlState & DFCS_BUTTONRADIOMASK)
    {
        FillRect(hdc, lprc, hbrWhite);
        hOldBrush = SelectBrush(hdc, hbrBlack);

        DrawArrow(hdc, rc.left,            rc.top, cy, DA_MIN | DA_HORZ);
        DrawArrow(hdc, rc.left + (cx / 2), rc.top, cy, DA_MAX | DA_HORZ);

        SelectBrush(hdc, hOldBrush);
    }
    else if (wControlState & DFCS_BUTTONRADIOIMAGE)
    {
        HBRUSH  hbrShadow, hbrHilight, hbrDkShadow, hbrLight;

        FillRect(hdc, lprc, hbrBlack);

        if (wControlState & (DFCS_MONO | DFCS_FLAT))
        {
            hbrShadow = hbrHilight = hbrDkShadow = hbrLight = HBR_WINDOWFRAME;
        }
        else
        {
            hbrShadow = HBR_3DSHADOW;
            hbrLight = HBR_3DLIGHT;
            hbrHilight = HBR_3DHILIGHT;
            hbrDkShadow = HBR_3DDKSHADOW;
        }

        // use the same hbr's as DrawBorder in wmsyserr.c does
        hOldBrush = SelectBrush(hdc, hbrShadow);
        DrawArrow(hdc, rc.left,            rc.top, cy, DA_MIN | DA_HORZ);

        SelectBrush(hdc, hbrHilight);
        DrawArrow(hdc, rc.left + (cx / 2), rc.top, cy, DA_MAX | DA_HORZ);

        InflateRect(&rc, -CXBORDER, -CYBORDER);
        cx = rc.right - rc.left;
        cy = rc.bottom - rc.top;
        SelectBrush(hdc, hbrDkShadow);
        DrawArrow(hdc, rc.left,            rc.top, cy, DA_MIN | DA_HORZ);
        SelectBrush(hdc, hbrLight);
        DrawArrow(hdc, rc.left + (cx / 2), rc.top, cy, DA_MAX | DA_HORZ);

        InflateRect(&rc, -CXBORDER, -CYBORDER);
        cx = rc.right - rc.left;
        cy = rc.bottom - rc.top;
        SelectBrush(hdc, (wControlState & DFCS_PUSHED) ? HBR_3DFACE : HBR_WINDOW);
        DrawArrow(hdc, rc.left,            rc.top, cy, DA_MIN | DA_HORZ);
        DrawArrow(hdc, rc.left + (cx / 2), rc.top, cy, DA_MAX | DA_HORZ);

        if (wControlState & DFCS_CHECKED)
        {
            SelectBrush(hdc, HBR_WINDOWTEXT);

            InflateRect(&rc, -CXBORDER, -CYBORDER);
            DrawBullet(hdc, &rc);
        }
        SelectBrush(hdc, hOldBrush);
    }
    else
    {
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST |
            (wControlState & (DFCS_MONO | DFCS_FLAT)));

        FillRect(hdc, &rc, (wControlState & DFCS_PUSHED) ? HBR_3DFACE : HBR_WINDOW);
        InflateRect(&rc, -CXBORDER, -CYBORDER);

        if (wControlState & DFCS_CHECKED)
        {
            if (wControlState & DFCS_BUTTON3STATE)
                FillRect(hdc, &rc, HBR_GRAYTEXT);
            else
            {
                hOldBrush = SelectBrush(hdc, HBR_WINDOWTEXT);
                DrawCheckMark(hdc, &rc, 3);
                SelectBrush(hdc, hOldBrush);
            }
        }
    }

    return(TRUE);
}


// ----------------------------------------------------------------------------
//
//  DrawMenuMark
//
// ----------------------------------------------------------------------------
BOOL NEAR DrawMenuMark(HDC hdc, LPRECT lprc, WORD wState)
{
    HBRUSH hOldBrush;
    int iFactor;
    
    FillRect(hdc, lprc, hbrWhite);
    
    //
    // This function draws in monochrome.  What it draws is later turned
    // into color by combining with COLOR_MENUTEXT.
    //
    hOldBrush = SelectBrush(hdc, hbrBlack);

    iFactor = (lprc->bottom - lprc->top) / -5;

    InflateRect(lprc, CXBORDER * iFactor, CYBORDER * iFactor);
    if (wState & DFCS_MENUCHECK)
        DrawCheckMark(hdc, lprc, 2);
    else if (wState & DFCS_MENUBULLET)
        DrawBullet(hdc, lprc);
    else
        DrawArrow(hdc, lprc->left+CXEDGE, lprc->top, lprc->bottom - lprc->top, DA_MAX | DA_HORZ);

    SelectBrush(hdc, hOldBrush);

    return(TRUE);
}

// ----------------------------------------------------------------------------
//
//  IDrawFrameControl
//
// ----------------------------------------------------------------------------
BOOL API DrawFrameControl(HDC hdc, LPRECT lprc, UINT wType, UINT wState)
{
    RECT    rc;
    BOOL    fRet = TRUE;
    BOOL    fButton = FALSE;
     
    CopyRect(&rc, lprc);

    // Enforce monochrome/flat
#ifndef WIN31
    if (oemInfo.BitCount == 1)
#else
    if (g_oemInfo_BitCount == 1)
#endif // !WIN31
        wState |= DFCS_MONO;
    if (wState & DFCS_MONO)
        wState |= DFCS_FLAT;
    
    if ((wType != DFC_MENU) &&
        ((wType != DFC_BUTTON) || (wState & DFCS_BUTTONPUSH)) &&
        ((wType != DFC_SCROLL) || !(wState & DFCS_SCROLLSIZEGRIP)))
    {
        WORD wBorder = BF_ADJUST;

        if (wType != DFC_SCROLL)
            wBorder |= BF_SOFT;

        Assert(DFCS_FLAT == BF_FLAT);
        Assert(DFCS_MONO == BF_MONO);

        wBorder |= (wState & (DFCS_FLAT | DFCS_MONO));

        DrawPushButton(hdc, &rc, wState, wBorder);

        if (wState & DFCS_ADJUSTRECT)
            CopyRect(lprc, &rc);
    
        fButton = TRUE;
    }
        
    if (!fButton)
    {
        if (wType == DFC_MENU)
            DrawMenuMark(hdc, &rc, wState);
        else if (wType == DFC_BUTTON)
            DrawBox(hdc, &rc, wState);
        else // wType == DFC_SCROLL
            DrawGrip(hdc, lprc, wState);
    }
#ifndef WIN31
    else if (wType == DFC_CAPTION)
    {
        if (wState & DFCS_CAPTIONRESTORE)
            DrawWindowSize(hdc, &rc, wState);
        else if (wState & DFCS_CAPTIONHELP)
            DrawHelp(hdc, &rc, wState);
        else
            DrawClose(hdc, &rc, wState);
    }
#endif // !WIN31
    else if (wType == DFC_SCROLL)
        DrawScrollArrow(hdc, &rc, wState);
    else if (wType != DFC_BUTTON)
        fRet = FALSE;
    
    return(fRet);
}

