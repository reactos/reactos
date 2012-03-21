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
    (((((((cx) * (bpp)) + 7) >> 3) * (cy)) + 3) & ~3)

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

    /* Optimization for scaling transformations */
    if (FLOATOBJ_bIsNull(&pptef->y))
    {
        efLength = pptef->x;
        FLOATOBJ_SetLong(&pptef->x, 1);
        return efLength;
    }

    if (FLOATOBJ_bIsNull(&pptef->x))
    {
        efLength = pptef->y;
        FLOATOBJ_SetLong(&pptef->y, -1);
        return efLength;
    }

    /* Convert the point into a 8.24 fixpoint vector */
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

    /* y axis is inverted! */
    FLOATOBJ_Neg(&pptef->y);

    /* Return the former length of the vector */
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
    FLOATOBJ efTemp, efScaleX, efScaleY;

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

    /* Set requested number of bits per pixel of the target */
    if (pfo->flFontType & FO_CLEARTYPE_X) pfont->jBpp = 8;
    else if (pfo->flFontType & FO_CLEARTYPE_Y) pfont->jBpp = 8;
    else if (pfo->flFontType & FO_GRAY16) pfont->jBpp = 4;
    else pfont->jBpp = 1;

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

    // FIXME: quantize to 16.16 fixpoint
    pfont->fdxQuantized.eXX = FLOATOBJ_GetFloat(&fxform.eM11);
    pfont->fdxQuantized.eXY = FLOATOBJ_GetFloat(&fxform.eM12);
    pfont->fdxQuantized.eYX = FLOATOBJ_GetFloat(&fxform.eM21);
    pfont->fdxQuantized.eYY = FLOATOBJ_GetFloat(&fxform.eM22);

    /* Get the base vectors (unnormalized) */
    pfont->ptefBase.x = fxform.eM11;
    pfont->ptefBase.y = fxform.eM21;
    pfont->ptefSide.x = fxform.eM12;
    pfont->ptefSide.y = fxform.eM22;

    /* Normalize the base vectors and get their length */
    efScaleX = FtfdNormalizeBaseVector(&pfont->ptefBase);
    efScaleY = FtfdNormalizeBaseVector(&pfont->ptefSide);

    /* Calculate maximum ascender and descender */
    efTemp = efScaleY;
    FLOATOBJ_MulLong(&efTemp, ftface->bbox.yMax << 4);
    pfont->metrics.fxMaxAscender = FLOATOBJ_GetLong(&efTemp);
    efTemp = efScaleY;
    FLOATOBJ_MulLong(&efTemp, (-ftface->bbox.yMin) << 4);
    pfont->metrics.fxMaxDescender = FLOATOBJ_GetLong(&efTemp);

    /* The coordinate transformation given by Windows transforms from font
     * space to device space. Since we use FT_Set_Char_Size, which allows
     * higher precision than FT_Set_Pixel_Sizes, we need to convert into
     * points. So we multiply our scaling coefficients with 72 and divide
     * by the device resolution. We also need a 26.6 fixpoint value, so we
     * multiply with 64. */
    FLOATOBJ_MulLong(&efScaleX, 64 * pface->ifiex.ifi.fwdUnitsPerEm * 72);
    FLOATOBJ_DivLong(&efScaleX, pfo->sizLogResPpi.cx);
    pfont->sizlScale.cx = FLOATOBJ_GetLong(&efScaleX);
    FLOATOBJ_MulLong(&efScaleY, 64 * pface->ifiex.ifi.fwdUnitsPerEm * 72);
    FLOATOBJ_DivLong(&efScaleY, pfo->sizLogResPpi.cy);
    pfont->sizlScale.cy = FLOATOBJ_GetLong(&efScaleY);

    /* Set the x and y character size for the font */
    fterror = FT_Set_Char_Size(ftface,
                               pfont->sizlScale.cx,
                               pfont->sizlScale.cy,
                               pfo->sizLogResPpi.cx,
                               pfo->sizLogResPpi.cy);
    if (fterror)
    {
        /* Failure! */
        WARN("Error setting face size\n");
        EngFreeMem(pfont);
        return NULL;
    }

    /* Check if there is rotation / skewing (cannot use iComplexity!?) */
    if (!FLOATOBJ_bIsNull(&fxform.eM12) || !FLOATOBJ_bIsNull(&fxform.eM21))
    {
        TRACE("Setting extended xform\n");
        //__debugbreak();

        /* Create a transformation matrix that is applied after the character
         * scaling. We simply use the normalized base vectors and convert them
         * to 16.16 fixpoint format */

        efTemp = pfont->ptefBase.x;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.xx = FLOATOBJ_GetLong(&efTemp);

        efTemp = pfont->ptefSide.x;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.xy = FLOATOBJ_GetLong(&efTemp);

        efTemp = pfont->ptefBase.y;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.yx = FLOATOBJ_GetLong(&efTemp);

        efTemp = pfont->ptefSide.y;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.yy = FLOATOBJ_GetLong(&efTemp);

        /* Set the transformation matrix */
        FT_Set_Transform(ftface, &ftmatrix, 0);
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
    pmetrics->ptlUnderline1.x = 0;
    pmetrics->ptlUnderline1.y = -pface->ifiex.ifi.fwdUnderscorePosition;
    pmetrics->ptlStrikeout.x = 0;
    pmetrics->ptlStrikeout.y = -pface->ifiex.ifi.fwdStrikeoutPosition;;
    pmetrics->ptlULThickness.x = 0;
    pmetrics->ptlULThickness.y = pface->ifiex.ifi.fwdUnderscoreSize;
    pmetrics->ptlSOThickness.x = 0;
    pmetrics->ptlSOThickness.y = pface->ifiex.ifi.fwdStrikeoutSize;
    pmetrics->aptfxBBox[0].x = ftface->bbox.xMin << 4;
    pmetrics->aptfxBBox[0].y = ftface->bbox.yMin << 4;
    pmetrics->aptfxBBox[1].x = ftface->bbox.xMax << 4;
    pmetrics->aptfxBBox[1].y = ftface->bbox.yMin << 4;
    pmetrics->aptfxBBox[2].x = ftface->bbox.xMax << 4;
    pmetrics->aptfxBBox[2].y = ftface->bbox.yMax << 4;
    pmetrics->aptfxBBox[3].x = ftface->bbox.xMin << 4;
    pmetrics->aptfxBBox[3].y = ftface->bbox.yMax << 4;

    /* Transform all coordinates into device space */
    if (!XFORMOBJ_bApplyXform(pxo, XF_LTOL, 8, pmetrics->aptl, pmetrics->aptl))
    {
        WARN("Failed apply coordinate transformation.\n");
        EngFreeMem(pfont);
        return NULL;
    }

    /* Extract the bounding box in FIX device coordinates */
    pfont->rcfxBBox.left = min(pmetrics->aptfxBBox[0].x, pmetrics->aptfxBBox[1].x);
    pfont->rcfxBBox.left = min(pfont->rcfxBBox.left, pmetrics->aptfxBBox[2].x);
    pfont->rcfxBBox.left = min(pfont->rcfxBBox.left, pmetrics->aptfxBBox[3].x);
    pfont->rcfxBBox.right = max(pmetrics->aptfxBBox[0].x, pmetrics->aptfxBBox[1].x);
    pfont->rcfxBBox.right = max(pfont->rcfxBBox.right, pmetrics->aptfxBBox[2].x);
    pfont->rcfxBBox.right = max(pfont->rcfxBBox.right, pmetrics->aptfxBBox[3].x);
    pfont->rcfxBBox.top = min(pmetrics->aptfxBBox[0].y, pmetrics->aptfxBBox[1].y);
    pfont->rcfxBBox.top = min(pfont->rcfxBBox.top, pmetrics->aptfxBBox[2].y);
    pfont->rcfxBBox.top = min(pfont->rcfxBBox.top, pmetrics->aptfxBBox[3].y);
    pfont->rcfxBBox.bottom = max(pmetrics->aptfxBBox[0].y, pmetrics->aptfxBBox[1].y);
    pfont->rcfxBBox.bottom = max(pfont->rcfxBBox.bottom, pmetrics->aptfxBBox[2].y);
    pfont->rcfxBBox.bottom = max(pfont->rcfxBBox.bottom, pmetrics->aptfxBBox[3].y);

    /* Round the bounding box margings to pixels */
    pfont->rcfxBBox.left = pfont->rcfxBBox.left & ~0xf;
    pfont->rcfxBBox.top = pfont->rcfxBBox.top & ~0xf;
    pfont->rcfxBBox.right = (pfont->rcfxBBox.right + 0xf) & ~0xf;
    pfont->rcfxBBox.bottom = (pfont->rcfxBBox.bottom + 0xf) & ~0xf;

    /* Calculate maximum extents in pixels */
    pfont->sizlMax.cx = (pfont->rcfxBBox.right - pfont->rcfxBBox.left) >> 4;
    pfont->sizlMax.cy = (pfont->rcfxBBox.bottom - pfont->rcfxBBox.top) >> 4;

    /* Fixup some minimum values */
    if (pmetrics->ptlULThickness.y <= 0) pmetrics->ptlULThickness.y = 1;
    if (pmetrics->ptlSOThickness.y <= 0) pmetrics->ptlSOThickness.y = 1;

    TRACE("Created font of size %ld (%ld)\n",
          pfont->sizlScale.cy, (pfont->sizlScale.cy+32)/64);
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

    /* Create a font instance if neccessary */
    if (!pfont) pfont = FtfdCreateFontInstance(pfo);

    /* Return the font instance */
    return pfont;
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
        /* Load the glyph into the freetype face slot */
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

        /* Accelerator flags (ignored atm) */
        pfddm->flRealizedType = 0;

        /* Check for fixed width font */
        if (FT_IS_FIXED_WIDTH(ftface))
        {
            /* Make sure a glyph is loaded */
            if ((pfont->hgSelected != -1) ||
                FtfdLoadGlyph(pfont, 0, 0))
            {
                /* Convert advance from 26.6 fixpoint to pixel format */
                pfddm->lD = pface->ftface->glyph->advance.x >> 6;
            }
            else
            {
                /* Fall back to dynamic pitch */
                WARN("Couldn't load a glyph\n");
                pfddm->lD = 0;
            }
        }
        else
        {
            /* Variable pitch */
            pfddm->lD = 0;
        }

        /* Copy some values from the font structure */
        pfddm->ptlUnderline1 = pfont->metrics.ptlUnderline1;
        pfddm->ptlStrikeout = pfont->metrics.ptlStrikeout;
        pfddm->ptlULThickness = pfont->metrics.ptlULThickness;
        pfddm->ptlSOThickness = pfont->metrics.ptlSOThickness;
        pfddm->cxMax = pfont->sizlMax.cx;
        pfddm->cyMax = pfont->sizlMax.cy;

        /* These values are rounded to pixels to fix inconsistent height
           of marked text and the rest of the marked row in XP list boxes */
        pfddm->fxMaxAscender = (pfont->metrics.fxMaxAscender + 15) & ~0x0f;
        pfddm->fxMaxDescender = (pfont->metrics.fxMaxDescender + 15) & ~0x0f;

        /* Convert the base vectors from FLOATOBJ to FLOATL */
        pfddm->pteBase.x = FLOATOBJ_GetFloat(&pfont->ptefBase.x);
        pfddm->pteBase.y = FLOATOBJ_GetFloat(&pfont->ptefBase.y);
        pfddm->pteSide.x = FLOATOBJ_GetFloat(&pfont->ptefSide.x);
        pfddm->pteSide.y = FLOATOBJ_GetFloat(&pfont->ptefSide.y);

        /* cjGlyphMax is the full size of the GLYPHBITS structure */
        pfddm->cjGlyphMax = GLYPHBITS_SIZE(pfddm->cxMax,
                                           pfddm->cyMax,
                                           pfont->jBpp);

        /* Copy the quantized matrix from the font structure */
        pfddm->fdxQuantized = pfont->fdxQuantized;

        pfddm->lNonLinearExtLeading =   0x80000000;
        pfddm->lNonLinearIntLeading =   0x80000000; // FIXME
        pfddm->lNonLinearMaxCharWidth = 0x80000000;
        pfddm->lNonLinearAvgCharWidth = 0x80000000;

        pfddm->lMinA = 0;
        pfddm->lMinC = 0;
        pfddm->lMinD = 0;
    }

    TRACE("pfddm->fxMaxAscender=%ld, yScale=%ld, height=%ld\n",
          pfddm->fxMaxAscender, pfont->sizlScale.cy,
          (pfont->sizlScale.cy+32)/64);
