/* 
 * PROJECT:          ReactOS win32 subsystem
 * PURPOSE:          Mouse pointer functions
 * FILE:             subsystems/win32k/eng/mouse.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * FUNCTION: Notify the mouse driver that drawing is about to begin in
 * a rectangle on a particular surface.
 */
INT INTERNAL_CALL
MouseSafetyOnDrawStart(
    SURFOBJ *SurfObj,
    LONG HazardX1,
    LONG HazardY1,
    LONG HazardX2,
    LONG HazardY2)
{
    LONG tmp;
    PDEVOBJ *ppdev;
    GDIPOINTER *pgp;

    ASSERT(SurfObj != NULL);

    ppdev = GDIDEV(SurfObj);
    if (ppdev == NULL)
    {
        return(FALSE);
    }

    pgp = &ppdev->Pointer;

    if (SPS_ACCEPT_NOEXCLUDE == pgp->Status ||
            pgp->Exclude.right == -1)
    {
        return(FALSE);
    }

    if (HazardX1 > HazardX2)
    {
        tmp = HazardX2;
        HazardX2 = HazardX1;
        HazardX1 = tmp;
    }
    if (HazardY1 > HazardY2)
    {
        tmp = HazardY2;
        HazardY2 = HazardY1;
        HazardY1 = tmp;
    }

    if (ppdev->SafetyRemoveLevel != 0)
    {
        ppdev->SafetyRemoveCount++;
        return FALSE;
    }

    ppdev->SafetyRemoveCount++;

    if (pgp->Exclude.right >= HazardX1
            && pgp->Exclude.left <= HazardX2
            && pgp->Exclude.bottom >= HazardY1
            && pgp->Exclude.top <= HazardY2)
    {
        ppdev->SafetyRemoveLevel = ppdev->SafetyRemoveCount;
        if (pgp->MovePointer)
            pgp->MovePointer(SurfObj, -1, -1, NULL);
        else
            EngMovePointer(SurfObj, -1, -1, NULL);
    }

    return(TRUE);
}

/*
 * FUNCTION: Notify the mouse driver that drawing has finished on a surface.
 */
INT INTERNAL_CALL
MouseSafetyOnDrawEnd(
    SURFOBJ *SurfObj)
{
    PDEVOBJ *ppdev;
    GDIPOINTER *pgp;

    ASSERT(SurfObj != NULL);

    ppdev = GDIDEV(SurfObj);

    if (ppdev == NULL)
    {
        return(FALSE);
    }

    pgp = &ppdev->Pointer;

    if (SPS_ACCEPT_NOEXCLUDE == pgp->Status ||
            pgp->Exclude.right == -1)
    {
        return FALSE;
    }

    if (--ppdev->SafetyRemoveCount >= ppdev->SafetyRemoveLevel)
    {
        return FALSE;
    }
    if (pgp->MovePointer)
        pgp->MovePointer(SurfObj, gpsi->ptCursor.x, gpsi->ptCursor.y, &pgp->Exclude);
    else
        EngMovePointer(SurfObj, gpsi->ptCursor.x, gpsi->ptCursor.y, &pgp->Exclude);

    ppdev->SafetyRemoveLevel = 0;

    return(TRUE);
}

/* SOFTWARE MOUSE POINTER IMPLEMENTATION **************************************/

