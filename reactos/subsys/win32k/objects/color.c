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
/* $Id: color.c,v 1.23 2003/08/28 12:35:59 gvg Exp $ */

// FIXME: Use PXLATEOBJ logicalToSystem instead of int *mapping

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <win32k/brush.h>
#include <win32k/dc.h>
#include <win32k/color.h>
#include <win32k/pen.h>
#include "../eng/handle.h"
#include <include/inteng.h>
#include <include/color.h>
#include <include/palette.h>

#define NDEBUG
#include <win32k/debug1.h>

int COLOR_gapStart = 256;
int COLOR_gapEnd = -1;
int COLOR_gapFilled = 0;
int COLOR_max = 256;

static HPALETTE hPrimaryPalette = 0; // used for WM_PALETTECHANGED
//static HPALETTE hLastRealizedPalette = 0; // UnrealizeObject() needs it

const PALETTEENTRY COLOR_sysPalTemplate[NB_RESERVED_COLORS] =
{
  // first 10 entries in the system palette
  // red  green blue  flags
  { 0x00, 0x00, 0x00, PC_SYS_USED },
  { 0x80, 0x00, 0x00, PC_SYS_USED },
  { 0x00, 0x80, 0x00, PC_SYS_USED },
  { 0x80, 0x80, 0x00, PC_SYS_USED },
  { 0x00, 0x00, 0x80, PC_SYS_USED },
  { 0x80, 0x00, 0x80, PC_SYS_USED },
  { 0x00, 0x80, 0x80, PC_SYS_USED },
  { 0xc0, 0xc0, 0xc0, PC_SYS_USED },
  { 0xc0, 0xdc, 0xc0, PC_SYS_USED },
  { 0xa6, 0xca, 0xf0, PC_SYS_USED },

  // ... c_min/2 dynamic colorcells
  // ... gap (for sparse palettes)
  // ... c_min/2 dynamic colorcells

  { 0xff, 0xfb, 0xf0, PC_SYS_USED },
  { 0xa0, 0xa0, 0xa4, PC_SYS_USED },
  { 0x80, 0x80, 0x80, PC_SYS_USED },
  { 0xff, 0x00, 0x00, PC_SYS_USED },
  { 0x00, 0xff, 0x00, PC_SYS_USED },
  { 0xff, 0xff, 0x00, PC_SYS_USED },
  { 0x00, 0x00, 0xff, PC_SYS_USED },
  { 0xff, 0x00, 0xff, PC_SYS_USED },
  { 0x00, 0xff, 0xff, PC_SYS_USED },
  { 0xff, 0xff, 0xff, PC_SYS_USED }     // last 10
};

ULONG FASTCALL NtGdiGetSysColor(int nIndex)
{
   const PALETTEENTRY *p = COLOR_sysPalTemplate + (nIndex * sizeof(PALETTEENTRY));
   return RGB(p->peRed, p->peGreen, p->peBlue);
}

HPEN STDCALL NtGdiGetSysColorPen(int nIndex)
{
  COLORREF Col;
  memcpy(&Col, COLOR_sysPalTemplate + nIndex, sizeof(COLORREF));
  return(NtGdiCreatePen(PS_SOLID, 1, Col));
}

HBRUSH STDCALL NtGdiGetSysColorBrush(int nIndex)
{
  COLORREF Col;
  memcpy(&Col, COLOR_sysPalTemplate + nIndex, sizeof(COLORREF));
  return(NtGdiCreateSolidBrush(Col));
}



const PALETTEENTRY* FASTCALL COLOR_GetSystemPaletteTemplate(void)
{
  return (const PALETTEENTRY*)&COLOR_sysPalTemplate;
}

BOOL STDCALL NtGdiAnimatePalette(HPALETTE  hpal,
                         UINT  StartIndex,
                         UINT  Entries,
                         CONST PPALETTEENTRY  ppe)
{
/*
  if( hPal != NtGdiGetStockObject(DEFAULT_PALETTE) )
  {
    PALETTEOBJ* palPtr = (PALETTEOBJ *)GDI_GetObjPtr(hPal, PALETTE_MAGIC);
    if (!palPtr) return FALSE;

    if( (StartIndex + NumEntries) <= palPtr->logpalette.palNumEntries )
    {
      UINT u;
      for( u = 0; u < NumEntries; u++ )
        palPtr->logpalette.palPalEntry[u + StartIndex] = PaletteColors[u];
      PALETTE_Driver->pSetMapping(palPtr, StartIndex, NumEntries, hPal != hPrimaryPalette );
      GDI_ReleaseObj(hPal);
      return TRUE;
    }
    GDI_ReleaseObj(hPal);
  }
  return FALSE;
*/
  UNIMPLEMENTED;
}

