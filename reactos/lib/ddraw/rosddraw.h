/* 
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              ddraw lib
 * PROGRAMMER:           Magnus Olsen
 * UPDATE HISTORY:
 */
#include "ddraw_private.h"



HANDLE STDCALL OsThunkDdCreateDirectDrawObject(HDC hdc);


void HAL_DirectDraw_final_release(IDirectDrawImpl *This);
HRESULT HAL_DirectDrawSet_exclusive_mode(IDirectDrawImpl *This, DWORD dwEnterExcl);



 

HRESULT HAL_DirectDraw_create_primary(IDirectDrawImpl* This, const DDSURFACEDESC2* pDDSD, LPDIRECTDRAWSURFACE7* ppSurf,
				      IUnknown* pUnkOuter);

HRESULT HAL_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
					 const DDSURFACEDESC2* pDDSD,
					 LPDIRECTDRAWSURFACE7* ppSurf,
					 IUnknown* pUnkOuter,
					 IDirectDrawSurfaceImpl* primary);

HRESULT HAL_DirectDraw_create_texture(IDirectDrawImpl* This,
				      const DDSURFACEDESC2* pDDSD,
				      LPDIRECTDRAWSURFACE7* ppSurf,
				      LPUNKNOWN pOuter,
				      DWORD dwMipMapLevel);

HRESULT DDRAW_Create(LPGUID lpGUID, LPVOID *lplpDD, LPUNKNOWN pUnkOuter, REFIID iid, BOOL ex); 


HRESULT WINAPI HAL7_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface, DWORD dwFlags, 
											 LPDIRECTDRAWCLIPPER *ppClipper, IUnknown *pUnkOuter);

HRESULT WINAPI HAL7_DirectDraw_QueryInterface(LPDIRECTDRAW7 iface,REFIID refiid,LPVOID *obj) ;

ULONG WINAPI HAL7_DirectDraw_AddRef(LPDIRECTDRAW7 iface) ;

ULONG WINAPI HAL7_DirectDraw_Release(LPDIRECTDRAW7 iface) ;

HRESULT WINAPI HAL7_DirectDraw_Compact(LPDIRECTDRAW7 iface) ;

HRESULT WINAPI HAL7_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
			      LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE* ppPalette,LPUNKNOWN pUnknown) ;

HRESULT WINAPI HAL7_DirectDraw_CreateSurface(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
			      LPDIRECTDRAWSURFACE7 *ppSurf,IUnknown *pUnkOuter) ;

HRESULT WINAPI HAL7_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
				 LPDIRECTDRAWSURFACE7* dst) ;

HRESULT WINAPI HAL7_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
				 LPDDSURFACEDESC2 pDDSD, LPVOID context, LPDDENUMMODESCALLBACK2 callback) ;

HRESULT WINAPI HAL7_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
			     LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
			     LPDDENUMSURFACESCALLBACK7 callback) ;

HRESULT WINAPI HAL7_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) ;

HRESULT WINAPI HAL7_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
			LPDDCAPS pHELCaps) ;


HRESULT WINAPI HAL7_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD) ;


HRESULT WINAPI HAL7_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes, LPDWORD pCodes);

HRESULT WINAPI HAL7_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface, 
											 LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface);


HRESULT WINAPI HAL7_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq);

HRESULT WINAPI HAL7_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine);

HRESULT WINAPI HAL7_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL status);

HRESULT WINAPI HAL7_DirectDraw_Initialize(LPDIRECTDRAW7 iface, LPGUID lpGuid);

HRESULT WINAPI HAL7_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface);


HRESULT WINAPI HAL7_DirectDraw_SetCooperativeLevel(LPDIRECTDRAW7 iface, HWND hwnd,
												   DWORD cooplevel);

HRESULT WINAPI HAL7_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
			      DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);

HRESULT WINAPI HAL7_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
												   HANDLE h);

HRESULT WINAPI HAL7_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free);

HRESULT WINAPI HAL7_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
												LPDIRECTDRAWSURFACE7 *lpDDS);


HRESULT WINAPI HAL7_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface);

HRESULT WINAPI HAL7_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface) ;

HRESULT WINAPI HAL7_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
				   LPDDDEVICEIDENTIFIER2 pDDDI, DWORD dwFlags);

HRESULT WINAPI HAL7_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
			      DWORD dwNumModes, DWORD dwFlags);

HRESULT WINAPI HAL7_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b);


HRESULT WINAPI HAL7_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
			      IUnknown* pUnkOuter, BOOL ex);
