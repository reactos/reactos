/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "bmfd.h"

static
VOID
FillFDDM(
    PFD_DEVICEMETRICS pfddm,
    PFONTINFO16 pFontInfo)
{
    /* Fill FD_DEVICEMETRICS */
    pfddm->flRealizedType = FDM_MASK;
    pfddm->pteBase.x = FLOATL_1;
    pfddm->pteBase.y = 0;
    pfddm->pteSide.x = 0;
    pfddm->pteSide.y = FLOATL_1;
    pfddm->ptlUnderline1.x = 0;
    pfddm->ptlUnderline1.y = 1;
    pfddm->ptlStrikeout.x = 0;
    pfddm->ptlStrikeout.y = -4;
    pfddm->ptlULThickness.x = 0;
    pfddm->ptlULThickness.y = 1;
    pfddm->ptlSOThickness.x = 0;
    pfddm->ptlSOThickness.y = 1;
    pfddm->lD = GETVAL(pFontInfo->dfPixWidth);
    pfddm->cxMax = GETVAL(pFontInfo->dfMaxWidth);
    pfddm->cyMax = GETVAL(pFontInfo->dfPixHeight);
    pfddm->cjGlyphMax = pfddm->cyMax * ((pfddm->cxMax + 7) / 8);
    pfddm->fxMaxAscender = GETVAL(pFontInfo->dfAscent) << 4;
    pfddm->fxMaxDescender = (pfddm->cyMax << 4) - pfddm->fxMaxAscender;
    pfddm->lMinA = 0;
    pfddm->lMinC = 0;
    pfddm->lMinD = 0;

    /* NOTE: fdxQuantized and NonLinear... stay unchanged */
}

/** Public Interface **********************************************************/

PFD_GLYPHATTR
APIENTRY
BmfdQueryGlyphAttrs(
    FONTOBJ *pfo,
    ULONG iMode)
{
    DbgPrint("BmfdQueryGlyphAttrs()\n");
    /* We don't support FO_ATTR_MODE_ROTATE */
    return NULL;
}

LONG
APIENTRY
BmfdQueryFontData(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    ULONG iMode,
    HGLYPH hg,
    OUT GLYPHDATA *pgd,
    PVOID pv,
    ULONG cjSize)
{
    PDRVFONT pfont = (PDRVFONT)pfo->iFile;
    PDRVFACE pface = &pfont->aface[pfo->iFace - 1];
    PGLYPHENTRY pge = (PGLYPHENTRY)hg;
    ULONG ulGlyphOffset, ulWidthBytes, ulPixWidth, ulPixHeight, x, y, cjRow;

    DbgPrint("BmfdQueryFontData(pfo=%p, iMode=%ld, hg=%p, pgd=%p, pv=%p, cjSize=%ld)\n", 
             pfo, iMode, hg, pgd, pv, cjSize);
//    DbgBreakPoint();

    switch (iMode)
    {
        case QFD_GLYPHANDBITMAP: /* 1 */
        {
            GLYPHBITS *pgb = pv;
            BYTE j, *pjGlyphBits;

//            DbgPrint("QFD_GLYPHANDBITMAP, hg=%p, pgd=%p, pv=%p, cjSize=%d\n",
//                     hg, pgd, pv, cjSize);

            if (!hg)
            {
                DbgPrint("no glyph handle given!\n");
                return FD_ERROR;
            }

            /* Get the bitmap offset depending on file version */
            if (pface->ulVersion >= 0x300)
            {
                ulPixWidth = GETVAL(pge->ge20.geWidth);
                ulGlyphOffset = GETVAL(pge->ge30.geOffset);
            }
            else
            {
                ulPixWidth = GETVAL(pge->ge30.geWidth);
                ulGlyphOffset = GETVAL(pge->ge20.geOffset);
            }
            ulPixHeight = pface->wPixHeight;

            /* Calculate number of bytes per row for gdi */
            cjRow = (ulPixWidth + 7) / 8; // FIXME: other bpp values

            if (pgd)
            {
                /* Fill GLYPHDATA structure */
                pgd->gdf.pgb = pgb;
                pgd->hg = hg;
                pgd->fxD = (pface->wA + ulPixWidth + pface->wC) << 4;
                pgd->fxA = pface->wA << 4;
                pgd->fxAB = (pface->wA + ulPixWidth) << 4;
                pgd->fxInkTop = pface->wAscent << 4;
                pgd->fxInkBottom = - (pface->wDescent << 4);
                pgd->rclInk.top = - pface->wAscent;
                pgd->rclInk.bottom = pface->wDescent ;
                pgd->rclInk.left = pface->wA;
                pgd->rclInk.right = pface->wA + ulPixWidth;
                pgd->ptqD.x.LowPart = 0;
                pgd->ptqD.x.HighPart = pgd->fxD;
                pgd->ptqD.y.LowPart = 0;
                pgd->ptqD.y.HighPart = 0;
            }

            if (pgb)
            {
//                DbgBreakPoint();

                /* Verify that the buffer is big enough */
                if (cjSize < ulPixHeight * cjRow)
                {
                    DbgPrint("Buffer too small (%ld), %ld,%ld\n", 
                             cjSize, ulPixWidth, ulPixHeight);
                    return FD_ERROR;
                }

                /* Fill GLYPHBITS structure */
                pgb->ptlOrigin.x = pface->wA;
                pgb->ptlOrigin.y = - pface->wAscent;
                pgb->sizlBitmap.cx = ulPixWidth;
                pgb->sizlBitmap.cy = ulPixHeight;

                /* Copy the bitmap bits */
                pjGlyphBits = (PBYTE)pface->pFontInfo + ulGlyphOffset;
                ulWidthBytes = pface->wWidthBytes;
                for (y = 0; y < ulPixHeight; y++)
                {
                    for (x = 0; x < cjRow; x++)
                    {
                        j = pjGlyphBits[x * ulPixHeight + y];
                        pgb->aj[y * cjRow + x] = j;
                    }
                }
                
                DbgPrint("iFace=%ld, ulGlyphOffset=%lx, ulPixHeight=%ld, cjRow=%ld\n", 
                         pfo->iFace, ulGlyphOffset, ulPixHeight, cjRow);
                DbgBreakPoint();
            }

            /* Return the size of the bitmap buffer */
            return ulPixHeight * cjRow;
        }

        case QFD_MAXEXTENTS: /* 3 */
        {
            if (pv)
            {
                if (cjSize < sizeof(FD_DEVICEMETRICS))
                {
                    /* Not enough space, fail */
                    return FD_ERROR;
                }

                /* Fill the PFD_DEVICEMETRICS structure */
                FillFDDM((PFD_DEVICEMETRICS)pv, pface->pFontInfo);
            }

            /* Return the size of the structure */
            return sizeof(FD_DEVICEMETRICS);
        }

            /* we support nothing else */
        default:
            return FD_ERROR;

    }

    return FD_ERROR;
}
