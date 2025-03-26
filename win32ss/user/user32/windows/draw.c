/*
 * ReactOS User32 Library
 * - Various drawing functions
 *
 * Copyright 2001 Casper S. Hournstroup
 * Copyright 2003 Andrew Greenwood
 * Copyright 2003 Filip Navara
 * Copyright 2009 Matthias Kupfer
 * Copyright 2017-2022 Katayama Hirofumi MZ
 *
 * Based on Wine code.
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 2002 Bill Medland
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* GLOBALS *******************************************************************/

/* These tables are used in:
 * UITOOLS_DrawDiagEdge()
 * UITOOLS_DrawRectEdge()
 */
static const signed char LTInnerNormal[] = {
            -1,           -1,                 -1,                 -1,
            -1,           COLOR_BTNHIGHLIGHT, COLOR_BTNHIGHLIGHT, -1,
            -1,           COLOR_3DDKSHADOW,   COLOR_3DDKSHADOW,   -1,
            -1,           -1,                 -1,                 -1
};

static const signed char LTOuterNormal[] = {
            -1,                 COLOR_3DLIGHT,     COLOR_BTNSHADOW, -1,
            COLOR_BTNHIGHLIGHT, COLOR_3DLIGHT,     COLOR_BTNSHADOW, -1,
            COLOR_3DDKSHADOW,   COLOR_3DLIGHT,     COLOR_BTNSHADOW, -1,
            -1,                 COLOR_3DLIGHT,     COLOR_BTNSHADOW, -1
};

static const signed char RBInnerNormal[] = {
            -1,           -1,                -1,              -1,
            -1,           COLOR_BTNSHADOW,   COLOR_BTNSHADOW, -1,
            -1,           COLOR_3DLIGHT,     COLOR_3DLIGHT,   -1,
            -1,           -1,                -1,              -1
};

static const signed char RBOuterNormal[] = {
            -1,              COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
            COLOR_BTNSHADOW, COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
            COLOR_3DLIGHT,   COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
            -1,              COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1
};

static const signed char LTInnerSoft[] = {
            -1,                  -1,                -1,              -1,
            -1,                  COLOR_3DLIGHT,     COLOR_3DLIGHT,   -1,
            -1,                  COLOR_BTNSHADOW,   COLOR_BTNSHADOW, -1,
            -1,                  -1,                -1,              -1
};

static const signed char LTOuterSoft[] = {
            -1,              COLOR_BTNHIGHLIGHT, COLOR_3DDKSHADOW, -1,
            COLOR_3DLIGHT,   COLOR_BTNHIGHLIGHT, COLOR_3DDKSHADOW, -1,
            COLOR_BTNSHADOW, COLOR_BTNHIGHLIGHT, COLOR_3DDKSHADOW, -1,
            -1,              COLOR_BTNHIGHLIGHT, COLOR_3DDKSHADOW, -1
};

#define RBInnerSoft RBInnerNormal   /* These are the same */
#define RBOuterSoft RBOuterNormal

static const signed char LTRBOuterMono[] = {
            -1,           COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
            COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
            COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
            COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME, COLOR_WINDOWFRAME,
};

static const signed char LTRBInnerMono[] = {
            -1, -1,           -1,           -1,
            -1, COLOR_WINDOW, COLOR_WINDOW, COLOR_WINDOW,
            -1, COLOR_WINDOW, COLOR_WINDOW, COLOR_WINDOW,
            -1, COLOR_WINDOW, COLOR_WINDOW, COLOR_WINDOW,
};

static const signed char LTRBOuterFlat[] = {
            -1,                COLOR_BTNSHADOW, COLOR_BTNSHADOW, COLOR_BTNSHADOW,
            COLOR_BTNFACE,     COLOR_BTNSHADOW, COLOR_BTNSHADOW, COLOR_BTNSHADOW,
            COLOR_BTNFACE,     COLOR_BTNSHADOW, COLOR_BTNSHADOW, COLOR_BTNSHADOW,
            COLOR_BTNFACE,     COLOR_BTNSHADOW, COLOR_BTNSHADOW, COLOR_BTNSHADOW,
};

static const signed char LTRBInnerFlat[] = {
            -1, -1,              -1,              -1,
            -1, COLOR_BTNFACE,     COLOR_BTNFACE,     COLOR_BTNFACE,
            -1, COLOR_BTNFACE,     COLOR_BTNFACE,     COLOR_BTNFACE,
            -1, COLOR_BTNFACE,     COLOR_BTNFACE,     COLOR_BTNFACE,
};
/* FUNCTIONS *****************************************************************/


HBRUSH WINAPI GetSysColorBrush(int nIndex);

