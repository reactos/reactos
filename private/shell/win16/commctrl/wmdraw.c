#include "ctlspriv.h"
// module stolen from user win 4.0 to get drawing functions for
// win31 comctl31.dll

// WIN31: helper stuff from user.h to make me build

/* DrawFrame() Commands */
#define DF_SHIFT0       0x0000
#define DF_SHIFT1       0x0001
#define DF_SHIFT2       0x0002
#define DF_SHIFT3       0x0003
#define DF_PATCOPY      0x0000
#define DF_PATINVERT    0x0004

#define DF_SCROLLBAR        (COLOR_SCROLLBAR << 3)
#define DF_BACKGROUND       (COLOR_DESKTOP << 3)
#define DF_ACTIVECAPTION    (COLOR_ACTIVECAPTION << 3)
#define DF_INACTIVECAPTION  (COLOR_INACTIVECAPTION << 3)
#define DF_MENU                 (COLOR_MENU << 3)
#define DF_WINDOW               (COLOR_WINDOW << 3)
#define DF_WINDOWFRAME      (COLOR_WINDOWFRAME << 3)
#define DF_MENUTEXT             (COLOR_MENUTEXT << 3)
#define DF_WINDOWTEXT       (COLOR_WINDOWTEXT << 3)
#define DF_CAPTIONTEXT      (COLOR_CAPTIONTEXT << 3)
#define DF_ACTIVEBORDER     (COLOR_ACTIVEBORDER << 3)
#define DF_INACTIVEBORDER   (COLOR_INACTIVEBORDER << 3)
#define DF_APPWORKSPACE     (COLOR_APPWORKSPACE << 3)
#define DF_3DSHADOW         (COLOR_3DSHADOW << 3)
#define DF_3DFACE           (COLOR_3DFACE << 3)
#define DF_GRAY             (COLOR_MAX << 3)

// WIN31: end helper stuff

void FAR DrawDiagonal(HDC hdc, LPRECT lprc, HBRUSH hbrTL, HBRUSH hbrBR, UINT flags);

#if 0
BOOL FAR BitBltSysBmp(HDC hdc, int x, int y, WORD i, WORD s)
{
    return(BitBlt(hdc, x, y, oemInfo.bm[i].cx, oemInfo.bm[i].cy, hdcBits, oemInfo.bm[i].cx * s, oemInfo.bm[i].dy, SRCCOPY));
}
#endif


#ifndef WIN31
// --------------------------------------------------------------------------
//
//  BitBltSysBmp()
//
// --------------------------------------------------------------------------
BOOL FAR BitBltSysBmp(HDC hdc, int x, int y, WORD i)
{
    POEMBITMAPINFO pOem = oemInfo.bm + i;
    return(BitBlt(hdc, x, y, pOem->cx, pOem->cy, hdcBits, pOem->x, pOem->y, SRCCOPY));
}
#endif // !WIN31


#ifndef WIN31
// --------------------------------------------------------------------------
//
//  DrawFrame()
//
//  Command bits:
//      Shift count for CXBORDER,CYBORDER
//      PatCopy/PatInvert
//      Color index
//
// --------------------------------------------------------------------------
#define DF_SHIFTMASK (DF_SHIFT0 | DF_SHIFT1 | DF_SHIFT2 | DF_SHIFT3)
#define DF_ROPMASK   (DF_PATCOPY | DF_PATINVERT)
#define DF_HBRMASK   ~(DF_SHIFTMASK | DF_ROPMASK)

void API DrawFrame(HDC hdc, LPRECT lprc, int clFrame, int cmd)
{
    int     x;
    int     y;
    int     cx;
    int     cy;
    int     cxWidth;
    int     cyWidth;
    int     ibr;
    HBRUSH  hbrSave;
    LONG    rop;

    x = lprc->left;
    y = lprc->top;

    cxWidth = CXBORDER * clFrame;
    cyWidth = CYBORDER * clFrame;

    cx = lprc->right - x - cxWidth;
    cy = lprc->bottom - y - cyWidth;

    rop = ((cmd & DF_ROPMASK) ? PATINVERT : PATCOPY);

    ibr = (cmd & DF_HBRMASK) >> 3;
    if (ibr == (DF_GRAY >> 3))
        hbrSave = hbrGray;
    else
        hbrSave = ahbrSystem[ibr];

    // We need to unrealize the object in order to ensure it gets realigned
    // with the DC's origin.
    UnrealizeObject(hbrSave);
    hbrSave = SelectBrush(hdc, hbrSave);

    //
    // Do the PatBlts.
    //
    PatBlt(hdc, x,           y,           cxWidth, cy,      rop);
    PatBlt(hdc, x + cxWidth, y,           cx,      cyWidth, rop);
    PatBlt(hdc, x,           y + cy,      cx,      cyWidth, rop);
    PatBlt(hdc, x + cx,      y + cyWidth, cxWidth, cy,      rop);

    SelectBrush(hdc, hbrSave);
}
#endif // !WIN31


