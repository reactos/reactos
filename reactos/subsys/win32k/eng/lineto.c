#include <ddk/winddi.h>
#include "objects.h"

// POSSIBLE FIXME: Switch X and Y's so that drawing a line doesn't try to draw from 150 to 50 (negative dx)

VOID LinePoint(SURFOBJ *Surface, SURFGDI *SurfGDI,
               LONG x, LONG y, ULONG iColor)
{
   ULONG offset;

   offset = (Surface->lDelta*y)+(x*SurfGDI->BytesPerPixel);

   memcpy(Surface->pvBits+offset,
          &iColor, SurfGDI->BytesPerPixel);
}

BOOL EngHLine(SURFOBJ *Surface, SURFGDI *SurfGDI,
              LONG x, LONG y, LONG len, ULONG iColor)
{
   ULONG offset, ix;

   offset = (Surface->lDelta*y)+(x*SurfGDI->BytesPerPixel);

   for(ix=0; ix<len*SurfGDI->BytesPerPixel; ix+=SurfGDI->BytesPerPixel)
      memcpy(Surface->pvBits+offset+ix,
             &iColor, SurfGDI->BytesPerPixel);

   return TRUE;
}

BOOL EngLineTo(SURFOBJ *Surface, CLIPOBJ *Clip, BRUSHOBJ *Brush,
               LONG x1, LONG y1, LONG x2, LONG y2,
               RECTL *RectBounds, MIX mix)
{
   SURFGDI *SurfGDI;
   LONG x, y, d, deltax, deltay, i, length, xchange, ychange, error;

   SurfGDI = AccessInternalObjectFromUserObject(Surface);

   if(Surface->iType!=STYPE_BITMAP)
   {
      // Call the driver's DrvLineTo
      return SurfGDI->LineTo(Surface, Clip, Brush, x1, y1, x2, y2,
                             RectBounds, mix);
   }

   // FIXME: Implement clipping

   if(y1==y2) return EngHLine(Surface, SurfGDI, x1, y1, (x2-x1), Brush->iSolidColor);

   x=x1;
   y=y1;
   deltax=x2-x1;
   deltay=y2-y1;

   if(deltax<0)
   {
      xchange=-1;
      deltax=-deltax;
   } else
   {
      xchange=1;
   }

   if(deltay<0)
   {
      ychange=-1;
      deltay=-deltay;
   } else
   {
      ychange=1;
   };

  error=0;
  i=0;

  if(deltax<deltay)
  {
     length=deltay+1;
     while(i<length)
     {
        LinePoint(Surface, SurfGDI, x, y, Brush->iSolidColor);
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
         LinePoint(Surface, SurfGDI, x, y, Brush->iSolidColor);
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