//__debugbreak();

    /* Return the size of the structure */
    return sizeof(FD_DEVICEMETRICS);
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
    SIZEL sizlBitmap;

    pgd->gdf.pgb = pvGlyphData;
    pgd->hg = hg;

    if (1 /* layout horizontal */)
    {
        // FIXME: ftglyph->metrics doesn't handle non-orthogonal transformations
        // http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_Set_Transform
        pgd->fxA = ftglyph->metrics.horiBearingX;
        pgd->fxAB = pgd->fxA + ftglyph->metrics.width / 4;
    }
    else
    {
        pgd->fxA = ftglyph->metrics.vertBearingX;
        pgd->fxAB = pgd->fxA + ftglyph->metrics.height;
    }

    /* D is the glyph advance width. Convert it from 26.6 to 28.4 fixpoint format */
    pgd->fxD = ftglyph->advance.x >> 2; // FIXME: should be projected on the x-axis

    /* Get the bitmap size */
    sizlBitmap.cx = ftglyph->bitmap.width;
    sizlBitmap.cy = ftglyph->bitmap.rows;

    /* Make the bitmap at least 1x1 pixel large */
    if (sizlBitmap.cx == 0) sizlBitmap.cx++;
    if (sizlBitmap.cy == 0) sizlBitmap.cy++;

    /* Don't let the bitmap be larger than the maximum */
    sizlBitmap.cx = min(sizlBitmap.cx, pfont->sizlMax.cx);
    sizlBitmap.cy = min(sizlBitmap.cy, pfont->sizlMax.cy);

    /* This is the box in which the bitmap fits */
    pgd->rclInk.left = ftglyph->bitmap_left;
    pgd->rclInk.top = -ftglyph->bitmap_top;
    pgd->rclInk.right = pgd->rclInk.left + sizlBitmap.cx;
    pgd->rclInk.bottom = pgd->rclInk.top + sizlBitmap.cy;

    /* FIX representation of bitmap top and bottom */
    pgd->fxInkBottom = (-pgd->rclInk.bottom) << 4;
    pgd->fxInkTop = pgd->rclInk.top << 4;

    // FIXME:
    pgd->ptqD.x.LowPart = pgd->fxD;
    pgd->ptqD.x.HighPart = 0;
    pgd->ptqD.y.LowPart = 0;
    pgd->ptqD.y.HighPart = 0;

