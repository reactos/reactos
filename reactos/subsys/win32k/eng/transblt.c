/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI TransparentBlt Function
 * FILE:             subsys/win32k/eng/transblt.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        4/6/2001: Created
 */

#include <ddk/winddi.h>
#include <ddk/ntddk.h>
#include <ntos/minmax.h>
#include "brush.h"
#include "enum.h"
#include "objects.h"

BOOL EngTransparentBlt(PSURFOBJ Dest, PSURFOBJ Source,
                       PCLIPOBJ Clip, PXLATEOBJ ColorTranslation,
                       PRECTL DestRect, PRECTL SourceRect,
                       ULONG TransparentColor, ULONG Reserved)
{
  PSURFGDI DestGDI   = (PSURFGDI)AccessInternalObjectFromUserObject(Dest),
           SourceGDI = (PSURFGDI)AccessInternalObjectFromUserObject(Source);
  HSURF     hTemp;
  PSURFOBJ  TempSurf;
  POINTL    TempPoint, SourcePoint;
  RECTL     TempRect;
  SIZEL     TempSize;
  BOOLEAN   ret;
  LONG dx, dy, sx, sy;

  dx = abs(DestRect->right  - DestRect->left);
  dy = abs(DestRect->bottom - DestRect->top);

  sx = abs(SourceRect->right  - SourceRect->left);
  sy = abs(SourceRect->bottom - SourceRect->top);

  if(sx<dx) dx = sx;
  if(sy<dy) dy = sy;

  MouseSafetyOnDrawStart(Source, SourceGDI, SourceRect->left, SourceRect->top, SourceRect->right, SourceRect->bottom);
  MouseSafetyOnDrawStart(Dest, DestGDI, DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);

  if(DestGDI->TransparentBlt != NULL)
  {
    // The destination is device managed, therefore get the source into a format compatible surface
    TempPoint.x = 0;
    TempPoint.y = 0;
    TempRect.top    = 0;
    TempRect.left   = 0;
    TempRect.bottom = dy;
    TempRect.right  = dx;
    TempSize.cx = TempRect.right;
    TempSize.cy = TempRect.bottom;

    hTemp = EngCreateBitmap(TempSize,
                 DIB_GetDIBWidthBytes(dx, BitsPerFormat(Dest->iBitmapFormat)), Dest->iBitmapFormat, 0, NULL);
    TempSurf = (PSURFOBJ)AccessUserObject(hTemp);

    SourcePoint.x = SourceRect->left;
    SourcePoint.y = SourceRect->top;

    // FIXME: Skip creating a TempSurf if we have the same BPP and palette
    EngBitBlt(TempSurf, Source, NULL, NULL, ColorTranslation, &TempRect, &SourcePoint, NULL, NULL, NULL, 0);

    ret = DestGDI->TransparentBlt(Dest, TempSurf, Clip, NULL, DestRect, SourceRect,
                                  TransparentColor, Reserved);

    MouseSafetyOnDrawEnd(Source, SourceGDI);
    MouseSafetyOnDrawEnd(Dest, DestGDI);

    return ret;
  }

  // Simulate a transparent blt

  MouseSafetyOnDrawEnd(Source, SourceGDI);
  MouseSafetyOnDrawEnd(Dest, DestGDI);

  return TRUE;
}
