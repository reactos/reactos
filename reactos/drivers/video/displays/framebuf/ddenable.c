/*
 * ReactOS Generic Framebuffer display driver directdraw interface
 *
 * Copyright (C) 2006 Magnus Olsen
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

#include "framebuf.h"

VOID APIENTRY
DrvDisableDirectDraw( IN DHPDEV  dhpdev)
{
   PPDEV ppdev = (PPDEV)dhpdev;
   ppdev->bDDInitialized = FALSE;
   /* Add Clean up code here if we need it
      when we shout down directx interface */
}

BOOL APIENTRY
DrvEnableDirectDraw(
  IN DHPDEV  dhpdev,
  OUT DD_CALLBACKS  *pCallBacks,
  OUT DD_SURFACECALLBACKS  *pSurfaceCallBacks,
  OUT DD_PALETTECALLBACKS  *pPaletteCallBacks)
{
    PPDEV ppdev = (PPDEV)dhpdev;

    if (ppdev->bDDInitialized == TRUE)
    {
        return TRUE;
    }

    if (pCallBacks !=NULL)
    {
        memset(pCallBacks,0,sizeof(DD_CALLBACKS));
        /* Fill in the HAL Callback pointers */

        pCallBacks->dwSize                = sizeof(DD_CALLBACKS);
        pCallBacks->dwFlags               = DDHAL_CB32_CANCREATESURFACE |
                                            DDHAL_CB32_CREATESURFACE ;
        //                                = DDHAL_CB32_WAITFORVERTICALBLANK |
        //                                  DDHAL_CB32_MAPMEMORY |
        //                                  DDHAL_CB32_GETSCANLINE |

        pCallBacks->CreateSurface         = DdCreateSurface;
        // pCallBacks->SetColorKey           = DdSetColorKey;
        // pCallBacks->WaitForVerticalBlank  = DdWaitForVerticalBlank;
        pCallBacks->CanCreateSurface      = DdCanCreateSurface;
        // pCallBacks->CreatePalette         = DdCreatePalette;
        // pCallBacks->GetScanLine           = DdGetScanLine;
        // pCallBacks->MapMemory             = DdMapMemory;

        /* Unused on Microsoft Windows 2000 and later and should be ignored by the driver.  '
            pCallBacks->DestroyDriver
            pCallBacks->SetMode
         */

    }

    if (pSurfaceCallBacks !=NULL)
    {
        memset(pSurfaceCallBacks,0,sizeof(DD_SURFACECALLBACKS));

        /* FILL pSurfaceCallBacks with hal stuff */
        // pSurfaceCallBacks->dwSize              = sizeof(DDHAL_DDSURFACECALLBACKS);
        //                                          DDHAL_SURFCB32_DESTROYSURFACE |
        //                                          DDHAL_SURFCB32_FLIP     |
        //                                          DDHAL_SURFCB32_LOCK     |
        //                                          DDHAL_SURFCB32_BLT |
        //                                          DDHAL_SURFCB32_GETBLTSTATUS |
        //                                          DDHAL_SURFCB32_GETFLIPSTATUS;
        // pSurfaceCallBacks->DestroySurface      = DdDestroySurface;
        // pSurfaceCallBacks->Flip                = DdFlip;
        // pSurfaceCallBacks->SetClipList         = DdSetClipList;
        // pSurfaceCallBacks->Lock                = DdLock;
        // pSurfaceCallBacks->Unlock              = DdUnlock;
        // pSurfaceCallBacks->Blt                 = DdBlt;
        // pSurfaceCallBacks->SetColorKey         = DdSetColorKey;
        // pSurfaceCallBacks->AddAttachedSurface  = DdAddAttachedSurface;
        // pSurfaceCallBacks->GetBltStatus        = DdGetBltStatus;
        // pSurfaceCallBacks->GetFlipStatus       = DdGetFlipStatus;
        // pSurfaceCallBacks->UpdateOverlay       = DdUpdateOverlay;
        // pSurfaceCallBacks->SetOverlayPosition  = DdSetOverlayPosition;
        // pSurfaceCallBacks->SetPalette          = DdSetPalette;
    }

    if (pPaletteCallBacks !=NULL)
    {
        memset(pPaletteCallBacks,0,sizeof(DD_PALETTECALLBACKS));
        /* FILL pPaletteCallBacks with hal stuff */
        pPaletteCallBacks->dwSize           = sizeof(DD_PALETTECALLBACKS);
        pPaletteCallBacks->dwFlags          = 0;
        // pPaletteCallBacks->DestroyPalette;
        // pPaletteCallBacks->SetEntries;
    }

    ppdev->bDDInitialized = TRUE;
    return ppdev->bDDInitialized;
}

