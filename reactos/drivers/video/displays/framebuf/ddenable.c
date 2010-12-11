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
        pCallBacks->dwFlags               = DDHAL_CB32_MAPMEMORY |
                                            DDHAL_CB32_CANCREATESURFACE |
                                            DDHAL_CB32_CREATESURFACE ;
        //                                = DDHAL_CB32_WAITFORVERTICALBLANK |
        //                                  DDHAL_CB32_GETSCANLINE |

        pCallBacks->CanCreateSurface      = (PDD_CANCREATESURFACE) DdCanCreateSurface;
        pCallBacks->CreateSurface         = (PDD_CREATESURFACE) DdCreateSurface;
        pCallBacks->MapMemory             = (PDD_MAPMEMORY) DdMapMemory;
        // pCallBacks->SetColorKey           = (PDD_SETCOLORKEY) DdSetColorKey;
        // pCallBacks->WaitForVerticalBlank  = (PDD_WAITFORVERTICALBLANK) DdWaitForVerticalBlank;
        // pCallBacks->CreatePalette         = (PDD_CREATEPALETTE) DdCreatePalette;
        // pCallBacks->GetScanLine           = (PDD_GETSCANLINE) DdGetScanLine;


        /* Unused on Microsoft Windows 2000 and later and should be ignored by the driver.  '
            pCallBacks->DestroyDriver
            pCallBacks->SetMode
         */

    }

    if (pSurfaceCallBacks !=NULL)
    {
        memset(pSurfaceCallBacks,0,sizeof(DD_SURFACECALLBACKS));

        /* FILL pSurfaceCallBacks with hal stuff */
        pSurfaceCallBacks->dwSize              = sizeof(DDHAL_DDSURFACECALLBACKS);
        //                                          DDHAL_SURFCB32_DESTROYSURFACE |
        //                                          DDHAL_SURFCB32_FLIP     |
        //                                          DDHAL_SURFCB32_LOCK     |
        //                                          DDHAL_SURFCB32_BLT |
        //                                          DDHAL_SURFCB32_GETBLTSTATUS |
        //                                          DDHAL_SURFCB32_GETFLIPSTATUS;
        // pSurfaceCallBacks->DestroySurface      = (PDD_SURFCB_DESTROYSURFACE) DdDestroySurface;
        // pSurfaceCallBacks->Flip                = (PDD_SURFCB_FLIP) DdFlip;
        // pSurfaceCallBacks->SetClipList         = (PDD_SURFCB_SETCLIPLIST) DdSetClipList;
        // pSurfaceCallBacks->Lock                = (PDD_SURFCB_LOCK ) DdLock;
        // pSurfaceCallBacks->Unlock              = (PDD_SURFCB_UNLOCK) DdUnlock;
        // pSurfaceCallBacks->Blt                 = (PDD_SURFCB_BLT) DdBlt;
        // pSurfaceCallBacks->SetColorKey         = (PDD_SURFCB_SETCOLORKEY) DdSetColorKey;
        // pSurfaceCallBacks->AddAttachedSurface  = (PDD_SURFCB_ADDATTACHEDSURFACE) DdAddAttachedSurface;
        // pSurfaceCallBacks->GetBltStatus        = (PDD_SURFCB_GETBLTSTATUS) DdGetBltStatus;
        // pSurfaceCallBacks->GetFlipStatus       = (PDD_SURFCB_GETFLIPSTATUS) DdGetFlipStatus;
        // pSurfaceCallBacks->UpdateOverlay       = (PDD_SURFCB_UPDATEOVERLAY) DdUpdateOverlay;
        // pSurfaceCallBacks->SetOverlayPosition  = (PDD_SURFCB_SETOVERLAYPOSITION) DdSetOverlayPosition;
        // pSurfaceCallBacks->SetPalette          = (PDD_SURFCB_SETPALETTE) DdSetPalette;
    }

    if (pPaletteCallBacks !=NULL)
    {
        memset(pPaletteCallBacks,0,sizeof(DD_PALETTECALLBACKS));
        /* FILL pPaletteCallBacks with hal stuff */
        pPaletteCallBacks->dwSize               = sizeof(DD_PALETTECALLBACKS);
        // pPaletteCallBacks->DestroyPalette    = (PDD_PALCB_DESTROYPALETTE) DdDestorypalette;
        // pPaletteCallBacks->SetEntries        = (PDD_PALCB_SETENTRIES) DdSetentries;
    }

    ppdev->bDDInitialized = TRUE;
    return ppdev->bDDInitialized;
}

/*
 * Note DrvGetDirectDrawInfo
 *   ms dxg.sys call DrvGetDirectDrawInfo before it call DrvEnableDirectDraw
 *   so we can not check if the drv have enable the directx or not
 *
 * Optimze tip ms dxg.sys frist time it call on this api the
 *   pdwNumHeaps and pdwFourCC is NULL,
 *   next time it call it have alloc memmory for them
 *   in the EDD_DIRECTDRAW_GLOBAL in their memory
 *   pdwFourCC have pool tag 'fddG' and
 *   pvmList  have pool tag 'vddG'
 *   so we only need fill pHalInfo,pdwNumHeaps,pdwNumFourCCCodes in the
 *   frist call.
 *
 * Importet do not forget to fill in pHalInfo->vmiData.pvPrimary if we
 *  forget that no directx hardware acclartions.
 */
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


    /*
        check see if it is our frist call
        if it is fill pHalInfo and pdwNumHeaps and  pdwNumFourCCCodes.

        if it our second call we must fill in pdwFourCC or pvmList
        if one or two of them have been set.

        But we do not support heap or any fourcc so we skipp it
    */

    if( (!pvmList) && (!pdwFourCC) )
    {
        /*  Setup heap */
        ppdev->dwHeap = 0;
        *pdwNumHeaps = 0;

        /* We do not support other fourcc */
        *pdwNumFourCCCodes = 0;


        /* not document ms dxg.sys call this api twice, frist time it call, the pvmList and pdwFourCC is NULL */
        RtlZeroMemory(pHalInfo, sizeof(DD_HALINFO));
        pHalInfo->dwSize = sizeof(DD_HALINFO);

        pHalInfo->ddCaps.dwCaps = 0;
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

        /* if we do not fill in pHalInfo->vmiData.pvPrimary dxg.sys will not activate the dx */
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

    return TRUE;
}

