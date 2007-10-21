/*
 * ReactOS Generic Framebuffer display driver
 *
 * Copyright (C) 2004 Filip Navara
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "framebuf.h"

/*
 * Standard color that must be in palette, because they're used for
 * drawing window borders and other GUI elements.
 */

const PALETTEENTRY BASEPALETTE[20] =
{
   { 0x00, 0x00, 0x00, 0x00 },
   { 0x80, 0x00, 0x00, 0x00 },
   { 0x00, 0x80, 0x00, 0x00 },
   { 0x80, 0x80, 0x00, 0x00 },
   { 0x00, 0x00, 0x80, 0x00 },
   { 0x80, 0x00, 0x80, 0x00 },
   { 0x00, 0x80, 0x80, 0x00 },
   { 0xC0, 0xC0, 0xC0, 0x00 },
   { 0xC0, 0xDC, 0xC0, 0x00 },
   { 0xD4, 0xD0, 0xC8, 0x00 },
   { 0xFF, 0xFB, 0xF0, 0x00 },
   { 0x3A, 0x6E, 0xA5, 0x00 },
   { 0x80, 0x80, 0x80, 0x00 },
   { 0xFF, 0x00, 0x00, 0x00 },
   { 0x00, 0xFF, 0x00, 0x00 },
   { 0xFF, 0xFF, 0x00, 0x00 },
   { 0x00, 0x00, 0xFF, 0x00 },
   { 0xFF, 0x00, 0xFF, 0x00 },
   { 0x00, 0xFF, 0xFF, 0x00 },
   { 0xFF, 0xFF, 0xFF, 0x00 },
};

/*
 * IntInitDefaultPalette
 *
 * Initializes default palette for PDEV and fill it with the colors specified
 * by the GDI standard.
 */

BOOL
IntInitDefaultPalette(
   PPDEV ppdev,
   PDEVINFO pDevInfo)
{
   ULONG ColorLoop;
   PPALETTEENTRY PaletteEntryPtr;

   if (ppdev->BitsPerPixel > 8)
   {
      ppdev->DefaultPalette = pDevInfo->hpalDefault =
         EngCreatePalette(PAL_BITFIELDS, 0, NULL,
            ppdev->RedMask, ppdev->GreenMask, ppdev->BlueMask);
   }
   else
   {
      ppdev->PaletteEntries = EngAllocMem(0, sizeof(PALETTEENTRY) << 8, ALLOC_TAG);
      if (ppdev->PaletteEntries == NULL)
      {
         return FALSE;
      }

      for (ColorLoop = 256, PaletteEntryPtr = ppdev->PaletteEntries;
           ColorLoop != 0;
           ColorLoop--, PaletteEntryPtr++)
      {
         PaletteEntryPtr->peRed = ((ColorLoop >> 5) & 7) * 255 / 7;
         PaletteEntryPtr->peGreen = ((ColorLoop >> 3) & 3) * 255 / 3;
         PaletteEntryPtr->peBlue = (ColorLoop & 7) * 255 / 7;
         PaletteEntryPtr->peFlags = 0;
      }

      memcpy(ppdev->PaletteEntries, BASEPALETTE, 10 * sizeof(PALETTEENTRY));
      memcpy(ppdev->PaletteEntries + 246, BASEPALETTE + 10, 10 * sizeof(PALETTEENTRY));

      ppdev->DefaultPalette = pDevInfo->hpalDefault =
         EngCreatePalette(PAL_INDEXED, 256, (PULONG)ppdev->PaletteEntries, 0, 0, 0);
    }

    return ppdev->DefaultPalette != NULL;
}

/*
 * IntSetPalette
 *
 * Requests that the driver realize the palette for a specified device. The
 * driver sets the hardware palette to match the entries in the given palette
 * as closely as possible.
 */

BOOL APIENTRY
IntSetPalette(
   IN DHPDEV dhpdev,
   IN PPALETTEENTRY ppalent,
   IN ULONG iStart,
   IN ULONG cColors)
{
   PVIDEO_CLUT pClut;
   ULONG ClutSize;

   ClutSize = sizeof(VIDEO_CLUT) + (cColors * sizeof(ULONG));
   pClut = EngAllocMem(0, ClutSize, ALLOC_TAG);
   pClut->FirstEntry = iStart;
   pClut->NumEntries = cColors;
   memcpy(&pClut->LookupTable[0].RgbLong, ppalent, sizeof(ULONG) * cColors);

   if (((PPDEV)dhpdev)->PaletteShift)
   {
      while (cColors--)
      {
         pClut->LookupTable[cColors].RgbArray.Red >>= ((PPDEV)dhpdev)->PaletteShift;
         pClut->LookupTable[cColors].RgbArray.Green >>= ((PPDEV)dhpdev)->PaletteShift;
         pClut->LookupTable[cColors].RgbArray.Blue >>= ((PPDEV)dhpdev)->PaletteShift;
         pClut->LookupTable[cColors].RgbArray.Unused = 0;
      }
   }
   else
   {
      while (cColors--)
      {
         pClut->LookupTable[cColors].RgbArray.Unused = 0;
      }
   }

   /*
    * Set the palette registers.
    */

   if (EngDeviceIoControl(((PPDEV)dhpdev)->hDriver, IOCTL_VIDEO_SET_COLOR_REGISTERS,
                          pClut, ClutSize, NULL, 0, &cColors))
   {
      EngFreeMem(pClut);
      return FALSE;
   }

   EngFreeMem(pClut);
   return TRUE;
}

/*
 * DrvSetPalette
 *
 * Requests that the driver realize the palette for a specified device. The
 * driver sets the hardware palette to match the entries in the given palette
 * as closely as possible.
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
DrvSetPalette(
   IN DHPDEV dhpdev,
   IN PALOBJ *ppalo,
   IN FLONG fl,
   IN ULONG iStart,
   IN ULONG cColors)
{
   PPALETTEENTRY PaletteEntries;

   PaletteEntries = EngAllocMem(0, cColors * sizeof(ULONG), ALLOC_TAG);
   if (PaletteEntries == NULL)
   {
      return FALSE;
   }

   if (PALOBJ_cGetColors(ppalo, iStart, cColors, (PULONG)PaletteEntries) !=
       cColors)
   {
      return FALSE;
   }

   return IntSetPalette(dhpdev, PaletteEntries, iStart, cColors);
}
