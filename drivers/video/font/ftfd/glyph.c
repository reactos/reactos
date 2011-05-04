/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver based on freetype
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "ftfd.h"

/** Private Interface *********************************************************/

#define GLYPHBITS_SIZE(cx, cy, bpp) \
    (FIELD_OFFSET(GLYPHBITS, aj) + ((((cx * bpp) + 31) >> 5) * cy * 4))

PFTFD_FONT
NTAPI
FtfdCreateFontInstance(
    FONTOBJ *pfo)
{
    PFTFD_FILE pfile = (PFTFD_FILE)pfo->iFile;
    PFTFD_FONT pfont;
    XFORMOBJ* pxo;
    FT_Error fterror;
    FT_Face ftface;

    /* Allocate a font structure */
    pfont = EngAllocMem(0, sizeof(FTFD_FONT), 0);
    if (!pfont)
    {
        return NULL;
    }

    /* Set basic fields */
    pfont->pfo = pfo;
    pfont->pfile = pfile;
    pfont->iFace = pfo->iFace;
    pfont->hgSelected = -1;

    /* Create a freetype face */
    fterror = FT_New_Memory_Face(gftlibrary,
                                 pfile->pvView,
                                 pfile->cjView,
                                 pfo->iFace - 1,
                                 &ftface);
    if (fterror)
    {
        /* Failure! */
        DbgPrint("Error creating face\n");
        return NULL;
    }

    pxo = FONTOBJ_pxoGetXform(pfo);

    fterror = FT_Set_Char_Size(ftface,
                               0,
                               16 * 64,
                               pfo->sizLogResPpi.cx,
                               pfo->sizLogResPpi.cy);
    if (fterror)
    {
        /* Failure! */
        DbgPrint("Error setting face size\n");
        return NULL;
    }

    /* Set non-orthogonal transformation */
    // FT_Set_Transform

    pfont->ftface = ftface;

    /* Set the pvProducer member of the fontobj */
    pfo->pvProducer = pfont;
    return pfont;
}

static
PFTFD_FONT
FtfdGetFontInstance(
    FONTOBJ *pfo)
{
    PFTFD_FONT pfont = pfo->pvProducer;

    if (!pfont)
    {
        pfont = FtfdCreateFontInstance(pfo);
    }

    /* Return the font instance */
    return pfont;
}

#define FLOATL_1 0x3f800000

