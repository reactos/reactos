// FIXME: Use PXLATEOBJ logicalToSystem instead of int *mapping

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <win32k/dc.h>
#include <win32k/color.h>
#include "../eng/handle.h"

// #define NDEBUG
#include <win32k/debug1.h>

int COLOR_gapStart = 256;
int COLOR_gapEnd = -1;
int COLOR_gapFilled = 0;
int COLOR_max = 256;

static HPALETTE hPrimaryPalette = 0; // used for WM_PALETTECHANGED
static HPALETTE hLastRealizedPalette = 0; // UnrealizeObject() needs it

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

//forward declarations
COLORREF COLOR_LookupNearestColor( PALETTEENTRY* palPalEntry, int size, COLORREF color );


const PALETTEENTRY* COLOR_GetSystemPaletteTemplate(void)
{
  return (const PALETTEENTRY*)&COLOR_sysPalTemplate;
}

BOOL STDCALL W32kAnimatePalette(HPALETTE  hpal,
                         UINT  StartIndex,
                         UINT  Entries,
                         CONST PPALETTEENTRY  ppe)
{
/*
  if( hPal != W32kGetStockObject(DEFAULT_PALETTE) )
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

HPALETTE STDCALL W32kCreateHalftonePalette(HDC  hDC)
{
  int i, r, g, b;
  struct {
    WORD Version;
    WORD NumberOfEntries;
    PALETTEENTRY aEntries[256];
  } Palette;

  Palette.Version = 0x300;
  Palette.NumberOfEntries = 256;
  W32kGetSystemPaletteEntries(hDC, 0, 256, Palette.aEntries);

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

  return W32kCreatePalette((LOGPALETTE *)&Palette);
}

HPALETTE STDCALL W32kCreatePalette(CONST PLOGPALETTE palette)
{
  PPALOBJ  PalObj;

  HPALETTE NewPalette = (HPALETTE)EngCreatePalette(PAL_INDEXED, palette->palNumEntries, (PULONG*) palette->palPalEntry, 0, 0, 0);
  ULONG size;

  PalObj = (PPALOBJ)AccessUserObject(NewPalette);

  size = sizeof(LOGPALETTE) + (palette->palNumEntries * sizeof(PALETTEENTRY));
  PalObj->logpalette = ExAllocatePool(NonPagedPool, size);
  memcpy(PalObj->logpalette, palette, size);
  PALETTE_ValidateFlags(PalObj->logpalette->palPalEntry, PalObj->logpalette->palNumEntries);
  PalObj->logicalToSystem = NULL;

  return NewPalette;
}

BOOL STDCALL W32kGetColorAdjustment(HDC  hDC,
                             LPCOLORADJUSTMENT  ca)
{
  UNIMPLEMENTED;
}

COLORREF STDCALL W32kGetNearestColor(HDC  hDC,
                              COLORREF  Color)
{
  COLORREF nearest = CLR_INVALID;
  PDC dc;
  PPALOBJ palObj;

  if( (dc = DC_HandleToPtr(hDC) ) )
  {
    HPALETTE hpal = (dc->w.hPalette)? dc->w.hPalette : W32kGetStockObject(DEFAULT_PALETTE);
    palObj = (PPALOBJ)AccessUserObject(hpal);
    if (!palObj) {
//      GDI_ReleaseObj(hdc);
      return nearest;
    }

    nearest = COLOR_LookupNearestColor(palObj->logpalette->palPalEntry,
                                       palObj->logpalette->palNumEntries, Color);
	// FIXME: release hpal!!
//    GDI_ReleaseObj( hpal );
    DC_ReleasePtr( hDC );
  }

  return nearest;
}

UINT STDCALL W32kGetNearestPaletteIndex(HPALETTE  hpal,
                                 COLORREF  Color)
{
  PPALOBJ     palObj = (PPALOBJ)AccessUserObject(hpal);
  UINT index  = 0;

  if( palObj )
  {
    // Return closest match for the given RGB color
    index = COLOR_PaletteLookupPixel(palObj->logpalette->palPalEntry, palObj->logpalette->palNumEntries, NULL, Color, FALSE);
//    GDI_ReleaseObj( hpalette );
  }

  return index;
}

UINT STDCALL W32kGetPaletteEntries(HPALETTE  hpal,
                            UINT  StartIndex,
                            UINT  Entries,
                            LPPALETTEENTRY  pe)
{
  PPALOBJ palPtr;
  UINT numEntries;

  palPtr = (PPALOBJ)AccessUserObject(hpal);
  if (!palPtr) return 0;

  numEntries = palPtr->logpalette->palNumEntries;
  if (StartIndex + Entries > numEntries) Entries = numEntries - StartIndex;
  if (pe)
  {
    if (StartIndex >= numEntries)
    {
//      GDI_ReleaseObj( hpalette );
      return 0;
    }
    memcpy(pe, &palPtr->logpalette->palPalEntry[StartIndex], Entries * sizeof(PALETTEENTRY));
    for(numEntries = 0; numEntries < Entries ; numEntries++)
      if (pe[numEntries].peFlags & 0xF0)
        pe[numEntries].peFlags = 0;
  }

//  GDI_ReleaseObj( hpalette );
  return Entries;
}

UINT STDCALL W32kGetSystemPaletteEntries(HDC  hDC,
                                  UINT  StartIndex,
                                  UINT  Entries,
                                  LPPALETTEENTRY  pe)
{
  UINT i;
  PDC dc;
/*
  if (!(dc = AccessUserObject(hdc))) return 0;

  if (!pe)
  {
    Entries = dc->devCaps->sizePalette;
    goto done;
  }

  if (StartIndex >= dc->devCaps->sizePalette)
  {
    Entries = 0;
    goto done;
  }

  if (StartIndex + Entries >= dc->devCaps->sizePalette) Entries = dc->devCaps->sizePalette - StartIndex;

  for (i = 0; i < Entries; i++)
  {
    *(COLORREF*)(entries + i) = COLOR_GetSystemPaletteEntry(StartIndex + i);
  }

  done:
//    GDI_ReleaseObj(hdc);
  return count; */
}

