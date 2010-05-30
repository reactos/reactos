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

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL
APIENTRY
EngSetPointerTag(
	IN HDEV hdev,
	IN SURFOBJ *psoMask,
	IN SURFOBJ *psoColor,
	IN XLATEOBJ *pxlo,
	IN FLONG fl)
{
    // This function is obsolete for Windows 2000 and later.
    // This function is still supported, but always returns FALSE.
    // www.osr.com/ddk/graphics/gdifncs_4yav.htm
    return FALSE;
}

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
                       &pgp->psurfMask->SurfObj,
                       NULL,
                       NULL,
                       NULL,
                       &rclSurf,
                       (POINTL*)&rclPointer,
                       NULL,
                       NULL,
                       NULL,
                       ROP3_TO_ROP4(SRCAND),
                       FALSE);

        IntEngBitBltEx(psoDest,
                       &pgp->psurfColor->SurfObj,
                       NULL,
                       NULL,
                       NULL,
                       &rclSurf,
                       (POINTL*)&rclPointer,
                       NULL,
                       NULL,
                       NULL,
                       ROP3_TO_ROP4(SRCINVERT),
                       FALSE);
    }
    else
    {
        IntEngBitBltEx(psoDest,
                       &pgp->psurfMask->SurfObj,
                       NULL,
                       NULL,
                       NULL,
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
                       NULL,
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
    LONG lDelta;
    HBITMAP hbmp;
    RECTL rcl;

    ASSERT(pso);

    ppdev = GDIDEV(pso);
    pgp = &ppdev->Pointer;

    if (psoColor)
    {
        pgp->Size.cx = psoColor->sizlBitmap.cx;
        pgp->Size.cy = psoColor->sizlBitmap.cy;
        if (psoMask)
        {
            // CHECKME: Is this really required? if we have a color surface,
            // we only need the AND part of the mask.
            /* Check if the sizes match as they should */
            if (psoMask->sizlBitmap.cx != psoColor->sizlBitmap.cx ||
                psoMask->sizlBitmap.cy != psoColor->sizlBitmap.cy * 2)
            {
                DPRINT("Sizes of mask (%ld,%ld) and color (%ld,%ld) don't match\n",
                       psoMask->sizlBitmap.cx, psoMask->sizlBitmap.cy,
                       psoColor->sizlBitmap.cx, psoColor->sizlBitmap.cy);
//                return SPS_ERROR;
            }
        }
    }
    else if (psoMask)
    {
        pgp->Size.cx = psoMask->sizlBitmap.cx;
        pgp->Size.cy = psoMask->sizlBitmap.cy / 2;
    }

    IntHideMousePointer(ppdev, pso);

    if (pgp->psurfColor)
    {
        EngDeleteSurface(pgp->psurfColor->BaseObject.hHmgr);
        SURFACE_ShareUnlockSurface(pgp->psurfColor);
        pgp->psurfColor = NULL;
    }

    if (pgp->psurfMask)
    {
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

    /* See if we are being asked to hide the pointer. */
    if (psoMask == NULL && psoColor == NULL)
    {
        return SPS_ACCEPT_NOEXCLUDE;
    }

    pgp->HotSpot.x = xHot;
    pgp->HotSpot.y = yHot;

    /* Calculate lDelta for our surfaces. */
    lDelta = DIB_GetDIBWidthBytes(pgp->Size.cx, 
                                  BitsPerFormat(pso->iBitmapFormat));

    rcl.left = 0;
    rcl.top = 0;
    rcl.right = pgp->Size.cx;
    rcl.bottom = pgp->Size.cy;

    /* Create surface for saving the pixels under the cursor. */
    hbmp = EngCreateBitmap(pgp->Size,
                           lDelta,
                           pso->iBitmapFormat,
                           BMF_TOPDOWN | BMF_NOZEROINIT,
                           NULL);
    pgp->psurfSave = SURFACE_ShareLockSurface(hbmp);

    /* Create a mask surface */
    if (psoMask)
    {
        EXLATEOBJ exlo;
        PPALETTE ppal;

        hbmp = EngCreateBitmap(psoMask->sizlBitmap,
                               lDelta,
                               pso->iBitmapFormat,
                               BMF_TOPDOWN | BMF_NOZEROINIT,
                               NULL);
        pgp->psurfMask = SURFACE_ShareLockSurface(hbmp);

        if(pgp->psurfMask)
        {
            ppal = PALETTE_LockPalette(ppdev->devinfo.hpalDefault);
            EXLATEOBJ_vInitialize(&exlo,
                                  &gpalMono,
                                  ppal,
                                  0,
                                  RGB(0xff,0xff,0xff),
                                  RGB(0,0,0));

            rcl.bottom = psoMask->sizlBitmap.cy;
            IntEngCopyBits(&pgp->psurfMask->SurfObj,
                           psoMask,
                           NULL,
                           &exlo.xlo,
                           &rcl,
                           (POINTL*)&rcl);

            EXLATEOBJ_vCleanup(&exlo);
            if (ppal)
                PALETTE_UnlockPalette(ppal);
        }
    }
    else
    {
        pgp->psurfMask = NULL;
    }

    /* Create a color surface */
    if (psoColor)
    {
        hbmp = EngCreateBitmap(psoColor->sizlBitmap,
                               lDelta,
                               pso->iBitmapFormat,
                               BMF_TOPDOWN | BMF_NOZEROINIT,
                               NULL);
        pgp->psurfColor = SURFACE_ShareLockSurface(hbmp);
        if (pgp->psurfColor)
        {
            rcl.bottom = psoColor->sizlBitmap.cy;
            IntEngCopyBits(&pgp->psurfColor->SurfObj,
                           psoColor,
                           NULL,
                           pxlo,
                           &rcl,
                           (POINTL*)&rcl);
        }
    }
    else
    {
        pgp->psurfColor = NULL;
    }

    if (x != -1)
    {
        ppdev->ptlPointer.x = x;
        ppdev->ptlPointer.y = y;

        IntShowMousePointer(ppdev, pso);

        if (prcl != NULL)
        {
            prcl->left = x - pgp->HotSpot.x;
            prcl->top = y - pgp->HotSpot.x;
            prcl->right = prcl->left + pgp->Size.cx;
            prcl->bottom = prcl->top + pgp->Size.cy;
        }
    }
    else if (prcl != NULL)
    {
        prcl->left = prcl->top = prcl->right = prcl->bottom = -1;
    }

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
            prcl->top = y - pgp->HotSpot.y;
            prcl->right = prcl->left + pgp->Size.cx;
            prcl->bottom = prcl->top + pgp->Size.cy;
        }
    } 
    else if (prcl != NULL)
    {
        prcl->left = prcl->top = prcl->right = prcl->bottom = -1;
    }
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

ULONG
NTAPI
GreSetPointerShape(
    HDC hdc,
    HBITMAP hbmMask,
    HBITMAP hbmColor,
    LONG xHot,
    LONG yHot,
    LONG x,
    LONG y)
{
    PDC pdc;
    PSURFACE psurf, psurfMask, psurfColor;
    EXLATEOBJ exlo;
    FLONG fl = 0;
    ULONG ulResult = 0;

    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        DPRINT1("Failed to lock the DC.\n");
        return 0;
    }

    psurf = pdc->dclevel.pSurface;
    if (!psurf)
    {
        DPRINT1("DC has no surface.\n");
        DC_UnlockDc(pdc);
        return 0;
    }

    /* Lock the mask bitmap */
    if (hbmMask)
        psurfMask = SURFACE_ShareLockSurface(hbmMask);
    else
        psurfMask = NULL;

    /* Check for color bitmap */
    if (hbmColor)
    {
        /* We have one, lock it */
        psurfColor = SURFACE_ShareLockSurface(hbmColor);
        
        if (psurfColor)
        {
            /* Create an XLATEOBJ, no mono support */
            EXLATEOBJ_vInitialize(&exlo, psurfColor->ppal, psurf->ppal, 0, 0, 0);
        }
    }
    else
        psurfColor = NULL;

    /* Call the driver or eng function */
    ulResult = IntEngSetPointerShape(&psurf->SurfObj,
                                     psurfMask ? &psurfMask->SurfObj : NULL,
                                     psurfColor ? &psurfColor->SurfObj : NULL,
                                     psurfColor ? &exlo.xlo : NULL,
                                     xHot,
                                     yHot,
                                     x,
                                     y,
                                     &pdc->ppdev->Pointer.Exclude,
                                     fl | SPS_CHANGE);

    /* Cleanup */
    if (psurfColor)
    {
        EXLATEOBJ_vCleanup(&exlo);
        SURFACE_ShareUnlockSurface(psurfColor);
    }

    if (psurfMask)
        SURFACE_ShareUnlockSurface(psurfMask);

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    /* Return result */
    return ulResult;
}

VOID
NTAPI
GreMovePointer(
    HDC hdc,
    LONG x,
    LONG y)
{
    PDC pdc;
    PRECTL prcl;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        DPRINT1("Failed to lock the DC.\n");
        return;
    }

    /* Store the cursor exclude position in the PDEV */
    prcl = &pdc->ppdev->Pointer.Exclude;

    /* Call Eng/Drv function */
    IntEngMovePointer(&pdc->dclevel.pSurface->SurfObj, x, y, prcl);

    /* Unlock the DC */
    DC_UnlockDc(pdc);
}


/* EOF */
