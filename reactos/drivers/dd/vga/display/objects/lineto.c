#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"


BOOL STDCALL
DrvLineTo(SURFOBJ *Surface,
	  CLIPOBJ *Clip,
	  BRUSHOBJ *Brush,
	  LONG x1,
	  LONG y1,
	  LONG x2,
	  LONG y2,
	  RECTL *RectBounds,
	  MIX mix)

// FIXME: Use ClipObj and RectBounds to clip the line where required
// FIXME: Use Mix to perform ROPs

{
  ULONG x, y, d, i, xchange, ychange, error,
        iSolidColor, hx, vy;
  LONG  deltax, deltay;

  iSolidColor = Brush->iSolidColor; // FIXME: Brush Realization...

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
    };

  if (y1 == y2)
    {
    return vgaHLine(hx, y1, deltax, iSolidColor);
    }
  if (x1 == x2)
    {
    return vgaVLine(x1, vy, deltay, iSolidColor);
    }

  // Using individual pixels to draw a line neither horizontal or vertical
  // Set up the VGA masking for individual pixels

  error = 0;

  if (deltax < deltay)
    {
    for (i = 0; i < deltay; i++)
      {
      vgaPutPixel(x, y, iSolidColor);
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
      vgaPutPixel(x, y, iSolidColor);
      x = x + xchange;
      error = error + deltay;
      if (deltax <= error)
        {
        y = y + ychange;
        error = error - deltax;
        }
      }
   }

   return TRUE;
}
