
#include <win32k.h>
#include "../diblib/DibLib_interface.h"
DBG_DEFAULT_CHANNEL(GdiFont);

#define SURFOBJ_flags(pso) (CONTAINING_RECORD(pso, SURFACE, SurfObj)->flags)

#define XCLIPOBJ CLIPGDI

XCLIPOBJ gxcoTrivial;


static
void
CalculateCoordinates(
    PBLTDATA pbltdata,
    PRECTL prclClipped,
    PRECTL prclOrg,
    PPOINTL pptlSrc,
    PPOINTL pptlMask,
    PPOINTL pptlPat,
    PSIZEL psizlPat)
{
    ULONG cx, cy;

    /* Copy the target start point */
    pbltdata->siDst.ptOrig.x = prclClipped->left;
    pbltdata->siDst.ptOrig.y = prclClipped->top;

    /* Calculate width and height of this rect */
    pbltdata->ulWidth = prclClipped->right - prclClipped->left;
    pbltdata->ulHeight = prclClipped->bottom - prclClipped->top;

    /* Calculate start position for target */
    pbltdata->siDst.pjBase = pbltdata->siDst.pvScan0;
    pbltdata->siDst.pjBase += pbltdata->siDst.ptOrig.y * pbltdata->siDst.lDelta;
    pbltdata->siDst.pjBase += pbltdata->siDst.ptOrig.x * pbltdata->siDst.jBpp / 8;

    /* Calculate the offset from the original coordinates */
    cx = (prclClipped->left - prclOrg->left);
    cy = (prclClipped->top - prclOrg->top);

    if (pptlSrc)
    {
        /* Calculate current origin for the source */
        pbltdata->siSrc.ptOrig.x = pptlSrc->x + cx;
        pbltdata->siSrc.ptOrig.y = pptlSrc->y + cy;

        /* Calculate start position for source */
        pbltdata->siSrc.pjBase = pbltdata->siSrc.pvScan0;
        pbltdata->siSrc.pjBase += pbltdata->siSrc.ptOrig.y * pbltdata->siSrc.lDelta;
        pbltdata->siSrc.pjBase += pbltdata->siSrc.ptOrig.x * pbltdata->siSrc.jBpp / 8;
    }

    if (pptlMask)
    {
        pbltdata->siMsk.ptOrig.x = pptlMask->x + cx;
        pbltdata->siMsk.ptOrig.y = pptlMask->y + cy;

        /* Calculate start position for mask */
        pbltdata->siMsk.pjBase = pbltdata->siMsk.pvScan0;
        pbltdata->siMsk.pjBase += pbltdata->siMsk.ptOrig.y * pbltdata->siMsk.lDelta;
        pbltdata->siMsk.pjBase += pbltdata->siMsk.ptOrig.x * pbltdata->siMsk.jBpp / 8;
    }

    if (pptlPat)
    {
        pbltdata->siPat.ptOrig.x = (pptlPat->x + cx) % psizlPat->cx;
        pbltdata->siPat.ptOrig.y = (pptlPat->x + cy) % psizlPat->cy;

        /* Calculate start position for pattern */
        pbltdata->siPat.pjBase = pbltdata->siPat.pvScan0;
        pbltdata->siPat.pjBase += pbltdata->siPat.ptOrig.y * pbltdata->siPat.lDelta;
        pbltdata->siPat.pjBase += pbltdata->siPat.ptOrig.x * pbltdata->siPat.jBpp / 8;
    }
}

