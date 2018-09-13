/***************************************************************************\
* Module Name: wmicon.c
*
* Icon Drawing Routines
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* 22-Jan-1991 MikeKe  from win30
* 13-Jan-1994 JohnL   rewrote from Chicago (m5)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define SetBestStretchMode(hdc, bpp, fHT)                                \
    GreSetStretchBltMode(hdc,                                            \
                         (fHT ? HALFTONE :                               \
                             ((bpp) == 1 ? BLACKONWHITE : COLORONCOLOR)))

#define GetCWidth(cxOrg, lrF, cxDes) \
    (cxOrg ? cxOrg : ((lrF & DI_DEFAULTSIZE) ? SYSMET(CXICON) : cxDes))

#define GetCHeight(cyOrg, lrF, cyDes) \
    (cyOrg ? cyOrg : ((lrF & DI_DEFAULTSIZE) ? SYSMET(CYICON) : cyDes))

/***************************************************************************\
* BltIcon
*
*
\***************************************************************************/

BOOL BltIcon(
    HDC     hdc,
    int     x,
    int     y,
    int     cx,
    int     cy,
    HDC     hdcSrc,
    PCURSOR pcur,
    BOOL    fMask,
    LONG    rop)
{
    HBITMAP hbmpSave;
    HBITMAP hbmpUse;
    LONG    rgbText;
    LONG    rgbBk;
    int     nMode;

    /*
     * Setup the DC for drawing
     */
    hbmpUse = (fMask || !pcur->hbmColor ? pcur->hbmMask : pcur->hbmColor);

    rgbBk   = GreSetBkColor(hdc, 0x00FFFFFFL);
    rgbText = GreSetTextColor(hdc, 0x00000000L);
    nMode   = SetBestStretchMode(hdc, pcur->bpp, FALSE);

    hbmpSave = GreSelectBitmap(hdcSrc, hbmpUse);

    /*
     * Do the output to the surface.  By passing in (-1) as the background
     * color, we are telling GDI to use the background-color already set
     * in the DC.
     */
    GreStretchBlt(hdc,
                  x,
                  y,
                  cx,
                  cy,
                  hdcSrc,
                  0,
                  (fMask || pcur->hbmColor ? 0 : pcur->cy / 2),
                  pcur->cx,
                  pcur->cy / 2,
                  rop,
                  (COLORREF)-1);

    GreSetStretchBltMode(hdc, nMode);
    GreSetTextColor(hdc, rgbText);
    GreSetBkColor(hdc, rgbBk);

    GreSelectBitmap(hdcSrc, hbmpSave);

    return TRUE;
}

/***************************************************************************\
* DrawIconEx
*
* Draws icon in desired size.
*
\***************************************************************************/
BOOL _DrawIconEx(
    HDC     hdc,
    int     x,
    int     y,
    PCURSOR pcur,
    int     cx,
    int     cy,
    UINT    istepIfAniCur,
    HBRUSH  hbr,
    UINT    diFlags)
{
    BOOL fSuccess = FALSE;

    /*
     * If this is an animated cursor, just grab the ith frame and use it
     * for drawing.
     */
    if (pcur->CURSORF_flags & CURSORF_ACON) {

        if ((int)istepIfAniCur >= ((PACON)pcur)->cicur) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "DrawIconEx, icon step out of range.");
            goto Done;
        }

        pcur = ((PACON)pcur)->aspcur[((PACON)pcur)->aicur[istepIfAniCur]];
    }

    /*
     * Setup defaults.
     */
    cx = GetCWidth(cx, diFlags, pcur->cx);
    cy = GetCHeight(cy, diFlags, (pcur->cy / 2));

    if (hbr) {

        HBITMAP    hbmpT = NULL;
        HDC        hdcT;
        HBITMAP    hbmpOld;
        POLYPATBLT PolyData;

        if (hdcT = GreCreateCompatibleDC(hdc)) {

            if (hbmpT = GreCreateCompatibleBitmap(hdc, cx, cy)) {
                POINT pt;
                BOOL bRet;

                hbmpOld = GreSelectBitmap(hdcT, hbmpT);

                /*
                 * Set new dc's brush origin in same relative
                 * location as passed-in dc's.
                 */
                bRet = GreGetBrushOrg(hdc, &pt);
                /*
                 * Bug 292396 - joejo
                 * Stop overactive asserts by replacing with RIPMSG.
                 */
                if (bRet != TRUE) {
                    RIPMSG0(RIP_WARNING, "DrawIconEx, GreGetBrushOrg failed.");
                }

                bRet = GreSetBrushOrg(hdcT, pt.x, pt.y, NULL);
                if (bRet != TRUE) {
                    RIPMSG0(RIP_WARNING, "DrawIconEx, GreSetBrushOrg failed.");
                }

                PolyData.x         = 0;
                PolyData.y         = 0;
                PolyData.cx        = cx;
                PolyData.cy        = cy;
                PolyData.BrClr.hbr = hbr;

                bRet = GrePolyPatBlt(hdcT, PATCOPY, &PolyData, 1, PPB_BRUSH);
                if (bRet != TRUE) {
                    RIPMSG0(RIP_WARNING, "DrawIconEx, GrePolyPatBlt failed.");
                }
                
                /*
                 * Output the image to the temporary memoryDC.
                 */
                BltIcon(hdcT, 0, 0, cx, cy, ghdcMem, pcur, TRUE, SRCAND);
                BltIcon(hdcT, 0, 0, cx, cy, ghdcMem, pcur, FALSE, SRCINVERT);

                /*
                 * Blt the bitmap to the original DC.
                 */
                GreBitBlt(hdc, x, y, cx, cy, hdcT, 0, 0, SRCCOPY, (COLORREF)-1);

                GreSelectBitmap(hdcT, hbmpOld);

                bRet = GreDeleteObject(hbmpT);
                if (bRet != TRUE) {
                    RIPMSG0(RIP_WARNING, "DrawIconEx, GreDeleteObject failed. Possible Leak");
                }
                
                fSuccess = TRUE;
            }

            GreDeleteDC(hdcT);
        }

    } else {

        if (diFlags & DI_MASK) {

            BltIcon(hdc,
                    x,
                    y,
                    cx,
                    cy,
                    ghdcMem,
                    pcur,
                    TRUE,
                    ((diFlags & DI_IMAGE) ? SRCAND : SRCCOPY));
        }

        if (diFlags & DI_IMAGE) {

            BltIcon(hdc,
                    x,
                    y,
                    cx,
                    cy,
                    ghdcMem,
                    pcur,
                    FALSE,
                    ((diFlags & DI_MASK) ? SRCINVERT : SRCCOPY));
        }

        fSuccess = TRUE;
    }

Done:

    return fSuccess;
}
