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
/* $Id: surface.c,v 1.40 2004/05/30 14:01:12 weiden Exp $
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
#include <w32k.h>

enum Rle_EscapeCodes
{
  RLE_EOL   = 0, /* End of line */
  RLE_END   = 1, /* End of bitmap */
  RLE_DELTA = 2  /* Delta */
};

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

static VOID Dummy_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c)
{
  return;
}

static ULONG Dummy_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y)
{
  return 0;
}

static VOID Dummy_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  return;
}

static VOID Dummy_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  return;
}

static BOOLEAN Dummy_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI,  SURFGDI *SourceGDI,
                            RECTL*  DestRect,  POINTL  *SourcePoint,
                            BRUSHOBJ* BrushObj, POINTL BrushOrign,
                            XLATEOBJ *ColorTranslation, ULONG Rop4)
{
  return FALSE;
}

static BOOLEAN Dummy_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                SURFGDI *DestGDI,  SURFGDI *SourceGDI,
                                RECTL*  DestRect,  RECTL  *SourceRect,
                                POINTL* MaskOrigin, POINTL BrushOrign,
                                CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                                ULONG Mode)
{
  return FALSE;
}

static BOOLEAN Dummy_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                    PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                                    RECTL*  DestRect,  POINTL  *SourcePoint,
                                    XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  return FALSE;
}


#define SURF_METHOD(c,n) DIB_##c##_##n
#define SET_SURFGDI(c)\
 SurfGDI->DIB_PutPixel=SURF_METHOD(c,PutPixel);\
 SurfGDI->DIB_GetPixel=SURF_METHOD(c,GetPixel);\
 SurfGDI->DIB_HLine=SURF_METHOD(c,HLine);\
 SurfGDI->DIB_VLine=SURF_METHOD(c,VLine);\
 SurfGDI->DIB_BitBlt=SURF_METHOD(c,BitBlt);\
 SurfGDI->DIB_StretchBlt=SURF_METHOD(c,StretchBlt);\
 SurfGDI->DIB_TransparentBlt=SURF_METHOD(c,TransparentBlt);

VOID FASTCALL InitializeFuncs(SURFGDI *SurfGDI, ULONG BitmapFormat)
{
  SurfGDI->BitBlt   = NULL;
  SurfGDI->StretchBlt   = NULL;
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

      SurfGDI->DIB_PutPixel       = Dummy_PutPixel;
      SurfGDI->DIB_GetPixel       = Dummy_GetPixel;
      SurfGDI->DIB_HLine          = Dummy_HLine;
      SurfGDI->DIB_VLine          = Dummy_VLine;
      SurfGDI->DIB_BitBlt         = Dummy_BitBlt;
      SurfGDI->DIB_StretchBlt     = Dummy_StretchBlt;
      SurfGDI->DIB_TransparentBlt = Dummy_TransparentBlt;
      break;
    }
}

/*
 * @implemented
 */
HBITMAP STDCALL
EngCreateDeviceBitmap(IN DHSURF dhsurf,
		      IN SIZEL Size,
		      IN ULONG Format)
{
  HBITMAP NewBitmap;
  SURFOBJ *SurfObj;

  NewBitmap = EngCreateBitmap(Size, DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(Format)), Format, 0, NULL);
  SurfObj = (PVOID)AccessUserObject((ULONG)NewBitmap);
  SurfObj->dhsurf = dhsurf;

  return NewBitmap;
}

VOID Decompress4bpp(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits, LONG Delta) 
{
	int x = 0;
	int y = Size.cy - 1;
	int c;
	int length;
	int width = ((Size.cx+1)/2);
	int height = Size.cy - 1;
	BYTE *begin = CompressedBits;
	BYTE *bits = CompressedBits;
	BYTE *temp;
	while (y >= 0)
	{
		length = *bits++ / 2;
		if (length)
		{
			c = *bits++;
			while (length--)
			{
				if (x >= width) break;
				temp = UncompressedBits + (((height - y) * Delta) + x);
				x++;
				*temp = c;
			}
		} else {
			length = *bits++;
			switch (length)
			{
			case RLE_EOL:
				x = 0;
				y--;
				break;
			case RLE_END:
				return;
			case RLE_DELTA:
				x += (*bits++)/2;
				y -= (*bits++)/2;
				break;
			default:
				length /= 2;
				while (length--)
				{
					c = *bits++;
					if (x < width)
					{
						temp = UncompressedBits + (((height - y) * Delta) + x);
						x++;
						*temp = c;
					}
				}
				if ((bits - begin) & 1)
					bits++;
			}
		}
	}
}

