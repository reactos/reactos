/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/ddraw.c
 * PURPOSE:              DirectDraw Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"


HRESULT WINAPI Main_DirectDraw_Initialize (LPDIRECTDRAW7 iface, LPGUID lpGUID)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	HRESULT ret;

	// this if it is not called by DirectDrawCreate
	if(FALSE)
		return DDERR_ALREADYINITIALIZED;

	// save the parameter
	This->lpGUID = lpGUID;

	// get the HDC
	This->hdc = GetWindowDC(GetDesktopWindow());
	This->Height = GetDeviceCaps(This->hdc, VERTRES);
	This->Width = GetDeviceCaps(This->hdc, HORZRES);
	This->Bpp = GetDeviceCaps(This->hdc, BITSPIXEL);

	// call software first
	if((ret = Hal_DirectDraw_Initialize (iface)) != DD_OK)
		return ret;
	
	// ... then overwrite with hal
	if((ret = Hel_DirectDraw_Initialize (iface)) != DD_OK)
		return ret;

	return DD_OK;
}

HRESULT WINAPI Main_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface, HWND hwnd, DWORD cooplevel)
{
	// TODO:															
	// - create a scaner that check which driver we should get the HDC from	
	//   for now we always asume it is the active dirver that should be use.
	// - allow more Flags

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	HRESULT ret;

	// check the parameters
	if (This->cooperative_level == cooplevel && This->window == hwnd)
		return DD_OK;

	if (This->window)
		return DDERR_HWNDALREADYSET;

	if (This->cooperative_level)
		return DDERR_EXCLUSIVEMODEALREADYSET;

	if ((cooplevel&DDSCL_EXCLUSIVE) && !(cooplevel&DDSCL_FULLSCREEN))
		return DDERR_INVALIDPARAMS;

	if (cooplevel&DDSCL_NORMAL && cooplevel&DDSCL_FULLSCREEN)
		return DDERR_INVALIDPARAMS;

	// set the data
	This->window = hwnd;
	This->hdc = GetDC(hwnd);
	This->cooperative_level = cooplevel;

	if((ret = Hel_DirectDraw_SetCooperativeLevel (iface)) != DD_OK)
		return ret;

	if((ret = Hal_DirectDraw_SetCooperativeLevel (iface)) != DD_OK)
		return ret;

   	return DD_OK;
}

HRESULT WINAPI Main_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
																DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	// this only for exclusive mode
	if(!(This->cooperative_level & DDSCL_EXCLUSIVE))
   		return DDERR_NOEXCLUSIVEMODE;

	// change the resolution using normal WinAPI function
	DEVMODE mode;
	mode.dmSize = sizeof(mode);
	mode.dmPelsWidth = dwWidth;
	mode.dmPelsHeight = dwHeight;
	mode.dmBitsPerPel = dwBPP;
	mode.dmDisplayFrequency = dwRefreshRate;
	mode.dmFields = 0;

	if(dwWidth)
		mode.dmFields |= DM_PELSWIDTH;
	if(dwHeight)
		mode.dmFields |= DM_PELSHEIGHT;
	if(dwBPP)
		mode.dmFields |= DM_BITSPERPEL;
	if(dwRefreshRate)
		mode.dmFields |= DM_DISPLAYFREQUENCY;

	if (ChangeDisplaySettings(&mode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		return DDERR_UNSUPPORTEDMODE;
	
	// TODO: reactivate ddraw object, maximize window, set it in foreground 
	// and set excluive mode (if implemented by the driver)

	if(dwWidth)
		This->Height = dwWidth;
	if(dwHeight)
		This->Width = dwHeight;
	if(dwBPP)
		This->Bpp = dwBPP;

	return DD_OK;
}

HRESULT WINAPI Main_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
											LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter) 
{
    if (pUnkOuter!=NULL) 
		return DDERR_INVALIDPARAMS; 

	if(sizeof(DDSURFACEDESC2)!=pDDSD->dwSize)
		return DDERR_UNSUPPORTED;

	// the nasty com stuff
	IDirectDrawSurfaceImpl* That; 

	That = (IDirectDrawSurfaceImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawSurfaceImpl));

	if (That == NULL) 
		return E_OUTOFMEMORY;

	That->lpVtbl = &DDrawSurface_VTable;
	That->ref = 1;
	*ppSurf = (LPDIRECTDRAWSURFACE7)That;

	// the real surface object creation
   	return That->lpVtbl->Initialize (*ppSurf, (LPDIRECTDRAW)iface, pDDSD);
}

ULONG WINAPI Main_DirectDraw_AddRef (LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    ULONG ref = InterlockedIncrement((PLONG)&This->DirectDrawGlobal.dwRefCnt);

   	return ref;
}