ULONG
NTAPI
FtfdQueryMaxExtents(
    FONTOBJ *pfo,
    PFD_DEVICEMETRICS pfddm,
    ULONG cjSize)
{
    PFTFD_FONT pfont = FtfdGetFontInstance(pfo);
    ULONG cjMaxWidth, cjMaxBitmapSize;

    DbgPrint("FtfdQueryMaxExtents\n");

    if (pfddm)
    {
        if (cjSize < sizeof(FD_DEVICEMETRICS))
        {
            /* Not enough space, fail */
            DbgPrint("ERROR: cjSize = %ld\n", cjSize);
            return FD_ERROR;
        }

        //xScale = pfont->xScale;
        //yScale = pfont->yScale;

        /* Fill FD_DEVICEMETRICS */
        pfddm->flRealizedType = 0;
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

#if 0
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
#endif

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

static
BOOL
FtfdLoadGlyph(
    PFTFD_FONT pfont,
    HGLYPH hg,
    ULONG iFormat) // 0 = bitmap, 1 = outline
{
    FT_Error fterror;

    /* Check if the glyph needs to be updated */
    if (pfont->hgSelected != hg)
    {
        /* Load the glypg into the freetype face slot */
        fterror = FT_Load_Glyph(pfont->ftface, hg, 0);
        if (fterror)
        {
            DbgPrint("FtfdLoadGlyph: couldn't load glyph 0x%lx\n", hg);
            pfont->hgSelected = -1;
            return FALSE;
        }

        /* Update the selected glyph */
        pfont->hgSelected = hg;
    }

    return TRUE;
}

static
VOID
FtfdQueryGlyphData(
    FONTOBJ *pfo,
    HGLYPH hg,
    GLYPHDATA *pgd,
    PVOID pvGlyphData)
{
    PFTFD_FONT pfont = pfo->pvProducer;
    FT_GlyphSlot ftglyph = pfont->ftface->glyph;

if (hg > 1) __debugbreak();

    pgd->gdf.pgb = pvGlyphData;
    pgd->hg = hg;

    if (1 /* layout horizontal */)
    {
        pgd->fxA = ftglyph->metrics.horiBearingX;
        pgd->fxAB = pgd->fxA + ftglyph->metrics.width;
        pgd->fxD = ftglyph->metrics.horiAdvance;
    }
    else
    {
        pgd->fxA = ftglyph->metrics.vertBearingX;
        pgd->fxAB = pgd->fxA + ftglyph->metrics.height;
        pgd->fxD = ftglyph->metrics.vertAdvance;
    }

    pgd->fxInkTop = 0;
    pgd->fxInkBottom = 0;
    pgd->rclInk.left = ftglyph->bitmap_left;
    pgd->rclInk.top = ftglyph->bitmap_top;
    pgd->rclInk.right = pgd->rclInk.left + ftglyph->bitmap.width;
    pgd->rclInk.bottom = pgd->rclInk.top + ftglyph->bitmap.rows;

    /* Make the bitmap at least 1x1 pixel */
    if (ftglyph->bitmap.width == 0) pgd->rclInk.right++;
    if (ftglyph->bitmap.rows == 0) pgd->rclInk.bottom++;

    pgd->ptqD.x.LowPart = 0;
    pgd->ptqD.x.HighPart = pgd->fxD;
    pgd->ptqD.y.LowPart = 0;
    pgd->ptqD.y.HighPart = 0;
    //pgd->ptqD.x.QuadPart = 0;
    //pgd->ptqD.y.QuadPart = 0;


if (hg > 1) __debugbreak();

}

VOID
FtfdQueryGlyphBits(
    FONTOBJ *pfo,
    HGLYPH hg,
    GLYPHBITS *pgb,
    ULONG cjSize)
{
    PFTFD_FONT pfont = pfo->pvProducer;
    FT_GlyphSlot ftglyph = pfont->ftface->glyph;

if (hg > 1) __debugbreak();

    pgb->ptlOrigin.x = 0;
    pgb->ptlOrigin.y = 0;
    pgb->sizlBitmap.cx = ftglyph->bitmap.width;
    pgb->sizlBitmap.cy = ftglyph->bitmap.rows;

if (hg > 1) __debugbreak();
}

VOID
FtfdQueryGlyphOutline(
    FONTOBJ *pfo,
    HGLYPH hg,
    PATHOBJ *ppo,
    ULONG cjSize)
{

}

/** Public Interface **********************************************************/

LONG
APIENTRY
FtfdQueryFontData(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    ULONG iMode,
    HGLYPH hg,
    OUT GLYPHDATA *pgd,
    PVOID pv,
    ULONG cjSize)
{
    PFTFD_FONT pfont = FtfdGetFontInstance(pfo);

    DbgPrint("FtfdQueryFontData, iMode=%ld, hg=%lx, pgd=%p, pv=%p, cjSize=%ld\n",
             iMode, hg, pgd, pv, cjSize);

    switch (iMode)
    {
        case QFD_GLYPHANDBITMAP:
            DbgPrint("QFD_GLYPHANDBITMAP\n");

            /* Load the requested glyph */
            if (!FtfdLoadGlyph(pfont, hg, 0)) return FD_ERROR;

            if (pgd) FtfdQueryGlyphData(pfo, hg, pgd, pv);


            if (pv)
            {
                FtfdQueryGlyphBits(pfo, hg, pv, cjSize);
            }

            /* Return the size for a 1bpp bitmap */
            return GLYPHBITS_SIZE(pfont->ftface->glyph->bitmap.width,
                                  pfont->ftface->glyph->bitmap.rows,
                                  1);

        case QFD_GLYPHANDOUTLINE:
            DbgPrint("QFD_GLYPHANDOUTLINE\n");

            /* Load the requested glyph */
            if (!FtfdLoadGlyph(pfont, hg, 1)) return FD_ERROR;

            if (pgd)
            {
                FtfdQueryGlyphData(pfo, hg, pgd, pv);
            }

            if (pv)
            {
                FtfdQueryGlyphOutline(pfo, hg, pv, cjSize);
            }
            break;

        case QFD_MAXEXTENTS:
            return FtfdQueryMaxExtents(pfo, pv, cjSize);

        case QFD_TT_GRAY1_BITMAP:
            DbgPrint("QFD_TT_GRAY1_BITMAP\n");
            break;
        case QFD_TT_GRAY2_BITMAP:
            DbgPrint("QFD_TT_GRAY2_BITMAP\n");
            break;
        case QFD_TT_GRAY4_BITMAP:
            DbgPrint("QFD_TT_GRAY4_BITMAP\n");
            break;
        case QFD_TT_GRAY8_BITMAP:
            DbgPrint("QFD_TT_GRAY8_BITMAP\n");
            break;
        default:
            DbgPrint("Impossible iMode value: %lx\n", iMode);
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return FD_ERROR;
    }

    __debugbreak();


    return FD_ERROR;
}

PFD_GLYPHATTR
APIENTRY
FtfdQueryGlyphAttrs(
    FONTOBJ *pfo,
    ULONG iMode)
{
    DbgPrint("FtfdQueryGlyphAttrs\n");

    /* Verify parameters */
    if (!pfo || iMode != FO_ATTR_MODE_ROTATE)
    {
        DbgPrint("ERROR: invalid parameters: %p, %ld\n", pfo, iMode);
        return NULL;
    }



    __debugbreak();
    return NULL;
}

BOOL
APIENTRY
FtfdQueryAdvanceWidths(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    ULONG iMode,
    HGLYPH *phg,
    PVOID pvWidths,
    ULONG cGlyphs)
{
    PFTFD_FONT pfont = FtfdGetFontInstance(pfo);
    PUSHORT pusWidths = pvWidths;
    BOOL bResult = TRUE;
    ULONG i, fl;
    FT_Face ftface = pfont->ftface;
    FT_Error fterror;
    FT_Fixed advance;

    DbgPrint("FtfdQueryAdvanceWidths\n");

    // FIXME: layout horizontal/vertical
    fl = (iMode == QAW_GETEASYWIDTHS) ? FT_ADVANCE_FLAG_FAST_ONLY : 0;
//      | (ftface->face_flags & FT_FACE_FLAG_VERTICAL) ? FT_LOAD_VERTICAL_LAYOUT : 0;
fl = 0;

    /* Loop all requested glyphs */
    for (i = 0; i < cGlyphs; i++)
    {
        /* Query advance width */
        fterror = FT_Get_Advance(ftface, (FT_UInt)phg[i], fl, &advance);
        if (fterror || advance > 0x0FFFF000)
        {
            DbgPrint("ERROR: failed to query advance width hg=%lx, fl=0x%lx\n",
                     phg[i], fl);
            pusWidths[i] = 0xffff;
            bResult = FALSE;
        }
        else
        {
            DbgPrint("Got advance width: hg=%lx, adv=%d\n", phg[i], advance >> 12);
            pusWidths[i] = (USHORT)advance >> 12;
        }
    }

    //__debugbreak();
    return bResult;
}

LONG
APIENTRY
FtfdQueryTrueTypeOutline(
    DHPDEV dhpdev,
    FONTOBJ *pfo,
    HGLYPH hg,
    BOOL bMetricsOnly,
    GLYPHDATA *pgldt,
    ULONG cjBuf,
    TTPOLYGONHEADER *ppoly)
{
    DbgPrint("FtfdQueryTrueTypeOutline\n");
    __debugbreak();
    return 0;
}

ULONG
APIENTRY
FtfdFontManagement(
    SURFOBJ *pso,
    FONTOBJ *pfo,
    ULONG iMode,
    ULONG cjIn,
    PVOID pvIn,
    ULONG cjOut,
    PVOID pvOut)
{
    DbgPrint("FtfdFontManagement\n");
    __debugbreak();
    return 0;
}

VOID
APIENTRY
FtfdDestroyFont(
    FONTOBJ *pfo)
{
    DbgPrint("FtfdDestroyFont()\n");
    __debugbreak();
}

