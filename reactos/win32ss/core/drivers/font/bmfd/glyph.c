/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "bmfd.h"

FORCEINLINE
ULONG
_ReadPixel(
    CHAR* pjBits,
    ULONG x,
    ULONG y,
    ULONG ulHeight)
{
    CHAR j;
    j = pjBits[(x/8) * ulHeight + y];
    return (j >> (~x & 0x7)) & 1;
}


FORCEINLINE
VOID
_WritePixel(
    CHAR* pjBits,
    ULONG x,
    ULONG y,
    ULONG cjRow,
    ULONG color)
{
    pjBits += y * cjRow;
    pjBits += x / 8;
    *pjBits |= color << (~x & 0x7);
}


PBMFD_FONT
BmfdGetFontInstance(
    FONTOBJ *pfo,
    PBMFD_FACE pface)
{
    PBMFD_FONT pfont = pfo->pvProducer;
    XFORMOBJ *pxo;
    FLOATOBJ_XFORM xfo;

    if (!pfont)
    {
        /* Allocate realization info */
        pfont = EngAllocMem(0, sizeof(BMFD_FONT), 0);
        if (!pfont)
        {
            return NULL;
        }

        pxo = FONTOBJ_pxoGetXform(pfo);
        XFORMOBJ_iGetFloatObjXform(pxo, &xfo);

        pfont->pfo = pfo;
        pfont->pface = pface;
        pfont->xScale = FLOATOBJ_GetLong(&xfo.eM11);
        pfont->yScale = FLOATOBJ_GetLong(&xfo.eM22);
        pfont->ulAngle = 0;

        /* Set the pvProducer member of the fontobj */
        pfo->pvProducer = pfont;
    }

    return pfont;
}


ULONG
BmfdQueryGlyphAndBitmap(
    PBMFD_FONT pfont,
    HGLYPH hg,
    GLYPHDATA *pgd,
    GLYPHBITS *pgb,
    ULONG cjSize)
{
    PBMFD_FACE pface = pfont->pface;
    PGLYPHENTRY pge;
    ULONG xSrc, ySrc, cxSrc, cySrc;
    ULONG xDst, yDst, cxDst, cyDst;
    ULONG xScale, yScale;
    ULONG ulGlyphOffset, cjDstRow, color;
    PVOID pvSrc0, pvDst0;

    /* The glyph handle is the byte offset to the glyph in the table */
    pge = (PGLYPHENTRY)(pface->pCharTable + hg);

    /* Get the bitmap offset depending on file version */
    if (pface->ulVersion >= 0x300)
    {
        cxSrc = GETVAL(pge->ge20.geWidth);
        ulGlyphOffset = GETVAL(pge->ge30.geOffset);
    }
    else
    {
        cxSrc = GETVAL(pge->ge30.geWidth);
        ulGlyphOffset = GETVAL(pge->ge20.geOffset);
    }
    cySrc = pface->wPixHeight;

    /* Pointer to the bitmap bits */
    pvSrc0 = (PBYTE)pface->pFontInfo + ulGlyphOffset;
    pvDst0 = pgb->aj;

    xScale = pfont->xScale;
    yScale = pfont->yScale;

    /* Calculate extents of destination bitmap */
    if (pfont->ulAngle == 90 || pfont->ulAngle == 270)
    {
        cxDst = cySrc * xScale;
        cyDst = cxSrc * yScale;
    }
    else
    {
        cxDst = cxSrc * xScale;
        cyDst = cySrc * yScale;
    }
    cjDstRow = (cxDst + 7) / 8;

    if (pgd)
    {
        /* Fill GLYPHDATA structure */
        pgd->gdf.pgb = pgb;
        pgd->hg = hg;
        pgd->fxD = xScale * (pface->wA + cxDst + pface->wC) << 4;
        pgd->fxA = xScale * pface->wA << 4;
        pgd->fxAB = xScale * (pface->wA + cxDst) << 4;
        pgd->fxInkTop = yScale * pface->wAscent << 4;
        pgd->fxInkBottom = - yScale * (pface->wDescent << 4);
        pgd->rclInk.top = - yScale * pface->wAscent;
        pgd->rclInk.bottom = yScale * pface->wDescent;
        pgd->rclInk.left = xScale * pface->wA;
        pgd->rclInk.right = pgd->rclInk.left + cxDst;
        pgd->ptqD.x.LowPart = 0;
        pgd->ptqD.x.HighPart = pgd->fxD;
        pgd->ptqD.y.LowPart = 0;
        pgd->ptqD.y.HighPart = 0;
    }

    if (pgb)
    {
        /* Verify that the buffer is big enough */
        if (cjSize < FIELD_OFFSET(GLYPHBITS, aj) + cyDst * cjDstRow)
        {
            DbgPrint("Buffer too small (%ld), %ld,%ld\n",
                     cjSize, cxSrc, cySrc);
            return FD_ERROR;
        }

        /* Fill GLYPHBITS structure */
        pgb->ptlOrigin.x = xScale * pface->wA;
        pgb->ptlOrigin.y = - yScale * pface->wAscent;
        pgb->sizlBitmap.cx = cxDst;
        pgb->sizlBitmap.cy = cyDst;

        /* Erase destination surface */
        memset(pvDst0, 0, cyDst * cjDstRow);

        switch (pfont->ulAngle)
        {
            case 90:
                /* Copy pixels */
                for (yDst = 0; yDst < cyDst ; yDst++)
                {
                    xSrc = yDst / yScale;
                    for (xDst = 0; xDst < cxDst; xDst++)
                    {
                        ySrc = (cxDst - xDst) / xScale;
                        color = _ReadPixel(pvSrc0, xSrc, ySrc, cySrc);
                        _WritePixel(pvDst0, xDst, yDst, cjDstRow, color);
                    }
                }
                break;

            case 180:
                for (yDst = 0; yDst < cyDst ; yDst++)
                {
                    ySrc = (cyDst - yDst) / yScale;
                    for (xDst = 0; xDst < cxDst; xDst++)
                    {
                        xSrc = (cxDst - xDst) / xScale;
                        color = _ReadPixel(pvSrc0, xSrc, ySrc, cySrc);
                        _WritePixel(pvDst0, xDst, yDst, cjDstRow, color);
                    }
                }
                break;

            case 270:
                for (yDst = 0; yDst < cyDst ; yDst++)
                {
                    xSrc = (cyDst - yDst) / yScale;
                    for (xDst = 0; xDst < cxDst; xDst++)
                    {
                        ySrc = xDst / xScale;
                        color = _ReadPixel(pvSrc0, xSrc, ySrc, cySrc);
                        _WritePixel(pvDst0, xDst, yDst, cjDstRow, color);
                    }
                }
                break;

            case 0:
            default:
                for (yDst = 0; yDst < cyDst ; yDst++)
                {
                    ySrc = yDst / yScale;
                    for (xDst = 0; xDst < cxDst; xDst++)
                    {
                        xSrc = xDst / xScale;
                        color = _ReadPixel(pvSrc0, xSrc, ySrc, cySrc);
                        _WritePixel(pvDst0, xDst, yDst, cjDstRow, color);
                    }
                }
        }
    }

    /* Return the size of the GLYPHBITS structure */
    return FIELD_OFFSET(GLYPHBITS, aj) + cyDst * cjDstRow;
}

