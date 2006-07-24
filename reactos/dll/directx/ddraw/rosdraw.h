#ifndef __DDRAW_PRIVATE
#define __DDRAW_PRIVATE

/********* Includes  *********/

#include <windows.h>
#include <stdio.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include <ddrawgdi.h>

/* DirectDraw startup code only internal use  */
HRESULT WINAPI StartDirectDraw(LPDIRECTDRAW* iface);
HRESULT WINAPI StartDirectDrawHal(LPDIRECTDRAW* iface);
HRESULT WINAPI StartDirectDrawHel(LPDIRECTDRAW* iface);
HRESULT WINAPI Create_DirectDraw (LPGUID pGUID, LPDIRECTDRAW* pIface, REFIID id, BOOL ex);

/* DirectDraw Cleanup code only internal use */
VOID Cleanup(LPDIRECTDRAW7 iface);

/* own macro to alloc memmory */
#define DxHeapMemAlloc(m)  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m) 
#define DxHeapMemFree(p)   HeapFree(GetProcessHeap(), 0, p);
/******** Main Object ********/

/* Public interface */
HRESULT WINAPI  Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface, REFIID id, LPVOID *obj);
ULONG   WINAPI  Main_DirectDraw_AddRef        (LPDIRECTDRAW7 iface);
ULONG   WINAPI  Main_DirectDraw_Release       (LPDIRECTDRAW7 iface);
HRESULT WINAPI  Main_DirectDraw_Compact       (LPDIRECTDRAW7 iface); 

HRESULT WINAPI  Main_DirectDraw_CreateClipper (LPDIRECTDRAW7 iface, 
											   DWORD dwFlags, 
											   LPDIRECTDRAWCLIPPER *ppClipper, 
											   IUnknown *pUnkOuter);

HRESULT WINAPI  Main_DirectDraw_CreatePalette (LPDIRECTDRAW7 iface, 
											   DWORD dwFlags,
                                               LPPALETTEENTRY palent, 
											   LPDIRECTDRAWPALETTE* ppPalette, 
											   LPUNKNOWN pUnkOuter);


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

	


	/* ExclusiveOwner */	
    
    DWORD cooperative_level;	
	

	BOOL InitializeDraw; 

	/* HEL stuff */
	DWORD HELMemoryAvilable;

    /* DD Callbacks info */   	
	DDHAL_DESTROYDRIVERDATA mDdDestroyDriver;
    DDHAL_CREATESURFACEDATA      mDdCreateSurface;
	DDHAL_SETCOLORKEYDATA mDdSetColorKey;
    DDHAL_SETMODEDATA mDdSetMode;
    DDHAL_WAITFORVERTICALBLANKDATA mDdWaitForVerticalBlank;
    DDHAL_CANCREATESURFACEDATA mDdCanCreateSurface;
    DDHAL_CREATEPALETTEDATA mDdCreatePalette;
    DDHAL_GETSCANLINEDATA mDdGetScanLine;
    DDHAL_SETEXCLUSIVEMODEDATA mDdSetExclusiveMode;
    DDHAL_FLIPTOGDISURFACEDATA mDdFlipToGDISurface;
	
    DDRAWI_DDRAWSURFACE_GBL mPrimaryGlobal;

	/* adding a switch */
	DWORD devicetype;

} IDirectDrawImpl; 

