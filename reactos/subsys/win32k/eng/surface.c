/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Surace Functions
 * FILE:              subsys/win32k/eng/surface.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/winddi.h>
#include <win32k/dc.h>
#include "objects.h"

BYTE bytesPerPixel(ULONG Format)
{
   // FIXME: GDI bitmaps are supposed to be pixel-packed. Right now if the
   // pixel size if < 1 byte we expand it to 1 byte

   if(Format==BMF_1BPP)
   {
      return 1;
   } else
   if((Format==BMF_4BPP) || (Format==BMF_4RLE))
   {
      return 1;
   } else
   if((Format==BMF_8BPP) || (Format==BMF_8RLE))
   {
      return 1;
   } else
   if(Format==BMF_16BPP)
   {
      return 2;
   } else
   if(Format==BMF_24BPP)
   {
      return 3;
   } else
   if(Format==BMF_32BPP)
   {
      return 4;
   }

   return 0;
}

VOID InitializeHooks(SURFGDI *SurfGDI)
{
   SurfGDI->BitBlt   = NULL;
   SurfGDI->CopyBits = NULL;
}

HBITMAP EngCreateDeviceBitmap(DHSURF dhsurf, SIZEL Size, ULONG Format)
{
   HBITMAP NewBitmap;
   SURFOBJ *SurfObj;
   SURFGDI *SurfGDI;

   SurfObj = EngAllocMem(FL_ZERO_MEMORY, sizeof(SURFOBJ), NULL);
   SurfGDI = EngAllocMem(FL_ZERO_MEMORY, sizeof(SURFGDI), NULL);

   NewBitmap = CreateGDIHandle(SurfGDI, SurfObj);

   InitializeHooks(SurfGDI);

   SurfGDI->BytesPerPixel = bytesPerPixel(Format);

   SurfObj->dhsurf = dhsurf;
   SurfObj->hsurf  = dhsurf; // FIXME: Is this correct??
   SurfObj->sizlBitmap = Size;
   SurfObj->iBitmapFormat = Format;
   SurfObj->lDelta = SurfGDI->BytesPerPixel * Size.cx;
   SurfObj->iType = STYPE_DEVBITMAP;

   return NewBitmap;
}

HBITMAP EngCreateBitmap(IN SIZEL  Size,
                        IN LONG  Width,
                        IN ULONG  Format,
                        IN ULONG  Flags,
                        IN PVOID  Bits)
{
   HBITMAP NewBitmap;
   SURFOBJ *SurfObj;
   SURFGDI *SurfGDI;

   SurfObj = EngAllocMem(FL_ZERO_MEMORY, sizeof(SURFOBJ), NULL);
   SurfGDI = EngAllocMem(FL_ZERO_MEMORY, sizeof(SURFGDI), NULL);

   NewBitmap = CreateGDIHandle(SurfGDI, SurfObj);

   InitializeHooks(SurfGDI);
   SurfGDI->BytesPerPixel = bytesPerPixel(Format);

   SurfObj->cjBits = Width * Size.cy;

   if(Bits!=NULL)
   {
      SurfObj->pvBits = Bits;
   } else
   {
      if(Flags & BMF_USERMEM)
      {
         SurfObj->pvBits = EngAllocUserMem(SurfObj->cjBits, 0);
      } else {
         if(Flags & BMF_NOZEROINIT)
         {
            SurfObj->pvBits = EngAllocMem(0, SurfObj->cjBits, 0);
         } else {
            SurfObj->pvBits = EngAllocMem(FL_ZERO_MEMORY, SurfObj->cjBits, 0);
         }
      }
   }

   SurfObj->dhsurf = 0;
   SurfObj->hsurf  = 0;
   SurfObj->sizlBitmap = Size;
   SurfObj->iBitmapFormat = Format;
   SurfObj->lDelta = Width;
   SurfObj->iType = STYPE_BITMAP;

   // Use flags to determine bitmap type -- TOP_DOWN or whatever

   return NewBitmap;
}

