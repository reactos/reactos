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
/* $Id: xlate.c,v 1.32 2004/04/09 20:39:10 navaraf Exp $
 * 
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI Color Translation Functions
 * FILE:             subsys/win32k/eng/xlate.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        8/20/1999: Created
 */

// TODO: Cache XLATEOBJs that are created by EngCreateXlate by checking if the given palettes match a cached list

#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <ddk/ntddvdeo.h>

#include <include/object.h>
#include <include/palette.h>
#include "handle.h"
#include <win32k/color.h>

#define NDEBUG
#include <win32k/debug1.h>

ULONG CCMLastSourceColor = 0, CCMLastColorMatch = 0;

static ULONG FASTCALL ShiftAndMask(XLATEGDI *XlateGDI, ULONG Color)
{
  ULONG TranslatedColor;

  TranslatedColor = 0;
  if (XlateGDI->RedShift < 0)
  {
    TranslatedColor = (Color >> -(XlateGDI->RedShift)) & XlateGDI->RedMask;
  } else
    TranslatedColor = (Color << XlateGDI->RedShift) & XlateGDI->RedMask;
  if (XlateGDI->GreenShift < 0)
  {
    TranslatedColor |= (Color >> -(XlateGDI->GreenShift)) & XlateGDI->GreenMask;
  } else
    TranslatedColor |= (Color << XlateGDI->GreenShift) & XlateGDI->GreenMask;
  if (XlateGDI->BlueShift < 0)
  {
    TranslatedColor |= (Color >> -(XlateGDI->BlueShift)) & XlateGDI->BlueMask;
  } else
    TranslatedColor |= (Color << XlateGDI->BlueShift) & XlateGDI->BlueMask;

  return TranslatedColor;
}


// FIXME: If the caller knows that the destinations are indexed and not RGB
// then we should cache more than one value. Same with the source.

// Takes indexed palette and a
ULONG STDCALL 
ClosestColorMatch(XLATEGDI *XlateGDI, ULONG SourceColor,
   PALETTEENTRY *DestColors, ULONG NumColors)
{
  LPPALETTEENTRY cSourceColor;
  LONG idx = 0;
  ULONG i;
  ULONG SourceRGB;
  ULONG SourceRed, SourceGreen, SourceBlue;
  ULONG cxRed, cxGreen, cxBlue, rt, BestMatch = 16777215;

  // Simple cache -- only one value because we don't want to waste time
  // if the colors aren't very sequential

  if(SourceColor == CCMLastSourceColor)
  {
    return CCMLastColorMatch;
  }

  if (PAL_BITFIELDS == XlateGDI->XlateObj.iSrcType || PAL_BGR == XlateGDI->XlateObj.iSrcType)
    {
      /* FIXME: must use bitfields */
      SourceRGB = ShiftAndMask(XlateGDI, SourceColor);
      cSourceColor = (LPPALETTEENTRY) &SourceRGB;
    }
  else
    {
      cSourceColor = (LPPALETTEENTRY)&SourceColor;
    } 
  SourceRed = cSourceColor->peRed;
  SourceGreen = cSourceColor->peGreen;
  SourceBlue = cSourceColor->peBlue;

  for (i=0; i<NumColors; i++)
  {
    cxRed = (SourceRed - DestColors[i].peRed);
	cxRed *= cxRed;  //compute cxRed squared
    cxGreen = (SourceGreen - DestColors[i].peGreen);
	cxGreen *= cxGreen;
    cxBlue = (SourceBlue - DestColors[i].peBlue);
	cxBlue *= cxBlue;

    rt = /* sqrt */ (cxRed + cxGreen + cxBlue);

    if(rt<=BestMatch)
    {
      idx = i;
      BestMatch = rt;
    }
  }

  CCMLastSourceColor = SourceColor;
  CCMLastColorMatch  = idx;

  return idx;
}