ULONG WINAPI Main_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    ULONG ref = InterlockedDecrement((PLONG)&This->DirectDrawGlobal.dwRefCnt);
    
    if (ref == 0)
    {
		// set resoltion back to the one in registry
		if(This->cooperative_level & DDSCL_EXCLUSIVE)
			ChangeDisplaySettings(NULL, 0);

		HeapFree(GetProcessHeap(), 0, This);
    }

   	return ref;
}

/**** Stubs ****/

HRESULT WINAPI Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface,REFIID refiid,LPVOID *obj) 
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_Compact(LPDIRECTDRAW7 iface) 
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface, DWORD dwFlags, 
											 LPDIRECTDRAWCLIPPER *ppClipper, IUnknown *pUnkOuter)
{
   	DX_STUB;
}
HRESULT WINAPI Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
			      LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE* ppPalette,LPUNKNOWN pUnknown)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
				 LPDIRECTDRAWSURFACE7* dst) 
{
	DX_STUB;	
}

HRESULT WINAPI Main_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
				 LPDDSURFACEDESC2 pDDSD, LPVOID context, LPDDENUMMODESCALLBACK2 callback) 
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
			     LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
			     LPDDENUMSURFACESCALLBACK7 callback) 
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) 
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
			LPDDCAPS pHELCaps) 
{
	DWORD status = DD_FALSE;
	IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

	if (pDriverCaps != NULL) 
	{
	  RtlCopyMemory(pDriverCaps,&This->DirectDrawGlobal.ddCaps,sizeof(DDCORECAPS));
	  status = DD_OK;
	}

	if (pHELCaps != NULL) 
	{
	  RtlCopyMemory(pDriverCaps,&This->DirectDrawGlobal.ddHELCaps,sizeof(DDCORECAPS));
	  status = DD_OK;
	}

	/* Both caps mixed ?? */
	/* DDCORECAPS ddBothCaps; */
	
	return status;
}

HRESULT WINAPI Main_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD) 
{    
	IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

	if (pDDSD == NULL)
	{
      return DD_FALSE;
	}
	
	pDDSD->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE | DDSD_WIDTH; 
	pDDSD->dwHeight  = This->DirectDrawGlobal.vmiData.dwDisplayHeight;
    pDDSD->dwWidth = This->DirectDrawGlobal.vmiData.dwDisplayWidth; 

	/* FIXME Do not use DUMMYUNIONNAME1 some how union lPitch does not see by the compiler
	   but rest of the union are visable. more header problem ??? 
    */
    pDDSD->DUMMYUNIONNAME1.lPitch  = This->DirectDrawGlobal.vmiData.lDisplayPitch;
	
    
	/* have not check where I should get hold of this info yet
    DWORD  dwBackBufferCount;
    */

    pDDSD->dwRefreshRate = This->DirectDrawGlobal.dwMonitorFrequency;

	/* have not check where I should get hold of this info yet
    DWORD  dwAlphaBitDepth;
    DWORD  dwReserved;
    LPVOID lpSurface;
    union
    {
        DDCOLORKEY    ddckCKDestOverlay;
        DWORD         dwEmptyFaceColor;
    } 
    DDCOLORKEY    ddckCKDestBlt;
    DDCOLORKEY    ddckCKSrcOverlay;
    DDCOLORKEY    ddckCKSrcBlt;
	*/

    RtlCopyMemory(&pDDSD->ddpfPixelFormat,&This->DirectDrawGlobal.vmiData.ddpfDisplay,sizeof(DDPIXELFORMAT));
    RtlCopyMemory(&pDDSD->ddsCaps,&This->DirectDrawGlobal.ddCaps,sizeof(DDCORECAPS));
	
	/* have not check where I should get hold of this info yet    
    DWORD         dwTextureStage;
    */
  
   	return DD_OK;
}


HRESULT WINAPI Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes, LPDWORD pCodes)
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface, 
											 LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface)
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq)
{  
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

	if (freq == NULL)
	{
	 return DD_FALSE;
	}

   	*freq = This->DirectDrawGlobal.dwMonitorFrequency;
	return DD_OK;
}

HRESULT WINAPI Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL status)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
												   HANDLE h)
{
  
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free)											   
{
  
   	DX_STUB;
}
												   
HRESULT WINAPI Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
												LPDIRECTDRAWSURFACE7 *lpDDS)
{  
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface)
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface) 
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
				   LPDDDEVICEIDENTIFIER2 pDDDI, DWORD dwFlags)
{    
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
			      DWORD dwNumModes, DWORD dwFlags)
{    
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b)
{    
   	DX_STUB;
}

IDirectDraw7Vtbl DirectDraw_VTable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface,
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    Main_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    Main_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
