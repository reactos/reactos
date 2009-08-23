/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engpoint.c
 * PURPOSE:         Pointer Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

#define TAG_MOUSE       TAG('M', 'O', 'U', 'S') /* mouse */

/* PUBLIC FUNCTIONS **********************************************************/

POINTL        ptlPointer;

VOID
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
        return;
    }

    /* Calculate cursor coordinates */
    pt.x = ptlPointer.x - pgp->HotSpot.x;
    pt.y = ptlPointer.y - pgp->HotSpot.y;

    rclDest.left = max(pt.x, 0);
    rclDest.top = max(pt.y, 0);
    rclDest.right = min(pt.x + pgp->Size.cx, psoDest->sizlBitmap.cx);
    rclDest.bottom = min(pt.y + pgp->Size.cy, psoDest->sizlBitmap.cy);

    ptlSave.x = rclDest.left - pt.x;
    ptlSave.y = rclDest.top - pt.y;

    GrepBitBltEx(psoDest,
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
    pt.x = ptlPointer.x - pgp->HotSpot.x;
    pt.y = ptlPointer.y - pgp->HotSpot.y;

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
    GrepBitBltEx(&pgp->psurfSave->SurfObj,
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
        GrepBitBltEx(psoDest,
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
        GrepBitBltEx(psoDest,
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

        GrepBitBltEx(psoDest,
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
 * FUNCTION: Notify the mouse driver that drawing is about to begin in
 * a rectangle on a particular surface.
 */
INT FASTCALL
MouseSafetyOnDrawStart(SURFOBJ *pso,
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
    if (!ppdev) return FALSE;

    pgp = &ppdev->Pointer;

    if (pgp->Exclude.right == -1)
        return FALSE;

    ppdev->SafetyRemoveCount++;

    if (ppdev->SafetyRemoveLevel != 0)
        return FALSE;

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

    return TRUE;
}

/*
 * FUNCTION: Notify the mouse driver that drawing has finished on a surface.
 */
INT FASTCALL
MouseSafetyOnDrawEnd(SURFOBJ *pso)
{
    PDEVOBJ *ppdev;
    GDIPOINTER *pgp;

    ASSERT(pso != NULL);

    ppdev = (PDEVOBJ*)pso->hdev;

    if (!ppdev) return FALSE;

    pgp = &ppdev->Pointer;

    if (pgp->Exclude.right == -1)
        return FALSE;

    if (--ppdev->SafetyRemoveCount >= ppdev->SafetyRemoveLevel)
        return FALSE;

    ppdev->pfnMovePointer(pso, ptlPointer.x, ptlPointer.y, &pgp->Exclude);

    ppdev->SafetyRemoveLevel = 0;

    return TRUE;
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
        SURFACE_ShareUnlock(pgp->psurfColor);
        pgp->psurfColor = NULL;
    }

    if (pgp->psurfMask)
    {
        EngDeleteSurface(pgp->psurfMask->BaseObject.hHmgr);
        SURFACE_ShareUnlock(pgp->psurfMask);
        pgp->psurfMask = NULL;
    }

    if (pgp->psurfSave != NULL)
    {
        EngDeleteSurface(pgp->psurfSave->BaseObject.hHmgr);
        SURFACE_ShareUnlock(pgp->psurfSave);
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
    pgp->psurfSave = SURFACE_ShareLock(hbmp);

    /* Create a mask surface */
    if (psoMask)
    {
        XLATEOBJ *xlo;
        //PPALETTE ppal;

        hbmp = EngCreateBitmap(psoMask->sizlBitmap,
                               lDelta,
                               pso->iBitmapFormat,
                               BMF_TOPDOWN | BMF_NOZEROINIT,
                               NULL);
        pgp->psurfMask = SURFACE_ShareLock(hbmp);

        if(pgp->psurfMask)
        {
            //ppal = PALETTE_LockPalette(ppdev->DevInfo.hpalDefault);
            /*EXLATEOBJ_vInitialize(&exlo,
                                  &gpalMono,
                                  ppal,
                                  0,
                                  RGB(0xff,0xff,0xff),
                                  RGB(0,0,0));*/
            xlo = IntEngCreateSrcMonoXlate(ppdev->DevInfo.hpalDefault,
                                           RGB(0xff,0xff,0xff),
                                           RGB(0,0,0)
                                           );

            rcl.bottom = psoMask->sizlBitmap.cy;

            GreCopyBits(&pgp->psurfMask->SurfObj,
                           psoMask,
                           NULL,
                           xlo,
                           &rcl,
                           (POINTL*)&rcl);

            //EXLATEOBJ_vCleanup(&exlo);
            EngDeleteXlate(xlo);
            //if (ppal)
            //    PALETTE_UnlockPalette(ppal);
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
        pgp->psurfColor = SURFACE_ShareLock(hbmp);
        if (pgp->psurfColor)
        {
            rcl.bottom = psoColor->sizlBitmap.cy;
            GreCopyBits(&pgp->psurfColor->SurfObj,
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
        ptlPointer.x = x;
        ptlPointer.y = y;

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

    ptlPointer.x = x;
    ptlPointer.y = y;

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
    } else if (prcl != NULL)
        prcl->left = prcl->top = prcl->right = prcl->bottom = -1;
}

BOOL
APIENTRY
EngSetPointerTag(
  IN HDEV  hdev,
  IN SURFOBJ  *psoMask,
  IN SURFOBJ  *psoColor,
  IN XLATEOBJ  *pxlo,
  IN FLONG  fl)
{
    UNIMPLEMENTED;
	return FALSE;
}
