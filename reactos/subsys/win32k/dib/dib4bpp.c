#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

PFN_DIB_PutPixel DIB_4BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  unsigned char *vp;
  unsigned char mask;
  PBYTE addr = SurfObj->pvBits;

  addr += (x>>1) + y * SurfObj->lDelta;
  *addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
}

PFN_DIB_GetPixel DIB_4BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE addr = SurfObj->pvBits;

  return (PFN_DIB_GetPixel)((addr[(x>>1) + y * SurfObj->lDelta] >> ((1-(x&1))<<2) ) & 0x0f);
}

PFN_DIB_HLine DIB_4BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE  addr = SurfObj->pvBits + (x1>>1) + y * SurfObj->lDelta;
  LONG  cx = x1;

  while(cx < x2) {
    *addr = (*addr & notmask[x1&1]) | (c << ((1-(x1&1))<<2));
    if((++x1 & 1) == 0)
      ++addr;
    ++cx;
  }
}

PFN_DIB_VLine DIB_4BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE  addr = SurfObj->pvBits;
  int  lDelta = SurfObj->lDelta;

  addr += (x>>1) + y1 * lDelta;
  while(y1++ < y2) {
    *addr = (*addr & notmask[x&1]) | (c << ((1-(x&1))<<2));
    addr += lDelta;
  }
}

BOOLEAN DIB_To_4BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
        LONG   Delta,      XLATEOBJ *ColorTranslation)
{
  LONG     i, j, sx, sy, f1, f2, xColor;
  PBYTE    SourceBits_24BPP, SourceLine_24BPP;
  PBYTE    DestBits, DestLine, SourceBits_4BPP, SourceBits_8BPP, SourceLine_4BPP, SourceLine_8BPP;
  PWORD    SourceBits_16BPP, SourceLine_16BPP;
  PDWORD    SourceBits_32BPP, SourceLine_32BPP;

  DestBits = DestSurf->pvBits + (DestRect->left>>1) + DestRect->top * DestSurf->lDelta;

  switch(SourceGDI->BitsPerPixel)
  {
    case 1:
      sx = SourcePoint->x;
      sy = SourcePoint->y;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0)
          {
            DIB_4BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_4BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
          }
          sx++;
        }
        sy++;
      }
      break;

    case 4:
      sy = SourcePoint->y;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          DIB_4BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, DIB_4BPP_GetPixel(SourceSurf, sx, sy)));
          sx++;
        }
        sy++;
      }
      break;

    case 8:
      SourceBits_8BPP = SourceSurf->pvBits + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_8BPP = SourceBits_8BPP;
        DestLine = DestBits;
        sx = SourcePoint->x;
        f1 = sx & 1;
        f2 = DestRect->left & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          *DestLine = (*DestLine & notmask[i&1]) |
            ((XLATEOBJ_iXlate(ColorTranslation, *SourceLine_8BPP)) << ((4 * (1-(sx & 1)))));
          if(f2 == 1) { DestLine++; f2 = 0; } else { f2 = 1; }
          SourceLine_8BPP++;
          sx++;
        }

        SourceBits_8BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    case 24:
      SourceBits_24BPP = SourceSurf->pvBits + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x * 3;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_24BPP = SourceBits_24BPP;
        DestLine = DestBits;
        sx = SourcePoint->x;
        f1 = sx & 1;
        f2 = DestRect->left & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = (*(SourceLine_24BPP + 2) << 0x10) +
             (*(SourceLine_24BPP + 1) << 0x08) +
             (*(SourceLine_24BPP));
          *DestLine = (*DestLine & notmask[i&1]) |
            ((XLATEOBJ_iXlate(ColorTranslation, xColor)) << ((4 * (1-(sx & 1)))));
          if(f2 == 1) { DestLine++; f2 = 0; } else { f2 = 1; }
          SourceLine_24BPP+=3;
          sx++;
        }

        SourceBits_24BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_4BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}
