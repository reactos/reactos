/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: draw.c,v 1.5 2002/11/24 20:25:49 jfilby Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* GLOBALS *******************************************************************/

#define COLOR_MAX (28)

/* HPEN STDCALL W32kGetSysColorPen(int nIndex); */

static const WORD wPattern_AA55[8] = { 0xaaaa, 0x5555, 0xaaaa, 0x5555,
                                       0xaaaa, 0x5555, 0xaaaa, 0x5555 };

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


HPEN STDCALL GetSysColorPen(int nIndex);
HBRUSH STDCALL GetSysColorBrush(int nIndex);

/* Ported from WINE20020904 */
/* Same as DrawEdge invoked with BF_DIAGONAL */
static BOOL UITOOLS95_DrawDiagEdge(HDC hdc, LPRECT rc,
				     UINT uType, UINT uFlags)
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

    if(InnerI != -1) InnerPen = GetSysColorPen(InnerI);
    if(OuterI != -1) OuterPen = GetSysColorPen(OuterI);

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
    LineTo(hdc, epx, epy);

    SelectObject(hdc, InnerPen);

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
        HBRUSH hb = GetSysColorBrush(uFlags & BF_MONO ? COLOR_WINDOW : COLOR_BTNFACE);
        HPEN hpsave;
        HPEN hp = GetSysColorPen(uFlags & BF_MONO ? COLOR_WINDOW : COLOR_BTNFACE);
        hbsave = (HBRUSH)SelectObject(hdc, hb);
        hpsave = (HPEN)SelectObject(hdc, hp);
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
static BOOL UITOOLS95_DrawRectEdge(HDC hdc, LPRECT rc,
				     UINT uType, UINT uFlags)
{
    signed char LTInnerI, LTOuterI;
    signed char RBInnerI, RBOuterI;
    HPEN LTInnerPen, LTOuterPen;
    HPEN RBInnerPen, RBOuterPen;
    RECT InnerRect = *rc;
    POINT SavePoint;
    HPEN SavePen;
    int LBpenplus = 0;
    int LTpenplus = 0;
    int RTpenplus = 0;
    int RBpenplus = 0;
    BOOL retval = !(   ((uType & BDR_INNER) == BDR_INNER
                       || (uType & BDR_OUTER) == BDR_OUTER)
                      && !(uFlags & (BF_FLAT|BF_MONO)) );

    /* Init some vars */
    LTInnerPen = LTOuterPen = RBInnerPen = RBOuterPen = (HPEN)GetStockObject(NULL_PEN);
    SavePen = (HPEN)SelectObject(hdc, LTInnerPen);

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
/*	if( TWEAK_WineLook == WIN98_LOOK && LTInnerI != -1 )
            LTInnerI = RBInnerI = COLOR_BTNFACE; */
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

    if((uFlags & BF_BOTTOMLEFT) == BF_BOTTOMLEFT)   LBpenplus = 1;
    if((uFlags & BF_TOPRIGHT) == BF_TOPRIGHT)       RTpenplus = 1;
    if((uFlags & BF_BOTTOMRIGHT) == BF_BOTTOMRIGHT) RBpenplus = 1;
    if((uFlags & BF_TOPLEFT) == BF_TOPLEFT)         LTpenplus = 1;

    if(LTInnerI != -1) LTInnerPen = GetSysColorPen(LTInnerI);
    if(LTOuterI != -1) LTOuterPen = GetSysColorPen(LTOuterI);
    if(RBInnerI != -1) RBInnerPen = GetSysColorPen(RBInnerI);
    if(RBOuterI != -1) RBOuterPen = GetSysColorPen(RBOuterI);

    MoveToEx(hdc, 0, 0, &SavePoint);

    /* Draw the outer edge */
    SelectObject(hdc, LTOuterPen);
    if(uFlags & BF_TOP)
    {
        MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
        LineTo(hdc, InnerRect.right, InnerRect.top);
    }
    if(uFlags & BF_LEFT)
    {
        MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
        LineTo(hdc, InnerRect.left, InnerRect.bottom);
    }
    SelectObject(hdc, RBOuterPen);
    if(uFlags & BF_BOTTOM)
    {
        MoveToEx(hdc, InnerRect.right-1, InnerRect.bottom-1, NULL);
        LineTo(hdc, InnerRect.left-1, InnerRect.bottom-1);
    }
    if(uFlags & BF_RIGHT)
    {
        MoveToEx(hdc, InnerRect.right-1, InnerRect.bottom-1, NULL);
        LineTo(hdc, InnerRect.right-1, InnerRect.top-1);
    }

    /* Draw the inner edge */
    SelectObject(hdc, LTInnerPen);
    if(uFlags & BF_TOP)
    {
        MoveToEx(hdc, InnerRect.left+LTpenplus, InnerRect.top+1, NULL);
        LineTo(hdc, InnerRect.right-RTpenplus, InnerRect.top+1);
    }
    if(uFlags & BF_LEFT)
    {
        MoveToEx(hdc, InnerRect.left+1, InnerRect.top+LTpenplus, NULL);
        LineTo(hdc, InnerRect.left+1, InnerRect.bottom-LBpenplus);
    }
    SelectObject(hdc, RBInnerPen);
    if(uFlags & BF_BOTTOM)
    {
        MoveToEx(hdc, InnerRect.right-1-RBpenplus, InnerRect.bottom-2, NULL);
        LineTo(hdc, InnerRect.left-1+LBpenplus, InnerRect.bottom-2);
    }
    if(uFlags & BF_RIGHT)
    {
        MoveToEx(hdc, InnerRect.right-2, InnerRect.bottom-1-RBpenplus, NULL);
        LineTo(hdc, InnerRect.right-2, InnerRect.top-1+RTpenplus);
    }

    if( ((uFlags & BF_MIDDLE) && retval) || (uFlags & BF_ADJUST) )
    {
        int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0)
                + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0);

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

    /* Cleanup */
    SelectObject(hdc, SavePen);
    MoveToEx(hdc, SavePoint.x, SavePoint.y, NULL);
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
      HBITMAP hbm = CreateBitmap(8, 8, 1, 1, wPattern_AA55);
      HBRUSH hbsave;
      HBRUSH hb = CreatePatternBrush(hbm);
      COLORREF bg;

      FillRect(dc, rect, GetSysColorBrush(COLOR_BTNFACE));
      bg = SetBkColor(dc, RGB(255, 255, 255));
      hbsave = (HBRUSH)SelectObject(dc, hb);
      PatBlt(dc, rect->left, rect->top, rect->right-rect->left, rect->bottom-rect->top, 0x00FA0089);
      SelectObject(dc, hbsave);
      SetBkColor(dc, bg);
      DeleteObject(hb);
      DeleteObject(hbm);
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
            UITOOLS95_DrawRectEdge(dc, &myr, edge, BF_MONO|BF_RECT|BF_ADJUST);
        else
            UITOOLS95_DrawRectEdge(dc, &myr, edge, (uFlags&DFCS_FLAT)|BF_RECT|BF_SOFT|BF_ADJUST);

	UITOOLS_DrawCheckedRect( dc, &myr );
        }
        else
        {
        if(uFlags & DFCS_MONO)
        {
            UITOOLS95_DrawRectEdge(dc, &myr, edge, BF_MONO|BF_RECT|BF_ADJUST);
            FillRect(dc, &myr, GetSysColorBrush(COLOR_BTNFACE));
        }
        else
        {
            UITOOLS95_DrawRectEdge(dc, r, edge, (uFlags&DFCS_FLAT) | BF_MIDDLE | BF_RECT);
        }
    }

    /* Adjust rectangle if asked */
    if(uFlags & DFCS_ADJUSTRECT)
    {
        r->left   += 2;
        r->right  -= 2;
        r->top    += 2;
        r->bottom -= 2;
    }

    return TRUE;
}

