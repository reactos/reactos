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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "framebuf.h"

BOOL 
InitSurface(PPDEV ppdev,
            BOOL bForcemapping)
{
   VIDEO_MEMORY VideoMemory;
   VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
   ULONG returnedDataLength;
   ULONG RemappingNeeded = 0;

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
   ppdev->pVideoMemCache = EngAllocMem(0, (ULONG)VideoMemoryInfo.VideoRamLength, ALLOC_TAG);
   if (ppdev->pVideoMemCache == NULL)
   {
        /* cached off for no avail system memory */
        ppdev->ScreenPtr = VideoMemoryInfo.FrameBufferBase;
   }
   else
   {
        /* cached on, system memory is avail */
        ppdev->ScreenPtr = ppdev->pRealVideoMem;
   }

   /* FIXME  hw mouse pointer */

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

   /*
    * Associate the surface with our device.
    */

   if (!EngAssociateSurface(hSurface, ppdev->hDevEng, HOOK_BITBLT | HOOK_COPYBITS |
                                                      HOOK_FILLPATH | HOOK_TEXTOUT |
                                                      HOOK_STROKEPATH | HOOK_LINETO))
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

   if (bEnable)
   {
      BOOLEAN Result;
      PBYTE   pRealVideoMem = ppdev->pRealVideoMem;

      /* Setup surface and remapping if it need it */
      if (!InitSurface(ppdev, FALSE))
      {
            return FALSE;
      }

      /* Check if we got same surface or not */
      if (pRealVideoMem != ppdev->pRealVideoMem)
      {
             if (ppdev->pVideoMemCache == NULL)
             {
                if ( !EngModifySurface(ppdev->hsurfEng,
                                   ppdev->hdevEng,
                                   ppdev->flHooks | HOOK_SYNCHRONIZE,
                                   MS_NOTSYSTEMMEMORY,
                                   (DHSURF)ppdev,
                                   ppdev->pRealVideoMem,
                                   ppdev->lDeltaScreen,
                                   NULL))
                {
                    return FALSE;
                }
             }
             else
             {
                if ( !EngModifySurface(ppdev->hsurfEng,
                                   ppdev->hdevEng,
                                   ppdev->flHooks | HOOK_SYNCHRONIZE,
                                   0,
                                   (DHSURF)ppdev,
                                   ppdev->pVideoMemCache,
                                   ppdev->lDeltaScreen,
                                   NULL))
                {
                    return FALSE;
                }
             }
      }

      return TRUE;

   }
   else
   {
      /*
       * Call the miniport driver to reset the device to a known state.
       */

      return !EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_RESET_DEVICE,
                                 NULL, 0, NULL, 0, &ulTemp);
   }
}
