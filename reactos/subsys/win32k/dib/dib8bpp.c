#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

VOID DIB_8BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  *byteaddr = c;
}

ULONG DIB_8BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta + x;

  return (ULONG)(*byteaddr);
}

VOID DIB_8BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y * SurfObj->lDelta;
  PBYTE addr = byteaddr + x1;
  LONG cx = x1;

  while(cx < x2) {
    *addr = c;
    ++addr;
    ++cx;
  }
}

VOID DIB_8BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE byteaddr = SurfObj->pvScan0 + y1 * SurfObj->lDelta;
  PBYTE addr = byteaddr + x;
  LONG lDelta = SurfObj->lDelta;

  byteaddr = addr;
  while(y1++ < y2) {
    *addr = c;

    addr += lDelta;
  }
}

BOOLEAN DIB_8BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
                        PRECTL  DestRect,  POINTL  *SourcePoint,
                        XLATEOBJ *ColorTranslation)
{
  ULONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;

  DestBits = DestSurf->pvScan0 + (DestRect->top * DestSurf->lDelta) + DestRect->left;

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
            DIB_8BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 0));
          } else {
            DIB_8BPP_PutPixel(DestSurf, i, j, XLATEOBJ_iXlate(ColorTranslation, 1));
          }
          sx++;
        }
        sy++;
      }
      break;

    case 4:
      SourceBits_4BPP = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + (SourcePoint->x >> 1);

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_4BPP = SourceBits_4BPP;
        sx = SourcePoint->x;
        f1 = sx & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation,
              (*SourceLine_4BPP & altnotmask[sx&1]) >> (4 * (1-(sx & 1))));
          DIB_8BPP_PutPixel(DestSurf, i, j, xColor);
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += SourceSurf->lDelta;
      }
      break;

    case 8:
      if (NULL == ColorTranslation || 0 != (ColorTranslation->flXlate & XO_TRIVIAL))
      {
	SourceBits = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;
	for (j = DestRect->top; j < DestRect->bottom; j++)
	{
	  RtlCopyMemory(DestBits, SourceBits, DestRect->right - DestRect->left);
	  SourceBits += SourceSurf->lDelta;
	  DestBits += DestSurf->lDelta;
	}
      }
      else
      {
	/* FIXME */
	DPRINT1("DIB_8BPP_Bitblt: Unhandled ColorTranslation for 32 -> 32 copy");
        return FALSE;
      }
      break;

    case 16:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 2 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PWORD) SourceBits);
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 2;
	  DestBits += 1;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case 24:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 3 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = (*(SourceBits + 2) << 0x10) +
             (*(SourceBits + 1) << 0x08) +
             (*(SourceBits));
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 3;
	  DestBits += 1;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    case 32:
      SourceLine = SourceSurf->pvScan0 + (SourcePoint->y * SourceSurf->lDelta) + 4 * SourcePoint->x;
      DestLine = DestBits;

      for (j = DestRect->top; j < DestRect->bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = DestRect->left; i < DestRect->right; i++)
        {
          xColor = *((PDWORD) SourceBits);
          *DestBits = XLATEOBJ_iXlate(ColorTranslation, xColor);
          SourceBits += 4;
	  DestBits += 1;
        }

        SourceLine += SourceSurf->lDelta;
        DestLine += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_8BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}