VOID Decompress8bpp(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits, LONG Delta) 
{
	int x = 0;
	int y = Size.cy - 1;
	int c;
	int length;
	int width = Size.cx;
	int height = Size.cy - 1;
	BYTE *begin = CompressedBits;
	BYTE *bits = CompressedBits;
	BYTE *temp;
	while (y >= 0)
	{
		length = *bits++;
		if (length)
		{
			c = *bits++;
			while (length--)
			{
				if (x >= width) break;
				temp = UncompressedBits + (((height - y) * Delta) + x);
				x++;
				*temp = c;
			}
		} else {
			length = *bits++;
			switch (length)
			{
			case RLE_EOL:
				x = 0;
				y--;
				break;
			case RLE_END:
				return;
			case RLE_DELTA:
				x += *bits++;
				y -= *bits++;
				break;
			default:
				while (length--)
				{
					c = *bits++;
					if (x < width)
					{
						temp = UncompressedBits + (((height - y) * Delta) + x);
						x++;
						*temp = c;
					}
				}
				if ((bits - begin) & 1)
					bits++;
			}
		}
	}
}

/*
 * @implemented
 */
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
  PVOID UncompressedBits;
  ULONG UncompressedFormat;

  NewBitmap = (PVOID)CreateGDIHandle(sizeof(SURFGDI), sizeof(SURFOBJ), (PVOID*)&SurfGDI, (PVOID*)&SurfObj);
  if( !ValidEngHandle( NewBitmap ) )
	return 0;

  SurfGDI->BitsPerPixel = BitsPerFormat(Format);
  if (Format == BMF_4RLE) {
	  SurfObj->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(BMF_4BPP));
	  SurfObj->cjBits = SurfObj->lDelta * Size.cy;
	  UncompressedFormat = BMF_4BPP;
      UncompressedBits = EngAllocMem(FL_ZERO_MEMORY, SurfObj->cjBits, 0);
	  Decompress4bpp(Size, (BYTE *)Bits, (BYTE *)UncompressedBits, SurfObj->lDelta);
  } else {
	  if (Format == BMF_8RLE) {
		  SurfObj->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(BMF_8BPP));
		  SurfObj->cjBits = SurfObj->lDelta * Size.cy;
	      UncompressedFormat = BMF_8BPP;
	      UncompressedBits = EngAllocMem(FL_ZERO_MEMORY, SurfObj->cjBits, 0);
		  Decompress8bpp(Size, (BYTE *)Bits, (BYTE *)UncompressedBits, SurfObj->lDelta);
	  } else {
  SurfObj->lDelta = Width;
  SurfObj->cjBits = SurfObj->lDelta * Size.cy;
		  UncompressedBits = Bits;
		  UncompressedFormat = Format;
	  }
  }
  if(UncompressedBits!=NULL)
  {
    SurfObj->pvBits = UncompressedBits;
  } else
  {
    if (SurfObj->cjBits == 0)
    {
      SurfObj->pvBits = NULL;
    }
    else
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
  }

  SurfObj->dhsurf = 0; // device managed surface
  SurfObj->hsurf  = 0;
  SurfObj->dhpdev = NULL;
  SurfObj->hdev = NULL;
  SurfObj->sizlBitmap = Size;
  SurfObj->iBitmapFormat = UncompressedFormat;
  SurfObj->iType = STYPE_BITMAP;
  SurfObj->fjBitmap = Flags & (BMF_TOPDOWN | BMF_NOZEROINIT);
  SurfObj->pvScan0 = SurfObj->pvBits;
  SurfObj->iUniq = 0;

  InitializeFuncs(SurfGDI, UncompressedFormat);

  // Use flags to determine bitmap type -- TOP_DOWN or whatever

  return NewBitmap;
}

/*
 * @unimplemented
 */
