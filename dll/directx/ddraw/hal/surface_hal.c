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

HRESULT Hal_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD2)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

	if(sizeof(DDSURFACEDESC2) != pDDSD2->dwSize)
		return DDERR_UNSUPPORTED;

	This->owner = (IDirectDrawImpl*)pDD;	

	/************ fill the discription of our primary surface ***********************/  

	memset (&This->ddsd, 0, sizeof(DDSURFACEDESC));
	This->ddsd.dwSize = sizeof(DDSURFACEDESC);

	RtlCopyMemory(&This->ddsd.ddckCKDestBlt,&pDDSD2->ddckCKDestBlt,sizeof(This->ddsd.ddckCKDestBlt));
	RtlCopyMemory(&This->ddsd.ddckCKDestOverlay,&pDDSD2->ddckCKDestOverlay,sizeof(This->ddsd.ddckCKDestOverlay));
	RtlCopyMemory(&This->ddsd.ddckCKSrcBlt,&pDDSD2->ddckCKSrcBlt,sizeof(This->ddsd.ddckCKSrcBlt));
	RtlCopyMemory(&This->ddsd.ddckCKSrcOverlay,&pDDSD2->ddckCKSrcOverlay,sizeof(This->ddsd.ddckCKSrcOverlay));
	RtlCopyMemory(&This->ddsd.ddpfPixelFormat,&pDDSD2->ddpfPixelFormat,sizeof(This->ddsd.ddpfPixelFormat));
	RtlCopyMemory(&This->ddsd.ddsCaps,&pDDSD2->ddsCaps,sizeof(This->ddsd.ddsCaps));

	This->ddsd.dwAlphaBitDepth = pDDSD2->dwAlphaBitDepth;
	This->ddsd.dwBackBufferCount = pDDSD2->dwBackBufferCount; 
	This->ddsd.dwFlags = pDDSD2->dwFlags;
	This->ddsd.dwHeight = pDDSD2->dwHeight;
	This->ddsd.dwLinearSize = pDDSD2->dwLinearSize; 
	This->ddsd.dwMipMapCount = pDDSD2->dwMipMapCount;
	This->ddsd.dwRefreshRate = pDDSD2->dwRefreshRate;
	This->ddsd.dwReserved = pDDSD2->dwReserved;
	This->ddsd.dwWidth  = pDDSD2->dwWidth;
	This->ddsd.lPitch = pDDSD2->lPitch; 
	This->ddsd.lpSurface = pDDSD2->lpSurface;

    /************ Test see if we can Create Surface ***********************/

	if (This->owner->DirectDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CANCREATESURFACE)
	{
		/* can the driver create the surface */
		DDHAL_CANCREATESURFACEDATA CanCreateData;
		memset(&CanCreateData, 0, sizeof(DDHAL_CANCREATESURFACEDATA));
		CanCreateData.lpDD = &This->owner->DirectDrawGlobal; 
		CanCreateData.lpDDSurfaceDesc = (LPDDSURFACEDESC)&This->ddsd;
			
		if (This->owner->DirectDrawGlobal.lpDDCBtmp->HALDD.CanCreateSurface(&CanCreateData) == DDHAL_DRIVER_NOTHANDLED)
			return DDERR_INVALIDPARAMS;
		
	   if(CanCreateData.ddRVal != DD_OK)
			return CanCreateData.ddRVal;
	}

   /************ Create Surface ***********************/
	 
	/* surface global struct */	
	memset(&This->Global, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));	
	This->Global.lpDD = &This->owner->DirectDrawGlobal;	
	This->Global.wHeight = This->owner->DirectDrawGlobal.vmiData.dwDisplayHeight;
	This->Global.wWidth = This->owner->DirectDrawGlobal.vmiData.dwDisplayWidth;
	This->Global.dwLinearSize =  This->owner->DirectDrawGlobal.vmiData.lDisplayPitch;
	if(pDDSD2->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		This->Global.dwGlobalFlags = DDRAWISURFGBL_ISGDISURFACE; 

	/* surface more struct */	
	memset(&This->More, 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
	This->More.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
	This->More.dmiDDrawReserved7.wWidth = This->Global.wWidth;
	This->More.dmiDDrawReserved7.wHeight = This->Global.wHeight;
	This->More.dmiDDrawReserved7.wBPP = This->owner->Bpp;
	This->More.dmiDDrawReserved7.wRefreshRate = This->owner->DirectDrawGlobal.dwMonitorFrequency;
	//This->More.dmiDDrawReserved7.wMonitorsAttachedToDesktop = 2;
	/* ToDo: fill ddsCapsEx */
 
	/* surface local struct */
	memset(&This->Local, 0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
	This->Local.lpGbl = &This->Global;
	This->Local.lpSurfMore = &This->More;
	This->Local.ddsCaps.dwCaps = pDDSD2->ddsCaps.dwCaps;
	This->Local.dwProcessId = This->owner->ExclusiveOwner.dwProcessId;
 
	/* for the double pointer below */	
	This->pLocal[0] = &This->Local; 
    This->pLocal[1] = NULL;  
 
	/* the parameter struct */
	DDHAL_CREATESURFACEDATA CreateData;
	memset(&CreateData, 0, sizeof(DDHAL_CREATESURFACEDATA));
	CreateData.lpDD = &This->owner->DirectDrawGlobal;	
	CreateData.lpDDSurfaceDesc = (LPDDSURFACEDESC)&This->ddsd; 
	CreateData.dwSCnt = 1;
	CreateData.lplpSList = This->pLocal;	
	asm("int3");
	CreateData.ddRVal = 1;

	/* this is the call we were waiting for */
	if(This->owner->DirectDrawGlobal.lpDDCBtmp->HALDD.CreateSurface(&CreateData) == DDHAL_DRIVER_NOTHANDLED)
		return DDERR_INVALIDPARAMS;
	asm("int3");

	/* FIXME remove the if and debug string*/
	if(CreateData.ddRVal != DD_OK)
		return CreateData.ddRVal;
	
	OutputDebugString(L"This does hit By Ati Readon but not for nvida :( ");
	OutputDebugString(L"Yet ;)");

   	return DD_OK;
}


HRESULT Hal_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rDest,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rSrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
	DX_STUB;

	IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
    IDirectDrawSurfaceImpl* That = (IDirectDrawSurfaceImpl*)src;
 	
	if (!(This->owner->DirectDrawGlobal.lpDDCBtmp->HALDDSurface.dwFlags  & DDHAL_SURFCB32_BLT)) 
	{
		return DDERR_NODRIVERSUPPORT;
	}

	DDHAL_BLTDATA BltData;
	BltData.lpDD = &This->owner->DirectDrawGlobal;
	BltData.dwFlags = dwFlags;
	BltData.lpDDDestSurface = &This->Local;
    if(rDest) BltData.rDest = *(RECTL*)rDest;
    if(rSrc) BltData.rSrc = *(RECTL*)rSrc;
    if(That) BltData.lpDDSrcSurface = &That->Local;
	if(lpbltfx) BltData.bltFX = *lpbltfx;

	if (This->owner->DirectDrawGlobal.lpDDCBtmp->HALDDSurface.Blt(&BltData) != DDHAL_DRIVER_HANDLED)
	{
	   return DDERR_NODRIVERSUPPORT;
	}
	
	return BltData.ddRVal;
}
