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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "framebuf.h"

VOID DDKAPI
DrvDisableDirectDraw( IN DHPDEV  dhpdev)
{
   PPDEV ppdev = (PPDEV)dhpdev;
   ppdev->bDDInitialized = FALSE;
   /* Add Clean up code here if we need it 
      when we shout down directx interface */
}

BOOL DDKAPI
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

	 /* Setup pixel format */	 
	 ppdev->ddpfDisplay.dwSize = sizeof( DDPIXELFORMAT );
     ppdev->ddpfDisplay.dwFourCC = 0;

     ppdev->ddpfDisplay.dwRBitMask = ppdev->RedMask;
     ppdev->ddpfDisplay.dwGBitMask = ppdev->GreenMask;
     ppdev->ddpfDisplay.dwBBitMask = ppdev->BlueMask;

	 switch(ppdev->BitsPerPixel)
	 {
		case BMF_8BPP:
             ppdev->ddpfDisplay.dwRGBAlphaBitMask = 0;
             ppdev->ddpfDisplay.dwRGBBitCount=8;
             ppdev->ddpfDisplay.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
             break;
        
        case BMF_16BPP:
             ppdev->ddpfDisplay.dwRGBBitCount=16;
             switch(ppdev->RedMask)
             {
                case 0x7C00:
                     ppdev->ddpfDisplay.dwRGBAlphaBitMask = 0x8000;
                     break;

                default:
                     ppdev->ddpfDisplay.dwRGBAlphaBitMask = 0;
			 }        
             break;

        case BMF_24BPP:
             ppdev->ddpfDisplay.dwRGBAlphaBitMask = 0;
             ppdev->ddpfDisplay.dwRGBBitCount=24;
             break;

        case BMF_32BPP:
             ppdev->ddpfDisplay.dwRGBAlphaBitMask = 0xff000000;
             ppdev->ddpfDisplay.dwRGBBitCount=32;
             break;
        default:
           /* FIXME unknown pixelformat */
            break;
	 }

   
    //InitDDHAL(ppdev);


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

BOOL DDKAPI
DrvGetDirectDrawInfo(
  IN DHPDEV  dhpdev,
  OUT DD_HALINFO  *pHalInfo,
  OUT DWORD  *pdwNumHeaps,
  OUT VIDEOMEMORY  *pvmList,
  OUT DWORD  *pdwNumFourCCCodes,
  OUT DWORD  *pdwFourCC)
{	
	return FALSE;
}