/* Ported from WINE20020904 */
/* Same as DrawEdge invoked with BF_DIAGONAL */
static BOOL IntDrawDiagEdge(HDC hdc, LPRECT rc, UINT uType, UINT uFlags)
{
    POINT Points[4];
    signed char InnerI, OuterI;
    HPEN InnerPen, OuterPen;
    POINT SavePoint;
    HPEN SavePen;
    int spx, spy;
    int epx, epy;
    int Width = rc->right - rc->left;
    int Height= rc->bottom - rc->top;
    int SmallDiam = Width > Height ? Height : Width;
    BOOL retval = !(   ((uType & BDR_INNER) == BDR_INNER
                        || (uType & BDR_OUTER) == BDR_OUTER)
                       && !(uFlags & (BF_FLAT|BF_MONO)) );
    int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0)
            + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0);

    /* Init some vars */
    OuterPen = InnerPen = (HPEN)GetStockObject(NULL_PEN);
    SavePen = (HPEN)SelectObject(hdc, InnerPen);
    spx = spy = epx = epy = 0; /* Satisfy the compiler... */

    /* Determine the colors of the edges */
    if(uFlags & BF_MONO)
    {
        InnerI = LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)];
        OuterI = LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)];
    }
    else if(uFlags & BF_FLAT)
    {
        InnerI = LTRBInnerFlat[uType & (BDR_INNER|BDR_OUTER)];
        OuterI = LTRBOuterFlat[uType & (BDR_INNER|BDR_OUTER)];
    }
    else if(uFlags & BF_SOFT)
    {
        if(uFlags & BF_BOTTOM)
        {
            InnerI = RBInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = RBOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
        }
        else
        {
            InnerI = LTInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = LTOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
        }
    }
    else
    {
        if(uFlags & BF_BOTTOM)
        {
            InnerI = RBInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = RBOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
        }
        else
        {
            InnerI = LTInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = LTOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
        }
    }

    if(InnerI != -1) InnerPen = GetStockObject(DC_PEN);
    if(OuterI != -1) OuterPen = GetStockObject(DC_PEN);

    MoveToEx(hdc, 0, 0, &SavePoint);

    /* Don't ask me why, but this is what is visible... */
    /* This must be possible to do much simpler, but I fail to */
    /* see the logic in the MS implementation (sigh...). */
    /* So, this might look a bit brute force here (and it is), but */
    /* it gets the job done;) */

    switch(uFlags & BF_RECT)
    {
        case 0:
        case BF_LEFT:
        case BF_BOTTOM:
        case BF_BOTTOMLEFT:
            /* Left bottom endpoint */
            epx = rc->left-1;
            spx = epx + SmallDiam;
            epy = rc->bottom;
            spy = epy - SmallDiam;
            break;

        case BF_TOPLEFT:
        case BF_BOTTOMRIGHT:
            /* Left top endpoint */
            epx = rc->left-1;
            spx = epx + SmallDiam;
            epy = rc->top-1;
            spy = epy + SmallDiam;
            break;

        case BF_TOP:
        case BF_RIGHT:
        case BF_TOPRIGHT:
        case BF_RIGHT|BF_LEFT:
        case BF_RIGHT|BF_LEFT|BF_TOP:
        case BF_BOTTOM|BF_TOP:
        case BF_BOTTOM|BF_TOP|BF_LEFT:
        case BF_BOTTOMRIGHT|BF_LEFT:
        case BF_BOTTOMRIGHT|BF_TOP:
        case BF_RECT:
            /* Right top endpoint */
            spx = rc->left;
            epx = spx + SmallDiam;
            spy = rc->bottom-1;
            epy = spy - SmallDiam;
            break;
    }

    MoveToEx(hdc, spx, spy, NULL);
    SelectObject(hdc, OuterPen);
    SetDCPenColor(hdc, GetSysColor(OuterI));
    LineTo(hdc, epx, epy);

    SelectObject(hdc, InnerPen);
    SetDCPenColor(hdc, GetSysColor(InnerI));

    switch(uFlags & (BF_RECT|BF_DIAGONAL))
    {
        case BF_DIAGONAL_ENDBOTTOMLEFT:
        case (BF_DIAGONAL|BF_BOTTOM):
        case BF_DIAGONAL:
        case (BF_DIAGONAL|BF_LEFT):
            MoveToEx(hdc, spx-1, spy, NULL);
            LineTo(hdc, epx, epy-1);
            Points[0].x = spx-add;
            Points[0].y = spy;
            Points[1].x = rc->left;
            Points[1].y = rc->top;
            Points[2].x = epx+1;
            Points[2].y = epy-1-add;
            Points[3] = Points[2];
            break;

        case BF_DIAGONAL_ENDBOTTOMRIGHT:
            MoveToEx(hdc, spx-1, spy, NULL);
            LineTo(hdc, epx, epy+1);
            Points[0].x = spx-add;
            Points[0].y = spy;
            Points[1].x = rc->left;
            Points[1].y = rc->bottom-1;
            Points[2].x = epx+1;
            Points[2].y = epy+1+add;
            Points[3] = Points[2];
            break;

        case (BF_DIAGONAL|BF_BOTTOM|BF_RIGHT|BF_TOP):
        case (BF_DIAGONAL|BF_BOTTOM|BF_RIGHT|BF_TOP|BF_LEFT):
        case BF_DIAGONAL_ENDTOPRIGHT:
        case (BF_DIAGONAL|BF_RIGHT|BF_TOP|BF_LEFT):
            MoveToEx(hdc, spx+1, spy, NULL);
            LineTo(hdc, epx, epy+1);
            Points[0].x = epx-1;
            Points[0].y = epy+1+add;
            Points[1].x = rc->right-1;
            Points[1].y = rc->top+add;
            Points[2].x = rc->right-1;
            Points[2].y = rc->bottom-1;
            Points[3].x = spx+add;
            Points[3].y = spy;
            break;

        case BF_DIAGONAL_ENDTOPLEFT:
            MoveToEx(hdc, spx, spy-1, NULL);
            LineTo(hdc, epx+1, epy);
            Points[0].x = epx+1+add;
            Points[0].y = epy+1;
            Points[1].x = rc->right-1;
            Points[1].y = rc->top;
            Points[2].x = rc->right-1;
            Points[2].y = rc->bottom-1-add;
            Points[3].x = spx;
            Points[3].y = spy-add;
            break;

        case (BF_DIAGONAL|BF_TOP):
        case (BF_DIAGONAL|BF_BOTTOM|BF_TOP):
        case (BF_DIAGONAL|BF_BOTTOM|BF_TOP|BF_LEFT):
            MoveToEx(hdc, spx+1, spy-1, NULL);
            LineTo(hdc, epx, epy);
            Points[0].x = epx-1;
            Points[0].y = epy+1;
            Points[1].x = rc->right-1;
            Points[1].y = rc->top;
            Points[2].x = rc->right-1;
            Points[2].y = rc->bottom-1-add;
            Points[3].x = spx+add;
            Points[3].y = spy-add;
            break;

        case (BF_DIAGONAL|BF_RIGHT):
        case (BF_DIAGONAL|BF_RIGHT|BF_LEFT):
        case (BF_DIAGONAL|BF_RIGHT|BF_LEFT|BF_BOTTOM):
            MoveToEx(hdc, spx, spy, NULL);
            LineTo(hdc, epx-1, epy+1);
            Points[0].x = spx;
            Points[0].y = spy;
            Points[1].x = rc->left;
            Points[1].y = rc->top+add;
            Points[2].x = epx-1-add;
            Points[2].y = epy+1+add;
            Points[3] = Points[2];
            break;
    }

    /* Fill the interior if asked */
    if((uFlags & BF_MIDDLE) && retval)
    {
        HBRUSH hbsave;
        HPEN hpsave;
        hbsave = (HBRUSH)SelectObject(hdc, GetStockObject(DC_BRUSH));
        hpsave = (HPEN)SelectObject(hdc, GetStockObject(DC_PEN));
        SetDCBrushColor(hdc, GetSysColor(uFlags & BF_MONO ? COLOR_WINDOW : COLOR_BTNFACE));
        SetDCPenColor(hdc, GetSysColor(uFlags & BF_MONO ? COLOR_WINDOW : COLOR_BTNFACE));
        Polygon(hdc, Points, 4);
        SelectObject(hdc, hbsave);
        SelectObject(hdc, hpsave);
    }

    /* Adjust rectangle if asked */
    if(uFlags & BF_ADJUST)
    {
        if(uFlags & BF_LEFT)   rc->left   += add;
        if(uFlags & BF_RIGHT)  rc->right  -= add;
        if(uFlags & BF_TOP)    rc->top    += add;
        if(uFlags & BF_BOTTOM) rc->bottom -= add;
    }

    /* Cleanup */
    SelectObject(hdc, SavePen);
    MoveToEx(hdc, SavePoint.x, SavePoint.y, NULL);

    return retval;
}

/* Ported from WINE20020904 */
/* Same as DrawEdge invoked without BF_DIAGONAL
 *
 * 23-Nov-1997: Changed by Bertho Stultiens
 * The width parameter sets the width of each outer and inner edge.
 *
 * Well, I started testing this and found out that there are a few things
 * that weren't quite as win95. The following rewrite should reproduce
 * win95 results completely.
 * The colorselection is table-driven to avoid awfull if-statements.
 * The table below show the color settings.
 *
 * Pen selection table for uFlags = 0
 *
 * uType |  LTI  |  LTO  |  RBI  |  RBO
 * ------+-------+-------+-------+-------
 *  0000 |   x   |   x   |   x   |   x
 *  0001 |   x   |  22   |   x   |  21
 *  0010 |   x   |  16   |   x   |  20
 *  0011 |   x   |   x   |   x   |   x
 * ------+-------+-------+-------+-------
 *  0100 |   x   |  20   |   x   |  16
 *  0101 |  20   |  22   |  16   |  21
 *  0110 |  20   |  16   |  16   |  20
 *  0111 |   x   |   x   |   x   |   x
 * ------+-------+-------+-------+-------
 *  1000 |   x   |  21   |   x   |  22
 *  1001 |  21   |  22   |  22   |  21
 *  1010 |  21   |  16   |  22   |  20
 *  1011 |   x   |   x   |   x   |   x
 * ------+-------+-------+-------+-------
 *  1100 |   x   |   x   |   x   |   x
 *  1101 |   x   | x (22)|   x   | x (21)
 *  1110 |   x   | x (16)|   x   | x (20)
 *  1111 |   x   |   x   |   x   |   x
 *
 * Pen selection table for uFlags = BF_SOFT
 *
 * uType |  LTI  |  LTO  |  RBI  |  RBO
 * ------+-------+-------+-------+-------
 *  0000 |   x   |   x   |   x   |   x
 *  0001 |   x   |  20   |   x   |  21
 *  0010 |   x   |  21   |   x   |  20
 *  0011 |   x   |   x   |   x   |   x
 * ------+-------+-------+-------+-------
 *  0100 |   x   |  22   |   x   |  16
 *  0101 |  22   |  20   |  16   |  21
 *  0110 |  22   |  21   |  16   |  20
 *  0111 |   x   |   x   |   x   |   x
 * ------+-------+-------+-------+-------
 *  1000 |   x   |  16   |   x   |  22
 *  1001 |  16   |  20   |  22   |  21
 *  1010 |  16   |  21   |  22   |  20
 *  1011 |   x   |   x   |   x   |   x
 * ------+-------+-------+-------+-------
 *  1100 |   x   |   x   |   x   |   x
 *  1101 |   x   | x (20)|   x   | x (21)
 *  1110 |   x   | x (21)|   x   | x (20)
 *  1111 |   x   |   x   |   x   |   x
 *
 * x = don't care; (n) = is what win95 actually uses
 * LTI = left Top Inner line
 * LTO = left Top Outer line
 * RBI = Right Bottom Inner line
 * RBO = Right Bottom Outer line
 * 15 = COLOR_BTNFACE
 * 16 = COLOR_BTNSHADOW
 * 20 = COLOR_BTNHIGHLIGHT
 * 21 = COLOR_3DDKSHADOW
 * 22 = COLOR_3DLIGHT
 */
