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
/* $Id: palette.c,v 1.19 2004/06/20 00:45:37 navaraf Exp $ */
#include <w32k.h>

#ifndef NO_MAPPING
static int           PALETTE_firstFree = 0; 
static unsigned char PALETTE_freeList[256];
#endif

int PALETTE_PaletteFlags     = 0;
PALETTEENTRY *COLOR_sysPal   = NULL;
int COLOR_gapStart;
int COLOR_gapEnd;
int COLOR_gapFilled;
int COLOR_max;

PPALETTEENTRY FASTCALL ReturnSystemPalette (VOID)
{
  return COLOR_sysPal;
}

static BOOL FASTCALL
PALETTE_InternalDelete(PPALGDI Palette)
{
  if (NULL != Palette->IndexedColors)
    {
      ExFreePool(Palette->IndexedColors);
    }

  return TRUE;
}

HPALETTE FASTCALL
PALETTE_AllocPalette(ULONG Mode,
                     ULONG NumColors,
                     ULONG *Colors,
                     ULONG Red,
                     ULONG Green,
                     ULONG Blue)
{
  HPALETTE NewPalette;
  PPALGDI PalGDI;

  NewPalette = (HPALETTE) GDIOBJ_AllocObj(sizeof(PALGDI), GDI_OBJECT_TYPE_PALETTE, (GDICLEANUPPROC) PALETTE_InternalDelete);
  if (NULL == NewPalette)
    {
      return NULL;
    }

  PalGDI = PALETTE_LockPalette(NewPalette);
  ASSERT( PalGDI );

  PalGDI->Self = NewPalette;
  PalGDI->Mode = Mode;

  if (NULL != Colors)
    {
      PalGDI->IndexedColors = ExAllocatePoolWithTag(PagedPool, sizeof(PALETTEENTRY) * NumColors, TAG_PALETTE);
      if (NULL == PalGDI->IndexedColors)
	{
	  PALETTE_UnlockPalette(NewPalette);
	  PALETTE_FreePalette(NewPalette);
	  return NULL;
	}
      RtlCopyMemory(PalGDI->IndexedColors, Colors, sizeof(PALETTEENTRY) * NumColors);
    }

  if (PAL_INDEXED == Mode)
    {
      PalGDI->NumColors = NumColors;
    }
  else if (PAL_BITFIELDS == Mode)
    {
      PalGDI->RedMask = Red;
      PalGDI->GreenMask = Green;
      PalGDI->BlueMask = Blue;
    }

  PALETTE_UnlockPalette(NewPalette);

  return NewPalette;
}

// Create the system palette
HPALETTE FASTCALL PALETTE_Init(VOID)
{
  int i;
  HPALETTE hpalette;
  PLOGPALETTE palPtr;
#ifndef NO_MAPPING
  PALOBJ *palObj;
#endif
  const PALETTEENTRY* __sysPalTemplate = (const PALETTEENTRY*)COLOR_GetSystemPaletteTemplate();

  // create default palette (20 system colors)
  palPtr = ExAllocatePoolWithTag(PagedPool, sizeof(LOGPALETTE) + (NB_RESERVED_COLORS * sizeof(PALETTEENTRY)), TAG_PALETTE);
  if (!palPtr) return FALSE;

  palPtr->palVersion = 0x300;
  palPtr->palNumEntries = NB_RESERVED_COLORS;
  for(i=0; i<NB_RESERVED_COLORS; i++)
  {
    palPtr->palPalEntry[i].peRed = __sysPalTemplate[i].peRed;
    palPtr->palPalEntry[i].peGreen = __sysPalTemplate[i].peGreen;
    palPtr->palPalEntry[i].peBlue = __sysPalTemplate[i].peBlue;
    palPtr->palPalEntry[i].peFlags = 0;
  }

  hpalette = NtGdiCreatePalette(palPtr);
  ExFreePool(palPtr);

#ifndef NO_MAPPING
  palObj = (PALOBJ*)PALETTE_LockPalette(hpalette);
  if (palObj)
  {
    if (!(palObj->mapping = ExAllocatePool(PagedPool, sizeof(int) * 20)))
    {
      DbgPrint("Win32k: Can not create palette mapping -- out of memory!");
      return FALSE;
    }
    PALETTE_UnlockPalette(hpalette);
  }
#endif

/*  palette_size = visual->map_entries; */

  return hpalette;
}

#ifndef NO_MAPPING
static void FASTCALL PALETTE_FormatSystemPalette(void)
{
  // Build free list so we'd have an easy way to find
  // out if there are any available colorcells. 

  int i, j = PALETTE_firstFree = NB_RESERVED_COLORS/2;

  COLOR_sysPal[j].peFlags = 0;
  for(i = (NB_RESERVED_COLORS>>1) + 1 ; i < 256 - (NB_RESERVED_COLORS>>1) ; i++)
  {
    if( i < COLOR_gapStart || i > COLOR_gapEnd )
    {
      COLOR_sysPal[i].peFlags = 0;  // unused tag
      PALETTE_freeList[j] = i; // next
      j = i;
    }
  }
  PALETTE_freeList[j] = 0;
}
#endif

