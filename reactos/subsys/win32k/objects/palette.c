#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/debug.h>
#include <win32k/bitmaps.h>
#include <win32k/color.h>
#include <debug.h>

static int           PALETTE_firstFree = 0; 
static unsigned char PALETTE_freeList[256];

int PALETTE_PaletteFlags     = 0;
PALETTEENTRY *COLOR_sysPal   = NULL;
int COLOR_gapStart;
int COLOR_gapEnd;
int COLOR_gapFilled;
int COLOR_max;

PALETTEENTRY *ReturnSystemPalette(void)
{
  return COLOR_sysPal;
}

// Create the system palette
HPALETTE PALETTE_Init(void)
{
  int i;
  HPALETTE hpalette;
  PLOGPALETTE palPtr;
  PPALOBJ palObj;
  const PALETTEENTRY* __sysPalTemplate = COLOR_GetSystemPaletteTemplate();

  // create default palette (20 system colors)
  palPtr = ExAllocatePool(NonPagedPool, sizeof(LOGPALETTE) + (NB_RESERVED_COLORS-1) * sizeof(PALETTEENTRY));
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
  hpalette = W32kCreatePalette(palPtr);
  ExFreePool(palPtr);

  palObj = AccessUserObject(hpalette);
  if (palObj)
  {
    if (!(palObj->mapping = ExAllocatePool(NonPagedPool, sizeof(int) * 20)))
    {
      DbgPrint("Win32k: Can not create palette mapping -- out of memory!");
      return FALSE;
    }
//      GDI_ReleaseObj( hpalette );
  }

  return hpalette;
}

static void PALETTE_FormatSystemPalette(void)
{
  // Build free list so we'd have an easy way to find
  // out if there are any available colorcells. 

  int i, j = PALETTE_firstFree = NB_RESERVED_COLORS/2;

  COLOR_sysPal[j].peFlags = 0;
  for(i = NB_RESERVED_COLORS/2 + 1 ; i < 256 - NB_RESERVED_COLORS/2 ; i++)
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

void PALETTE_ValidateFlags(PALETTEENTRY* lpPalE, int size)
{
  int i = 0;
  for( ; i<size ; i++ )
    lpPalE[i].peFlags = PC_SYS_USED | (lpPalE[i].peFlags & 0x07);
}

// Set the color-mapping table for selected palette. 
// Return number of entries which mapping has changed.
int PALETTE_SetMapping(PPALOBJ palPtr, UINT uStart, UINT uNum, BOOL mapOnly)
{
  char flag;
  int  prevMapping = (palPtr->mapping) ? 1 : 0;
  int  index, iRemapped = 0;
  int *mapping;

  // reset dynamic system palette entries

  if( !mapOnly && PALETTE_firstFree != -1) PALETTE_FormatSystemPalette();

  // initialize palette mapping table
 
  //mapping = HeapReAlloc( GetProcessHeap(), 0, palPtr->mapping,
  //                       sizeof(int)*palPtr->logpalette.palNumEntries);
  ExFreePool(palPtr->mapping);
  mapping = ExAllocatePool(NonPagedPool, sizeof(int)*palPtr->logpalette.palNumEntries);

  palPtr->mapping = mapping;

  for(uNum += uStart; uStart < uNum; uStart++)
  {
    index = -1;
    flag = PC_SYS_USED;

    switch( palPtr->logpalette.palPalEntry[uStart].peFlags & 0x07 )
    {
      case PC_EXPLICIT:   // palette entries are indices into system palette
                          // The PC_EXPLICIT flag is used to copy an entry from the system palette into the logical palette
        index = *(WORD*)(palPtr->logpalette.palPalEntry + uStart);
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
                                              *(COLORREF*)(palPtr->logpalette.palPalEntry + uStart));
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
            color.red = palPtr->logpalette.palPalEntry[uStart].peRed << 8;
            color.green = palPtr->logpalette.palPalEntry[uStart].peGreen << 8;
            color.blue = palPtr->logpalette.palPalEntry[uStart].peBlue << 8;
            color.flags = DoRed | DoGreen | DoBlue;
            TSXStoreColor(display, PALETTE_PaletteXColormap, &color);

            COLOR_sysPal[index] = palPtr->logpalette.palPalEntry[uStart];
            COLOR_sysPal[index].peFlags = flag;
            PALETTE_freeList[index] = 0;

            if(PALETTE_PaletteToXPixel) index = PALETTE_PaletteToXPixel[index]; */
            break;
          }
/*          else if (PALETTE_PaletteFlags & PALETTE_VIRTUAL)
          {
            index = PALETTE_ToPhysical(NULL, 0x00ffffff &
                                       *(COLORREF*)(palPtr->logpalette.palPalEntry + uStart));
             break;     
           } FIXME */

           // we have to map to existing entry in the system palette

           index = COLOR_PaletteLookupPixel(COLOR_sysPal, 256, NULL,
                                            *(COLORREF*)(palPtr->logpalette.palPalEntry + uStart), TRUE);
           }
           palPtr->logpalette.palPalEntry[uStart].peFlags |= PC_SYS_USED;

/*         if(PALETTE_PaletteToXPixel) index = PALETTE_PaletteToXPixel[index]; FIXME */
           break;
        }

        if( !prevMapping || palPtr->mapping[uStart] != index ) iRemapped++;
        palPtr->mapping[uStart] = index;

  }
  return iRemapped;
}
