/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id: surface.c,v 1.19 2003/05/18 17:16:17 ea Exp $
 * 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Surace Functions
 * FILE:              subsys/win32k/eng/surface.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 *                 9/11/2000: Updated to handle real pixel packed bitmaps (UPDATE TO DATE COMPLETED)
 * TESTING TO BE DONE:
 * - Create a GDI bitmap with all formats, perform all drawing operations on them, render to VGA surface
 *   refer to \test\microwin\src\engine\devdraw.c for info on correct pixel plotting for various formats
 */

#include <ddk/winddi.h>
#include <win32k/dc.h>
#include <include/dib.h>
#include <include/object.h>
#include <include/paint.h>
#include "handle.h"
#include "../dib/dib.h"

#define NDEBUG
#include <win32k/debug1.h>

INT FASTCALL BitsPerFormat(ULONG Format)
{
  switch(Format)
  {
    case BMF_1BPP: return 1;
    case BMF_4BPP:
    case BMF_4RLE: return 4;
    case BMF_8BPP:
    case BMF_8RLE: return 8;
    case BMF_16BPP: return 16;
    case BMF_24BPP: return 24;
    case BMF_32BPP: return 32;
    default: return 0;
  }
}

ULONG FASTCALL BitmapFormat(WORD Bits, DWORD Compression)
{
  switch(Compression)
  {
    case BI_RGB:
      switch(Bits)
      {
        case 1: return BMF_1BPP;
        case 4: return BMF_4BPP;
        case 8: return BMF_8BPP;
        case 16: return BMF_16BPP;
        case 24: return BMF_24BPP;
        case 32: return BMF_32BPP;
      }

    case BI_RLE4: return BMF_4RLE;
    case BI_RLE8: return BMF_8RLE;

    default: return 0;
  }
}

static VOID Dummy_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c)
{
  return;
}

static VOID Dummy_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  return;
}

static VOID Dummy_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  return;
}

static BOOLEAN Dummy_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI,  SURFGDI *SourceGDI,
                            PRECTL  DestRect,  POINTL  *SourcePoint,
                            XLATEOBJ *ColorTranslation)
{
  return FALSE;
}

#define SURF_METHOD(c,n) DIB_##c##_##n
#define SET_SURFGDI(c)\
 SurfGDI->DIB_PutPixel=SURF_METHOD(c,PutPixel);\
 SurfGDI->DIB_HLine=SURF_METHOD(c,HLine);\
 SurfGDI->DIB_VLine=SURF_METHOD(c,VLine);\
 SurfGDI->DIB_BitBlt=SURF_METHOD(c,BitBlt);

VOID FASTCALL InitializeFuncs(SURFGDI *SurfGDI, ULONG BitmapFormat)
{
  SurfGDI->BitBlt   = NULL;
  SurfGDI->CopyBits = NULL;
  SurfGDI->CreateDeviceBitmap = NULL;
  SurfGDI->SetPalette = NULL;
  SurfGDI->TransparentBlt = NULL;

  switch(BitmapFormat)
    {
    case BMF_1BPP:  SET_SURFGDI(1BPP) break;
    case BMF_4BPP:  SET_SURFGDI(4BPP) break;
    case BMF_8BPP:  SET_SURFGDI(8BPP) break;
    case BMF_16BPP: SET_SURFGDI(16BPP) break;
    case BMF_24BPP: SET_SURFGDI(24BPP) break;
    case BMF_32BPP: SET_SURFGDI(32BPP) break;
    case BMF_4RLE:
    case BMF_8RLE:
      /* Not supported yet, fall through to unrecognized case */
    default:
      DPRINT1("InitializeFuncs: unsupported DIB format %d\n",
               BitmapFormat);

      SurfGDI->DIB_PutPixel = Dummy_PutPixel;
      SurfGDI->DIB_HLine    = Dummy_HLine;
      SurfGDI->DIB_VLine    = Dummy_VLine;
      SurfGDI->DIB_BitBlt   = Dummy_BitBlt;
      break;
    }
}

HBITMAP STDCALL
EngCreateDeviceBitmap(IN DHSURF dhsurf,
		      IN SIZEL Size,
		      IN ULONG Format)
{
  HBITMAP NewBitmap;
  SURFOBJ *SurfObj;

  NewBitmap = EngCreateBitmap(Size, DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(Format)), Format, 0, NULL);
  SurfObj = (PVOID)AccessUserObject((ULONG)NewBitmap);
  SurfObj->dhpdev = dhsurf;

  return NewBitmap;
}

HBITMAP STDCALL
EngCreateBitmap(IN SIZEL Size,
		IN LONG Width,
		IN ULONG Format,
		IN ULONG Flags,
		IN PVOID Bits)
{
  HBITMAP NewBitmap;
  SURFOBJ *SurfObj;
  SURFGDI *SurfGDI;


  NewBitmap = (PVOID)CreateGDIHandle(sizeof(SURFGDI), sizeof(SURFOBJ));
  if( !ValidEngHandle( NewBitmap ) )
	return 0;

  SurfObj = (SURFOBJ*) AccessUserObject( (ULONG) NewBitmap );
  SurfGDI = (SURFGDI*) AccessInternalObject( (ULONG) NewBitmap );
  ASSERT( SurfObj );
  ASSERT( SurfGDI );

  SurfGDI->BitsPerPixel = BitsPerFormat(Format);
  SurfObj->lDelta = Width;
  SurfObj->cjBits = SurfObj->lDelta * Size.cy;

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

  SurfObj->dhsurf = 0; // device managed surface
  SurfObj->hsurf  = 0;
  SurfObj->sizlBitmap = Size;
  SurfObj->iBitmapFormat = Format;
  SurfObj->iType = STYPE_BITMAP;
  SurfObj->fjBitmap = Flags & (BMF_TOPDOWN | BMF_NOZEROINIT);
  SurfObj->pvScan0 = SurfObj->pvBits;

  InitializeFuncs(SurfGDI, Format);

  // Use flags to determine bitmap type -- TOP_DOWN or whatever

  return NewBitmap;
}

