/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: dib1bpp.c,v 1.6 2003/05/18 17:16:17 ea Exp $ */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

VOID
DIB_1BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE addr = SurfObj->pvScan0;

  addr += y * SurfObj->lDelta + (x >> 3);

  if(c == 0)
  {
    *addr = (*addr ^ mask1Bpp[x % 8]);
  }
    else
  {
    *addr = (*addr | mask1Bpp[x % 8]);
  }
}

ULONG
DIB_1BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE addr = SurfObj->pvScan0 + y * SurfObj->lDelta + (x >> 3);

  return (*addr & mask1Bpp[x % 8] ? 1 : 0);
}

VOID
DIB_1BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  while(x1 < x2) {
    DIB_1BPP_PutPixel(SurfObj, x1, y, c);
    x1++;
  }
}

VOID
DIB_1BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  while(y1 < y2) {
    DIB_1BPP_PutPixel(SurfObj, x, y1, c);
    y1++;
  }
}

BOOLEAN
DIB_1BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
                        PRECTL  DestRect,  POINTL  *SourcePoint,
                        XLATEOBJ *ColorTranslation)
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

    case 8:
      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(XLATEOBJ_iXlate(ColorTranslation, DIB_8BPP_GetPixel(SourceSurf, sx, sy)) == 0)
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

    case 16:
      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(XLATEOBJ_iXlate(ColorTranslation, DIB_16BPP_GetPixel(SourceSurf, sx, sy)) == 0)
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

    case 32:
      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        sx = SourcePoint->x;
        for (i=DestRect->left; i<DestRect->right; i++)
        {
          if(XLATEOBJ_iXlate(ColorTranslation, DIB_32BPP_GetPixel(SourceSurf, sx, sy)) == 0)
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
/* EOF */
