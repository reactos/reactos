#include <ddk/winddi.h>
#include <include/inteng.h>
#include <include/dib.h>
#include "objects.h"
#include "../dib/dib.h"
#include "misc.h"

#include <include/mouse.h>
#include <include/object.h>
#include <include/surface.h>

BOOL STDCALL
EngLineTo(SURFOBJ *DestObj,
	  CLIPOBJ *Clip,
	  BRUSHOBJ *Brush,
	  LONG x1,
	  LONG y1,
	  LONG x2,
	  LONG y2,
	  RECTL *RectBounds,
	  MIX mix)
{
  LONG x, y, deltax, deltay, i, xchange, ychange, error, hx, vy;
  ULONG Pixel = Brush->iSolidColor;
  SURFOBJ *OutputObj;
  RECTL DestRect;
  POINTL Translate;
  INTENG_ENTER_LEAVE EnterLeave;

  // These functions are assigned if we're working with a DIB
  // The assigned functions depend on the bitsPerPixel of the DIB
  PFN_DIB_PutPixel DIB_PutPixel;
  PFN_DIB_HLine    DIB_HLine;
  PFN_DIB_VLine    DIB_VLine;

  DestRect.left = x1;
  if (x1 != x2)
    {
    DestRect.right = x2;
    }
  else
    {
    DestRect.right = x2 + 1;
    }
  DestRect.top = y1;
  if (y1 != y2)
    {
    DestRect.bottom = y2;
    }
  else
    {
    DestRect.bottom = y2 + 1;
    }

  if (! IntEngEnter(&EnterLeave, DestObj, &DestRect, FALSE, &Translate, &OutputObj))
    {
    return FALSE;
    }

  x1 += Translate.x;
  x2 += Translate.x;
  y1 += Translate.y;
  y2 += Translate.y;

  // Assign DIB functions according to bytes per pixel
  switch(BitsPerFormat(OutputObj->iBitmapFormat))
    {
    case 1:
      DIB_PutPixel = (PFN_DIB_PutPixel)DIB_1BPP_PutPixel;
      DIB_HLine    = (PFN_DIB_HLine)DIB_1BPP_HLine;
      DIB_VLine    = (PFN_DIB_VLine)DIB_1BPP_VLine;
      break;

    case 4:
      DIB_PutPixel = (PFN_DIB_PutPixel)DIB_4BPP_PutPixel;
      DIB_HLine    = (PFN_DIB_HLine)DIB_4BPP_HLine;
      DIB_VLine    = (PFN_DIB_VLine)DIB_4BPP_VLine;
      break;

    case 16:
      DIB_PutPixel = (PFN_DIB_PutPixel)DIB_16BPP_PutPixel;
      DIB_HLine    = (PFN_DIB_HLine)DIB_16BPP_HLine;
      DIB_VLine    = (PFN_DIB_VLine)DIB_16BPP_VLine;
      break;

    case 24:
      DIB_PutPixel = (PFN_DIB_PutPixel)DIB_24BPP_PutPixel;
      DIB_HLine    = (PFN_DIB_HLine)DIB_24BPP_HLine;
      DIB_VLine    = (PFN_DIB_VLine)DIB_24BPP_VLine;
      break;

    default:
      DbgPrint("EngLineTo: unsupported DIB format %u (bitsPerPixel:%u)\n", OutputObj->iBitmapFormat,
               BitsPerFormat(OutputObj->iBitmapFormat));
      return FALSE;
    }

  // FIXME: Implement clipping
  x = x1;
  y = y1;
  deltax = x2 - x1;
  deltay = y2 - y1;

  if (deltax < 0)
    {
    xchange = -1;
    deltax = - deltax;
    hx = x2;
    x--;
    }
  else
    {
    xchange = 1;
    hx = x1;
    }

  if (deltay < 0)
    {
    ychange = -1;
    deltay = - deltay;
    vy = y2;
    y--;
    }
  else
    {
    ychange = 1;
    vy = y1;
    }

  if (y1 == y2)
    {
    DIB_HLine(OutputObj, hx, hx + deltax, y1, Pixel);
    }
  else if (x1 == x2)
    {
    DIB_VLine(OutputObj, x1, vy, vy + deltay, Pixel);
    }
  else
    {
    error = 0;

    if (deltax < deltay)
      {
      for (i = 0; i < deltay; i++)
        {
        DIB_PutPixel(OutputObj, x, y, Pixel);
        y = y + ychange;
        error = error + deltax;

        if (deltay <= error)
          {
          x = x + xchange;
          error = error - deltay;
          }
        }
      }
     else
      {
      for (i = 0; i < deltax; i++)
        {
        DIB_PutPixel(OutputObj, x, y, Pixel);
        x = x + xchange;
        error = error + deltay;
        if (deltax <= error)
          {
          y = y + ychange;
          error = error - deltax;
          }
        }
      }
    }

  return IntEngLeave(&EnterLeave);
}

BOOL STDCALL
IntEngLineTo(SURFOBJ *DestSurf,
	     CLIPOBJ *Clip,
	     BRUSHOBJ *Brush,
	     LONG x1,
	     LONG y1,
	     LONG x2,
	     LONG y2,
	     RECTL *RectBounds,
	     MIX mix)
{
  BOOLEAN ret;
  SURFGDI *SurfGDI;

  /* No success yet */
  ret = FALSE;
  SurfGDI = (SURFGDI*)AccessInternalObjectFromUserObject(DestSurf);

  MouseSafetyOnDrawStart(DestSurf, SurfGDI, x1, y1, x2, y2);

  if (NULL != SurfGDI->LineTo) {
    /* Call the driver's DrvLineTo */
    ret = SurfGDI->LineTo(DestSurf, Clip, Brush, x1, y1, x2, y2, RectBounds, mix);
  }

#if 0
  if (! ret && NULL != SurfGDI->StrokePath) {
    /* FIXME: Emulate LineTo using drivers DrvStrokePath and set ret on success */
  }
#endif

  if (! ret) {
    ret = EngLineTo(DestSurf, Clip, Brush, x1, y1, x2, y2, RectBounds, mix);
  }

  MouseSafetyOnDrawEnd(DestSurf, SurfGDI);

  return ret;
}
