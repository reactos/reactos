/*	DirectDraw HAL driver
 *
 * Copyright 2001 TransGaming Technologies Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



#include <windows.h>
#include "ddraw.h"
#include "rosddraw.h"
#include "ddraw_private.h"

static IDirectDraw7Vtbl MAIN_DirectDraw_VTable;


HRESULT MAIN_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex)
{    
    //This->local.lpGbl = &dd_gbl;

    This->final_release = MAIN_DirectDraw_final_release;
    This->set_exclusive_mode = MAIN_DirectDrawSet_exclusive_mode;
   // This->create_palette = MAIN_DirectDrawPalette_Create;

    This->create_primary    = MAIN_DirectDraw_create_primary;
    This->create_backbuffer = MAIN_DirectDraw_create_backbuffer;
    This->create_texture    = MAIN_DirectDraw_create_texture;

    ICOM_INIT_INTERFACE(This, IDirectDraw7, MAIN_DirectDraw_VTable);
    return S_OK;
}

void MAIN_DirectDraw_final_release(IDirectDrawImpl *This)
{
 
}

HRESULT MAIN_DirectDrawSet_exclusive_mode(IDirectDrawImpl *This, DWORD dwEnterExcl)
{
 return DDERR_UNSUPPORTED;
}


HRESULT MAIN_DirectDraw_create_primary(IDirectDrawImpl* This, const DDSURFACEDESC2* pDDSD, LPDIRECTDRAWSURFACE7* ppSurf,
				      IUnknown* pUnkOuter)

{
	return DDERR_UNSUPPORTED;
  }

HRESULT MAIN_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
					 const DDSURFACEDESC2* pDDSD,
					 LPDIRECTDRAWSURFACE7* ppSurf,
					 IUnknown* pUnkOuter,
					 IDirectDrawSurfaceImpl* primary)
{
	return DDERR_UNSUPPORTED;
  }

HRESULT MAIN_DirectDraw_create_texture(IDirectDrawImpl* This,
				      const DDSURFACEDESC2* pDDSD,
				      LPDIRECTDRAWSURFACE7* ppSurf,
				      LPUNKNOWN pOuter,
				      DWORD dwMipMapLevel)
{
	return DDERR_UNSUPPORTED;
  }






/* basic funtion for the com object */
HRESULT WINAPI MAIN_DirectDraw_QueryInterface(LPDIRECTDRAW7 iface,REFIID refiid,LPVOID *obj) 
{
	return DDERR_UNSUPPORTED;
  }

ULONG WINAPI MAIN_DirectDraw_AddRef(LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    //TRACE("(%p)->() incrementing from %lu.\n", This, ref -1);

    return ref;
}

ULONG WINAPI MAIN_DirectDraw_Release(LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    
    if (ref == 0)
    {
	if (This->final_release != NULL)
	    This->final_release(This);

	/* We free the private. This is an artifact of the fact that I don't
	 * have the destructors set up correctly. */
	if (This->private != (This+1))
	    HeapFree(GetProcessHeap(), 0, This->private);

	HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

HRESULT WINAPI MAIN_DirectDraw_Compact(LPDIRECTDRAW7 iface) 
{
 
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface, DWORD dwFlags, 
											 LPDIRECTDRAWCLIPPER *ppClipper, IUnknown *pUnkOuter)
{
    return DDERR_UNSUPPORTED;
}
HRESULT WINAPI MAIN_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
			      LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE* ppPalette,LPUNKNOWN pUnknown)
{
	return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_CreateSurface(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
			      LPDIRECTDRAWSURFACE7 *ppSurf,IUnknown *pUnkOuter) 
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
				 LPDIRECTDRAWSURFACE7* dst) 
{
 return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
				 LPDDSURFACEDESC2 pDDSD, LPVOID context, LPDDENUMMODESCALLBACK2 callback) 
{
 return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
			     LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
			     LPDDENUMSURFACESCALLBACK7 callback) 
{
 return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) 
{
return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
			LPDDCAPS pHELCaps) 
{
return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD) 
{
    return DDERR_UNSUPPORTED;
}


HRESULT WINAPI MAIN_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes, LPDWORD pCodes)
{
  
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface, 
											 LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface)
{
  
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq)
{  
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{
 return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL status)
{
 return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_Initialize(LPDIRECTDRAW7 iface, LPGUID lpGuid)
{
 return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_SetCooperativeLevel(LPDIRECTDRAW7 iface, HWND hwnd,
												   DWORD cooplevel)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
			      DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
  
    return DDERR_UNSUPPORTED;
}


HRESULT WINAPI MAIN_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
												   HANDLE h)
{
  
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free)											   

{
  
    return DDERR_UNSUPPORTED;
}
												   
HRESULT WINAPI MAIN_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
												LPDIRECTDRAWSURFACE7 *lpDDS)
{  
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface) 
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
				   LPDDDEVICEIDENTIFIER2 pDDDI, DWORD dwFlags)
{    
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
			      DWORD dwNumModes, DWORD dwFlags)
{    
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI MAIN_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b)
{    
    return DDERR_UNSUPPORTED;
}

/* End com interface */




HRESULT WINAPI MAIN_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
			      IUnknown* pUnkOuter, BOOL ex)
{
   
    IDirectDrawImpl* This;    
	*pIface = ICOM_INTERFACE(This, IDirectDraw7);

    return DD_OK;
}

static IDirectDraw7Vtbl MAIN_DirectDraw_VTable =
{
    MAIN_DirectDraw_QueryInterface,
    MAIN_DirectDraw_AddRef,
    MAIN_DirectDraw_Release,
    MAIN_DirectDraw_Compact,
    MAIN_DirectDraw_CreateClipper,
    MAIN_DirectDraw_CreatePalette,
    MAIN_DirectDraw_CreateSurface,
    MAIN_DirectDraw_DuplicateSurface,
    MAIN_DirectDraw_EnumDisplayModes,
    MAIN_DirectDraw_EnumSurfaces,
    MAIN_DirectDraw_FlipToGDISurface,
    MAIN_DirectDraw_GetCaps,
    MAIN_DirectDraw_GetDisplayMode,
    MAIN_DirectDraw_GetFourCCCodes,
    MAIN_DirectDraw_GetGDISurface,
    MAIN_DirectDraw_GetMonitorFrequency,
    MAIN_DirectDraw_GetScanLine,
    MAIN_DirectDraw_GetVerticalBlankStatus,
    MAIN_DirectDraw_Initialize,
    MAIN_DirectDraw_RestoreDisplayMode,
    MAIN_DirectDraw_SetCooperativeLevel,
    MAIN_DirectDraw_SetDisplayMode,
    MAIN_DirectDraw_WaitForVerticalBlank,
    MAIN_DirectDraw_GetAvailableVidMem,
    MAIN_DirectDraw_GetSurfaceFromDC,
    MAIN_DirectDraw_RestoreAllSurfaces,
    MAIN_DirectDraw_TestCooperativeLevel,
    MAIN_DirectDraw_GetDeviceIdentifier,
    MAIN_DirectDraw_StartModeTest,
    MAIN_DirectDraw_EvaluateMode
};
