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
/* $Id: palette.c,v 1.16 2003/07/11 15:59:37 royce Exp $
 * 
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

/*
 * @implemented
 */
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

/*
 * @implemented
 */
BOOL STDCALL
EngDeletePalette(IN HPALETTE Palette)
{
  FreeGDIHandle((ULONG)Palette);
  return TRUE;
}

/*
 * @implemented
 */
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
/* EOF */
