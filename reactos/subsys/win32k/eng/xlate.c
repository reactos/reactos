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
/* $Id: xlate.c,v 1.36 2004/06/28 15:53:17 navaraf Exp $
 * 
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GDI Color Translation Functions
 * FILE:             subsys/win32k/eng/xlate.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        8/20/1999: Created
 */
#include <w32k.h>

// TODO: Cache XLATEOBJs that are created by EngCreateXlate by checking if the given palettes match a cached list

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
  static const union { PALETTEENTRY Color; ULONG Mask; } Red = {{255, 0, 0}};
  static const union { PALETTEENTRY Color; ULONG Mask; } Green = {{0, 255, 0}};
  static const union { PALETTEENTRY Color; ULONG Mask; } Blue = {{0, 0, 255}};

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
      *GreenMask = Palette->GreenMask;
      *BlueMask = Palette->BlueMask;
      break;
    case PAL_INDEXED:
      *RedMask = Red.Mask;
      *GreenMask = Green.Mask;
      *BlueMask = Blue.Mask;
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

XLATEOBJ* STDCALL
IntEngCreateXlate(USHORT DestPalType, USHORT SourcePalType,
                  HPALETTE PaletteDest, HPALETTE PaletteSource)
{
   ULONG NewXlate;
   XLATEOBJ *XlateObj;
   XLATEGDI *XlateGDI;
   PALGDI *SourcePalGDI = 0;
   PALGDI *DestPalGDI = 0;
   ULONG SourceRedMask, SourceGreenMask, SourceBlueMask;
   ULONG DestRedMask, DestGreenMask, DestBlueMask;
   ULONG i;

   NewXlate = CreateGDIHandle(sizeof(XLATEGDI), sizeof(XLATEOBJ), (PVOID*)&XlateGDI, (PVOID*)&XlateObj);
   if (!ValidEngHandle(NewXlate))
      return NULL;

   if (PaletteSource != NULL)
      SourcePalGDI = PALETTE_LockPalette(PaletteSource);
   if (PaletteDest == PaletteSource)
      DestPalGDI = SourcePalGDI;
   else if (PaletteDest != NULL)
      DestPalGDI = PALETTE_LockPalette(PaletteDest);

   XlateObj->iSrcType = SourcePalType;
   XlateObj->iDstType = DestPalType;
   XlateObj->flXlate = 0;

   /* Store handles of palettes in internal Xlate GDI object (or NULLs) */
   XlateGDI->SourcePal = PaletteSource;
   XlateGDI->DestPal = PaletteDest;

   XlateGDI->UseShiftAndMask = FALSE;

   /*
    * Compute bit fiddeling constants unless both palettes are indexed, then
    * we don't need them.
    */
   if (SourcePalType != PAL_INDEXED || DestPalType != PAL_INDEXED)
   {
      BitMasksFromPal(SourcePalType, SourcePalGDI, &SourceRedMask,
                      &SourceBlueMask, &SourceGreenMask);
      BitMasksFromPal(DestPalType, DestPalGDI, &DestRedMask,
                      &DestBlueMask, &DestGreenMask);
      XlateGDI->RedShift = CalculateShift(SourceRedMask) - CalculateShift(DestRedMask);
      XlateGDI->RedMask = DestRedMask;
      XlateGDI->GreenShift = CalculateShift(SourceGreenMask) - CalculateShift(DestGreenMask);
      XlateGDI->GreenMask = DestGreenMask;
      XlateGDI->BlueShift = CalculateShift(SourceBlueMask) - CalculateShift(DestBlueMask);
      XlateGDI->BlueMask = DestBlueMask;
   }

   /* If source and destination palettes are the same or if they're RGB/BGR */
   if (PaletteDest == PaletteSource ||
       (DestPalType == PAL_RGB && SourcePalType == PAL_RGB) ||
       (DestPalType == PAL_BGR && SourcePalType == PAL_BGR))
   {
      XlateObj->flXlate |= XO_TRIVIAL;
      goto end;
   }

   /*
    * If source and destination are bitfield based (RGB and BGR are just
    * special bitfields) we can use simple shifting.
    */
   if ((DestPalType == PAL_RGB || DestPalType == PAL_BGR ||
        DestPalType == PAL_BITFIELDS) &&
       (SourcePalType == PAL_RGB || SourcePalType == PAL_BGR ||
        SourcePalType == PAL_BITFIELDS))
   {
      if (SourceRedMask == DestRedMask &&
          SourceBlueMask == DestBlueMask &&
          SourceGreenMask == DestGreenMask)
      {
         XlateObj->flXlate |= XO_TRIVIAL;
      }
      XlateGDI->UseShiftAndMask = TRUE;
      goto end;
   }

   /* Indexed -> Indexed */
   if (SourcePalType == PAL_INDEXED && DestPalType == PAL_INDEXED)
   {
      XlateGDI->translationTable = 
         EngAllocMem(0, sizeof(ULONG) * SourcePalGDI->NumColors, 0);
      IndexedToIndexedTranslationTable(XlateGDI, XlateGDI->translationTable, DestPalGDI, SourcePalGDI);
      XlateObj->flXlate |= XO_TABLE;
      XlateObj->pulXlate = XlateGDI->translationTable;
      goto end;
   }

   /* Indexed -> Bitfields/RGB/BGR */
   if (SourcePalType == PAL_INDEXED)
   {
      XlateGDI->translationTable = 
         EngAllocMem(0, sizeof(ULONG) * SourcePalGDI->NumColors, 0);
      for (i = 0; i < SourcePalGDI->NumColors; i++)
         XlateGDI->translationTable[i] =
            ShiftAndMask(XlateGDI, *((ULONG *)&SourcePalGDI->IndexedColors[i]));
      XlateObj->flXlate |= XO_TABLE;
      XlateObj->pulXlate = XlateGDI->translationTable;
      goto end;
   }

   /*
    * Last case: Bitfields/RGB/BGR -> Indexed
    * isn't handled here yet and all the logic is in XLATEOBJ_iXlate now.
    */

end:
   if (PaletteSource != NULL)
      PALETTE_UnlockPalette(PaletteSource);
   if (PaletteDest != NULL && PaletteDest != PaletteSource)
      PALETTE_UnlockPalette(PaletteDest);
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

   NewXlate = (HPALETTE)CreateGDIHandle(sizeof(XLATEGDI), sizeof(XLATEOBJ), (PVOID*)&XlateGDI, (PVOID*)&XlateObj);
   if (!ValidEngHandle(NewXlate))
      return NULL;

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
            XlateGDI->RedShift = CalculateShift(0xFF) - CalculateShift(XlateGDI->RedMask);
            XlateGDI->GreenShift = CalculateShift(0xFF00) - CalculateShift(XlateGDI->GreenMask);
            XlateGDI->BlueShift = CalculateShift(0xFF0000) - CalculateShift(XlateGDI->BlueMask);
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
   XLATEGDI *XlateGDI;
   PALGDI *PalGDI;
   ULONG Closest;

   /* Return the original color if there's no color translation object. */
   if (!XlateObj)
      return Color;

   if (XlateObj->flXlate & XO_TRIVIAL)
      return Color;

   if (XlateObj->flXlate & XO_TABLE)
      return XlateObj->pulXlate[Color];         

   XlateGDI = (XLATEGDI *)AccessInternalObjectFromUserObject(XlateObj);

   if (XlateObj->flXlate & XO_TO_MONO)
      return Color == XlateGDI->BackgroundColor;

   if (XlateGDI->UseShiftAndMask)
      return ShiftAndMask(XlateGDI, Color);

   if (XlateObj->iSrcType == PAL_RGB || XlateObj->iSrcType == PAL_BGR ||
       XlateObj->iSrcType == PAL_BITFIELDS)
   {
      /* FIXME: should we cache colors used often? */
      /* FIXME: won't work if destination isn't indexed */

      /* Extract the destination palette. */
      PalGDI = PALETTE_LockPalette(XlateGDI->DestPal);

      /* Convert the source color to the palette RGB format. */
      Color = ShiftAndMask(XlateGDI, Color);

      /* Return closest match for the given color. */
      Closest = ClosestColorMatch(XlateGDI, Color, PalGDI->IndexedColors, PalGDI->NumColors);
      PALETTE_UnlockPalette(XlateGDI->DestPal);
      return Closest;
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
