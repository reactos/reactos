/**************************** Module Header ********************************\
* Module Name: mngray.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Server-side version of DrawState
*
* History:
* 06-Jan-1993 FritzS    Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define PATOR           0x00FA0089L
#define SRCSTENCIL      0x00B8074AL
#define SRCINVSTENCIL   0x00E20746L

/***************************************************************************\
 *
 * CreateCompatiblePublicDC
 *
 * This is used in several callback routines to the lpk(s).  We can't
 * pass G_TERM(pDispInfo)->hdcGray or HDCBITS() to the client since they are public DCs.
 * We can't just change the owner since we're about to leave the
 * critical section.  Some other thread may enter before we return
 * and use hdcGray or HDCBITS().  Instead, we create a compatible dc with the same
 * font and bitmap that are currently selected in hdcGray (or HDCBITS()).  Pass that
 * to the client lpk.
 *
 * If the function returns successfully , then the dc and bitmap object are
 * guaranteed to be successfully created.
 *
 * History:
 *
 * Dec-16-1997  Samer Arafeh  [samera]
 * Jan-20-1998  Samer Arafeh  [samera] Add support for both hdcGray and HDCBITS()
 *
\***************************************************************************/
HDC CreateCompatiblePublicDC(
    HDC      hdcPublic,
    HBITMAP *pbmPublicDC)
{
    HDC     hdcCompatible = 0;
    HBITMAP hbmCompatible;
    HFONT   hFont;
    int     cx,cy;

    if ((hdcPublic != gpDispInfo->hdcGray) && (hdcPublic != HDCBITS())) {
        return hdcPublic;
    }

    if ((hdcCompatible = GreCreateCompatibleDC(hdcPublic)) == NULL) {
        RIPMSG1(RIP_WARNING, "CreateCompatiblePublicDC: GreCreateCompatibleDC Failed %lX", hdcPublic);
        return (HDC)NULL;
    }
    if (!GreSetDCOwner(hdcCompatible, OBJECT_OWNER_CURRENT)) {
        RIPMSG1(RIP_WARNING, "CreateCompatiblePublicDC: SetDCOwner Failed %lX", hdcCompatible);
        GreDeleteDC(hdcCompatible);
        return (HDC)NULL;
    }

    if (hdcPublic == gpDispInfo->hdcGray) {
        cx = gpDispInfo->cxGray;
        cy = gpDispInfo->cxGray;
    } else {
        if (hdcPublic == HDCBITS()) {
            BITMAP bmBits;

            GreExtGetObjectW(ghbmBits, sizeof(BITMAP), &bmBits);
            cx = bmBits.bmWidth;
            cy = bmBits.bmHeight;
        } else {

          //
          // We should not reach here.
          //
          RIPMSG1(RIP_ERROR, "CreateCompatiblePublicDC: GreCreateCompatibleDC Failed %lX", hdcPublic);
        }

    }

    hbmCompatible = GreCreateCompatibleBitmap(hdcPublic,
                                              cx,
                                              cy);

    //
    // Check whether bitmap couldn't be created or can't
    // be set to OBJECT_OWNER_CURRENT, then fail and
    // do necessary cleanup now!
    //

    if( (hbmCompatible == NULL) ||
        (!GreSetBitmapOwner(hbmCompatible, OBJECT_OWNER_CURRENT)) ) {

        RIPMSG1(RIP_WARNING, "CreateCompatiblePublicDC: GreCreateCompatibleBitmap Failed %lX", hbmCompatible);
        GreDeleteDC( hdcCompatible );

        if( hbmCompatible ) {
            GreDeleteObject( hbmCompatible );
        }

        return (HDC)NULL;
    }

    GreSelectBitmap(hdcCompatible, hbmCompatible);
    /*
     * Make sure we use the same font and text alignment.
     */
    hFont = GreSelectFont(hdcPublic, ghFontSys);
    GreSelectFont(hdcPublic, hFont);
    GreSelectFont(hdcCompatible, hFont);
    GreSetTextAlign(hdcCompatible, GreGetTextAlign(hdcPublic));
    /*
     * Copy any information already written into G_TERM(pDispInfo)->hdcGray.
     */

#ifdef USE_MIRRORING
    //
    // Mirror the created DC if the hdcGray is currently mirrored,
    // so that TextOut won't get mirrored on User Client DCs
    //
    if (GreGetLayout(hdcPublic) & LAYOUT_RTL)
        GreSetLayout(hdcCompatible, cx - 1, LAYOUT_RTL);
#endif
    GreBitBlt(hdcCompatible, 0, 0, cx, cy, hdcPublic, 0, 0, SRCCOPY, 0);

    *pbmPublicDC = hbmCompatible ;      // for later deletion, by the server side

    return hdcCompatible;
}