/******** Surface Object ********/
typedef struct 
{    
	 /* Primarey surface we must reach it from every where */
   
    DDRAWI_DDRAWSURFACE_MORE mPrimaryMore;
    DDRAWI_DDRAWSURFACE_LCL mPrimaryLocal;
    DDRAWI_DDRAWSURFACE_LCL *mpPrimaryLocals[1];
    DDRAWI_DDRAWCLIPPER_LCL mPrimaryClipperLocal;
    DDRAWI_DDRAWCLIPPER_GBL mPrimaryClipperGlobal;

    DDSURFACEDESC mddsdPrimary;

    DDRAWI_DDRAWSURFACE_LCL *mpInUseSurfaceLocals[1];
    
    DDRAWI_DDRAWSURFACE_GBL mSurfGlobal;
    DDRAWI_DDRAWSURFACE_MORE mSurfMore;
    DDRAWI_DDRAWSURFACE_LCL mSurfLocal;
    DDRAWI_DDRAWSURFACE_LCL *mpSurfLocals[1];
    DDRAWI_DDRAWCLIPPER_LCL mSurfClipperLocal;
    DDRAWI_DDRAWCLIPPER_GBL mSurfClipperGlobal;

    DDRAWI_DDRAWSURFACE_GBL mOverlayGlobal;
    DDRAWI_DDRAWSURFACE_LCL mOverlayLocal[6];
    DDRAWI_DDRAWSURFACE_LCL *mpOverlayLocals[6];
    DDRAWI_DDRAWSURFACE_MORE mOverlayMore[6];

    DDSURFACEDESC mddsdOverlay;

} DxSurf;

typedef struct 
{
	IDirectDrawSurface7Vtbl* lpVtbl;
	IDirectDrawSurface3Vtbl* lpVtbl_v3;
   
    IDirectDrawImpl* Owner;

	DDRAWI_DDRAWSURFACE_GBL Global; 
	DDRAWI_DDRAWSURFACE_MORE More; 
	DDRAWI_DDRAWSURFACE_LCL Local;
	DDRAWI_DDRAWSURFACE_LCL *pLocal[2]; 
	DDSURFACEDESC ddsd; 

    DxSurf *Surf;

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
    
    IDirectDrawImpl* owner;
	DDRAWI_DDRAWPALETTE_GBL DDPalette;    
} IDirectDrawPaletteImpl;

/******** Gamma Object ********/

typedef struct 
{
	IDirectDrawGammaControlVtbl* lpVtbl;
    LONG ref;
    IDirectDrawImpl* Owner;
    IDirectDrawSurfaceImpl* Surf;


} IDirectDrawGammaImpl;

/******** Color Object ********/

typedef struct 
{
	IDirectDrawColorControlVtbl* lpVtbl;
    LONG ref;
    IDirectDrawImpl* Owner;
    IDirectDrawSurfaceImpl* Surf;


} IDirectDrawColorImpl;

/******** Kernel Object ********/

typedef struct 
{
	IDirectDrawKernelVtbl* lpVtbl;
    LONG ref;
    IDirectDrawImpl* Owner;
    IDirectDrawSurfaceImpl* Surf;

} IDirectDrawKernelImpl;

/******** SurfaceKernel Object ********/

typedef struct 
{
	IDirectDrawSurfaceKernelVtbl* lpVtbl;
    LONG ref;
    IDirectDrawImpl* Owner;
    IDirectDrawSurfaceImpl* Surf;
    IDirectDrawKernelImpl * Kernl;

} IDirectDrawSurfaceKernelImpl;


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
extern IDirectDrawKernelVtbl        DirectDrawKernel_Vtable;
extern IDirectDrawSurfaceKernelVtbl DirectDrawSurfaceKernel_Vtable;

/********* Prototypes **********/

HRESULT WINAPI Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps, LPDWORD total, LPDWORD free); 


VOID Hal_DirectDraw_Release (LPDIRECTDRAW7 );
HRESULT Hal_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7, LPDDSCAPS2, LPDWORD, LPDWORD );	
HRESULT Hal_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7, DWORD, HANDLE ); 
HRESULT Hal_DirectDraw_GetScanLine(LPDIRECTDRAW7 , LPDWORD );
HRESULT Hal_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 ); 
HRESULT Hal_DirectDraw_SetDisplayMode (LPDIRECTDRAW7, DWORD, DWORD, DWORD, DWORD, DWORD );
HRESULT Hal_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX );
HRESULT Hal_DDrawSurface_Lock(LPDIRECTDRAWSURFACE7 iface, LPRECT prect, LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE event);
HRESULT Hal_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAWSURFACE7 override, DWORD dwFlags);
HRESULT Hal_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags, LPDDCOLORKEY pCKey);
HRESULT Hal_DDrawSurface_Unlock(LPDIRECTDRAWSURFACE7 iface, LPRECT pRect);	
HRESULT Hal_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags);
HRESULT Hal_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags);

