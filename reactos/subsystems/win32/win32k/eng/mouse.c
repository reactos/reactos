/*
 * PROJECT:          ReactOS win32 subsystem
 * PURPOSE:          Mouse pointer functions
 * FILE:             subsystems/win32k/eng/mouse.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Timo Kreuzer (timo.kreuzer@reactos.org)
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
    SURFOBJ *pso,
    LONG HazardX1,
    LONG HazardY1,
    LONG HazardX2,
    LONG HazardY2)
{
    LONG tmp;
    PDEVOBJ *ppdev;
    GDIPOINTER *pgp;

    ASSERT(pso != NULL);

    ppdev = GDIDEV(pso);
    if (ppdev == NULL)
    {
        return FALSE;
    }

    pgp = &ppdev->Pointer;

    if (pgp->Exclude.right == -1)
    {
        return FALSE;
    }

    ppdev->SafetyRemoveCount++;

    if (ppdev->SafetyRemoveLevel != 0)
    {
        return FALSE;
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

    if (pgp->Exclude.right >= HazardX1
            && pgp->Exclude.left <= HazardX2
            && pgp->Exclude.bottom >= HazardY1
            && pgp->Exclude.top <= HazardY2)
    {
        ppdev->SafetyRemoveLevel = ppdev->SafetyRemoveCount;
        ppdev->pfnMovePointer(pso, -1, -1, NULL);
    }

    return(TRUE);
}

/*
 * FUNCTION: Notify the mouse driver that drawing has finished on a surface.
 */
INT INTERNAL_CALL
MouseSafetyOnDrawEnd(
    SURFOBJ *pso)
{
    PDEVOBJ *ppdev;
    GDIPOINTER *pgp;

    ASSERT(pso != NULL);

    ppdev = (PDEVOBJ*)pso->hdev;

    if (ppdev == NULL)
    {
        return(FALSE);
    }

    pgp = &ppdev->Pointer;

    if (pgp->Exclude.right == -1)
    {
        return FALSE;
    }

    if (--ppdev->SafetyRemoveCount >= ppdev->SafetyRemoveLevel)
    {
        return FALSE;
    }

    ppdev->pfnMovePointer(pso, gpsi->ptCursor.x, gpsi->ptCursor.y, &pgp->Exclude);

    ppdev->SafetyRemoveLevel = 0;

    return(TRUE);
}

/* SOFTWARE MOUSE POINTER IMPLEMENTATION **************************************/

VOID
INTERNAL_CALL
IntHideMousePointer(
    PDEVOBJ *ppdev,
    SURFOBJ *psoDest)
{
    GDIPOINTER *pgp;
    POINTL pt;
    RECTL rclDest;
    POINTL ptlSave;

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
        return;
    }

    if (!pgp->psurfSave)
    {
        DPRINT1("No SaveSurface!\n");
        return;
    }

    /* Calculate cursor coordinates */
    pt.x = ppdev->ptlPointer.x - pgp->HotSpot.x;
    pt.y = ppdev->ptlPointer.y - pgp->HotSpot.y;

    rclDest.left = max(pt.x, 0);
    rclDest.top = max(pt.y, 0);
    rclDest.right = min(pt.x + pgp->Size.cx, psoDest->sizlBitmap.cx);
    rclDest.bottom = min(pt.y + pgp->Size.cy, psoDest->sizlBitmap.cy);

    ptlSave.x = rclDest.left - pt.x;
    ptlSave.y = rclDest.top - pt.y;

    IntEngBitBltEx(psoDest,
                   &pgp->psurfSave->SurfObj,
                   NULL,
                   NULL,
                   NULL,
                   &rclDest,
                   &ptlSave,
                   &ptlSave,
                   NULL,
                   NULL,
                   ROP3_TO_ROP4(SRCCOPY),
                   FALSE);
}