/***************************************************************************\
*
*  xxxDrawState()
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
\***************************************************************************/
BOOL xxxDrawState(
    HDC           hdcDraw,
    HBRUSH        hbrFore,
    LPARAM        lData,
    int           x,
    int           y,
    int           cx,
    int           cy,
    UINT          uFlags)
{
    HFONT   hFont;
    HFONT   hFontSave = NULL;
    HDC     hdcT;
    HBITMAP hbmpT;
    POINT   ptOrg;
    BOOL    fResult;
    int     oldAlign;
#ifdef USE_MIRRORING
    DWORD   dwOldLayout=0;
#endif

    /*
     * These require monochrome conversion
     *
     * Enforce monochrome: embossed doesn't look great with 2 color displays
     */
    if ((uFlags & DSS_DISABLED) &&
        ((gpsi->BitCount == 1) || SYSMET(SLOWMACHINE))) {

        uFlags &= ~DSS_DISABLED;
        uFlags |= DSS_UNION;
    }

    if (uFlags & (DSS_INACTIVE | DSS_DISABLED | DSS_DEFAULT | DSS_UNION))
        uFlags |= DSS_MONO;

    /*
     * Validate flags - we only support DST_COMPLEX in kernel
     */
    if ((uFlags & DST_TYPEMASK) != DST_COMPLEX) {
        RIPMSG1(RIP_ERROR, "xxxDrawState: invalid DST_ type %x", (uFlags & DST_TYPEMASK));
        return FALSE;
    }

    /*
     * Optimize:  nothing to draw
     */
    if (!cx || !cy) {
        return TRUE;
    }

    /*
     * Setup drawing dc
     */
    if (uFlags & DSS_MONO) {

        hdcT = gpDispInfo->hdcGray;
#ifdef USE_MIRRORING
        /*
         * First turn off mirroring on hdcGray if any.
         */
        GreSetLayout(hdcT, -1, 0);
        /*
         * Set the hdcGray layout to be equal to the screen hdcDraw layout.
         */
        dwOldLayout = GreGetLayout(hdcDraw);
        if (dwOldLayout != GDI_ERROR) {
            GreSetLayout(hdcT, cx, dwOldLayout);
        }
#endif

        /*
         * Is our scratch bitmap big enough?  We need potentially
         * cx+1 by cy pixels for default etc.
         */
        if ((gpDispInfo->cxGray < cx + 1) || (gpDispInfo->cyGray < cy)) {

            if (hbmpT = GreCreateBitmap(max(gpDispInfo->cxGray, cx + 1), max(gpDispInfo->cyGray, cy), 1, 1, 0L)) {

                HBITMAP hbmGray;

                hbmGray = GreSelectBitmap(gpDispInfo->hdcGray, hbmpT);
                GreDeleteObject(hbmGray);

                GreSetBitmapOwner(hbmpT, OBJECT_OWNER_PUBLIC);

                gpDispInfo->cxGray = max(gpDispInfo->cxGray, cx + 1);
                gpDispInfo->cyGray = max(gpDispInfo->cyGray, cy);

            } else {
                cx = gpDispInfo->cxGray - 1;
                cy = gpDispInfo->cyGray;
            }
        }

        GrePatBlt(gpDispInfo->hdcGray,
                  0,
                  0,
                  gpDispInfo->cxGray,
                  gpDispInfo->cyGray,
                  WHITENESS);

        GreSetTextCharacterExtra(gpDispInfo->hdcGray,
                                 GreGetTextCharacterExtra(hdcDraw));

        oldAlign = GreGetTextAlign(hdcT);
        GreSetTextAlign(hdcT, (oldAlign & ~(TA_RTLREADING |TA_CENTER |TA_RIGHT))
                     | (GreGetTextAlign(hdcDraw) & (TA_RTLREADING |TA_CENTER |TA_RIGHT)));
        /*
         * Setup font
         */
        if (GreGetHFONT(hdcDraw) != ghFontSys) {
            hFont = GreSelectFont(hdcDraw, ghFontSys);
            GreSelectFont(hdcDraw, hFont);
            hFontSave = GreSelectFont(gpDispInfo->hdcGray, hFont);
        }
    } else {
        hdcT = hdcDraw;
        /*
         * Adjust viewport
         */
        GreGetViewportOrg(hdcT, &ptOrg);
        GreSetViewportOrg(hdcT, ptOrg.x+x, ptOrg.y+y, NULL);

    }

    /*
     * Now, draw original image
     */
    fResult = xxxRealDrawMenuItem(hdcT, (PGRAYMENU)lData, cx, cy);

    /*
     * The callbacks could have altered the attributes of hdcGray
     */
    if (hdcT == gpDispInfo->hdcGray) {
        GreSetBkColor(gpDispInfo->hdcGray, RGB(255, 255, 255));
        GreSetTextColor(gpDispInfo->hdcGray, RGB(0, 0, 0));
        GreSelectBrush(gpDispInfo->hdcGray, ghbrBlack);
        GreSetBkMode(gpDispInfo->hdcGray, OPAQUE);
    }

    /*
     * Clean up
     */
    if (uFlags & DSS_MONO) {

        /*
         * Reset font
         */
        if (hFontSave)
            GreSelectFont(hdcT, hFontSave);
        GreSetTextAlign(hdcT, oldAlign);
    } else {
        /*
         * Reset DC.
         */
        GreSetViewportOrg(hdcT, ptOrg.x, ptOrg.y, NULL);
        return TRUE;
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
         PolyData.cy        = cy;
         PolyData.BrClr.hbr = gpsi->hbrGray;

         GrePolyPatBlt(gpDispInfo->hdcGray, PATOR, &PolyData, 1, PPB_BRUSH);
    }

    if (uFlags & DSS_INACTIVE) {

        BltColor(hdcDraw,
                 SYSHBR(3DSHADOW),
                 gpDispInfo->hdcGray,
                 x,
                 y,
                 cx,
                 cy,
                 0,
                 0,
                 BC_INVERT);

    } else if (uFlags & DSS_DISABLED) {

        /*
         * Emboss
         * Draw over-1/down-1 in hilight color, and in same position in shadow.
         */

        BltColor(hdcDraw,
                 SYSHBR(3DHILIGHT),
                 gpDispInfo->hdcGray,
                 x+1,
                 y+1,
                 cx,
                 cy,
                 0,
                 0,
                 BC_INVERT);

        BltColor(hdcDraw,
                 SYSHBR(3DSHADOW),
                 gpDispInfo->hdcGray,
                 x,
                 y,
                 cx,
                 cy,
                 0,
                 0,
                 BC_INVERT);

    } else if (uFlags & DSS_DEFAULT) {

        BltColor(hdcDraw,
                 hbrFore,
                 gpDispInfo->hdcGray,
                 x,
                 y,
                 cx,
                 cy,
                 0,
                 0,
                 BC_INVERT);

        BltColor(hdcDraw,
                 hbrFore,
                 gpDispInfo->hdcGray,
                 x+1,
                 y,
                 cx,
                 cy,
                 0,
                 0,
                 BC_INVERT);

    } else {

        BltColor(hdcDraw,
                 hbrFore,
                 gpDispInfo->hdcGray,
                 x,
                 y,
                 cx,
                 cy,
                 0,
                 0,
                 BC_INVERT);
    }


#ifdef USE_MIRRORING
    if ((uFlags & DSS_MONO)){
        /*
         * Set the hdcGray layout to 0, it is a public DC.
         */
       GreSetLayout(hdcT, -1, 0);
    }
#endif
    return fResult;
}

