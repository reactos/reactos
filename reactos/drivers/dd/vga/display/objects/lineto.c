#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"


BOOL VGADDILineTo(SURFOBJ *Surface, CLIPOBJ *Clip, BRUSHOBJ *Brush,
                  LONG x1, LONG y1, LONG x2, LONG y2,
                  RECTL *RectBounds, MIX mix)

// FIXME: Use ClipObj and RectBounds to clip the line where required
// FIXME: Use Mix to perform ROPs

{
   ULONG x, y, d, i, length, xchange, ychange, error,
         iSolidColor, hx, vy;
   LONG  deltax, deltay;
DbgPrint("drvlineto\n");
   iSolidColor = Brush->iSolidColor; // FIXME: Brush Realization...

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
   };

   if(y1==y2)
   {
     return vgaHLine(hx, y1, deltax, iSolidColor);
   }
   if(x1==x2)
   {
     return vgaVLine(x1, vy, deltay, iSolidColor);
   }

  error=0;
  i=0;

  if(deltax<deltay)
  {
     length=deltay+1;
     while(i<length)
     {
        vgaPutPixel(x, y, iSolidColor);
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
         vgaPutPixel(x, y, iSolidColor);
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
