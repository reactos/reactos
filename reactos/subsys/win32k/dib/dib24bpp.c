#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

PFN_DIB_PutPixel DIB_24BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvBits + y * SurfObj->lDelta;
  RGBTRIPLE *addr = (RGBTRIPLE*)byteaddr + x;

  *(PULONG)(addr) = c;
}

PFN_DIB_GetPixel DIB_24BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = SurfObj->pvBits + y * SurfObj->lDelta;
  RGBTRIPLE *addr = (RGBTRIPLE*)byteaddr + x;

  return (PFN_DIB_GetPixel)(*(PULONG)(addr));
}

PFN_DIB_HLine DIB_24BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvBits + y * SurfObj->lDelta;
  RGBTRIPLE *addr = (RGBTRIPLE*)byteaddr + x1;
  LONG cx = x1;

  while(cx <= x2) {
    *(PULONG)(addr) = c;
    ++addr;
    ++cx;
  }
}

PFN_DIB_VLine DIB_24BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvBits + y1 * SurfObj->lDelta;
  RGBTRIPLE *addr = (RGBTRIPLE*)byteaddr + x;
  ULONG  lDelta = SurfObj->lDelta;

  byteaddr = (PBYTE)addr;
  while(y1++ <= y2) {
    *(PULONG)(addr) = c;

    byteaddr += lDelta;
    addr = (RGBTRIPLE*)byteaddr;
  }
}

VOID DIB_24BPP_BltTo_24BPP(SURFOBJ* dstpsd, LONG dstx, LONG dsty, LONG w, LONG h,
  SURFOBJ* srcpsd, LONG srcx, LONG srcy)
{
  RGBTRIPLE  *dst;
  RGBTRIPLE  *src;
  PBYTE  bytedst;
  PBYTE  bytesrc;
  int  i;
  int  dlDelta = dstpsd->lDelta;
  int  slDelta = srcpsd->lDelta;

  bytedst = (char *)dstpsd->pvBits + dsty * dlDelta;
  bytesrc = (char *)srcpsd->pvBits + srcy * slDelta;
  dst = (RGBTRIPLE*)bytedst + dstx;
  src = (RGBTRIPLE*)bytesrc + srcx;

  while(--h >= 0) {
    RGBTRIPLE  *d = dst;
    RGBTRIPLE  *s = src;
    LONG  dx = dstx;
    LONG  sx = srcx;
    for(i=0; i<w; ++i) {
      *d = *s;
      ++d;
      ++s;
    }
    dst += dlDelta;
    src += slDelta;
  }
}

BOOLEAN DIB_To_24BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
        ULONG   Delta,     XLATEOBJ *ColorTranslation)
{
  ULONG    i, j, sx, xColor, f1;
  PBYTE    DestBits, SourceBits_24BPP, DestLine, SourceLine_24BPP;
  RGBTRIPLE  *SPDestBits, *SPSourceBits_24BPP, *SPDestLine, *SPSourceLine_24BPP; // specially for 24-to-24 blit
  PBYTE    SourceBits_4BPP, SourceBits_8BPP, SourceLine_4BPP, SourceLine_8BPP;
  PWORD    SourceBits_16BPP, SourceLine_16BPP;
  PDWORD    SourceBits_32BPP, SourceLine_32BPP;

  DestBits = DestSurf->pvBits + (DestRect->top * DestSurf->lDelta) + DestRect->left * 3;

  switch(SourceGDI->BitsPerPixel)
  {
    case 4:
      SourceBits_4BPP = SourceSurf->pvBits + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_4BPP = SourceBits_4BPP;
        DestLine = DestBits;
        sx = SourcePoint->x;
        f1 = sx & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation,
              (*SourceLine_4BPP & altnotmask[sx&1]) >> (4 * (1-(sx & 1))));
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = xColor >> 8;
          DestLine += 2;
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_24BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}
