/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Palette Functions
 * FILE:              subsys/win32k/eng/palette.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 11/7/1999: Created
 */

#include <ddk/winddi.h>
#include <include/object.h>
#include "handle.h"

#define NDEBUG
#include <win32k/debug1.h>

HPALETTE STDCALL
EngCreatePalette(ULONG Mode,
		 ULONG NumColors,
		 ULONG *Colors,
		 ULONG Red,
		 ULONG Green,
		 ULONG Blue)
{
  HPALETTE NewPalette;
  PALGDI *PalGDI;

  NewPalette = (HPALETTE)CreateGDIHandle(sizeof(PALGDI), sizeof(PALOBJ));
  if( !ValidEngHandle( NewPalette ) )
	return 0;

  PalGDI = (PALGDI*) AccessInternalObject( (ULONG) NewPalette );
  ASSERT( PalGDI );

  PalGDI->Mode = Mode;

  if(Colors != NULL)
  {
    PalGDI->IndexedColors = ExAllocatePool(NonPagedPool, sizeof(PALETTEENTRY) * NumColors);
    RtlCopyMemory(PalGDI->IndexedColors, Colors, sizeof(PALETTEENTRY) * NumColors);
  }

  if(Mode==PAL_INDEXED)
  {
    PalGDI->NumColors     = NumColors;
  } else
  if(Mode==PAL_BITFIELDS)
  {
    PalGDI->RedMask   = Red;
    PalGDI->GreenMask = Green;
    PalGDI->BlueMask  = Blue;
  }

  return NewPalette;
}

BOOL STDCALL
EngDeletePalette(IN HPALETTE Palette)
{
  FreeGDIHandle((ULONG)Palette);
  return TRUE;
}

ULONG STDCALL
PALOBJ_cGetColors(PALOBJ *PalObj,
		  ULONG Start,
		  ULONG Colors,
		  ULONG *PaletteEntry)
{
  ULONG i;
  PALGDI *PalGDI;

  PalGDI = (PALGDI*)AccessInternalObjectFromUserObject(PalObj);

  for(i=Start; i<Colors; i++)
  {
    PaletteEntry[i] = PalGDI->IndexedColors[i];
  }

  return Colors;
}