VOID STDCALL 
IndexedToIndexedTranslationTable(XLATEGDI *XlateGDI, ULONG *TranslationTable,
                                      PALGDI *PalDest, PALGDI *PalSource)
{
  ULONG i;
  BOOL Trivial;

  Trivial = TRUE;
  for(i=0; i<PalSource->NumColors; i++)
    {
      TranslationTable[i] = ClosestColorMatch(XlateGDI, *((ULONG*)&PalSource->IndexedColors[i]), PalDest->IndexedColors, PalDest->NumColors);
      Trivial = Trivial && (TranslationTable[i] == i);
    }
  if (Trivial)
    {
      XlateGDI->XlateObj.flXlate |= XO_TRIVIAL;
    }
}

static VOID STDCALL
BitMasksFromPal(USHORT PalType, PPALGDI Palette,
                            PULONG RedMask, PULONG BlueMask, PULONG GreenMask)
{
  switch(PalType)
  {
    case PAL_RGB:
      *RedMask = RGB(255, 0, 0);
      *GreenMask = RGB(0, 255, 0);
      *BlueMask = RGB(0, 0, 255);
      break;
    case PAL_BGR:
      *RedMask = RGB(0, 0, 255);
      *GreenMask = RGB(0, 255, 0);
      *BlueMask = RGB(255, 0, 0);
      break;
    case PAL_BITFIELDS:
      *RedMask = Palette->RedMask;
      *BlueMask = Palette->BlueMask;
      *GreenMask = Palette->GreenMask;
      break;
  }
}

/*
 * Calculate the number of bits Mask must be shift to the left to get a
 * 1 in the most significant bit position
 */
static INT FASTCALL CalculateShift(ULONG Mask)
{
   ULONG Shift = 0;
   ULONG LeftmostBit = 1 << (8 * sizeof(ULONG) - 1);

   while (0 == (Mask & LeftmostBit) && Shift < 8 * sizeof(ULONG))
     {
     Mask = Mask << 1;
     Shift++;
     }

   return Shift;
}

VOID FASTCALL EngDeleteXlate(XLATEOBJ *XlateObj)
{
  XLATEGDI *XlateGDI;
  HANDLE HXlate;

  if (NULL == XlateObj)
    {
      DPRINT1("Trying to delete NULL XLATEOBJ\n");
      return;
    }

  XlateGDI = (XLATEGDI *) AccessInternalObjectFromUserObject(XlateObj);
  HXlate = (HANDLE) AccessHandleFromUserObject(XlateObj);

  if((XlateObj->flXlate & XO_TABLE) && NULL != XlateGDI->translationTable)
    {
      EngFreeMem(XlateGDI->translationTable);
    }

  FreeGDIHandle((ULONG)HXlate);
}

