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
/* $Id: draw.c,v 1.30 2003/12/08 20:58:44 chorns Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/*
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 2002 Bill Medland
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>

// Needed for DrawState
#include <string.h>
#include <unicode.h>
#include <draw.h>

#define NDEBUG
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
/*	if( TWEAK_WineLook == WIN98_LOOK && LTInnerI != -1 ) */
            LTInnerI = RBInnerI = COLOR_BTNFACE;
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
    if((uFlags & BF_MIDDLE) && retval)
	{
            FillRect(hdc, &InnerRect, GetSysColorBrush(uFlags & BF_MONO ?
                         COLOR_WINDOW : COLOR_BTNFACE));
	}
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
        MoveToEx(hdc, InnerRect.left, InnerRect.bottom-1, NULL);
        LineTo(hdc, InnerRect.right, InnerRect.bottom-1);
    }
    if(uFlags & BF_RIGHT)
    {
        MoveToEx(hdc, InnerRect.right-1, InnerRect.top, NULL);
        LineTo(hdc, InnerRect.right-1, InnerRect.bottom);
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
        MoveToEx(hdc, InnerRect.left+LBpenplus, InnerRect.bottom-2, NULL);
        LineTo(hdc, InnerRect.right-RBpenplus, InnerRect.bottom-2);
    }
    if(uFlags & BF_RIGHT)
    {
        MoveToEx(hdc, InnerRect.right-2, InnerRect.top+RTpenplus, NULL);
        LineTo(hdc, InnerRect.right-2, InnerRect.bottom-RBpenplus);
    }

    if( ((uFlags & BF_MIDDLE) && retval) || (uFlags & BF_ADJUST) )
    {
        int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0)
                + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0);

        if(uFlags & BF_LEFT)   InnerRect.left   += add;
        if(uFlags & BF_RIGHT)  InnerRect.right  -= add;
        if(uFlags & BF_TOP)    InnerRect.top    += add;
        if(uFlags & BF_BOTTOM) InnerRect.bottom -= add;

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

  if(uFlags & DFCS_PUSHED)
      UITOOLS95_DrawRectEdge(dc,r,EDGE_SUNKEN, BF_RECT | BF_MIDDLE | BF_SOFT);
    else
      UITOOLS95_DrawRectEdge(dc,r,BDR_RAISEDINNER | BDR_RAISEDOUTER, BF_RECT |
                               BF_SOFT | BF_MIDDLE);

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
   /* Win95 doesn't support drawing in other mapping modes
    if(GetMapMode(hdc) != MM_TEXT)
        return FALSE;
    */
    switch(uType)
    {
    case DFC_BUTTON:
      return UITOOLS95_DrawFrameButton(hdc, rc, uState);
    case DFC_CAPTION:
      return UITOOLS95_DrawFrameCaption(hdc, rc, uState);
    case DFC_MENU:
      return UITOOLS95_DrawFrameMenu(hdc, rc, uState);
    /*
    case DFC_POPUPMENU:
      break;
    */
    case DFC_SCROLL:
      return UITOOLS95_DrawFrameScroll(hdc, rc, uState);
    default:
      DbgPrint("(%p,%p,%d,%x), bad type!\n", hdc,rc,uType,uState );
    }
    return FALSE;
}
/* Ported from WINE20020904 */
BOOL WINAPI DrawEdge( HDC hdc, LPRECT rc, UINT edge, UINT flags )
{
    if(flags & BF_DIAGONAL)
      return UITOOLS95_DrawDiagEdge(hdc, rc, edge, flags);
    else
      return UITOOLS95_DrawRectEdge(hdc, rc, edge, flags);
}


WINBOOL
STDCALL
INTERNAL_GrayString(
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
    HDC     MemDC = NULL;
    HBITMAP MemBMP = NULL,
            OldBMP = NULL;
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
            nCount = strlen((CHAR*)lpData);
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

        PatBlt(MemDC, 0, 0, nWidth, nHeight, PATCOPY);
//      This is how WINE does it: (but we should have our own graying brush already)
//        hbsave = (HBRUSH)SelectObject(memdc, CACHE_GetPattern55AABrush());
//        PatBlt(memdc, 0, 0, cx, cy, 0x000A0329);
//        SelectObject(memdc, hbsave);
    }

    if (! BitBlt(hDC, X, Y, nWidth, nHeight, MemDC, 0, 0, SRCCOPY)) goto cleanup;

    cleanup :
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