BOOL APIENTRY
DrvGetDirectDrawInfo(
  IN DHPDEV  dhpdev,
  OUT DD_HALINFO  *pHalInfo,
  OUT DWORD  *pdwNumHeaps,
  OUT VIDEOMEMORY  *pvmList,
  OUT DWORD  *pdwNumFourCCCodes,
  OUT DWORD  *pdwFourCC)
{
    PPDEV ppdev = (PPDEV)dhpdev;
    DWORD heap = 1; /* we always alloc one heap */
    BOOL bDDrawHeap = FALSE;

    if (ppdev->bDDInitialized == FALSE);
    {
        return FALSE;
    }

    /*  Setup heap */
    if ( (ppdev->ScreenWidth < ppdev->MemWidth) || (ppdev->ScreenHeight < ppdev->MemHeight))
    {
       bDDrawHeap = TRUE;
       heap++;
    }

    ppdev->dwHeap = heap;
    *pdwNumHeaps  = heap;

    /* We do not support other fourcc */
    *pdwNumFourCCCodes = 0;


    /*
        check see if pvmList and pdwFourCC are frist call
        or frist. Secon call  we fill in pHalInfo info
    */

    if(!(pvmList && pdwFourCC))
    {
        RtlZeroMemory(pHalInfo, sizeof(DD_HALINFO));
        pHalInfo->dwSize = sizeof(DD_HALINFO);

        pHalInfo->ddCaps.dwCaps = DDCAPS_NOHARDWARE;
        /* we do not support all this caps
        pHalInfo->ddCaps.dwCaps =  DDCAPS_BLT        | DDCAPS_BLTQUEUE | DDCAPS_BLTCOLORFILL | DDCAPS_READSCANLINE |
                                   DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY | DDCAPS_CANBLTSYSMEM;

        pHalInfo->ddCaps.dwFXCaps = DDFXCAPS_BLTSTRETCHY     | DDFXCAPS_BLTSTRETCHX        |
                                    DDFXCAPS_BLTSTRETCHYN    | DDFXCAPS_BLTSTRETCHXN       |
                                    DDFXCAPS_BLTSHRINKY      | DDFXCAPS_BLTSHRINKX         |
                                    DDFXCAPS_BLTSHRINKYN     | DDFXCAPS_BLTSHRINKXN        |
                                    DDFXCAPS_BLTMIRRORUPDOWN | DDFXCAPS_BLTMIRRORLEFTRIGHT;

        pHalInfo->ddCaps.dwCaps2 = DDCAPS2_NONLOCALVIDMEM | DDCAPS2_NONLOCALVIDMEMCAPS;

        pHalInfo->ddCaps.ddsCaps.dwCaps =  DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP;

        pHalInfo->ddCaps.dwCKeyCaps = DDCKEYCAPS_SRCBLT | DDCKEYCAPS_SRCBLTCLRSPACE;

        pHalInfo->ddCaps.dwSVBCaps = DDCAPS_BLT;
        pHalInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM;
        */

        pHalInfo->ddCaps.ddsCaps.dwCaps =  DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP;

        /* Calc how much memmory is left on the video cards memmory */
        pHalInfo->ddCaps.dwVidMemTotal = (ppdev->MemHeight - ppdev->ScreenHeight) * ppdev->ScreenDelta;

        /* fill in some basic info that we need */
        pHalInfo->vmiData.pvPrimary                 = ppdev->ScreenPtr;
        pHalInfo->vmiData.dwDisplayWidth            = ppdev->ScreenWidth;
        pHalInfo->vmiData.dwDisplayHeight           = ppdev->ScreenHeight;
        pHalInfo->vmiData.lDisplayPitch             = ppdev->ScreenDelta;
        pHalInfo->vmiData.ddpfDisplay.dwSize        = sizeof(DDPIXELFORMAT);
        pHalInfo->vmiData.ddpfDisplay.dwFlags       = DDPF_RGB;
        pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = ppdev->BitsPerPixel;
        pHalInfo->vmiData.ddpfDisplay.dwRBitMask    = ppdev->RedMask;
        pHalInfo->vmiData.ddpfDisplay.dwGBitMask    = ppdev->GreenMask;
        pHalInfo->vmiData.ddpfDisplay.dwBBitMask    = ppdev->BlueMask;
        pHalInfo->vmiData.dwOffscreenAlign = 4;

        if ( ppdev->BitsPerPixel == 8 )
        {
            pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
        }

        if ( ppdev->BitsPerPixel == 4 )
        {
            pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED4;
        }
    }

    /* Now build pvmList info */
    if(pvmList)
    {
        ppdev->pvmList = pvmList;

        if ( bDDrawHeap == TRUE)
        {
            pvmList->dwFlags        = VIDMEM_ISLINEAR ;
            pvmList->fpStart        = ppdev->ScreenHeight * ppdev->ScreenDelta;
            pvmList->fpEnd          = ppdev->MemHeight * ppdev->ScreenDelta - 1;
            pvmList->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
            pvmList++;
        }

        pvmList->fpStart = 0;
        pvmList->fpEnd = (ppdev->MemHeight * ppdev->ScreenDelta)  - 1;
        pvmList->dwFlags = VIDMEM_ISNONLOCAL | VIDMEM_ISLINEAR | VIDMEM_ISWC;
        pvmList->ddsCaps.dwCaps =  DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER ;
        pvmList->ddsCapsAlt.dwCaps = DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER;

        pvmList = ppdev->pvmList;
    }

    return TRUE;
}