VOID INTERNAL_CALL
IntHideMousePointer(
    PDEVOBJ *ppdev,
    SURFOBJ *psoDest)
{
    GDIPOINTER *pgp;
    POINTL pt;

    ASSERT(ppdev);
    ASSERT(psoDest);

    pgp = &ppdev->Pointer;

    if (!pgp->Enabled)
    {
        return;
    }

    pgp->Enabled = FALSE;

    /* The mouse is hide from ShowCours and it is frist ?? */
    if (pgp->ShowPointer < 0)
    {
        return ;
    }

    /* Hide the cours */
    pt.x = gpsi->ptCursor.x - pgp->HotSpot.x;
    pt.y = gpsi->ptCursor.y - pgp->HotSpot.y;


    if (pgp->SaveSurface != NULL)
    {
        RECTL DestRect;
        POINTL SrcPoint;
        SURFOBJ *SaveSurface;
        SURFOBJ *MaskSurface;

        DestRect.left = max(pt.x, 0);
        DestRect.top = max(pt.y, 0);
        DestRect.right = min(
                             pt.x + pgp->Size.cx,
                             psoDest->sizlBitmap.cx);
        DestRect.bottom = min(
                              pt.y + pgp->Size.cy,
                              psoDest->sizlBitmap.cy);

        SrcPoint.x = max(-pt.x, 0);
        SrcPoint.y = max(-pt.y, 0);

        if ((SaveSurface = EngLockSurface(pgp->SaveSurface)))
        {
            if ((MaskSurface = EngLockSurface(pgp->MaskSurface)))
            {
                IntEngBitBltEx(psoDest, SaveSurface, MaskSurface, NULL, NULL,
                               &DestRect, &SrcPoint, &SrcPoint, NULL, NULL,
                               ROP3_TO_ROP4(SRCCOPY), FALSE);
                EngUnlockSurface(MaskSurface);
            }
            EngUnlockSurface(SaveSurface);
        }
    }
}

VOID INTERNAL_CALL
IntShowMousePointer(PDEVOBJ *ppdev, SURFOBJ *psoDest)
{
    GDIPOINTER *pgp;
    SURFOBJ *SaveSurface;
    POINTL pt;

    ASSERT(ppdev);
    ASSERT(psoDest);

    pgp = &ppdev->Pointer;

    if (pgp->Enabled)
    {
        return;
    }

    pgp->Enabled = TRUE;

    /* Do not blt the mouse if it in hide */
    if (pgp->ShowPointer < 0)
    {
        return ;
    }

    pt.x = gpsi->ptCursor.x - pgp->HotSpot.x;
    pt.y = gpsi->ptCursor.y - pgp->HotSpot.y;

    /* Copy the pixels under the cursor to temporary surface. */
    if (pgp->SaveSurface != NULL &&
            (SaveSurface = EngLockSurface(pgp->SaveSurface)))
    {
        RECTL DestRect;
        POINTL SrcPoint;

        SrcPoint.x = max(pt.x, 0);
        SrcPoint.y = max(pt.y, 0);

        DestRect.left = SrcPoint.x - pt.x;
        DestRect.top = SrcPoint.y - pt.y;
        DestRect.right = min(
                             pgp->Size.cx,
                             psoDest->sizlBitmap.cx - pt.x);
        DestRect.bottom = min(
                              pgp->Size.cy,
                              psoDest->sizlBitmap.cy - pt.y);

        IntEngBitBltEx(SaveSurface, psoDest, NULL, NULL, NULL,
                       &DestRect, &SrcPoint, NULL, NULL, NULL,
                       ROP3_TO_ROP4(SRCCOPY), FALSE);
        EngUnlockSurface(SaveSurface);
    }


    /* Blit the cursor on the screen. */
    {
        RECTL DestRect;
        POINTL SrcPoint;
        SURFOBJ *psoColor;
        SURFOBJ *psoMask = NULL;

        DestRect.left = max(pt.x, 0);
        DestRect.top = max(pt.y, 0);
        DestRect.right = min(
                             pt.x + pgp->Size.cx,
                             psoDest->sizlBitmap.cx);
        DestRect.bottom = min(
                              pt.y + pgp->Size.cy,
                              psoDest->sizlBitmap.cy);

        SrcPoint.x = max(-pt.x, 0);
        SrcPoint.y = max(-pt.y, 0);

        if (pgp->MaskSurface)
            psoMask = EngLockSurface(pgp->MaskSurface);

        if (psoMask != NULL)
        {
            if (pgp->ColorSurface != NULL)
            {
                if ((psoColor = EngLockSurface(pgp->ColorSurface)))
                {
                    IntEngBitBltEx(psoDest, psoColor, psoMask, NULL,
                                   pgp->XlateObject, &DestRect, &SrcPoint, &SrcPoint,
                                   NULL, NULL, R4_MASK, FALSE);
                    EngUnlockSurface(psoColor);
                }
            }
            else
            {
                IntEngBitBltEx(psoDest, psoMask, NULL, NULL, pgp->XlateObject,
                               &DestRect, &SrcPoint, NULL, NULL, NULL,
                               ROP3_TO_ROP4(SRCAND), FALSE);
                SrcPoint.y += pgp->Size.cy;
                IntEngBitBltEx(psoDest, psoMask, NULL, NULL, pgp->XlateObject,
                               &DestRect, &SrcPoint, NULL, NULL, NULL,
                               ROP3_TO_ROP4(SRCINVERT), FALSE);
            }
            EngUnlockSurface(psoMask);
        }
    }
}

