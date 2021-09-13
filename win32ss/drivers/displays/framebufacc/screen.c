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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "framebufacc.h"

/*
 * GetAvailableModes
 *
 * Calls the miniport to get the list of modes supported by the kernel driver,
 * and returns the list of modes supported by the display driver.
 */

DWORD
GetAvailableModes(
   HANDLE hDriver,
   PVIDEO_MODE_INFORMATION *ModeInfo,
   DWORD *ModeInfoSize)
{
   ULONG ulTemp;
   VIDEO_NUM_MODES Modes;
   PVIDEO_MODE_INFORMATION ModeInfoPtr;

   /*
    * Get the number of modes supported by the mini-port
    */

   if (EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES, NULL,
                          0, &Modes, sizeof(VIDEO_NUM_MODES), &ulTemp))
   {
      return 0;
   }

   *ModeInfoSize = Modes.ModeInformationLength;

   /*
    * Allocate the buffer for the miniport to write the modes in.
    */

   *ModeInfo = (PVIDEO_MODE_INFORMATION)EngAllocMem(0, Modes.NumModes *
      Modes.ModeInformationLength, ALLOC_TAG);

   if (*ModeInfo == NULL)
   {
      return 0;
   }

   /*
    * Ask the miniport to fill in the available modes.
    */

   if (EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_AVAIL_MODES, NULL, 0,
                          *ModeInfo, Modes.NumModes * Modes.ModeInformationLength,
                          &ulTemp))
   {
      EngFreeMem(*ModeInfo);
      *ModeInfo = (PVIDEO_MODE_INFORMATION)NULL;
      return 0;
   }

   /*
    * Now see which of these modes are supported by the display driver.
    * As an internal mechanism, set the length to 0 for the modes we
    * DO NOT support.
    */

   ulTemp = Modes.NumModes;
   ModeInfoPtr = *ModeInfo;

   /*
    * Mode is rejected if it is not one plane, or not graphics, or is not
    * one of 8, 16 or 32 bits per pel.
    */

   while (ulTemp--)
   {
       /* FIXME add banked graphic mode */
      if ((ModeInfoPtr->NumberOfPlanes != 1) ||
          !(ModeInfoPtr->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
          ((ModeInfoPtr->BitsPerPlane != 8) &&
           (ModeInfoPtr->BitsPerPlane != 16) &&
           (ModeInfoPtr->BitsPerPlane != 24) &&
           (ModeInfoPtr->BitsPerPlane != 32)))
      {
         ModeInfoPtr->Length = 0;
      }

      ModeInfoPtr = (PVIDEO_MODE_INFORMATION)
         (((PUCHAR)ModeInfoPtr) + Modes.ModeInformationLength);
   }

   return Modes.NumModes;
}