BOOL
APIENTRY
EngBitBlt(
    _Inout_ SURFOBJ *psoTrg,
    _In_opt_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclTrg,
    _When_(psoSrc, _In_) POINTL *pptlSrc,
    _When_(psoMask, _In_) POINTL *pptlMask,
    _In_opt_ BRUSHOBJ *pbo,
    _When_(pbo, _In_) POINTL *pptlBrush,
    _In_ ROP4 rop4)
{
    BLTDATA bltdata;
    ULONG i, iFunctionIndex, iDirection = CD_ANY;
    RECTL rcTrg;
    PFN_DIBFUNCTION pfnBitBlt;
    BOOL bEnumMore;
    RECT_ENUM rcenum;
    PSIZEL psizlPat;


    ASSERT(psoTrg);
    ASSERT(psoTrg->iBitmapFormat >= BMF_1BPP);
    ASSERT(psoTrg->iBitmapFormat <= BMF_32BPP);
    ASSERT(prclTrg);
    ASSERT(prclTrg->left >= 0);
    ASSERT(prclTrg->top >= 0);
    ASSERT(prclTrg->right <= psoTrg->sizlBitmap.cx);
    ASSERT(prclTrg->bottom <= psoTrg->sizlBitmap.cy);

    rcTrg = *prclTrg;

    bltdata.rop4 = rop4;
    if (!pxlo) pxlo = &gexloTrivial.xlo;
    bltdata.pxlo = pxlo;
    bltdata.pfnXlate = XLATEOBJ_pfnXlate(pxlo);

    bltdata.siDst.pvScan0 = psoTrg->pvScan0;
    bltdata.siDst.lDelta = psoTrg->lDelta;
    bltdata.siDst.jBpp = gajBitsPerFormat[psoTrg->iBitmapFormat];

    /* Check if the ROP uses a source */
    if (ROP4_USES_SOURCE(rop4))
    {
        /* Sanity checks */
        ASSERT(psoSrc);
        ASSERT(psoSrc->iBitmapFormat >= BMF_1BPP);
        ASSERT(psoSrc->iBitmapFormat <= BMF_32BPP);
        ASSERT(pptlSrc);
        ASSERT(pptlSrc->x >= 0);
        ASSERT(pptlSrc->y >= 0);
        ASSERT(pptlSrc->x <= psoSrc->sizlBitmap.cx);
        ASSERT(pptlSrc->y <= psoSrc->sizlBitmap.cy);

        bltdata.siSrc.pvScan0 = psoSrc->pvScan0;
        bltdata.siSrc.lDelta = psoSrc->lDelta;
        bltdata.siSrc.jBpp = gajBitsPerFormat[psoSrc->iBitmapFormat];

        /* Check if source and target are equal */
        if (psoSrc == psoTrg)
        {
            /* Analyze the copying direction */
            if (rcTrg.top < pptlSrc->y)
                iDirection = rcTrg.left < pptlSrc->x ? CD_RIGHTDOWN : CD_LEFTDOWN;
            else
                iDirection = rcTrg.left < pptlSrc->x ? CD_RIGHTUP : CD_LEFTUP;

            /* Check for special right to left case */
            if ((rcTrg.top == pptlSrc->y) && (rcTrg.left > pptlSrc->x))
            {
                /* Use 0 as target format to get special right to left versions */
                bltdata.siDst.iFormat = 0;
                bltdata.siSrc.iFormat = psoSrc->iBitmapFormat;
            }
            else
            {
                /* Use 0 as source format to get special equal surface versions */
                bltdata.siDst.iFormat = psoTrg->iBitmapFormat;
                bltdata.siSrc.iFormat = 0;
            }
        }
        else
        {
            bltdata.siDst.iFormat = psoTrg->iBitmapFormat;
            bltdata.siSrc.iFormat = psoSrc->iBitmapFormat;
        }
    }
    else
    {
        bltdata.siDst.iFormat = psoTrg->iBitmapFormat;
    }

    /* Check if the ROP uses a pattern / brush */
    if (ROP4_USES_PATTERN(rop4))
    {
        /* Must have a brush */
        ASSERT(pbo); // FIXME: test this!

        /* Copy the solid color */
        bltdata.ulSolidColor = pbo->iSolidColor;

        /* Check if this is a pattern brush */
        if (pbo->iSolidColor == 0xFFFFFFFF)
        {
            __debugbreak();

            // FIXME
            bltdata.siPat.iFormat = 0;//psoPat->iBitmapFormat;
            bltdata.siPat.pvScan0 = 0;//psoPat->pvScan0;
            bltdata.siPat.lDelta = 0;//psoPat->lDelta;

            bltdata.ulPatWidth = 0;
            bltdata.ulPatHeight = 0;

            psizlPat = 0;// fixme
        }
        else
        {
            pptlBrush = NULL;
            psizlPat = NULL;
        }
    }
    else
    {
        pptlBrush = NULL;
        psizlPat = NULL;
    }

    /* Check if the ROP uses a mask */
    if (ROP4_USES_MASK(rop4))
    {
        /* Must have a mask surface and point */
        ASSERT(psoMask);
        ASSERT(pptlMask);

        __debugbreak();

        bltdata.siMsk.iFormat = psoMask->iBitmapFormat;
        bltdata.siMsk.pvScan0 = psoMask->pvScan0;
        bltdata.siMsk.lDelta = psoMask->lDelta;

        bltdata.apfnDoRop[0] = gapfnRop[ROP4_BKGND(rop4)];
        bltdata.apfnDoRop[1] = gapfnRop[ROP4_FGND(rop4)];

        /* Calculate the masking function index */
        iFunctionIndex = ROP4_USES_PATTERN(rop4) ? 1 : 0;
        iFunctionIndex |= ROP4_USES_SOURCE(rop4) ? 2 : 0;
        iFunctionIndex |= ROP4_USES_DEST(rop4) ? 4 : 0;

        /* Get the masking function */
        pfnBitBlt = gapfnMaskFunction[iFunctionIndex];
    }
    else
    {
        /* Get the function index from the foreground ROP index*/
        iFunctionIndex = gajIndexPerRop[ROP4_FGND(rop4)];

        /* Get the dib function */
        pfnBitBlt = gapfnDibFunction[iFunctionIndex];
    }

    /* If no clip object is given, use trivial one */
    if (!pco) pco = (CLIPOBJ*)&gxcoTrivial;

    /* Check if we need to enumerate rects */
    if (pco->iDComplexity == DC_COMPLEX)
    {
        /* Start the enumeration of the clip object */
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, iDirection, 0);
        do
        {
            /* Enumerate a set of rectangles */
            bEnumMore = CLIPOBJ_bEnum(pco, sizeof(rcenum), (ULONG*)&rcenum);

            /* Loop all rectangles we got */
            for (i = 0; i < rcenum.c; i++)
            {
                /* Intersect this rect with the target rect */
                if (!RECTL_bIntersectRect(&rcenum.arcl[i], &rcenum.arcl[i], &rcTrg))
                {
                    /* This rect is outside the bounds, continue */
                    continue;
                }

                /* Calculate the coordinates */
                CalculateCoordinates(&bltdata,
                                     &rcenum.arcl[i],
                                     prclTrg,
                                     pptlSrc,
                                     pptlMask,
                                     pptlBrush,
                                     psizlPat);

                /* Call the dib function */
                pfnBitBlt(&bltdata);
            }
        }
        while (bEnumMore);
    }
    else
    {
        /* Check if there is something to clip */
        if (pco->iDComplexity == DC_RECT)
        {
            /* Clip the target rect to the bounds of the clipping region */
            RECTL_bIntersectRect(&rcTrg, &rcTrg, &pco->rclBounds);
        }

        /* Calculate the coordinates */
        CalculateCoordinates(&bltdata,
                             &rcTrg,
                             prclTrg,
                             pptlSrc,
                             pptlMask,
                             pptlBrush,
                             psizlPat);

        /* Call the dib function */
        pfnBitBlt(&bltdata);
    }

    return TRUE;
}