#ifndef WIN31
// ----------------------------------------------------------------------------
//
//  IsSysFontAndDefaultMode()
//
//  Returns TRUE if font selected into DC is the system font AND the current
//  mapping mode of the DC is MM_TEXT (Default mode); else returns FALSE. This
//  is called by interrupt time code so it needs to be in the fixed code
//  segment.
//
// ----------------------------------------------------------------------------
BOOL FAR IsSysFontAndDefaultMode(HDC hdc)
{
    return((GetCurLogFont(hdc) == hFontSys) && (GetMapMode(hdc) == MM_TEXT));
}
#endif //!WIN31


//+-------------------------------------------------------------------------
//
//  Function:		IDrawEdge
//
//  Synopsis:		Draws a 3D edge using 2 3D borders
//
//  Effects:		Adjusts interior rectangle if desired
//					And fills it if requested
//
//  Returns:		FALSE if error
//
//  History:		30-January-91		Laurabu		Created.
//
//--------------------------------------------------------------------------
BOOL API DrawEdge(HDC hdc, LPRECT lprc, UINT edge, UINT flags)
{
    HBRUSH  hbrTL;
    HBRUSH  hbrBR;
    HBRUSH  hbrT = NULL;
    RECT    rc;
    UINT    bdrType;

    //
    // Enforce monochromicity and flatness
    //
#ifndef WIN31
    if (oemInfo.BitCount == 1)
#else
    if (g_oemInfo_BitCount == 1)
#endif
        flags |= BF_MONO;
    if (flags & BF_MONO)
        flags |= BF_FLAT;

    CopyRect(&rc, lprc);

    //
    // Draw the border segment(s), and calculate the remaining space as we
    // go.
    //
    if (bdrType = (edge & BDR_OUTER))
    {
DrawBorder:
        //
        // Get brushes.  Note the symmetry between raised outer, sunken inner and
        // sunken outer, raised inner.
        //

        if (flags & BF_FLAT)
        {
            if (flags & BF_MONO)
                hbrBR = (bdrType & BDR_OUTER) ? HBR_WINDOWFRAME : HBR_WINDOW;
            else
                hbrBR = (bdrType & BDR_OUTER) ? HBR_3DSHADOW : HBR_3DFACE;
            
            hbrTL = hbrBR;
        }
        else
        {
            // 5 == HILIGHT
            // 4 == LIGHT
            // 3 == FACE
            // 2 == SHADOW
            // 1 == DKSHADOW

            switch (bdrType)
            {
                // +2 above surface
                case BDR_RAISEDOUTER:           // 5 : 4
                    hbrTL = ((flags & BF_SOFT) ? HBR_3DHILIGHT : HBR_3DLIGHT);
                    hbrBR = HBR_3DDKSHADOW;     // 1
                    break;

                // +1 above surface
                case BDR_RAISEDINNER:           // 4 : 5
                    hbrTL = ((flags & BF_SOFT) ? HBR_3DLIGHT : HBR_3DHILIGHT);
                    hbrBR = HBR_3DSHADOW;       // 2
                    break;

                // -1 below surface
                case BDR_SUNKENOUTER:           // 1 : 2
                    hbrTL = ((flags & BF_SOFT) ? HBR_3DDKSHADOW : HBR_3DSHADOW);
                    hbrBR = HBR_3DHILIGHT;      // 5
                    break;

                // -2 below surface
                case BDR_SUNKENINNER:           // 2 : 1
                    hbrTL = ((flags & BF_SOFT) ? HBR_3DSHADOW : HBR_3DDKSHADOW);
                    hbrBR = HBR_3DLIGHT;        // 4
                    break;

                default:
#ifndef WIN31
                    DebugErr(DBF_ERROR, "DrawEdge:  Undefined border type");
#endif
                    return(FALSE);
            }
        }

        //
        // Draw the sides of the border.  NOTE THAT THE ALGORITHM FAVORS THE
        // BOTTOM AND RIGHT SIDES, since the light source is assumed to be top
        // left.  If we ever decide to let the user set the light source to a
        // particular corner, then change this algorithm.
        //

        if (flags & BF_DIAGONAL)
            DrawDiagonal(hdc, &rc, hbrTL, hbrBR, flags);
        else
        {
            // Bottom Right edges
            if (flags & (BF_RIGHT | BF_BOTTOM))
            {
                if (hbrT)
                    SelectBrush(hdc, hbrBR);
                else
                    hbrT = SelectBrush(hdc, hbrBR);

                // Right
                if (flags & BF_RIGHT)
                {       
                    rc.right -= CXBORDER;
                    PatBlt(hdc, rc.right, rc.top, CXBORDER, rc.bottom - rc.top, PATCOPY);
                }

                // Bottom
                if (flags & BF_BOTTOM)
                {
                    rc.bottom -= CYBORDER;
                    PatBlt(hdc, rc.left, rc.bottom, rc.right - rc.left, CYBORDER, PATCOPY);
                }
            }

            // Top Left edges
            if (flags & (BF_TOP | BF_LEFT))
            {
                if (hbrT)
                    SelectBrush(hdc, hbrTL);
                else
                    hbrT = SelectBrush(hdc, hbrTL);
            
                // Left
                if (flags & BF_LEFT)
                {
                    PatBlt(hdc, rc.left, rc.top, CXBORDER, rc.bottom - rc.top, PATCOPY);
                    rc.left += CXBORDER;
                }

                // Top
                if (flags & BF_TOP)
                {
                    PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, CYBORDER, PATCOPY);
                    rc.top += CYBORDER;
                }
            }
        }
    }

    if (bdrType = (edge & BDR_INNER))
    {
        //
        // Strip this so the next time through, bdrType will be 0.
        // Otherwise, we'll loop forever.
        //
        edge &= ~BDR_INNER;
        goto DrawBorder;
    }


    //
    // Select old brush back in, if we changed it.
    //
    if (hbrT)
        SelectBrush(hdc, hbrT);

    //
    // Fill the middle & clean up if asked
    // These are pointless if BF_DIAGONAL!
    //
    if (flags & BF_MIDDLE)
        FillRect(hdc, &rc, (flags & BF_MONO) ? HBR_WINDOW : HBR_3DFACE);

    if (flags & BF_ADJUST)
        CopyRect(lprc, &rc);

    return(TRUE);
}


