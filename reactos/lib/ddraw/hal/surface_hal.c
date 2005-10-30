/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/hal/surface.c
 * PURPOSE:              DirectDraw HAL Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"

HRESULT Hal_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rdst,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rsrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
  
	DDHAL_BLTDATA BltData;
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
 	
	if (!(This->DirectDrawGlobal.lpDDCBtmp->HALDDSurface.dwFlags  & DDHAL_SURFCB32_BLT)) 
	{
		return DDERR_NODRIVERSUPPORT;
	}

	BltData.lpDD = &This->DirectDrawGlobal;
	/* RtlCopyMemory( &BltData.bltFX, lpbltfx,sizeof(DDBLTFX)); */
	BltData.dwFlags =  dwFlags;

    /* FIXME blt is not complete */

	if (This->DirectDrawGlobal.lpDDCBtmp->HALDDSurface.Blt(&BltData) != DDHAL_DRIVER_HANDLED)
	{
	   return DDERR_NODRIVERSUPPORT;
	}
	
	return BltData.ddRVal;

}