HSURF STDCALL
EngCreateDeviceSurface(IN DHSURF dhsurf,
		       IN SIZEL Size,
		       IN ULONG Format)
{
  HSURF   NewSurface;
  SURFOBJ *SurfObj;
  SURFGDI *SurfGDI;

  NewSurface = (HSURF)CreateGDIHandle(sizeof( SURFGDI ), sizeof( SURFOBJ ), (PVOID*)&SurfGDI, (PVOID*)&SurfObj);
  if( !ValidEngHandle( NewSurface ) )
	return 0;

  SurfGDI->BitsPerPixel = BitsPerFormat(Format);
  SurfObj->dhsurf = dhsurf;
  SurfObj->hsurf  = (HSURF) dhsurf; // FIXME: Is this correct??
  SurfObj->sizlBitmap = Size;
  SurfObj->iBitmapFormat = Format;
  SurfObj->lDelta = DIB_GetDIBWidthBytes(Size.cx, BitsPerFormat(Format));
  SurfObj->iType = STYPE_DEVICE;
  SurfObj->iUniq = 0;

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

/*
 * @implemented
 */
BOOL STDCALL
EngAssociateSurface(IN HSURF Surface,
		    IN HDEV Dev,
		    IN ULONG Hooks)
{
  SURFOBJ *SurfObj;
  SURFGDI *SurfGDI;
  GDIDEVICE* Device;

  Device = (GDIDEVICE*)Dev;

  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)Surface);
  ASSERT(SurfObj);
  SurfGDI = (SURFGDI*)AccessInternalObjectFromUserObject(SurfObj);

  // Associate the hdev
  SurfObj->hdev = Dev;
  SurfObj->dhpdev = Device->PDev;

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
  if(Hooks & HOOK_GRADIENTFILL)      SurfGDI->GradientFill      = Device->DriverFunctions.GradientFill;

  SurfGDI->CreateDeviceBitmap = Device->DriverFunctions.CreateDeviceBitmap;
  SurfGDI->SetPalette = Device->DriverFunctions.SetPalette;
  SurfGDI->MovePointer = Device->DriverFunctions.MovePointer;
  SurfGDI->SetPointerShape = (PFN_SetPointerShape)Device->DriverFunctions.SetPointerShape;

  SurfGDI->DriverLock = &Device->DriverLock;

  return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
EngModifySurface(
   IN HSURF hsurf,
   IN HDEV hdev,
   IN FLONG flHooks,
   IN FLONG flSurface,
   IN DHSURF dhsurf,
   OUT VOID *pvScan0,
   IN LONG lDelta,
   IN VOID *pvReserved)
{
   SURFOBJ *pso;

   pso = EngLockSurface(hsurf);
   if (pso == NULL)
   {
      return FALSE;
   }

   if (!EngAssociateSurface(hsurf, hdev, flHooks))
   {
      EngUnlockSurface(pso);

      return FALSE;
   }

   pso->dhsurf = dhsurf;
   pso->lDelta = lDelta;
   pso->pvScan0 = pvScan0;

   EngUnlockSurface(pso);

   return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
EngDeleteSurface(IN HSURF Surface)
{
  FreeGDIHandle((ULONG)Surface);
  return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
EngEraseSurface(SURFOBJ *Surface,
		RECTL *Rect,
		ULONG iColor)
{
  return FillSolid(Surface, Rect, iColor);
}

/*
 * @unimplemented
 */
SURFOBJ * STDCALL
EngLockSurface(IN HSURF Surface)
{
  /*
   * FIXME - don't know if GDIOBJ_LockObj's return value is a SURFOBJ or not...
   * also, what is HSURF's correct magic #?
   */
#ifdef TODO
  GDIOBJ_LockObj ( Surface, GDI_OBJECT_TYPE_DONTCARE );
#endif
  return (SURFOBJ*)AccessUserObject((ULONG)Surface);
}

/*
 * @unimplemented
 */
VOID STDCALL
EngUnlockSurface(IN SURFOBJ *Surface)
{
  /*
   * FIXME what is HSURF's correct magic #?
   */
#ifdef TODO
  GDIOBJ_UnlockObj ( Surface->hsurf, GDI_OBJECT_TYPE_DONTCARE );
#endif
}
/* EOF */