/***************************************************************************\
* BltColor
*
* <brief description>
*
* History:
* 13-Nov-1990 JimA      Ported from Win3.
\***************************************************************************/

VOID BltColor(
    HDC    hdc,
    HBRUSH hbr,
    HDC    hdcSrce,
    int    xO,
    int    yO,
    int    cx,
    int    cy,
    int    xO1,
    int    yO1,
    UINT   uBltFlags)
{
    HBRUSH hbrSave;
    DWORD  textColorSave;
    DWORD  bkColorSave;
    DWORD  ROP;

    /*
     * Set the Text and Background colors so that bltColor handles the
     * background of buttons (and other bitmaps) properly.
     * Save the HDC's old Text and Background colors.  This causes problems
     * with Omega (and probably other apps) when calling GrayString which
     * uses this routine...
     */
    textColorSave = GreSetTextColor(hdc, 0x00000000L);
    bkColorSave = GreSetBkColor(hdc, 0x00FFFFFFL);

    if (hbr != NULL)
        hbrSave = GreSelectBrush(hdc, hbr);
    if (uBltFlags & BC_INVERT)
        ROP = 0xB8074AL;
    else
        ROP = 0xE20746L;

    if (uBltFlags & BC_NOMIRROR)
        ROP |= NOMIRRORBITMAP;

    GreBitBlt(hdc,
              xO,
              yO,
              cx,
              cy,
              hdcSrce ? hdcSrce : gpDispInfo->hdcGray,
              xO1,
              yO1,
              ROP,
              0x00FFFFFF);

    if (hbr != NULL)
        GreSelectBrush(hdc, hbrSave);

    /*
     * Restore saved colors
     */
    GreSetTextColor(hdc, textColorSave);
    GreSetBkColor(hdc, bkColorSave);
}
