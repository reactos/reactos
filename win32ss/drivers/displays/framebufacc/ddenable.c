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

	 if (ppdev->bDDInitialized)
	 {
		 return TRUE;
	 }

	 /* Setup pixel format */
	 ppdev->ddpfDisplay.dwSize = sizeof( DDPIXELFORMAT );
     ppdev->ddpfDisplay.dwFourCC = 0;

     ppdev->ddpfDisplay.dwRBitMask = ppdev->RedMask;
     ppdev->ddpfDisplay.dwGBitMask = ppdev->GreenMask;
     ppdev->ddpfDisplay.dwBBitMask = ppdev->BlueMask;

     ppdev->ddpfDisplay.dwRGBBitCount=ppdev->BitsPerPixel;
	 ppdev->ddpfDisplay.dwRGBAlphaBitMask = 0;
	 ppdev->ddpfDisplay.dwFlags = DDPF_RGB;

	 ppdev->pvmList = NULL;

	 switch(ppdev->iDitherFormat)
	 {
		case BMF_8BPP:
             ppdev->ddpfDisplay.dwFlags  |= DDPF_PALETTEINDEXED8;
             break;

        case BMF_16BPP:
             switch(ppdev->RedMask)
             {
                case 0x7C00:
                     ppdev->ddpfDisplay.dwRGBAlphaBitMask = 0x8000;
                     break;

                default:
                     break;
			 }
             break;

        case BMF_24BPP:
             break;

        case BMF_32BPP:
             ppdev->ddpfDisplay.dwRGBAlphaBitMask = 0xff000000;
             break;

        default:
           /* FIXME unknown pixel bits */
			ppdev->ddpfDisplay.dwRGBBitCount=0;
            break;
	 }

	 if (pCallBacks !=NULL)
	 {
		 memset(pCallBacks,0,sizeof(DD_CALLBACKS));

		 /* FILL pCallBacks with hal stuff */
         pCallBacks->dwSize = sizeof(DDHAL_DDCALLBACKS);
         pCallBacks->CanCreateSurface = (PDD_CANCREATESURFACE)DdCanCreateSurface;
         pCallBacks->CreateSurface =  (PDD_CREATESURFACE)DdCreateSurface;

         /* Fill in the HAL Callback flags */
         pCallBacks->dwFlags = DDHAL_CB32_CANCREATESURFACE | DDHAL_CB32_CREATESURFACE;
	 }

	 if (pSurfaceCallBacks !=NULL)
	 {
		 memset(pSurfaceCallBacks,0,sizeof(DD_SURFACECALLBACKS));

		 /* FILL pSurfaceCallBacks with hal stuff */
         // pSurfaceCallBacks.dwSize = sizeof(DDHAL_DDSURFACECALLBACKS);
         // pSurfaceCallBacks.DestroySurface = DdDestroySurface;
         // pSurfaceCallBacks.Lock = DdLock;
         // pSurfaceCallBacks.Blt = DdBlt;

        // pSurfaceCallBacks->dwFlags = DDHAL_SURFCB32_DESTROYSURFACE | DDHAL_SURFCB32_LOCK | DDHAL_SURFCB32_BLT ;
	 }

	 if (pPaletteCallBacks !=NULL)
	 {
		 memset(pPaletteCallBacks,0,sizeof(DD_PALETTECALLBACKS));
		 /* FILL pPaletteCallBacks with hal stuff */
		 /* We will not support this callback in the framebuf.dll */
	 }


	 /* Fixme fill the ppdev->dxHalInfo with the info we need */
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
	LONG i;
	DWORD heap = 1; /* we always alloc one heap */
	BOOL bDDrawHeap = FALSE;

	if  (ppdev == NULL)
        return FALSE;

	/*   check so pHalInfo,  pdwNumHeaps, pdwNumFourCCCodes is not NULL
	     pdwFourCC and pvmList can be null
	 */

	if (pHalInfo == NULL)
		return FALSE;

	if (pdwNumHeaps == NULL)
		return FALSE;

	if (pdwNumFourCCCodes == NULL)
		return FALSE;

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

	    /*  FIXME
		    Config the rops we do not doing that yet
		     for we need write the rops table
        */
        for(i=0;i<DD_ROP_SPACE;i++ )
        {
           // pHALInfo->ddCaps.dwSVBRops[i] = rops[i];
		  //  pHALInfo->ddCaps.dwRops[i] = rops[i];
        }
	}

	/* Now build pvmList info */
	if(pvmList)
	{
		ppdev->pvmList = pvmList;

		if (bDDrawHeap)
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