//+--------------------------------------------------------------------------
//
//  Function:		DrawPushButton
//
//  Synopsis:		Draws a push style button in the given state
//
//  Effects:		Adjusts passed in rectangle if desired
//
//  Algorithm:		Depending on the state we either draw it raised or
//                      pressed, possibly mono & possibly soft
//
//					If it is an option push button (a push button that is
//						really a check button or a radio button like buttons
//						in tool bars), and it is checked, then we draw it
//						depressed with a different fill in the middle.
//
//--------------------------------------------------------------------------
void FAR DrawPushButton(HDC hdc, LPRECT lprc, UINT state, UINT flags)
{
    RECT   rc;
    HBRUSH hbrMiddle;
    DWORD  rgbBack;
    DWORD  rgbFore;

    CopyRect(&rc, lprc);
    DrawEdge(hdc, &rc, 
             (state & (DFCS_PUSHED | DFCS_CHECKED)) ? EDGE_SUNKEN : EDGE_RAISED,
             (UINT) (BF_ADJUST | BF_RECT |
            (flags & (BF_SOFT | BF_FLAT | BF_MONO))));
       
    //
    // BOGUS
    // On monochrome, need to do something to make pushed buttons look 
    // better.
    //

    // Fill in middle.  If checked, use dither brush (gray brush) with
    // black becoming normal color.

    if (state & DFCS_CHECKED)
    {
        hbrMiddle = hbrGray;
        rgbBack = SetBkColor(hdc, RGB_3DHILIGHT);
        rgbFore = SetTextColor(hdc, RGB_3DFACE);
    }
    else
        hbrMiddle = HBR_3DFACE;

    // BOGUS -- should do a PatBlt if this brush is selected in
    UnrealizeObject(hbrMiddle);
    FillRect(hdc, &rc, hbrMiddle);
   
    if (state & DFCS_CHECKED)
    {
        SetBkColor(hdc, rgbBack);
        SetTextColor(hdc, rgbFore);
    }

    if (flags & BF_ADJUST)
        CopyRect(lprc, &rc);
}

#ifdef _3DSTUFFLATER
//+-------------------------------------------------------------------------
//
//  Function:       _DrawUserGlyph
//
//  Synopsis:       Draws a user glyph in a particular location with the
//                      desired brush
//
//  Notes:
//--------------------------------------------------------------------------
BOOL DrawUserGlyph
(
    HDC     hdc,
    GLY     gly,
    HBRUSH  hbr,
    int     x,
    int     y,
    int     cx,
    int     cy,
    DWORD   rop
)
{
    HBRUSH hbrT;
    DWORD rgbBack;
    DWORD rgbFore;

    if (cx == 0)
        cx = aglyphUser[gly].dx;
    if (cy == 0)
        cy = aglyphUser[gly].dy;

	if (hbr != NULL)
		hbrT = SelectBrush(hdc, hbr);

    // Need to do this because glyphs are monochrome
    rgbBack = SetBkColor(hdc, RGB_WHITE);
    rgbFore = SetTextColor(hdc, RGB_BLACK);

    StretchBlt(hdc, x, y, cx, cy, hdcMonoBits, aglyphUser[gly].dxStart,
            0, aglyphUser[gly].dx, aglyphUser[gly].dy, rop, NULL);

    SetTextColor(hdc, rgbFore);
    SetBkColor(hdc, rgbBack);

    if (hbr != NULL)
       SelectBrush(hdc, hbrT);

    return(TRUE);
}
#endif //_3DSTUFFLATER

