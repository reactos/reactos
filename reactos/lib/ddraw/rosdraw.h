#ifndef __DDRAW_PRIVATE
#define __DDRAW_PRIVATE

/********* Includes  *********/

#include <windows.h>
#include <stdio.h>
#include <ddraw.h>

#include <ddk/ddrawint.h>
#include <ddk/d3dnthal.h>
#include <ddk/d3dhal.h>
#include <ddrawgdi.h>
#include <d3d8thk.h>


/******** Main Object ********/

typedef struct 
{
	DDHAL_DDCALLBACKS DdMain;
    DDHAL_DDSURFACECALLBACKS DdSurface;
    DDHAL_DDPALETTECALLBACKS DdPalette;
    D3DHAL_CALLBACKS D3dMain;
    DDHAL_DDEXEBUFCALLBACKS	D3dBufferCallbacks;

} DRIVERCALLBACKS;

typedef struct 
{
	IDirectDraw7Vtbl* lpVtbl;
	DRIVERCALLBACKS DriverCallbacks;
    DWORD ref;

	DDHALINFO HalInfo;	
    D3DHAL_GLOBALDRIVERDATA	D3dDriverData;

	LPDDSURFACEDESC pD3dTextureFormats;
	LPDWORD pdwFourCC;
	LPVIDMEM pvmList;

    HWND window;
    DWORD cooperative_level;
	HDC hdc;
	int Height, Width, Bpp;

	GUID* lpGUID;
	DDRAWI_DIRECTDRAW_GBL DirectDrawGlobal;

} IDirectDrawImpl; 


/******** Surface Object ********/

typedef struct 
{
	IDirectDrawSurface7Vtbl* lpVtbl;
    DWORD ref;

    IDirectDrawImpl* owner;

} IDirectDrawSurfaceImpl;


/******** Clipper Object ********/

typedef struct 
{
	IDirectDrawClipperVtbl* lpVtbl;
    DWORD ref;

    IDirectDrawImpl* owner;

} IDirectDrawClipperImpl;


/******** Palette Object ********/

typedef struct 
{
	IDirectDrawPaletteVtbl* lpVtbl;
    DWORD ref;

    IDirectDrawImpl* owner;

} IDirect3DDeviceImpl;


/*********** VTables ************/

extern IDirectDraw7Vtbl DirectDraw_VTable;
extern IDirectDrawSurface7Vtbl DDrawSurface_VTable;


/********* Prototypes **********/

HRESULT Hal_DirectDraw_Initialize (LPDIRECTDRAW7 iface);
HRESULT Hal_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface);
VOID Hal_DirectDraw_Release (LPDIRECTDRAW7 iface);

HRESULT Hel_DirectDraw_Initialize (LPDIRECTDRAW7 iface);
HRESULT Hel_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface);
VOID Hel_DirectDraw_Release (LPDIRECTDRAW7 iface);

#endif /* __DDRAW_PRIVATE */
