#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/debug.h>
#include <win32k/bitmaps.h>
#include <win32k/color.h>
#include <debug.h>
#include "../include/palette.h"

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
  const PALETTEENTRY* __sysPalTemplate = (const PALETTEENTRY*)COLOR_GetSystemPaletteTemplate();

  // create default palette (20 system colors)
  palPtr = ExAllocatePool(NonPagedPool, sizeof(LOGPALETTE) + (NB_RESERVED_COLORS * sizeof(PALETTEENTRY)));
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

  palObj = (PPALOBJ)AccessUserObject(hpalette);
  if (palObj)
  {
    if (!(palObj->mapping = ExAllocatePool(NonPagedPool, sizeof(int) * 20)))
    {
      DbgPrint("Win32k: Can not create palette mapping -- out of memory!");
      return FALSE;
    }
//      GDI_ReleaseObj( hpalette );
  }

/*  palette_size = visual->map_entries; */

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

/* Ported from WINE 20020804 (graphics\x11drv\palette.c) */
static int SysPaletteLookupPixel( COLORREF col, BOOL skipReserved )
{
  int i, best = 0, diff = 0x7fffffff;
  int r,g,b;

  for( i = 0; i < palette_size && diff ; i++ )
  {
    if( !(COLOR_sysPal[i].peFlags & PC_SYS_USED) || (skipReserved && COLOR_sysPal[i].peFlags  & PC_SYS_RESERVED) )
      continue;

    r = COLOR_sysPal[i].peRed - GetRValue(col);
    g = COLOR_sysPal[i].peGreen - GetGValue(col);
    b = COLOR_sysPal[i].peBlue - GetBValue(col);

    r = r*r + g*g + b*b;

    if( r < diff ) { best = i; diff = r; }
  }
  return best;
}

/* Ported from WINE 20020804 (graphics\x11drv\palette.c) */
/* Make sure this is required - ROS's xlate may make this redundant */
UINT WINAPI GetNearestPaletteIndex(
    HPALETTE hpalette, /* [in] Handle of logical color palette */
    COLORREF color)      /* [in] Color to be matched */
{
  PPALOBJ palObj = (PPALOBJ)AccessUserObject(hpalette);
  UINT    index  = 0;

  if( palObj )
  {
    int i, diff = 0x7fffffff;
    int r,g,b;
    PALETTEENTRY* entry = palObj->logpalette->palPalEntry;

    for( i = 0; i < palObj->logpalette->palNumEntries && diff ; i++, entry++)
    {
      if (!(entry->peFlags & PC_SYS_USED)) continue;

      r = entry->peRed - GetRValue(color);
      g = entry->peGreen - GetGValue(color);
      b = entry->peBlue - GetBValue(color);

      r = r*r + g*g + b*b;

      if( r < diff ) { index = i; diff = r; }
    }
//        GDI_ReleaseObj( hpalette );
  }
  DPRINT("(%04x,%06lx): returning %d\n", hpalette, color, index );
  return index;
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
  //                       sizeof(int)*palPtr->logpalette->palNumEntries);
  ExFreePool(palPtr->mapping);
  mapping = ExAllocatePool(NonPagedPool, sizeof(int)*palPtr->logpalette->palNumEntries);

  palPtr->mapping = mapping;

  for(uNum += uStart; uStart < uNum; uStart++)
  {
    index = -1;
    flag = PC_SYS_USED;

    switch( palPtr->logpalette->palPalEntry[uStart].peFlags & 0x07 )
    {
      case PC_EXPLICIT:   // palette entries are indices into system palette
                          // The PC_EXPLICIT flag is used to copy an entry from the system palette into the logical palette
        index = *(WORD*)(palPtr->logpalette->palPalEntry + uStart);
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
                                              *(COLORREF*)(palPtr->logpalette->palPalEntry + uStart));
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
                                            *(COLORREF*)(palPtr->logpalette->palPalEntry + uStart), TRUE);
           }
           palPtr->logpalette->palPalEntry[uStart].peFlags |= PC_SYS_USED;

/*         if(PALETTE_PaletteToXPixel) index = PALETTE_PaletteToXPixel[index]; FIXME */
           break;
        }

        if( !prevMapping || palPtr->mapping[uStart] != index ) iRemapped++;
        palPtr->mapping[uStart] = index;

  }
  return iRemapped;
}

