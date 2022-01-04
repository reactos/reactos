/*
 * ReactOS Generic Framebuffer acclations display driver
 *
 * Copyright (C) 2004 Filip Navara
 * Copyright (C) 2007 Magnus Olsen
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

BOOL
InitSurface(PPDEV ppdev,
            BOOL bForcemapping)
{
   VIDEO_MEMORY VideoMemory;
   VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
   ULONG returnedDataLength;
   ULONG RemappingNeeded = 0;
   ULONG PointerMaxWidth = 0;
   ULONG PointerMaxHeight = 0;

   /*
    * Set video mode of our adapter.
    */

   if (EngDeviceIoControl(ppdev->hDriver,
                          IOCTL_VIDEO_SET_CURRENT_MODE,
                          &(ppdev->ModeIndex),
                          sizeof(ULONG),
                          &RemappingNeeded,
                          sizeof(ULONG),
                          &returnedDataLength))
   {
      return FALSE;
   }

   /* Check if mapping is need it */
   if ((!bForcemapping) &&
       (!RemappingNeeded))
   {
       return TRUE;
   }


   /*
    * Map the framebuffer into our memory.
    */

   VideoMemory.RequestedVirtualAddress = NULL;
   if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                          &VideoMemory, sizeof(VIDEO_MEMORY),
                          &VideoMemoryInfo, sizeof(VIDEO_MEMORY_INFORMATION),
                          &returnedDataLength))
   {
      return FALSE;
   }

   /*
    * Save the real video memory
    */
    ppdev->pRealVideoMem = VideoMemoryInfo.FrameBufferBase;
    ppdev->VideoMemSize = VideoMemoryInfo.VideoRamLength;


   /*
    * Video memory cached
    *
    * We maby should only ask max 8MB as cached ?, think of the video ram length is 256MB
    */

    ppdev->pVideoMemCache = NULL;
#ifdef EXPERIMENTAL_ACC_SUPPORT

   ppdev->pVideoMemCache = EngAllocMem(0, (ULONG)VideoMemoryInfo.VideoRamLength, ALLOC_TAG);
   if (ppdev->pVideoMemCache == NULL)
   {
        /* cached off for no avail system memory */
        ppdev->ScreenPtr = VideoMemoryInfo.FrameBufferBase;
   }
   else

#endif
   {
        /* cached on, system memory is avail */
        ppdev->ScreenPtr = ppdev->pRealVideoMem;
   }

   /* hw mouse pointer support */
   PointerMaxHeight = ppdev->PointerCapabilities.MaxHeight;
   PointerMaxWidth = ppdev->PointerCapabilities.MaxWidth * sizeof(ULONG);
   if (ppdev->PointerCapabilities.Flags & VIDEO_MODE_COLOR_POINTER)
   {
        PointerMaxWidth = (ppdev->PointerCapabilities.MaxWidth + 7) / 8;
   }

   ppdev->PointerAttributesSize = sizeof(VIDEO_POINTER_ATTRIBUTES) + ((sizeof(UCHAR) * PointerMaxWidth * PointerMaxHeight) << 1);

   ppdev->pPointerAttributes = EngAllocMem(0, ppdev->PointerAttributesSize, ALLOC_TAG);

   if (ppdev->pPointerAttributes != NULL)
   {
        ppdev->pPointerAttributes->Flags = ppdev->PointerCapabilities.Flags;
        ppdev->pPointerAttributes->WidthInBytes = PointerMaxWidth;
        ppdev->pPointerAttributes->Width = ppdev->PointerCapabilities.MaxWidth;
        ppdev->pPointerAttributes->Height = PointerMaxHeight;
        ppdev->pPointerAttributes->Column = 0;
        ppdev->pPointerAttributes->Row = 0;
        ppdev->pPointerAttributes->Enable = 0;
   }
   else
   {
       /* no hw mouse was avail */
       ppdev->PointerAttributesSize = 0;
   }

   return TRUE;
}

/*
 * DrvEnableSurface
 *
 * Create engine bitmap around frame buffer and set the video mode requested
 * when PDEV was initialized.
 *
 * Status
 *    @implemented
 */

