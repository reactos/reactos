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

	 if (pCallBacks !=NULL)
	 {
		 memset(pCallBacks,0,sizeof(DD_CALLBACKS));

		 /* FILL pCallBacks with hal stuff */
	 }

	 if (pSurfaceCallBacks !=NULL)
	 {
		 memset(pSurfaceCallBacks,0,sizeof(DD_SURFACECALLBACKS));

		 /* FILL pSurfaceCallBacks with hal stuff */
	 }

	 if (pPaletteCallBacks !=NULL)
	 {
		 memset(pPaletteCallBacks,0,sizeof(DD_PALETTECALLBACKS));

		 /* FILL pPaletteCallBacks with hal stuff */
	 }
  
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

