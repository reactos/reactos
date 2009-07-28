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
    SURFACE_LockBitmapBits(pso);
    GDIDEVFUNCS(pso).MovePointer(pso, x, y, prcl);
    SURFACE_UnlockBitmapBits(pso);
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
    PFN_DrvSetPointerShape pfnSetPointerShape;
    PPDEVOBJ ppdev = &PrimarySurface;

    pfnSetPointerShape = GDIDEVFUNCS(pso).SetPointerShape;

    SURFACE_LockBitmapBits(pso);
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

    SURFACE_UnlockBitmapBits(pso);

    return ulResult;
}



BOOL
NTAPI
GreSetCursor(ICONINFO* NewCursor, PSYSTEM_CURSORINFO CursorInfo)
{
   SURFOBJ *pso;
   SURFOBJ *MaskBmpObj = NULL;
   HBITMAP hMask = 0;
   SURFOBJ *soMask = NULL, *soColor = NULL;
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

   MaskBmpObj = EngLockSurface((HSURF)NewCursor->hbmMask);
   if (MaskBmpObj)
   {
      const int maskBpp = BitsPerFormat(MaskBmpObj->iBitmapFormat);
      EngUnlockSurface(MaskBmpObj);
      if (maskBpp != 1)
      {
         DPRINT1("SetCursor: The Mask bitmap must have 1BPP!\n");
         EngUnlockSurface(pso);
         return FALSE;
      }

      if ((PrimarySurface.DevInfo.flGraphicsCaps2 & GCAPS2_ALPHACURSOR) &&
            pso->iBitmapFormat >= BMF_16BPP &&
            pso->iBitmapFormat <= BMF_32BPP)
      {
         /* FIXME - Create a color pointer, only 32bit bitmap, set alpha bits!
                    Do not pass a mask bitmap to DrvSetPointerShape()!
                    Create a XLATEOBJ that describes the colors of the bitmap. */
         DPRINT1("SetCursor: (Colored) alpha cursors are not supported!\n");
      }
      else
      {
         if(NewCursor->hbmColor)
         {
            /* FIXME - Create a color pointer, create only one 32bit bitmap!
                       Do not pass a mask bitmap to DrvSetPointerShape()!
                       Create a XLATEOBJ that describes the colors of the bitmap.
                       (16bit bitmaps are propably allowed) */
            DPRINT1("SetCursor: Cursors with colors are not supported!\n");
         }
         else
         {
            MaskBmpObj = EngLockSurface((HSURF)NewCursor->hbmMask);
            if(MaskBmpObj)
            {
               RECTL DestRect = {0, 0, MaskBmpObj->sizlBitmap.cx, MaskBmpObj->sizlBitmap.cy};
               POINTL SourcePoint = {0, 0};

               /*
                * NOTE: For now we create the cursor in top-down bitmap,
                * because VMware driver rejects it otherwise. This should
                * be fixed later.
                */
               hMask = EngCreateBitmap(
                          MaskBmpObj->sizlBitmap, abs(MaskBmpObj->lDelta),
                          MaskBmpObj->iBitmapFormat, BMF_TOPDOWN,
                          NULL);
               if ( !hMask )
               {
                  EngUnlockSurface(MaskBmpObj);
                  EngUnlockSurface(pso);
                  return FALSE;
               }
               soMask = EngLockSurface((HSURF)hMask);
               EngCopyBits(soMask, MaskBmpObj, NULL, NULL,
                           &DestRect, &SourcePoint);
               EngUnlockSurface(MaskBmpObj);
            }
         }
      }
      CursorInfo->ShowingCursor = TRUE;
      CursorInfo->CurrentCursorObject = *NewCursor;
   }
   else
   {
      CursorInfo->ShowingCursor = FALSE;
   }

   Status  = GreSetPointerShape(pso,
                                   soMask,
                                   soColor,
                                   XlateObj,
                                   NewCursor->xHotspot,
                                   NewCursor->yHotspot,
                                   CursorInfo->CursorPos.x,
                                   CursorInfo->CursorPos.y,
                                   NULL,
                                   SPS_CHANGE);

   if (Status != SPS_ACCEPT_NOEXCLUDE)
   {
       DPRINT1("GreSetPointerShape returned %lx\n", Status);
   }

   if(hMask)
   {
      EngUnlockSurface(soMask);
      EngDeleteSurface((HSURF)hMask);
   }
   if(XlateObj)
   {
      EngDeleteXlate(XlateObj);
   }

   EngUnlockSurface(pso);

   return TRUE;
}

