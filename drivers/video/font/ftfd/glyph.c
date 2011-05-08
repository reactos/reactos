/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver based on freetype
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "ftfd.h"

/** Private Interface *********************************************************/

#define FLOATL_1 0x3f800000

#define BITMAP_SIZE(cx, cy, bpp) \
    ((((((cx * bpp) + 7) >> 3) * cy) + 3) & ~3)

#define GLYPHBITS_SIZE(cx, cy, bpp) \
    (FIELD_OFFSET(GLYPHBITS, aj) + BITMAP_SIZE(cx, cy, bpp))

static
FLOATOBJ
FtfdNormalizeBaseVector(
    POINTEF *pptef)
{
    FLOATOBJ efTmp, efLength;
    FT_Vector ftvector;
    LONG lLength;

//__debugbreak();
#if 0
    /* Optimization for scaling transformations */
    if (pptef->y.ul1 == 0 && ppfted->y.ul2 == 0)
    {
        efLength = pptef->x;
        FLOATOBJ_SetLong(&pptef->x, 1);
        return efLength;
    }

    if (pptef->x.ul1 == 0 && ppfted->x.ul2 == 0)
    {
        efLength = pptef->y;
        FLOATOBJ_SetLong(&pptef->y, 1);
        return efLength;
    }
#endif
    /* Convert the point into 8.24 fixpoint format */
    efTmp = pptef->x;
    FLOATOBJ_MulLong(&efTmp, 0x01000000);
    ftvector.x = FLOATOBJ_GetLong(&efTmp);
    efTmp = pptef->y;
    FLOATOBJ_MulLong(&efTmp, 0x01000000);
    ftvector.y = FLOATOBJ_GetLong(&efTmp);

    /* Get the length of the fixpoint vector */
    lLength = FT_Vector_Length(&ftvector);

    /* Convert the fixpoint back into a FLOATOBJ */
    FLOATOBJ_SetLong(&efLength, lLength);
    FLOATOBJ_DivLong(&efLength, 0x01000000);

    /* Now divide the vector by the length */
    FLOATOBJ_Div(&pptef->x, &efLength);
    FLOATOBJ_Div(&pptef->y, &efLength);

    return efLength;
}

