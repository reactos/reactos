/****************************** Module Header ******************************\
* Module Name: draw.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains common drawing functions.
*
* History:
* 12-Feb-1992 MikeKe    Moved Drawtext to the client side
\***************************************************************************/


CONST WCHAR szRadio[] = L"nmlkji";
CONST WCHAR szCheck[] = L"gfedcb";

/***************************************************************************\
* FlipUserTextOutW
*
* Flip the check mark if the hdc is mirrored otherwise it just calls UserTextOutW
*
\***************************************************************************/
BOOL FlipUserTextOutW(HDC hdc, int x, int y, LPCWSTR ch, int nCount)
{
    BOOL bRet;
#ifdef USE_MIRRORING
    int iOldTextAlign, iGraphicsModeOld;

    if ((UserGetLayout(hdc) & LAYOUT_RTL) &&
        (nCount == 1) && 
        ((ch[0] == TEXT('a')) ||(ch[0] == TEXT('b')))
       ) 
    {
        //Check mark then set the hdc in GM_COMPATIBLE to unmirror it.
        if (iGraphicsModeOld = UserSetGraphicsMode(hdc, GM_COMPATIBLE))
        {
            iOldTextAlign = UserGetTextAlign(hdc);
            if ((iOldTextAlign & TA_CENTER) != TA_CENTER)
            {
                UserSetTextAlign(hdc, iOldTextAlign^TA_RIGHT);
            }
            bRet = UserTextOutW(hdc, x, y, ch, nCount);
            UserSetGraphicsMode(hdc, iGraphicsModeOld);
            UserSetTextAlign(hdc, iOldTextAlign);
        }
    }
    else
#endif
        bRet = UserTextOutW(hdc, x, y, ch, nCount);

    return bRet;
}
/***************************************************************************\
* FillRect
*
* Callable from either client or server contexts
*
* History:
* 29-Oct-1990 MikeHar   Ported from Windows.
\***************************************************************************/

int APIENTRY FillRect(
    HDC     hdc,
    LPCRECT prc,
    HBRUSH  hBrush)
{
    ULONG_PTR   iBrush;
    POLYPATBLT PolyData;

    iBrush = (ULONG_PTR)hBrush - 1;
    if (iBrush <= COLOR_ENDCOLORS) {
        hBrush = SYSHBRUSH(iBrush);
    }

    PolyData.x         = prc->left;
    PolyData.y         = prc->top;
    PolyData.cx        = prc->right - prc->left;
    PolyData.cy        = prc->bottom - prc->top;
    PolyData.BrClr.hbr = hBrush;

    /*
     * Win95 incompatibility: they return either hBrush or the brush that
     * was previosuly selected in hdc. Not documented this way though.
     */
    return UserPolyPatBlt(hdc, PATCOPY, &PolyData, 1, PPB_BRUSH);
}

/***************************************************************************\
* InvertRect
*
* Can be called from either the client or server contexts.
*
* History:
* 29-Oct-1990 MikeHar   Ported from Windows.
\***************************************************************************/

BOOL APIENTRY InvertRect(
    HDC     hdc,
    LPCRECT prc)
{
    return UserPatBlt(hdc,
                      prc->left,
                      prc->top,
                      prc->right - prc->left,
                      prc->bottom - prc->top,
                      DSTINVERT);
}

/***************************************************************************\
* DrawDiagonalLine
*
* History:
\***************************************************************************/