BOOL
IntInitScreenInfo(
   PPDEV ppdev,
   LPDEVMODEW pDevMode,
   PGDIINFO pGdiInfo,
   PDEVINFO pDevInfo)
{
   ULONG ModeCount;
   ULONG ModeInfoSize;
   PVIDEO_MODE_INFORMATION ModeInfo, ModeInfoPtr, SelectedMode = NULL;
   VIDEO_COLOR_CAPABILITIES ColorCapabilities;
/* hack
   LOGFONTW SystemFont = {16, 7, 0, 0, 700, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_DONTCARE, L"System"};
   LOGFONTW AnsiVariableFont = {12, 9, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_DONTCARE, L"MS Sans Serif"};
   LOGFONTW AnsiFixedFont = {12, 9, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Courier"};
*/
   ULONG Temp;

   /*
    * Call miniport to get information about video modes.
    */

   ModeCount = GetAvailableModes(ppdev->hDriver, &ModeInfo, &ModeInfoSize);
   if (ModeCount == 0)
   {
      return FALSE;
   }

   /*
    * Select the video mode depending on the info passed in pDevMode.
    */

   if (pDevMode->dmPelsWidth == 0 && pDevMode->dmPelsHeight == 0 &&
       pDevMode->dmBitsPerPel == 0 && pDevMode->dmDisplayFrequency == 0)
   {
      ModeInfoPtr = ModeInfo;
      while (ModeCount-- > 0)
      {
         if (ModeInfoPtr->Length == 0)
         {
            ModeInfoPtr = (PVIDEO_MODE_INFORMATION)
               (((PUCHAR)ModeInfoPtr) + ModeInfoSize);
            continue;
         }
         SelectedMode = ModeInfoPtr;
         break;
      }
   }
   else
   {
      ModeInfoPtr = ModeInfo;
      while (ModeCount-- > 0)
      {
         if (ModeInfoPtr->Length > 0 &&
	     pDevMode->dmPelsWidth == ModeInfoPtr->VisScreenWidth &&
             pDevMode->dmPelsHeight == ModeInfoPtr->VisScreenHeight &&
             pDevMode->dmBitsPerPel == (ModeInfoPtr->BitsPerPlane *
                                        ModeInfoPtr->NumberOfPlanes) &&
             pDevMode->dmDisplayFrequency == ModeInfoPtr->Frequency)
         {
            SelectedMode = ModeInfoPtr;
            break;
         }

         ModeInfoPtr = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)ModeInfoPtr) + ModeInfoSize);
      }
   }

   if (SelectedMode == NULL)
   {
      EngFreeMem(ModeInfo);
      return FALSE;
   }

   /*
    * Fill in the GDIINFO data structure with the information returned from
    * the kernel driver.
    */

   ppdev->ModeIndex = SelectedMode->ModeIndex;
   ppdev->ScreenWidth = SelectedMode->VisScreenWidth;
   ppdev->ScreenHeight = SelectedMode->VisScreenHeight;
   ppdev->ScreenDelta = SelectedMode->ScreenStride;
   ppdev->BitsPerPixel = SelectedMode->BitsPerPlane * SelectedMode->NumberOfPlanes;

   ppdev->MemWidth = SelectedMode->VideoMemoryBitmapWidth;
   ppdev->MemHeight = SelectedMode->VideoMemoryBitmapHeight;

   ppdev->RedMask = SelectedMode->RedMask;
   ppdev->GreenMask = SelectedMode->GreenMask;
   ppdev->BlueMask = SelectedMode->BlueMask;

   pGdiInfo->ulVersion = GDI_DRIVER_VERSION;
   pGdiInfo->ulTechnology = DT_RASDISPLAY;
   pGdiInfo->ulHorzSize = SelectedMode->XMillimeter;
   pGdiInfo->ulVertSize = SelectedMode->YMillimeter;
   pGdiInfo->ulHorzRes = SelectedMode->VisScreenWidth;
   pGdiInfo->ulVertRes = SelectedMode->VisScreenHeight;
   pGdiInfo->ulPanningHorzRes = SelectedMode->VisScreenWidth;
   pGdiInfo->ulPanningVertRes = SelectedMode->VisScreenHeight;
   pGdiInfo->cBitsPixel = SelectedMode->BitsPerPlane;
   pGdiInfo->cPlanes = SelectedMode->NumberOfPlanes;
   pGdiInfo->ulVRefresh = SelectedMode->Frequency;
   pGdiInfo->ulBltAlignment = 1;
   pGdiInfo->ulLogPixelsX = pDevMode->dmLogPixels;
   pGdiInfo->ulLogPixelsY = pDevMode->dmLogPixels;
   pGdiInfo->flTextCaps = TC_RA_ABLE;
   pGdiInfo->flRaster = 0;
   pGdiInfo->ulDACRed = SelectedMode->NumberRedBits;
   pGdiInfo->ulDACGreen = SelectedMode->NumberGreenBits;
   pGdiInfo->ulDACBlue = SelectedMode->NumberBlueBits;
   pGdiInfo->ulAspectX = 0x24;
   pGdiInfo->ulAspectY = 0x24;
   pGdiInfo->ulAspectXY = 0x33;
   pGdiInfo->xStyleStep = 1;
   pGdiInfo->yStyleStep = 1;
   pGdiInfo->denStyleStep = 3;
   pGdiInfo->ptlPhysOffset.x = 0;
   pGdiInfo->ptlPhysOffset.y = 0;
   pGdiInfo->szlPhysSize.cx = 0;
   pGdiInfo->szlPhysSize.cy = 0;

   /*
    * Try to get the color info from the miniport.
    */

   if (!EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES,
                           NULL, 0, &ColorCapabilities,
                           sizeof(VIDEO_COLOR_CAPABILITIES), &Temp))
   {
      pGdiInfo->ciDevice.Red.x = ColorCapabilities.RedChromaticity_x;
      pGdiInfo->ciDevice.Red.y = ColorCapabilities.RedChromaticity_y;
      pGdiInfo->ciDevice.Green.x = ColorCapabilities.GreenChromaticity_x;
      pGdiInfo->ciDevice.Green.y = ColorCapabilities.GreenChromaticity_y;
      pGdiInfo->ciDevice.Blue.x = ColorCapabilities.BlueChromaticity_x;
      pGdiInfo->ciDevice.Blue.y = ColorCapabilities.BlueChromaticity_y;
      pGdiInfo->ciDevice.AlignmentWhite.x = ColorCapabilities.WhiteChromaticity_x;
      pGdiInfo->ciDevice.AlignmentWhite.y = ColorCapabilities.WhiteChromaticity_y;
      pGdiInfo->ciDevice.AlignmentWhite.Y = ColorCapabilities.WhiteChromaticity_Y;
      if (ColorCapabilities.AttributeFlags & VIDEO_DEVICE_COLOR)
      {
         pGdiInfo->ciDevice.RedGamma = ColorCapabilities.RedGamma;
         pGdiInfo->ciDevice.GreenGamma = ColorCapabilities.GreenGamma;
         pGdiInfo->ciDevice.BlueGamma = ColorCapabilities.BlueGamma;
      }
      else
      {
         pGdiInfo->ciDevice.RedGamma = ColorCapabilities.WhiteGamma;
         pGdiInfo->ciDevice.GreenGamma = ColorCapabilities.WhiteGamma;
         pGdiInfo->ciDevice.BlueGamma = ColorCapabilities.WhiteGamma;
      }
   }
   else
   {
      pGdiInfo->ciDevice.Red.x = 6700;
      pGdiInfo->ciDevice.Red.y = 3300;
      pGdiInfo->ciDevice.Green.x = 2100;
      pGdiInfo->ciDevice.Green.y = 7100;
      pGdiInfo->ciDevice.Blue.x = 1400;
      pGdiInfo->ciDevice.Blue.y = 800;
      pGdiInfo->ciDevice.AlignmentWhite.x = 3127;
      pGdiInfo->ciDevice.AlignmentWhite.y = 3290;
      pGdiInfo->ciDevice.AlignmentWhite.Y = 0;
      pGdiInfo->ciDevice.RedGamma = 20000;
      pGdiInfo->ciDevice.GreenGamma = 20000;
      pGdiInfo->ciDevice.BlueGamma = 20000;
   }

   pGdiInfo->ciDevice.Red.Y = 0;
   pGdiInfo->ciDevice.Green.Y = 0;
   pGdiInfo->ciDevice.Blue.Y = 0;
   pGdiInfo->ciDevice.Cyan.x = 0;
   pGdiInfo->ciDevice.Cyan.y = 0;
   pGdiInfo->ciDevice.Cyan.Y = 0;
   pGdiInfo->ciDevice.Magenta.x = 0;
   pGdiInfo->ciDevice.Magenta.y = 0;
   pGdiInfo->ciDevice.Magenta.Y = 0;
   pGdiInfo->ciDevice.Yellow.x = 0;
   pGdiInfo->ciDevice.Yellow.y = 0;
   pGdiInfo->ciDevice.Yellow.Y = 0;
   pGdiInfo->ciDevice.MagentaInCyanDye = 0;
   pGdiInfo->ciDevice.YellowInCyanDye = 0;
   pGdiInfo->ciDevice.CyanInMagentaDye = 0;
   pGdiInfo->ciDevice.YellowInMagentaDye = 0;
   pGdiInfo->ciDevice.CyanInYellowDye = 0;
   pGdiInfo->ciDevice.MagentaInYellowDye = 0;
   pGdiInfo->ulDevicePelsDPI = 0;
   pGdiInfo->ulPrimaryOrder = PRIMARY_ORDER_CBA;
   pGdiInfo->ulHTPatternSize = HT_PATSIZE_4x4_M;
   pGdiInfo->flHTFlags = HT_FLAG_ADDITIVE_PRIMS;

   pDevInfo->flGraphicsCaps = 0;