HRESULT Hel_DirectDraw_Initialize (LPDIRECTDRAW7 );
HRESULT Hel_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 );
VOID Hel_DirectDraw_Release (LPDIRECTDRAW7 );
HRESULT Hel_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 , LPDDSCAPS2 ddsaps, LPDWORD , LPDWORD );	
HRESULT Hel_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7, DWORD, HANDLE ); 
HRESULT Hel_DirectDraw_GetScanLine(LPDIRECTDRAW7 , LPDWORD );
HRESULT Hel_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 );
HRESULT Hel_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 , DWORD , DWORD ,DWORD , DWORD , DWORD );
HRESULT Hel_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX );
HRESULT Hel_DDrawSurface_Lock(LPDIRECTDRAWSURFACE7 iface, LPRECT prect, LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE event);
HRESULT Hel_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags, LPDDCOLORKEY pCKey);
HRESULT Hel_DDrawSurface_Unlock(LPDIRECTDRAWSURFACE7 iface, LPRECT pRect);
HRESULT Hel_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags);
HRESULT Hel_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAWSURFACE7 override, DWORD dwFlags);
HRESULT Hel_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags);

/* HEL CALLBACK */
DWORD CALLBACK  HelDdDestroyDriver(LPDDHAL_DESTROYDRIVERDATA lpDestroyDriver);
DWORD CALLBACK  HelDdCreateSurface(LPDDHAL_CREATESURFACEDATA lpCreateSurface);
DWORD CALLBACK  HelDdSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKey);
DWORD CALLBACK  HelDdSetMode(LPDDHAL_SETMODEDATA SetMode);
DWORD CALLBACK  HelDdWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank);
DWORD CALLBACK  HelDdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurface);
DWORD CALLBACK  HelDdCreatePalette(LPDDHAL_CREATEPALETTEDATA lpCreatePalette);
DWORD CALLBACK  HelDdGetScanLine(LPDDHAL_GETSCANLINEDATA lpGetScanLine);
DWORD CALLBACK  HelDdSetExclusiveMode(LPDDHAL_SETEXCLUSIVEMODEDATA lpSetExclusiveMode);
DWORD CALLBACK  HelDdFlipToGDISurface(LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurface);


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
	
#define DX_STUB_DD_OK \
	static BOOL firstcall = TRUE; \
	if (firstcall) \
	{ \
		char buffer[1024]; \
		sprintf ( buffer, "Function %s is not implemented yet (%s:%d)\n", __FUNCTION__,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
		firstcall = FALSE; \
	} \
	return DD_OK; 	
	

#define DX_STUB_str(x) \
		{ \
        char buffer[1024]; \
		sprintf ( buffer, "Function %s %s (%s:%d)\n", __FUNCTION__,x,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
        }


//#define DX_WINDBG_trace()  


#define DX_WINDBG_trace() \
	static BOOL firstcallx = TRUE; \
	if (firstcallx) \
	{ \
		char buffer[1024]; \
		sprintf ( buffer, "Enter Function %s (%s:%d)\n", __FUNCTION__,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
		firstcallx = TRUE; \
	}


#define DX_WINDBG_trace_res(width,height,bpp) \
	static BOOL firstcallxx = TRUE; \
	if (firstcallxx) \
	{ \
		char buffer[1024]; \
		sprintf ( buffer, "Setmode have been req width=%d, height=%d bpp=%d\n",width,height,bpp); \
		OutputDebugStringA(buffer); \
		firstcallxx = FALSE; \
	}

#endif /* __DDRAW_PRIVATE */