PFTFD_FONT
NTAPI
FtfdCreateFontInstance(
    FONTOBJ *pfo)
{
    PFTFD_FILE pfile = (PFTFD_FILE)pfo->iFile;
    PFTFD_FACE pface = pfile->apface[pfo->iFace - 1];
    PFTFD_FONT pfont;
    XFORMOBJ* pxo;
    FLOATOBJ_XFORM fxform;
    FT_Error fterror;
    FT_Face ftface;
    FT_Matrix ftmatrix;
    ULONG iComplexity;
    FTFD_DEVICEMETRICS *pmetrics;
    FLOATOBJ efScaleX, efScaleY;

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
    pfont->pface = pface;
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
        WARN("Error creating face\n");
        EngFreeMem(pfont);
        return NULL;
    }

    pfont->ftface = ftface;

    /* Get the XFORMOBJ from the font */
    pxo = FONTOBJ_pxoGetXform(pfo);
    if (!pxo)
    {
        WARN("Error there is no XFORMOBJ!\n");
        EngFreeMem(pfont);
        return NULL;
    }

    /* Get a FLOATOBJ_XFORM matrix */
    iComplexity = XFORMOBJ_iGetFloatObjXform(pxo, &fxform);
    ASSERT(iComplexity != DDI_ERROR);

    /* Get the base vectors (unnormalized) */
    pfont->ptefBase.x = fxform.eM11;
    pfont->ptefBase.y = fxform.eM21;
    pfont->ptefSide.x = fxform.eM12;
    pfont->ptefSide.y = fxform.eM22;

    /* Normalize the base vectors and get their length */
    efScaleX = FtfdNormalizeBaseVector(&pfont->ptefBase);
    efScaleY = FtfdNormalizeBaseVector(&pfont->ptefSide);

    // FIXME: quantize to 16.16 fixpoint
    pfont->fdxQuantized.eXX = FLOATOBJ_GetFloat(&fxform.eM11);
    pfont->fdxQuantized.eXY = FLOATOBJ_GetFloat(&fxform.eM12);
    pfont->fdxQuantized.eYX = FLOATOBJ_GetFloat(&fxform.eM21);
    pfont->fdxQuantized.eYY = FLOATOBJ_GetFloat(&fxform.eM22);

    /* The coordinate transformation given by Windows transforms from font
     * space to device space. Since we use FT_Set_Char_Size, which allows
     * higher precision than FT_Set_Pixel_Sizes, we need to convert into
     * points. So we multiply our scaling coefficients with 72 divided by
     * the device resolution. We also need a 26.4 fixpoint value, so we
     * multiply with 64. */
    FLOATOBJ_MulLong(&efScaleX, 64 * pface->ifiex.ifi.fwdUnitsPerEm * 72);
    FLOATOBJ_DivLong(&efScaleX, pfo->sizLogResPpi.cx);
    FLOATOBJ_MulLong(&efScaleY, 64 * pface->ifiex.ifi.fwdUnitsPerEm * 72);
    FLOATOBJ_DivLong(&efScaleY, pfo->sizLogResPpi.cy);

    /* Set the x and y character size for the font */
    fterror = FT_Set_Char_Size(ftface,
                               FLOATOBJ_GetLong(&efScaleX),
                               FLOATOBJ_GetLong(&efScaleY),
                               pfo->sizLogResPpi.cx,
                               pfo->sizLogResPpi.cy);
    if (fterror)
    {
        /* Failure! */
        WARN("Error setting face size\n");
        EngFreeMem(pfont);
        return NULL;
    }

    /* Check if there is rotation / shearing (cannot use iComplexity!?) */
    if (fxform.eM12.ul1 != 0 || fxform.eM12.ul2 != 0 ||
        fxform.eM21.ul1 != 0 || fxform.eM21.ul2 != 0)
    {
        __debugbreak();
        // FIXME: need to calculate scaling

        /* Initialize a matrix */
        ftmatrix.xx = 0x10000 * pface->ifiex.ifi.fwdUnitsPerEm * 72 /
                          pfo->sizLogResPpi.cx;
        ftmatrix.xy = 0x00000000;
        ftmatrix.yx = 0x00000000;
        ftmatrix.yy = 0x10000 * pface->ifiex.ifi.fwdUnitsPerEm * 72 /
                          pfo->sizLogResPpi.cy;

        /* Apply the XFORMOBJ to the matrix */
        XFORMOBJ_bApplyXform(pxo, XF_LTOL, 2, &ftmatrix, &ftmatrix);

        /* Set non-orthogonal transformation */
        // FT_Set_Transform

    }

    /* Check if there is a design vector */
    if (pfile->dv.dvReserved == STAMP_DESIGNVECTOR)
    {
        /* Set the coordinates */
        fterror = FT_Set_MM_Design_Coordinates(ftface,
                                               pfile->dv.dvNumAxes,
                                               pfile->dv.dvValues);
        if (fterror)
        {
            /* Failure! */
            WARN("Failed to set design vector\n");
            EngFreeMem(pfont);
            return NULL;
        }
    }

    /* Prepare required coordinates in font space */
    pmetrics = &pfont->metrics;
    pmetrics->ptfxMaxAscender.x = 0;
    pmetrics->ptfxMaxAscender.y = ftface->bbox.yMax << 4; // FIXME: not exact
    pmetrics->ptfxMaxDescender.x = 0;
    pmetrics->ptfxMaxDescender.y = -ftface->bbox.yMin << 4; // FIXME: not exact
    pmetrics->ptlUnderline1.x = 0;
    pmetrics->ptlUnderline1.y = -pface->ifiex.ifi.fwdUnderscorePosition;
    pmetrics->ptlStrikeout.x = 0;
    pmetrics->ptlStrikeout.y = -pface->ifiex.ifi.fwdStrikeoutPosition;;
    pmetrics->ptlULThickness.x = 0;
    pmetrics->ptlULThickness.y = pface->ifiex.ifi.fwdUnderscoreSize;
    pmetrics->ptlSOThickness.x = 0;
    pmetrics->ptlSOThickness.y = pface->ifiex.ifi.fwdStrikeoutSize;
    pmetrics->sizlMax.cx = ftface->bbox.xMax - ftface->bbox.xMin;
    pmetrics->sizlMax.cy = ftface->bbox.yMax - ftface->bbox.yMin;

    /* Transform all coordinates into device space */
    if (!XFORMOBJ_bApplyXform(pxo, XF_LTOL, 7, pmetrics->aptl, pmetrics->aptl))
    {
        WARN("Failed apply coordinate transformation.\n");
        EngFreeMem(pfont);
        return NULL;
    }

    /* Fixup some minimum values */
    if (pmetrics->ptlULThickness.y <= 0) pmetrics->ptlULThickness.y = 1;
    if (pmetrics->ptlSOThickness.y <= 0) pmetrics->ptlSOThickness.y = 1;

    //TRACE("Created font of size %ld (%ld)\n", yScale, (yScale+32)/64);
    //__debugbreak();

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