XLATEOBJ * STDCALL IntEngCreateXlate(USHORT DestPalType, USHORT SourcePalType,
                            HPALETTE PaletteDest, HPALETTE PaletteSource)
{
  // FIXME: Add support for BGR conversions

  HPALETTE NewXlate;
  XLATEOBJ *XlateObj;
  XLATEGDI *XlateGDI;
  PALGDI   *SourcePalGDI, *DestPalGDI;
  ULONG    IndexedColors;
  ULONG    SourceRedMask, SourceGreenMask, SourceBlueMask;
  ULONG    DestRedMask, DestGreenMask, DestBlueMask;
  UINT     i;

  NewXlate = (HPALETTE)CreateGDIHandle(sizeof( XLATEGDI ), sizeof( XLATEOBJ ));
  if ( !ValidEngHandle ( NewXlate ) )
    return NULL;

  XlateObj = (XLATEOBJ*) AccessUserObject( (ULONG) NewXlate );
  XlateGDI = (XLATEGDI*) AccessInternalObject( (ULONG) NewXlate );
  ASSERT( XlateObj );
  ASSERT( XlateGDI );

  if (NULL != PaletteSource)
  {
    SourcePalGDI = PALETTE_LockPalette(PaletteSource);
  }
  if (PaletteDest == PaletteSource)
  {
    DestPalGDI = SourcePalGDI;
  }
  else if (NULL != PaletteDest)
  {
    DestPalGDI = PALETTE_LockPalette(PaletteDest);
  }

  XlateObj->iSrcType = SourcePalType;
  XlateObj->iDstType = DestPalType;

  // Store handles of palettes in internal Xlate GDI object (or NULLs)
  XlateGDI->DestPal   = PaletteDest;
  XlateGDI->SourcePal = PaletteSource;

  XlateObj->flXlate = 0;

  XlateGDI->UseShiftAndMask = FALSE;

  /* Compute bit fiddeling constants unless both palettes are indexed, then we don't need them */
  if (PAL_INDEXED != SourcePalType || PAL_INDEXED != DestPalType)
  {
    BitMasksFromPal(PAL_INDEXED == SourcePalType ? PAL_RGB : SourcePalType,
                    SourcePalGDI, &SourceRedMask, &SourceBlueMask, &SourceGreenMask);
    BitMasksFromPal(PAL_INDEXED == DestPalType ? PAL_RGB : DestPalType,
                    DestPalGDI, &DestRedMask, &DestBlueMask, &DestGreenMask);
    XlateGDI->RedShift = CalculateShift(SourceRedMask) - CalculateShift(DestRedMask);
    XlateGDI->RedMask = DestRedMask;
    XlateGDI->GreenShift = CalculateShift(SourceGreenMask) - CalculateShift(DestGreenMask);
    XlateGDI->GreenMask = DestGreenMask;
    XlateGDI->BlueShift = CalculateShift(SourceBlueMask) - CalculateShift(DestBlueMask);
    XlateGDI->BlueMask = DestBlueMask;
  }

  // If source and destination palettes are the same or if they're RGB/BGR
  if( (PaletteDest == PaletteSource) ||
      ((DestPalType == PAL_RGB) && (SourcePalType == PAL_RGB)) ||
      ((DestPalType == PAL_BGR) && (SourcePalType == PAL_BGR)) )
  {
    XlateObj->flXlate |= XO_TRIVIAL;
    if (NULL != PaletteSource)
    {
      PALETTE_UnlockPalette(PaletteSource);
    }
    if (NULL != PaletteDest && PaletteDest != PaletteSource)
    {
      PALETTE_UnlockPalette(PaletteDest);
    }
    return XlateObj;
  }

  /* If source and destination are bitfield based (RGB and BGR are just special bitfields) */
  if ((PAL_RGB == DestPalType || PAL_BGR == DestPalType || PAL_BITFIELDS == DestPalType) &&
      (PAL_RGB == SourcePalType || PAL_BGR == SourcePalType || PAL_BITFIELDS == SourcePalType))
  {
    if (SourceRedMask == DestRedMask &&
        SourceBlueMask == DestBlueMask &&
        SourceGreenMask == DestGreenMask)
      {
      XlateObj->flXlate |= XO_TRIVIAL;
      }
    XlateGDI->UseShiftAndMask = TRUE;
    if (NULL != PaletteSource)
    {
      PALETTE_UnlockPalette(PaletteSource);
    }
    if (NULL != PaletteDest && PaletteDest != PaletteSource)
    {
      PALETTE_UnlockPalette(PaletteDest);
    }
    return XlateObj;
  }

  // Prepare the translation table
  if (PAL_INDEXED == SourcePalType || PAL_RGB == SourcePalType || PAL_BGR == SourcePalType)
  {
    XlateObj->flXlate |= XO_TABLE;
    if ((SourcePalType == PAL_INDEXED) && (DestPalType == PAL_INDEXED))
    {
      if(SourcePalGDI->NumColors > DestPalGDI->NumColors)
      {
        IndexedColors = SourcePalGDI->NumColors;
      } else
        IndexedColors = DestPalGDI->NumColors;
    }
    else if (SourcePalType == PAL_INDEXED) { IndexedColors = SourcePalGDI->NumColors; }
    else if (DestPalType   == PAL_INDEXED) { IndexedColors = DestPalGDI->NumColors; }

    XlateGDI->translationTable = EngAllocMem(FL_ZERO_MEMORY, sizeof(ULONG)*IndexedColors, 0);
    if (NULL == XlateGDI->translationTable)
      {
	if (NULL != PaletteSource)
	  {
	    PALETTE_UnlockPalette(PaletteSource);
	  }
	if (NULL != PaletteDest && PaletteDest != PaletteSource)
	  {
	    PALETTE_UnlockPalette(PaletteDest);
	  }
	EngDeleteXlate(XlateObj);
	return NULL;
      }
  }

  // Source palette is indexed
  if(XlateObj->iSrcType == PAL_INDEXED)
  {
    if(XlateObj->iDstType == PAL_INDEXED)
    {
      // Converting from indexed to indexed
      IndexedToIndexedTranslationTable(XlateGDI, XlateGDI->translationTable, DestPalGDI, SourcePalGDI);
    } else
      if (PAL_RGB == XlateObj->iDstType || PAL_BITFIELDS == XlateObj->iDstType )
      {
        // FIXME: Is this necessary? I think the driver has to call this
        // function anyways if pulXlate is NULL and Source is PAL_INDEXED

        // Converting from indexed to RGB

	RtlCopyMemory(XlateGDI->translationTable, SourcePalGDI->IndexedColors, sizeof(ULONG) * SourcePalGDI->NumColors);
	if (PAL_BITFIELDS == XlateObj->iDstType)
	{
	  for (i = 0; i < SourcePalGDI->NumColors; i++)
	  {
	  XlateGDI->translationTable[i] = ShiftAndMask(XlateGDI, XlateGDI->translationTable[i]);
	  }
	}
      }

    XlateObj->pulXlate = XlateGDI->translationTable;
  }

  // Source palette is RGB
  if (PAL_RGB == XlateObj->iSrcType || PAL_BGR == XlateObj->iSrcType)
  {
    if(PAL_INDEXED == XlateObj->iDstType)
    {
      // FIXME: Is this necessary? I think the driver has to call this
      // function anyways if pulXlate is NULL and Dest is PAL_INDEXED

      // Converting from RGB to indexed
      RtlCopyMemory(XlateGDI->translationTable, DestPalGDI->IndexedColors, sizeof(ULONG) * DestPalGDI->NumColors);
    }
  }

  // FIXME: Add support for XO_TO_MONO
  if (NULL != PaletteSource)
  {
    PALETTE_UnlockPalette(PaletteSource);
  }
  if (NULL != PaletteDest && PaletteDest != PaletteSource)
  {
    PALETTE_UnlockPalette(PaletteDest);
  }

  return XlateObj;
}