HSURF APIENTRY
DrvEnableSurface(
   IN DHPDEV dhpdev)
{
   PPDEV ppdev = (PPDEV)dhpdev;
   HSURF hSurface;
   ULONG BitmapType;
   SIZEL ScreenSize;


   /* Setup surface and force the mapping */
   if (!InitSurface(ppdev, TRUE))
   {
       return FALSE;
   }

   /* Rest the desktop vitual position */
   ppdev->ScreenOffsetXY.x = 0;
   ppdev->ScreenOffsetXY.y = 0;


   switch (ppdev->BitsPerPixel)
   {
      case 8:
         IntSetPalette(dhpdev, ppdev->PaletteEntries, 0, 256);
         BitmapType = BMF_8BPP;
         break;

      case 16:
         BitmapType = BMF_16BPP;
         break;

      case 24:
         BitmapType = BMF_24BPP;
         break;

      case 32:
         BitmapType = BMF_32BPP;
         break;

      default:
         return FALSE;
   }

   ppdev->iDitherFormat = BitmapType;

   ScreenSize.cx = ppdev->ScreenWidth;
   ScreenSize.cy = ppdev->ScreenHeight;

   hSurface = (HSURF)EngCreateBitmap(ScreenSize, ppdev->ScreenDelta, BitmapType,
                                     (ppdev->ScreenDelta > 0) ? BMF_TOPDOWN : 0,
                                     ppdev->ScreenPtr);
   if (hSurface == NULL)
   {
      return FALSE;
   }

   /* Which api we hooking to */
   ppdev->dwHooks = HOOK_BITBLT | HOOK_COPYBITS | HOOK_FILLPATH | HOOK_TEXTOUT | HOOK_STROKEPATH | HOOK_LINETO ;

   /*
    * Associate the surface with our device.
    */


   if (!EngAssociateSurface(hSurface, ppdev->hDevEng, ppdev->dwHooks))
   {
      EngDeleteSurface(hSurface);
      return FALSE;
   }

   ppdev->hSurfEng = hSurface;

   return hSurface;
}

/*
 * DrvDisableSurface
 *
 * Used by GDI to notify a driver that the surface created by DrvEnableSurface
 * for the current device is no longer needed.
 *
 * Status
 *    @implemented
 */

VOID APIENTRY
DrvDisableSurface(
   IN DHPDEV dhpdev)
{
   DWORD ulTemp;
   VIDEO_MEMORY VideoMemory;
   PPDEV ppdev = (PPDEV)dhpdev;

   EngDeleteSurface(ppdev->hSurfEng);
   ppdev->hSurfEng = NULL;


   /* Free the video memory cache */
   if (ppdev->pVideoMemCache)
   {
        EngFreeMem(ppdev->pVideoMemCache);
   }

   /*
    * Unmap the framebuffer.
    */

   VideoMemory.RequestedVirtualAddress = ppdev->pRealVideoMem;
   EngDeviceIoControl(((PPDEV)dhpdev)->hDriver, IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                      &VideoMemory, sizeof(VIDEO_MEMORY), NULL, 0, &ulTemp);

   ppdev->pRealVideoMem  = NULL;
   ppdev->pVideoMemCache = NULL;

}

/*
 * DrvAssertMode
 *
 * Sets the mode of the specified physical device to either the mode specified
 * when the PDEV was initialized or to the default mode of the hardware.
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
DrvAssertMode(
   IN DHPDEV dhpdev,
   IN BOOL bEnable)
{
   PPDEV ppdev = (PPDEV)dhpdev;
   ULONG ulTemp;
   BOOLEAN Result = TRUE;

   if (bEnable)
   {
      PVOID pRealVideoMem = ppdev->pRealVideoMem;

      /* Setup surface and remapping if it need it */
      if (!InitSurface(ppdev, FALSE))
      {
            Result = FALSE;
      }
      else
      {
            /* Check if we got same surface or not */
            if (pRealVideoMem != ppdev->pRealVideoMem)
            {
                PVOID pVideoMem= NULL;

                if (ppdev->pVideoMemCache == NULL)
                {
                    pVideoMem = ppdev->pRealVideoMem;
                }
                else
                {
                    pVideoMem = ppdev->pVideoMemCache;
                }

                Result = !EngModifySurface(ppdev->hSurfEng, ppdev->hDevEng,
                                           ppdev->dwHooks | HOOK_SYNCHRONIZE,
                                           0, (DHSURF)ppdev, pVideoMem,
                                           ppdev->ScreenDelta, NULL);
            }

            /* if the pRealVideoMem == ppdev->pRealVideoMem are
             * the Result is then TRUE
             */
      }

   }
   else
   {
      /*
       * Call the miniport driver to reset the device to a known state.
       */

      Result = !EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_RESET_DEVICE,
                                 NULL, 0, NULL, 0, &ulTemp);
   }

   return Result;
}
