/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engblt.c
 * PURPOSE:         Bit-Block Transfer Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
EngAlphaBlend(IN SURFOBJ* DestSurf,
              IN SURFOBJ* SourceSurf,
              IN CLIPOBJ* ClipRegion,
              IN XLATEOBJ* ColorTranslation,
              IN PRECTL DestRect,
              IN PRECTL SourceRect,
              IN BLENDOBJ* BlendObj)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
EngBitBlt(SURFOBJ* DestSurf,
          SURFOBJ* SourceSurf,
          SURFOBJ* Mask,
          CLIPOBJ* ClipRegion,
          XLATEOBJ* ColorTranslation,
          PRECTL DestRect,
          PPOINTL SourcePoint,
          PPOINTL MaskOrigin,
          BRUSHOBJ* BrushObj,
          PPOINTL BrushOrigin,
          ROP4 Rop4)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
EngStretchBlt(IN SURFOBJ* DestSurf,
              IN SURFOBJ* SourceSurf,
              IN SURFOBJ* Mask,
              IN CLIPOBJ* ClipRegion,
              IN XLATEOBJ* ColorTranslation,
              IN COLORADJUSTMENT* ColorAdjustment,
              IN PPOINTL BrushOrigin,
              IN PRECTL prclDest,
              IN PRECTL prclSource,
              IN PPOINTL MaskOrigin,
              IN ULONG Mode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
EngPlgBlt(
   IN SURFOBJ *Dest,
   IN SURFOBJ *Source,
   IN SURFOBJ *Mask,
   IN CLIPOBJ *Clip,
   IN XLATEOBJ *Xlate,
   IN COLORADJUSTMENT *ColorAdjustment,
   IN POINTL *BrusOrigin,
   IN POINTFIX *DestPoints,
   IN RECTL *SourceRect,
   IN POINTL *MaskPoint,
   IN ULONG Mode)
{
   UNIMPLEMENTED;
   return FALSE;
}

BOOL
APIENTRY
EngStretchBltROP(IN SURFOBJ* DestSurf,
                 IN SURFOBJ* SourceSurf,
                 IN SURFOBJ* Mask,
                 IN CLIPOBJ* ClipRegion,
                 IN XLATEOBJ* ColorTranslation,
                 IN COLORADJUSTMENT* ColorAdjustment,
                 IN PPOINTL BrushOrigin,
                 IN PRECTL DestRect,
                 IN PRECTL SourceRect,
                 IN PPOINTL MaskPoint,
                 IN ULONG Mode,
                 IN BRUSHOBJ* BrushObj,
                 IN DWORD Rop4)
{
   UNIMPLEMENTED;
   return FALSE;
}

BOOL
APIENTRY
EngTransparentBlt(IN SURFOBJ* DestSurf,
                  IN SURFOBJ* SourceSurf,
                  IN CLIPOBJ* ClipRegion,
                  IN XLATEOBJ* ColorTranslation,
                  IN PRECTL prclDest,
                  IN PRECTL prclSource,
                  IN ULONG iTransColor,
                  IN ULONG ulReserved)
{
    UNIMPLEMENTED;
    return FALSE;
}
