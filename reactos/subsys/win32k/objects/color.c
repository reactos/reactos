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
/* $Id: color.c,v 1.49 2004/07/04 17:09:45 navaraf Exp $ */
#include <w32k.h>

// FIXME: Use PXLATEOBJ logicalToSystem instead of int *mapping

int COLOR_gapStart = 256;
int COLOR_gapEnd = -1;
int COLOR_gapFilled = 0;
int COLOR_max = 256;

#ifndef NO_MAPPING
static HPALETTE hPrimaryPalette = 0; // used for WM_PALETTECHANGED
#endif
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
  { 0xd4, 0xd0, 0xc7, PC_SYS_USED },

  // ... c_min/2 dynamic colorcells
  // ... gap (for sparse palettes)
  // ... c_min/2 dynamic colorcells

  { 0xff, 0xfb, 0xf0, PC_SYS_USED },
  { 0x3a, 0x6e, 0xa5, PC_SYS_USED },
  { 0x80, 0x80, 0x80, PC_SYS_USED },
  { 0xff, 0x00, 0x00, PC_SYS_USED },
  { 0x00, 0xff, 0x00, PC_SYS_USED },
  { 0xff, 0xff, 0x00, PC_SYS_USED },
  { 0x00, 0x00, 0xff, PC_SYS_USED },
  { 0xff, 0x00, 0xff, PC_SYS_USED },
  { 0x00, 0xff, 0xff, PC_SYS_USED },
  { 0xff, 0xff, 0xff, PC_SYS_USED }     // last 10
};

const PALETTEENTRY* FASTCALL COLOR_GetSystemPaletteTemplate(void)
{
   return (const PALETTEENTRY*)&COLOR_sysPalTemplate;
}

BOOL STDCALL NtGdiAnimatePalette(HPALETTE hPalette, UINT uStartIndex,
   UINT uEntries, CONST PPALETTEENTRY ppe)
{
   UNIMPLEMENTED;
   SetLastWin32Error(ERROR_CALL_NOT_IMPLEMENTED);
   return FALSE;
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
   if (NtGdiGetSystemPaletteEntries(hDC, 0, 256, Palette.aEntries) == 0)
   {
      return 0;
   }

   for (r = 0; r < 6; r++)
      for (g = 0; g < 6; g++)
         for (b = 0; b < 6; b++)
         {
            i = r + g*6 + b*36 + 10;
            Palette.aEntries[i].peRed = r * 51;
            Palette.aEntries[i].peGreen = g * 51;
            Palette.aEntries[i].peBlue = b * 51;
         }

   for (i = 216; i < 246; i++)
   {
      int v = (i - 216) << 3;
      Palette.aEntries[i].peRed = v;
      Palette.aEntries[i].peGreen = v;
      Palette.aEntries[i].peBlue = v;
   }

   return NtGdiCreatePalette((LOGPALETTE *)&Palette);
}

HPALETTE STDCALL NtGdiCreatePalette(CONST PLOGPALETTE palette)
{
  PPALGDI PalGDI;

  HPALETTE NewPalette = PALETTE_AllocPalette(
	  PAL_INDEXED,
	  palette->palNumEntries,
	  (PULONG)palette->palPalEntry,
	  0, 0, 0);

  PalGDI = (PPALGDI) PALETTE_LockPalette(NewPalette);

  PALETTE_ValidateFlags(PalGDI->IndexedColors, PalGDI->NumColors);
  PalGDI->logicalToSystem = NULL;

  PALETTE_UnlockPalette(NewPalette);

  return NewPalette;
}

BOOL STDCALL NtGdiGetColorAdjustment(HDC  hDC,
                             LPCOLORADJUSTMENT  ca)
{
  UNIMPLEMENTED;
}

unsigned short GetNumberOfBits(unsigned int dwMask)
{
   unsigned short wBits;
   for (wBits = 0; dwMask; dwMask = dwMask & (dwMask - 1))
      wBits++;
   return wBits;
}

COLORREF STDCALL NtGdiGetNearestColor(HDC hDC, COLORREF Color)
{
   COLORREF nearest = CLR_INVALID;
   PDC dc;
   PPALGDI palGDI;
   LONG RBits, GBits, BBits;

   dc = DC_LockDc(hDC);
   if (NULL != dc)
   {
      HPALETTE hpal = dc->w.hPalette;
      palGDI = (PPALGDI) PALETTE_LockPalette(hpal);
      if (!palGDI)
      {
         DC_UnlockDc(hDC);
         return nearest;
      }

      switch (palGDI->Mode)
      {
         case PAL_INDEXED:
            nearest = COLOR_LookupNearestColor(palGDI->IndexedColors,
               palGDI->NumColors, Color);
            break;
         case PAL_BGR:
         case PAL_RGB:
            nearest = Color;
            break;
         case PAL_BITFIELDS:
            RBits = 8 - GetNumberOfBits(palGDI->RedMask);
            GBits = 8 - GetNumberOfBits(palGDI->GreenMask);
            BBits = 8 - GetNumberOfBits(palGDI->BlueMask);
            nearest = RGB(
              (GetRValue(Color) >> RBits) << RBits,
              (GetGValue(Color) >> GBits) << GBits,
              (GetBValue(Color) >> BBits) << BBits);
            break;
      }
      PALETTE_UnlockPalette(hpal);
      DC_UnlockDc(hDC);
   }

   return nearest;
}

