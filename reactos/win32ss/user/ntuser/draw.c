/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Win32ss internal drawing support.
 * FILE:             win32ss/user/ntuser/draw.c
 * PROGRAMER:        
 */

//
//	Original code from Wine see user32/windows/draw.c.
//
//

#include <win32k.h>

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

/* Ported from WINE20020904 */
/* Same as DrawEdge invoked with BF_DIAGONAL */
BOOL FASTCALL IntDrawDiagEdge(HDC hdc, LPRECT rc, UINT uType, UINT uFlags)
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
    OuterPen = InnerPen = (HPEN)NtGdiGetStockObject(NULL_PEN);
    SavePen = (HPEN)NtGdiSelectPen(hdc, InnerPen);
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

    if(InnerI != -1)
        InnerPen = NtGdiGetStockObject(DC_PEN);
    if(OuterI != -1)
        OuterPen = NtGdiGetStockObject(DC_PEN);

    GreMoveTo(hdc, 0, 0, &SavePoint);

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

    GreMoveTo(hdc, spx, spy, NULL);
    NtGdiSelectPen(hdc, OuterPen);
    IntSetDCPenColor(hdc, IntGetSysColor(OuterI));
    NtGdiLineTo(hdc, epx, epy);

    NtGdiSelectPen(hdc, InnerPen);
    IntSetDCPenColor(hdc, IntGetSysColor(InnerI));

    switch(uFlags & (BF_RECT|BF_DIAGONAL))
    {
        case BF_DIAGONAL_ENDBOTTOMLEFT:
        case (BF_DIAGONAL|BF_BOTTOM):
                    case BF_DIAGONAL:
                        case (BF_DIAGONAL|BF_LEFT):
                                GreMoveTo(hdc, spx-1, spy, NULL);
            NtGdiLineTo(hdc, epx, epy-1);
            Points[0].x = spx-add;
            Points[0].y = spy;
            Points[1].x = rc->left;
            Points[1].y = rc->top;
            Points[2].x = epx+1;
            Points[2].y = epy-1-add;
            Points[3] = Points[2];
            break;

        case BF_DIAGONAL_ENDBOTTOMRIGHT:
            GreMoveTo(hdc, spx-1, spy, NULL);
            NtGdiLineTo(hdc, epx, epy+1);
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
                                    GreMoveTo(hdc, spx+1, spy, NULL);
            NtGdiLineTo(hdc, epx, epy+1);
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
            GreMoveTo(hdc, spx, spy-1, NULL);
            NtGdiLineTo(hdc, epx+1, epy);
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
                                GreMoveTo(hdc, spx+1, spy-1, NULL);
            NtGdiLineTo(hdc, epx, epy);
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
                                GreMoveTo(hdc, spx, spy, NULL);
            NtGdiLineTo(hdc, epx-1, epy+1);
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
        hbsave = (HBRUSH)NtGdiSelectBrush(hdc, NtGdiGetStockObject(DC_BRUSH));
        hpsave = (HPEN)NtGdiSelectPen(hdc, NtGdiGetStockObject(DC_PEN));
        IntSetDCBrushColor(hdc, IntGetSysColor(uFlags & BF_MONO ? COLOR_WINDOW : COLOR_BTNFACE));
        IntSetDCPenColor(hdc, IntGetSysColor(uFlags & BF_MONO ? COLOR_WINDOW : COLOR_BTNFACE));
        IntPolygon(hdc, Points, 4);
        NtGdiSelectBrush(hdc, hbsave);
        NtGdiSelectPen(hdc, hpsave);
    }

    /* Adjust rectangle if asked */
    if(uFlags & BF_ADJUST)
    {
        if(uFlags & BF_LEFT)
            rc->left   += add;
        if(uFlags & BF_RIGHT)
            rc->right  -= add;
        if(uFlags & BF_TOP)
            rc->top    += add;
        if(uFlags & BF_BOTTOM)
            rc->bottom -= add;
    }

    /* Cleanup */
    NtGdiSelectPen(hdc, SavePen);
    GreMoveTo(hdc, SavePoint.x, SavePoint.y, NULL);

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
BOOL FASTCALL IntDrawRectEdge(HDC hdc, LPRECT rc, UINT uType, UINT uFlags)
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
    LTInnerPen = LTOuterPen = RBInnerPen = RBOuterPen = (HPEN)NtGdiGetStockObject(NULL_PEN);
    SavePen = (HPEN)NtGdiSelectPen(hdc, LTInnerPen);

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
        if( LTInnerI != -1 )
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

    if((uFlags & BF_BOTTOMLEFT) == BF_BOTTOMLEFT)
        LBpenplus = 1;
    if((uFlags & BF_TOPRIGHT) == BF_TOPRIGHT)
        RTpenplus = 1;
    if((uFlags & BF_BOTTOMRIGHT) == BF_BOTTOMRIGHT)
        RBpenplus = 1;
    if((uFlags & BF_TOPLEFT) == BF_TOPLEFT)
        LTpenplus = 1;

    if(LTInnerI != -1)
        LTInnerPen = NtGdiGetStockObject(DC_PEN);
    if(LTOuterI != -1)
        LTOuterPen = NtGdiGetStockObject(DC_PEN);
    if(RBInnerI != -1)
        RBInnerPen = NtGdiGetStockObject(DC_PEN);
    if(RBOuterI != -1)
        RBOuterPen = NtGdiGetStockObject(DC_PEN);
    if((uFlags & BF_MIDDLE) && retval)
    {
        FillRect(hdc, &InnerRect, IntGetSysColorBrush(uFlags & BF_MONO ?
                 COLOR_WINDOW : COLOR_BTNFACE));
    }
    GreMoveTo(hdc, 0, 0, &SavePoint);

    /* Draw the outer edge */
    NtGdiSelectPen(hdc, LTOuterPen);
    IntSetDCPenColor(hdc, IntGetSysColor(LTOuterI));
    if(uFlags & BF_TOP)
    {
        GreMoveTo(hdc, InnerRect.left, InnerRect.top, NULL);
        NtGdiLineTo(hdc, InnerRect.right, InnerRect.top);
    }
    if(uFlags & BF_LEFT)
    {
        GreMoveTo(hdc, InnerRect.left, InnerRect.top, NULL);
        NtGdiLineTo(hdc, InnerRect.left, InnerRect.bottom);
    }
    NtGdiSelectPen(hdc, RBOuterPen);
    IntSetDCPenColor(hdc, IntGetSysColor(RBOuterI));
    if(uFlags & BF_BOTTOM)
    {
        GreMoveTo(hdc, InnerRect.left, InnerRect.bottom-1, NULL);
        NtGdiLineTo(hdc, InnerRect.right, InnerRect.bottom-1);
    }
    if(uFlags & BF_RIGHT)
    {
        GreMoveTo(hdc, InnerRect.right-1, InnerRect.top, NULL);
        NtGdiLineTo(hdc, InnerRect.right-1, InnerRect.bottom);
    }

    /* Draw the inner edge */
    NtGdiSelectPen(hdc, LTInnerPen);
    IntSetDCPenColor(hdc, IntGetSysColor(LTInnerI));
    if(uFlags & BF_TOP)
    {
        GreMoveTo(hdc, InnerRect.left+LTpenplus, InnerRect.top+1, NULL);
        NtGdiLineTo(hdc, InnerRect.right-RTpenplus, InnerRect.top+1);
    }
    if(uFlags & BF_LEFT)
    {
        GreMoveTo(hdc, InnerRect.left+1, InnerRect.top+LTpenplus, NULL);
        NtGdiLineTo(hdc, InnerRect.left+1, InnerRect.bottom-LBpenplus);
    }
    NtGdiSelectPen(hdc, RBInnerPen);
    IntSetDCPenColor(hdc, IntGetSysColor(RBInnerI));
    if(uFlags & BF_BOTTOM)
    {
        GreMoveTo(hdc, InnerRect.left+LBpenplus, InnerRect.bottom-2, NULL);
        NtGdiLineTo(hdc, InnerRect.right-RBpenplus, InnerRect.bottom-2);
    }
    if(uFlags & BF_RIGHT)
    {
        GreMoveTo(hdc, InnerRect.right-2, InnerRect.top+RTpenplus, NULL);
        NtGdiLineTo(hdc, InnerRect.right-2, InnerRect.bottom-RBpenplus);
    }

    if( ((uFlags & BF_MIDDLE) && retval) || (uFlags & BF_ADJUST) )
    {
        int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0)
                      + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0);

        if(uFlags & BF_LEFT)
            InnerRect.left += add;
        if(uFlags & BF_RIGHT)
            InnerRect.right -= add;
        if(uFlags & BF_TOP)
            InnerRect.top += add;
        if(uFlags & BF_BOTTOM)
            InnerRect.bottom -= add;

        if(uFlags & BF_ADJUST)
            *rc = InnerRect;
    }

    /* Cleanup */
    NtGdiSelectPen(hdc, SavePen);
    GreMoveTo(hdc, SavePoint.x, SavePoint.y, NULL);
    return retval;
}