DWORD DrawDiagonalLine(
    HDC    hdc,
    LPRECT lprc,
    int    iDirection,
    int    iThickness,
    UINT   flags)
{
    RECT    rc;
    LPINT   py;
    int     cx;
    int     cy;
    int     dx;
    int     dy;
    LPINT   pc;

    POLYPATBLT ppbData[8];
    int        ppbCount = 0;

    if (IsRectEmpty(lprc))
        return 0L;

    rc = *lprc;

    /*
     * We draw slopes < 1 by varying y instead of x.
     */
    --iThickness;

    /*
     * HACK HACK HACK. REMOVE THIS ONCE MARLETT IS AROUND
     */
    cy = rc.bottom - rc.top;
    cx = rc.right - rc.left;

    if (!flags && (cy != cx))
        cy -= iThickness * SYSMETRTL(CYBORDER);

    if (cy >= cx) {

        /*
         * "slope" is >= 1, so vary x by 1
         */
        cy /= cx;
        pc = &cy;

        cx = SYSMETRTL(CXBORDER);

    } else {

        /*
         * "slope" is < 1, so vary y by 1
         */
        cx /= cy;
        pc = &cx;

        cy = SYSMETRTL(CYBORDER);
    }

    dx = cx;
    dy = iDirection * cy;

    *pc = (*pc + iThickness) * SYSMETRTL(CYBORDER);

    rc.right  -= cx;
    rc.bottom -= cy;

    /*
     * For negative slopes, start from opposite side.
     */
    py = ((iDirection < 0) ? &rc.top : &rc.bottom);

    while ((rc.left <= rc.right) && (rc.top <= rc.bottom)) {

        if (!(flags & BF_MIDDLE)) {

            /*
             * UserPatBlt(hdc, rc.left, *py, cx, cy, PATCOPY);
             */

            ppbData[ppbCount].x         = rc.left;
            ppbData[ppbCount].y         = *py;
            ppbData[ppbCount].cx        = cx;
            ppbData[ppbCount].cy        = cy;
            ppbData[ppbCount].BrClr.hbr = NULL;

            ppbCount++;

        } else {

            /*
             * Fill interior.  We can determine vertex in interior
             * by vector define.
             */
            if (cy > SYSMETRTL(CYBORDER)) {

                if (flags & BF_LEFT) {

                    /*
                     * UserPatBlt(hdc, rc.left, lprc->top, cx, *py - lprc->top + cy, PATCOPY);
                     */

                    ppbData[ppbCount].x         = rc.left;
                    ppbData[ppbCount].y         = lprc->top;
                    ppbData[ppbCount].cx        = cx;
                    ppbData[ppbCount].cy        = *py - lprc->top + cy;
                    ppbData[ppbCount].BrClr.hbr = NULL;

                    ppbCount++;

                } else {
                    /*
                     * UserPatBlt(hdc, rc.left, *py, cx, lprc->bottom - *py, PATCOPY);
                     */

                    ppbData[ppbCount].x          = rc.left;
                    ppbData[ppbCount].y          = *py;
                    ppbData[ppbCount].cx         = cx;
                    ppbData[ppbCount].cy         = lprc->bottom - *py;
                    ppbData[ppbCount].BrClr.hbr  = NULL;

                    ppbCount++;
                }

            } else {

                if (flags & BF_TOP) {

                    /*
                     * UserPatBlt(hdc, rc.left, *py, lprc->right - rc.left, cy, PATCOPY);
                     */

                    ppbData[ppbCount].x          = rc.left;
                    ppbData[ppbCount].y          = *py;
                    ppbData[ppbCount].cx         = lprc->right - rc.left;
                    ppbData[ppbCount].cy         = cy;
                    ppbData[ppbCount].BrClr.hbr  = NULL;

                    ppbCount++;

                } else {
                    /*
                     * UserPatBlt(hdc, lprc->left, *py, rc.left - lprc->left + cx, cy, PATCOPY);
                     */

                    ppbData[ppbCount].x          = lprc->left;
                    ppbData[ppbCount].y          = *py;
                    ppbData[ppbCount].cx         = rc.left - lprc->left + cx;
                    ppbData[ppbCount].cy         = cy;
                    ppbData[ppbCount].BrClr.hbr  = NULL;

                    ppbCount++;

                }
            }
        }

        rc.left += dx;
        *py     -= dy;

        /*
         * do we need to flush PolyPatBlt ?
         */
        if (ppbCount == 8) {
            UserPolyPatBlt(hdc, PATCOPY, &ppbData[0], 8, PPB_BRUSH);
            ppbCount = 0;
        }
    }

    /*
     * any left-over PolyPatblt buffered operations?
     */
    if (ppbCount != 0) {
        UserPolyPatBlt(hdc, PATCOPY, &ppbData[0], ppbCount, PPB_BRUSH);
    }

    return MAKELONG(cx, cy);
}

/***************************************************************************\
* FillTriangle
*
* Fills in the triangle whose sides are two rectangle edges and a
* diagonal.  The vertex in the interior can be determined from the
* vector type.
*
* History:
\***************************************************************************/

BOOL FillTriangle(
    HDC    hdc,
    LPRECT lprc,
    HBRUSH hbr,
    UINT   flags)
{
    HBRUSH hbrT;
    int    nDirection;

    switch (flags & (BF_RECT | BF_DIAGONAL)) {

    case BF_DIAGONAL_ENDTOPLEFT:
    case BF_DIAGONAL_ENDBOTTOMRIGHT:
        nDirection = -1;
        break;

    default:
        nDirection = 1;
        break;
    }
    hbrT = UserSelectBrush(hdc, hbr);
    DrawDiagonalLine(hdc, lprc, nDirection, 1, flags);
    /*
     * Don't care if the above functions failed for a bad hdc
     */
    return (UserSelectBrush(hdc, hbrT) != NULL);
}

/***************************************************************************\
* DrawDiagonal
*
* Called by DrawEdge() for BF_DIAGONAL edges.
*
* Draws line of slope 1, one of 4 different ones.  The difference is
* where the line starts and where the end point is.  The BF_ flags for
* BF_DIAGONAL specify where the end point is.  For example, BF_DIAGONAL |
* BF_TOP | BF_LEFT means to draw a line ending up at the top left corner.
* So the origin must be bottom right, and the angle must be 3pi/4, or
* 135 degrees.
*
* History:
\***************************************************************************/