static BOOL IntDrawRectEdge(HDC hdc, LPRECT rc, UINT uType, UINT uFlags, UINT width)
{
    signed char LTInnerI, LTOuterI;
    signed char RBInnerI, RBOuterI;
    HBRUSH lti_brush, lto_brush, rbi_brush, rbo_brush;
    RECT InnerRect = *rc, fill_rect;
    int lbi_offset = 0, lti_offset = 0, rti_offset = 0, rbi_offset = 0;
    BOOL retval = !(   ((uType & BDR_INNER) == BDR_INNER
                       || (uType & BDR_OUTER) == BDR_OUTER)
                      && !(uFlags & (BF_FLAT|BF_MONO)) );

    lti_brush = lto_brush = rbi_brush = rbo_brush = GetStockObject(NULL_BRUSH);

    /* Determine the colors of the edges */
    if(uFlags & BF_MONO)
    {
        LTInnerI = RBInnerI = LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = RBOuterI = LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)];
    }
    else if(uFlags & BF_FLAT)
    {
        LTInnerI = RBInnerI = LTRBInnerFlat[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = RBOuterI = LTRBOuterFlat[uType & (BDR_INNER|BDR_OUTER)];

        /* Bertho Stultiens states above that this function exactly matches win95
         * In win98 BF_FLAT rectangles have an inner border same color as the
        * middle (COLOR_BTNFACE). I believe it's the same for win95 but since
        * I don't know I go with Bertho and just sets it for win98 until proven
        * otherwise.
        *                                          Dennis Björklund, 10 June, 99
        */
	if( LTInnerI != -1 ) LTInnerI = RBInnerI = COLOR_BTNFACE;
    }
    else if(uFlags & BF_SOFT)
    {
        LTInnerI = LTInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = LTOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
        RBInnerI = RBInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
        RBOuterI = RBOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
    }
    else
    {
        LTInnerI = LTInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = LTOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
        RBInnerI = RBInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
        RBOuterI = RBOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
    }

    if((uFlags & BF_BOTTOMLEFT) == BF_BOTTOMLEFT)   lbi_offset = width;
    if((uFlags & BF_TOPRIGHT) == BF_TOPRIGHT)       rti_offset = width;
    if((uFlags & BF_BOTTOMRIGHT) == BF_BOTTOMRIGHT) rbi_offset = width;
    if((uFlags & BF_TOPLEFT) == BF_TOPLEFT)         lti_offset = width;

    if(LTInnerI != -1) lti_brush = GetSysColorBrush(LTInnerI);
    if(LTOuterI != -1) lto_brush = GetSysColorBrush(LTOuterI);
    if(RBInnerI != -1) rbi_brush = GetSysColorBrush(RBInnerI);
    if(RBOuterI != -1) rbo_brush = GetSysColorBrush(RBOuterI);

    /* Draw the outer edge */
    if(uFlags & BF_TOP)
    {
        fill_rect = InnerRect;
        fill_rect.bottom = fill_rect.top + width;
        FillRect( hdc, &fill_rect, lto_brush );
    }
    if(uFlags & BF_LEFT)
    {
        fill_rect = InnerRect;
        fill_rect.right = fill_rect.left + width;
        FillRect( hdc, &fill_rect, lto_brush );
    }
    if(uFlags & BF_BOTTOM)
    {
        fill_rect = InnerRect;
        fill_rect.top = fill_rect.bottom - width;
        FillRect( hdc, &fill_rect, rbo_brush );
    }
    if(uFlags & BF_RIGHT)
    {
        fill_rect = InnerRect;
        fill_rect.left = fill_rect.right - width;
        FillRect( hdc, &fill_rect, rbo_brush );
    }

    /* Draw the inner edge */
    if(uFlags & BF_TOP)
    {
        SetRect( &fill_rect, InnerRect.left + lti_offset, InnerRect.top + width,
                 InnerRect.right - rti_offset, InnerRect.top + 2 * width );
        FillRect( hdc, &fill_rect, lti_brush );
    }
    if(uFlags & BF_LEFT)
    {
        SetRect( &fill_rect, InnerRect.left + width, InnerRect.top + lti_offset,
                 InnerRect.left + 2 * width, InnerRect.bottom - lbi_offset );
        FillRect( hdc, &fill_rect, lti_brush );
    }
    if(uFlags & BF_BOTTOM)
    {
        SetRect( &fill_rect, InnerRect.left + lbi_offset, InnerRect.bottom - 2 * width,
                 InnerRect.right - rbi_offset, InnerRect.bottom - width );
        FillRect( hdc, &fill_rect, rbi_brush );
    }
    if(uFlags & BF_RIGHT)
    {
        SetRect( &fill_rect, InnerRect.right - 2 * width, InnerRect.top + rti_offset,
                 InnerRect.right - width, InnerRect.bottom - rbi_offset );
        FillRect( hdc, &fill_rect, rbi_brush );
    }

    if( ((uFlags & BF_MIDDLE) && retval) || (uFlags & BF_ADJUST) )
    {
        int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? width : 0)
                + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? width : 0);

        if(uFlags & BF_LEFT)   InnerRect.left   += add;
        if(uFlags & BF_RIGHT)  InnerRect.right  -= add;
        if(uFlags & BF_TOP)    InnerRect.top    += add;
        if(uFlags & BF_BOTTOM) InnerRect.bottom -= add;

        if((uFlags & BF_MIDDLE) && retval)
	{
            FillRect(hdc, &InnerRect, GetSysColorBrush(uFlags & BF_MONO ?
						       COLOR_WINDOW : COLOR_BTNFACE));
	}

	if(uFlags & BF_ADJUST)
	    *rc = InnerRect;
    }

    return retval;
}

/* Ported from WINE20020904 */
/* Utility to create a square rectangle and returning the width */
static int UITOOLS_MakeSquareRect(LPRECT src, LPRECT dst)
{
    int Width  = src->right - src->left;
    int Height = src->bottom - src->top;
    int SmallDiam = Width > Height ? Height : Width;

    *dst = *src;

    /* Make it a square box */
    if(Width < Height)      /* SmallDiam == Width */
    {
        dst->top += (Height-Width)/2;
        dst->bottom = dst->top + SmallDiam;
    }
    else if(Width > Height) /* SmallDiam == Height */
    {
        dst->left += (Width-Height)/2;
        dst->right = dst->left + SmallDiam;
    }

    return SmallDiam;
}