HPALETTE STDCALL NtGdiCreateHalftonePalette(HDC  hDC)
{
  int i, r, g, b;
  struct {
    WORD Version;
    WORD NumberOfEntries;
    PALETTEENTRY aEntries[256];
  } Palette;

  Palette.Version = 0x300;
  Palette.NumberOfEntries = 256;
  NtGdiGetSystemPaletteEntries(hDC, 0, 256, Palette.aEntries);

  for (r = 0; r < 6; r++) {
    for (g = 0; g < 6; g++) {
      for (b = 0; b < 6; b++) {
        i = r + g*6 + b*36 + 10;
        Palette.aEntries[i].peRed = r * 51;
        Palette.aEntries[i].peGreen = g * 51;
        Palette.aEntries[i].peBlue = b * 51;
      }
     }
   }

  for (i = 216; i < 246; i++) {
    int v = (i - 216) * 8;
    Palette.aEntries[i].peRed = v;
    Palette.aEntries[i].peGreen = v;
    Palette.aEntries[i].peBlue = v;
  }

  return NtGdiCreatePalette((LOGPALETTE *)&Palette);
}

HPALETTE STDCALL NtGdiCreatePalette(CONST PLOGPALETTE palette)
{
  PPALOBJ  PalObj;

  HPALETTE NewPalette = PALETTE_AllocPalette(
	  PAL_INDEXED,
	  palette->palNumEntries,
	  (PULONG)palette->palPalEntry,
	  0, 0, 0);
  ULONG size;

  PalObj = (PPALOBJ) PALETTE_LockPalette(NewPalette);

  size = sizeof(LOGPALETTE) + (palette->palNumEntries * sizeof(PALETTEENTRY));
  PalObj->logpalette = ExAllocatePool(NonPagedPool, size);
  memcpy(PalObj->logpalette, palette, size);
  PALETTE_ValidateFlags(PalObj->logpalette->palPalEntry, PalObj->logpalette->palNumEntries);
  PalObj->logicalToSystem = NULL;

  PALETTE_UnlockPalette(NewPalette);

  return NewPalette;
}

BOOL STDCALL NtGdiGetColorAdjustment(HDC  hDC,
                             LPCOLORADJUSTMENT  ca)
{
  UNIMPLEMENTED;
}

COLORREF STDCALL NtGdiGetNearestColor(HDC  hDC,
                              COLORREF  Color)
{
  COLORREF nearest = CLR_INVALID;
  PDC dc;
  PPALOBJ palObj;

  dc = DC_LockDc(hDC);
  if (NULL != dc)
    {
      HPALETTE hpal = (dc->w.hPalette) ? dc->w.hPalette : NtGdiGetStockObject(DEFAULT_PALETTE);
      palObj = (PPALOBJ) PALETTE_LockPalette(hpal);
      if (!palObj)
	{
	  DC_UnlockDc(hDC);
	  return nearest;
	}

      nearest = COLOR_LookupNearestColor(palObj->logpalette->palPalEntry,
                                         palObj->logpalette->palNumEntries, Color);
      PALETTE_UnlockPalette(hpal);
      DC_UnlockDc( hDC );
    }

  return nearest;
}

UINT STDCALL NtGdiGetNearestPaletteIndex(HPALETTE  hpal,
                                 COLORREF  Color)
{
  PPALOBJ palObj = (PPALOBJ) PALETTE_LockPalette(hpal);
  UINT index  = 0;

  if (NULL != palObj)
    {
      /* Return closest match for the given RGB color */
      index = COLOR_PaletteLookupPixel(palObj->logpalette->palPalEntry, palObj->logpalette->palNumEntries, NULL, Color, FALSE);
      PALETTE_UnlockPalette(hpal);
    }

  return index;
}