static
VOID
AdjustOffsetAndSize(
    _Out_ PPOINTL pptOffset,
    _Out_ PSIZEL psizTrg,
    _In_ PPOINTL pptlSrc,
    _In_ PSIZEL psizSrc)
{
    LONG x, y, cxMax, cyMax;

    x = pptlSrc->x + pptOffset->x;
    if (x < 0) pptOffset->x -= x, x = 0;

    cxMax = psizSrc->cx - x;
    if (psizTrg->cx > cxMax) psizTrg->cx= cxMax;

    y = pptlSrc->y + pptOffset->y;
    if (y < 0) pptOffset->y -= y, y = 0;

    cyMax = psizSrc->cy - y;
    if (psizTrg->cy > cyMax) psizTrg->cy = cyMax;
}

BOOL
APIENTRY
IntEngBitBlt(
    _Inout_ SURFOBJ *psoTrg,
    _In_opt_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMask,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclTrg,
    _When_(psoSrc, _In_) POINTL *pptlSrc,
    _When_(psoMask, _In_) POINTL *pptlMask,
    _In_opt_ BRUSHOBJ *pbo,
    _When_(pbo, _In_) POINTL *pptlBrush,
    _In_ ROP4 rop4)
{
    BOOL bResult;
    RECTL rcClipped;
    POINTL ptOffset, ptSrc, ptMask;
    SIZEL sizTrg;
    PFN_DrvBitBlt pfnBitBlt;

//__debugbreak();

    /* Sanity checks */
    ASSERT(IS_VALID_ROP4(rop4));
    ASSERT(psoTrg);
    ASSERT(psoTrg->iBitmapFormat >= BMF_1BPP);
    ASSERT(psoTrg->iBitmapFormat <= BMF_32BPP);
    ASSERT(prclTrg);
    ASSERT(RECTL_bIsWellOrdered(prclTrg));

    /* Clip the target rect to the extents of the target surface */
    rcClipped.left = max(prclTrg->left, 0);
    rcClipped.top = max(prclTrg->top, 0);
    rcClipped.right = min(prclTrg->right, psoTrg->sizlBitmap.cx);
    rcClipped.bottom = min(prclTrg->bottom, psoTrg->sizlBitmap.cy);

    /* If no clip object is given, use trivial one */
    if (!pco) pco = (CLIPOBJ*)&gxcoTrivial;

    /* Check if there is something to clip */
    if (pco->iDComplexity != DC_TRIVIAL)
    {
        /* Clip the target rect to the bounds of the clipping region */
        RECTL_bIntersectRect(&rcClipped, &rcClipped, &pco->rclBounds);
    }

    /* Don't pass a clip object with a single rectangle */
//    if (pco->iDComplexity == DC_RECT) pco = (CLIPOBJ*)&gxcoTrivial;

    /* Calculate initial offset and size */
    ptOffset.x = rcClipped.left - prclTrg->left;
    ptOffset.y = rcClipped.top - prclTrg->top;
    sizTrg.cx = rcClipped.right - rcClipped.left;
    sizTrg.cy = rcClipped.bottom - rcClipped.top;

    /* Check if the ROP uses a source */
    if (ROP4_USES_SOURCE(rop4))
    {
        /* Must have a source surface and point */
        ASSERT(psoSrc);
        ASSERT(pptlSrc);

        /* Get the source point */
        ptSrc = *pptlSrc;

        /* Clip against the extents of the source surface */
        AdjustOffsetAndSize(&ptOffset, &sizTrg, &ptSrc, &psoSrc->sizlBitmap);
    }
    else
    {
        psoSrc = NULL;
        ptSrc.x = 0;
        ptSrc.y = 0;
    }

    /* Check if the ROP uses a mask */
    if (ROP4_USES_MASK(rop4))
    {
        /* Must have a mask surface and point */
        ASSERT(psoMask);
        ASSERT(pptlMask);

        /* Get the mask point */
        ptMask = *pptlMask;

        /* Clip against the extents of the mask surface */
        AdjustOffsetAndSize(&ptOffset, &sizTrg, &ptMask, &psoMask->sizlBitmap);
    }
    else
    {
        psoMask = NULL;
        ptMask.x = 0;
        ptMask.y = 0;
    }

    /* Adjust the points */
    ptSrc.x += ptOffset.x;
    ptSrc.y += ptOffset.y;
    ptMask.x += ptOffset.x;
    ptMask.y += ptOffset.y;

    /* Recalculate the target rect */
    rcClipped.left = prclTrg->left + ptOffset.x;
    rcClipped.top = prclTrg->top + ptOffset.y;
    rcClipped.right = rcClipped.left + sizTrg.cx;
    rcClipped.bottom = rcClipped.top + sizTrg.cy;

    /* Is the target surface device managed? */
    if (SURFOBJ_flags(psoTrg) & HOOK_BITBLT)
    {
        /* Is the source a different device managed surface? */
        if (psoSrc && (psoSrc->hdev != psoTrg->hdev) &&
            (SURFOBJ_flags(psoSrc) & HOOK_BITBLT))
        {
            ERR("Need to copy to standard bitmap format!\n");
            ASSERT(FALSE);
        }

        pfnBitBlt = GDIDEVFUNCS(psoTrg).BitBlt;
    }
    /* Otherwise is the source surface device managed? */
    else if (psoSrc && (SURFOBJ_flags(psoSrc) & HOOK_BITBLT))
    {
        pfnBitBlt = GDIDEVFUNCS(psoSrc).BitBlt;
    }
    else
    {
        pfnBitBlt = EngBitBlt;
    }

    bResult = pfnBitBlt(psoTrg,
                        psoSrc,
                        psoMask,
                        pco,
                        pxlo,
                        &rcClipped,
                        psoSrc ? &ptSrc : NULL,
                        psoMask ? &ptMask : NULL,
                        pbo,
                        pptlBrush,
                        rop4);

    // FIXME: cleanup temp surface!

    return bResult;
}