/* Ported from WINE20020904 */
static void UITOOLS_DrawCheckedRect( HDC dc, LPRECT rect )
{
    if(GetSysColor(COLOR_BTNHIGHLIGHT) == RGB(255, 255, 255))
    {
        HBRUSH hbsave;
        COLORREF bg;

        FillRect(dc, rect, GetSysColorBrush(COLOR_BTNFACE));
        bg = SetBkColor(dc, RGB(255, 255, 255));
        hbsave = (HBRUSH)SelectObject(dc, gpsi->hbrGray);
        PatBlt(dc, rect->left, rect->top, rect->right-rect->left, rect->bottom-rect->top, 0x00FA0089);
        SelectObject(dc, hbsave);
        SetBkColor(dc, bg);
    }
    else
    {
        FillRect(dc, rect, GetSysColorBrush(COLOR_BTNHIGHLIGHT));
    }
}

/* Ported from WINE20020904 */
/* Draw a push button coming from DrawFrameControl()
 *
 * Does a pretty good job in emulating MS behavior. Some quirks are
 * however there because MS uses a TrueType font (Marlett) to draw
 * the buttons.
 *
 * FIXME: This looks a little bit strange, needs to be rewritten completely
 * (several quirks with adjust, DFCS_CHECKED aso)
 */
static BOOL UITOOLS95_DFC_ButtonPush(HDC dc, LPRECT r, UINT uFlags)
{
    UINT edge;
    RECT myr = *r;

    if(uFlags & (DFCS_PUSHED | DFCS_CHECKED | DFCS_FLAT))
        edge = EDGE_SUNKEN;
    else
        edge = EDGE_RAISED;

    if(uFlags & DFCS_CHECKED)
    {
        if(uFlags & DFCS_MONO)
            IntDrawRectEdge(dc, &myr, edge, BF_MONO|BF_RECT|BF_ADJUST, 1);
        else
            IntDrawRectEdge(dc, &myr, edge, (uFlags&DFCS_FLAT)|BF_RECT|BF_SOFT|BF_ADJUST, 1);

        UITOOLS_DrawCheckedRect( dc, &myr );
    }
    else
    {
        if(uFlags & DFCS_MONO)
        {
            IntDrawRectEdge(dc, &myr, edge, BF_MONO|BF_RECT|BF_ADJUST, 1);
            FillRect(dc, &myr, GetSysColorBrush(COLOR_BTNFACE));
        }
        else
        {
            IntDrawRectEdge(dc, r, edge, (uFlags&DFCS_FLAT) | BF_MIDDLE | BF_RECT | BF_SOFT, 1);
        }
    }

    /* Adjust rectangle if asked */
    if(uFlags & DFCS_ADJUSTRECT)
        InflateRect(r, -2, -2);

    return TRUE;
}

static BOOL UITOOLS95_DFC_ButtonCheckRadio(HDC dc, LPRECT r, UINT uFlags, BOOL Radio)
{
    LOGFONTW lf;
    HFONT hFont, hOldFont;
    int i;
    TCHAR OutRight, OutLeft, InRight, InLeft, Center;
    INT X, Y, Width, Height, Shorter;
    INT BkMode = GetBkMode(dc);
    COLORREF TextColor = GetTextColor(dc);

    Width = r->right - r->left;
    Height = r->bottom - r->top;
    Shorter = (Width < Height) ? Width : Height;
    X = r->left + (Width - Shorter) / 2;
    Y = r->top + (Height - Shorter) / 2;

    if (Radio)
    {
        OutRight = 'j'; // Outer right
        OutLeft  = 'k'; // Outer left
        InRight  = 'l'; // inner left
        InLeft   = 'm'; // inner right
        Center   = 'n'; // center
    }
    else
    {
        OutRight = 'c'; // Outer right
        OutLeft  = 'd'; // Outer left
        InRight  = 'e'; // inner left
        InLeft   = 'f'; // inner right
        Center   = 'g'; // center
    }

    ZeroMemory(&lf, sizeof(LOGFONTW));
    lf.lfHeight = Shorter;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, TEXT("Marlett"));
    if (Radio && ((uFlags & 0xFF) == DFCS_BUTTONRADIOMASK))
    {
        lf.lfQuality = NONANTIALIASED_QUALITY;
    }
    hFont = CreateFontIndirect(&lf);
    hOldFont = SelectObject(dc, hFont);

    if (Radio && ((uFlags & 0xFF) == DFCS_BUTTONRADIOMASK))
    {
#if 1
        // FIXME: improve font rendering
        RECT Rect;
        HGDIOBJ hbrOld, hpenOld;
        SetRect(&Rect, X, Y, X + Shorter, Y + Shorter);
        InflateRect(&Rect, -(Shorter * 8) / 54, -(Shorter * 8) / 54);
        hbrOld = SelectObject(dc, GetStockObject(BLACK_BRUSH));
        hpenOld = SelectObject(dc, GetStockObject(NULL_PEN));
        Ellipse(dc, Rect.left, Rect.top, Rect.right, Rect.bottom);
        SelectObject(dc, hbrOld);
        SelectObject(dc, hpenOld);
#else
        SetBkMode(dc, OPAQUE);
        SetBkColor(dc, RGB(255, 255, 255));
        SetTextColor(dc, RGB(0, 0, 0));
        TextOut(dc, X, Y, &Center, 1);
        SetBkMode(dc, TRANSPARENT);
        TextOut(dc, X, Y, &OutRight, 1);
        TextOut(dc, X, Y, &OutLeft, 1);
        TextOut(dc, X, Y, &InRight, 1);
        TextOut(dc, X, Y, &InLeft, 1);
#endif
    }
    else
    {
        SetBkMode(dc, TRANSPARENT);

        /* Center section, white for active, grey for inactive */
        if ((uFlags & (DFCS_INACTIVE | DFCS_PUSHED)))
            i = COLOR_BTNFACE;
        else
            i = COLOR_WINDOW;
        SetTextColor(dc, GetSysColor(i));
        TextOut(dc, X, Y, &Center, 1);

        if (uFlags & (DFCS_FLAT | DFCS_MONO))
        {
            SetTextColor(dc, GetSysColor(COLOR_WINDOWFRAME));
            TextOut(dc, X, Y, &OutRight, 1);
            TextOut(dc, X, Y, &OutLeft, 1);
            TextOut(dc, X, Y, &InRight, 1);
            TextOut(dc, X, Y, &InLeft, 1);
        }
        else
        {
            SetTextColor(dc, GetSysColor(COLOR_BTNSHADOW));
            TextOut(dc, X, Y, &OutRight, 1);
            SetTextColor(dc, GetSysColor(COLOR_BTNHIGHLIGHT));
            TextOut(dc, X, Y, &OutLeft, 1);
            SetTextColor(dc, GetSysColor(COLOR_3DDKSHADOW));
            TextOut(dc, X, Y, &InRight, 1);
            SetTextColor(dc, GetSysColor(COLOR_3DLIGHT));
            TextOut(dc, X, Y, &InLeft, 1);
        }

        if (uFlags & DFCS_CHECKED)
        {
            TCHAR Check = (Radio) ? 'i' : 'b';

            SetTextColor(dc, GetSysColor((uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_WINDOWTEXT));
            TextOut(dc, X, Y, &Check, 1);
        }
    }

    SelectObject(dc, hOldFont);
    DeleteObject(hFont);

    SetTextColor(dc, TextColor);
    SetBkMode(dc, BkMode);

    return TRUE;
}