UINT STDCALL NtGdiGetPaletteEntries(HPALETTE  hpal,
                            UINT  StartIndex,
                            UINT  Entries,
                            LPPALETTEENTRY  pe)
{
  PPALOBJ palPtr;
  UINT numEntries;

  palPtr = (PPALOBJ) PALETTE_LockPalette(hpal);
  if (NULL == palPtr)
    {
      return 0;
    }

  numEntries = palPtr->logpalette->palNumEntries;
  if (numEntries < StartIndex + Entries)
    {
      Entries = numEntries - StartIndex;
    }
  if (NULL != pe)
    {
      if (numEntries <= StartIndex)
	{
	  PALETTE_UnlockPalette(hpal);
	  return 0;
	}
      memcpy(pe, &palPtr->logpalette->palPalEntry[StartIndex], Entries * sizeof(PALETTEENTRY));
      for (numEntries = 0; numEntries < Entries; numEntries++)
	{
	  if (pe[numEntries].peFlags & 0xF0)
	    {
	      pe[numEntries].peFlags = 0;
	    }
	}
    }

  PALETTE_UnlockPalette(hpal);
  return Entries;
}

UINT STDCALL NtGdiGetSystemPaletteEntries(HDC  hDC,
                                  UINT  StartIndex,
                                  UINT  Entries,
                                  LPPALETTEENTRY  pe)
{
  //UINT i;
  //PDC dc;
/*
  if (!(dc = AccessUserObject(hdc))) return 0;

  if (!pe)
  {
    Entries = dc->GDIInfo->ulNumPalReg;
    goto done;
  }

  if (StartIndex >= dc->GDIInfo->ulNumPalReg)
  {
    Entries = 0;
    goto done;
  }

  if (StartIndex + Entries >= dc->GDIInfo->ulNumPalReg) Entries = dc->GDIInfo->ulNumPalReg - StartIndex;

  for (i = 0; i < Entries; i++)
  {
    *(COLORREF*)(entries + i) = COLOR_GetSystemPaletteEntry(StartIndex + i);
  }

  done:
//    GDI_ReleaseObj(hdc);
  return count; */
  // FIXME UNIMPLEMENTED;
  return 0;
}