/* hack
   pDevInfo->lfDefaultFont = SystemFont;
   pDevInfo->lfAnsiVarFont = AnsiVariableFont;
   pDevInfo->lfAnsiFixFont = AnsiFixedFont;
*/
   pDevInfo->cFonts = 0;
   pDevInfo->cxDither = 0;
   pDevInfo->cyDither = 0;
   pDevInfo->hpalDefault = 0;
   pDevInfo->flGraphicsCaps2 = 0;

   if (ppdev->BitsPerPixel == 8)
   {
      pGdiInfo->ulNumColors = 20;
      pGdiInfo->ulNumPalReg = 1 << ppdev->BitsPerPixel;
      pGdiInfo->ulHTOutputFormat = HT_FORMAT_8BPP;
      pDevInfo->flGraphicsCaps |= GCAPS_PALMANAGED;
      pDevInfo->iDitherFormat = BMF_8BPP;
      /* Assuming palette is orthogonal - all colors are same size. */
      ppdev->PaletteShift = 8 - pGdiInfo->ulDACRed;
   }
   else
   {
      pGdiInfo->ulNumColors = (ULONG)(-1);
      pGdiInfo->ulNumPalReg = 0;
      switch (ppdev->BitsPerPixel)
      {
         case 16:
            pGdiInfo->ulHTOutputFormat = HT_FORMAT_16BPP;
            pDevInfo->iDitherFormat = BMF_16BPP;
            break;

         case 24:
            pGdiInfo->ulHTOutputFormat = HT_FORMAT_24BPP;
            pDevInfo->iDitherFormat = BMF_24BPP;
            break;

         default:
            pGdiInfo->ulHTOutputFormat = HT_FORMAT_32BPP;
            pDevInfo->iDitherFormat = BMF_32BPP;
      }
   }

   EngFreeMem(ModeInfo);
   return TRUE;
}