/* Ported from WINE20020904 */
static BOOL UITOOLS95_DrawFrameButton(HDC hdc, LPRECT rc, UINT uState)
{
    switch(uState & 0x1f)
    {
        case DFCS_BUTTONPUSH:
            return UITOOLS95_DFC_ButtonPush(hdc, rc, uState);

        case DFCS_BUTTONCHECK:
        case DFCS_BUTTON3STATE:
            return UITOOLS95_DFC_ButtonCheckRadio(hdc, rc, uState, FALSE);

        case DFCS_BUTTONRADIOIMAGE:
        case DFCS_BUTTONRADIOMASK:
            if (uState & DFCS_BUTTONRADIOIMAGE)
                FillRect(hdc, rc, (HBRUSH)GetStockObject(BLACK_BRUSH)); /* Fill by black */
            else
                FillRect(hdc, rc, (HBRUSH)GetStockObject(WHITE_BRUSH)); /* Fill by white */

            return UITOOLS95_DFC_ButtonCheckRadio(hdc, rc, uState, TRUE);

        case DFCS_BUTTONRADIO:
            return UITOOLS95_DFC_ButtonCheckRadio(hdc, rc, uState, TRUE);

        default:
            ERR("Invalid button state=0x%04x\n", uState);
    }

    return FALSE;
}

static BOOL UITOOLS95_DrawFrameCaption(HDC dc, LPRECT r, UINT uFlags)
{
    LOGFONTW lf;
    HFONT hFont, hOldFont;
    COLORREF clrsave;
    RECT myr;
    INT bkmode;
    TCHAR Symbol;
    switch(uFlags & 0xf)
    {
        case DFCS_CAPTIONCLOSE:
		Symbol = 'r';
		break;
        case DFCS_CAPTIONHELP:
		Symbol = 's';
		break;
        case DFCS_CAPTIONMIN:
		Symbol = '0';
		break;
        case DFCS_CAPTIONMAX:
		Symbol = '1';
		break;
        case DFCS_CAPTIONRESTORE:
		Symbol = '2';
		break;
        default:
             ERR("Invalid caption; flags=0x%04x\n", uFlags);
             return FALSE;
    }
    IntDrawRectEdge(dc,r,(uFlags&DFCS_PUSHED) ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT | BF_MIDDLE | BF_SOFT, 1);
    ZeroMemory(&lf, sizeof(LOGFONTW));
    UITOOLS_MakeSquareRect(r, &myr);
    myr.left += 1;
    myr.top += 1;
    myr.right -= 1;
    myr.bottom -= 1;
    if(uFlags & DFCS_PUSHED)
       OffsetRect(&myr,1,1);
    lf.lfHeight = myr.bottom - myr.top;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, TEXT("Marlett"));
    hFont = CreateFontIndirect(&lf);
    /* save font and text color */
    hOldFont = SelectObject(dc, hFont);
    clrsave = GetTextColor(dc);
    bkmode = GetBkMode(dc);
    /* set color and drawing mode */
    SetBkMode(dc, TRANSPARENT);
    if(uFlags & DFCS_INACTIVE)
    {
        /* draw shadow */
        SetTextColor(dc, GetSysColor(COLOR_BTNHIGHLIGHT));
        TextOut(dc, myr.left + 1, myr.top + 1, &Symbol, 1);
    }
    SetTextColor(dc, GetSysColor((uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT));
    /* draw selected symbol */
    TextOut(dc, myr.left, myr.top, &Symbol, 1);
    /* restore previous settings */
    SetTextColor(dc, clrsave);
    SelectObject(dc, hOldFont);
    SetBkMode(dc, bkmode);
    DeleteObject(hFont);
    return TRUE;
}

static BOOL UITOOLS95_DrawFrameScroll(HDC dc, LPRECT r, UINT uFlags)
{
    LOGFONTW lf;
    HFONT hFont, hOldFont;
    COLORREF clrsave;
    RECT myr;
    INT bkmode;
    TCHAR Symbol;
    switch(uFlags & 0x1f)
    {
        case DFCS_SCROLLCOMBOBOX:
        case DFCS_SCROLLDOWN:
		Symbol = '6';
		break;

	case DFCS_SCROLLUP:
		Symbol = '5';
		break;

	case DFCS_SCROLLLEFT:
		Symbol = '3';
		break;

	case DFCS_SCROLLRIGHT:
		Symbol = '4';
		break;

	case DFCS_SCROLLSIZEGRIP:
	case DFCS_SCROLLSIZEGRIPRIGHT:
		ZeroMemory(&lf, sizeof(LOGFONTW));
		UITOOLS_MakeSquareRect(r, &myr);
		lf.lfHeight = myr.bottom - myr.top;
		lf.lfWidth = 0;
		lf.lfWeight = FW_NORMAL;
		lf.lfCharSet = DEFAULT_CHARSET;
		lstrcpy(lf.lfFaceName, TEXT("Marlett"));
		hFont = CreateFontIndirect(&lf);
		/* save font and text color */
		hOldFont = SelectObject(dc, hFont);
		clrsave = GetTextColor(dc);
		bkmode = GetBkMode(dc);
		/* set color and drawing mode */
		SetBkMode(dc, TRANSPARENT);
		if (!(uFlags & (DFCS_MONO | DFCS_FLAT)))
		{
			SetTextColor(dc, GetSysColor(COLOR_BTNHIGHLIGHT));
			/* draw selected symbol */
			Symbol = ((uFlags & 0xff) == DFCS_SCROLLSIZEGRIP) ? 'o' : 'x';
			TextOut(dc, myr.left, myr.top, &Symbol, 1);
			SetTextColor(dc, GetSysColor(COLOR_BTNSHADOW));
		} else
			SetTextColor(dc, GetSysColor(COLOR_WINDOWFRAME));
		/* draw selected symbol */
		Symbol = ((uFlags & 0xff) == DFCS_SCROLLSIZEGRIP) ? 'p' : 'y';
		TextOut(dc, myr.left, myr.top, &Symbol, 1);
		/* restore previous settings */
		SetTextColor(dc, clrsave);
		SelectObject(dc, hOldFont);
		SetBkMode(dc, bkmode);
		DeleteObject(hFont);
            return TRUE;
	default:
	    ERR("Invalid scroll; flags=0x%04x\n", uFlags);
            return FALSE;
    }
    IntDrawRectEdge(dc, r, (uFlags & DFCS_PUSHED) ? EDGE_SUNKEN : EDGE_RAISED, (uFlags&DFCS_FLAT) | BF_MIDDLE | BF_RECT, 1);
    ZeroMemory(&lf, sizeof(LOGFONTW));
    UITOOLS_MakeSquareRect(r, &myr);
    myr.left += 1;
    myr.top += 1;
    myr.right -= 1;
    myr.bottom -= 1;
    if(uFlags & DFCS_PUSHED)
       OffsetRect(&myr,1,1);
    lf.lfHeight = myr.bottom - myr.top;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, TEXT("Marlett"));
    hFont = CreateFontIndirect(&lf);
    /* save font and text color */
    hOldFont = SelectObject(dc, hFont);
    clrsave = GetTextColor(dc);
    bkmode = GetBkMode(dc);
    /* set color and drawing mode */
    SetBkMode(dc, TRANSPARENT);
    if(uFlags & DFCS_INACTIVE)
    {
        /* draw shadow */
        SetTextColor(dc, GetSysColor(COLOR_BTNHIGHLIGHT));
        TextOut(dc, myr.left + 1, myr.top + 1, &Symbol, 1);
    }
    SetTextColor(dc, GetSysColor((uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT));
    /* draw selected symbol */
    TextOut(dc, myr.left, myr.top, &Symbol, 1);
    /* restore previous settings */
    SetTextColor(dc, clrsave);
    SelectObject(dc, hOldFont);
    SetBkMode(dc, bkmode);
    DeleteObject(hFont);
    return TRUE;
}