/*
 * @implemented
 */
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
  return INTERNAL_GrayString(hDC, hBrush, lpOutputFunc, lpData, nCount, X, Y, nWidth, nHeight, FALSE);
}


/*
 * @implemented
 */
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
  return INTERNAL_GrayString(hDC, hBrush, lpOutputFunc, lpData, nCount, X, Y, nWidth, nHeight, TRUE);
}


/*
 * @implemented
 */
WINBOOL
STDCALL
InvertRect(
  HDC hDC,
  CONST RECT *lprc)
{
  
  return PatBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left,
  			lprc->bottom - lprc->top, DSTINVERT);
}


/*
 * @implemented
 */
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
  LONG ret;
  DWORD len;
  LPWSTR strW;

  len = MultiByteToWideChar(CP_ACP, 0, lpString, nCount, NULL, 0);
  strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
  if (!strW)
    {
	  return 0;
    }
  MultiByteToWideChar(CP_ACP, 0, lpString, nCount, strW, len);
  ret = TabbedTextOutW(hDC, X, Y, strW, len, nTabPositions, lpnTabStopPositions, nTabOrigin);
  HeapFree(GetProcessHeap(), 0, strW);
  return ret;
}


/***********************************************************************
 *           TEXT_TabbedTextOut
 *
 * Helper function for TabbedTextOut() and GetTabbedTextExtent().
 * Note: this doesn't work too well for text-alignment modes other
 *       than TA_LEFT|TA_TOP. But we want bug-for-bug compatibility :-)
 */
static LONG TEXT_TabbedTextOut( HDC hdc, INT x, INT y, LPCWSTR lpstr,
                                INT count, INT cTabStops, const INT *lpTabPos, INT nTabOrg,
                                BOOL fDisplayText )
{
    INT defWidth;
    SIZE extent;
    int i, tabPos = x;
    int start = x;

    extent.cx = 0;
    extent.cy = 0;

    if (!lpTabPos)
        cTabStops=0;

    if (cTabStops == 1 && *lpTabPos >= /* sic */ 0)
    {
        defWidth = *lpTabPos;
        cTabStops = 0;
    }
    else
    {
        TEXTMETRICA tm;
        GetTextMetricsA( hdc, &tm );
        defWidth = 8 * tm.tmAveCharWidth;
        if (cTabStops == 1)
            cTabStops = 0; /* on negative *lpTabPos */
    }

    while (count > 0)
    {
        for (i = 0; i < count; i++)
            if (lpstr[i] == '\t') break;
        GetTextExtentPointW( hdc, lpstr, i, &extent );
        while ((cTabStops > 0) &&
               (nTabOrg + *lpTabPos <= x + extent.cx))
        {
            lpTabPos++;
            cTabStops--;
        }
        if (i == count)
            tabPos = x + extent.cx;
        else if (cTabStops > 0)
            tabPos = nTabOrg + *lpTabPos;
        else if (defWidth <= 0)
            tabPos = x + extent.cx;
        else
            tabPos = nTabOrg + ((x + extent.cx - nTabOrg) / defWidth + 1) * defWidth;
        if (fDisplayText)
        {
            RECT r;
            r.left   = x;
            r.top    = y;
            r.right  = tabPos;
            r.bottom = y + extent.cy;
            ExtTextOutW( hdc, x, y, GetBkMode(hdc) == OPAQUE ? ETO_OPAQUE : 0,
                         &r, lpstr, i, NULL );
        }
        x = tabPos;
        count -= i+1;
        lpstr += i+1;
    }
    return MAKELONG(tabPos - start, extent.cy);
}


/*
 * @implemented
 */
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
  return TEXT_TabbedTextOut(hDC, X, Y, lpString, nCount, nTabPositions, lpnTabStopPositions, nTabOrigin, TRUE);
}


/*
 * @implemented
 */