XLATEOBJ * STDCALL IntEngCreateMonoXlate(
   USHORT SourcePalType, HPALETTE PaletteDest, HPALETTE PaletteSource,
   ULONG BackgroundColor)
{
   HPALETTE NewXlate;
   XLATEOBJ *XlateObj;
   XLATEGDI *XlateGDI;
   PALGDI *SourcePalGDI;

   NewXlate = (HPALETTE)CreateGDIHandle(sizeof(XLATEGDI), sizeof(XLATEOBJ));
   if (!ValidEngHandle(NewXlate))
      return NULL;

   XlateObj = (XLATEOBJ *)AccessUserObject((ULONG)NewXlate);
   XlateGDI = (XLATEGDI *)AccessInternalObject((ULONG)NewXlate);
   ASSERT(XlateObj);
   ASSERT(XlateGDI);

   XlateObj->iSrcType = SourcePalType;
   XlateObj->iDstType = PAL_INDEXED;

   // Store handles of palettes in internal Xlate GDI object (or NULLs)
   XlateGDI->DestPal = PaletteDest;
   XlateGDI->SourcePal = PaletteSource;

   XlateObj->flXlate = XO_TO_MONO;
   /* FIXME: Map into source palette type */
   switch (SourcePalType)
   {
      case PAL_INDEXED:
         XlateGDI->BackgroundColor = NtGdiGetNearestPaletteIndex(
            PaletteSource, BackgroundColor);
         break;
      case PAL_RGB:
         XlateGDI->BackgroundColor = BackgroundColor;
         break;
      case PAL_BGR:
         XlateGDI->BackgroundColor =
            ((BackgroundColor & 0xFF) << 16) |
            ((BackgroundColor & 0xFF0000) >> 16) |
            (BackgroundColor & 0xFF00);
         break;
      case PAL_BITFIELDS:
         {
            SourcePalGDI = PALETTE_LockPalette(PaletteSource);
            BitMasksFromPal(SourcePalType, SourcePalGDI, &XlateGDI->RedMask,
               &XlateGDI->BlueMask, &XlateGDI->GreenMask);
            XlateGDI->RedShift = CalculateShift(0xFF0000) - CalculateShift(XlateGDI->RedMask);
            XlateGDI->GreenShift = CalculateShift(0xFF00) - CalculateShift(XlateGDI->GreenMask);
            XlateGDI->BlueShift = CalculateShift(0xFF) - CalculateShift(XlateGDI->BlueMask);
            XlateGDI->BackgroundColor = ShiftAndMask(XlateGDI, BackgroundColor);
            PALETTE_UnlockPalette(PaletteSource);
         }
         break;
   }

   return XlateObj;
}

