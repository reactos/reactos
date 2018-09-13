/****************************** Module Header ******************************\
* Module Name: mngray.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the DrawState API
*
* History:
* 01-05-94  FritzS  Ported from Chicago
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define PATOR               0x00FA0089L
#define SRCSTENCIL          0x00B8074AL
#define SRCINVSTENCIL       0x00E20746L

#define BC_INVERT             0x00000001

void BltColor(HDC hdc, HBRUSH hbr, HDC hdcSrce,int xO, int yO,
       int cx, int cy, int xO1, int yO1, UINT uBltFlags);
/***************************************************************************\
*
*  BitBltSysBmp()
*
*  From Chicago -- client *only* for now.
\***************************************************************************/
BOOL FAR BitBltSysBmp(HDC hdc, int x, int y, UINT i)
{
    POEMBITMAPINFO pOem = gpsi->oembmi + i;

    return(NtUserBitBltSysBmp(hdc, x, y, pOem->cx, pOem->cy, pOem->x, pOem->y, SRCCOPY));
}


/***************************************************************************\
*
*  DrawState()
*
*  Generic state drawing routine.  Does simple drawing into same DC if
*  normal state;  uses offscreen bitmap otherwise.
*
*  We do drawing for these simple types ourselves:
*      (1) Text
*          lData is string pointer.
*          wData is string length
*      (2) Icon
*          LOWORD(lData) is hIcon
*      (3) Bitmap
*          LOWORD(lData) is hBitmap
*      (4) Glyph (internal)
*          LOWORD(lData) is OBI_ value, one of
*              OBI_CHECKMARK
*              OBI_BULLET
*              OBI_MENUARROW
*          right now
*
*  Other types are required to draw via the callback function, and are
*  allowed to stick whatever they want in lData and wData.
*
*  We apply the following effects onto the image:
*      (1) Normal      (nothing)
*      (2) Default     (drop shadow)
*      (3) Union       (gray string dither)
*      (4) Disabled    (embossed)
*
*  Note that we do NOT stretch anything.  We just clip.
*
*
*   FritzS note -- this is client-side *only*.  Similar code is in server\mngray.c
*
*
\***************************************************************************/
BOOL DrawStateW(
    HDC             hdcDraw,
    HBRUSH          hbrFore,
    DRAWSTATEPROC   qfnCallBack,
    LPARAM          lData,
    WPARAM          wData,
    int             x,
    int             y,
    int             cx,
    int             cy,
    UINT            uFlags)
{
    HFONT   hFont;
    HFONT   hFontSave = NULL;
    HDC     hdcT;
    HBITMAP hbmpT;
    BOOL    fResult = FALSE;
    DWORD   dwPSMFlags;
    POINT   ptOrg;
    int     oldAlign;
#ifdef USE_MIRRORING
    DWORD   dwLayout=0;
#endif

    if (ghdcGray == NULL)
        return FALSE;

    RtlEnterCriticalSection(&gcsHdc);

    /*
     * These require monochrome conversion
     *
     * Enforce monochrome: embossed doesn't look great with 2 color displays
     */
    if ((uFlags & DSS_DISABLED) &&
        (gpsi->BitCount == 1 || SYSMET(SLOWMACHINE))) {

        uFlags &= ~DSS_DISABLED;
        uFlags |= DSS_UNION;
    }

    if (uFlags & (DSS_DISABLED | DSS_DEFAULT | DSS_UNION))
        uFlags |= DSS_MONO;

    /*
     * Get drawing sizes etc. AND VALIDATE.
     */
    switch (uFlags & DST_TYPEMASK) {

        case DST_GLYPH:

            /*
             * LOWORD(lData) is OBI_ value.
             */
            if (LOWORD(lData) >= (WORD)OBI_COUNT) {
                goto CDS_Leave;
            }

            if (!cx) {
                cx = gpsi->oembmi[LOWORD(lData)].cx;
            }

            if (!cy) {
                cy = gpsi->oembmi[LOWORD(lData)].cy;
            }

            break;

        case DST_BITMAP:

            /*
             * LOWORD(lData) is hbmp.
             */
            if (GetObjectType((HGDIOBJ)lData) != OBJ_BITMAP) {
                goto CDS_Leave;
            }

            if (!cx || !cy) {

                BITMAP bmp;

                GetObjectW((HGDIOBJ)lData, sizeof(BITMAP), &bmp);

                if (!cx)
                    cx = bmp.bmWidth;

                if (!cy)
                    cy = bmp.bmHeight;
            }
            break;

        case DST_ICON:

            /*
             * lData is hicon.
             */
            if (!cx || !cy) {

                int cx1 = 0;
                int cy1 = 0;

                NtUserGetIconSize((HICON)lData, 0, &cx1, &cy1);

                if (!cx)
                    cx = cx1;

                if (!cy)
                    cy = cy1 / 2; // icons are double height in NT
            }
            break;

        case DST_TEXT:

            /*
             * lData is LPSTR
             * NOTE THAT WE DO NOT VALIDATE lData, DUE TO COMPATIBILITY
             * WITH GRAYSTRING().  THIS _SHOULD_ FAULT IF YOU PASS IN NULL.
             *
             * wData is cch.
             */
            if (!wData)
                wData = wcslen((LPWSTR)lData);

            if (!cx || !cy) {

                SIZE size;

                /*
                 * Make sure we use right dc w/ right font.
                 */
                GetTextExtentPointW(hdcDraw, (LPWSTR)lData, (INT)wData, &size);

                if (!cx)
                    cx = size.cx;

                if (!cy)
                    cy = size.cy;

            }

            /*
             * Now, pretend we're complex if qfnCallBack is supplied AND
             * we're supporting GrayString().
             */
#if 0 // This will get turned on if/when we change GrayString to tie
      // into DrawState.
      //
      // FritzS
            if ((uFlags & DST_GRAYSTRING) && SELECTOROF(qfnCallBack)) {
                uFlags &= ~DST_TYPEMASK;
                uFlags |= DST_COMPLEX;
            }
#endif
            break;

        case DST_PREFIXTEXT:

            if (lData == 0) {
                RIPMSG0(RIP_ERROR, "DrawState: NULL DST_PREFIXTEXT string");
                goto CDS_Leave;
            }

            if (!wData)
                wData = wcslen((LPWSTR)lData);

            if (!cx || !cy) {

                SIZE size;

                PSMGetTextExtent(hdcDraw, (LPWSTR)lData, (int)wData, &size);

                if (!cx)
                    cx = size.cx;

                if (!cy)
                    cy = size.cy;
            }

            /*
             * Add on height for prefix
             */
            cy += (2 * SYSMET(CYBORDER));
            break;

        case DST_COMPLEX:
#if 0
            if (!SELECTOROF(qfnCallBack)) {
                DebugErr(DBF_ERROR, "DrawState: invalid callback for DST_COMPLEX");
                goto CDS_Leave;
            }
#endif
            break;

        default:
            RIPMSG0(RIP_ERROR, "DrawState: invalid DST_ type");
            goto CDS_Leave;
    }

    /*
     * Optimize:  nothing to draw
     * Have to call callback if GRAYSTRING for compatibility.
     */
    if ((!cx || !cy)
//        && !(uFlags & DST_GRAYSTRING)
    ) {
        fResult = TRUE;
        goto CDS_Leave;
    }

    /*
     * Setup drawing dc
     */
    if (uFlags & DSS_MONO) {

        hdcT = ghdcGray;
#ifdef USE_MIRRORING
        /*
         * First turn off mirroring on hdcGray if any.
         */
        SetLayoutWidth(hdcT, -1, 0);
        /*
         * Set the ghdcGray layout to be equal to the screen hdcDraw layout.
         */
        dwLayout = GetLayout(hdcDraw);
        if (dwLayout != GDI_ERROR) {
            SetLayoutWidth(hdcT, cx, dwLayout);
        }
#endif

        /*
         * Is our scratch bitmap big enough?  We need potentially
         * cx+1 by cy pixels for default etc.
         */
        if ((gcxGray < cx + 1) || (gcyGray < cy)) {

            if (hbmpT = CreateBitmap(max(gcxGray, cx + 1), max(gcyGray, cy), 1, 1, 0L)) {

                HBITMAP hbmGray;

                hbmGray = SelectObject(ghdcGray, hbmpT);
                DeleteObject(hbmGray);

                gcxGray = max(gcxGray, cx + 1);
                gcyGray = max(gcyGray, cy);

            } else {
                cx = gcxGray - 1;
                cy = gcyGray;
            }
        }

        PatBlt(ghdcGray, 0, 0, gcxGray, gcyGray, WHITENESS);
        SetTextCharacterExtra(ghdcGray, GetTextCharacterExtra(hdcDraw));

        oldAlign = GetTextAlign(hdcT);
        SetTextAlign(hdcT, (oldAlign & ~(TA_RTLREADING |TA_CENTER |TA_RIGHT))
                     | (GetTextAlign(hdcDraw) & (TA_RTLREADING |TA_CENTER |TA_RIGHT)));

        /*
         * Setup font
         */
        if ((uFlags & DST_TYPEMASK) <= DST_TEXTMAX) {

            if (GetCurrentObject(hdcDraw, OBJ_FONT) != ghFontSys) {
                hFont = SelectObject(hdcDraw, ghFontSys);
                SelectObject(hdcDraw, hFont);
                hFontSave = SelectObject(ghdcGray, hFont);
            }
        }

    } else {

        hdcT = hdcDraw;

        /*
         * Adjust viewport
         */
        GetViewportOrgEx(hdcT, &ptOrg);
        SetViewportOrgEx(hdcT, ptOrg.x + x, ptOrg.y + y, NULL);
    }

    /*
     * Now, draw original image
     */
    fResult = TRUE;

    switch (uFlags & DST_TYPEMASK) {

        case DST_GLYPH:
            /*
             * Blt w/ current brush in hdcT
             */
            BitBltSysBmp(hdcT, 0, 0, LOWORD(lData));
            break;

        case DST_BITMAP:
            /*
             * Draw the bitmap.  If mono, it'll use the colors set up
             * in the dc.
             */
//            RtlEnterCriticalSection(&gcsHdcBits2);
            UserAssert(GetBkColor(ghdcBits2) == RGB(255, 255, 255));
            UserAssert(GetTextColor(ghdcBits2) == RGB(0, 0, 0));

            hbmpT = SelectObject(ghdcBits2, (HBITMAP)lData);
            BitBlt(hdcT, 0, 0, cx, cy, ghdcBits2, 0, 0, SRCCOPY);
            SelectObject(ghdcBits2, hbmpT);
//            RtlLeaveCriticalSection(&gcsHdcBits2);
            break;

        case DST_ICON:
            /*
             * Draw the icon.
             */
            DrawIconEx(hdcT, 0, 0, (HICON)lData, 0, 0, 0, NULL, DI_NORMAL);
            break;

        case DST_PREFIXTEXT:
            if (uFlags & DSS_HIDEPREFIX) {
                dwPSMFlags = DT_HIDEPREFIX;
            } else if (uFlags & DSS_PREFIXONLY) {
                dwPSMFlags = DT_PREFIXONLY;
            } else {
                dwPSMFlags = 0;
            }
            PSMTextOut(hdcT, 0, 0, (LPWSTR)lData, (int)wData, dwPSMFlags);
            break;

        case DST_TEXT:
            fResult = TextOutW(hdcT, 0, 0, (LPWSTR)lData, (int)wData);
            break;

        default:

            fResult = (qfnCallBack)(hdcT, lData, wData, cx, cy);

            /*
             * The callbacks could have altered the attributes of ghdcGray
             */
            if (hdcT == ghdcGray) {
                SetBkColor(ghdcGray, RGB(255, 255, 255));
                SetTextColor(ghdcGray, RGB(0, 0, 0));
                SelectObject(ghdcGray, GetStockObject(BLACK_BRUSH));
                SetBkMode(ghdcGray, OPAQUE);
            }
            break;
    }

    /*
     * Clean up
     */
    if (uFlags & DSS_MONO) {
        /*
         * Reset font
         */
        if (hFontSave)
            SelectObject(hdcT, hFontSave);
        SetTextAlign(hdcT, oldAlign);
    } else {
        /*
         * Reset DC.
         */
        SetViewportOrgEx(hdcT, ptOrg.x, ptOrg.y, NULL);
        goto CDS_Leave;
    }

    /*
     * UNION state
     * Dither over image
     * We want white pixels to stay white, in either dest or pattern.
     */
    if (uFlags & DSS_UNION) {

        POLYPATBLT PolyData;

        PolyData.x         = 0;
        PolyData.y         = 0;
        PolyData.cx        = cx;
        PolyData.cx        = cy;
        PolyData.BrClr.hbr = gpsi->hbrGray;

        PolyPatBlt(ghdcGray, PATOR, &PolyData, 1, PPB_BRUSH);
    }

    /*
     * DISABLED state
     * Emboss
     * Draw over-1/down-1 in hilight color, and in same position in shadow.
     *
     * DEFAULT state
     * Drop shadow
     * Draw over-1/down-1 in shadow color, and in same position in foreground
     * Draw offset down in shadow color,
     */
    if (uFlags & DSS_DISABLED) {

        BltColor(hdcDraw,
                 SYSHBR(3DHILIGHT),
                 ghdcGray,
                 x + 1,
                 y + 1,
                 cx,
                 cy,
                 0,
                 0,
                 BC_INVERT);

        BltColor(hdcDraw,
                 SYSHBR(3DSHADOW),
                 ghdcGray,
                 x,
                 y,
                 cx,
                 cy,
                 0,
                 0,
                 BC_INVERT);

    } else if (uFlags & DSS_DEFAULT) {

        BltColor(hdcDraw,
                 SYSHBR(3DSHADOW),
                 ghdcGray,
                 x+1,
                 y+1,
                 cx,
                 cy,
                 0,
                 0,
                 BC_INVERT);

        goto DrawNormal;

    } else {

DrawNormal:

        BltColor(hdcDraw, hbrFore, ghdcGray, x, y, cx, cy, 0, 0, BC_INVERT);
    }

CDS_Leave:

#ifdef USE_MIRRORING
    if (uFlags & DSS_MONO) {
        /*
         * Set the ghdcGray layout to 0, it is a public DC.
         */
        SetLayoutWidth(hdcT, -1, 0);
    }
#endif
    RtlLeaveCriticalSection(&gcsHdc);

    return fResult;
}

