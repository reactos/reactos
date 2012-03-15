
#include "DibLib.h"

#define ROP4_FGND(Rop4)    ((Rop4) & 0x00FF)
#define ROP4_BKGND(Rop4)    (((Rop4) & 0xFF00) >> 8)

#define ROP4_USES_SOURCE(Rop4)  (((((Rop4) & 0xCC00) >> 2) != ((Rop4) & 0x3300)) || ((((Rop4) & 0xCC) >> 2) != ((Rop4) & 0x33)))
#define ROP4_USES_MASK(Rop4)    (((Rop4) & 0xFF00) != (((Rop4) & 0xff) << 8))
#define ROP4_USES_DEST(Rop4)    (((((Rop4) & 0xAA) >> 1) != ((Rop4) & 0x55)) || ((((Rop4) & 0xAA00) >> 1) != ((Rop4) & 0x5500)))
#define ROP4_USES_PATTERN(Rop4) (((((Rop4) & 0xF0) >> 4) != ((Rop4) & 0x0F)) || ((((Rop4) & 0xF000) >> 4) != ((Rop4) & 0x0F00)))


typedef struct
{
    ULONG c;
    RECTL arcl[5];
} RECT_ENUM;

static
VOID
AdjustOffsetAndSize(
    _Out_ PPOINTL pptOffset,
    _Out_ PSIZEL psizTrg,
    _In_ PPOINTL pptlSrc,
    _In_ PSIZEL psizSrc)
{
    ULONG x, y, cxMax, cyMax;

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
    ULONG i, iDComplexity, iDirection, rop3Fg, iFunctionIndex;
    RECTL rcTrg;
    POINTL ptOffset, ptSrc, ptMask;
    SIZEL sizTrg;
    PFN_DIBFUNCTION pfnBitBlt;
    BOOL bEnumMore;
    RECT_ENUM rcenum;

    /* Clip the target rect to the extents of the target surface */
    rcTrg.left = min(prclTrg->left, 0);
    rcTrg.top = min(prclTrg->top, 0);
    rcTrg.right = min(prclTrg->right, psoTrg->sizlBitmap.cx);
    rcTrg.bottom = min(prclTrg->bottom, psoTrg->sizlBitmap.cy);

    /* Check if there is a clipping region */
    iDComplexity = pco ? pco->iDComplexity : DC_TRIVIAL;
    if (iDComplexity != DC_TRIVIAL)
    {
        /* Clip the target rect to the bounds of the clipping region */
        RECTL_bIntersectRect(&rcTrg, prclTrg, &pco->rclBounds);
    }

    /* Calculate initial offset and size */
    ptOffset.x = rcTrg.left - prclTrg->left;
    ptOffset.y = rcTrg.top - prclTrg->top;
    sizTrg.cx = rcTrg.right - rcTrg.left;
    sizTrg.cy = rcTrg.bottom - rcTrg.top;

    /* Check if the ROP uses a source */
    if (ROP4_USES_SOURCE(rop4))
    {
        /* Must have a source surface and point */
        if (!psoSrc || !pptlSrc) return FALSE;

        /* Get the source point */
        ptSrc = *pptlSrc;

        /* Clip against the extents of the source surface */
        AdjustOffsetAndSize(&ptOffset, &sizTrg, &ptSrc, &psoSrc->sizlBitmap);

        /* Check if source and target are equal */
        if (psoSrc == psoTrg)
        {
            /* Use 0 to mark source as equal to target */
            bltdata.siSrc.iFormat = 0;

        }
        else
        {
            bltdata.siSrc.iFormat = psoSrc->iBitmapFormat;
        }
        bltdata.siSrc.pjBase = psoSrc->pvScan0;
        bltdata.siSrc.lDelta = psoSrc->lDelta;
    }
    else
    {
        ptSrc.x = 0;
        ptSrc.y = 0;
    }

    /* Check if the ROP uses a mask */
    if (ROP4_USES_MASK(rop4))
    {
        /* Must have a mask surface and point */
        if (!psoMask || !pptlMask) return FALSE;

        /* Get the mask point */
        ptMask = *pptlMask;

        /* Clip against the extents of the mask surface */
        AdjustOffsetAndSize(&ptOffset, &sizTrg, &ptMask, &psoMask->sizlBitmap);

        bltdata.siMsk.iFormat = psoMask->iBitmapFormat;
        bltdata.siMsk.pjBase = psoMask->pvScan0;
        bltdata.siMsk.lDelta = psoMask->lDelta;
    }
    else
    {
        ptMask.x = 0;
        ptMask.y = 0;
    }

    /* Adjust the points */
    ptSrc.x += ptOffset.x;
    ptSrc.y += ptOffset.y;
    ptMask.x += ptOffset.x;
    ptMask.y += ptOffset.y;

    /* Recalculate the target rect */
    rcTrg.left = prclTrg->left + ptOffset.x;
    rcTrg.top = prclTrg->top + ptOffset.y;
    rcTrg.right = rcTrg.left + sizTrg.cx;
    rcTrg.bottom = rcTrg.top + sizTrg.cy;

    bltdata.ulWidth = prclTrg->right - prclTrg->left;
    bltdata.ulHeight = prclTrg->bottom - prclTrg->top;
    bltdata.pxlo = pxlo;
    bltdata.rop4 = rop4;

    bltdata.siDst.iFormat = psoTrg->iBitmapFormat;
    bltdata.siDst.pjBase = psoTrg->pvScan0;
    bltdata.siDst.lDelta = psoTrg->lDelta;
    bltdata.siDst.ptOrig.x = prclTrg->left;
    bltdata.siDst.ptOrig.y = prclTrg->top;


    /* Check of the ROP uses a pattern / brush */
    if (ROP4_USES_PATTERN(rop4))
    {
        /* Must have a brush */
        if (!pbo) return FALSE;

        /* Copy the solid color */
        bltdata.ulSolidColor = pbo->iSolidColor;

        /* Check if this is a pattern brush */
        if (pbo->iSolidColor == 0xFFFFFFFF)
        {
            // FIXME: use EBRUSHOBJ

            bltdata.siPat.iFormat = 0;//psoPat->iBitmapFormat;
            bltdata.siPat.pjBase = 0;//psoPat->pvScan0;
            bltdata.siPat.lDelta = 0;//psoPat->lDelta;
            bltdata.siPat.ptOrig = *pptlBrush;

            bltdata.ulPatWidth = 0;
            bltdata.ulPatHeight = 0;
        }
    }

    /* Analyze the copying direction */
    if (psoTrg == psoSrc)
    {
        if (rcTrg.top < ptSrc.y)
            iDirection = rcTrg.left < ptSrc.x ? CD_RIGHTDOWN : CD_LEFTDOWN;
        else
            iDirection = rcTrg.left < ptSrc.x ? CD_RIGHTUP : CD_LEFTUP;
    }
    else
        iDirection = CD_ANY;

    /* Check if this is a masking ROP */
    if (ROP4_USES_MASK(rop4))
    {
        bltdata.apfnDoRop[0] = gapfnRop[ROP4_BKGND(rop4)];
        bltdata.apfnDoRop[1] = gapfnRop[ROP4_FGND(rop4)];
    }

    /* Get the foreground ROP index */
    rop3Fg = ROP4_FGND(rop4);

    /* Get the function index */
    iFunctionIndex = aiIndexPerRop[rop3Fg];

    /* Get the dib function */
    pfnBitBlt = apfnDibFunction[iFunctionIndex];

    /* Check if we need to enumerate rects */
    if (iDComplexity == DC_COMPLEX)
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
                /* Intersect this rect with the already calculated bounds rect */
                if (!RECTL_bIntersectRect(&rcenum.arcl[i], &rcenum.arcl[i], &rcTrg))
                {
                    /* This rect is outside the valid bounds, continue */
                    continue;
                }

                /* Copy the target start point */
                bltdata.siDst.ptOrig.x = rcenum.arcl[i].left;
                bltdata.siDst.ptOrig.x = rcenum.arcl[i].top;

                /* Calculate width and height of this rect */
                bltdata.ulWidth = rcenum.arcl[i].right - rcenum.arcl[i].left;
                bltdata.ulHeight = rcenum.arcl[i].bottom - rcenum.arcl[i].top;

                /* Calculate the offset of this rect from the original coordinates */
                ptOffset.x = rcenum.arcl[i].left - prclTrg->left;
                ptOffset.y = rcenum.arcl[i].top - prclTrg->top;

                /* Calculate current origin for source and mask */
                bltdata.siSrc.ptOrig.x = pptlSrc->x + ptOffset.x;
                bltdata.siSrc.ptOrig.y = pptlSrc->y + ptOffset.y;
                bltdata.siMsk.ptOrig.x = pptlMask->x + ptOffset.x;
                bltdata.siMsk.ptOrig.y = pptlMask->y + ptOffset.y;

                //bltdata.siPat.ptOrig.x = (pptlMask->x + ptOffset.x) % psoMask->sizlBitmap.cx;
                //bltdata.siPat.ptOrig.y = (pptlMask->y + ptOffset.y) % psoMask->sizlBitmap.cx;

            }
        }
        while (bEnumMore);
    }
    else
    {
        /* Call the dib function */
        pfnBitBlt(&bltdata);
    }

    return TRUE;
}

ULONG
NTAPI
XLATEOBJ_iXlate(XLATEOBJ *pxlo, ULONG ulColor)
{
    return ulColor;
}

BOOL
//__fastcall
RECTL_bIntersectRect(RECTL* prclDst, const RECTL* prcl1, const RECTL* prcl2)
{
    return TRUE;
}