int FASTCALL UITOOLS_MakeSquareRect(LPRECT src, LPRECT dst)
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
void FASTCALL UITOOLS_DrawCheckedRect( HDC dc, LPRECT rect )
{
    if(IntGetSysColor(COLOR_BTNHIGHLIGHT) == RGB(255, 255, 255))
    {
        HBRUSH hbsave;
        COLORREF bg;

        FillRect(dc, rect, IntGetSysColorBrush(COLOR_BTNFACE));
        bg = IntGdiSetBkColor(dc, RGB(255, 255, 255));
        hbsave = NtGdiSelectBrush(dc, gpsi->hbrGray);
        NtGdiPatBlt(dc, rect->left, rect->top, rect->right-rect->left, rect->bottom-rect->top, 0x00FA0089);
        NtGdiSelectBrush(dc, hbsave);
        IntGdiSetBkColor(dc, bg);
    }
    else
    {
        FillRect(dc, rect, IntGetSysColorBrush(COLOR_BTNHIGHLIGHT));
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
BOOL FASTCALL UITOOLS95_DFC_ButtonPush(HDC dc, LPRECT r, UINT uFlags)
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
            IntDrawRectEdge(dc, &myr, edge, BF_MONO|BF_RECT|BF_ADJUST);
        else
            IntDrawRectEdge(dc, &myr, edge, (uFlags&DFCS_FLAT)|BF_RECT|BF_SOFT|BF_ADJUST);

        UITOOLS_DrawCheckedRect( dc, &myr );
    }
    else
    {
        if(uFlags & DFCS_MONO)
        {
            IntDrawRectEdge(dc, &myr, edge, BF_MONO|BF_RECT|BF_ADJUST);
            FillRect(dc, &myr, IntGetSysColorBrush(COLOR_BTNFACE));
        }
        else
        {
            IntDrawRectEdge(dc, r, edge, (uFlags&DFCS_FLAT) | BF_MIDDLE | BF_RECT | BF_SOFT);
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

BOOL FASTCALL UITOOLS95_DFC_ButtonCheckRadio(HDC dc, LPRECT r, UINT uFlags, BOOL Radio)
{
    LOGFONTW lf;
    HFONT hFont, hOldFont;
    int i;
    WCHAR OutRight, OutLeft, InRight, InLeft, Center;

    if (Radio)
    {
        OutRight = 'j'; // Outer right
        OutLeft  = 'k'; // Outer left
        InRight  = 'l'; // inner left
        InLeft   = 'm'; // inner right
        Center   = 'n'; // center
    } else
    {
        OutRight = 'c'; // Outer right
        OutLeft  = 'd'; // Outer left
        InRight  = 'e'; // inner left
        InLeft   = 'f'; // inner right
        Center   = 'g'; // center
    }

    RtlZeroMemory(&lf, sizeof(LOGFONTW));
    lf.lfHeight = r->top - r->bottom;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    RtlCopyMemory(lf.lfFaceName, L"Marlett", sizeof(L"Marlett"));
    hFont = GreCreateFontIndirectW(&lf);
    hOldFont = NtGdiSelectFont(dc, hFont);

    if(Radio && ((uFlags & 0xff) == DFCS_BUTTONRADIOMASK))
    {
        IntGdiSetBkMode(dc, OPAQUE);
        IntGdiSetTextColor(dc, IntGetSysColor(COLOR_WINDOWFRAME));
        GreTextOutW(dc, r->left, r->top, &Center, 1);
        IntGdiSetBkMode(dc, TRANSPARENT);
        IntGdiSetTextColor(dc, IntGetSysColor(COLOR_WINDOWFRAME));
        GreTextOutW(dc, r->left, r->top, &OutRight, 1);
        IntGdiSetTextColor(dc, IntGetSysColor(COLOR_WINDOWFRAME));
        GreTextOutW(dc, r->left, r->top, &OutLeft, 1);
    }
    else
    {
        IntGdiSetBkMode(dc, TRANSPARENT);

        /* Center section, white for active, grey for inactive */
        i= !(uFlags & (DFCS_INACTIVE|DFCS_PUSHED)) ? COLOR_WINDOW : COLOR_BTNFACE;
        IntGdiSetTextColor(dc, IntGetSysColor(i));
        GreTextOutW(dc, r->left, r->top, &Center, 1);

        if(uFlags & (DFCS_FLAT | DFCS_MONO))
        {
            IntGdiSetTextColor(dc, IntGetSysColor(COLOR_WINDOWFRAME));
            GreTextOutW(dc, r->left, r->top, &OutRight, 1);
            GreTextOutW(dc, r->left, r->top, &OutLeft, 1);
            GreTextOutW(dc, r->left, r->top, &InRight, 1);
            GreTextOutW(dc, r->left, r->top, &InLeft, 1);
        }
        else
        {
            IntGdiSetTextColor(dc, IntGetSysColor(COLOR_BTNSHADOW));
            GreTextOutW(dc, r->left, r->top, &OutRight, 1);
            IntGdiSetTextColor(dc, IntGetSysColor(COLOR_BTNHIGHLIGHT));
            GreTextOutW(dc, r->left, r->top, &OutLeft, 1);
            IntGdiSetTextColor(dc, IntGetSysColor(COLOR_3DDKSHADOW));
            GreTextOutW(dc, r->left, r->top, &InRight, 1);
            IntGdiSetTextColor(dc, IntGetSysColor(COLOR_3DLIGHT));
            GreTextOutW(dc, r->left, r->top, &InLeft, 1);
        }
    }

    if(uFlags & DFCS_CHECKED)
    {
        WCHAR Check = (Radio) ? 'i' : 'b';

        IntGdiSetTextColor(dc, IntGetSysColor(COLOR_WINDOWTEXT));
        GreTextOutW(dc, r->left, r->top, &Check, 1);
    }

    IntGdiSetTextColor(dc, IntGetSysColor(COLOR_WINDOWTEXT));
    NtGdiSelectFont(dc, hOldFont);
    GreDeleteObject(hFont);

    return TRUE;
}

/* Ported from WINE20020904 */
BOOL FASTCALL UITOOLS95_DrawFrameButton(HDC hdc, LPRECT rc, UINT uState)
{
    switch(uState & 0xff)
    {
        case DFCS_BUTTONPUSH:
            return UITOOLS95_DFC_ButtonPush(hdc, rc, uState);

        case DFCS_BUTTONCHECK:
        case DFCS_BUTTON3STATE:
            return UITOOLS95_DFC_ButtonCheckRadio(hdc, rc, uState, FALSE);

        case DFCS_BUTTONRADIOIMAGE:
        case DFCS_BUTTONRADIOMASK:
        case DFCS_BUTTONRADIO:
            return UITOOLS95_DFC_ButtonCheckRadio(hdc, rc, uState, TRUE);

/*
        default:
            DbgPrint("Invalid button state=0x%04x\n", uState);
*/
    }
    return FALSE;
}

BOOL FASTCALL UITOOLS95_DrawFrameCaption(HDC dc, LPRECT r, UINT uFlags)
{
    LOGFONTW lf;
    HFONT hFont, hOldFont;
    COLORREF clrsave;
    RECT myr;
    INT bkmode;
    WCHAR Symbol;
    switch(uFlags & 0xff)
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
             return FALSE;
    }
    IntDrawRectEdge(dc,r,(uFlags&DFCS_PUSHED) ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT | BF_MIDDLE | BF_SOFT);
    RtlZeroMemory(&lf, sizeof(LOGFONTW));
    UITOOLS_MakeSquareRect(r, &myr);
    myr.left += 1;
    myr.top += 1;
    myr.right -= 1;
    myr.bottom -= 1;
    if(uFlags & DFCS_PUSHED)
       RECTL_vOffsetRect(&myr,1,1);
    lf.lfHeight = myr.bottom - myr.top;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    RtlCopyMemory(lf.lfFaceName, L"Marlett", sizeof(L"Marlett"));
    hFont = GreCreateFontIndirectW(&lf);
    /* save font and text color */
    hOldFont = NtGdiSelectFont(dc, hFont);
    clrsave = GreGetTextColor(dc);
    bkmode = GreGetBkMode(dc);
    /* set color and drawing mode */
    IntGdiSetBkMode(dc, TRANSPARENT);
    if(uFlags & DFCS_INACTIVE)
    {
        /* draw shadow */
        IntGdiSetTextColor(dc, IntGetSysColor(COLOR_BTNHIGHLIGHT));
        GreTextOutW(dc, myr.left + 1, myr.top + 1, &Symbol, 1);
    }
    IntGdiSetTextColor(dc, IntGetSysColor((uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT));
    /* draw selected symbol */
    GreTextOutW(dc, myr.left, myr.top, &Symbol, 1);
    /* restore previous settings */
    IntGdiSetTextColor(dc, clrsave);
    NtGdiSelectFont(dc, hOldFont);
    IntGdiSetBkMode(dc, bkmode);
    GreDeleteObject(hFont);
    return TRUE;
}

BOOL FASTCALL UITOOLS95_DrawFrameScroll(HDC dc, LPRECT r, UINT uFlags)
{
    LOGFONTW lf;
    HFONT hFont, hOldFont;
    COLORREF clrsave;
    RECT myr;
    INT bkmode;
    WCHAR Symbol;
    switch(uFlags & 0xff)
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
		RtlZeroMemory(&lf, sizeof(LOGFONTW));
		UITOOLS_MakeSquareRect(r, &myr);
		lf.lfHeight = myr.bottom - myr.top;
		lf.lfWidth = 0;
		lf.lfWeight = FW_NORMAL;
		lf.lfCharSet = DEFAULT_CHARSET;
		RtlCopyMemory(lf.lfFaceName, L"Marlett", sizeof(L"Marlett"));
		hFont = GreCreateFontIndirectW(&lf);
		/* save font and text color */
		hOldFont = NtGdiSelectFont(dc, hFont);
		clrsave = GreGetTextColor(dc);
		bkmode = GreGetBkMode(dc);
		/* set color and drawing mode */
		IntGdiSetBkMode(dc, TRANSPARENT);
		if (!(uFlags & (DFCS_MONO | DFCS_FLAT)))
		{
			IntGdiSetTextColor(dc, IntGetSysColor(COLOR_BTNHIGHLIGHT));
			/* draw selected symbol */
			Symbol = ((uFlags & 0xff) == DFCS_SCROLLSIZEGRIP) ? 'o' : 'x';
			GreTextOutW(dc, myr.left, myr.top, &Symbol, 1);
			IntGdiSetTextColor(dc, IntGetSysColor(COLOR_BTNSHADOW));
		} else
			IntGdiSetTextColor(dc, IntGetSysColor(COLOR_WINDOWFRAME));
		/* draw selected symbol */
		Symbol = ((uFlags & 0xff) == DFCS_SCROLLSIZEGRIP) ? 'p' : 'y';
		GreTextOutW(dc, myr.left, myr.top, &Symbol, 1);
		/* restore previous settings */
		IntGdiSetTextColor(dc, clrsave);
		NtGdiSelectFont(dc, hOldFont);
		IntGdiSetBkMode(dc, bkmode);
		GreDeleteObject(hFont);
            return TRUE;
	default:
            return FALSE;
    }
    IntDrawRectEdge(dc, r, (uFlags & DFCS_PUSHED) ? EDGE_SUNKEN : EDGE_RAISED, (uFlags&DFCS_FLAT) | BF_MIDDLE | BF_RECT);
    RtlZeroMemory(&lf, sizeof(LOGFONTW));
    UITOOLS_MakeSquareRect(r, &myr);
    myr.left += 1;
    myr.top += 1;
    myr.right -= 1;
    myr.bottom -= 1;
    if(uFlags & DFCS_PUSHED)
       RECTL_vOffsetRect(&myr,1,1);
    lf.lfHeight = myr.bottom - myr.top;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    RtlCopyMemory(lf.lfFaceName, L"Marlett", sizeof(L"Marlett"));
    hFont = GreCreateFontIndirectW(&lf);
    /* save font and text color */
    hOldFont = NtGdiSelectFont(dc, hFont);
    clrsave = GreGetTextColor(dc);
    bkmode = GreGetBkMode(dc);
    /* set color and drawing mode */
    IntGdiSetBkMode(dc, TRANSPARENT);
    if(uFlags & DFCS_INACTIVE)
    {
        /* draw shadow */
        IntGdiSetTextColor(dc, IntGetSysColor(COLOR_BTNHIGHLIGHT));
        GreTextOutW(dc, myr.left + 1, myr.top + 1, &Symbol, 1);
    }
    IntGdiSetTextColor(dc, IntGetSysColor((uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT));
    /* draw selected symbol */
    GreTextOutW(dc, myr.left, myr.top, &Symbol, 1);
    /* restore previous settings */
    IntGdiSetTextColor(dc, clrsave);
    NtGdiSelectFont(dc, hOldFont);
    IntGdiSetBkMode(dc, bkmode);
    GreDeleteObject(hFont);
    return TRUE;
}

BOOL FASTCALL UITOOLS95_DrawFrameMenu(HDC dc, LPRECT r, UINT uFlags)
{
    LOGFONTW lf;
    HFONT hFont, hOldFont;
    WCHAR Symbol;
    switch(uFlags & 0xff)
    {
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
            Symbol = 'a';
            break;

        default:
/*
            DbgPrint("Invalid menu; flags=0x%04x\n", uFlags);
*/
            return FALSE;
    }
    /* acquire ressources only if valid menu */
    RtlZeroMemory(&lf, sizeof(LOGFONTW));
    lf.lfHeight = r->bottom - r->top;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    RtlCopyMemory(lf.lfFaceName, L"Marlett", sizeof(L"Marlett"));
    hFont = GreCreateFontIndirectW(&lf);
    /* save font */
    hOldFont = NtGdiSelectFont(dc, hFont);
    // FIXME selecting color doesn't work
#if 0
    if(uFlags & DFCS_INACTIVE)
    {
        /* draw shadow */
        IntGdiSetTextColor(dc, IntGetSysColor(COLOR_BTNHIGHLIGHT));
        GreTextOutW(dc, r->left + 1, r->top + 1, &Symbol, 1);
    }
    IntGdiSetTextColor(dc, IntGetSysColor((uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT));
#endif
    /* draw selected symbol */
    GreTextOutW(dc, r->left, r->top, &Symbol, 1);
    /* restore previous settings */
    NtGdiSelectFont(dc, hOldFont);
    GreDeleteObject(hFont);
    return TRUE;
}

//
//
//	Win32ss API support.
//
//


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
            hbr = IntGetSysColorBrush(PtrToUlong(hbr) - 1);
        
        prevhbr = NtGdiSelectBrush(hDC, hbr);
        if (prevhbr == NULL)
            return (INT)FALSE;
    }

    Ret = NtGdiPatBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left,
                 lprc->bottom - lprc->top, PATCOPY);

    /* Select old brush */
    if (prevhbr)
        NtGdiSelectBrush(hDC, prevhbr);

    return (INT)Ret;
}

BOOL WINAPI
DrawEdge(HDC hDC, LPRECT rc, UINT edge, UINT flags)
{
    if (flags & BF_DIAGONAL)
       return IntDrawDiagEdge(hDC, rc, edge, flags);
    else
       return IntDrawRectEdge(hDC, rc, edge, flags);
}

BOOL WINAPI
DrawFrameControl(HDC hDC, LPRECT rc, UINT uType, UINT uState)
{
    if (GreGetMapMode(hDC) != MM_TEXT)
        return FALSE;

    switch(uType)
    {
        case DFC_BUTTON:
            return UITOOLS95_DrawFrameButton(hDC, rc, uState);
        case DFC_CAPTION:
            return UITOOLS95_DrawFrameCaption(hDC, rc, uState);
        case DFC_MENU:
            return UITOOLS95_DrawFrameMenu(hDC, rc, uState);
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

