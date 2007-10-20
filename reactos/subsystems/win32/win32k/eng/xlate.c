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
/* $Id$
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

#define NDEBUG
#include <debug.h>

static __inline ULONG
ShiftAndMask(XLATEGDI *XlateGDI, ULONG Color)
{
   ULONG TranslatedColor;

   if (XlateGDI->RedShift < 0)
      TranslatedColor = (Color >> -(XlateGDI->RedShift)) & XlateGDI->RedMask;
   else
      TranslatedColor = (Color << XlateGDI->RedShift) & XlateGDI->RedMask;
   if (XlateGDI->GreenShift < 0)
      TranslatedColor |= (Color >> -(XlateGDI->GreenShift)) & XlateGDI->GreenMask;
   else
      TranslatedColor |= (Color << XlateGDI->GreenShift) & XlateGDI->GreenMask;
   if (XlateGDI->BlueShift < 0)
      TranslatedColor |= (Color >> -(XlateGDI->BlueShift)) & XlateGDI->BlueMask;
   else
      TranslatedColor |= (Color << XlateGDI->BlueShift) & XlateGDI->BlueMask;

   return TranslatedColor;
}


static __inline ULONG
ClosestColorMatch(XLATEGDI *XlateGDI, LPPALETTEENTRY SourceColor,
                  PALETTEENTRY *DestColors, ULONG NumColors)
{
   ULONG SourceRed, SourceGreen, SourceBlue;
   ULONG cxRed, cxGreen, cxBlue, Rating, BestMatch = 0xFFFFFF;
   ULONG CurrentIndex, BestIndex = 0;

   SourceRed = SourceColor->peRed;
   SourceGreen = SourceColor->peGreen;
   SourceBlue = SourceColor->peBlue;

   for (CurrentIndex = 0; CurrentIndex < NumColors; CurrentIndex++, DestColors++)
   {
      cxRed = abs((SHORT)SourceRed - (SHORT)DestColors->peRed);
      cxRed *= cxRed;
      cxGreen = abs((SHORT)SourceGreen - (SHORT)DestColors->peGreen);
      cxGreen *= cxGreen;
      cxBlue = abs((SHORT)SourceBlue - (SHORT)DestColors->peBlue);
      cxBlue *= cxBlue;

      Rating = cxRed + cxGreen + cxBlue;

      if (Rating == 0)
      {
         /* Exact match */
         BestIndex = CurrentIndex;
         break;
      }

      if (Rating < BestMatch)
      {
         BestIndex = CurrentIndex;
         BestMatch = Rating;
      }
   }

   return BestIndex;
}