static BOOL UITOOLS95_DrawFrameMenu(HDC dc, LPRECT r, UINT uFlags)
{
    LOGFONTW lf;
    HFONT hFont, hOldFont;
    TCHAR Symbol;
    RECT myr;
    INT cxy;
    cxy = UITOOLS_MakeSquareRect(r, &myr);
    switch(uFlags & 0x1f)
    {
        case DFCS_MENUARROWUP:
            Symbol = '5';
            break;

        case DFCS_MENUARROWDOWN:
            Symbol = '6';
            break;

        case DFCS_MENUARROW:
            Symbol = '8';
            break;

        case DFCS_MENUARROWRIGHT:
	    Symbol = 'w'; // FIXME: needs to confirm
	    break;

        case DFCS_MENUBULLET:
            Symbol = 'h';
            break;

        case DFCS_MENUCHECK:
        case DFCS_MENUCHECK | DFCS_MENUBULLET:
            Symbol = 'a';
            break;

        default:
            ERR("Invalid menu; flags=0x%04x\n", uFlags);
            return FALSE;
    }
    /* acquire ressources only if valid menu */
    ZeroMemory(&lf, sizeof(LOGFONTW));
    lf.lfHeight = cxy;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, TEXT("Marlett"));
    hFont = CreateFontIndirect(&lf);
    /* save font */
    hOldFont = SelectObject(dc, hFont);

    if ((uFlags & 0x1f) == DFCS_MENUARROWUP ||  
        (uFlags & 0x1f) == DFCS_MENUARROWDOWN ) 
    {
#if 0
       if (uFlags & DFCS_INACTIVE)
       {
           /* draw shadow */
           SetTextColor(dc, GetSysColor(COLOR_BTNHIGHLIGHT));
           TextOut(dc, myr.left + 1, myr.top + 1, &Symbol, 1);
       }
#endif
       SetTextColor(dc, GetSysColor((uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT));
    }
    /* draw selected symbol */
    TextOut(dc, myr.left, myr.top, &Symbol, 1);
    /* restore previous settings */
    SelectObject(dc, hOldFont);
    DeleteObject(hFont);
    return TRUE;
}

BOOL
WINAPI
IntGrayString(
    HDC hDC,
    HBRUSH hBrush,
    GRAYSTRINGPROC lpOutputFunc,
    LPARAM lpData,
    int nCount,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    BOOL unicode)
{
    // AG: Mostly implemented, but probably won't work properly or return
    // correct error codes. I doubt it grays strings either... Untested!

    BOOL    success = FALSE;
    HBRUSH  hbsave;
    HDC     MemDC = NULL;
    HBITMAP MemBMP = NULL, OldBMP = NULL;
    HBRUSH  OldBrush = NULL;
    HFONT   OldFont = NULL;
    RECT    r;
    COLORREF ForeColor, BackColor;

    ForeColor = SetTextColor(hDC, RGB(0, 0, 0));
    BackColor = SetBkColor(hDC, RGB(255, 255, 255));

    if (! hBrush)
    {
        // The documentation is a little vague on what exactly should happen
        // here. Something about using the same brush for window text???
        hBrush = (HBRUSH) GetCurrentObject(hDC, OBJ_BRUSH);
    }

    if ((nCount == -1) && (! lpOutputFunc))
        return FALSE;

    if (! nCount)
    {
        // TODO: calculate the length (easy enough)

        if (unicode)
            nCount = lstrlenW((WCHAR*)lpData);
        else
            nCount = lstrlenA((CHAR*)lpData);
    }

    if (! nWidth || ! nHeight)
    {
        SIZE s;
        // TODO: calculate the rect

        if (unicode)
            success = GetTextExtentPoint32W(hDC, (WCHAR*) lpData, nCount, &s);
        else
            success = GetTextExtentPoint32A(hDC, (CHAR*) lpData, nCount, &s);

        if (! success) goto cleanup;

        if (! nWidth)   nWidth = s.cx;
        if (! nHeight)  nHeight = s.cy;
    }

    SetRect(&r, X, Y, X + nWidth, Y + nHeight);

    MemDC = CreateCompatibleDC(hDC);
    if (! MemDC) goto cleanup;
    MemBMP = CreateBitmap(nWidth, nHeight, 1, 1, NULL);
    if (! MemBMP) goto cleanup;
    OldBMP = SelectObject(MemDC, MemBMP);
    if (! OldBMP) goto cleanup;
    OldFont = SelectObject(MemDC, GetCurrentObject(hDC, OBJ_FONT));
    if (! OldFont) goto cleanup;
    OldBrush = SelectObject(MemDC, hBrush);
    if (! OldBrush) goto cleanup;

    if (! BitBlt(MemDC, 0, 0, nWidth, nHeight, hDC, X, Y, SRCCOPY)) goto cleanup;

    SetTextColor(MemDC, RGB(255, 255, 255));
    SetBkColor(MemDC, RGB(0, 0, 0));

    if (lpOutputFunc)
    {
        success = lpOutputFunc(MemDC, lpData, nCount); // Set brush etc first?

        if ((nCount == -1) && (! success))
        {
            // Don't gray (documented behaviour)
            success = (BOOL) BitBlt(hDC, X, Y, nWidth, nHeight, MemDC, 0, 0, SRCCOPY);
            goto cleanup;
        }
    }
    else
    {
        if (unicode)
            success = TextOutW(MemDC, 0, 0, (WCHAR*) lpData, nCount);
        else
            success = TextOutA(MemDC, 0, 0, (CHAR*) lpData, nCount);

        if (! success) goto cleanup;

        hbsave = (HBRUSH)SelectObject(MemDC, gpsi->hbrGray);
        PatBlt(MemDC, 0, 0, nWidth, nHeight, 0x000A0329);
        SelectObject(MemDC, hbsave);
    }

    if (! BitBlt(hDC, X, Y, nWidth, nHeight, MemDC, 0, 0, SRCCOPY)) goto cleanup;

cleanup:
    SetTextColor(hDC, ForeColor);
    SetBkColor(hDC, BackColor);

    if (MemDC)
    {
        if (OldFont) SelectObject(MemDC, OldFont);
        if (OldBrush) SelectObject(MemDC, OldBrush);
        if (OldBMP) SelectObject(MemDC, OldBMP);
        if (MemBMP) DeleteObject(MemBMP);
        DeleteDC(MemDC);
    }

    return success;
}

/**********************************************************************
 *          PAINTING_DrawStateJam
 *
 * Jams in the requested type in the dc
 */
static BOOL PAINTING_DrawStateJam(HDC hdc, UINT opcode,
                                  DRAWSTATEPROC func, LPARAM lp, WPARAM wp,
                                  LPRECT rc, UINT dtflags, BOOL unicode )
{
    HDC memdc;
    HBITMAP hbmsave;
    BOOL retval;
    INT cx = rc->right - rc->left;
    INT cy = rc->bottom - rc->top;

    switch(opcode)
    {
        case DST_TEXT:
        case DST_PREFIXTEXT:
            if(unicode)
                return DrawTextW(hdc, (LPWSTR)lp, (INT)wp, rc, dtflags);
            else
                return DrawTextA(hdc, (LPSTR)lp, (INT)wp, rc, dtflags);

        case DST_ICON:
            return DrawIconEx(hdc, rc->left, rc->top, (HICON)lp, cx, cy, 0, NULL, DI_NORMAL);

        case DST_BITMAP:
            memdc = CreateCompatibleDC(hdc);
            if(!memdc)
                return FALSE;
            hbmsave = (HBITMAP)SelectObject(memdc, (HBITMAP)lp);
            if(!hbmsave)
            {
                DeleteDC(memdc);
                return FALSE;
            }
            retval = BitBlt(hdc, rc->left, rc->top, cx, cy, memdc, 0, 0, SRCCOPY);
            SelectObject(memdc, hbmsave);
            DeleteDC(memdc);
            return retval;

        case DST_COMPLEX:
            if(func)
            {
                BOOL bRet;
                /* DRAWSTATEPROC assumes that it draws at the center of coordinates  */

                OffsetViewportOrgEx(hdc, rc->left, rc->top, NULL);
                bRet = func(hdc, lp, wp, cx, cy);
                /* Restore origin */
                OffsetViewportOrgEx(hdc, -rc->left, -rc->top, NULL);
                return bRet;
            }
            else
            {
                return FALSE;
            }
    }
    return FALSE;
}

