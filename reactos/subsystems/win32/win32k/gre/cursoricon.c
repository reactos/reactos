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
GreMovePointer(
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

ULONG NTAPI
GreSetPointerShape(
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



BOOL
NTAPI
GreSetCursor(ICONINFO* NewCursor, PSYSTEM_CURSORINFO CursorInfo)
{
    SURFOBJ *pso;
    HBITMAP hbmMask, hbmColor;
    PSURFACE psurfMask, psurfColor;
    XLATEOBJ *XlateObj = NULL;
    ULONG Status;

    pso = EngLockSurface(PrimarySurface.pSurface);

    if (!NewCursor)
    {
        if (CursorInfo->ShowingCursor)
        {
            DPRINT("Removing pointer!\n");
            /* Remove the cursor if it was displayed */
            GreMovePointer(pso, -1, -1, NULL);
        }
        CursorInfo->ShowingCursor = 0;
        EngUnlockSurface(pso);
        return TRUE;
    }

    CursorInfo->ShowingCursor = TRUE;
    CursorInfo->CurrentCursorObject = *NewCursor;

    hbmMask = NewCursor->hbmMask;
    hbmColor = NewCursor->hbmColor;

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

   Status  = GreSetPointerShape(pso,
                                psurfMask ? &psurfMask->SurfObj : NULL,
                                psurfColor ? &psurfColor->SurfObj : NULL,
                                XlateObj,
                                NewCursor->xHotspot,
                                NewCursor->yHotspot,
                                CursorInfo->CursorPos.x,
                                CursorInfo->CursorPos.y,
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