BOOL DrawStateA(HDC hDC, HBRUSH hBrush, DRAWSTATEPROC func,
    LPARAM lParam, WPARAM wParam, int x, int y, int cx, int cy, UINT wFlags) {

    LPARAM lpwstr = lParam;
    BOOL bRet;
    BOOL bFree;

    if (((wFlags & DST_TYPEMASK) == DST_TEXT) ||
        ((wFlags & DST_TYPEMASK) == DST_PREFIXTEXT)) {

        bFree = TRUE;

        if ((wParam = MBToWCS((LPSTR)lParam, wParam ? (int)wParam : USER_AWCONV_COUNTSTRINGSZ, &(LPWSTR)lpwstr, -1, TRUE)) == 0)
            return FALSE;
    } else {
        bFree = FALSE;
    }

    bRet = DrawStateW(hDC, hBrush, func, lpwstr, wParam, x, y, cx, cy, wFlags);

    if (bFree) {
        UserLocalFree((HANDLE)lpwstr);
    }
    return bRet;
}

/***************************************************************************\
* BltColor
*
* History:
\***************************************************************************/

void BltColor(
    HDC hdc,
    HBRUSH hbr,
    HDC hdcSrce,
    int xO,
    int yO,
    int cx,
    int cy,
    int xO1,
    int yO1,
    UINT uBltFlags
)
{
    HBRUSH hbrSave;
    HBRUSH hbrNew = NULL;
    DWORD  textColorSave;
    DWORD  bkColorSave;

    if (hbr == (HBRUSH)NULL) {
        LOGBRUSH lb;

        lb.lbStyle = BS_SOLID;
        lb.lbColor = GetSysColor(COLOR_WINDOWTEXT);
        hbrNew = hbr = CreateBrushIndirect(&lb);
    }

    /*
     * Set the Text and Background colors so that bltColor handles the
     * background of buttons (and other bitmaps) properly.
     * Save the HDC's old Text and Background colors.  This causes problems with
     * Omega (and probably other apps) when calling GrayString which uses this
     * routine...
     */
    textColorSave = SetTextColor(hdc, 0x00000000L);
    bkColorSave = SetBkColor(hdc, 0x00FFFFFFL);

    hbrSave = SelectObject(hdc, hbr);

    BitBlt(hdc, xO, yO, cx, cy, hdcSrce,
        xO1, yO1, ((uBltFlags & BC_INVERT) ? 0xB8074AL : 0xE20746L));
        //xO1, yO1, (fInvert ? 0xB80000 : 0xE20000));

    SelectObject(hdc, hbrSave);

    /*
     * Restore saved colors
     */
    SetTextColor(hdc, textColorSave);
    SetBkColor(hdc, bkColorSave);

    if (hbrNew) {
        DeleteObject(hbrNew);
    }
}