static BOOL
IntDrawState(HDC hdc, HBRUSH hbr, DRAWSTATEPROC func, LPARAM lp, WPARAM wp,
             INT x, INT y, INT cx, INT cy, UINT flags, BOOL unicode)
{
    HBITMAP hbm, hbmsave;
    HFONT hfsave;
    HBRUSH hbsave, hbrtmp = 0;
    HDC memdc;
    RECT rc;
    UINT dtflags = DT_NOCLIP;
    COLORREF fg, bg;
    UINT opcode = flags & 0xf;
    INT len = wp;
    BOOL retval, tmp;
    LOGFONTW lf;
    HFONT hFontOriginal, hNaaFont = NULL;

    if((opcode == DST_TEXT || opcode == DST_PREFIXTEXT) && !len)    /* The string is '\0' terminated */
    {
        if(unicode)
            len = lstrlenW((LPWSTR)lp);
        else
            len = lstrlenA((LPSTR)lp);
    }

    hFontOriginal = GetCurrentObject(hdc, OBJ_FONT);
    if (flags & (DSS_MONO | DSS_DISABLED))
    {
        /* Create a non-antialiased font */
        GetObjectW(hFontOriginal, sizeof(lf), &lf);
        lf.lfQuality = NONANTIALIASED_QUALITY;
        hNaaFont = CreateFontIndirectW(&lf);
    }

    /* Find out what size the image has if not given by caller */
    if(!cx || !cy)
    {
        SIZE s;
        BITMAP bm;

        switch(opcode)
        {
            case DST_TEXT:
            case DST_PREFIXTEXT:
                if(unicode)
                    retval = GetTextExtentPoint32W(hdc, (LPWSTR)lp, len, &s);
                else
                    retval = GetTextExtentPoint32A(hdc, (LPSTR)lp, len, &s);
                if(!retval)
                    return FALSE;
                break;

            case DST_ICON:
                if(!get_icon_size((HICON)lp, &s))
                    return FALSE;
                break;

            case DST_BITMAP:
                if(!GetObjectW((HBITMAP)lp, sizeof(bm), &bm))
                    return FALSE;
                s.cx = bm.bmWidth;
                s.cy = bm.bmHeight;
                break;

            case DST_COMPLEX: /* cx and cy must be set in this mode */
                return FALSE;

            default:
                ERR("Invalid opcode: %u\n", opcode);
                return FALSE;
        }

        if(!cx) cx = s.cx;
        if(!cy) cy = s.cy;
    }

    SetRect(&rc, x, y, x + cx, y + cy);

    if(flags & DSS_RIGHT)    /* This one is not documented in the win32.hlp file */
        dtflags |= DT_RIGHT;
    if(opcode == DST_TEXT)
        dtflags |= DT_NOPREFIX;
    else if(opcode == DST_PREFIXTEXT)
    {
        if (flags & DSS_HIDEPREFIX)
            dtflags |= DT_HIDEPREFIX;
        if (flags & DSS_PREFIXONLY)
            dtflags |= DT_PREFIXONLY;
    }

    /* For DSS_NORMAL we just jam in the image and return */
    if((flags & 0x79f0) == DSS_NORMAL)
    {
        return PAINTING_DrawStateJam(hdc, opcode, func, lp, len, &rc, dtflags, unicode);
    }

    /* For all other states we need to convert the image to B/W in a local bitmap */
    /* before it is displayed */
    fg = SetTextColor(hdc, RGB(0, 0, 0));
    bg = SetBkColor(hdc, RGB(255, 255, 255));
    hbm = NULL; hbmsave = NULL;
    memdc = NULL; hbsave = NULL;
    retval = FALSE; /* assume failure */

    /* From here on we must use "goto cleanup" when something goes wrong */
    hbm     = CreateBitmap(cx, cy, 1, 1, NULL);
    if(!hbm) goto cleanup;
    memdc   = CreateCompatibleDC(hdc);
    if(!memdc) goto cleanup;
    hbmsave = (HBITMAP)SelectObject(memdc, hbm);
    if(!hbmsave) goto cleanup;
    SetRect(&rc, 0, 0, cx, cy);
    if(!FillRect(memdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH))) goto cleanup;
    SetBkColor(memdc, RGB(255, 255, 255));
    SetTextColor(memdc, RGB(0, 0, 0));
    if (hNaaFont)
        hfsave  = (HFONT)SelectObject(memdc, hNaaFont);
    else
        hfsave  = (HFONT)SelectObject(memdc, hFontOriginal);
    SetLayout( memdc, GetLayout( hdc ));

    /* DST_COMPLEX may draw text as well,
     * so we must be sure that correct font is selected
     */
    if(!hfsave && (opcode <= DST_PREFIXTEXT)) goto cleanup;
    tmp = PAINTING_DrawStateJam(memdc, opcode, func, lp, len, &rc, dtflags, unicode);
    if(hfsave) SelectObject(memdc, hfsave);
    if (hNaaFont) DeleteObject(hNaaFont);
    if(!tmp) goto cleanup;

    /* This state cause the image to be dithered */
    if(flags & DSS_UNION)
    {
        hbsave = (HBRUSH)SelectObject(memdc, gpsi->hbrGray);
        if(!hbsave) goto cleanup;
        tmp = PatBlt(memdc, 0, 0, cx, cy, 0x00FA0089);
        SelectObject(memdc, hbsave);
        if(!tmp) goto cleanup;
    }

    if (flags & DSS_DISABLED)
        hbrtmp = GetSysColorBrush(COLOR_3DHILIGHT);
    else if (flags & DSS_DEFAULT)
        hbrtmp = GetSysColorBrush(COLOR_3DSHADOW);

    /* Draw light or dark shadow */
    if (flags & (DSS_DISABLED|DSS_DEFAULT))
    {
        if(!hbrtmp) goto cleanup;
        hbsave = (HBRUSH)SelectObject(hdc, hbrtmp);
        if(!hbsave) goto cleanup;
        if(!BitBlt(hdc, x+1, y+1, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;
        SelectObject(hdc, hbsave);
    }

    if (flags & DSS_DISABLED)
    {
        hbr = hbrtmp = GetSysColorBrush(COLOR_3DSHADOW);
        if(!hbrtmp) goto cleanup;
    }
    else if (!hbr)
    {
        hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
    }

    hbsave = (HBRUSH)SelectObject(hdc, hbr);

    if(!BitBlt(hdc, x, y, cx, cy, memdc, 0, 0, 0x00B8074A)) goto cleanup;

    retval = TRUE; /* We succeeded */

cleanup:
    SetTextColor(hdc, fg);
    SetBkColor(hdc, bg);

    if(hbsave) SelectObject(hdc, hbsave);
    if(hbmsave) SelectObject(memdc, hbmsave);
    if(hbm) DeleteObject(hbm);
    if(memdc) DeleteDC(memdc);

    return retval;
}

/*
 * @implemented
 */
BOOL WINAPI
RealDrawFrameControl(HDC hDC, LPRECT rc, UINT uType, UINT uState)
{
    if (GetMapMode(hDC) != MM_TEXT)
        return FALSE;

    switch(uType)
    {
        case DFC_BUTTON:
            return UITOOLS95_DrawFrameButton(hDC, rc, uState);
        case DFC_CAPTION:
            return UITOOLS95_DrawFrameCaption(hDC, rc, uState);
        case DFC_MENU:
        {
            BOOL ret;
            COLORREF rgbOldText;
            INT iOldBackMode;

            if (uState & (DFCS_MENUARROWUP | DFCS_MENUARROWDOWN))
            {
                if (!(uState & DFCS_TRANSPARENT))
                    FillRect(hDC, rc, (HBRUSH)(COLOR_MENU + 1)); /* Fill by menu color */
            }
            else
            {
                FillRect(hDC, rc, (HBRUSH)GetStockObject(WHITE_BRUSH)); /* Fill by white */
            }

            rgbOldText = SetTextColor(hDC, RGB(0, 0, 0)); /* Draw by black */
            iOldBackMode = SetBkMode(hDC, TRANSPARENT);
            ret = UITOOLS95_DrawFrameMenu(hDC, rc, uState);
            SetBkMode(hDC, iOldBackMode);
            SetTextColor(hDC, rgbOldText);
            return ret;
        }
#if 0
        case DFC_POPUPMENU:
            UNIMPLEMENTED;
            break;
#endif
        case DFC_SCROLL:
            return UITOOLS95_DrawFrameScroll(hDC, rc, uState);
    }
    return FALSE;
}

BOOL WINAPI
DrawFrameControl(HDC hDC, LPRECT rc, UINT uType, UINT uState)
{
   BOOL Hook, Ret = FALSE;

   LoadUserApiHook();

   Hook = BeginIfHookedUserApiHook();

   /* Bypass SEH and go direct. */
   if (!Hook) return RealDrawFrameControl(hDC, rc, uType, uState);

   _SEH2_TRY
   {
      Ret = guah.DrawFrameControl(hDC, rc, uType, uState);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       ERR("Got exception in hooked DrawFrameControl!\n");
   }
   _SEH2_END;

   EndUserApiHook();

   return Ret;
}

/*
 * @implemented
 */
BOOL WINAPI
DrawEdge(HDC hDC, LPRECT rc, UINT edge, UINT flags)
{
    if (flags & BF_DIAGONAL)
        return IntDrawDiagEdge(hDC, rc, edge, flags);
    else
        return IntDrawRectEdge(hDC, rc, edge, flags, 1);
}

/*
 * @implemented
 */
BOOL WINAPI
GrayStringA(HDC hDC, HBRUSH hBrush, GRAYSTRINGPROC lpOutputFunc, LPARAM lpData,
            int nCount, int X, int Y, int nWidth, int nHeight)
{
    return IntGrayString(hDC, hBrush, lpOutputFunc, lpData, nCount, X, Y, nWidth, nHeight, FALSE);
}

/*
 * @implemented
 */
BOOL WINAPI
GrayStringW(HDC hDC, HBRUSH hBrush, GRAYSTRINGPROC lpOutputFunc, LPARAM lpData,
            int nCount, int X, int Y, int nWidth, int nHeight)
{
    return IntGrayString(hDC, hBrush, lpOutputFunc, lpData, nCount, X, Y, nWidth, nHeight, TRUE);
}

/*
 * @implemented
 */
BOOL WINAPI
InvertRect(HDC hDC, CONST RECT *lprc)
{
    return PatBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left,
                  lprc->bottom - lprc->top, DSTINVERT);
}