/*
 * @implemented
 */
PULONG STDCALL
XLATEOBJ_piVector(XLATEOBJ *XlateObj)
{
   XLATEGDI *XlateGDI = (XLATEGDI*)AccessInternalObjectFromUserObject(XlateObj);

   if (XlateObj->iSrcType == PAL_INDEXED)
   {
      return XlateGDI->translationTable;
   }

   return NULL;
}

/*
 * @implemented
 */
ULONG STDCALL
XLATEOBJ_iXlate(XLATEOBJ *XlateObj, ULONG Color)
{
   PALGDI *PalGDI;
   ULONG Closest;

   /* Return the original color if there's no color translation object. */
   if (!XlateObj)
      return Color;

   if (XlateObj->flXlate & XO_TRIVIAL)
   {
      return Color;
   } else
   {
      XLATEGDI *XlateGDI = (XLATEGDI *)AccessInternalObjectFromUserObject(XlateObj);

      if (XlateObj->flXlate & XO_TO_MONO)
      {
         return Color == XlateGDI->BackgroundColor;
      } else
      if (XlateGDI->UseShiftAndMask)
      {
         return ShiftAndMask(XlateGDI, Color);
      } else
      if (XlateObj->iSrcType == PAL_RGB || XlateObj->iSrcType == PAL_BGR ||
          XlateObj->iSrcType == PAL_BITFIELDS)
      {
         /* FIXME: should we cache colors used often? */
         /* FIXME: won't work if destination isn't indexed */

         /* Extract the destination palette. */
         PalGDI = PALETTE_LockPalette(XlateGDI->DestPal);

         /* Return closest match for the given color. */
         Closest = ClosestColorMatch(XlateGDI, Color, PalGDI->IndexedColors, PalGDI->NumColors);
         PALETTE_UnlockPalette(XlateGDI->DestPal);
         return Closest;
      } else
      if (XlateObj->iSrcType == PAL_INDEXED)
      {
         return XlateGDI->translationTable[Color];
      }
  }

  return 0;
}

/*
 * @implemented
 */
ULONG STDCALL
XLATEOBJ_cGetPalette(XLATEOBJ *XlateObj, ULONG PalOutType, ULONG cPal,
   ULONG *OutPal)
{
   HPALETTE hPalette;
   XLATEGDI *XlateGDI;
   PALGDI *PalGDI;

   XlateGDI = (XLATEGDI*)AccessInternalObjectFromUserObject(XlateObj);
   if (PalOutType == XO_SRCPALETTE)
      hPalette = XlateGDI->SourcePal;
   else if (PalOutType == XO_DESTPALETTE)
      hPalette = XlateGDI->DestPal;
   else
      UNIMPLEMENTED;

   PalGDI = PALETTE_LockPalette(hPalette);
   RtlCopyMemory(OutPal, PalGDI->IndexedColors, sizeof(ULONG) * cPal);
   PALETTE_UnlockPalette(hPalette);

   return cPal;
}

/* EOF */
