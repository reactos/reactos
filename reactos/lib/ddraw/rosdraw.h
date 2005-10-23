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
	DDRAWI_DIRECTDRAW_GBL DirectDrawGlobal;
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

extern IDirectDraw7Vtbl				DirectDraw_Vtable;
extern IDirectDrawSurface7Vtbl		DirectDrawSurface_Vtable;
extern IDirectDrawPaletteVtbl		DirectDrawPalette_Vtable;
extern IDirectDrawClipperVtbl		DirectDrawClipper_Vtable;
extern IDirectDrawColorControlVtbl	DirectDrawColorControl_Vtable;
extern IDirectDrawGammaControlVtbl	DirectDrawGammaControl_Vtable;

/********* Prototypes **********/

HRESULT Hal_DirectDraw_Initialize (LPDIRECTDRAW7 iface);
HRESULT Hal_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface);
VOID Hal_DirectDraw_Release (LPDIRECTDRAW7 iface);

HRESULT Hel_DirectDraw_Initialize (LPDIRECTDRAW7 iface);
HRESULT Hel_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface);
VOID Hel_DirectDraw_Release (LPDIRECTDRAW7 iface);

/*********** Macros ***********/

#define DX_STUB return DDERR_UNSUPPORTED; 

#endif /* __DDRAW_PRIVATE */