/*
 * @implemented
 */
INT WINAPI
FrameRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr)
{
    HBRUSH oldbrush;
    RECT r = *lprc;

    if (IsRectEmpty(&r)) return 0;
    if (!(oldbrush = SelectObject(hDC, hbr))) return 0;

    PatBlt(hDC, r.left, r.top, 1, r.bottom - r.top, PATCOPY);
    PatBlt(hDC, r.right - 1, r.top, 1, r.bottom - r.top, PATCOPY);
    PatBlt(hDC, r.left, r.top, r.right - r.left, 1, PATCOPY);
    PatBlt(hDC, r.left, r.bottom - 1, r.right - r.left, 1, PATCOPY);

    SelectObject(hDC, oldbrush);
    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
FlashWindow(HWND hWnd, BOOL bInvert)
{
    FLASHWINFO FlashWndInfo;

    FlashWndInfo.cbSize = sizeof(FLASHWINFO);
    FlashWndInfo.hwnd = hWnd;
    FlashWndInfo.dwFlags = !bInvert ? 0 : (FLASHW_TRAY | FLASHW_CAPTION);
    FlashWndInfo.uCount = 1;
    FlashWndInfo.dwTimeout = 0;

    return NtUserFlashWindowEx(&FlashWndInfo);
}

/*
 * @implemented
 */
INT WINAPI
FillRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr)
{
    BOOL Ret;
    HBRUSH prevhbr = NULL;

    /* Select brush if specified */
    if (hbr)
    {
        /* Handle system colors */
        if (hbr <= (HBRUSH)(COLOR_MENUBAR + 1))
            hbr = GetSysColorBrush(PtrToUlong(hbr) - 1);

        prevhbr = SelectObject(hDC, hbr);
        if (prevhbr == NULL)
            return (INT)FALSE;
    }

    Ret = PatBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left,
                 lprc->bottom - lprc->top, PATCOPY);

    /* Select old brush */
    if (prevhbr)
        SelectObject(hDC, prevhbr);

    return (INT)Ret;
}

/*
 * @implemented
 */
BOOL WINAPI
DrawFocusRect(HDC hdc, CONST RECT *rect)
{
    HGDIOBJ OldObj;
    UINT cx, cy;

    NtUserSystemParametersInfo(SPI_GETFOCUSBORDERWIDTH, 0, &cx, 0);
    NtUserSystemParametersInfo(SPI_GETFOCUSBORDERHEIGHT, 0, &cy, 0);

    OldObj = SelectObject(hdc, gpsi->hbrGray);

    /* top */
    PatBlt(hdc, rect->left, rect->top, rect->right - rect->left, cy, PATINVERT);
    /* bottom */
    PatBlt(hdc, rect->left, rect->bottom - cy, rect->right - rect->left, cy, PATINVERT);
    /* left */
    PatBlt(hdc, rect->left, rect->top + cy, cx, rect->bottom - rect->top - (2 * cy), PATINVERT);
    /* right */
    PatBlt(hdc, rect->right - cx, rect->top + cy, cx, rect->bottom - rect->top - (2 * cy), PATINVERT);

    SelectObject(hdc, OldObj);
    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
DrawStateA(HDC hDC, HBRUSH hBrush, DRAWSTATEPROC lpOutputFunc, LPARAM lData,
           WPARAM wData, int x, int y, int cx, int cy, UINT fuFlags)
{
    return IntDrawState(hDC, hBrush, lpOutputFunc, lData, wData, x, y, cx, cy, fuFlags, FALSE);
}

/*
 * @implemented
 */
BOOL WINAPI
DrawStateW(HDC hDC, HBRUSH hBrush, DRAWSTATEPROC lpOutputFunc, LPARAM lData,
           WPARAM wData, int x, int y, int cx, int cy, UINT fuFlags)
{
    return IntDrawState(hDC, hBrush, lpOutputFunc, lData, wData, x, y, cx, cy, fuFlags, TRUE);
}