/* Return the physical color closest to 'color'. */
/* Ported from WINE 20020804 (graphics\x11drv\palette.c) */
int PALETTE_ToPhysical( PDC dc, COLORREF color )
{
    WORD            index = 0;
    HPALETTE        hPal = (dc)? dc->w.hPalette: W32kGetStockObject(DEFAULT_PALETTE);
    unsigned char   spec_type = color >> 24;
    PPALOBJ         palPtr = (PPALOBJ)AccessUserObject(hPal);

    /* palPtr can be NULL when DC is being destroyed */
    if( !palPtr ) return 0;

    if ( PALETTE_PaletteFlags & PALETTE_FIXED )
    {
        /* there is no colormap limitation; we are going to have to compute
         * the pixel value from the visual information stored earlier
	 */

	unsigned 	long red, green, blue;
	unsigned 	idx = 0;

	switch(spec_type)
        {
          case 1: /* PALETTEINDEX */

            if( (idx = color & 0xffff) >= palPtr->logpalette->palNumEntries)
            {
                DPRINT("RGB(%lx) : idx %d is out of bounds, assuming black\n", color, idx);
//		GDI_ReleaseObj( hPal );
                return 0;
            }

            if( palPtr->mapping )
	    {
                int ret = palPtr->mapping[idx];
//		GDI_ReleaseObj( hPal );
		return ret;
	    }
	    color = *(COLORREF*)(palPtr->logpalette->palPalEntry + idx);
	    break;

	  default:
	    color &= 0xffffff;
	    /* fall through to RGB */

	  case 0: /* RGB */
	    if( dc && (dc->w.bitsPerPixel == 1) )
	    {
//		GDI_ReleaseObj( hPal );
		return (((color >> 16) & 0xff) +
			((color >> 8) & 0xff) + (color & 0xff) > 255*3/2) ? 1 : 0;
	    }

	}

        red = GetRValue(color); green = GetGValue(color); blue = GetBValue(color);

	if (PALETTE_Graymax)
        {
	    /* grayscale only; return scaled value */
//	    GDI_ReleaseObj( hPal );
            return ( (red * 30 + green * 59 + blue * 11) * PALETTE_Graymax) / 25500;
	}
	else
        {
	    /* scale each individually and construct the TrueColor pixel value */
	    if (PALETTE_PRed.scale < 8)
		red = red >> (8-PALETTE_PRed.scale);
	    else if (PALETTE_PRed.scale > 8)
		red =   red   << (PALETTE_PRed.scale-8) |
                        red   >> (16-PALETTE_PRed.scale);
	    if (PALETTE_PGreen.scale < 8)
		green = green >> (8-PALETTE_PGreen.scale);
	    else if (PALETTE_PGreen.scale > 8)
		green = green << (PALETTE_PGreen.scale-8) |
                        green >> (16-PALETTE_PGreen.scale);
	    if (PALETTE_PBlue.scale < 8)
		blue =  blue  >> (8-PALETTE_PBlue.scale);
	    else if (PALETTE_PBlue.scale > 8)
		blue =  blue  << (PALETTE_PBlue.scale-8) |
                        blue  >> (16-PALETTE_PBlue.scale);

//	    GDI_ReleaseObj( hPal );
            return (red << PALETTE_PRed.shift) | (green << PALETTE_PGreen.shift) | (blue << PALETTE_PBlue.shift);
        }
    }
    else
    {

	if( !palPtr->mapping )
            DPRINT("Palette %04x is not realized\n", dc->w.hPalette);

	switch(spec_type)	/* we have to peruse DC and system palette */
    	{
	    default:
		color &= 0xffffff;
		/* fall through to RGB */

       	    case 0:  /* RGB */
		if( dc && (dc->w.bitsPerPixel == 1) )
		{
//		    GDI_ReleaseObj( hPal );
		    return (((color >> 16) & 0xff) +
			    ((color >> 8) & 0xff) + (color & 0xff) > 255*3/2) ? 1 : 0;
		}

	    	index = SysPaletteLookupPixel( color, FALSE);

/*                if (PALETTE_PaletteToXPixel) index = PALETTE_PaletteToXPixel[index]; */

		/* DPRINT(palette,"RGB(%lx) -> pixel %i\n", color, index);
		 */
	    	break;
       	    case 1:  /* PALETTEINDEX */
		index = color & 0xffff;

	        if( index >= palPtr->logpalette->palNumEntries )
		    DbgPrint("RGB(%lx) : index %i is out of bounds\n", color, index);
		else if( palPtr->mapping ) index = palPtr->mapping[index];

		/*  DPRINT(palette,"PALETTEINDEX(%04x) -> pixel %i\n", (WORD)color, index);
		 */
		break;
            case 2:  /* PALETTERGB */
                index = GetNearestPaletteIndex( hPal, color );
                if (palPtr->mapping) index = palPtr->mapping[index];
		/* DPRINT(palette,"PALETTERGB(%lx) -> pixel %i\n", color, index);
		 */
		break;
	}
    }

//    GDI_ReleaseObj( hPal );
    return index;
}