VOID FASTCALL PALETTE_ValidateFlags(PALETTEENTRY* lpPalE, INT size)
{
  int i = 0;
  for( ; i<size ; i++ )
    lpPalE[i].peFlags = PC_SYS_USED | (lpPalE[i].peFlags & 0x07);
}

#ifndef NO_MAPPING
// Set the color-mapping table for selected palette. 
// Return number of entries which mapping has changed.
INT STDCALL PALETTE_SetMapping(PALOBJ *palPtr, UINT uStart, UINT uNum, BOOL mapOnly)
{
  char flag;
  int  prevMapping = (palPtr->mapping) ? 1 : 0;
  int  index, iRemapped = 0;
  int *mapping;
  HPALETTE hSysPal = NtGdiGetStockObject(DEFAULT_PALETTE);
  PPALGDI pSysPal = PALETTE_LockPalette(hSysPal);
  PPALGDI palGDI = (PPALGDI) palPtr;

  COLOR_sysPal = pSysPal->IndexedColors;
  PALETTE_UnlockPalette(hSysPal); // FIXME: Is this a right way to obtain pointer to the system palette?


  // reset dynamic system palette entries

  if( !mapOnly && PALETTE_firstFree != -1) PALETTE_FormatSystemPalette();

  // initialize palette mapping table
 
  //mapping = HeapReAlloc( GetProcessHeap(), 0, palPtr->mapping,
  //                       sizeof(int)*palPtr->logpalette->palNumEntries);
  ExFreePool(palPtr->mapping);
  mapping = ExAllocatePoolWithTag(PagedPool, sizeof(int)*palGDI->NumColors, TAG_PALETTEMAP);

  palPtr->mapping = mapping;

  for(uNum += uStart; uStart < uNum; uStart++)
  {
    index = -1;
    flag = PC_SYS_USED;

    switch( palGDI->IndexedColors[uStart].peFlags & 0x07 )
    {
      case PC_EXPLICIT:   // palette entries are indices into system palette
                          // The PC_EXPLICIT flag is used to copy an entry from the system palette into the logical palette
        index = *(WORD*)(palGDI->IndexedColors + uStart);
        if(index > 255 || (index >= COLOR_gapStart && index <= COLOR_gapEnd))
        {
          DbgPrint("Win32k: PC_EXPLICIT: idx %d out of system palette, assuming black.\n", index); 
          index = 0;
        }
        break;

      case PC_RESERVED:   // forbid future mappings to this entry
                          // For palette animation, the entries in the logical palette need the PC_RESERVED flag
        flag |= PC_SYS_RESERVED;

      // fall through
      default: // try to collapse identical colors
        index = COLOR_PaletteLookupExactIndex(COLOR_sysPal, 256,  
                                              *(COLORREF*)(palGDI->IndexedColors + uStart));
            // fall through

      case PC_NOCOLLAPSE:
        // If an entry in the logical palette is marked with the PC_NOCOLLAPSE flag, the palette
        // manager allocates a free entry in the system palette if one is available and only uses the
        // closest colour match if there are no (more) free entries in the system palette

        DbgPrint("Win32k: WARNING: PC_NOCOLLAPSE is not yet working properly\n");

        if( index < 0 )
        {
          if(PALETTE_firstFree > 0 /* && !(PALETTE_PaletteFlags & PALETTE_FIXED) FIXME */ )
          {
            DbgPrint("Win32k: Unimplemented Palette Operation: PC_NOCOLLAPSE [objects/palette.c]\n");
/*            XColor color;
            index = PALETTE_firstFree;  // ought to be available
            PALETTE_firstFree = PALETTE_freeList[index];

            color.pixel = (PALETTE_PaletteToXPixel) ? PALETTE_PaletteToXPixel[index] : index;
            color.red = palPtr->logpalette->palPalEntry[uStart].peRed << 8;
            color.green = palPtr->logpalette->palPalEntry[uStart].peGreen << 8;
            color.blue = palPtr->logpalette->palPalEntry[uStart].peBlue << 8;
            color.flags = DoRed | DoGreen | DoBlue;
            TSXStoreColor(display, PALETTE_PaletteXColormap, &color);

            COLOR_sysPal[index] = palPtr->logpalette->palPalEntry[uStart];
            COLOR_sysPal[index].peFlags = flag;
            PALETTE_freeList[index] = 0;

            if(PALETTE_PaletteToXPixel) index = PALETTE_PaletteToXPixel[index]; */
            break;
          }
/*          else if (PALETTE_PaletteFlags & PALETTE_VIRTUAL)
          {
            index = PALETTE_ToPhysical(NULL, 0x00ffffff &
                                       *(COLORREF*)(palPtr->logpalette->palPalEntry + uStart));
             break;     
           } FIXME */

           // we have to map to existing entry in the system palette

           index = COLOR_PaletteLookupPixel(COLOR_sysPal, 256, NULL,
                                            *(COLORREF*)(palGDI->IndexedColors + uStart), TRUE);
           }
           palGDI->IndexedColors[uStart].peFlags |= PC_SYS_USED;

/*         if(PALETTE_PaletteToXPixel) index = PALETTE_PaletteToXPixel[index]; FIXME */
           break;
        }

        if( !prevMapping || palPtr->mapping[uStart] != index ) iRemapped++;
        palPtr->mapping[uStart] = index;

  }
  return iRemapped;
}
#endif

/* EOF */
