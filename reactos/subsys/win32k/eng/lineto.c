#include <ddk/winddi.h>
#include <include/inteng.h>
#include "objects.h"
#include "../dib/dib.h"

#include <include/mouse.h>
#include <include/object.h>
#include <include/surface.h>

BOOL STDCALL
EngLineTo(SURFOBJ *Surface,
	  CLIPOBJ *Clip,
	  BRUSHOBJ *Brush,
	  LONG x1,
	  LONG y1,
	  LONG x2,
	  LONG y2,
	  RECTL *RectBounds,
	  MIX mix)
{
  LONG x, y, deltax, deltay, i, length, xchange, ychange, error, hx, vy;
  ULONG Pixel = Brush->iSolidColor;

  // These functions are assigned if we're working with a DIB
  // The assigned functions depend on the bitsPerPixel of the DIB
  PFN_DIB_PutPixel DIB_PutPixel;
  PFN_DIB_HLine    DIB_HLine;
  PFN_DIB_VLine    DIB_VLine;

  // Assign DIB functions according to bytes per pixel
  switch(BitsPerFormat(Surface->iBitmapFormat))
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
      DbgPrint("EngLineTo: unsupported DIB format %u (bitsPerPixel:%u)\n", Surface->iBitmapFormat,
               BitsPerFormat(Surface->iBitmapFormat));
      return FALSE;
  }

  // FIXME: Implement clipping
  x=x1;
  y=y1;
  deltax=x2-x1;
  deltay=y2-y1;

  if(deltax<0)
  {
    xchange=-1;
    deltax=-deltax;
    hx = x2;
  } else
  {
    xchange=1;
    hx = x1;
  }

  if(deltay<0)
  {
    ychange=-1;
    deltay=-deltay;
    vy = y2;
  } else
  {
    ychange=1;
    vy = y1;
  }

  if(y1==y2) { DIB_HLine(Surface, hx, hx + deltax, y1, Pixel); return TRUE; }
  if(x1==x2) { DIB_VLine(Surface, x1, vy, vy + deltay, Pixel); return TRUE; }

  error=0;
  i=0;

  if(deltax<deltay)
  {
    length=deltay+1;
    while(i<length)
    {
      DIB_PutPixel(Surface, x, y, Pixel);
      y=y+ychange;
      error=error+deltax;

      if(error>deltay)
      {
        x=x+xchange;
        error=error-deltay;
      }
      i=i+1;
    }
  } else
  {
    length=deltax+1;
    while(i<length)
    {
      DIB_PutPixel(Surface, x, y, Pixel);
      x=x+xchange;
      error=error+deltay;
      if(error>deltax)
      {
        y=y+ychange;
        error=error-deltax;
      }
      i=i+1;
    }
  }

  return TRUE;
}

BOOL STDCALL
IntEngLineTo(SURFOBJ *Surface,
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
  SurfGDI = (SURFGDI*)AccessInternalObjectFromUserObject(Surface);

  MouseSafetyOnDrawStart(Surface, SurfGDI, x1, y1, x2, y2);

  if (NULL != SurfGDI->LineTo) {
    /* Call the driver's DrvLineTo */
    ret = SurfGDI->LineTo(Surface, Clip, Brush, x1, y1, x2, y2, RectBounds, mix);
  }

#if 0
  if (! ret && NULL != SurfGDI->StrokePath) {
    /* FIXME: Emulate LineTo using drivers DrvStrokePath and set ret on success */
  }
#endif

  if (! ret) {
    ret = EngLineTo(Surface, Clip, Brush, x1, y1, x2, y2, RectBounds, mix);
  }

  MouseSafetyOnDrawEnd(Surface, SurfGDI);

  return ret;
}