BOOL DrawDiagonal(
    HDC    hdc,
    LPRECT lprc,
    HBRUSH hbrTL,
    HBRUSH hbrBR,
    UINT   flags)
{
    HBRUSH  hbrT;
    int     nDirection;
    DWORD   dAdjust;

    /*
     * Away from light source
     */
    hbrT = ((flags & BF_BOTTOM) ? hbrBR : hbrTL);

    switch (flags & (BF_RECT | BF_DIAGONAL)){

    case BF_DIAGONAL_ENDTOPLEFT:
    case BF_DIAGONAL_ENDBOTTOMRIGHT:
        nDirection = -1;
        break;

    default:
        nDirection = 1;
        break;
    }

    hbrT = UserSelectBrush(hdc, hbrT);
    dAdjust = DrawDiagonalLine(hdc, lprc, nDirection, 1, (flags & ~BF_MIDDLE));
    /*
     * Adjust rectangle for next border
     */
    if (flags & BF_TOP)
        lprc->left += LOWORD(dAdjust);
    else
        lprc->right -= LOWORD(dAdjust);

    if (flags & BF_RIGHT)
        lprc->top += HIWORD(dAdjust);
    else
        lprc->bottom -= HIWORD(dAdjust);

    /*
     * Moved this to the end to save a check for return value
     */
    return (UserSelectBrush(hdc, hbrT) != NULL);
}

/***************************************************************************\
* DrawGrip
*
* History:
\***************************************************************************/

BOOL DrawGrip(
    HDC    hdc,
    LPRECT lprc,
    UINT   wState)
{
    int        x;
    int        y;
    int        c;
    HBRUSH     hbrOld;
    DWORD      rgbHilight;
    DWORD      rgbShadow;
    DWORD      rgbOld;
    POLYPATBLT PolyData;

    c = min((lprc->right - lprc->left), (lprc->bottom - lprc->top));
    x = lprc->right  - c;    // right justify
    y = lprc->bottom - c;    // bottom justify

    /*
     * Setup colors
     */
    if (wState & (DFCS_FLAT | DFCS_MONO)) {
        hbrOld = SYSHBR(WINDOW);
        rgbHilight = SYSRGBRTL(WINDOWFRAME);
        rgbShadow = SYSRGBRTL(WINDOWFRAME);
    } else {
        hbrOld = SYSHBR(3DFACE);
        rgbHilight = SYSRGBRTL(3DHILIGHT);
        rgbShadow = SYSRGBRTL(3DSHADOW);
    }

    PolyData.x         = lprc->left;
    PolyData.y         = lprc->top;
    PolyData.cx        = lprc->right-lprc->left;
    PolyData.cy        = lprc->bottom-lprc->top;
    PolyData.BrClr.hbr = hbrOld;
    UserPolyPatBlt(hdc, PATCOPY, &PolyData, 1, PPB_BRUSH);

    rgbOld = UserSetTextColor(hdc, rgbHilight);

    if (wState & DFCS_SCROLLSIZEGRIPRIGHT) {
        UserTextOutW(hdc, x, y, L"x", 1);
        UserSetTextColor(hdc, rgbShadow);
        UserTextOutW(hdc, x, y, L"y", 1);
    } else {
        UserTextOutW(hdc, x, y, L"o", 1);
        UserSetTextColor(hdc, rgbShadow);
        UserTextOutW(hdc, x, y, L"p", 1);
    }

    UserSetTextColor(hdc, rgbOld);
    return TRUE;
}

/***************************************************************************\
* DrawBox
*
* History:
\***************************************************************************/

