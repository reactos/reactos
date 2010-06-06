/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/cursoricon.c
 * PURPOSE:         ReactOS cursor support routines
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <win32k.h>
#define NDEBUG
#include <debug.h>

extern PDEVOBJ PrimarySurface;

VOID NTAPI
IntEngMovePointer(
    IN SURFOBJ *pso,
    IN LONG x,
    IN LONG y,
    IN RECTL *prcl)
{
    SURFACE *pSurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
    PPDEVOBJ ppdev = GDIDEV(pso);

    SURFACE_LockBitmapBits(pSurf);
    ppdev->pfnMovePointer(pso, x, y, prcl);
    SURFACE_UnlockBitmapBits(pSurf);
}

VOID
NTAPI
GreMovePointer(
    HDC hdc,
    LONG x,
    LONG y)
{
    PRECTL prcl;
    SURFOBJ *pso;

    pso = EngLockSurface(PrimarySurface.pSurface);

    /* Store the cursor exclude position in the PDEV */
    prcl = &(GDIDEV(pso)->Pointer.Exclude);

    /* Call Eng/Drv function */
    IntEngMovePointer(pso, x, y, prcl);

    EngUnlockSurface(pso);
}


ULONG NTAPI
IntSetPointerShape(
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
    SURFACE *pSurf = CONTAINING_RECORD(pso, SURFACE, SurfObj);
    PFN_DrvSetPointerShape pfnSetPointerShape;
    PPDEVOBJ ppdev = GDIDEV(pso);

    pfnSetPointerShape = GDIDEVFUNCS(pso).SetPointerShape;

    SURFACE_LockBitmapBits(pSurf);
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

    SURFACE_UnlockBitmapBits(pSurf);

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
    SURFOBJ *pso;
    PSURFACE psurfMask, psurfColor;
    XLATEOBJ *XlateObj = NULL;
    ULONG Status;

    pso = EngLockSurface(PrimarySurface.pSurface);

    /* Lock the mask bitmap */
    if (hbmMask)
        psurfMask = SURFACE_ShareLock(hbmMask);
    else
        psurfMask = NULL;

    /* Check for color bitmap */
    if (hbmColor)
    {
        /* We have one, lock it */
        psurfColor = SURFACE_ShareLock(hbmColor);

        if (psurfColor)
        {
            /* Create an XLATEOBJ, no mono support */
            //EXLATEOBJ_vInitialize(&exlo, psurfColor->ppal, psurf->ppal, 0, 0, 0);
            UNIMPLEMENTED;
        }
    }
    else
        psurfColor = NULL;

   Status  = IntSetPointerShape(pso,
                                psurfMask ? &psurfMask->SurfObj : NULL,
                                psurfColor ? &psurfColor->SurfObj : NULL,
                                XlateObj,
                                xHot,
                                yHot,
                                x,
                                y,
                                &(GDIDEV(pso)->Pointer.Exclude),
                                SPS_CHANGE);

   if (Status != SPS_ACCEPT_NOEXCLUDE)
   {
       DPRINT1("GreSetPointerShape returned %lx\n", Status);
   }

    /* Cleanup */
    if (psurfColor)
    {
        //EXLATEOBJ_vCleanup(&exlo);
        SURFACE_ShareUnlock(psurfColor);
    }

    if (psurfMask)
        SURFACE_ShareUnlock(psurfMask);

    EngUnlockSurface(pso);

    return TRUE;
}

