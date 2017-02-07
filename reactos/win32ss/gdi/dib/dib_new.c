/*
 * PROJECT:         Win32 subsystem
 * LICENSE:         See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/dib/dib.c
 * PURPOSE:         ROP handling, function pointer arrays, misc
 * PROGRAMMERS:     Ge van Geldorp
 */


#include <win32k.h>
#include "..\diblib\DibLib_interface.h"

/* Static data */

unsigned char notmask[2] = { 0x0f, 0xf0 };
unsigned char altnotmask[2] = { 0xf0, 0x0f };

ULONG
DIB_DoRop(ULONG Rop, ULONG Dest, ULONG Source, ULONG Pattern)
{
    return gapfnRop[Rop & 0xFF](Dest, Source, Pattern);
}

DIB_FUNCTIONS DibFunctionsForBitmapFormat[] =
{
  /* 0 */
  {
    Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
    0, 0, Dummy_StretchBlt, Dummy_TransparentBlt,
    0, Dummy_AlphaBlend
  },
  /* BMF_1BPP */
  {
    DIB_1BPP_PutPixel, DIB_1BPP_GetPixel, DIB_1BPP_HLine, DIB_1BPP_VLine,
    0, 0, DIB_XXBPP_StretchBlt,
    DIB_1BPP_TransparentBlt, 0, DIB_XXBPP_AlphaBlend
  },
  /* BMF_4BPP */
  {
    DIB_4BPP_PutPixel, DIB_4BPP_GetPixel, DIB_4BPP_HLine, DIB_4BPP_VLine,
    0, 0, DIB_XXBPP_StretchBlt,
    DIB_4BPP_TransparentBlt, 0, DIB_XXBPP_AlphaBlend
  },
  /* BMF_8BPP */
  {
    DIB_8BPP_PutPixel, DIB_8BPP_GetPixel, DIB_8BPP_HLine, DIB_8BPP_VLine,
    0, 0, DIB_XXBPP_StretchBlt,
    DIB_8BPP_TransparentBlt, 0, DIB_XXBPP_AlphaBlend
  },
  /* BMF_16BPP */
  {
    DIB_16BPP_PutPixel, DIB_16BPP_GetPixel, DIB_16BPP_HLine, DIB_16BPP_VLine,
    0, 0, DIB_XXBPP_StretchBlt,
    DIB_16BPP_TransparentBlt, 0, DIB_XXBPP_AlphaBlend
  },
  /* BMF_24BPP */
  {
    DIB_24BPP_PutPixel, DIB_24BPP_GetPixel, DIB_24BPP_HLine, DIB_24BPP_VLine,
    0, 0, DIB_XXBPP_StretchBlt,
    DIB_24BPP_TransparentBlt, 0, DIB_24BPP_AlphaBlend
  },
  /* BMF_32BPP */
  {
    DIB_32BPP_PutPixel, DIB_32BPP_GetPixel, DIB_32BPP_HLine, DIB_32BPP_VLine,
    0, 0, DIB_XXBPP_StretchBlt,
    DIB_32BPP_TransparentBlt, 0, DIB_32BPP_AlphaBlend
  },
  /* BMF_4RLE */
  {
    Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
    0, 0, Dummy_StretchBlt, Dummy_TransparentBlt,
    0, Dummy_AlphaBlend
  },
  /* BMF_8RLE */
  {
    Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
    0, 0, Dummy_StretchBlt, Dummy_TransparentBlt,
    0, Dummy_AlphaBlend
  },
  /* BMF_JPEG */
  {
    Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
    0, 0, Dummy_StretchBlt, Dummy_TransparentBlt,
    0, Dummy_AlphaBlend
  },
  /* BMF_PNG */
  {
    Dummy_PutPixel, Dummy_GetPixel, Dummy_HLine, Dummy_VLine,
    0, 0, Dummy_StretchBlt, Dummy_TransparentBlt,
    0, Dummy_AlphaBlend
  }
};


VOID Dummy_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c)
{
  return;
}

ULONG Dummy_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y)
{
  return 0;
}

VOID Dummy_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  return;
}

VOID Dummy_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  return;
}

BOOLEAN Dummy_BitBlt(PBLTINFO BltInfo)
{
  return FALSE;
}

BOOLEAN Dummy_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         SURFOBJ *PatternSurface, SURFOBJ *MaskSurf,
                         RECTL*  DestRect,  RECTL  *SourceRect,
                         POINTL* MaskOrigin, BRUSHOBJ* Brush,
                         POINTL* BrushOrign,
                         XLATEOBJ *ColorTranslation,
                         ROP4 Rop)
{
  return FALSE;
}

BOOLEAN Dummy_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                             RECTL*  DestRect,  RECTL  *SourceRect,
                             XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  return FALSE;
}

BOOLEAN Dummy_ColorFill(SURFOBJ* Dest, RECTL* DestRect, ULONG Color)
{
  return FALSE;
}


BOOLEAN
Dummy_AlphaBlend(SURFOBJ* Dest, SURFOBJ* Source, RECTL* DestRect,
                 RECTL* SourceRect, CLIPOBJ* ClipRegion,
                 XLATEOBJ* ColorTranslation, BLENDOBJ* BlendObj)
{
  return FALSE;
}

/* EOF */