BOOL DrawBox(
    HDC    hdc,
    LPRECT lprc,
    UINT   wControlState)
{
    int      cx;
    int      cy;
    int      c;
    int      x;
    int      y;
    LPCWSTR  lp = szRadio;
    int      i;
    BOOL     fSkip0thItem;
    COLORREF clr[6];
    COLORREF clrOld;

    fSkip0thItem = ((wControlState & (DFCS_BUTTON3STATE | DFCS_PUSHED |
        DFCS_INACTIVE | DFCS_CHECKED)) == (DFCS_BUTTON3STATE | DFCS_CHECKED));

    /*
     * Don't need radio mask with marlett font!
     */
    if (wControlState & DFCS_BUTTONRADIOMASK) {

        clr[0] = clr[1] = clr[2] = clr[3] = clr[4] = 0L;
        FillRect(hdc, lprc, ghbrWhite);

    } else {

        /*
         * DFCS_BUTTONRADIOIMAGE
         */
        if (wControlState & (DFCS_MONO | DFCS_FLAT)) {
            clr[1] = clr[2] = clr[3] = clr[4] = SYSRGBRTL(WINDOWFRAME);
        } else {
            clr[1] = SYSRGBRTL(3DLIGHT);
            clr[2] = SYSRGBRTL(3DDKSHADOW);
            clr[3] = SYSRGBRTL(3DHILIGHT);
            clr[4] = SYSRGBRTL(3DSHADOW);
        }


        if (wControlState & (DFCS_PUSHED | DFCS_INACTIVE))
            clr[0] = SYSRGBRTL(3DFACE);
        else if (fSkip0thItem)
            clr[0] = SYSRGBRTL(3DHILIGHT);
        else
            clr[0] = SYSRGBRTL(WINDOW);

        if (wControlState & DFCS_BUTTONRADIOIMAGE)
            FillRect(hdc, lprc, ghbrBlack);
        else if (!(wControlState & DFCS_BUTTONRADIO))
            lp = szCheck;
    }

    cx = lprc->right - lprc->left;
    cy = lprc->bottom - lprc->top;

    c = min(cx,cy);
    x = lprc->left + ((cx - c) / 2); // - 1;
    y = lprc->top  + ((cy - c) / 2);

    if (fSkip0thItem &&
        ((gpsi->BitCount < 8) || (SYSRGB(3DHILIGHT) == RGB(255,255,255)))) {

        COLORREF   clrBk;
        POLYPATBLT PolyData;

         /*
          * Make the interior of a 3State checkbox which is just checked a
          * dither, just like an indeterminate push button which is pressed.
          */
         clrBk  = UserSetBkColor(hdc, SYSRGB(3DHILIGHT));
         clrOld = UserSetTextColor(hdc, SYSRGB(3DFACE));

         PolyData.x         = x;
         PolyData.y         = y;
         PolyData.cx        = cx;
         PolyData.cy        = cy;
         PolyData.BrClr.hbr = gpsi->hbrGray;
         UserPolyPatBlt(hdc, PATCOPY, &PolyData, 1, PPB_BRUSH);

         UserSetBkColor(hdc, clrBk);

    } else {
        clrOld = UserSetTextColor(hdc, clr[0]);
        UserTextOutW(hdc, x, y, lp, 1);
    }

    lp++;

    for (i = 1; i < 5; i++) {
        UserSetTextColor(hdc, clr[i]);
        UserTextOutW(hdc, x, y, lp++, 1);
    }

    if (wControlState & DFCS_CHECKED) {
        COLORREF clrCheck;

        if (wControlState & (DFCS_BUTTON3STATE | DFCS_INACTIVE)) {
            clrCheck = SYSRGBRTL(3DSHADOW);
        } else if (wControlState & DFCS_HOT) {
            clrCheck = SYSRGBRTL(HOTLIGHT);
        } else {
            clrCheck = SYSRGBRTL(WINDOWTEXT);
        }

        UserSetTextColor(hdc, clrCheck);
        FlipUserTextOutW(hdc, x, y, lp, 1);
    }

    UserSetTextColor(hdc, clrOld);

    return TRUE;
}
/***************************************************************************\
* GetCaptionChar
*
* History:
* 04/02/97 GerardoB Created
\***************************************************************************/
WCHAR GetCaptionChar (UINT wState)
{
    wState &= DFCS_CAPTIONALL;
    switch (wState) {
    case DFCS_CAPTIONCLOSE:
        return TEXT('r');
    case DFCS_CAPTIONMIN:
        return TEXT('0');
    case DFCS_CAPTIONMAX:
        return TEXT('1');
    case DFCS_CAPTIONRESTORE:
        return TEXT('2');
    /* case DFCS_CAPTIONHELP: */
    default:
        return TEXT('s');
    }
}
/***************************************************************************\
* DrawMenuMark
*
* History:
\***************************************************************************/

BOOL DrawMenuMark(
    HDC    hdc,
    LPRECT lprc,
    UINT   wType,
    UINT   wState)
{
    COLORREF rgbOld;
    int      x;
    int      y;
    int      c;
    int      cx;
    int      cy;
    WCHAR    ch;

    cx = lprc->right - lprc->left;
    cy = lprc->bottom - lprc->top;

    c = min(cx,cy);
    x = lprc->left + ((cx - c) / 2) - ((cx > 0xb) ? 1 : 0);
    y = lprc->top  + ((cy - c) / 2);

    FillRect(hdc, lprc, ghbrWhite);

    rgbOld = UserSetTextColor(hdc, 0L);

    if (wType == DFC_MENU) {
        if (wState & DFCS_MENUCHECK) {
            ch = TEXT('a');
        } else if (wState & DFCS_MENUBULLET) {
            ch = TEXT('h');
        } else if (wState & DFCS_MENUARROWRIGHT) {
            ch = TEXT('w');
        } else {
            ch = TEXT('8');
        }
    } else {
        UserAssert(wType == DFC_POPUPMENU);
        ch = GetCaptionChar(wState);
    }

    FlipUserTextOutW(hdc, x, y, &ch, 1);
    UserSetTextColor(hdc, rgbOld);

    return TRUE;
}

/***************************************************************************\
* DrawIt
*
* History:
\***************************************************************************/

BOOL DrawIt(
    HDC    hdc,
    LPRECT lprc,
    UINT   wState,
    WCHAR  ch)
{
    COLORREF rgb;
    int      x;
    int      y;
    int      c;
    int      cx;
    int      cy;
    BOOL     fDrawDisabled = wState & DFCS_INACTIVE;

    cx = lprc->right - lprc->left;
    cy = lprc->bottom - lprc->top;

    c = min(cx,cy);
    x = lprc->left + ((cx - c) / 2);
    y = lprc->top  + ((cy - c) / 2);

    if (fDrawDisabled) {
        rgb = SYSRGBRTL(3DHILIGHT);
    } else if (wState & DFCS_HOT) {
        rgb = SYSRGBRTL(HOTLIGHT);
    } else {
        rgb = SYSRGBRTL(BTNTEXT);
    }

    rgb = UserSetTextColor(hdc, rgb);

    if (wState & (DFCS_INACTIVE | DFCS_PUSHED)) {
        x++;
        y++;
    }

    UserTextOutW(hdc, x, y, &ch, 1);

    if (fDrawDisabled) {
        UserSetTextColor(hdc, SYSRGBRTL(3DSHADOW));
        UserTextOutW(hdc, x - 1, y - 1, &ch, 1);
    }

    UserSetTextColor(hdc, rgb);

    return TRUE;
}

