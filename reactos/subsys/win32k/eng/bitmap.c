/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Bitmap Functions
 * FILE:              subsys/win32k/eng/bitmap.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/winddi.h>

HBITMAP EngCreateDeviceBitmap(DHSURF dhsurf, SIZEL Size, ULONG Format)
{
   BITMAP *btc;
   ULONG bpp, cplanes;

   if(Format==BMF_1BPP)
   {
      bpp=1;
      cplanes=1;
   } else
   if((Format==BMF_4BPP) || (Format==BMF_4RLE))
   {
      bpp=4;
      cplanes=1;
   } else
   if((Format==BMF_8BPP) || (Format==BMF_8RLE))
   {
      bpp=8;
      cplanes=1;
   } else
   if(Format==BMF_16BPP)
   {
      bpp=16;
      cplanes=2;
   } else
   if(Format==BMF_24BPP)
   {
      bpp=24;
      cplanes=3;
   } else
   if(Format==BMF_32BPP)
   {
      bpp=32;
      cplanes=4;
   }

   // Allocate memory for the bitmap structure
   btc=EngAllocMem(FL_ZERO_MEMORY, sizeof(BITMAP), 0);

   // Assign properties of newly created bitmap
   btc->bmType   = Format;
   btc->bmWidth  = Size.cx;
   btc->bmHeight = Size.cy;
   btc->bmWidthBytes = Size.cx * cplanes;
   btc->bmPlanes     = cplanes;
   btc->bmBitsPixel  = bpp;

   // Assume that the value returned from ExAllocatePool is the handle value
   return btc;
}


HBITMAP EngCreateBitmap(IN SIZEL  Size,
                        IN LONG  Width,
                        IN ULONG  Format,
                        IN ULONG  Flags,
                        IN PVOID  Bits)
{
   BITMAP *btc;
   ULONG tsize;

   // Create the handle for the bitmap
   btc = EngCreateDeviceBitmap(NULL, Size, Format);

   // Size of bitmap = total pixels * color planes
   tsize=Size.cx*Size.cy*btc->bmPlanes;

   // Allocate memory for the bitmap
   if(Bits!=NULL)
   {
      if((Flags & BMF_USERMEM)==0)
      {
         if((Flags & BMF_NOZEROINIT)==0)
         {
            Bits=EngAllocMem(FL_ZERO_MEMORY, tsize, 0);
         } else {
            Bits=EngAllocMem(0, tsize, 0);
         }
      } else
      {
         Bits=EngAllocUserMem(tsize, 0);
      }
   }

   btc->bmBits       = Bits;

   // Assume that the value returned from ExAllocatePool is the handle value
   return btc;
}
