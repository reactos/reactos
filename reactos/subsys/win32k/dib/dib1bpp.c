#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

PFN_DIB_PutPixel DIB_1BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  unsigned char *vp;
  unsigned char mask;
  PBYTE addr = SurfObj->pvBits;

  addr += y * SurfObj->lDelta + (x >> 3);

  if(c == 1)
  {
    *addr = (*addr | mask1Bpp[mod(x, 8)]);
  }
    else
  {
    *addr = (*addr ^ mask1Bpp[mod(x, 8)]);
  }
}

PFN_DIB_GetPixel DIB_1BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE addr = SurfObj->pvBits + y * SurfObj->lDelta + (x >> 3);

  if(*addr & mask1Bpp[mod(x, 8)]) return (PFN_DIB_GetPixel)(1);
  return (PFN_DIB_GetPixel)(0);
}

PFN_DIB_HLine DIB_1BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  LONG  cx = x1;

  while(cx <= x2) {
    DIB_1BPP_PutPixel(SurfObj, cx, y, c);
  }
}

PFN_DIB_VLine DIB_1BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  while(y1++ <= y2) {
    DIB_1BPP_PutPixel(SurfObj, x, y1, c);
  }
}

BOOLEAN DIB_To_1BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
        LONG    Delta,     XLATEOBJ *ColorTranslation)
{
  LONG    i, j, sx, sy = SourcePoint->y;

  switch(SourceGDI->BitsPerPixel)
  {
    case 1:
      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0)
          {
            DIB_1BPP_PutPixel(DestSurf, i, j, 0);
          } else {
            DIB_1BPP_PutPixel(DestSurf, i, j, 1);
          }
          sx++;
        }
        sy++;
      }
      break;

    case 4:
      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy)) == 0)
          {
            DIB_1BPP_PutPixel(DestSurf, i, j, 0);
          } else {
            DIB_1BPP_PutPixel(DestSurf, i, j, 1);
          }
          sx++;
        }
        sy++;
      }
      break;

    case 24:
      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(XLATEOBJ_iXlate(ColorTranslation, DIB_24BPP_GetPixel(SourceSurf, sx, sy)) == 0)
          {
            DIB_1BPP_PutPixel(DestSurf, i, j, 0);
          } else {
            DIB_1BPP_PutPixel(DestSurf, i, j, 1);
          }
          sx++;
        }
        sy++;
      }
      break;

    default:
      DbgPrint("DIB_1BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}