/***************************************************************************\
* DrawScrollArrow
*
* History:
\***************************************************************************/

BOOL DrawScrollArrow(
    HDC    hdc,
    LPRECT lprc,
    UINT   wControlState)
{
    WCHAR ch = (wControlState & DFCS_SCROLLHORZ) ? TEXT('3') : TEXT('5');

    if (wControlState & DFCS_SCROLLMAX)
        ch++;

    return DrawIt(hdc, lprc, wControlState, ch);
}

/***************************************************************************\
* DrawFrameControl
*
* History:
\***************************************************************************/

BOOL DrawFrameControl(
    HDC    hdc,
    LPRECT lprc,
    UINT   wType,
    UINT   wState)
{
    RECT     rc;
    HFONT    hFont;
    HFONT    hOldFont;
    BOOL     fRet = TRUE;
    int      iOldBk;
    int      c;
    BOOL     fButton = FALSE;
    LOGFONTW lfw;
#ifdef USE_MIRRORING
    int      iGraphicsModeOld = 0;
    int      iOldTextAlign;
#endif

    rc = *lprc;

#ifdef USE_MIRRORING
    /*
     * If the hdc is mirrored then set it in GM_ADVANCED mode
     * to enforce the text to be mirrored.
     */
    if (UserGetLayout(hdc) & LAYOUT_RTL) {
        if (iGraphicsModeOld = UserSetGraphicsMode(hdc, GM_ADVANCED))
        {   
            iOldTextAlign = UserGetTextAlign(hdc);
            if ((iOldTextAlign & TA_CENTER) != TA_CENTER)
            {
                UserSetTextAlign(hdc, iOldTextAlign^TA_RIGHT);
            }
        }
    }
#endif
    /*
     * Enforce monochrome/flat
     */
    if (gpsi->BitCount == 1)
        wState |= DFCS_MONO;

    if (wState & DFCS_MONO)
        wState |= DFCS_FLAT;

    if ((wType != DFC_MENU)
            && (wType != DFC_POPUPMENU)
            && ((wType != DFC_BUTTON) || (wState & DFCS_BUTTONPUSH))
            && ((wType != DFC_SCROLL)
                || !(wState & (DFCS_SCROLLSIZEGRIP | DFCS_SCROLLSIZEGRIPRIGHT))))
    {
        UINT wBorder = BF_ADJUST;

        if (wType != DFC_SCROLL)
            wBorder |= BF_SOFT;

        UserAssert(DFCS_FLAT == BF_FLAT);
        UserAssert(DFCS_MONO == BF_MONO);

        wBorder |= (wState & (DFCS_FLAT | DFCS_MONO));

        DrawPushButton(hdc, &rc, wState, wBorder);

        if (wState & DFCS_ADJUSTRECT)
            *lprc = rc;

        fButton = TRUE;
    }

    iOldBk = UserSetBkMode(hdc, TRANSPARENT);
    if (!iOldBk) {
        /*
         * return FALSE if the hdc is bogus
         */
#ifdef USE_MIRRORING
        if (iGraphicsModeOld) {
            UserSetGraphicsMode(hdc, iGraphicsModeOld);
            UserSetTextAlign(hdc, iOldTextAlign);
        }
#endif
        return FALSE;
    }

    c = min(rc.right - rc.left, rc.bottom - rc.top);

    if (c <= 0) {
#ifdef USE_MIRRORING
        if (iGraphicsModeOld){
            UserSetGraphicsMode(hdc, iGraphicsModeOld);
            UserSetTextAlign(hdc, iOldTextAlign);
        }
#endif
        return FALSE;
    }

    RtlZeroMemory(&lfw, sizeof(lfw));
    lfw.lfHeight         = c;
    lfw.lfWeight         = FW_NORMAL;
    lfw.lfCharSet        = SYMBOL_CHARSET;
    RtlCopyMemory(lfw.lfFaceName, L"Marlett", sizeof(L"Marlett"));
    hFont = UserCreateFontIndirectW(&lfw);

    hOldFont = UserSelectFont(hdc, hFont);

    if (!fButton) {

        if ((wType == DFC_MENU) || (wType == DFC_POPUPMENU)) {
            if (wState & (DFCS_MENUARROWUP | DFCS_MENUARROWDOWN)) {
                if (!(wState & DFCS_TRANSPARENT))  {
                    POLYPATBLT ppbData;

                    ppbData.x  = lprc->left;
                    ppbData.y  = lprc->top;
                    ppbData.cx = lprc->right - lprc->left;
                    ppbData.cy = lprc->bottom - lprc->top;
                    ppbData.BrClr.hbr = SYSHBR(MENU);
                    UserPolyPatBlt(hdc, PATCOPY, &ppbData, 1, PPB_BRUSH);
                }
                DrawScrollArrow(hdc, &rc,
                        (wState & (DFCS_HOT | DFCS_INACTIVE)) | ((wState & DFCS_MENUARROWUP) ? DFCS_SCROLLUP : DFCS_SCROLLDOWN));
            } else {
                DrawMenuMark(hdc, &rc, wType, wState);
            }
        } else if (wType == DFC_BUTTON) {
            DrawBox(hdc, &rc, wState);
        } else {  // wType == DFC_SCROLL
            DrawGrip(hdc, lprc, wState);
        }

    } else if (wType == DFC_CAPTION) {
        DrawIt(hdc, &rc, wState, GetCaptionChar(wState));
    } else if (wType == DFC_SCROLL) {

        DrawScrollArrow(hdc, &rc, wState);

    } else if (wType != DFC_BUTTON) {

        fRet = FALSE;
    }

#ifdef USE_MIRRORING
    if (iGraphicsModeOld){
        UserSetGraphicsMode(hdc, iGraphicsModeOld);
        UserSetTextAlign(hdc, iOldTextAlign);
    }
#endif
    UserSetBkMode(hdc, iOldBk);
    UserSelectFont(hdc, hOldFont);
    UserDeleteObject(hFont);

    return fRet;
}