/*
 * DrvGetModes
 *
 * Returns the list of available modes for the device.
 *
 * Status
 *    @implemented
 */

ULONG APIENTRY
DrvGetModes(
   IN HANDLE hDriver,
   IN ULONG cjSize,
   OUT DEVMODEW *pdm)
{
   ULONG ModeCount;
   ULONG ModeInfoSize;
   PVIDEO_MODE_INFORMATION ModeInfo, ModeInfoPtr;
   ULONG OutputSize;

   ModeCount = GetAvailableModes(hDriver, &ModeInfo, &ModeInfoSize);
   if (ModeCount == 0)
   {
      return 0;
   }

   if (pdm == NULL)
   {
      EngFreeMem(ModeInfo);
      return ModeCount * sizeof(DEVMODEW);
   }

   /*
    * Copy the information about supported modes into the output buffer.
    */

   OutputSize = 0;
   ModeInfoPtr = ModeInfo;

   while (ModeCount-- > 0)
   {
      if (ModeInfoPtr->Length == 0)
      {
         ModeInfoPtr = (PVIDEO_MODE_INFORMATION)(((ULONG_PTR)ModeInfoPtr) + ModeInfoSize);
         continue;
      }

      memset(pdm, 0, sizeof(DEVMODEW));
      memcpy(pdm->dmDeviceName, DEVICE_NAME, sizeof(DEVICE_NAME));
      pdm->dmSpecVersion =
      pdm->dmDriverVersion = DM_SPECVERSION;
      pdm->dmSize = sizeof(DEVMODEW);
      pdm->dmDriverExtra = 0;
      pdm->dmBitsPerPel = ModeInfoPtr->NumberOfPlanes * ModeInfoPtr->BitsPerPlane;
      pdm->dmPelsWidth = ModeInfoPtr->VisScreenWidth;
      pdm->dmPelsHeight = ModeInfoPtr->VisScreenHeight;
      pdm->dmDisplayFrequency = ModeInfoPtr->Frequency;
      pdm->dmDisplayFlags = 0;
      pdm->dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                      DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

      ModeInfoPtr = (PVIDEO_MODE_INFORMATION)(((ULONG_PTR)ModeInfoPtr) + ModeInfoSize);
      pdm = (LPDEVMODEW)(((ULONG_PTR)pdm) + sizeof(DEVMODEW));
      OutputSize += sizeof(DEVMODEW);
   }

   EngFreeMem(ModeInfo);
   return OutputSize;
}
