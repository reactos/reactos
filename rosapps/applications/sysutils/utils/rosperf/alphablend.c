/*
 *  ReactOS RosPerf - ReactOS GUI performance test program
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

#include <windows.h>
#include <wingdi.h>
#include "rosperf.h"

typedef struct _ALPHABLEND_CONTEXT {
  HDC BitmapDc;
  HBITMAP Bitmap;
} ALPHABLEND_CONTEXT, *PALPHABLEND_CONTEXT;

unsigned
AlphaBlendInit(void **Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  PALPHABLEND_CONTEXT ctx = HeapAlloc(GetProcessHeap(), 0, sizeof (ALPHABLEND_CONTEXT));
  INT x, y;

  ctx->BitmapDc = CreateCompatibleDC(PerfInfo->BackgroundDc);
  ctx->Bitmap = CreateCompatibleBitmap(PerfInfo->BackgroundDc, PerfInfo->WndWidth, PerfInfo->WndHeight);
  SelectObject(ctx->BitmapDc, ctx->Bitmap);

  for (y = 0; y < PerfInfo->WndHeight; y++)
    {
      for (x = 0; x < PerfInfo->WndWidth; x++)
        {
          SetPixel(ctx->BitmapDc, x, y, RGB(0xff, 0x00, 0x00));
        }
    }

  *Context = ctx;

  return Reps;
}

void
AlphaBlendCleanup(void *Context, PPERF_INFO PerfInfo)
{
  PALPHABLEND_CONTEXT ctx = Context;
  DeleteDC(ctx->BitmapDc);
  DeleteObject(ctx->Bitmap);
  HeapFree(GetProcessHeap(), 0, ctx);
}


ULONG
DbgPrint(
  IN PCSTR  Format,
  IN ...);

void
AlphaBlendProc(void *Context, PPERF_INFO PerfInfo, unsigned Reps)
{
  PALPHABLEND_CONTEXT ctx = Context;
  unsigned Rep;
  BLENDFUNCTION BlendFunc = { AC_SRC_OVER, 0, 0, 0 };

  for (Rep = 0; Rep < Reps; Rep++)
    {
      BlendFunc.SourceConstantAlpha = 255 * Rep / Reps;
#if 0
      PatBlt(PerfInfo->BackgroundDc, 0, 0, PerfInfo->WndWidth, PerfInfo->WndHeight, PATCOPY);
#endif
      if (!AlphaBlend(PerfInfo->BackgroundDc, 0, 0, PerfInfo->WndWidth, PerfInfo->WndHeight,
                      ctx->BitmapDc, 0, 0, PerfInfo->WndWidth, PerfInfo->WndHeight,
                      BlendFunc))
        {
          DbgPrint("AlphaBlend failed (0x%lx)\n", GetLastError());
        }
    }
}

/* EOF */