static __inline VOID
BitMasksFromPal(USHORT PalType, PPALGDI Palette,
                PULONG RedMask, PULONG BlueMask, PULONG GreenMask)
{
   static const union { PALETTEENTRY Color; ULONG Mask; } Red   = {{0xFF, 0x00, 0x00}};
   static const union { PALETTEENTRY Color; ULONG Mask; } Green = {{0x00, 0xFF, 0x00}};
   static const union { PALETTEENTRY Color; ULONG Mask; } Blue  = {{0x00, 0x00, 0xFF}};

   switch (PalType)
   {
      case PAL_RGB:
         *RedMask   = RGB(0xFF, 0x00, 0x00);
         *GreenMask = RGB(0x00, 0xFF, 0x00);
         *BlueMask  = RGB(0x00, 0x00, 0xFF);
         break;
      case PAL_BGR:
         *RedMask   = RGB(0x00, 0x00, 0xFF);
         *GreenMask = RGB(0x00, 0xFF, 0x00);
         *BlueMask  = RGB(0xFF, 0x00, 0x00);
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
static __inline INT
CalculateShift(ULONG Mask)
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

XLATEOBJ* FASTCALL
IntEngCreateXlate(USHORT DestPalType, USHORT SourcePalType,
                  HPALETTE PaletteDest, HPALETTE PaletteSource)
{
   XLATEOBJ *XlateObj;
   XLATEGDI *XlateGDI;
   PALGDI *SourcePalGDI = 0;
   PALGDI *DestPalGDI = 0;
   ULONG SourceRedMask = 0, SourceGreenMask = 0, SourceBlueMask = 0;
   ULONG DestRedMask = 0, DestGreenMask = 0, DestBlueMask = 0;
   ULONG i;

   XlateGDI = EngAllocMem(0, sizeof(XLATEGDI), TAG_XLATEOBJ);
   if (XlateGDI == NULL)
   {
      DPRINT1("Failed to allocate memory for a XLATE structure!\n");
      return NULL;
   }
   XlateObj = GDIToObj(XlateGDI, XLATE);

   if (PaletteSource != NULL)
      SourcePalGDI = PALETTE_LockPalette(PaletteSource);
   if (PaletteDest == PaletteSource)
      DestPalGDI = SourcePalGDI;
   else if (PaletteDest != NULL)
      DestPalGDI = PALETTE_LockPalette(PaletteDest);

   if (SourcePalType == 0)
      SourcePalType = SourcePalGDI->Mode;
   if (DestPalType == 0)
      DestPalType = DestPalGDI->Mode;

   XlateObj->iSrcType = SourcePalType;
   XlateObj->iDstType = DestPalType;
   XlateObj->flXlate = 0;
   XlateObj->cEntries = 0;

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
      XlateObj->cEntries = SourcePalGDI->NumColors;
      XlateObj->pulXlate =
         EngAllocMem(0, sizeof(ULONG) * XlateObj->cEntries, 0);

      XlateObj->flXlate |= XO_TRIVIAL;
      for (i = 0; i < XlateObj->cEntries; i++)
      {
         XlateObj->pulXlate[i] = ClosestColorMatch(
            XlateGDI, SourcePalGDI->IndexedColors + i,
            DestPalGDI->IndexedColors, XlateObj->cEntries);
         if (XlateObj->pulXlate[i] != i)
            XlateObj->flXlate &= ~XO_TRIVIAL;
      }

      XlateObj->flXlate |= XO_TABLE;
      goto end;
   }

   /* Indexed -> Bitfields/RGB/BGR */
   if (SourcePalType == PAL_INDEXED)
   {
      XlateObj->cEntries = SourcePalGDI->NumColors;
      XlateObj->pulXlate =
         EngAllocMem(0, sizeof(ULONG) * XlateObj->cEntries, 0);
      for (i = 0; i < XlateObj->cEntries; i++)
         XlateObj->pulXlate[i] =
            ShiftAndMask(XlateGDI, *((ULONG *)&SourcePalGDI->IndexedColors[i]));
      XlateObj->flXlate |= XO_TABLE;
      goto end;
   }

   /*
    * Last case: Bitfields/RGB/BGR -> Indexed
    * isn't handled here yet and all the logic is in XLATEOBJ_iXlate now.
    */

end:
   if (PaletteSource != NULL)
      PALETTE_UnlockPalette(SourcePalGDI);
   if (PaletteDest != NULL && PaletteDest != PaletteSource)
      PALETTE_UnlockPalette(DestPalGDI);
   return XlateObj;
}

XLATEOBJ* FASTCALL
IntEngCreateMonoXlate(
   USHORT SourcePalType, HPALETTE PaletteDest, HPALETTE PaletteSource,
   ULONG BackgroundColor)
{
   XLATEOBJ *XlateObj;
   XLATEGDI *XlateGDI;
   PALGDI *SourcePalGDI;

   XlateGDI = EngAllocMem(0, sizeof(XLATEGDI), TAG_XLATEOBJ);
   if (XlateGDI == NULL)
   {
      DPRINT1("Failed to allocate memory for a XLATE structure!\n");
      return NULL;
   }
   XlateObj = GDIToObj(XlateGDI, XLATE);

   SourcePalGDI = PALETTE_LockPalette(PaletteSource);
   /* FIXME - SourcePalGDI can be NULL!!! Handle this case instead of ASSERT! */
   ASSERT(SourcePalGDI);

   if (SourcePalType == 0)
      SourcePalType = SourcePalGDI->Mode;

   XlateObj->iSrcType = SourcePalType;
   XlateObj->iDstType = PAL_INDEXED;

   /* Store handles of palettes in internal Xlate GDI object (or NULLs) */
   XlateGDI->DestPal = PaletteDest;
   XlateGDI->SourcePal = PaletteSource;

   XlateObj->flXlate = XO_TO_MONO;
   XlateObj->cEntries = 1;
   XlateObj->pulXlate = &XlateGDI->BackgroundColor;
   switch (SourcePalType)
   {
      case PAL_INDEXED:
         XlateGDI->BackgroundColor = NtGdiGetNearestPaletteIndex(
            PaletteSource, BackgroundColor);
         break;
      case PAL_BGR:
         XlateGDI->BackgroundColor = BackgroundColor;
         break;
      case PAL_RGB:
         XlateGDI->BackgroundColor =
            ((BackgroundColor & 0xFF) << 16) |
            ((BackgroundColor & 0xFF0000) >> 16) |
            (BackgroundColor & 0xFF00);
         break;
      case PAL_BITFIELDS:
         {
            BitMasksFromPal(SourcePalType, SourcePalGDI, &XlateGDI->RedMask,
               &XlateGDI->BlueMask, &XlateGDI->GreenMask);
            XlateGDI->RedShift = CalculateShift(0xFF) - CalculateShift(XlateGDI->RedMask);
            XlateGDI->GreenShift = CalculateShift(0xFF00) - CalculateShift(XlateGDI->GreenMask);
            XlateGDI->BlueShift = CalculateShift(0xFF0000) - CalculateShift(XlateGDI->BlueMask);
            XlateGDI->BackgroundColor = ShiftAndMask(XlateGDI, BackgroundColor);
         }
         break;
   }

   PALETTE_UnlockPalette(SourcePalGDI);

   return XlateObj;
}

XLATEOBJ* FASTCALL
IntEngCreateSrcMonoXlate(HPALETTE PaletteDest,
                         ULONG ForegroundColor,
                         ULONG BackgroundColor)
{
   XLATEOBJ *XlateObj;
   XLATEGDI *XlateGDI;
   PALGDI *DestPalGDI;

   DestPalGDI = PALETTE_LockPalette(PaletteDest);
   if (DestPalGDI == NULL)
      return NULL;

   XlateGDI = EngAllocMem(0, sizeof(XLATEGDI), TAG_XLATEOBJ);
   if (XlateGDI == NULL)
   {
      PALETTE_UnlockPalette(DestPalGDI);
      DPRINT1("Failed to allocate memory for a XLATE structure!\n");
      return NULL;
   }
   XlateObj = GDIToObj(XlateGDI, XLATE);

   XlateObj->cEntries = 2;
   XlateObj->pulXlate = EngAllocMem(0, sizeof(ULONG) * XlateObj->cEntries, 0);
   if (XlateObj->pulXlate == NULL)
   {
      PALETTE_UnlockPalette(DestPalGDI);
      EngFreeMem(XlateGDI);
      return NULL;
   }

   XlateObj->iSrcType = PAL_INDEXED;
   XlateObj->iDstType = DestPalGDI->Mode;

   /* Store handles of palettes in internal Xlate GDI object (or NULLs) */
   XlateGDI->SourcePal = NULL;
   XlateGDI->DestPal = PaletteDest;

   XlateObj->flXlate = XO_TABLE;

   BitMasksFromPal(DestPalGDI->Mode, DestPalGDI, &XlateGDI->RedMask,
      &XlateGDI->BlueMask, &XlateGDI->GreenMask);

   XlateGDI->RedShift =   CalculateShift(RGB(0xFF, 0x00, 0x00)) - CalculateShift(XlateGDI->RedMask);
   XlateGDI->GreenShift = CalculateShift(RGB(0x00, 0xFF, 0x00)) - CalculateShift(XlateGDI->GreenMask);
   XlateGDI->BlueShift =  CalculateShift(RGB(0x00, 0x00, 0xFF)) - CalculateShift(XlateGDI->BlueMask);

   XlateObj->pulXlate[0] = ShiftAndMask(XlateGDI, BackgroundColor);
   XlateObj->pulXlate[1] = ShiftAndMask(XlateGDI, ForegroundColor);

   if (XlateObj->iDstType == PAL_INDEXED)
   {
      XlateObj->pulXlate[0] =
         ClosestColorMatch(XlateGDI,
                           (LPPALETTEENTRY)&XlateObj->pulXlate[0],
                           DestPalGDI->IndexedColors,
                           DestPalGDI->NumColors);
      XlateObj->pulXlate[1] =
         ClosestColorMatch(XlateGDI,
                           (LPPALETTEENTRY)&XlateObj->pulXlate[1],
                           DestPalGDI->IndexedColors,
                           DestPalGDI->NumColors);
   }

   PALETTE_UnlockPalette(DestPalGDI);

   return XlateObj;
}

HPALETTE FASTCALL
IntEngGetXlatePalette(XLATEOBJ *XlateObj,
                      ULONG Palette)
{
   XLATEGDI *XlateGDI = ObjToGDI(XlateObj, XLATE);
   switch (Palette)
   {
   case XO_DESTPALETTE:
      return XlateGDI->DestPal;
      break;

   case XO_SRCPALETTE:
      return XlateGDI->SourcePal;
      break;
   }
   return 0;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VOID FASTCALL
EngDeleteXlate(XLATEOBJ *XlateObj)
{
   XLATEGDI *XlateGDI;

   if (XlateObj == NULL)
   {
      DPRINT1("Trying to delete NULL XLATEOBJ\n");
      return;
   }

   XlateGDI = ObjToGDI(XlateObj, XLATE);

   if ((XlateObj->flXlate & XO_TABLE) &&
       XlateObj->pulXlate != NULL)
   {
      EngFreeMem(XlateObj->pulXlate);
   }

   EngFreeMem(XlateGDI);
}

/*
 * @implemented
 */
PULONG STDCALL
XLATEOBJ_piVector(XLATEOBJ *XlateObj)
{
   if (XlateObj->iSrcType == PAL_INDEXED)
   {
      return XlateObj->pulXlate;
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
   {
      if (Color >= XlateObj->cEntries)
          Color %= XlateObj->cEntries;

      return XlateObj->pulXlate[Color];
   }


   if (XlateObj->flXlate & XO_TO_MONO)
      return Color == XlateObj->pulXlate[0];

   XlateGDI = ObjToGDI(XlateObj, XLATE);

   if (XlateGDI->UseShiftAndMask)
      return ShiftAndMask(XlateGDI, Color);

   if (XlateObj->iSrcType == PAL_RGB || XlateObj->iSrcType == PAL_BGR ||
       XlateObj->iSrcType == PAL_BITFIELDS)
   {
      /* FIXME: should we cache colors used often? */
      /* FIXME: won't work if destination isn't indexed */

      /* Convert the source color to the palette RGB format. */
      Color = ShiftAndMask(XlateGDI, Color);

      /* Extract the destination palette. */
      PalGDI = PALETTE_LockPalette(XlateGDI->DestPal);
      if(PalGDI != NULL)
      {
         /* Return closest match for the given color. */
         Closest = ClosestColorMatch(XlateGDI, (LPPALETTEENTRY)&Color, PalGDI->IndexedColors, PalGDI->NumColors);
         PALETTE_UnlockPalette(PalGDI);
         return Closest;
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
   ULONG *InPal;

   XlateGDI = ObjToGDI(XlateObj, XLATE);
   if (PalOutType == XO_SRCPALETTE)
      hPalette = XlateGDI->SourcePal;
   else if (PalOutType == XO_DESTPALETTE)
      hPalette = XlateGDI->DestPal;
   else
   {
      UNIMPLEMENTED;
      return 0;
    }

   PalGDI = PALETTE_LockPalette(hPalette);
   if(PalGDI != NULL)
   {
     /* copy the indexed colors into the buffer */

     for(InPal = (ULONG*)PalGDI->IndexedColors;
         cPal > 0;
         cPal--, InPal++, OutPal++)
     {
       *OutPal = *InPal;
     }

     PALETTE_UnlockPalette(PalGDI);

     return cPal;
   }

   return 0;
}

/* EOF */
