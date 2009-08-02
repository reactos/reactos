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
    POINT BrushOrigin = {0, 0};

    DestRect.left = LeftRect + pDC->rcDcRect.left + pDC->rcVport.left;
    DestRect.right = RightRect + pDC->rcDcRect.left + pDC->rcVport.left;
    DestRect.top = TopRect + pDC->rcDcRect.top + pDC->rcVport.top;
    DestRect.bottom = BottomRect + pDC->rcDcRect.top + pDC->rcVport.top;

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
                               pDC->CombinedClip,
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
        Mix = ROP2_TO_MIX(R2_COPYPEN);/*pdcattr->jROP2*/
        GreLineTo(&pDC->pBitmap->SurfObj,
                  pDC->CombinedClip,
                  &pDC->pLineBrush->BrushObj,
                  DestRect.left, DestRect.top, DestRect.right, DestRect.top,
                  &DestRect, // Bounding rectangle
                  Mix);

        GreLineTo(&pDC->pBitmap->SurfObj,
                  pDC->CombinedClip,
                  &pDC->pLineBrush->BrushObj,
                  DestRect.right, DestRect.top, DestRect.right, DestRect.bottom,
                  &DestRect, // Bounding rectangle
                  Mix);

        GreLineTo(&pDC->pBitmap->SurfObj,
                  pDC->CombinedClip,
                  &pDC->pLineBrush->BrushObj,
                  DestRect.right, DestRect.bottom, DestRect.left, DestRect.bottom,
                  &DestRect, // Bounding rectangle
                  Mix);

        GreLineTo(&pDC->pBitmap->SurfObj,
                  pDC->CombinedClip,
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
    POINT BrushOrigin = {0, 0};

    // HACK
    DestRect.left = 0;
    DestRect.top = 0;
    DestRect.bottom = PrimarySurface.GDIInfo.ulVertRes;
    DestRect.right = PrimarySurface.GDIInfo.ulHorzRes;

    /* Draw brush-based polygon */
    if (pDC->pFillBrush)
    {
        if (!(pDC->pFillBrush->flAttrs & GDIBRUSH_IS_NULL))
        {
            //BrushOrigin = *((PPOINTL)&pbrFill->ptOrigin);
            //BrushOrigin.x += dc->ptlDCOrig.x;
            //BrushOrigin.y += dc->ptlDCOrig.y;
            GrepFillPolygon(pDC,
                            &pDC->pBitmap->SurfObj,
                            &pDC->pFillBrush->BrushObj,
                            ptPoints,
                            count,
                            DestRect,
                            &BrushOrigin);
        }
    }

    /* Draw pen-based polygon */
    if (!(pDC->pLineBrush->flAttrs & GDIBRUSH_IS_NULL))
    {
        Mix = ROP2_TO_MIX(R2_COPYPEN);/*pdcattr->jROP2*/
        for (i=0; i<count-1; i++)
        {
            bRet = GreLineTo(&pDC->pBitmap->SurfObj,
                             pDC->CombinedClip,
                             &pDC->pLineBrush->BrushObj,
                             ptPoints[i].x,
                             ptPoints[i].y,
                             ptPoints[i+1].x,
                             ptPoints[i+1].y,
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