BOOL
APIENTRY
NtGdiEngBitBlt(
    IN SURFOBJ *psoTrgUMPD,
    IN SURFOBJ *psoSrcUMPD,
    IN SURFOBJ *psoMaskUMPD,
    IN CLIPOBJ *pcoUMPD,
    IN XLATEOBJ *pxloUMPD,
    IN RECTL *prclTrg,
    IN POINTL *pptlSrc,
    IN POINTL *pptlMask,
    IN BRUSHOBJ *pboUMPD,
    IN POINTL *pptlBrush,
    IN ROP4 rop4)
{
    RECTL  rclTrg;
    POINTL ptlSrc, ptlMask, ptlBrush;
    HSURF hsurfTrg, hsurfSrc = NULL, hsurfMask = NULL;
    HANDLE hBrushObj; // HUMPDOBJ
    SURFOBJ *psoTrg, *psoSrc, *psoMask;
    CLIPOBJ *pco;
    XLATEOBJ *pxlo;
    BRUSHOBJ *pbo;
    BOOL bResult;

    _SEH2_TRY
    {
        ProbeForRead(prclTrg, sizeof(RECTL), 1);
        rclTrg = *prclTrg;

        ProbeForRead(psoTrgUMPD, sizeof(SURFOBJ), 1);
        hsurfTrg = psoTrgUMPD->hsurf;

        if (ROP4_USES_SOURCE(rop4))
        {
            ProbeForRead(pptlSrc, sizeof(POINTL), 1);
            ptlSrc = *pptlSrc;

            ProbeForRead(psoSrcUMPD, sizeof(SURFOBJ), 1);
            hsurfSrc = psoSrcUMPD->hsurf;
        }

        if (ROP4_USES_MASK(rop4))
        {
            ProbeForRead(pptlMask, sizeof(POINTL), 1);
            ptlMask = *pptlMask;

            ProbeForRead(psoMaskUMPD, sizeof(SURFOBJ), 1);
            hsurfMask = psoMaskUMPD->hsurf;
        }

        if (ROP4_USES_PATTERN(rop4))
        {
            ProbeForRead(pptlBrush, sizeof(POINTL), 1);
            ptlBrush = *pptlBrush;

            ProbeForRead(psoSrcUMPD, sizeof(SURFOBJ), 1);
            hBrushObj = pboUMPD->pvRbrush; // FIXME
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    // FIXME: these need to be converted/locked!
    psoTrg = NULL;
    psoSrc = NULL;
    psoMask = NULL;
    pco = NULL;
    pxlo = NULL;
    pbo = NULL;

    bResult = EngBitBlt(psoTrg,
                        psoSrc,
                        psoMask,
                        pco,
                        pxlo,
                        &rclTrg,
                        pptlSrc ? &ptlSrc : NULL,
                        pptlMask ? &ptlMask : NULL,
                        pbo,
                        pptlBrush ? &ptlBrush : NULL,
                        rop4);

    return bResult;
}