UINT STDCALL W32kGetSystemPaletteUse(HDC  hDC)
{
  UNIMPLEMENTED;
}

UINT STDCALL W32kRealizePalette(HDC  hDC)
/*
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
{
  PPALOBJ palPtr, sysPtr;
  PPALGDI palGDI, sysGDI;
  int realized = 0;
  PDC dc = (PDC)AccessUserObject(hDC);
  HPALETTE systemPalette;
  PSURFGDI SurfGDI;
  BOOLEAN success;

  if (!dc) return 0;

  palPtr = (PPALOBJ)AccessUserObject(dc->w.hPalette);
  SurfGDI = (PSURFGDI)AccessInternalObjectFromUserObject(dc->Surface);
  systemPalette = W32kGetStockObject(STOCK_DEFAULT_PALETTE);
  sysPtr = (PPALOBJ)AccessInternalObject(systemPalette);
  palGDI = (PPALGDI)AccessInternalObject(dc->w.hPalette);
  sysGDI = (PPALGDI)AccessInternalObject(systemPalette);

  // Step 1: Create mapping of system palette\DC palette
  realized = PALETTE_SetMapping(palPtr, 0, palPtr->logpalette->palNumEntries,
               (dc->w.hPalette != hPrimaryPalette) ||
               (dc->w.hPalette == W32kGetStockObject(DEFAULT_PALETTE)));

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
    palPtr->logicalToSystem = EngCreateXlate(sysGDI->Mode, palGDI->Mode, systemPalette, dc->w.hPalette);
  }

//  GDI_ReleaseObj(dc->w.hPalette);
//  GDI_ReleaseObj(hdc);

  return realized;
}

BOOL STDCALL W32kResizePalette(HPALETTE  hpal,
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

HPALETTE STDCALL W32kSelectPalette(HDC  hDC,
                            HPALETTE  hpal,
                            BOOL  ForceBackground)
{
  PDC dc = (PDC)AccessUserObject(hDC);
  HPALETTE oldPal;

  oldPal = dc->w.hPalette;
  dc->w.hPalette = hpal;

  // FIXME: mark the palette as a [fore\back]ground pal

  return oldPal;
}

BOOL STDCALL W32kSetColorAdjustment(HDC  hDC,
                             CONST LPCOLORADJUSTMENT  ca)
{
  UNIMPLEMENTED;
}

UINT STDCALL W32kSetPaletteEntries(HPALETTE  hpal,
                            UINT  Start,
                            UINT  Entries,
                            CONST LPPALETTEENTRY  pe)
{
  PPALOBJ palPtr;
  INT numEntries;

  palPtr = (PPALOBJ)AccessUserObject(hpal);
  if (!palPtr) return 0;

  numEntries = palPtr->logpalette->palNumEntries;
  if (Start >= numEntries)
  {
//    GDI_ReleaseObj( hpalette );
    return 0;
  }
  if (Start + Entries > numEntries) Entries = numEntries - Start;
  memcpy(&palPtr->logpalette->palPalEntry[Start], pe, Entries * sizeof(PALETTEENTRY));
  PALETTE_ValidateFlags(palPtr->logpalette->palPalEntry, palPtr->logpalette->palNumEntries);
  ExFreePool(palPtr->logicalToSystem);
  palPtr->logicalToSystem = NULL;
//  GDI_ReleaseObj( hpalette );
  return Entries;
}

UINT STDCALL W32kSetSystemPaletteUse(HDC  hDC,
                              UINT  Usage)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kUnrealizeObject(HGDIOBJ  hgdiobj)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kUpdateColors(HDC  hDC)
{
  PDC dc;
  HWND hWnd;
  int size;
/*
  if (!(dc = AccessUserObject(hDC))) return 0;
  size = dc->devCaps->sizePalette;
//  GDI_ReleaseObj( hDC );

  if (Callout.WindowFromDC)
  {
    hWnd = Callout.WindowFromDC( hDC );

    // Docs say that we have to remap current drawable pixel by pixel
    // but it would take forever given the speed of XGet/PutPixel.
    if (hWnd && size) Callout.RedrawWindow( hWnd, NULL, 0, RDW_INVALIDATE );
  } */
  return 0x666;
}

int COLOR_PaletteLookupPixel(PALETTEENTRY *palPalEntry, int size,
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
  return (XlateObj->pulXlate) ? XlateObj->pulXlate[best] : best;
}

COLORREF COLOR_LookupNearestColor( PALETTEENTRY* palPalEntry, int size, COLORREF color )
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

int COLOR_PaletteLookupExactIndex( PALETTEENTRY* palPalEntry, int size,
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