ULONG
NTAPI
FtfdQueryMaxExtents(
    FONTOBJ *pfo,
    PFD_DEVICEMETRICS pfddm,
    ULONG cjSize)
{
    PFTFD_FONT pfont = FtfdGetFontInstance(pfo);
    PFTFD_FACE pface = pfont->pface;
    FT_Face ftface = pfont->ftface;
    XFORMOBJ *pxo;

    TRACE("FtfdQueryMaxExtents\n");

    if (pfddm)
    {
        /* Verify parameter */
        if (cjSize < sizeof(FD_DEVICEMETRICS))
        {
            /* Not enough space, fail */
            WARN("cjSize = %ld\n", cjSize);
            return FD_ERROR;
        }

        /* Get the XFORMOBJ */
        pxo = FONTOBJ_pxoGetXform(pfo);

        /* Accelerator flags (ignored atm) */
        pfddm->flRealizedType = 0;

        /* Fixed width advance */
        if (FT_IS_FIXED_WIDTH(ftface))
            pfddm->lD = ftface->max_advance_width;
        else
            pfddm->lD = 0;

        /* Copy some values from the font structure */
        pfddm->fxMaxAscender = pfont->metrics.ptfxMaxAscender.y;
        pfddm->fxMaxDescender = pfont->metrics.ptfxMaxDescender.y;
        pfddm->ptlUnderline1 = pfont->metrics.ptlUnderline1;
        pfddm->ptlStrikeout = pfont->metrics.ptlStrikeout;
        pfddm->ptlULThickness = pfont->metrics.ptlULThickness;
        pfddm->ptlSOThickness = pfont->metrics.ptlSOThickness;
        pfddm->cxMax = pfont->metrics.sizlMax.cx;
        pfddm->cyMax = pfont->metrics.sizlMax.cy;

        /* Convert the base vectors from FLOATOBJ to FLOATL */
        pfddm->pteBase.x = FLOATOBJ_GetFloat(&pfont->ptefBase.x);
        pfddm->pteBase.y = FLOATOBJ_GetFloat(&pfont->ptefBase.y);
        pfddm->pteSide.x = FLOATOBJ_GetFloat(&pfont->ptefSide.x);
        pfddm->pteSide.y = FLOATOBJ_GetFloat(&pfont->ptefSide.y);

        /* cjGlyphMax is the full size of the GLYPHBITS structure */
        pfddm->cjGlyphMax = GLYPHBITS_SIZE(pfddm->cxMax, pfddm->cyMax, 4);

        /* Copy the quantized matrix from the font structure */
        pfddm->fdxQuantized = pfont->fdxQuantized;

        pfddm->lNonLinearExtLeading =   0x00000000;
        pfddm->lNonLinearIntLeading =   0x00000010;
        pfddm->lNonLinearMaxCharWidth = 0x80000000;
        pfddm->lNonLinearAvgCharWidth = 0x80000000;

        pfddm->lMinA = 0;
        pfddm->lMinC = 0;
        pfddm->lMinD = 0;
    }

//__debugbreak();

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
            WARN("Couldn't load glyph 0x%lx\n", hg);
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

    pgd->gdf.pgb = pvGlyphData;
    pgd->hg = hg;

    if (1 /* layout horizontal */)
    {
        pgd->fxA = ftglyph->metrics.horiBearingX;
        pgd->fxAB = pgd->fxA + ftglyph->metrics.width / 4;
        pgd->fxD = ftglyph->metrics.horiAdvance / 4;
    }
    else
    {
        pgd->fxA = ftglyph->metrics.vertBearingX;
        pgd->fxAB = pgd->fxA + ftglyph->metrics.height;
        pgd->fxD = ftglyph->metrics.vertAdvance;
    }

    pgd->fxInkBottom = 0;
    pgd->fxInkTop = pgd->fxInkBottom + (ftglyph->bitmap.rows << 4);
    pgd->rclInk.left = ftglyph->bitmap_left;
    pgd->rclInk.top = -ftglyph->bitmap_top;
    pgd->rclInk.right = pgd->rclInk.left + ftglyph->bitmap.width;
    pgd->rclInk.bottom = pgd->rclInk.top + ftglyph->bitmap.rows;

    /* Make the bitmap at least 1x1 pixel */
    if (ftglyph->bitmap.width == 0) pgd->rclInk.right++;
    if (ftglyph->bitmap.rows == 0) pgd->rclInk.bottom++;

    pgd->ptqD.x.LowPart = 0x000000fd;
    pgd->ptqD.x.HighPart = 0;
    pgd->ptqD.y.LowPart = 0x000000a0; // 0x000000a0
    pgd->ptqD.y.HighPart = 0;
    //pgd->ptqD.x.QuadPart = 0;
    //pgd->ptqD.y.QuadPart = 0;
//__debugbreak();
}