/*
 * @implemented
 */
ULONG APIENTRY
EngSetPointerShape(
    IN SURFOBJ *pso,
    IN SURFOBJ *psoMask,
    IN SURFOBJ *psoColor,
    IN XLATEOBJ *pxlo,
    IN LONG xHot,
    IN LONG yHot,
    IN LONG x,
    IN LONG y,
    IN RECTL *prcl,
    IN FLONG fl)
{
    PDEVOBJ *ppdev;
    SURFOBJ *psoTemp;
    GDIPOINTER *pgp;

    ASSERT(pso);

    ppdev = GDIDEV(pso);
    pgp = &ppdev->Pointer;

    IntHideMousePointer(ppdev, pso);

    if (pgp->ColorSurface != NULL)
    {
        /* FIXME: Is this really needed? */
        if ((psoTemp = EngLockSurface(pgp->ColorSurface)))
        {
            EngFreeMem(psoTemp->pvBits);
            psoTemp->pvBits = 0;
            EngUnlockSurface(psoTemp);
        }

        EngDeleteSurface(pgp->ColorSurface);
        pgp->MaskSurface = NULL;
    }

    if (pgp->MaskSurface != NULL)
    {
        /* FIXME: Is this really needed? */
        if ((psoTemp = EngLockSurface(pgp->MaskSurface)))
        {
            EngFreeMem(psoTemp->pvBits);
            psoTemp->pvBits = 0;
            EngUnlockSurface(psoTemp);
        }

        EngDeleteSurface(pgp->MaskSurface);
        pgp->MaskSurface = NULL;
    }

    if (pgp->SaveSurface != NULL)
    {
        EngDeleteSurface(pgp->SaveSurface);
        pgp->SaveSurface = NULL;
    }

    if (pgp->XlateObject != NULL)
    {
        EngDeleteXlate(pgp->XlateObject);
        pgp->XlateObject = NULL;
    }

    /* See if we are being asked to hide the pointer. */
    if (psoMask == NULL)
    {
        return SPS_ACCEPT_NOEXCLUDE;
    }

    pgp->HotSpot.x = xHot;
    pgp->HotSpot.y = yHot;

    /* Actually this should be set by 'the other side', but it would be
     * done right after this. It helps IntShowMousePointer. */
    if (x != -1)
    {
        gpsi->ptCursor.x = x;
        gpsi->ptCursor.y = y;
    }

    pgp->Size.cx = abs(psoMask->lDelta) << 3;
    pgp->Size.cy = (psoMask->cjBits / abs(psoMask->lDelta)) >> 1;

    if (psoColor != NULL)
    {
        PBYTE Bits;

        Bits = EngAllocMem(0, psoColor->cjBits, TAG_MOUSE);
        if (Bits == NULL)
        {
            return SPS_ERROR;
        }

        memcpy(Bits, psoColor->pvBits, psoColor->cjBits);

        pgp->ColorSurface = (HSURF)EngCreateBitmap(pgp->Size,
                            psoColor->lDelta, psoColor->iBitmapFormat,
                            psoColor->lDelta < 0 ? 0 : BMF_TOPDOWN, Bits);
    }
    else
    {
        pgp->ColorSurface = NULL;
    }

    {
        SIZEL Size;
        PBYTE Bits;

        Size.cx = pgp->Size.cx;
        Size.cy = pgp->Size.cy << 1;
        Bits = EngAllocMem(0, psoMask->cjBits, TAG_MOUSE);
        if (Bits == NULL)
        {
            return SPS_ERROR;
        }

        memcpy(Bits, psoMask->pvBits, psoMask->cjBits);

        pgp->MaskSurface = (HSURF)EngCreateBitmap(Size,
                           psoMask->lDelta, psoMask->iBitmapFormat,
                           psoMask->lDelta < 0 ? 0 : BMF_TOPDOWN, Bits);
    }

    /* Create an XLATEOBJ that will be used for drawing masks.
     * FIXME: We should get this in pxlo parameter! */
    if (pxlo == NULL)
    {
        HPALETTE BWPalette, DestPalette;
        ULONG BWColors[] = {0, 0xFFFFFF};

        BWPalette = EngCreatePalette(PAL_INDEXED, sizeof(BWColors) / sizeof(ULONG),
                                     BWColors, 0, 0, 0);

        DestPalette = ppdev->DevInfo.hpalDefault;
        pgp->XlateObject = IntEngCreateXlate(0, PAL_INDEXED,
                                             DestPalette, BWPalette);
        EngDeletePalette(BWPalette);
    }
    else
    {
        pgp->XlateObject = pxlo;
    }

    /* Create surface for saving the pixels under the cursor. */
    {
        LONG lDelta;

        switch (pso->iBitmapFormat)
        {
            case BMF_1BPP:
                lDelta = pgp->Size.cx >> 3;
                break;
            case BMF_4BPP:
                lDelta = pgp->Size.cx >> 1;
                break;
            case BMF_8BPP:
                lDelta = pgp->Size.cx;
                break;
            case BMF_16BPP:
                lDelta = pgp->Size.cx << 1;
                break;
            case BMF_24BPP:
                lDelta = pgp->Size.cx * 3;
                break;
            case BMF_32BPP:
                lDelta = pgp->Size.cx << 2;
                break;
            default:
                lDelta = 0;
                break;
        }

        pgp->SaveSurface = (HSURF)EngCreateBitmap(pgp->Size,
                                                  lDelta,
                                                  pso->iBitmapFormat,
                                                  BMF_TOPDOWN | BMF_NOZEROINIT, NULL);
    }

    if (x != -1)
    {
        IntShowMousePointer(ppdev, pso);

        if (prcl != NULL)
        {
            prcl->left = x - pgp->HotSpot.x;
            prcl->top = y - pgp->HotSpot.x;
            prcl->right = prcl->left + pgp->Size.cx;
            prcl->bottom = prcl->top + pgp->Size.cy;
        }
    } else if (prcl != NULL)
        prcl->left = prcl->top = prcl->right = prcl->bottom = -1;

    return SPS_ACCEPT_EXCLUDE;
}