/***************************************************************************\
* DrawEdge
*
* Draws a 3D edge using 2 3D borders.  Adjusts interior rectangle if desired
* And fills it if requested.
*
* Returns:
*     FALSE if error
*
* History:
* 30-Jan-1991 Laurabu   Created.
\***************************************************************************/

BOOL DrawEdge(
    HDC    hdc,
    LPRECT lprc,
    UINT   edge,
    UINT   flags)
{
    HBRUSH     hbrTL;
    HBRUSH     hbrBR;
    RECT       rc;
    UINT       bdrType;
    POLYPATBLT ppbData[4];
    UINT       ppbCount;
    BOOL       fResult = TRUE;

    /*
     * Enforce monochromicity and flatness
     */
    if (gpsi->BitCount == 1)
        flags |= BF_MONO;

    if (flags & BF_MONO)
        flags |= BF_FLAT;

    rc = *lprc;

    /*
     * Draw the border segment(s), and calculate the remaining space as we
     * go.
     */
    if (bdrType = (edge & BDR_OUTER)) {

DrawBorder:

        /*
         * Get brushes.  Note the symmetry between raised outer,
         * sunken inner and sunken outer, raised inner.
         */
        if (flags & BF_FLAT) {

            if (flags & BF_MONO)
                hbrBR = (bdrType & BDR_OUTER) ? SYSHBR(WINDOWFRAME) : SYSHBR(WINDOW);
            else
                hbrBR = (bdrType & BDR_OUTER) ? SYSHBR(3DSHADOW) : SYSHBR(3DFACE);

            hbrTL = hbrBR;

        } else {

            /*
             * 5 == HILIGHT
             * 4 == LIGHT
             * 3 == FACE
             * 2 == SHADOW
             * 1 == DKSHADOW
             */

            switch (bdrType) {
            /*
             * +2 above surface
             */
            case BDR_RAISEDOUTER:
                hbrTL = ((flags & BF_SOFT) ? SYSHBR(3DHILIGHT) : SYSHBR(3DLIGHT));
                hbrBR = SYSHBR(3DDKSHADOW);     // 1
                break;

            /*
             * +1 above surface
             */
            case BDR_RAISEDINNER:
                hbrTL = ((flags & BF_SOFT) ? SYSHBR(3DLIGHT) : SYSHBR(3DHILIGHT));
                hbrBR = SYSHBR(3DSHADOW);       // 2
                break;

            /*
             * -1 below surface
             */
            case BDR_SUNKENOUTER:
                hbrTL = ((flags & BF_SOFT) ? SYSHBR(3DDKSHADOW) : SYSHBR(3DSHADOW));
                hbrBR = SYSHBR(3DHILIGHT);      // 5
                break;

            /*
             * -2 below surface
             */
            case BDR_SUNKENINNER:
                hbrTL = ((flags & BF_SOFT) ? SYSHBR(3DSHADOW) : SYSHBR(3DDKSHADOW));
                hbrBR = SYSHBR(3DLIGHT);        // 4
                break;

            default:
                return FALSE;
            }
        }

        /*
         * Draw the sides of the border.  NOTE THAT THE ALGORITHM FAVORS THE
         * BOTTOM AND RIGHT SIDES, since the light source is assumed to be top
         * left.  If we ever decide to let the user set the light source to a
         * particular corner, then change this algorithm.
         */
        if (flags & BF_DIAGONAL) {

            fResult = DrawDiagonal(hdc, &rc, hbrTL, hbrBR, flags);

        } else {

            /*
             * reset ppbData index
             */
            ppbCount = 0;

            /*
             * Bottom Right edges
             */
                /*
                 * Right
                 */
            if (flags & BF_RIGHT) {

                rc.right -= SYSMETRTL(CXBORDER);

                ppbData[ppbCount].x         = rc.right;
                ppbData[ppbCount].y         = rc.top;
                ppbData[ppbCount].cx        = SYSMETRTL(CXBORDER);
                ppbData[ppbCount].cy        = rc.bottom - rc.top;
                ppbData[ppbCount].BrClr.hbr = hbrBR;
                ppbCount++;
            }

            /*
             * Bottom
             */
            if (flags & BF_BOTTOM) {
                rc.bottom -= SYSMETRTL(CYBORDER);

                ppbData[ppbCount].x         = rc.left;
                ppbData[ppbCount].y         = rc.bottom;
                ppbData[ppbCount].cx        = rc.right - rc.left;
                ppbData[ppbCount].cy        = SYSMETRTL(CYBORDER);
                ppbData[ppbCount].BrClr.hbr = hbrBR;
                ppbCount++;
            }

            /*
             * Top Left edges
             */
            /*
             * Left
             */
            if (flags & BF_LEFT) {
                ppbData[ppbCount].x         = rc.left;
                ppbData[ppbCount].y         = rc.top;
                ppbData[ppbCount].cx        = SYSMETRTL(CXBORDER);
                ppbData[ppbCount].cy        = rc.bottom - rc.top;
                ppbData[ppbCount].BrClr.hbr = hbrTL;
                ppbCount++;

                rc.left += SYSMETRTL(CXBORDER);
            }

            /*
             * Top
             */
            if (flags & BF_TOP) {
                ppbData[ppbCount].x         = rc.left;
                ppbData[ppbCount].y         = rc.top;
                ppbData[ppbCount].cx        = rc.right - rc.left;
                ppbData[ppbCount].cy        = SYSMETRTL(CYBORDER);
                ppbData[ppbCount].BrClr.hbr = hbrTL;
                ppbCount++;

                rc.top += SYSMETRTL(CYBORDER);
            }
            /*
             * Send all queued PatBlts to GDI in one go
             */
            fResult = UserPolyPatBlt(hdc,PATCOPY,&ppbData[0],ppbCount,PPB_BRUSH);
        }
    }

    if (bdrType = (edge & BDR_INNER)) {
        /*
         * Strip this so the next time through, bdrType will be 0.
         * Otherwise, we'll loop forever.
         */
        edge &= ~BDR_INNER;
        goto DrawBorder;
    }


    /*
     * Select old brush back in, if we changed it.
     */

    /*
     * Fill the middle & clean up if asked
     */
    if (flags & BF_MIDDLE) {
        if (flags & BF_DIAGONAL)
            fResult = FillTriangle(hdc, &rc, ((flags & BF_MONO) ? (HBRUSH)SYSHBR(WINDOW) : (HBRUSH)SYSHBR(3DFACE)), flags);
        else
            fResult = FillRect(hdc, &rc, ((flags & BF_MONO) ? (HBRUSH)SYSHBR(WINDOW) : (HBRUSH)SYSHBR(3DFACE)));
    }

    if (flags & BF_ADJUST)
        *lprc = rc;

    return fResult;
}

