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


HRESULT Hal_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rDest,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rSrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
	DX_STUB;

	IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
    IDirectDrawSurfaceImpl* That = (IDirectDrawSurfaceImpl*)src;
 	
	if (!(This->owner->mDDrawGlobal.lpDDCBtmp->HALDDSurface.dwFlags  & DDHAL_SURFCB32_BLT)) 
	{
		return DDERR_NODRIVERSUPPORT;
	}

	DDHAL_BLTDATA BltData;
	BltData.lpDD = &This->owner->mDDrawGlobal;
	BltData.dwFlags = dwFlags;
	BltData.lpDDDestSurface = &This->Local;
    if(rDest) BltData.rDest = *(RECTL*)rDest;
    if(rSrc) BltData.rSrc = *(RECTL*)rSrc;
    if(That) BltData.lpDDSrcSurface = &That->Local;
	if(lpbltfx) BltData.bltFX = *lpbltfx;

	if (This->owner->mDDrawGlobal.lpDDCBtmp->HALDDSurface.Blt(&BltData) != DDHAL_DRIVER_HANDLED)
	{
	   return DDERR_NODRIVERSUPPORT;
	}
	
	return BltData.ddRVal;
}