UINT STDCALL NtGdiGetSystemPaletteUse(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

/*!
The RealizePalette function modifies the palette for the device associated with the specified device context. If the device context is a memory DC, the color table for the bitmap selected into the DC is modified. If the device context is a display DC, the physical palette for that device is modified.

A logical palette is a buffer between color-intensive applications and the system, allowing these applications to use as many colors as needed without interfering with colors displayed by other windows.

1= IF DRAWING TO A DEVICE
-- If it is a paletted bitmap, and is not an identity palette, then an XLATEOBJ is created between the logical palette and
   the system palette.
-- If it is an RGB palette, then an XLATEOBJ is created between the RGB values and the system palette.

2= IF DRAWING TO A MEMORY DC\BITMAP
-- If it is a paletted bitmap, and is not an identity palette, then an XLATEOBJ is created between the logical palette and
   the dc palette.
-- If it is an RGB palette, then an XLATEOBJ is created between the RGB values and the dc palette.
*/
UINT STDCALL NtGdiRealizePalette(HDC hDC)
{
  PPALOBJ palPtr, sysPtr;
  PPALGDI palGDI, sysGDI;
  int realized = 0;
  PDC dc;
  HPALETTE systemPalette;
  PSURFGDI SurfGDI;
  BOOLEAN success;

  dc = DC_LockDc(hDC);
  if (!dc)
  	return 0;

  SurfGDI = (PSURFGDI)AccessInternalObject((ULONG)dc->Surface);
  systemPalette = NtGdiGetStockObject((INT)DEFAULT_PALETTE);
  palGDI = PALETTE_LockPalette(dc->w.hPalette);
  palPtr = (PPALOBJ) palGDI;
  sysGDI = PALETTE_LockPalette(systemPalette);
  sysPtr = (PPALOBJ) sysGDI;

  // Step 1: Create mapping of system palette\DC palette
  realized = PALETTE_SetMapping(palPtr, 0, palPtr->logpalette->palNumEntries,
               (dc->w.hPalette != hPrimaryPalette) ||
               (dc->w.hPalette == NtGdiGetStockObject(DEFAULT_PALETTE)));

  // Step 2:
  // The RealizePalette function modifies the palette for the device associated with the specified device context. If the
  // device context is a memory DC, the color table for the bitmap selected into the DC is modified. If the device
  // context is a display DC, the physical palette for that device is modified.
  if(dc->w.flags == DC_MEMORY)
  {
    // Memory managed DC
    DbgPrint("win32k: realizepalette unimplemented step 2 for DC_MEMORY");
  } else {
    if(SurfGDI->SetPalette)
    {
      success = SurfGDI->SetPalette(dc->PDev, sysPtr, 0, 0, sysPtr->logpalette->palNumEntries);
    }
  }

  // Step 3: Create the XLATEOBJ for device managed DCs
  if(dc->w.flags != DC_MEMORY)
  {
    // Device managed DC
    palPtr->logicalToSystem = IntEngCreateXlate(sysGDI->Mode, palGDI->Mode, systemPalette, dc->w.hPalette);
  }

  PALETTE_UnlockPalette(systemPalette);
  PALETTE_UnlockPalette(dc->w.hPalette);
  DC_UnlockDc(hDC);

  return realized;
}

BOOL STDCALL NtGdiResizePalette(HPALETTE  hpal,
                        UINT  Entries)
{
/*  PPALOBJ palPtr = (PPALOBJ)AccessUserObject(hPal);
  UINT cPrevEnt, prevVer;
  INT prevsize, size = sizeof(LOGPALETTE) + (cEntries - 1) * sizeof(PALETTEENTRY);
  PXLATEOBJ XlateObj = NULL;

  if(!palPtr) return FALSE;
  cPrevEnt = palPtr->logpalette->palNumEntries;
  prevVer = palPtr->logpalette->palVersion;
  prevsize = sizeof(LOGPALETTE) + (cPrevEnt - 1) * sizeof(PALETTEENTRY) + sizeof(int*) + sizeof(GDIOBJHDR);
  size += sizeof(int*) + sizeof(GDIOBJHDR);
  XlateObj = palPtr->logicalToSystem;

  if (!(palPtr = GDI_ReallocObject(size, hPal, palPtr))) return FALSE;

  if(XlateObj)
  {
    PXLATEOBJ NewXlateObj = (int*) HeapReAlloc(GetProcessHeap(), 0, XlateObj, cEntries * sizeof(int));
    if(NewXlateObj == NULL)
    {
      ERR("Can not resize logicalToSystem -- out of memory!");
      GDI_ReleaseObj( hPal );
      return FALSE;
    }
    palPtr->logicalToSystem = NewXlateObj;
  }

  if(cEntries > cPrevEnt)
  {
    if(XlateObj) memset(palPtr->logicalToSystem + cPrevEnt, 0, (cEntries - cPrevEnt)*sizeof(int));
    memset( (BYTE*)palPtr + prevsize, 0, size - prevsize );
    PALETTE_ValidateFlags((PALETTEENTRY*)((BYTE*)palPtr + prevsize), cEntries - cPrevEnt );
  }
  palPtr->logpalette->palNumEntries = cEntries;
  palPtr->logpalette->palVersion = prevVer;
//    GDI_ReleaseObj( hPal );
  return TRUE; */

  UNIMPLEMENTED;
}

/*!
 * Select logical palette into device context.
 * \param	hDC 				handle to the device context
 * \param	hpal				handle to the palette
 * \param	ForceBackground 	If this value is FALSE the logical palette will be copied to the device palette only when the applicatioon
 * 								is in the foreground. If this value is TRUE then map the colors in the logical palette to the device
 * 								palette colors in the best way.
 * \return	old palette
 *
 * \todo	implement ForceBackground == TRUE
*/
HPALETTE STDCALL NtGdiSelectPalette(HDC  hDC,
                            HPALETTE  hpal,
                            BOOL  ForceBackground)
{
  PDC dc;
  HPALETTE oldPal;
  PPALGDI PalGDI;

  // FIXME: mark the palette as a [fore\back]ground pal
  dc = DC_LockDc(hDC);
  if (NULL != dc)
    {
      /* Check if this is a valid palette handle */
      PalGDI = PALETTE_LockPalette(hpal);
      if (NULL != PalGDI)
	{
	  PALETTE_UnlockPalette(hpal);
	  oldPal = dc->w.hPalette;
	  dc->w.hPalette = hpal;
	}
      else
	{
	  oldPal = NULL;
	}
      DC_UnlockDc(hDC);
    }

  return oldPal;
}

BOOL STDCALL NtGdiSetColorAdjustment(HDC  hDC,
                             CONST LPCOLORADJUSTMENT  ca)
{
  UNIMPLEMENTED;
}

UINT STDCALL NtGdiSetPaletteEntries(HPALETTE  hpal,
                            UINT  Start,
                            UINT  Entries,
                            CONST LPPALETTEENTRY  pe)
{
  PPALOBJ palPtr;
  WORD numEntries;

  palPtr = (PPALOBJ)PALETTE_LockPalette(hpal);
  if (!palPtr) return 0;

  numEntries = palPtr->logpalette->palNumEntries;
  if (Start >= numEntries)
    {
      PALETTE_UnlockPalette(hpal);
      return 0;
    }
  if (numEntries < Start + Entries)
    {
      Entries = numEntries - Start;
    }
  memcpy(&palPtr->logpalette->palPalEntry[Start], pe, Entries * sizeof(PALETTEENTRY));
  PALETTE_ValidateFlags(palPtr->logpalette->palPalEntry, palPtr->logpalette->palNumEntries);
  ExFreePool(palPtr->logicalToSystem);
  palPtr->logicalToSystem = NULL;
  PALETTE_UnlockPalette(hpal);

  return Entries;
}

UINT STDCALL NtGdiSetSystemPaletteUse(HDC  hDC,
                              UINT  Usage)
{
  UNIMPLEMENTED;
}

BOOL STDCALL NtGdiUnrealizeObject(HGDIOBJ  hgdiobj)
{
  UNIMPLEMENTED;
}

BOOL STDCALL NtGdiUpdateColors(HDC  hDC)
{
  //PDC dc;
  //HWND hWnd;
  //int size;
/*
  if (!(dc = AccessUserObject(hDC))) return 0;
  size = dc->GDIInfo->ulNumPalReg;
//  GDI_ReleaseObj( hDC );

  if (Callout.WindowFromDC)
  {
    hWnd = Callout.WindowFromDC( hDC );

    // Docs say that we have to remap current drawable pixel by pixel
    // but it would take forever given the speed of XGet/PutPixel.
    if (hWnd && size) Callout.RedrawWindow( hWnd, NULL, 0, RDW_INVALIDATE );
  } */
  // FIXME UNIMPLEMENTED
  return 0x666;
}

INT STDCALL COLOR_PaletteLookupPixel(PALETTEENTRY *palPalEntry, INT size,
                             PXLATEOBJ XlateObj, COLORREF col, BOOL skipReserved)
{
  int i, best = 0, diff = 0x7fffffff;
  int r, g, b;

  for( i = 0; i < size && diff ; i++ )
  {
    if(!(palPalEntry[i].peFlags & PC_SYS_USED) || (skipReserved && palPalEntry[i].peFlags  & PC_SYS_RESERVED))
      continue;

    r = palPalEntry[i].peRed - GetRValue(col);
    g = palPalEntry[i].peGreen - GetGValue(col);
    b = palPalEntry[i].peBlue - GetBValue(col);

    r = r*r + g*g + b*b;

    if( r < diff ) { best = i; diff = r; }
  }
  return (XlateObj->pulXlate) ? (INT)XlateObj->pulXlate[best] : best;
}

COLORREF STDCALL COLOR_LookupNearestColor( PALETTEENTRY* palPalEntry, int size, COLORREF color )
{
  unsigned char spec_type = color >> 24;
  int i;
  PALETTEENTRY *COLOR_sysPal = (PALETTEENTRY*)ReturnSystemPalette();

  // we need logical palette for PALETTERGB and PALETTEINDEX colorrefs

  if( spec_type == 2 ) /* PALETTERGB */
    color = *(COLORREF*)(palPalEntry + COLOR_PaletteLookupPixel(palPalEntry,size,NULL,color,FALSE));

  else if( spec_type == 1 ) /* PALETTEINDEX */
  {
    if( (i = color & 0x0000ffff) >= size )
    {
      DbgPrint("RGB(%lx) : idx %d is out of bounds, assuming NULL\n", color, i);
      color = *(COLORREF*)palPalEntry;
    }
    else color = *(COLORREF*)(palPalEntry + i);
  }

  color &= 0x00ffffff;
  return (0x00ffffff & *(COLORREF*)(COLOR_sysPal + COLOR_PaletteLookupPixel(COLOR_sysPal, 256, NULL, color, FALSE)));
}

int STDCALL COLOR_PaletteLookupExactIndex( PALETTEENTRY* palPalEntry, int size,
                                   COLORREF col )
{
  int i;
  BYTE r = GetRValue(col), g = GetGValue(col), b = GetBValue(col);
  for( i = 0; i < size; i++ )
  {
    if( palPalEntry[i].peFlags & PC_SYS_USED )  /* skips gap */
      if(palPalEntry[i].peRed == r && palPalEntry[i].peGreen == g && palPalEntry[i].peBlue == b) return i;
  }
  return -1;
}
/* EOF */