/***************************************************************************\
* DrawPushButton
*
* Draws a push style button in the given state.  Adjusts passed in rectangle
* if desired.
*
* Algorithm:
*    Depending on the state we either draw
*             * raised edge   (undepressed)
*             * sunken edge with extra shadow (depressed)
*     If it is an option push button (a push button that is
*             really a check button or a radio button like buttons
*             in tool bars), and it is checked, then we draw it
*             depressed with a different fill in the middle.
*
* History:
* 05-Feb-19 Laurabu     Created.
\***************************************************************************/

VOID DrawPushButton(
    HDC    hdc,
    LPRECT lprc,
    UINT   state,
    UINT   flags)
{
    RECT   rc;
    HBRUSH hbrMiddle;
    DWORD  rgbBack;
    DWORD  rgbFore;
    BOOL   fDither;

    rc = *lprc;

    DrawEdge(hdc,
             &rc,
             (state & (DFCS_PUSHED | DFCS_CHECKED)) ? EDGE_SUNKEN : EDGE_RAISED,
             (UINT)(BF_ADJUST | BF_RECT | (flags & (BF_SOFT | BF_FLAT | BF_MONO))));

    /*
     * BOGUS
     * On monochrome, need to do something to make pushed buttons look
     * better.
     */

    /*
     * Fill in middle.  If checked, use dither brush (gray brush) with
     * black becoming normal color.
     */
    fDither = FALSE;

    if (state & DFCS_CHECKED) {

        if ((gpsi->BitCount < 8) || (SYSRGBRTL(3DHILIGHT) == RGB(255,255,255))) {
            hbrMiddle = gpsi->hbrGray;
            rgbBack = UserSetBkColor(hdc, SYSRGBRTL(3DHILIGHT));
            rgbFore = UserSetTextColor(hdc, SYSRGBRTL(3DFACE));
            fDither = TRUE;
        } else {
            hbrMiddle = SYSHBR(3DHILIGHT);
        }

    } else {
        hbrMiddle = SYSHBR(3DFACE);
    }

    FillRect(hdc, &rc, hbrMiddle);

    if (fDither) {
        UserSetBkColor(hdc, rgbBack);
        UserSetTextColor(hdc, rgbFore);
    }

    if (flags & BF_ADJUST)
        *lprc = rc;
}

/***************************************************************************\
* DrawFrame
*
* History:
\***************************************************************************/

