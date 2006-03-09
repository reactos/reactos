#include <w32k.h>

#include "regtests.h"

static void SetupSurface(SURFOBJ* surface, RECTL* rect)
{
  UINT sizex;
  UINT sizey;
  UINT size;
  UINT depth;

  RtlZeroMemory(surface, sizeof(SURFOBJ));
  depth = BitsPerFormat(BMF_24BPP);
  sizex = rect->right - rect->left;
  sizey = rect->bottom - rect->top;
  size = sizey * sizex * depth;
  surface->pvScan0 = malloc(size);
  surface->lDelta = DIB_GetDIBWidthBytes(sizex, depth);
}

static void CleanupSurface(SURFOBJ* surface)
{
  free(surface->pvScan0);
}

static void RunTest()
{
  static RECTL rect = { 0, 0, 100, 100 };
  SURFOBJ surface;
  UINT color;
  UINT i;

  SetupSurface(&surface, &rect);
  for (i = 0; i < 10000; i++)
  {
    BOOLEAN success = DIB_24BPP_ColorFill(&surface, &rect, color);
    _AssertTrue(success);
    if (!success)
      break;
  }
  CleanupSurface(&surface);
}

_DispatcherType(Dib_24bpp_colorfill_performanceTest, "DIB_24BPP_ColorFill performance", TT_PERFORMANCE)