VOID
INTERNAL_CALL
IntShowMousePointer(PDEVOBJ *ppdev, SURFOBJ *psoDest)
{
    GDIPOINTER *pgp;
    POINTL pt;
    RECTL rclSurf, rclPointer;

    ASSERT(ppdev);
    ASSERT(psoDest);

    pgp = &ppdev->Pointer;

    if (pgp->Enabled)
    {
        return;
    }

    pgp->Enabled = TRUE;

    /* Do not blt the pointer, if it is hidden */
    if (pgp->ShowPointer < 0)
    {
        return ;
    }

    /* Calculate pointer coordinates */
    pt.x = ppdev->ptlPointer.x - pgp->HotSpot.x;
    pt.y = ppdev->ptlPointer.y - pgp->HotSpot.y;

    /* Calculate the rect on the surface */
    rclSurf.left = max(pt.x, 0);
    rclSurf.top = max(pt.y, 0);
    rclSurf.right = min(pt.x + pgp->Size.cx, psoDest->sizlBitmap.cx);
    rclSurf.bottom = min(pt.y + pgp->Size.cy, psoDest->sizlBitmap.cy);

    /* Calculate the rect in the pointer bitmap */
    rclPointer.left = rclSurf.left - pt.x;
    rclPointer.top = rclSurf.top - pt.y;
    rclPointer.right = min(pgp->Size.cx, psoDest->sizlBitmap.cx - pt.x);
    rclPointer.bottom = min(pgp->Size.cy, psoDest->sizlBitmap.cy - pt.y);

    /* Copy the pixels under the cursor to temporary surface. */
    IntEngBitBltEx(&pgp->psurfSave->SurfObj,
                   psoDest,
                   NULL,
                   NULL,
                   NULL,
                   &rclPointer,
                   (POINTL*)&rclSurf,
                   NULL,
                   NULL,
                   NULL,
                   ROP3_TO_ROP4(SRCCOPY),
                   FALSE);

    /* Blt the pointer on the screen. */
    if (pgp->psurfColor)
    {
        IntEngBitBltEx(psoDest,
                       &pgp->psurfColor->SurfObj,
                       &pgp->psurfMask->SurfObj,
                       NULL,
                       pgp->XlateObject,
                       &rclSurf,
                       (POINTL*)&rclPointer,
                       (POINTL*)&rclPointer,
                       NULL,
                       NULL,
                       R4_MASK,
                       FALSE);
    }
    else
    {
        IntEngBitBltEx(psoDest,
                       &pgp->psurfMask->SurfObj,
                       NULL,
                       NULL,
                       pgp->XlateObject,
                       &rclSurf,
                       (POINTL*)&rclPointer,
                       NULL,
                       NULL,
                       NULL,
                       ROP3_TO_ROP4(SRCAND),
                       FALSE);

        rclPointer.top += pgp->Size.cy;

        IntEngBitBltEx(psoDest,
                       &pgp->psurfMask->SurfObj,
                       NULL,
                       NULL,
                       pgp->XlateObject,
                       &rclSurf,
                       (POINTL*)&rclPointer,
                       NULL,
                       NULL,
                       NULL,
                       ROP3_TO_ROP4(SRCINVERT),
                       FALSE);
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
    GDIPOINTER *pgp;
    PBYTE Bits;
    SIZEL Size;
    LONG lDelta;
    HBITMAP hbmp;

    ASSERT(pso);

    ppdev = GDIDEV(pso);
    pgp = &ppdev->Pointer;

    IntHideMousePointer(ppdev, pso);

    if (pgp->psurfColor)
    {
        /* FIXME: let GDI allocate/free memory */
        EngFreeMem(pgp->psurfColor->SurfObj.pvBits);
        pgp->psurfColor->SurfObj.pvBits = 0;

        EngDeleteSurface(pgp->psurfColor->BaseObject.hHmgr);
        SURFACE_ShareUnlockSurface(pgp->psurfColor);
        pgp->psurfColor = NULL;
    }

    if (pgp->psurfMask)
    {
        /* FIXME: let GDI allocate/free memory */
        EngFreeMem(pgp->psurfMask->SurfObj.pvBits);
        pgp->psurfMask->SurfObj.pvBits = 0;

        EngDeleteSurface(pgp->psurfMask->BaseObject.hHmgr);
        SURFACE_ShareUnlockSurface(pgp->psurfMask);
        pgp->psurfMask = NULL;
    }

    if (pgp->psurfSave != NULL)
    {
        EngDeleteSurface(pgp->psurfSave->BaseObject.hHmgr);
        SURFACE_ShareUnlockSurface(pgp->psurfSave);
        pgp->psurfSave = NULL;
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

    if (x != -1)
    {
        ppdev->ptlPointer.x = x;
        ppdev->ptlPointer.y = y;
    }

    pgp->Size.cx = abs(psoMask->lDelta) << 3;
    pgp->Size.cy = (psoMask->cjBits / abs(psoMask->lDelta)) >> 1;

    if (psoColor != NULL)
    {
        /* FIXME: let GDI allocate/free memory */
        Bits = EngAllocMem(0, psoColor->cjBits, TAG_MOUSE);
        if (Bits == NULL)
        {
            return SPS_ERROR;
        }

        memcpy(Bits, psoColor->pvBits, psoColor->cjBits);

        hbmp = EngCreateBitmap(pgp->Size,
                               psoColor->lDelta,
                               psoColor->iBitmapFormat,
                               psoColor->lDelta < 0 ? 0 : BMF_TOPDOWN,
                               Bits);

        pgp->psurfColor = SURFACE_ShareLockSurface(hbmp);
    }
    else
    {
        pgp->psurfColor = NULL;
    }

    Size.cx = pgp->Size.cx;
    Size.cy = pgp->Size.cy << 1;
    Bits = EngAllocMem(0, psoMask->cjBits, TAG_MOUSE);
    if (Bits == NULL)
    {
        return SPS_ERROR;
    }

    memcpy(Bits, psoMask->pvBits, psoMask->cjBits);

    hbmp = EngCreateBitmap(Size,
                           psoMask->lDelta,
                           psoMask->iBitmapFormat,
                           psoMask->lDelta < 0 ? 0 : BMF_TOPDOWN,
                           Bits);

    pgp->psurfMask = SURFACE_ShareLockSurface(hbmp);

    /* Create an XLATEOBJ that will be used for drawing masks.
     * FIXME: We should get this in pxlo parameter! */
    if (pxlo == NULL)
    {
        HPALETTE BWPalette, DestPalette;
        ULONG BWColors[] = {0, 0xFFFFFF};

        BWPalette = EngCreatePalette(PAL_INDEXED, sizeof(BWColors) / sizeof(ULONG),
                                     BWColors, 0, 0, 0);

        DestPalette = ppdev->DevInfo.hpalDefault;
        pgp->XlateObject = IntEngCreateXlate(0, 0,
                                             DestPalette, BWPalette);
        EngDeletePalette(BWPalette);
    }
    else
    {
        pgp->XlateObject = pxlo;
    }

    /* Create surface for saving the pixels under the cursor. */
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

    hbmp = EngCreateBitmap(pgp->Size,
                           lDelta,
                           pso->iBitmapFormat,
                           BMF_TOPDOWN | BMF_NOZEROINIT,
                           NULL);

    pgp->psurfSave = SURFACE_ShareLockSurface(hbmp);

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

    return SPS_ACCEPT_NOEXCLUDE;
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

    ppdev->ptlPointer.x = x;
    ppdev->ptlPointer.y = y;

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
}

VOID APIENTRY
IntEngMovePointer(
    IN SURFOBJ *pso,
    IN LONG x,
    IN LONG y,
    IN RECTL *prcl)
{
    SURFACE *psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
    PPDEVOBJ ppdev = (PPDEVOBJ)pso->hdev;

    SURFACE_LockBitmapBits(psurf);
    ppdev->pfnMovePointer(pso, x, y, prcl);
    SURFACE_UnlockBitmapBits(psurf);
}

ULONG APIENTRY
IntEngSetPointerShape(
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
    ULONG ulResult = SPS_DECLINE;
    SURFACE *psurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
    PFN_DrvSetPointerShape pfnSetPointerShape;
    PPDEVOBJ ppdev = GDIDEV(pso);

    pfnSetPointerShape = GDIDEVFUNCS(pso).SetPointerShape;

    SURFACE_LockBitmapBits(psurf);
    if (pfnSetPointerShape)
    {
        ulResult = pfnSetPointerShape(pso,
                                      psoMask,
                                      psoColor,
                                      pxlo,
                                      xHot,
                                      yHot,
                                      x,
                                      y,
                                      prcl,
                                      fl);
    }

    /* Check if the driver accepted it */
    if (ulResult == SPS_ACCEPT_NOEXCLUDE)
    {
        /* Set MovePointer to the driver function */
        ppdev->pfnMovePointer = GDIDEVFUNCS(pso).MovePointer;
    }
    else
    {
        /* Set software pointer */
        ulResult = EngSetPointerShape(pso,
                                      psoMask,
                                      psoColor,
                                      pxlo,
                                      xHot,
                                      yHot,
                                      x,
                                      y,
                                      prcl,
                                      fl);
        /* Set MovePointer to the eng function */
        ppdev->pfnMovePointer = EngMovePointer;
    }

    SURFACE_UnlockBitmapBits(psurf);

    return ulResult;
}

/* EOF */