ULONG
BmfdQueryMaxExtents(
    PBMFD_FONT pfont,
    PFD_DEVICEMETRICS pfddm,
    ULONG cjSize)
{
    ULONG cjMaxWidth, cjMaxBitmapSize;
    PFONTINFO16 pFontInfo;
    ULONG xScale, yScale;

    if (pfddm)
    {
        if (cjSize < sizeof(FD_DEVICEMETRICS))
        {
            /* Not enough space, fail */
            return FD_ERROR;
        }

        pFontInfo = pfont->pface->pFontInfo;

        xScale = pfont->xScale;
        yScale = pfont->yScale;

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
        pfddm->lMinA = 0;
        pfddm->lMinC = 0;
        pfddm->lMinD = 0;

        if (pfont->ulAngle == 90 || pfont->ulAngle == 270)
        {
            pfddm->cxMax = xScale * GETVAL(pFontInfo->dfPixHeight);
            pfddm->cyMax = yScale * GETVAL(pFontInfo->dfMaxWidth);
            pfddm->fxMaxAscender = yScale * GETVAL(pFontInfo->dfAscent) << 4;
            pfddm->fxMaxDescender = (pfddm->cyMax << 4) - pfddm->fxMaxAscender;
        }
        else
        {
            pfddm->cxMax = xScale * GETVAL(pFontInfo->dfMaxWidth);
            pfddm->cyMax = yScale * GETVAL(pFontInfo->dfPixHeight);
            pfddm->fxMaxAscender = yScale * GETVAL(pFontInfo->dfAscent) << 4;
            pfddm->fxMaxDescender = (pfddm->cyMax << 4) - pfddm->fxMaxAscender;
        }

        pfddm->lD = pfddm->cxMax;

        /* Calculate Width in bytes */
        cjMaxWidth = ((pfddm->cxMax + 7) >> 3);

        /* Calculate size of the bitmap, rounded to DWORDs */
        cjMaxBitmapSize = ((cjMaxWidth * pfddm->cyMax) + 3) & ~3;

        /* cjGlyphMax is the full size of the GLYPHBITS structure */
        pfddm->cjGlyphMax = FIELD_OFFSET(GLYPHBITS, aj) + cjMaxBitmapSize;

        /* NOTE: fdxQuantized and NonLinear... stay unchanged */
    }

    /* Return the size of the structure */
    return sizeof(FD_DEVICEMETRICS);
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
    PBMFD_FILE pfile = (PBMFD_FILE)pfo->iFile;
    PBMFD_FACE pface = &pfile->aface[pfo->iFace - 1];
    PBMFD_FONT pfont= BmfdGetFontInstance(pfo, pface);

    DbgPrint("BmfdQueryFontData(pfo=%p, iMode=%ld, hg=%p, pgd=%p, pv=%p, cjSize=%ld)\n",
             pfo, iMode, hg, pgd, pv, cjSize);
//    __debugbreak();

    switch (iMode)
    {
        case QFD_GLYPHANDBITMAP: /* 1 */
            return BmfdQueryGlyphAndBitmap(pfont, hg, pgd, pv, cjSize);

        case QFD_MAXEXTENTS: /* 3 */
            return BmfdQueryMaxExtents(pfont, pv, cjSize);

            /* we support nothing else */
        default:
            return FD_ERROR;

    }

    return FD_ERROR;
}
