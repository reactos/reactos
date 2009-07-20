/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/rect.c
 * PURPOSE:         Graphic engine: rectangles
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

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
    POINT BrushOrigin = {0, 0};

    DestRect.left = LeftRect + pDC->rcDcRect.left;
    DestRect.right = RightRect + pDC->rcDcRect.left;
    DestRect.top = TopRect + pDC->rcDcRect.top;
    DestRect.bottom = BottomRect + pDC->rcDcRect.top;

    /* Draw brush-based rectangle */
    if (pDC->pFillBrush)
    {
        if (!(pDC->pFillBrush->flAttrs & GDIBRUSH_IS_NULL))
        {
            //BrushOrigin = *((PPOINTL)&pbrFill->ptOrigin);
            //BrushOrigin.x += dc->ptlDCOrig.x;
            //BrushOrigin.y += dc->ptlDCOrig.y;
            bRet = GrepBitBltEx(&pDC->pBitmap->SurfObj,
                               NULL,
                               NULL,
                               NULL,//dc->rosdc.CombinedClip,
                               NULL,
                               &DestRect,
                               NULL,
                               NULL,
                               &pDC->pFillBrush->BrushObj,
                               &BrushOrigin,
                               ROP3_TO_ROP4(PATCOPY),
                               TRUE);
        }
    }


    /* Draw pen-based rectangle */
    if (!(pDC->pLineBrush->flAttrs & GDIBRUSH_IS_NULL))
    {
        Mix = 0;//ROP2_TO_MIX(pdcattr->jROP2);
        GreLineTo(&pDC->pBitmap->SurfObj,
                  NULL,//dc->rosdc.CombinedClip,
                  &pDC->pLineBrush->BrushObj,
                  DestRect.left, DestRect.top, DestRect.right, DestRect.top,
                  &DestRect, // Bounding rectangle
                  Mix);

        GreLineTo(&pDC->pBitmap->SurfObj,
                  NULL,//dc->rosdc.CombinedClip,
                  &pDC->pLineBrush->BrushObj,
                  DestRect.right, DestRect.top, DestRect.right, DestRect.bottom,
                  &DestRect, // Bounding rectangle
                  Mix);

        GreLineTo(&pDC->pBitmap->SurfObj,
                  NULL,//dc->rosdc.CombinedClip,
                  &pDC->pLineBrush->BrushObj,
                  DestRect.right, DestRect.bottom, DestRect.left, DestRect.bottom,
                  &DestRect, // Bounding rectangle
                  Mix);

        GreLineTo(&pDC->pBitmap->SurfObj,
                  NULL,//dc->rosdc.CombinedClip,
                  &pDC->pLineBrush->BrushObj,
                  DestRect.left, DestRect.bottom, DestRect.left, DestRect.top,
                  &DestRect, // Bounding rectangle
                  Mix);
    }
}

VOID
NTAPI
GrePolygon(PDC pDC,
           const POINT *ptPoints,
           INT count)
{
    BOOLEAN bRet;
    RECTL DestRect;
    MIX Mix;
    INT i;
    //POINT BrushOrigin = {0, 0};

    // HACK
    DestRect.left = 0;
    DestRect.top = 0;
    DestRect.bottom = 600;
    DestRect.right = 800;

    /* Draw brush-based polygon */
    if (pDC->pFillBrush)
    {
        if (!(pDC->pFillBrush->flAttrs & GDIBRUSH_IS_NULL))
        {
            //BrushOrigin = *((PPOINTL)&pbrFill->ptOrigin);
            //BrushOrigin.x += dc->ptlDCOrig.x;
            //BrushOrigin.y += dc->ptlDCOrig.y;
#if 0
            bRet = GrepBitBltEx(&pDC->pBitmap->SurfObj,
                               NULL,
                               NULL,
                               NULL,//dc->rosdc.CombinedClip,
                               NULL,
                               &DestRect,
                               NULL,
                               NULL,
                               &pDC->pFillBrush->BrushObj,
                               &BrushOrigin,
                               ROP3_TO_ROP4(PATCOPY),
                               TRUE);
#endif
        }
    }

    /* Draw pen-based polygon */
    if (!(pDC->pLineBrush->flAttrs & GDIBRUSH_IS_NULL))
    {
        Mix = 0;//ROP2_TO_MIX(pdcattr->jROP2);
        for (i=0; i<count-1; i++)
        {
            bRet = GreLineTo(&pDC->pBitmap->SurfObj,
                             NULL,//dc->rosdc.CombinedClip,
                             &pDC->pLineBrush->BrushObj,
                             ptPoints[i].x + pDC->rcDcRect.left,
                             ptPoints[i].y + pDC->rcDcRect.top,
                             ptPoints[i+1].x + pDC->rcDcRect.left,
                             ptPoints[i+1].y + pDC->rcDcRect.top,
                             &DestRect, // Bounding rectangle
                             Mix);
        }
    }
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