HSURF EngCreateDeviceSurface(DHSURF dhsurf, SIZEL Size, ULONG Format)
{
   HSURF   NewSurface;
   SURFOBJ *SurfObj;
   SURFGDI *SurfGDI;

   // DrvCreateDeviceSurface???

   SurfObj = EngAllocMem(FL_ZERO_MEMORY, sizeof(SURFOBJ), NULL);
   SurfGDI = EngAllocMem(FL_ZERO_MEMORY, sizeof(SURFGDI), NULL);

   NewSurface = CreateGDIHandle(SurfGDI, SurfObj);

   InitializeHooks(SurfGDI);

   SurfGDI->BytesPerPixel = bytesPerPixel(Format);

   SurfObj->dhsurf = dhsurf;
   SurfObj->hsurf  = dhsurf; // FIXME: Is this correct??
   SurfObj->sizlBitmap = Size;
   SurfObj->iBitmapFormat = Format;
   SurfObj->lDelta = SurfGDI->BytesPerPixel * Size.cx;
   SurfObj->iType = STYPE_DEVICE;

   return NewSurface;
}

PFN DriverFunction(DRVENABLEDATA *DED, ULONG DriverFunc)
{
   ULONG i;

   for(i=0; i<DED->c; i++)
   {
      if(DED->pdrvfn[i].iFunc == DriverFunc)
         return DED->pdrvfn[i].pfn;
   }
   return NULL;
}

BOOL EngAssociateSurface(HSURF Surface, HDEV Dev, ULONG Hooks)
{
   SURFOBJ *SurfObj;
   SURFGDI *SurfGDI;

// it looks like this Dev is actually a pointer to the DC!
   PDC Dc = (PDC)Dev;

//   DRVENABLEDATA *DED;

   SurfGDI = AccessInternalObject(Surface);
   SurfObj = AccessUserObject(Surface);

//   DED = AccessInternalObject(Dev);

   // Associate the hdev
   SurfObj->hdev = Dev;

   // Hook up specified functions
   if(Hooks & HOOK_BITBLT)            SurfGDI->BitBlt            = Dc->DriverFunctions.BitBlt;
   if(Hooks & HOOK_STRETCHBLT)        SurfGDI->StretchBlt        = Dc->DriverFunctions.StretchBlt;
   if(Hooks & HOOK_TEXTOUT)           SurfGDI->TextOut           = Dc->DriverFunctions.TextOut;
   if(Hooks & HOOK_PAINT)             SurfGDI->Paint             = Dc->DriverFunctions.Paint;
   if(Hooks & HOOK_STROKEPATH)        SurfGDI->StrokePath        = Dc->DriverFunctions.StrokePath;
   if(Hooks & HOOK_FILLPATH)          SurfGDI->FillPath          = Dc->DriverFunctions.FillPath;
   if(Hooks & HOOK_STROKEANDFILLPATH) SurfGDI->StrokeAndFillPath = Dc->DriverFunctions.StrokeAndFillPath;
   if(Hooks & HOOK_LINETO) {           SurfGDI->LineTo            = Dc->DriverFunctions.LineTo;
DbgPrint("associating LineTo is now %08x\n", SurfGDI->LineTo);
}
   if(Hooks & HOOK_COPYBITS)          SurfGDI->CopyBits          = Dc->DriverFunctions.CopyBits;
   if(Hooks & HOOK_SYNCHRONIZE)       SurfGDI->Synchronize       = Dc->DriverFunctions.Synchronize;
   if(Hooks & HOOK_SYNCHRONIZEACCESS) SurfGDI->SynchronizeAccess = TRUE;

   return TRUE;
}

BOOL EngDeleteSurface(HSURF Surface)
{
   SURFOBJ *SurfObj;
   SURFGDI *SurfGDI;

   SurfGDI = AccessInternalObject(Surface);
   SurfObj = AccessUserObject(Surface);

   EngFreeMem(SurfGDI);
   EngFreeMem(SurfObj);
   FreeGDIHandle(Surface);

   return TRUE;
}

SURFOBJ *EngLockSurface(HSURF Surface)
{
   // FIXME: Do we need to allocate new memory for this user object??

   return AccessUserObject(Surface);
}