//__debugbreak();
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

    /* Make the bitmap at least 1x1 pixel large */
    if (pgb->sizlBitmap.cx == 0) pgb->sizlBitmap.cx++;
    if (pgb->sizlBitmap.cy == 0) pgb->sizlBitmap.cy++;

    /* Don't let the bitmap be larger than the maximum */
    pgb->sizlBitmap.cx = min(pgb->sizlBitmap.cx, pfont->sizlMax.cx);
    pgb->sizlBitmap.cy = min(pgb->sizlBitmap.cy, pfont->sizlMax.cy);

    cjBitmapSize = BITMAP_SIZE(pgb->sizlBitmap.cx,
                               pgb->sizlBitmap.cy,
                               pfont->jBpp);
    if (cjBitmapSize + FIELD_OFFSET(GLYPHBITS, aj) > cjSize)
    {
        WARN("Buffer too small, got %ld, need %ld\n",
                 cjSize, cjBitmapSize + FIELD_OFFSET(GLYPHBITS, aj));
        __debugbreak();
        return;
    }

    /* Copy the bitmap */
    FtfdCopyBits(pfont->jBpp, pgb, &ftglyph->bitmap);

    //TRACE("QueryGlyphBits hg=%lx, (%ld,%ld) cjSize=%ld, need %ld\n",
    //      hg, pgb->sizlBitmap.cx, pgb->sizlBitmap.cy, cjSize,
    //      GLYPHBITS_SIZE(pgb->sizlBitmap.cx, pgb->sizlBitmap.cy, pfont->jBpp));
}

