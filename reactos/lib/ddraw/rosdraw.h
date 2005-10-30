#ifndef __DDRAW_PRIVATE
#define __DDRAW_PRIVATE

/********* Includes  *********/

#include <windows.h>
#include <stdio.h>
#include <ddraw.h>
#include <ddk/ddrawi.h>
#include <ddk/d3dhal.h>
#include <ddrawgdi.h>

/******** Main Object ********/

typedef struct 
{
	IDirectDraw7Vtbl* lpVtbl;
	IDirectDraw4Vtbl* lpVtbl_v4;
	IDirectDraw2Vtbl* lpVtbl_v2;
	IDirectDrawVtbl*  lpVtbl_v1;

	DDRAWI_DIRECTDRAW_GBL DirectDrawGlobal;
	DDHAL_DDMISCELLANEOUSCALLBACKS Misc2Callback;
	DDHALINFO HalInfo;	

    HWND window;
    DWORD cooperative_level;
	HDC hdc;
	int Height, Width, Bpp;

	GUID* lpGUID;

} IDirectDrawImpl; 

/******** Surface Object ********/

typedef struct 
{
	IDirectDrawSurface7Vtbl* lpVtbl;
	IDirectDrawSurface3Vtbl* lpVtbl_v3;

    LONG ref;

    IDirectDrawImpl* owner;

} IDirectDrawSurfaceImpl;

/******** Clipper Object ********/

typedef struct 
{
	IDirectDrawClipperVtbl* lpVtbl;
    LONG ref;

    IDirectDrawImpl* owner;

} IDirectDrawClipperImpl;

/******** Palette Object ********/

typedef struct 
{
	IDirectDrawPaletteVtbl* lpVtbl;
    LONG ref;

    IDirectDrawImpl* owner;

} IDirectDrawPaletteImpl;

/*********** VTables ************/

extern IDirectDraw7Vtbl				DirectDraw7_Vtable;
extern IDirectDrawVtbl				DDRAW_IDirectDraw_VTable;
extern IDirectDraw2Vtbl				DDRAW_IDirectDraw2_VTable;
extern IDirectDraw4Vtbl				DDRAW_IDirectDraw4_VTable;

extern IDirectDrawSurface7Vtbl		DirectDrawSurface7_Vtable;
extern IDirectDrawSurface3Vtbl		DDRAW_IDDS3_Thunk_VTable;

extern IDirectDrawPaletteVtbl		DirectDrawPalette_Vtable;
extern IDirectDrawClipperVtbl		DirectDrawClipper_Vtable;
extern IDirectDrawColorControlVtbl	DirectDrawColorControl_Vtable;
extern IDirectDrawGammaControlVtbl	DirectDrawGammaControl_Vtable;

/********* Prototypes **********/

HRESULT Hal_DirectDraw_Initialize (LPDIRECTDRAW7 iface);
HRESULT Hal_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface);
VOID Hal_DirectDraw_Release (LPDIRECTDRAW7 iface);

HRESULT Hal_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free);	

HRESULT Hal_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
					HANDLE h); 

HRESULT Hal_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine);
HRESULT Hal_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface); 
HRESULT Hal_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
													DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);





HRESULT Hel_DirectDraw_Initialize (LPDIRECTDRAW7 iface);
HRESULT Hel_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface);
VOID Hel_DirectDraw_Release (LPDIRECTDRAW7 iface);

HRESULT Hel_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free);	

HRESULT Hel_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
					HANDLE h); 

HRESULT Hel_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine);
HRESULT Hel_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface);
HRESULT Hel_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
													DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);


/*********** Macros ***********/

#define DX_STUB \
	static BOOL firstcall = TRUE; \
	if (firstcall) \
	{ \
		char buffer[1024]; \
		sprintf ( buffer, "Function %s is not implemented yet (%s:%d)\n", __FUNCTION__,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
		firstcall = FALSE; \
	} \
	return DDERR_UNSUPPORTED; 

#endif /* __DDRAW_PRIVATE */