/* Ported from WINE20020904 */
/* Draw a check/3state button coming from DrawFrameControl()
 *
 * Does a pretty good job in emulating MS behavior. Some quirks are
 * however there because MS uses a TrueType font (Marlett) to draw
 * the buttons.
 */
static BOOL UITOOLS95_DFC_ButtonCheck(HDC dc, LPRECT r, UINT uFlags)
{
    RECT myr, bar;
    UINT flags = BF_RECT | BF_ADJUST;
    UITOOLS_MakeSquareRect(r, &myr);

    if(uFlags & DFCS_FLAT) flags |= BF_FLAT;
    else if(uFlags & DFCS_MONO) flags |= BF_MONO;

    UITOOLS95_DrawRectEdge( dc, &myr, EDGE_SUNKEN, flags );

    if(uFlags & (DFCS_INACTIVE|DFCS_PUSHED))
        FillRect(dc, &myr, GetSysColorBrush(COLOR_BTNFACE));
    else if( (uFlags & DFCS_BUTTON3STATE) && (uFlags & DFCS_CHECKED) )
        UITOOLS_DrawCheckedRect( dc, &myr );
    else
    {
        FillRect(dc, &myr, GetSysColorBrush(COLOR_WINDOW));
    }

    if(uFlags & DFCS_CHECKED)
    {
        int i, k;
        i = (uFlags & DFCS_INACTIVE) || (uFlags & 0xff) == DFCS_BUTTON3STATE ?
		COLOR_BTNSHADOW : COLOR_WINDOWTEXT;

        /* draw 7 bars, with h=3w to form the check */
        bar.left = myr.left;
	bar.top = myr.top + 2;
        for (k = 0; k < 7; k++) {
            bar.left = bar.left + 1;
	    bar.top = (k < 3) ? bar.top + 1 : bar.top - 1;
	    bar.bottom = bar.top + 3;
   	    bar.right = bar.left + 1;
            FillRect(dc, &bar, GetSysColorBrush(i));
	}
    }
    return TRUE;
}

/* Ported from WINE20020904 */
/* Draw a radio/radioimage/radiomask button coming from DrawFrameControl()
 *
 * Does a pretty good job in emulating MS behavior. Some quirks are
 * however there because MS uses a TrueType font (Marlett) to draw
 * the buttons.
 */