HSURF STDCALL
EngCreateDeviceSurface(IN DHSURF dhsurf,
		       IN SIZEL Size,
		       IN ULONG Format)
{
  HSURF   NewSurface;
  SURFOBJ *SurfObj;
  SURFGDI *SurfGDI;

  NewSurface = (HSURF)CreateGDIHandle(sizeof( SURFGDI ), sizeof( SURFOBJ ));
  if( !ValidEngHandle( NewSurface ) )
	return 0;

  SurfObj = (SURFOBJ*) AccessUserObject( (ULONG) NewSurface );
  SurfGDI = (SURFGDI*) AccessInternalObject( (ULONG) NewSurface );
  ASSERT( SurfObj );
  ASSERT( SurfGDI );

  SurfGDI->BitsPerPixel = BitsPerFormat(Format);
  SurfObj->dhsurf = dhsurf;
  SurfObj->hsurf  = dhsurf; // FIXME: Is this correct??
  SurfObj->sizlBitmap = Size;
  SurfObj->iBitmapFormat = Format;
  SurfObj->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(Format));
  SurfObj->iType = STYPE_DEVICE;

  InitializeFuncs(SurfGDI, Format);

  return NewSurface;
}

PFN FASTCALL DriverFunction(DRVENABLEDATA *DED, ULONG DriverFunc)
{
  ULONG i;

  for(i=0; i<DED->c; i++)
  {
    if(DED->pdrvfn[i].iFunc == DriverFunc)
      return DED->pdrvfn[i].pfn;
  }
  return NULL;
}

BOOL STDCALL
EngAssociateSurface(IN HSURF Surface,
		    IN HDEV Dev,
		    IN ULONG Hooks)
{
  SURFOBJ *SurfObj;
  SURFGDI *SurfGDI;
  GDIDEVICE* Device;

  Device = (GDIDEVICE*)Dev;

  SurfGDI = (PVOID)AccessInternalObject((ULONG)Surface);
  SurfObj = (PVOID)AccessUserObject((ULONG)Surface);

  // Associate the hdev
  SurfObj->hdev = Dev;

  // Hook up specified functions
  if(Hooks & HOOK_BITBLT)            SurfGDI->BitBlt            = Device->DriverFunctions.BitBlt;
  if(Hooks & HOOK_TRANSPARENTBLT)    SurfGDI->TransparentBlt	= Device->DriverFunctions.TransparentBlt;
  if(Hooks & HOOK_STRETCHBLT)        SurfGDI->StretchBlt        = (PFN_StretchBlt)Device->DriverFunctions.StretchBlt;
  if(Hooks & HOOK_TEXTOUT)           SurfGDI->TextOut           = Device->DriverFunctions.TextOut;
  if(Hooks & HOOK_PAINT)             SurfGDI->Paint             = Device->DriverFunctions.Paint;
  if(Hooks & HOOK_STROKEPATH)        SurfGDI->StrokePath        = Device->DriverFunctions.StrokePath;
  if(Hooks & HOOK_FILLPATH)          SurfGDI->FillPath          = Device->DriverFunctions.FillPath;
  if(Hooks & HOOK_STROKEANDFILLPATH) SurfGDI->StrokeAndFillPath = Device->DriverFunctions.StrokeAndFillPath;
  if(Hooks & HOOK_LINETO)            SurfGDI->LineTo            = Device->DriverFunctions.LineTo;
  if(Hooks & HOOK_COPYBITS)          SurfGDI->CopyBits          = Device->DriverFunctions.CopyBits;
  if(Hooks & HOOK_SYNCHRONIZE)       SurfGDI->Synchronize       = Device->DriverFunctions.Synchronize;
  if(Hooks & HOOK_SYNCHRONIZEACCESS) SurfGDI->SynchronizeAccess = TRUE;

  SurfGDI->CreateDeviceBitmap = Device->DriverFunctions.CreateDeviceBitmap;
  SurfGDI->SetPalette = Device->DriverFunctions.SetPalette;
  SurfGDI->MovePointer = Device->DriverFunctions.MovePointer;
  SurfGDI->SetPointerShape = (PFN_SetPointerShape)Device->DriverFunctions.SetPointerShape;

  return TRUE;
}

BOOL STDCALL
EngDeleteSurface(IN HSURF Surface)
{
  FreeGDIHandle((ULONG)Surface);
  return TRUE;
}

BOOL STDCALL
EngEraseSurface(SURFOBJ *Surface,
		RECTL *Rect,
		ULONG iColor)
{
  return FillSolid(Surface, Rect, iColor);
}

SURFOBJ * STDCALL
EngLockSurface(IN HSURF Surface)
{
  // FIXME: Call GDI_LockObject (see subsys/win32k/objects/gdi.c)
  return (SURFOBJ*)AccessUserObject((ULONG)Surface);
}

VOID STDCALL
EngUnlockSurface(IN SURFOBJ *Surface)
{
  // FIXME: Call GDI_UnlockObject
}
/* EOF */