VOID
FtfdQueryGlyphOutline(
    FONTOBJ *pfo,
    HGLYPH hg,
    PATHOBJ *ppo,
    ULONG cjSize)
{
    WARN("FtfdQueryGlyphOutline is unimplemented\n");
    __debugbreak();
}

BOOL
FtRenderGlyphBitmap(
    PFTFD_FONT pfont)
{
    FT_Error fterror;
    FT_Render_Mode mode;

    /* Determine the right render mode */
    if (pfont->jBpp == 1) mode = FT_RENDER_MODE_MONO;
    else if (pfont->pfo->flFontType & FO_CLEARTYPE_X) mode = FT_RENDER_MODE_LCD;
    else if (pfont->pfo->flFontType & FO_CLEARTYPE_Y) mode = FT_RENDER_MODE_LCD_V;
    else mode = FT_RENDER_MODE_NORMAL;

    /* Render the glyph */
    fterror = FT_Render_Glyph(pfont->ftface->glyph, mode);
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
    ULONG cx, cy;

    //TRACE("FtfdQueryFontData, iMode=%ld, hg=%lx, pgd=%p, pv=%p, cjSize=%ld\n",
    //      iMode, hg, pgd, pv, cjSize);

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

            /* Return the size for a bitmap at least 1x1 pixels */
            cx = max(1, pfont->ftface->glyph->bitmap.width);
            cy = max(1, pfont->ftface->glyph->bitmap.rows);
            return GLYPHBITS_SIZE(cx, cy, pfont->jBpp);

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

    //TRACE("FtfdQueryAdvanceWidths\n");

    /* The selected glyph will be changed */
    pfont->hgSelected = -1;

    /* Check if fast version is requested */
    if (0 && iMode == QAW_GETEASYWIDTHS)
    {
        fl = FT_ADVANCE_FLAG_FAST_ONLY;

        /* Check if the font layout is vertical */
        if (ftface->face_flags & FT_FACE_FLAG_VERTICAL)
        {
            fl |= FT_LOAD_VERTICAL_LAYOUT;
        }

        /* Loop all requested glyphs */
        for (i = 0; i < cGlyphs; i++)
        {
            /* Query advance width */
            fterror = FT_Get_Advances(ftface, (FT_UInt)phg[i], 1, fl, &advance);
            if (fterror || advance > 0xFFFF)
            {
                pusWidths[i] = 0xffff;
                bResult = FALSE;
            }
            else
            {
                pusWidths[i] = (USHORT)(advance >> 2);
                //TRACE("Got advance width: hg=%lx, adv=%lx->%ld\n", phg[i], advance, pt.x);
            }
        }
    }
    else
    {
        /* Loop all requested glyphs */
        for (i = 0; i < cGlyphs; i++)
        {
            /* Load the glyph */
            fterror = FT_Load_Glyph(ftface, (FT_UInt)phg[i], 0);
            if (fterror)
            {
                pusWidths[i] = 0xffff;
                bResult = FALSE; // FIXME: return FALSE or DDI_ERROR?
            }
            else
            {
                pusWidths[i] = (USHORT)ftface->glyph->advance.x >> 2;
            }
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

