/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/rect.c
 * PURPOSE:         Graphic engine: rectangles
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#include "object.h"
#include "handle.h"
#include "user.h"
#define NDEBUG
#include <debug.h>

extern PDEVOBJ PrimarySurface;

/* PUBLIC FUNCTIONS **********************************************************/

VOID
NTAPI
GreRectangle(PDC pDC,
             INT LeftRect,
             INT TopRect,
             INT RightRect,
             INT BottomRect)
{
    BOOLEAN bRet;
    RECTL DestRect;
    MIX Mix;
    POINTL BrushOrigin;

    DestRect.left = LeftRect + pDC->ptlDCOrig.x;
    DestRect.right = RightRect + pDC->ptlDCOrig.x;
    DestRect.top = TopRect + pDC->ptlDCOrig.y;
    DestRect.bottom = BottomRect + pDC->ptlDCOrig.y;

    BrushOrigin.x = pDC->dclevel.ptlBrushOrigin.x;
    BrushOrigin.y = pDC->dclevel.ptlBrushOrigin.y;

    /* Draw brush-based rectangle */
    if (pDC->dclevel.pbrFill)
    {
        if (!(pDC->dclevel.pbrFill->flAttrs & GDIBRUSH_IS_NULL))
        {
            bRet = GrepBitBltEx(&pDC->dclevel.pSurface->SurfObj,
                               NULL,
                               NULL,
                               pDC->CombinedClip,
                               NULL,
                               &DestRect,
                               NULL,
                               NULL,
                               &pDC->eboFill.BrushObject,
                               &BrushOrigin,
                               ROP3_TO_ROP4(PATCOPY),
                               TRUE);
            UNREFERENCED_LOCAL_VARIABLE(bRet);
        }
    }


    /* Draw pen-based rectangle */
    if (!(pDC->dclevel.pbrLine->flAttrs & GDIBRUSH_IS_NULL))
    {
        Mix = ROP2_TO_MIX(R2_COPYPEN);/*pdcattr->jROP2*/
        GreLineTo(&pDC->dclevel.pSurface->SurfObj,
                  pDC->CombinedClip,
                  &pDC->eboLine.BrushObject,
                  DestRect.left, DestRect.top, DestRect.right, DestRect.top,
                  &DestRect, // Bounding rectangle
                  Mix);

        GreLineTo(&pDC->dclevel.pSurface->SurfObj,
                  pDC->CombinedClip,
                  &pDC->eboLine.BrushObject,
                  DestRect.right, DestRect.top, DestRect.right, DestRect.bottom,
                  &DestRect, // Bounding rectangle
                  Mix);

        GreLineTo(&pDC->dclevel.pSurface->SurfObj,
                  pDC->CombinedClip,
                  &pDC->eboLine.BrushObject,
                  DestRect.right, DestRect.bottom, DestRect.left, DestRect.bottom,
                  &DestRect, // Bounding rectangle
                  Mix);

        GreLineTo(&pDC->dclevel.pSurface->SurfObj,
                  pDC->CombinedClip,
                  &pDC->eboLine.BrushObject,
                  DestRect.left, DestRect.bottom, DestRect.left, DestRect.top,
                  &DestRect, // Bounding rectangle
                  Mix);
    }
}

VOID
NTAPI
GrePolygon(PDC pDC,
           const POINT *ptPoints,
           INT count,
           PRECTL DestRect)
{
    MIX Mix;
    INT i;
    POINTL BrushOrigin;

    BrushOrigin.x = pDC->dclevel.ptlBrushOrigin.x;
    BrushOrigin.y = pDC->dclevel.ptlBrushOrigin.y;

    /* Draw brush-based polygon */
    if (pDC->dclevel.pbrFill)
    {
        if (!(pDC->dclevel.pbrFill->flAttrs & GDIBRUSH_IS_NULL))
        {
            GrepFillPolygon(pDC,
                            &pDC->dclevel.pSurface->SurfObj,
                            &pDC->eboFill.BrushObject,
                            ptPoints,
                            count,
                            DestRect,
                            &BrushOrigin);
        }
    }

    /* Draw pen-based polygon */
    if (!(pDC->dclevel.pbrLine->flAttrs & GDIBRUSH_IS_NULL))
    {
        Mix = ROP2_TO_MIX(R2_COPYPEN);/*pdcattr->jROP2*/
        for (i=0; i<count-1; i++)
        {
            GreLineTo(&pDC->dclevel.pSurface->SurfObj,
                             pDC->CombinedClip,
                             &pDC->eboLine.BrushObject,
                             ptPoints[i].x,
                             ptPoints[i].y,
                             ptPoints[i+1].x,
                             ptPoints[i+1].y,
                             DestRect,
                             Mix);
        }
    }
}

BOOL NTAPI GreFloodFill( PDC dc, POINTL *Pt, COLORREF Color, UINT FillType )
{
    SURFACE    *psurf = NULL;
    HPALETTE   hpal;
    PPALETTE   ppal;
    EXLATEOBJ  exlo;
    BOOL       Ret = FALSE;
    RECTL      DestRect;
    ULONG      ConvColor;
    rectangle_t r;

    Ret = point_in_region(dc->Clipping, Pt->x, Pt->y);
    if (!Ret)
        return FALSE;

    get_region_extents(dc->Clipping ,&r);

    DestRect.bottom = r.bottom;
    DestRect.left = r.left;
    DestRect.right = r.right;
    DestRect.top = r.top;

    psurf = dc->dclevel.pSurface;
    if (!psurf)
        return FALSE;

    hpal = dc->dclevel.pSurface->hDIBPalette;
    if (!hpal) hpal = pPrimarySurface->devinfo.hpalDefault;
    ppal = PALETTE_ShareLockPalette(hpal);
    
    EXLATEOBJ_vInitialize(&exlo, &gpalRGB, ppal, 0, 0xffffff, 0);

    /* Only solid fills supported for now
     * How to support pattern brushes and non standard surfaces (not offering dib functions):
     * Version a (most likely slow): call DrvPatBlt for every pixel
     * Version b: create a flood mask and let MaskBlt blit a masked brush */
    ConvColor = XLATEOBJ_iXlate(&exlo.xlo, Color);
    Ret = DIB_XXBPP_FloodFillSolid(&psurf->SurfObj, &dc->eboFill.BrushObject, &DestRect, Pt, ConvColor, FillType);

    EXLATEOBJ_vCleanup(&exlo);
    PALETTE_ShareUnlockPalette(ppal);

    return Ret;
}

BOOLEAN
NTAPI
RECTL_bIntersectRect(RECTL* prclDst, const RECTL* prcl1, const RECTL* prcl2)
{
    prclDst->left  = max(prcl1->left, prcl2->left);
    prclDst->right = min(prcl1->right, prcl2->right);

    if (prclDst->left < prclDst->right)
    {
        prclDst->top    = max(prcl1->top, prcl2->top);
        prclDst->bottom = min(prcl1->bottom, prcl2->bottom);

        if (prclDst->top < prclDst->bottom)
        {
            return TRUE;
        }
    }

    RECTL_vSetEmptyRect(prclDst);

    return FALSE;
}


/* EOF */