int
STDCALL
FrameRect(
  HDC hDC,
  CONST RECT *lprc,
  HBRUSH hbr)
{
HBRUSH oldbrush;
RECT r = *lprc;

	if ( (r.right <= r.left) || (r.bottom <= r.top) ) return 0;
	if (!(oldbrush = SelectObject( hDC, hbr ))) return 0;
        
	PatBlt( hDC, r.left, r.top, 1, r.bottom - r.top, PATCOPY );
	PatBlt( hDC, r.right - 1, r.top, 1, r.bottom - r.top, PATCOPY );
	PatBlt( hDC, r.left, r.top, r.right - r.left, 1, PATCOPY );
	PatBlt( hDC, r.left, r.bottom - 1, r.right - r.left, 1, PATCOPY );
                                                                            
	SelectObject( hDC, oldbrush );
	return TRUE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
FlashWindow(
  HWND hWnd,
  WINBOOL bInvert)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
FlashWindowEx(
  PFLASHWINFO pfwi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
int STDCALL
FillRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr)
{
  HBRUSH prevhbr;
  if ((prevhbr = SelectObject(hDC, hbr)) == NULL)
    {
      return(FALSE);
    }
  PatBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left,
	 lprc->bottom - lprc->top, PATCOPY);
  SelectObject(hDC, prevhbr);
  return(TRUE);
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
DrawAnimatedRects(
  HWND hwnd,
  int idAni,
  CONST RECT *lprcFrom,
  CONST RECT *lprcTo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
DrawFocusRect(
  HDC hdc,
  CONST RECT *rect)
{
#if 0 /* FIXME PS_ALTERNATE and R2_XORPEN not yet implemented */
  HBRUSH hOldBrush;
  HPEN hOldPen, hNewPen;
  INT oldDrawMode, oldBkMode;

  hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
  hNewPen = CreatePen(PS_ALTERNATE, 1, GetSysColor(COLOR_WINDOWTEXT));
  hOldPen = SelectObject(hdc, hNewPen);
  oldDrawMode = SetROP2(hdc, R2_XORPEN);
  oldBkMode = SetBkMode(hdc, TRANSPARENT);

  Rectangle(hdc, rect->left, rect->top, rect->right, rect->bottom);

  SetBkMode(hdc, oldBkMode);
  SetROP2(hdc, oldDrawMode);
  SelectObject(hdc, hOldPen);
  DeleteObject(hNewPen);
  SelectObject(hdc, hOldBrush);

  return TRUE;
#else
  HBRUSH hbrush = SelectObject(hdc, GetStockObject(GRAY_BRUSH));
  PatBlt(hdc, rect->left, rect->top,
	 rect->right - rect->left - 1, 1, PATINVERT);
  PatBlt(hdc, rect->left, rect->top + 1, 1,
	 rect->bottom - rect->top - 1, PATINVERT);
  PatBlt(hdc, rect->left + 1, rect->bottom - 1,
	 rect->right - rect->left - 1, -1, PATINVERT);
  PatBlt(hdc, rect->right - 1, rect->top, -1,
	 rect->bottom - rect->top - 1, PATINVERT);
  SelectObject(hdc, hbrush);

  return TRUE;
#endif
}


// These are internal functions, based on the WINE sources. Currently they
// are only implemented for DSS_NORMAL and DST_TEXT, although some handling
// for DST_BITMAP has been included.

WINBOOL INTERNAL_DrawStateDraw(HDC hdc, UINT type, DRAWSTATEPROC lpOutputFunc,
                        LPARAM lData, WPARAM wData, LPRECT rc, UINT dtflags,
                        BOOL unicode)
{
//    HDC MemDC;
//    HBITMAP MemBMP;
    BOOL retval = FALSE;
    INT cx = rc->right - rc->left;
    INT cy = rc->bottom - rc->top;

//  Is this supposed to happen?
//    if (((type == DST_TEXT) || (type == DST_PREFIXTEXT)) && (lpOutputFunc))
//        type = DST_COMPLEX;

    switch(type)
    {
        case DST_TEXT :
        case DST_PREFIXTEXT :
        {
            DbgPrint("Drawing DST_TEXT\n");
            if (unicode)
                return DrawTextW(hdc, (LPWSTR)lData, (INT)wData, rc, dtflags);
            else
                return DrawTextA(hdc, (LPSTR)lData, (INT)wData, rc, dtflags);
        }
        
        case DST_ICON :
        {
            // TODO
            DbgPrint("Drawing DST_ICON\n");
            return retval;
        }
        
        case DST_BITMAP :
        {
            // TODO
            DbgPrint("Drawing DST_BITMAP\n");
            return retval;
        }
        
        case DST_COMPLEX :
        {
            DbgPrint("Drawing DST_COMPLEX\n");
            // Call lpOutputFunc, if necessary
            if (lpOutputFunc)
            {
                // Something seems to be wrong with OffsetViewportOrgEx:
                OffsetViewportOrgEx(hdc, rc->left, rc->top, NULL);
                DbgPrint("Calling lpOutputFunc(0x%x, 0x%x, 0x%x, %d, %d)\n", hdc, lData, wData, cx, cy);
                retval = lpOutputFunc(hdc, lData, wData, cx, cy);
                OffsetViewportOrgEx(hdc, -rc->left, -rc->top, NULL);
                return retval;
            }
            else
                return FALSE;
        }
    }
    
    return FALSE;
}


WINBOOL INTERNAL_DrawState(
  HDC hdc,
  HBRUSH hbr,
  DRAWSTATEPROC lpOutputFunc,
  LPARAM lData,
  WPARAM wData,
  int x,
  int y,
  int cx,
  int cy,
  UINT fuFlags,
  BOOL unicode)
{
    UINT type;
    UINT state;
    INT len;
    RECT rect;
    UINT dtflags = DT_NOCLIP;           // Flags for DrawText
    BOOL retval = FALSE;                // Return value

    COLORREF ForeColor,                 // Foreground color
             BackColor;                 // Background color

    HDC MemDC = NULL;                   // Memory DC
    HBITMAP MemBMP = NULL,              // Memory bitmap (for MemDC)
            OldBMP = NULL;              // Old memory bitmap (for MemDC)
    HFONT   Font = NULL;                // Old font (for MemDC)
    HBRUSH  OldBrush = NULL,            // Old brush (for MemDC)
            TempBrush = NULL;           // Temporary brush (for MemDC)

    // AG: Experimental, unfinished, and most likely buggy! I haven't been
    // able to test this - my intention was to implement some things needed
    // by the button control.

    if ((! lpOutputFunc) && (fuFlags & DST_COMPLEX))
        return FALSE;

    type = fuFlags & 0xf;          // DST_xxx
    state = fuFlags & 0x7ff0;      // DSS_xxx
    len = wData;                    // Data length

    DbgPrint("Entered DrawState, fuFlags %d, type %d, state %d\n", fuFlags, type, state);

    if ((type == DST_TEXT || type == DST_PREFIXTEXT) && ! len)
    {
        // The string is NULL-terminated
        if (unicode)
            len = lstrlenW((LPWSTR) lData);
        else
            len = strlen((LPSTR) lData);
    }
    
    // Identify the image size if not specified
    if (!cx || !cy)
    {
        SIZE s;
//        CURSORICONINFO *ici;
        BITMAP bm;
        
        switch(type)  // TODO
        {
            case DST_TEXT :
            case DST_PREFIXTEXT :
            {
                BOOL success;

                DbgPrint("Calculating rect of DST_TEXT / DST_PREFIXTEXT\n");
            
                if (unicode)
                    success = GetTextExtentPoint32W(hdc, (LPWSTR) lData, len, &s);
                else
                    success = GetTextExtentPoint32A(hdc, (LPSTR) lData, len, &s);

                if (!success) return FALSE;
                break;
            }

            case DST_ICON :
            {
                DbgPrint("Calculating rect of DST_ICON\n");
                // TODO
                break;
            }

            case DST_BITMAP :
            {
                DbgPrint("Calculating rect of DST_BITMAP\n");

                if (!GetObjectA((HBITMAP) lData, sizeof(bm), &bm))
                    return FALSE;
                
                s.cx = bm.bmWidth;
                s.cy = bm.bmHeight;
                break;
            }

            case DST_COMPLEX :  // cx and cy must be set in this mode
                DbgPrint("Calculating rect of DST_COMPLEX - Not allowed!\n");
                return FALSE;
        }
        
        if (! cx) cx = s.cx;
        if (! cy) cy = s.cy;
    }

    // Flags for DrawText
    if (fuFlags & DSS_RIGHT)  // Undocumented
        dtflags |= DT_RIGHT;
    if (type == DST_TEXT)
        dtflags |= DT_NOPREFIX;
        
    // No additional processing needed for DSS_NORMAL
    if (state == DSS_NORMAL)
    {
        DbgPrint("DSS_NORMAL (no additional processing necessary)\n");
        SetRect(&rect, x, y, x + cx, y + cy);
        return INTERNAL_DrawStateDraw(hdc, type, lpOutputFunc, lData, wData, &rect, dtflags, unicode);
    }
    
    // WARNING: FROM THIS POINT ON THE CODE IS VERY BUGGY

    // Set the rectangle to that of the memory DC
    SetRect(&rect, 0, 0, cx, cy);

    // Set colors
    ForeColor = SetTextColor(hdc, RGB(0, 0, 0));
    BackColor = SetBkColor(hdc, RGB(255, 255, 255));

    // Create and initialize the memory DC
    MemDC = CreateCompatibleDC(hdc);
    if (! MemDC) goto cleanup;
    MemBMP = CreateBitmap(cx, cy, 1, 1, NULL);
    if (! MemBMP) goto cleanup;
    OldBMP = (HBITMAP) SelectObject(MemDC, MemBMP);
    if (! OldBMP) goto cleanup;

    DbgPrint("Created and inited MemDC\n");

    // Set up the default colors and font
    if (! FillRect(MemDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH))) goto cleanup;
    SetBkColor(MemDC, RGB(255, 255, 255));
    SetTextColor(MemDC, RGB(0, 0, 0));
    Font = (HFONT)SelectObject(MemDC, GetCurrentObject(hdc, OBJ_FONT));

    DbgPrint("Selected font and set colors\n");

    // Enable this line to use the current DC image to begin with (wrong?)
//    if (! BitBlt(MemDC, 0, 0, cx, cy, hdc, x, y, SRCCOPY)) goto cleanup;

    // DST_COMPLEX may draw text as well, so make sure font is selected
    if (! Font && (type <= DST_PREFIXTEXT)) // THIS FAILS
      goto cleanup;

    {
      BOOL TempResult = INTERNAL_DrawStateDraw(MemDC, type, lpOutputFunc, lData, wData, &rect, dtflags, unicode);
      if (Font) SelectObject(MemDC, Font);
      if (! TempResult) goto cleanup;
    }

    DbgPrint("Done drawing\n");

    // Apply state(s?)
    if (state & DSS_UNION)
    {
        DbgPrint("DSS_UNION\n");
        // Dither the image (not implemented in ReactOS yet?)
        // TODO
    }

    // Prepare shadow brush
    if (state & DSS_DISABLED)
        TempBrush = CreateSolidBrush(GetSysColor(COLOR_3DHILIGHT));
    // else if (state & DSS_DEFAULT)
    // TempBrush = CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));

    // Draw shadow
    if (state & (DSS_DISABLED /*|DSS_DEFAULT*/))
    {
        DbgPrint("DSS_DISABLED - Drawing shadow\n");
        if (! TempBrush) goto cleanup;
        OldBrush = (HBRUSH)SelectObject(hdc, TempBrush);
        if (! OldBrush) goto cleanup;
        if (! BitBlt(hdc, x + 1, y + 1, cx, cy, MemDC, 0, 0, 0x00B8074A)) goto cleanup;
        SelectObject(hdc, OldBrush);
        DeleteObject(TempBrush);
        TempBrush = NULL;
    }

    if (state & DSS_DISABLED)
    {
        DbgPrint("DSS_DISABLED - Creating shadow brush 2\n");
        hbr = TempBrush = CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
        if (! TempBrush) goto cleanup;
    }
    else if (! hbr)
    {
        DbgPrint("Creating a brush\n");
        hbr = (HBRUSH) GetStockObject(BLACK_BRUSH);
    }
    
    DbgPrint("Selecting new brush\n");
    OldBrush = (HBRUSH) SelectObject(hdc, hbr);

    // Copy to hdc from MemDC    
    DbgPrint("Blitting\n");
    if (! BitBlt(hdc, x, y, cx, cy, MemDC, 0, 0, 0x00B8074A)) goto cleanup;
    
    retval = TRUE;

    
    cleanup :
        DbgPrint("In cleanup : Font %x  OldBrush %x  OldBMP %x  Tempbrush %x  MemBMP %x  MemDC %x\n",
                  Font, OldBrush, OldBMP, TempBrush, MemBMP, MemDC);
        SetTextColor(hdc, ForeColor);
        SetBkColor(hdc, BackColor);
        if (OldBrush)   SelectObject(MemDC, OldBrush);
        if (OldBMP)     SelectObject(MemDC, OldBMP);
        if (TempBrush)  DeleteObject(TempBrush);
        if (MemBMP)     DeleteObject(MemBMP);
        if (MemDC)      DeleteDC(MemDC);
        
        DbgPrint("Leaving DrawState() with retval %d\n", retval);
        
        return retval;
}


/*
 * @implemented
 */
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
    return INTERNAL_DrawState(hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, fuFlags, FALSE);
}


/*
 * @implemented
 */
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
    return INTERNAL_DrawState(hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, fuFlags, TRUE);
}