VOID
FtfdCopyBitmap(
    BYTE *pjDest,
    FT_Bitmap *ftbitmap)
{
    ULONG ulRows, ulDstDelta, ulSrcDelta;
    PBYTE pjDstLine, pjSrcLine;


    pjDstLine = pjDest;
    ulDstDelta = (ftbitmap->width*4 + 7) / 8;

    pjSrcLine = ftbitmap->buffer;
    ulSrcDelta = abs(ftbitmap->pitch);

    ulRows = ftbitmap->rows;
    while (ulRows--)
    {
        ULONG ulWidth = ulDstDelta;
        BYTE j, *pjSrc;

        pjSrc = pjSrcLine;
        while (ulWidth--)
        {
            /* Get the 1st pixel */
            j = (*pjSrc++) & 0xf0;

            /* Get the 2nd pixel */
            if (ulWidth > 0 || !(ftbitmap->width & 1))
                j |= (*pjSrc++) >> 4;
            *pjDstLine++ = j;
        }

        /* Go to the next line */
        //pjDstLine += ulDstDelta;
        pjSrcLine += ulSrcDelta;
    }
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
    ULONG cjBitmapSize;

    pgb->ptlOrigin.x =   ftglyph->bitmap_left;
    pgb->ptlOrigin.y = - ftglyph->bitmap_top;
    pgb->sizlBitmap.cx = ftglyph->bitmap.width;
    pgb->sizlBitmap.cy = ftglyph->bitmap.rows;

    cjBitmapSize = BITMAP_SIZE(pgb->sizlBitmap.cx, pgb->sizlBitmap.cy, 4);
    if (cjBitmapSize + FIELD_OFFSET(GLYPHBITS, aj) > cjSize)
    {
        WARN("Buffer too small, got %ld, need %ld\n",
                 cjSize, cjBitmapSize + FIELD_OFFSET(GLYPHBITS, aj));
        __debugbreak();
        return;
    }

    /* Copy the bitmap */
    FtfdCopyBitmap(pgb->aj, &ftglyph->bitmap);

    //RtlCopyMemory(pgb->aj, ftglyph->bitmap.buffer, cjBitmapSize);

    TRACE("QueryGlyphBits hg=%lx, (%ld,%ld) cjSize=%ld, need %ld\n",
          hg, pgb->sizlBitmap.cx, pgb->sizlBitmap.cy, cjSize,
          GLYPHBITS_SIZE(pgb->sizlBitmap.cx, pgb->sizlBitmap.cy, 4));

}

