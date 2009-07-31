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
#define GDIDEV(SurfObj) ((PDEVOBJ *)((SurfObj)->hdev))

/* PUBLIC FUNCTIONS **********************************************************/

GDIPOINTER    Pointer;
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

    pgp = &Pointer;

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
                   pgp->psurfSave,
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


    pgp = &Pointer;

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
    GrepBitBltEx(pgp->psurfSave,
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
                       pgp->psurfColor,
                       pgp->psurfMask,
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
                       pgp->psurfMask,
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
                       pgp->psurfMask,
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
    pgp = &Pointer;

    IntHideMousePointer(ppdev, pso);

    if (pgp->psurfColor)
    {
        /* FIXME: let GDI allocate/free memory */
        EngFreeMem(pgp->psurfColor->pvBits);
        pgp->psurfColor->pvBits = 0;

        EngDeleteSurface(pgp->psurfColor->hsurf);
        //SURFACE_ShareUnlockSurface(pgp->psurfColor);
        pgp->psurfColor = NULL;
    }

    if (pgp->psurfMask)
    {
        /* FIXME: let GDI allocate/free memory */
        EngFreeMem(pgp->psurfMask->pvBits);
        pgp->psurfMask->pvBits = 0;

        EngDeleteSurface(pgp->psurfMask->hsurf);
        //SURFACE_ShareUnlockSurface(pgp->psurfMask);
        pgp->psurfMask = NULL;
    }

    if (pgp->psurfSave != NULL)
    {
        EngDeleteSurface(pgp->psurfSave->hsurf);
        //SURFACE_ShareUnlockSurface(pgp->psurfSave);
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
        ptlPointer.x = x;
        ptlPointer.y = y;
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

        pgp->psurfColor = EngLockSurface(hbmp);
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

    pgp->psurfMask = EngLockSurface(hbmp);

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

    pgp->psurfSave = EngLockSurface(hbmp);

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

    pgp = &Pointer;

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