UINT STDCALL NtGdiGetNearestPaletteIndex(HPALETTE  hpal,
                                 COLORREF  Color)
{
  PPALGDI palGDI = (PPALGDI) PALETTE_LockPalette(hpal);
  UINT index  = 0;

  if (NULL != palGDI)
    {
      /* Return closest match for the given RGB color */
      index = COLOR_PaletteLookupPixel(palGDI->IndexedColors, palGDI->NumColors, NULL, Color, FALSE);
      PALETTE_UnlockPalette(hpal);
    }

  return index;
}

UINT STDCALL NtGdiGetPaletteEntries(HPALETTE  hpal,
                            UINT  StartIndex,
                            UINT  Entries,
                            LPPALETTEENTRY  pe)
{
  PPALGDI palGDI;
  UINT numEntries;

  palGDI = (PPALGDI) PALETTE_LockPalette(hpal);
  if (NULL == palGDI)
    {
      return 0;
    }

  numEntries = palGDI->NumColors;
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
      memcpy(pe, palGDI->IndexedColors + StartIndex, Entries * sizeof(PALETTEENTRY));
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
  /*
   * This function doesn't do any real work now and there's plenty
   * of bugd in it (calling SetPalette for high/true-color modes,
   * using DEFAULT_PALETTE instead of the device palette, ...).
   */
#if 1
  DPRINT1("NtGdiRealizePalette is unimplemented\n");
  return 0;
#else
  PALOBJ *palPtr, *sysPtr;
  PPALGDI palGDI, sysGDI;
  int realized = 0;
  PDC dc;
  HPALETTE systemPalette;
  SURFOBJ *SurfObj;
  BOOLEAN success;
  USHORT sysMode, palMode;

  dc = DC_LockDc(hDC);
  if (!dc)
  	return 0;

  SurfObj = (SURFOBJ*)AccessUserObject((ULONG)dc->Surface);
  systemPalette = NtGdiGetStockObject((INT)DEFAULT_PALETTE);
  palGDI = PALETTE_LockPalette(dc->w.hPalette);
  palPtr = (PALOBJ*) palGDI;

  // Step 1: Create mapping of system palette\DC palette
#ifndef NO_MAPPING
  realized = PALETTE_SetMapping(palPtr, 0, palGDI->NumColors,
               (dc->w.hPalette != hPrimaryPalette) ||
               (dc->w.hPalette == NtGdiGetStockObject(DEFAULT_PALETTE)));
#else
  realized = 0;
#endif

  sysGDI = PALETTE_LockPalette(systemPalette);
  sysPtr = (PALOBJ*) sysGDI;

  // Step 2:
  // The RealizePalette function modifies the palette for the device associated with the specified device context. If the
  // device context is a memory DC, the color table for the bitmap selected into the DC is modified. If the device
  // context is a display DC, the physical palette for that device is modified.
  if(dc->w.flags == DC_MEMORY)
  {
    // Memory managed DC
    DbgPrint("win32k: realizepalette unimplemented step 2 for DC_MEMORY");
  } else {
    if(GDIDEVFUNCS(SurfObj).SetPalette)
    {
      ASSERT(sysGDI->NumColors <= 256);
      success = GDIDEVFUNCS(SurfObj).SetPalette(
        dc->PDev, sysPtr, 0, 0, sysGDI->NumColors);
    }
  }

  // need to pass this to IntEngCreateXlate with palettes unlocked
  sysMode = sysGDI->Mode;
  palMode = palGDI->Mode;
  PALETTE_UnlockPalette(systemPalette);
  PALETTE_UnlockPalette(dc->w.hPalette);

  // Step 3: Create the XLATEOBJ for device managed DCs
  if(dc->w.flags != DC_MEMORY)
  {
    // Device managed DC
    palGDI->logicalToSystem = IntEngCreateXlate(sysMode, palMode, systemPalette, dc->w.hPalette);
  }

  DC_UnlockDc(hDC);

  return realized;
#endif
}

