#ifndef __DDRAW_PRIVATE
#define __DDRAW_PRIVATE

/********* Includes  *********/

#include <windows.h>
#include <stdio.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include <ddrawgdi.h>

/* own macro to alloc memmory */
#define DxHeapMemAlloc(m)  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m) 
#define DxHeapMemFree(p)   HeapFree(GetProcessHeap(), 0, p);
/******** Main Object ********/

typedef struct 
{
	/* Setup the Vtbl COM table */
	IDirectDraw7Vtbl* lpVtbl;
	IDirectDraw4Vtbl* lpVtbl_v4;
	IDirectDraw2Vtbl* lpVtbl_v2;
	IDirectDrawVtbl*  lpVtbl_v1;

	/* The main struct that contain all info from the HAL and HEL */	
	HDC hdc;
    DDRAWI_DIRECTDRAW_GBL mDDrawGlobal;
    DDRAWI_DIRECTDRAW_LCL mDDrawLocal;
    DDHALINFO mHALInfo;

    DDHAL_CALLBACKS mCallbacks;
    DDHAL_DDEXEBUFCALLBACKS mD3dBufferCallbacks;
    D3DHAL_CALLBACKS mD3dCallbacks;
    D3DHAL_GLOBALDRIVERDATA mD3dDriverData;

    UINT mcModeInfos;
    DDHALMODEINFO *mpModeInfos;

    UINT mcvmList;
    VIDMEM *mpvmList;

    UINT mcFourCC;
    DWORD *mpFourCC;

    UINT mcTextures;
    DDSURFACEDESC *mpTextures;

	/* Surface */
	DDRAWI_DDRAWSURFACE_GBL mPrimaryGlobal;
    DDRAWI_DDRAWSURFACE_MORE mPrimaryMore;
    DDRAWI_DDRAWSURFACE_LCL mPrimaryLocal;
    DDRAWI_DDRAWSURFACE_LCL *mpPrimaryLocals[1];
    DDRAWI_DDRAWCLIPPER_LCL mPrimaryClipperLocal;
    DDRAWI_DDRAWCLIPPER_GBL mPrimaryClipperGlobal;
    //DDRAWI_DDRAWCLIPPER_INT mPrimaryClipperInterface;
    DDSURFACEDESC mddsdPrimary;
    DDSURFACEDESC mddsdOverlay;

    DDRAWI_DDRAWSURFACE_GBL mOverlayGlobal;
    DDRAWI_DDRAWSURFACE_LCL mOverlayLocal[6];
    DDRAWI_DDRAWSURFACE_LCL *mpOverlayLocals[6];
    DDRAWI_DDRAWSURFACE_MORE mOverlayMore[6];


	/* ExclusiveOwner */
	DDRAWI_DIRECTDRAW_LCL ExclusiveOwner;				
    
    DWORD cooperative_level;	
	HWND CooperativeHWND;

	BOOL InitializeDraw; 

	/* HEL stuff */
	DWORD HELMemoryAvilable;

} IDirectDrawImpl; 

/******** Surface Object ********/

typedef struct 
{
	IDirectDrawSurface7Vtbl* lpVtbl;
	IDirectDrawSurface3Vtbl* lpVtbl_v3;
   
    IDirectDrawImpl* owner;

	DDRAWI_DDRAWSURFACE_GBL Global; 
	DDRAWI_DDRAWSURFACE_MORE More; 
	DDRAWI_DDRAWSURFACE_LCL Local;
	DDRAWI_DDRAWSURFACE_LCL *pLocal[2]; 
	DDSURFACEDESC ddsd; 

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

HRESULT WINAPI Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps, LPDWORD total, LPDWORD free); 

HRESULT Hal_DirectDraw_Initialize (LPDIRECTDRAW7 );
HRESULT Hal_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 );
VOID Hal_DirectDraw_Release (LPDIRECTDRAW7 );
HRESULT Hal_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7, LPDDSCAPS2, LPDWORD, LPDWORD );	
HRESULT Hal_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7, DWORD, HANDLE ); 
HRESULT Hal_DirectDraw_GetScanLine(LPDIRECTDRAW7 , LPDWORD );
HRESULT Hal_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 ); 
HRESULT Hal_DirectDraw_SetDisplayMode (LPDIRECTDRAW7, DWORD, DWORD, DWORD, DWORD, DWORD );
HRESULT Hal_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX );
HRESULT Hal_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD, LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter);       


HRESULT Hel_DirectDraw_Initialize (LPDIRECTDRAW7 );
HRESULT Hel_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 );
VOID Hel_DirectDraw_Release (LPDIRECTDRAW7 );
HRESULT Hel_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 , LPDDSCAPS2 ddsaps, LPDWORD , LPDWORD );	
HRESULT Hel_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7, DWORD, HANDLE ); 
HRESULT Hel_DirectDraw_GetScanLine(LPDIRECTDRAW7 , LPDWORD );
HRESULT Hel_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 );
HRESULT Hel_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 , DWORD , DWORD ,DWORD , DWORD , DWORD );
HRESULT Hel_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX );
HRESULT Hel_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD, LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter);       

/* Setting for HEL should be move to ros special reg key ? */

/* setup how much graphic memory should hel be limit, set it now to 64MB */
#define HEL_GRAPHIC_MEMORY_MAX 67108864

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