static BOOL UITOOLS95_DFC_ButtonRadio(HDC dc, LPRECT r, UINT uFlags)
{
    RECT myr;
    int i;
    int SmallDiam = UITOOLS_MakeSquareRect(r, &myr);
    int BorderShrink = SmallDiam / 16;
    HPEN hpsave;
    HBRUSH hbsave;
    int xc, yc;

    if(BorderShrink < 1) BorderShrink = 1;

    if((uFlags & 0xff) == DFCS_BUTTONRADIOIMAGE)
    {
        FillRect(dc, r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    xc = myr.left + SmallDiam - SmallDiam/2;
    yc = myr.top  + SmallDiam - SmallDiam/2;

    /* Define bounding box */
    i = 14*SmallDiam/16;
    myr.left   = xc - i+i/2;
    myr.right  = xc + i/2;
    myr.top    = yc - i+i/2;
    myr.bottom = yc + i/2;

    if((uFlags & 0xff) == DFCS_BUTTONRADIOMASK)
    {
        hbsave = (HBRUSH)SelectObject(dc, GetStockObject(BLACK_BRUSH));
        Ellipse(dc, myr.left, myr.top, myr.right, myr.bottom);
        SelectObject(dc, hbsave);
    }
    else
    {
        if(uFlags & (DFCS_FLAT|DFCS_MONO))
        {
            hpsave = (HPEN)SelectObject(dc, GetSysColorPen(COLOR_WINDOWFRAME));
            hbsave = (HBRUSH)SelectObject(dc, GetSysColorBrush(COLOR_WINDOWFRAME));
            Ellipse(dc, myr.left, myr.top, myr.right, myr.bottom);
            SelectObject(dc, hbsave);
            SelectObject(dc, hpsave);
        }
        else
        {
            hpsave = (HPEN)SelectObject(dc, GetSysColorPen(COLOR_BTNHIGHLIGHT));
            hbsave = (HBRUSH)SelectObject(dc, GetSysColorBrush(COLOR_BTNHIGHLIGHT));
            Pie(dc, myr.left, myr.top, myr.right+1, myr.bottom+1, myr.left-1, myr.bottom, myr.right+1, myr.top);

            SelectObject(dc, GetSysColorPen(COLOR_BTNSHADOW));
            SelectObject(dc, GetSysColorBrush(COLOR_BTNSHADOW));
            Pie(dc, myr.left, myr.top, myr.right+1, myr.bottom+1, myr.right+1, myr.top, myr.left-1, myr.bottom);

            myr.left   += BorderShrink;
            myr.right  -= BorderShrink;
            myr.top    += BorderShrink;
            myr.bottom -= BorderShrink;

            SelectObject(dc, GetSysColorPen(COLOR_3DLIGHT));
            SelectObject(dc, GetSysColorBrush(COLOR_3DLIGHT));
            Pie(dc, myr.left, myr.top, myr.right+1, myr.bottom+1, myr.left-1, myr.bottom, myr.right+1, myr.top);

            SelectObject(dc, GetSysColorPen(COLOR_3DDKSHADOW));
            SelectObject(dc, GetSysColorBrush(COLOR_3DDKSHADOW));
            Pie(dc, myr.left, myr.top, myr.right+1, myr.bottom+1, myr.right+1, myr.top, myr.left-1, myr.bottom);
            SelectObject(dc, hbsave);
            SelectObject(dc, hpsave);
        }

        i = 10*SmallDiam/16;
        myr.left   = xc - i+i/2;
        myr.right  = xc + i/2;
        myr.top    = yc - i+i/2;
        myr.bottom = yc + i/2;
        i= !(uFlags & (DFCS_INACTIVE|DFCS_PUSHED)) ? COLOR_WINDOW : COLOR_BTNFACE;
        hpsave = (HPEN)SelectObject(dc, GetSysColorPen(i));
        hbsave = (HBRUSH)SelectObject(dc, GetSysColorBrush(i));
        Ellipse(dc, myr.left, myr.top, myr.right, myr.bottom);
        SelectObject(dc, hbsave);
        SelectObject(dc, hpsave);
    }

    if(uFlags & DFCS_CHECKED)
    {
        i = 6*SmallDiam/16;
        i = i < 1 ? 1 : i;
        myr.left   = xc - i+i/2;
        myr.right  = xc + i/2;
        myr.top    = yc - i+i/2;
        myr.bottom = yc + i/2;

        i = uFlags & DFCS_INACTIVE ? COLOR_BTNSHADOW : COLOR_WINDOWTEXT;
        hbsave = (HBRUSH)SelectObject(dc, GetSysColorBrush(i));
        hpsave = (HPEN)SelectObject(dc, GetSysColorPen(i));
        Ellipse(dc, myr.left, myr.top, myr.right, myr.bottom);
        SelectObject(dc, hpsave);
        SelectObject(dc, hbsave);
    }

    /* FIXME: M$ has a Polygon in the center at relative points: */
    /* 0.476, 0.476 (times SmallDiam, SmallDiam) */
    /* 0.476, 0.525 */
    /* 0.500, 0.500 */
    /* 0.500, 0.499 */
    /* when the button is unchecked. The reason for it is unknown. The */
    /* color is COLOR_BTNHIGHLIGHT, although the Polygon gets painted at */
    /* least 3 times (it looks like a clip-region when you see it happen). */
    /* I do not really see a reason why this should be implemented. If you */
    /* have a good reason, let me know. Maybe this is a quirk in the Marlett */
    /* font. */

    return TRUE;
}

/* Ported from WINE20020904 */
static BOOL UITOOLS95_DrawFrameButton(HDC hdc, LPRECT rc, UINT uState)
{
    switch(uState & 0xff)
    {
    case DFCS_BUTTONPUSH:
        return UITOOLS95_DFC_ButtonPush(hdc, rc, uState);

    case DFCS_BUTTONCHECK:
    case DFCS_BUTTON3STATE:
        return UITOOLS95_DFC_ButtonCheck(hdc, rc, uState);

    case DFCS_BUTTONRADIOIMAGE:
    case DFCS_BUTTONRADIOMASK:
    case DFCS_BUTTONRADIO:
        return UITOOLS95_DFC_ButtonRadio(hdc, rc, uState);

    default:
        DbgPrint("Invalid button state=0x%04x\n", uState);
    }

    return FALSE;
}

/* Ported from WINE20020904 */
/* Draw caption buttons (win95), coming from DrawFrameControl() */
static BOOL UITOOLS95_DrawFrameCaption(HDC dc, LPRECT r, UINT uFlags)
{
    POINT Line1[10];
    POINT Line2[10];
    int Line1N;
    int Line2N;
    RECT myr;
    int SmallDiam = UITOOLS_MakeSquareRect(r, &myr)-2;
    int i;
    HBRUSH hbsave;
    HPEN hpsave;
    HFONT hfsave, hf;
    int colorIdx = uFlags & DFCS_INACTIVE ? COLOR_BTNSHADOW : COLOR_BTNTEXT;
    int xc = (myr.left+myr.right)/2;
    int yc = (myr.top+myr.bottom)/2;
    int edge, move;
    char str[2] = "?";
    UINT alignsave;
    int bksave;
    COLORREF clrsave;
    SIZE size;

    UITOOLS95_DFC_ButtonPush(dc, r, uFlags & 0xff00);

    switch(uFlags & 0xff)
    {
    case DFCS_CAPTIONCLOSE:
    {
        /* The "X" is made by drawing a series of lines.
         * The number of lines drawn depends on the size
         * of the bounding rect.  e.g. For a 6x5 inside rect,
         * two lines are drawn from top-left to bottom-right,
         * and two lines from top-right to bottom-left.
         *
         * 0 1 2 3 4 5       0 1 2 3 4 5
         * 1 * *                     * *
         * 2   * *                 * *
         * 3     * *             * *
         * 4       * *         * *
         *
         * Drawing one line for every 6 pixels in width
         * seems to provide the best proportions.
         */

        POINT start, oldPos;
        INT width = myr.right - myr.left - 5;
        INT height = myr.bottom - myr.top - 6;
        INT numLines = (width / 6) + 1;

        hpsave = (HPEN)SelectObject(dc, GetSysColorPen(colorIdx));

        start.x = myr.left + 2;
        start.y = myr.top + 2;

        if (width < 6)
            height = width;
        else
            start.y++;

        if (uFlags & DFCS_PUSHED)
        {
            start.x++;
			start.y++;
        }

        /* now use the width of each line */
        width -= numLines - 1;

        for (i = 0; i < numLines; i++)
        {
            MoveToEx(dc, start.x + i, start.y, &oldPos);
            LineTo(dc, start.x + i + width, start.y + height);

            MoveToEx(dc, start.x + i, start.y + height - 1, &oldPos);
            LineTo(dc, start.x + i + width, start.y - 1);
        }

        SelectObject(dc, hpsave);
        return TRUE;
    }

    case DFCS_CAPTIONHELP:
        /* This one breaks the flow */
        /* FIXME: We need the Marlett font in order to get this right. */

        hf = CreateFontA(-SmallDiam, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                         ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "System");
        alignsave = SetTextAlign(dc, TA_TOP|TA_LEFT);
        bksave = SetBkMode(dc, TRANSPARENT);
        clrsave = GetTextColor(dc);
        hfsave = (HFONT)SelectObject(dc, hf);
        GetTextExtentPoint32A(dc, str, 1, &size);

        if(uFlags & DFCS_INACTIVE)
        {
            SetTextColor(dc, GetSysColor(COLOR_BTNHIGHLIGHT));
            TextOutA(dc, xc-size.cx/2+1, yc-size.cy/2+1, str, 1);
        }
        SetTextColor(dc, GetSysColor(colorIdx));
        TextOutA(dc, xc-size.cx/2, yc-size.cy/2, str, 1);

        SelectObject(dc, hfsave);
        SetTextColor(dc, clrsave);
        SetBkMode(dc, bksave);
        SetTextAlign(dc, alignsave);
        DeleteObject(hf);
        return TRUE;

    case DFCS_CAPTIONMIN:
        Line1[0].x = Line1[3].x = myr.left   +  96*SmallDiam/750+2;
        Line1[1].x = Line1[2].x = Line1[0].x + 372*SmallDiam/750;
        Line1[0].y = Line1[1].y = myr.top    + 563*SmallDiam/750+1;
        Line1[2].y = Line1[3].y = Line1[0].y +  92*SmallDiam/750;
        Line1N = 4;
        Line2N = 0;
        break;

    case DFCS_CAPTIONMAX:
        edge = 47*SmallDiam/750;
        Line1[0].x = Line1[5].x = myr.left +  57*SmallDiam/750+3;
        Line1[0].y = Line1[1].y = myr.top  + 143*SmallDiam/750+1;
        Line1[1].x = Line1[2].x = Line1[0].x + 562*SmallDiam/750;
        Line1[5].y = Line1[4].y = Line1[0].y +  93*SmallDiam/750;
        Line1[2].y = Line1[3].y = Line1[0].y + 513*SmallDiam/750;
        Line1[3].x = Line1[4].x = Line1[1].x -  edge;

        Line2[0].x = Line2[5].x = Line1[0].x;
        Line2[3].x = Line2[4].x = Line1[1].x;
        Line2[1].x = Line2[2].x = Line1[0].x + edge;
        Line2[0].y = Line2[1].y = Line1[0].y;
        Line2[4].y = Line2[5].y = Line1[2].y;
        Line2[2].y = Line2[3].y = Line1[2].y - edge;
        Line1N = 6;
        Line2N = 6;
        break;

    case DFCS_CAPTIONRESTORE:
        /* FIXME: this one looks bad at small sizes < 15x15 :( */
        edge = 47*SmallDiam/750;
        move = 420*SmallDiam/750;
        Line1[0].x = Line1[9].x = myr.left + 198*SmallDiam/750+2;
        Line1[0].y = Line1[1].y = myr.top  + 169*SmallDiam/750+1;
        Line1[6].y = Line1[7].y = Line1[0].y + 93*SmallDiam/750;
        Line1[7].x = Line1[8].x = Line1[0].x + edge;
        Line1[1].x = Line1[2].x = Line1[0].x + move;
        Line1[5].x = Line1[6].x = Line1[1].x - edge;
        Line1[9].y = Line1[8].y = Line1[0].y + 187*SmallDiam/750;
        Line1[2].y = Line1[3].y = Line1[0].y + 327*SmallDiam/750;
        Line1[4].y = Line1[5].y = Line1[2].y - edge;
        Line1[3].x = Line1[4].x = Line1[2].x - 140*SmallDiam/750;

        Line2[1].x = Line2[2].x = Line1[3].x;
        Line2[7].x = Line2[8].x = Line2[1].x - edge;
        Line2[0].x = Line2[9].x = Line2[3].x = Line2[4].x = Line2[1].x - move;
        Line2[5].x = Line2[6].x = Line2[0].x + edge;
        Line2[0].y = Line2[1].y = Line1[9].y;
        Line2[4].y = Line2[5].y = Line2[8].y = Line2[9].y = Line2[0].y + 93*SmallDiam/750;
        Line2[2].y = Line2[3].y = Line2[0].y + 327*SmallDiam/750;
        Line2[6].y = Line2[7].y = Line2[2].y - edge;
        Line1N = 10;
        Line2N = 10;
        break;

    default:
        DbgPrint("Invalid caption; flags=0x%04x\n", uFlags);
        return FALSE;
    }

    /* Here the drawing takes place */
    if(uFlags & DFCS_INACTIVE)
    {
        /* If we have an inactive button, then you see a shadow */
        hbsave = (HBRUSH)SelectObject(dc, GetSysColorBrush(COLOR_BTNHIGHLIGHT));
        hpsave = (HPEN)SelectObject(dc, GetSysColorPen(COLOR_BTNHIGHLIGHT));
        Polygon(dc, Line1, Line1N);
        if(Line2N > 0)
            Polygon(dc, Line2, Line2N);
        SelectObject(dc, hpsave);
        SelectObject(dc, hbsave);
    }

    /* Correct for the shadow shift */
    if (!(uFlags & DFCS_PUSHED))
    {
        for(i = 0; i < Line1N; i++)
        {
            Line1[i].x--;
            Line1[i].y--;
        }
        for(i = 0; i < Line2N; i++)
        {
            Line2[i].x--;
            Line2[i].y--;
        }
    }

    /* Make the final picture */
    hbsave = (HBRUSH)SelectObject(dc, GetSysColorBrush(colorIdx));
    hpsave = (HPEN)SelectObject(dc, GetSysColorPen(colorIdx));

    Polygon(dc, Line1, Line1N);
    if(Line2N > 0)
        Polygon(dc, Line2, Line2N);
    SelectObject(dc, hpsave);
    SelectObject(dc, hbsave);

    return TRUE;
}

/* Ported from WINE20020904 */
/* Draw a scroll-bar control coming from DrawFrameControl() */
static BOOL UITOOLS95_DrawFrameScroll(HDC dc, LPRECT r, UINT uFlags)
{
    POINT Line[4];
    RECT myr;
    int SmallDiam = UITOOLS_MakeSquareRect(r, &myr) - 2;
    int i;
    HBRUSH hbsave, hb, hb2;
    HPEN hpsave, hp, hp2;
    int tri = 290*SmallDiam/1000 - 1;
    int d46, d93;

    /*
     * This fixes a problem with really tiny "scroll" buttons. In particular
     * with the updown control.
     * Making sure that the arrow is as least 3 pixels wide (or high).
     */
    if (tri == 0)
      tri = 1;

    switch(uFlags & 0xff)
    {
    case DFCS_SCROLLCOMBOBOX:
    case DFCS_SCROLLDOWN:
        Line[2].x = myr.left + 470*SmallDiam/1000 + 2;
        Line[2].y = myr.top  + 687*SmallDiam/1000 + 1;
        Line[0].x = Line[2].x - tri;
        Line[1].x = Line[2].x + tri;
        Line[0].y = Line[1].y = Line[2].y - tri;
        break;

    case DFCS_SCROLLUP:
        Line[2].x = myr.left + 470*SmallDiam/1000 + 2;
        Line[2].y = myr.bottom - (687*SmallDiam/1000 + 1);
        Line[0].x = Line[2].x - tri;
        Line[1].x = Line[2].x + tri;
        Line[0].y = Line[1].y = Line[2].y + tri;
        break;

    case DFCS_SCROLLLEFT:
        Line[2].x = myr.right - (687*SmallDiam/1000 + 1);
        Line[2].y = myr.top  + 470*SmallDiam/1000 + 2;
        Line[0].y = Line[2].y - tri;
        Line[1].y = Line[2].y + tri;
        Line[0].x = Line[1].x = Line[2].x + tri;
        break;

    case DFCS_SCROLLRIGHT:
        Line[2].x = myr.left + 687*SmallDiam/1000 + 1;
        Line[2].y = myr.top  + 470*SmallDiam/1000 + 2;
        Line[0].y = Line[2].y - tri;
        Line[1].y = Line[2].y + tri;
        Line[0].x = Line[1].x = Line[2].x - tri;
        break;

    case DFCS_SCROLLSIZEGRIP:
        /* This one breaks the flow... */
        UITOOLS95_DrawRectEdge(dc, r, EDGE_BUMP, BF_MIDDLE | ((uFlags&(DFCS_MONO|DFCS_FLAT)) ? BF_MONO : 0));
        hpsave = (HPEN)SelectObject(dc, GetStockObject(NULL_PEN));
        hbsave = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        if(uFlags & (DFCS_MONO|DFCS_FLAT))
        {
            hp = hp2 = GetSysColorPen(COLOR_WINDOWFRAME);
            hb = hb2 = GetSysColorBrush(COLOR_WINDOWFRAME);
        }
        else
        {
            hp  = GetSysColorPen(COLOR_BTNHIGHLIGHT);
            hp2 = GetSysColorPen(COLOR_BTNSHADOW);
            hb  = GetSysColorBrush(COLOR_BTNHIGHLIGHT);
            hb2 = GetSysColorBrush(COLOR_BTNSHADOW);
        }
        Line[0].x = Line[1].x = r->right-1;
        Line[2].y = Line[3].y = r->bottom-1;
        d46 = 46*SmallDiam/750;
        d93 = 93*SmallDiam/750;

        i = 586*SmallDiam/750;
        Line[0].y = r->bottom - i - 1;
        Line[3].x = r->right - i - 1;
        Line[1].y = Line[0].y + d46;
        Line[2].x = Line[3].x + d46;
        SelectObject(dc, hb);
        SelectObject(dc, hp);
        Polygon(dc, Line, 4);

        Line[1].y++; Line[2].x++;
        Line[0].y = Line[1].y + d93;
        Line[3].x = Line[2].x + d93;
        SelectObject(dc, hb2);
        SelectObject(dc, hp2);
        Polygon(dc, Line, 4);

        i = 398*SmallDiam/750;
        Line[0].y = r->bottom - i - 1;
        Line[3].x = r->right - i - 1;
        Line[1].y = Line[0].y + d46;
        Line[2].x = Line[3].x + d46;
        SelectObject(dc, hb);
        SelectObject(dc, hp);
        Polygon(dc, Line, 4);

        Line[1].y++; Line[2].x++;
        Line[0].y = Line[1].y + d93;
        Line[3].x = Line[2].x + d93;
        SelectObject(dc, hb2);
        SelectObject(dc, hp2);
        Polygon(dc, Line, 4);

        i = 210*SmallDiam/750;
        Line[0].y = r->bottom - i - 1;
        Line[3].x = r->right - i - 1;
        Line[1].y = Line[0].y + d46;
        Line[2].x = Line[3].x + d46;
        SelectObject(dc, hb);
        SelectObject(dc, hp);
        Polygon(dc, Line, 4);

        Line[1].y++; Line[2].x++;
        Line[0].y = Line[1].y + d93;
        Line[3].x = Line[2].x + d93;
        SelectObject(dc, hb2);
        SelectObject(dc, hp2);
        Polygon(dc, Line, 4);

        SelectObject(dc, hpsave);
        SelectObject(dc, hbsave);
        return TRUE;

    default:
        DbgPrint("Invalid scroll; flags=0x%04x\n", uFlags);
        return FALSE;
    }

    /* Here do the real scroll-bar controls end up */
    if( ! (uFlags & (0xff00 & ~DFCS_ADJUSTRECT)) )
      /* UITOOLS95_DFC_ButtonPush always uses BF_SOFT which we don't */
      /* want for the normal scroll-arrow button. */
      UITOOLS95_DrawRectEdge( dc, r, EDGE_RAISED, (uFlags&DFCS_ADJUSTRECT) | BF_MIDDLE | BF_RECT);
    else
      UITOOLS95_DFC_ButtonPush(dc, r, (uFlags & 0xff00) );

    if(uFlags & DFCS_INACTIVE)
    {
        hbsave = (HBRUSH)SelectObject(dc, GetSysColorBrush(COLOR_BTNHIGHLIGHT));
        hpsave = (HPEN)SelectObject(dc, GetSysColorPen(COLOR_BTNHIGHLIGHT));
        Polygon(dc, Line, 3);
        SelectObject(dc, hpsave);
        SelectObject(dc, hbsave);
    }

    if( (uFlags & DFCS_INACTIVE) || !(uFlags & DFCS_PUSHED) )
      for(i = 0; i < 3; i++)
      {
        Line[i].x--;
        Line[i].y--;
      }

    i = uFlags & DFCS_INACTIVE ? COLOR_BTNSHADOW : COLOR_BTNTEXT;
    hbsave = (HBRUSH)SelectObject(dc, GetSysColorBrush(i));
    hpsave = (HPEN)SelectObject(dc, GetSysColorPen(i));
    Polygon(dc, Line, 3);
    SelectObject(dc, hpsave);
    SelectObject(dc, hbsave);

    return TRUE;
}

/* Ported from WINE20020904 */
/* Draw a menu control coming from DrawFrameControl() */
static BOOL UITOOLS95_DrawFrameMenu(HDC dc, LPRECT r, UINT uFlags)
{
    POINT Points[6];
    RECT myr;
    int SmallDiam = UITOOLS_MakeSquareRect(r, &myr);
    int i;
    HBRUSH hbsave;
    HPEN hpsave;
    int xe, ye;
    int xc, yc;
    BOOL retval = TRUE;

    /* Using black and white seems to be utterly wrong, but win95 doesn't */
    /* use anything else. I think I tried all sys-colors to change things */
    /* without luck. It seems as if this behavior is inherited from the */
    /* win31 DFC() implementation... (you remember, B/W menus). */

    FillRect(dc, r, (HBRUSH)GetStockObject(WHITE_BRUSH));

    hbsave = (HBRUSH)SelectObject(dc, GetStockObject(BLACK_BRUSH));
    hpsave = (HPEN)SelectObject(dc, GetStockObject(BLACK_PEN));

    switch(uFlags & 0xff)
    {
    case DFCS_MENUARROW:
        i = 187*SmallDiam/750;
        Points[2].x = myr.left + 468*SmallDiam/750;
        Points[2].y = myr.top  + 352*SmallDiam/750+1;
        Points[0].y = Points[2].y - i;
        Points[1].y = Points[2].y + i;
        Points[0].x = Points[1].x = Points[2].x - i;
        Polygon(dc, Points, 3);
        break;

    case DFCS_MENUBULLET:
        xe = myr.left;
        ye = myr.top  + SmallDiam - SmallDiam/2;
        xc = myr.left + SmallDiam - SmallDiam/2;
        yc = myr.top  + SmallDiam - SmallDiam/2;
        i = 234*SmallDiam/750;
        i = i < 1 ? 1 : i;
        myr.left   = xc - i+i/2;
        myr.right  = xc + i/2;
        myr.top    = yc - i+i/2;
        myr.bottom = yc + i/2;
        Pie(dc, myr.left, myr.top, myr.right, myr.bottom, xe, ye, xe, ye);
        break;

    case DFCS_MENUCHECK:
        Points[0].x = myr.left + 253*SmallDiam/1000;
        Points[0].y = myr.top  + 445*SmallDiam/1000;
        Points[1].x = myr.left + 409*SmallDiam/1000;
        Points[1].y = Points[0].y + (Points[1].x-Points[0].x);
        Points[2].x = myr.left + 690*SmallDiam/1000;
        Points[2].y = Points[1].y - (Points[2].x-Points[1].x);
        Points[3].x = Points[2].x;
        Points[3].y = Points[2].y + 3*SmallDiam/16;
        Points[4].x = Points[1].x;
        Points[4].y = Points[1].y + 3*SmallDiam/16;
        Points[5].x = Points[0].x;
        Points[5].y = Points[0].y + 3*SmallDiam/16;
        Polygon(dc, Points, 6);
        break;

    default:
        DbgPrint("Invalid menu; flags=0x%04x\n", uFlags);
        retval = FALSE;
        break;
    }

    SelectObject(dc, hpsave);
    SelectObject(dc, hbsave);
    return retval;
}

/* Ported from WINE20020904 */
BOOL WINAPI DrawFrameControl( HDC hdc, LPRECT rc, UINT uType,
                                  UINT uState )
{
    /* Win95 doesn't support drawing in other mapping modes */
    if(GetMapMode(hdc) != MM_TEXT)
        return FALSE;

    switch(uType)
    {
    case DFC_BUTTON:
      return UITOOLS95_DrawFrameButton(hdc, rc, uState);
    case DFC_CAPTION:
      return UITOOLS95_DrawFrameCaption(hdc, rc, uState);
    case DFC_MENU:
      return UITOOLS95_DrawFrameMenu(hdc, rc, uState);
    case DFC_SCROLL:
      return UITOOLS95_DrawFrameScroll(hdc, rc, uState);
    default:
      DbgPrint("(%x,%p,%d,%x), bad type!\n", hdc,rc,uType,uState );
    }
    return FALSE;
}

/* Ported from WINE20020904 */
BOOL WINAPI DrawEdge( HDC hdc, LPRECT rc, UINT edge, UINT flags )
{
    DbgPrint("%04x %d,%d-%d,%d %04x %04x\n",
             hdc, rc->left, rc->top, rc->right, rc->bottom, edge, flags );

    if(flags & BF_DIAGONAL)
      return UITOOLS95_DrawDiagEdge(hdc, rc, edge, flags);
    else
      return UITOOLS95_DrawRectEdge(hdc, rc, edge, flags);
}

WINBOOL
STDCALL
GrayStringA(
  HDC hDC,
  HBRUSH hBrush,
  GRAYSTRINGPROC lpOutputFunc,
  LPARAM lpData,
  int nCount,
  int X,
  int Y,
  int nWidth,
  int nHeight)
{
  return FALSE;
}

WINBOOL
STDCALL
GrayStringW(
  HDC hDC,
  HBRUSH hBrush,
  GRAYSTRINGPROC lpOutputFunc,
  LPARAM lpData,
  int nCount,
  int X,
  int Y,
  int nWidth,
  int nHeight)
{
  return FALSE;
}
WINBOOL
STDCALL
InvertRect(
  HDC hDC,
  CONST RECT *lprc)
{
  return FALSE;
}
LONG
STDCALL
TabbedTextOutA(
  HDC hDC,
  int X,
  int Y,
  LPCSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions,
  int nTabOrigin)
{
  return 0;
}

LONG
STDCALL
TabbedTextOutW(
  HDC hDC,
  int X,
  int Y,
  LPCWSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions,
  int nTabOrigin)
{
  return 0;
}
int
STDCALL
FrameRect(
  HDC hDC,
  CONST RECT *lprc,
  HBRUSH hbr)
{
  return 0;
}
WINBOOL
STDCALL
FlashWindow(
  HWND hWnd,
  WINBOOL bInvert)
{
  return FALSE;
}

WINBOOL
STDCALL
FlashWindowEx(
  PFLASHWINFO pfwi)
{
  return FALSE;
}

int STDCALL
FillRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr)
{
  HBRUSH prevhbr;
  /*if (hbr <= (HBRUSH)(COLOR_MAX + 1))
    {
      hbr = GetSysColorBrush((INT)hbr - 1);
      }*/
  if ((prevhbr = SelectObject(hDC, hbr)) == NULL)
    {
      return(FALSE);
    }
  PatBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left,
	 lprc->bottom - lprc->top, PATCOPY);
  SelectObject(hDC, prevhbr);
  return(TRUE);
}

WINBOOL
STDCALL
DrawAnimatedRects(
  HWND hwnd,
  int idAni,
  CONST RECT *lprcFrom,
  CONST RECT *lprcTo)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawCaption(
  HWND hwnd,
  HDC hdc,
  LPRECT lprc,
  UINT uFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawFocusRect(
  HDC hDC,
  CONST RECT *lprc)
{
  return FALSE;
}


WINBOOL
STDCALL
DrawStateA(
  HDC hdc,
  HBRUSH hbr,
  DRAWSTATEPROC lpOutputFunc,
  LPARAM lData,
  WPARAM wData,
  int x,
  int y,
  int cx,
  int cy,
  UINT fuFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawStateW(
  HDC hdc,
  HBRUSH hbr,
  DRAWSTATEPROC lpOutputFunc,
  LPARAM lData,
  WPARAM wData,
  int x,
  int y,
  int cx,
  int cy,
  UINT fuFlags)
{
  return FALSE;
}