BOOL DrawFrame(
    HDC   hdc,
    PRECT prc,
    int   clFrame,
    int   cmd)
{
    int        x;
    int        y;
    int        cx;
    int        cy;
    int        cxWidth;
    int        cyWidth;
    HANDLE     hbrSave;
    LONG       rop;
    POLYPATBLT PolyData[4];

    x = prc->left;
    y = prc->top;

    cxWidth = SYSMETRTL(CXBORDER) * clFrame;
    cyWidth = SYSMETRTL(CYBORDER) * clFrame;

    cx = prc->right - x - cxWidth;
    cy = prc->bottom - y - cyWidth;

    rop = ((cmd & DF_ROPMASK) ? PATINVERT : PATCOPY);

    if ((cmd & DF_HBRMASK) == DF_GRAY) {
        hbrSave = gpsi->hbrGray;
    } else {
        UserAssert(((cmd & DF_HBRMASK) >> 3) < COLOR_MAX);
        hbrSave = SYSHBRUSH((cmd & DF_HBRMASK) >> 3);
    }

    PolyData[0].x         = x;
    PolyData[0].y         = y;
    PolyData[0].cx        = cxWidth;
    PolyData[0].cy        = cy;
    PolyData[0].BrClr.hbr = hbrSave;

    PolyData[1].x         = x + cxWidth;
    PolyData[1].y         = y;
    PolyData[1].cx        = cx;
    PolyData[1].cy        = cyWidth;
    PolyData[1].BrClr.hbr = hbrSave;

    PolyData[2].x         = x;
    PolyData[2].y         = y + cy;
    PolyData[2].cx        = cx;
    PolyData[2].cy        = cyWidth;
    PolyData[2].BrClr.hbr = hbrSave;

    PolyData[3].x         = x + cx;
    PolyData[3].y         = y + cyWidth;
    PolyData[3].cx        = cxWidth;
    PolyData[3].cy        = cy;
    PolyData[3].BrClr.hbr = hbrSave;

    UserPolyPatBlt(hdc, rop, &PolyData[0], 4, PPB_BRUSH);

    return TRUE;
}

/***************************************************************************\
* GetSignFromMappingMode
*
* For the current mapping mode,  find out the sign of x from left to right,
* and the sign of y from top to bottom.
*
* History:
\***************************************************************************/

BOOL GetSignFromMappingMode (
    HDC    hdc,
    PPOINT pptSign)
{
    SIZE sizeViewPortExt;
    SIZE sizeWindowExt;

    if (!UserGetViewportExtEx(hdc, &sizeViewPortExt)
            || !UserGetWindowExtEx(hdc, &sizeWindowExt)) {

        return FALSE;
    }

    pptSign->x = ((sizeViewPortExt.cx ^ sizeWindowExt.cx) < 0) ? -1 : 1;

    pptSign->y = ((sizeViewPortExt.cy ^ sizeWindowExt.cy) < 0) ? -1 : 1;

    return TRUE;
}

/***************************************************************************\
* ClientFrame
*
* Draw a rectangle
*
* History:
* 19-Jan-1993 MikeKe    Created
\***************************************************************************/

BOOL ClientFrame(
    HDC     hDC,
    LPCRECT pRect,
    HBRUSH  hBrush,
    DWORD   patOp)
{
    int        x;
    int        y;
    POINT      point;
    POINT      ptSign;
    POLYPATBLT PolyData[4];

    if (!GetSignFromMappingMode (hDC, &ptSign))
        return FALSE;

    y = pRect->bottom - (point.y = pRect->top);
    if (y < 0) {
        return FALSE;
    }

    x = pRect->right -  (point.x = pRect->left);

    /*
     * Check width and height signs
     */
    if (((x ^ ptSign.x) < 0) || ((y ^ ptSign.y) < 0))
        return FALSE;

    // Top border
    PolyData[0].x         = point.x;
    PolyData[0].y         = point.y;
    PolyData[0].cx        = x;
    PolyData[0].cy        = ptSign.y;
    PolyData[0].BrClr.hbr = hBrush;

    // Bottom border
    point.y = pRect->bottom - ptSign.y;
    PolyData[1].x         = point.x;
    PolyData[1].y         = point.y;
    PolyData[1].cx        = x;
    PolyData[1].cy        = ptSign.y;
    PolyData[1].BrClr.hbr = hBrush;

    /*
     * Left Border
     * Don't xor the corners twice
     */
    point.y = pRect->top + ptSign.y;
    y -= 2 * ptSign.y;
    PolyData[2].x         = point.x;
    PolyData[2].y         = point.y;
    PolyData[2].cx        = ptSign.x;
    PolyData[2].cy        = y;
    PolyData[2].BrClr.hbr = hBrush;

    // Right Border
    point.x = pRect->right - ptSign.x;
    PolyData[3].x         = point.x;
    PolyData[3].y         = point.y;
    PolyData[3].cx        = ptSign.x;
    PolyData[3].cy        = y;
    PolyData[3].BrClr.hbr = hBrush;

    return UserPolyPatBlt(hDC, patOp, PolyData, sizeof (PolyData) / sizeof (*PolyData), PPB_BRUSH);
}