VOID
FtfdQueryGlyphOutline(
    FONTOBJ *pfo,
    HGLYPH hg,
    PATHOBJ *ppo,
    ULONG cjSize)
{

}

BOOL
FtRenderGlyphBitmap(
    PFTFD_FONT pfont)
{
    FT_Error fterror;

    fterror = FT_Render_Glyph(pfont->ftface->glyph, FT_RENDER_MODE_NORMAL);
    if (fterror)
    {
        WARN("Cound't render glyph\n");
        return FALSE;
    }

    return TRUE;
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

    TRACE("FtfdQueryFontData, iMode=%ld, hg=%lx, pgd=%p, pv=%p, cjSize=%ld\n",
          iMode, hg, pgd, pv, cjSize);

    switch (iMode)
    {
        case QFD_GLYPHANDBITMAP:
            /* Load the requested glyph */
            if (!FtfdLoadGlyph(pfont, hg, 0)) return FD_ERROR;

            /* Render the glyph bitmap */
            if (!FtRenderGlyphBitmap(pfont)) return FD_ERROR;

            if (pgd) FtfdQueryGlyphData(pfo, hg, pgd, pv);


            if (pv)
            {
                FtfdQueryGlyphBits(pfo, hg, pv, cjSize);
            }

            /* Return the size for a 1bpp bitmap */
            return GLYPHBITS_SIZE(pfont->ftface->glyph->bitmap.width,
                                  pfont->ftface->glyph->bitmap.rows,
                                  4);

        case QFD_GLYPHANDOUTLINE:
            TRACE("QFD_GLYPHANDOUTLINE\n");

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
            TRACE("QFD_TT_GRAY1_BITMAP\n");
            break;
        case QFD_TT_GRAY2_BITMAP:
            TRACE("QFD_TT_GRAY2_BITMAP\n");
            break;
        case QFD_TT_GRAY4_BITMAP:
            TRACE("QFD_TT_GRAY4_BITMAP\n");
            break;
        case QFD_TT_GRAY8_BITMAP:
            TRACE("QFD_TT_GRAY8_BITMAP\n");
            break;
        default:
            WARN("Invalid iMode value: %lx\n", iMode);
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
    TRACE("FtfdQueryGlyphAttrs\n");

    /* Verify parameters */
    if (!pfo || iMode != FO_ATTR_MODE_ROTATE)
    {
        WARN("Invalid parameters: %p, %ld\n", pfo, iMode);
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

    TRACE("FtfdQueryAdvanceWidths\n");

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
            WARN("Failed to query advance width hg=%lx, fl=0x%lx\n",
                     phg[i], fl);
            pusWidths[i] = 0xffff;
            bResult = FALSE;
        }
        else
        {
            /* Transform from 16.16 points to 28.4 pixels */
            pusWidths[i] = (USHORT)((advance * 72 / pfo->sizLogResPpi.cx) >> 12);
            //TRACE("Got advance width: hg=%lx, adv=%lx->%ld\n", phg[i], advance, pt.x);
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
    TRACE("FtfdQueryTrueTypeOutline\n");
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
    TRACE("FtfdFontManagement\n");
    __debugbreak();
    return 0;
}

VOID
APIENTRY
FtfdDestroyFont(
    FONTOBJ *pfo)
{
    PFTFD_FONT pfont = pfo->pvProducer;

    TRACE("FtfdDestroyFont()\n");

    /* Nothing to do? */
    if (!pfont) return;

    /* We don't need this anymore */
    pfo->pvProducer = NULL;

    /* Cleanup the freetype face for this font */
    FT_Done_Face(pfont->ftface);

    /* Free the font structure */
    EngFreeMem(pfont);
}