BOOL STDCALL NtGdiResizePalette(HPALETTE  hpal,
                        UINT  Entries)
{
/*  PALOBJ *palPtr = (PALOBJ*)AccessUserObject(hPal);
  UINT cPrevEnt, prevVer;
  INT prevsize, size = sizeof(LOGPALETTE) + (cEntries - 1) * sizeof(PALETTEENTRY);
  XLATEOBJ *XlateObj = NULL;

  if(!palPtr) return FALSE;
  cPrevEnt = palPtr->logpalette->palNumEntries;
  prevVer = palPtr->logpalette->palVersion;
  prevsize = sizeof(LOGPALETTE) + (cPrevEnt - 1) * sizeof(PALETTEENTRY) + sizeof(int*) + sizeof(GDIOBJHDR);
  size += sizeof(int*) + sizeof(GDIOBJHDR);
  XlateObj = palPtr->logicalToSystem;

  if (!(palPtr = GDI_ReallocObject(size, hPal, palPtr))) return FALSE;

  if(XlateObj)
  {
    XLATEOBJ *NewXlateObj = (int*) HeapReAlloc(GetProcessHeap(), 0, XlateObj, cEntries * sizeof(int));
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
  HPALETTE oldPal = NULL;
  PPALGDI PalGDI;

  // FIXME: mark the palette as a [fore\back]ground pal
  dc = DC_LockDc(hDC);
  if (NULL != dc)
    {
      /* Check if this is a valid palette handle */
      PalGDI = PALETTE_LockPalette(hpal);
      if (NULL != PalGDI)
	{
          /* Is this a valid palette for this depth? */
          if ((dc->w.bitsPerPixel <= 8 && PAL_INDEXED == PalGDI->Mode)
              || (8 < dc->w.bitsPerPixel && PAL_INDEXED != PalGDI->Mode))
            {
              PALETTE_UnlockPalette(hpal);
              oldPal = dc->w.hPalette;
              dc->w.hPalette = hpal;
            }
          else
            {
              PALETTE_UnlockPalette(hpal);
              oldPal = NULL;
            }
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
  PPALGDI palGDI;
  WORD numEntries;

  palGDI = PALETTE_LockPalette(hpal);
  if (!palGDI) return 0;

  numEntries = palGDI->NumColors;
  if (Start >= numEntries)
    {
      PALETTE_UnlockPalette(hpal);
      return 0;
    }
  if (numEntries < Start + Entries)
    {
      Entries = numEntries - Start;
    }
  memcpy(palGDI->IndexedColors + Start, pe, Entries * sizeof(PALETTEENTRY));
  PALETTE_ValidateFlags(palGDI->IndexedColors, palGDI->NumColors);
  ExFreePool(palGDI->logicalToSystem);
  palGDI->logicalToSystem = NULL;
  PALETTE_UnlockPalette(hpal);

  return Entries;
}

UINT STDCALL
NtGdiSetSystemPaletteUse(HDC hDC, UINT Usage)
{
   UNIMPLEMENTED;
}

BOOL STDCALL
NtGdiUnrealizeObject(HGDIOBJ hgdiobj)
{
   UNIMPLEMENTED;
}

BOOL STDCALL
NtGdiUpdateColors(HDC hDC)
{
   HWND hWnd;

   hWnd = (HWND)NtUserCallOneParam((DWORD)hDC, ONEPARAM_ROUTINE_WINDOWFROMDC);
   if (hWnd == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }
   return NtUserRedrawWindow(hWnd, NULL, 0, RDW_INVALIDATE);
}

INT STDCALL COLOR_PaletteLookupPixel(PALETTEENTRY *palPalEntry, INT size,
                             XLATEOBJ *XlateObj, COLORREF col, BOOL skipReserved)
{
  int i, best = 0, diff = 0x7fffffff;
  int r, g, b;

  for( i = 0; i < size && diff ; i++ )
  {
#if 0
    if(!(palPalEntry[i].peFlags & PC_SYS_USED) || (skipReserved && palPalEntry[i].peFlags  & PC_SYS_RESERVED))
      continue;
#endif

    r = abs((SHORT)palPalEntry[i].peRed - GetRValue(col));
    g = abs((SHORT)palPalEntry[i].peGreen - GetGValue(col));
    b = abs((SHORT)palPalEntry[i].peBlue - GetBValue(col));

    r = r*r + g*g + b*b;

    if( r < diff ) { best = i; diff = r; }
  }

  if (XlateObj == NULL)
    return best;
  else
    return (XlateObj->pulXlate) ? (INT)XlateObj->pulXlate[best] : best;
}

COLORREF STDCALL COLOR_LookupNearestColor( PALETTEENTRY* palPalEntry, int size, COLORREF color )
{
   INT index;

   index = COLOR_PaletteLookupPixel(palPalEntry, size, NULL, color, FALSE);
   return RGB(
      palPalEntry[index].peRed,
      palPalEntry[index].peGreen,
      palPalEntry[index].peBlue);
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