/*
 * @implemented
 */

VOID APIENTRY
EngMovePointer(
    IN SURFOBJ *pso,
    IN LONG x,
    IN LONG y,
    IN RECTL *prcl)
{
    PDEVOBJ *ppdev;
    GDIPOINTER *pgp;

    ASSERT(pso);

    ppdev = GDIDEV(pso);

    ASSERT(ppdev);

    pgp = &ppdev->Pointer;

    IntHideMousePointer(ppdev, pso);
    if (x != -1)
    {
        /* Actually this should be set by 'the other side', but it would be
         * done right after this. It helps IntShowMousePointer. */
        gpsi->ptCursor.x = x;
        gpsi->ptCursor.y = y;
        IntShowMousePointer(ppdev, pso);
        if (prcl != NULL)
        {
            prcl->left = x - pgp->HotSpot.x;
            prcl->top = y - pgp->HotSpot.x;
            prcl->right = prcl->left + pgp->Size.cx;
            prcl->bottom = prcl->top + pgp->Size.cy;
        }
    } else if (prcl != NULL)
        prcl->left = prcl->top = prcl->right = prcl->bottom = -1;

}

VOID APIENTRY
IntEngMovePointer(
    IN SURFOBJ *pso,
    IN LONG x,
    IN LONG y,
    IN RECTL *prcl)
{
    SURFACE *psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);

    SURFACE_LockBitmapBits(psurf);
    if (GDIDEV(pso)->Pointer.MovePointer)
    {
        GDIDEV(pso)->Pointer.MovePointer(pso, x, y, prcl);
    }
    else
    {
        EngMovePointer(pso, x, y, prcl);
    }
    SURFACE_UnlockBitmapBits(psurf);
}

/* EOF */
