#include <ddk/winddi.h>
#include "objects.h"
#include "../dib/dib.h"

BOOL EngLineTo(SURFOBJ *Surface, CLIPOBJ *Clip, BRUSHOBJ *Brush,
               LONG x1, LONG y1, LONG x2, LONG y2,
               RECTL *RectBounds, MIX mix)
{
  BOOLEAN ret;
  SURFGDI *SurfGDI;
  LONG x, y, d, deltax, deltay, i, length, xchange, ychange, error, hx, vy;

  // These functions are assigned if we're working with a DIB
  // The assigned functions depend on the bitsPerPixel of the DIB
  PFN_DIB_PutPixel DIB_PutPixel;
  PFN_DIB_HLine    DIB_HLine;
  PFN_DIB_VLine    DIB_VLine;

  SurfGDI = (SURFGDI*)AccessInternalObjectFromUserObject(Surface);

  MouseSafetyOnDrawStart(Surface, SurfGDI, x1, y1, x2, y2);

  if(Surface->iType!=STYPE_BITMAP)
  {
    // Call the driver's DrvLineTo
    ret = SurfGDI->LineTo(Surface, Clip, Brush, x1, y1, x2, y2, RectBounds, mix);
    MouseSafetyOnDrawEnd(Surface, SurfGDI);
    return ret;
  }

  // Assign DIB functions according to bytes per pixel
  switch(BitsPerFormat(Surface->iBitmapFormat))
  {
    case 4:
      DIB_PutPixel = DIB_4BPP_PutPixel;
      DIB_HLine    = DIB_4BPP_HLine;
      DIB_VLine    = DIB_4BPP_VLine;
      break;

    case 24:
      DIB_PutPixel = DIB_24BPP_PutPixel;
      DIB_HLine    = DIB_24BPP_HLine;
      DIB_VLine    = DIB_24BPP_VLine;
      break;

    default:
      DbgPrint("EngLineTo: unsupported DIB format %u (bitsPerPixel:%u)\n", Surface->iBitmapFormat,
               BitsPerFormat(Surface->iBitmapFormat));

      MouseSafetyOnDrawEnd(Surface, SurfGDI);
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

  if(y1==y2) { DIB_HLine(Surface, hx, hx + deltax, y1, Brush->iSolidColor); MouseSafetyOnDrawEnd(Surface, SurfGDI); return TRUE; }
  if(x1==x2) { DIB_VLine(Surface, x1, vy, vy + deltay, Brush->iSolidColor); MouseSafetyOnDrawEnd(Surface, SurfGDI); return TRUE; }

  error=0;
  i=0;

  if(deltax<deltay)
  {
    length=deltay+1;
    while(i<length)
    {
      DIB_PutPixel(Surface, x, y, Brush->iSolidColor);
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
      DIB_PutPixel(Surface, x, y, Brush->iSolidColor);
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

  MouseSafetyOnDrawEnd(Surface, SurfGDI);
  return TRUE;
}
