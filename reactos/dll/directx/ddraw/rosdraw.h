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
extern DDRAWI_DIRECTDRAW_GBL ddgbl;
extern DDRAWI_DDRAWSURFACE_GBL ddSurfGbl;
extern WCHAR classname[128];
extern WNDCLASSW wnd_class;


HRESULT WINAPI StartDirectDraw(LPDIRECTDRAW* iface, LPGUID pGUID, BOOL reenable);
HRESULT WINAPI StartDirectDrawHal(LPDIRECTDRAW* iface, BOOL reenable);
HRESULT WINAPI StartDirectDrawHel(LPDIRECTDRAW* iface, BOOL reenable);
HRESULT WINAPI Create_DirectDraw (LPGUID pGUID, LPDIRECTDRAW* pIface, REFIID id, BOOL ex);

HRESULT WINAPI ReCreateDirectDraw(LPDIRECTDRAW* iface);

/* DirectDraw Cleanup code only internal use */
VOID Cleanup(LPDIRECTDRAW7 iface);

/* own macro to alloc memmory */
#define DxHeapMemAlloc(m)  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m)
#define DxHeapMemFree(p)   HeapFree(GetProcessHeap(), 0, p); \
                           p = NULL;

/******** Main Object ********/

/* Public interface */
VOID WINAPI AcquireDDThreadLock();
VOID WINAPI ReleaseDDThreadLock();

HRESULT WINAPI Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 , REFIID , LPVOID *);
ULONG   WINAPI Main_DirectDraw_AddRef(LPDIRECTDRAW7 );
ULONG   WINAPI Main_DirectDraw_Release(LPDIRECTDRAW7 );
HRESULT WINAPI Main_DirectDraw_Compact(LPDIRECTDRAW7 );
HRESULT WINAPI Main_DirectDraw_CreateClipper(LPDIRECTDRAW7, DWORD, LPDIRECTDRAWCLIPPER *, IUnknown *);
HRESULT WINAPI Main_DirectDraw_CreatePalette(LPDIRECTDRAW7, DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE*, LPUNKNOWN);
HRESULT WINAPI Main_DirectDraw_CreateSurface(LPDIRECTDRAW7, LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE7 *, IUnknown *);
HRESULT WINAPI Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7, LPDIRECTDRAWSURFACE7, LPDIRECTDRAWSURFACE7*);
HRESULT WINAPI Main_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7, LPDDDEVICEIDENTIFIER2, DWORD);

HRESULT WINAPI Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7, DWORD, LPDDSURFACEDESC2, LPVOID,
											LPDDENUMSURFACESCALLBACK7);

HRESULT WINAPI Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7);
HRESULT WINAPI Main_DirectDraw_GetDisplayMode(LPDIRECTDRAW7, LPDDSURFACEDESC2);
HRESULT WINAPI Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7, HDC, LPDIRECTDRAWSURFACE7 *);
HRESULT WINAPI Main_DirectDraw_GetCaps(LPDIRECTDRAW7, LPDDCAPS pDriverCaps, LPDDCAPS);
HRESULT WINAPI Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7, LPDWORD pNumCodes, LPDWORD);
HRESULT WINAPI Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7, LPDIRECTDRAWSURFACE7 *);
HRESULT WINAPI Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7, DWORD dwFlags, HANDLE);
HRESULT WINAPI Main_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7, LPDWORD);
HRESULT WINAPI Main_DirectDraw_GetScanLine(LPDIRECTDRAW7, LPDWORD);
HRESULT WINAPI Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7, LPBOOL);
HRESULT WINAPI Main_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7);
HRESULT WINAPI Main_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7, HWND, DWORD);
HRESULT WINAPI Main_DirectDraw_SetDisplayMode (LPDIRECTDRAW7, DWORD, DWORD, DWORD, DWORD, DWORD);
HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface);
HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface);


ULONG   WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE7);
ULONG   WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7, REFIID, LPVOID*);
HRESULT WINAPI Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7, HDC);
HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX);
HRESULT WINAPI Main_DDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7, LPDDBLTBATCH, DWORD, DWORD);
HRESULT WINAPI Main_DDrawSurface_BltFast(LPDIRECTDRAWSURFACE7, DWORD, DWORD, LPDIRECTDRAWSURFACE7, LPRECT, DWORD);
HRESULT WINAPI Main_DDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7, DWORD, LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7, LPVOID, LPDDENUMSURFACESCALLBACK7);
HRESULT WINAPI Main_DDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7, DWORD, LPVOID,LPDDENUMSURFACESCALLBACK7);
HRESULT WINAPI Main_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 , LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7, REFGUID);
HRESULT WINAPI Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7, LPDDSCAPS2, LPDIRECTDRAWSURFACE7*);
HRESULT WINAPI Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7, DWORD dwFlags);
HRESULT WINAPI Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7, LPDDSCAPS2 pCaps);
HRESULT WINAPI Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7, LPDIRECTDRAWCLIPPER*);
HRESULT WINAPI Main_DDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE7, HDC *);
HRESULT WINAPI Main_DDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7, LPVOID*);
HRESULT WINAPI Main_DDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7, LPLONG, LPLONG);
HRESULT WINAPI Main_DDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7, LPDIRECTDRAWPALETTE*);
HRESULT WINAPI Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7, LPDDPIXELFORMAT);
HRESULT WINAPI Main_DDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7, REFGUID, LPVOID, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7, LPDDSURFACEDESC2);
HRESULT WINAPI Main_DDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7, LPDWORD);
HRESULT WINAPI Main_DDrawSurface_IsLost(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_PageLock(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7, HDC);
HRESULT WINAPI Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE7, LPDIRECTDRAWCLIPPER);
HRESULT WINAPI Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7, DWORD, LPDDCOLORKEY);
HRESULT WINAPI Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE7, LONG, LONG);
HRESULT WINAPI Main_DDrawSurface_SetPalette (LPDIRECTDRAWSURFACE7, LPDIRECTDRAWPALETTE);
HRESULT WINAPI Main_DDrawSurface_SetPriority (LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_SetPrivateData (LPDIRECTDRAWSURFACE7, REFGUID, LPVOID, DWORD, DWORD);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDIRECTDRAWSURFACE7, DWORD, LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE7, DDSURFACEDESC2 *, DWORD);
HRESULT WINAPI Main_DDrawSurface_SetLOD(LPDIRECTDRAWSURFACE7, DWORD);
HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE7, LPRECT);
HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE7, LPDIRECTDRAW, LPDDSURFACEDESC2);
HRESULT WINAPI Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE7, LPRECT, LPDDSURFACEDESC2, DWORD, HANDLE);
HRESULT WINAPI Main_DDrawSurface_Restore(LPDIRECTDRAWSURFACE7);
HRESULT WINAPI Main_DDrawSurface_UpdateOverlay (LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT,
												DWORD, LPDDOVERLAYFX);


ULONG WINAPI  DirectDrawClipper_AddRef (LPDIRECTDRAWCLIPPER iface);
HRESULT WINAPI  DirectDrawClipper_Initialize( LPDIRECTDRAWCLIPPER iface, LPDIRECTDRAW lpDD, DWORD dwFlags);



HRESULT CreateOverlaySurface(LPDDRAWI_DIRECTDRAW_INT This, LPDDRAWI_DDRAWSURFACE_INT *That, LPDDSURFACEDESC2 pDDSD);
HRESULT CreateBackBufferSurface(LPDDRAWI_DIRECTDRAW_INT This, LPDDRAWI_DDRAWSURFACE_INT *That, LPDDRAWI_DDRAWSURFACE_LCL *lpLcl, LPDDSURFACEDESC2 pDDSD);
HRESULT CreatePrimarySurface(LPDDRAWI_DIRECTDRAW_INT This, LPDDRAWI_DDRAWSURFACE_INT *That,LPDDRAWI_DDRAWSURFACE_LCL *lpLcl, LPDDSURFACEDESC2 pDDSD);

/* DirectDraw Object struct

   DDRAWI_DIRECTDRAW_INT
   it is the the return pointer from ddraw.dll to the program

   DDRAWI_DIRECTDRAW_LCL
   It is the program own private data

   DDRAWI_DIRECTDRAW_GBL
   This struct is gboal for whole ddraw.dll for all program
   it is static in ddraw if it change it change for all
   program

   The struct

   from http://msdn2.microsoft.com/en-us/library/ms898267.aspx
   it was not document for windows 2000/xp/2003 but ms did document it
   for windows ce 5.0 the link are to windows ce 5.0 arch

typedef struct _DDRAWI_DIRECTDRAW_INT {
  LPVOID lpVtbl;
  LPDDRAWI_DIRECTDRAW_LCL lpLcl;
  LPDDRAWI_DIRECTDRAW_INT lpLink;
  DWORD dwIntRefCnt;
} DDRAWI_DIRECTDRAW_INT;

 rest of the struct are from msdn for windows 2000/xp/2003
typedef struct _DDRAWI_DIRECTDRAW_LCL {
  DWORD  lpDDMore;
  LPDDRAWI_DIRECTDRAW_GBL  lpGbl; // fill it from function Create_DirectDraw with static pointer ddgbl
  DWORD  dwUnused0;
  DWORD  dwLocalFlags;
  DWORD  dwLocalRefCnt;
  DWORD  dwProcessId;
  IUnknown FAR  *pUnkOuter;
  DWORD  dwObsolete1;
  ULONG_PTR  hWnd;
  ULONG_PTR  hDC;                       // create HDC and save it to this pointer
  DWORD  dwErrorMode;
  LPDDRAWI_DDRAWSURFACE_INT  lpPrimary;
  LPDDRAWI_DDRAWSURFACE_INT  lpCB;
  DWORD   dwPreferredMode;
  HINSTANCE  hD3DInstance;
  IUnknown FAR  *pD3DIUnknown;
  LPDDHAL_CALLBACKS  lpDDCB;           // same memory pointer as DDRAWI_DIRECTDRAW_GBL->lpDDCBtmp, setup by function StartDirectDraw
  ULONG_PTR  hDDVxd;
  DWORD   dwAppHackFlags;
  ULONG_PTR    hFocusWnd;
  DWORD   dwHotTracking;
  DWORD   dwIMEState;
  ULONG_PTR  hWndPopup;
  ULONG_PTR  hDD;
  ULONG_PTR  hGammaCalibrator;
  LPDDGAMMACALIBRATORPROC  lpGammaCalibrator;
} DDRAWI_DIRECTDRAW_LCL;


typedef struct _DDRAWI_DIRECTDRAW_GBL {
  DWORD  dwRefCnt;
  DWORD  dwFlags;
  FLATPTR  fpPrimaryOrig;
  DDCORECAPS  ddCaps;
  DWORD  dwInternal1;
  DWORD  dwUnused1[9];
  LPDDHAL_CALLBACKS  lpDDCBtmp;
  LPDDRAWI_DDRAWSURFACE_INT  dsList;
  LPDDRAWI_DDRAWPALETTE_INT  palList;
  LPDDRAWI_DDRAWCLIPPER_INT  clipperList;
  LPDDRAWI_DIRECTDRAW_GBL  lp16DD;    // pointer to it self (DDRAWI_DIRECTDRAW_GBL)
  DWORD  dwMaxOverlays;
  DWORD  dwCurrOverlays;
  DWORD  dwMonitorFrequency;
  DDCORECAPS  ddHELCaps;
  DWORD  dwUnused2[50];
  DDCOLORKEY  ddckCKDestOverlay;
  DDCOLORKEY  ddckCKSrcOverlay;
  VIDMEMINFO  vmiData;
  LPVOID  lpDriverHandle;
  LPDDRAWI_DIRECTDRAW_LCL  lpExclusiveOwner;
  DWORD  dwModeIndex;
  DWORD  dwModeIndexOrig;
  DWORD  dwNumFourCC;
  DWORD FAR  *lpdwFourCC;
  DWORD  dwNumModes;
  LPDDHALMODEINFO  lpModeInfo;
  PROCESS_LIST  plProcessList;
  DWORD  dwSurfaceLockCount;
  DWORD  dwAliasedLockCnt;
  ULONG_PTR  dwReserved3;
  ULONG_PTR  hDD;               // GdiEntry1 are filling this pointer
  char  cObsolete[12];
  DWORD  dwReserved1;
  DWORD  dwReserved2;
  DBLNODE  dbnOverlayRoot;
  volatile LPWORD  lpwPDeviceFlags;
  DWORD  dwPDevice;
  DWORD  dwWin16LockCnt;
  DWORD  dwUnused3;
  DWORD  hInstance;
  DWORD  dwEvent16;
  DWORD  dwSaveNumModes;
  ULONG_PTR  lpD3DGlobalDriverData;
  ULONG_PTR  lpD3DHALCallbacks;
  DDCORECAPS  ddBothCaps;
  LPDDVIDEOPORTCAPS  lpDDVideoPortCaps;
  LPDDRAWI_DDVIDEOPORT_INT  dvpList;
  ULONG_PTR  lpD3DHALCallbacks2;
  RECT  rectDevice;
  DWORD  cMonitors;
  LPVOID  gpbmiSrc;
  LPVOID  gpbmiDest;
  LPHEAPALIASINFO  phaiHeapAliases;
  ULONG_PTR  hKernelHandle;
  ULONG_PTR  pfnNotifyProc;
  LPDDKERNELCAPS  lpDDKernelCaps;
  LPDDNONLOCALVIDMEMCAPS  lpddNLVCaps;
  LPDDNONLOCALVIDMEMCAPS  lpddNLVHELCaps;
  LPDDNONLOCALVIDMEMCAPS  lpddNLVBothCaps;
  ULONG_PTR  lpD3DExtendedCaps;
  DWORD  dwDOSBoxEvent;
  RECT  rectDesktop;
  char  cDriverName[MAX_DRIVER_NAME];
  ULONG_PTR   lpD3DHALCallbacks3;
  DWORD  dwNumZPixelFormats;
  LPDDPIXELFORMAT  lpZPixelFormats;
  LPDDRAWI_DDMOTIONCOMP_INT mcList;
  DWORD  hDDVxd;
  DDSCAPSEX  ddsCapsMore;
} DDRAWI_DIRECTDRAW_GBL;


*/


/* Clipper Object struct
   DDRAWI_DDRAWCLIPPER_INT
   it is the the return pointer from ddraw.dll to the program

   DDRAWI_DDRAWCLIPPER_LCL
   It is the program own private data

   DDRAWI_DDRAWCLIPPER_GBL
   This struct is gboal for whole ddraw.dll for all program
   it is static in ddraw if it change it change for all
   program

   The struct

typedef struct _DDRAWI_DDRAWCLIPPER_INT {
  LPVOID  lpVtbl;
  LPDDRAWI_DDRAWCLIPPER_LCL  lpLcl;
  LPDDRAWI_DDRAWCLIPPER_INT  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_DDRAWCLIPPER_INT;

typedef struct _DDRAWI_DDRAWCLIPPER_LCL {
  DWORD  lpClipMore;
  LPDDRAWI_DDRAWCLIPPER_GBL  lpGbl;
  LPDDRAWI_DIRECTDRAW_LCL  lpDD_lcl;
  DWORD  dwLocalRefCnt;
  IUnknown  FAR  *pUnkOuter;
  LPDDRAWI_DIRECTDRAW_INT  lpDD_int;
  ULONG_PTR  dwReserved1;
  IUnknown  *pAddrefedThisOwner;
} DDRAWI_DDRAWCLIPPER_LCL;

typedef struct _DDRAWI_DDRAWCLIPPER_GBL {
  DWORD  dwRefCnt;
  DWORD  dwFlags;
  LPDDRAWI_DIRECTDRAW_GBL lpDD;
  DWORD  dwProcessId;
  ULONG_PTR  dwReserved1;
  ULONG_PTR  hWnd;
  LPRGNDATA  lpStaticClipList;
} DDRAWI_DDRAWCLIPPER_GBL;
*/



/*
typedef struct _DDRAWI_DDRAWPALETTE_INT {
  LPVOID  lpVtbl;
  LPDDRAWI_DDRAWPALETTE_LCL  lpLcl;
  LPDDRAWI_DDRAWPALETTE_INT  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_DDRAWPALETTE_INT;

typedef struct _DDRAWI_DDRAWPALETTE_LCL {
  DWORD  lpPalMore;
  LPDDRAWI_DDRAWPALETTE_GBL  lpGbl;
  ULONG_PTR  dwUnused0;
  DWORD  dwLocalRefCnt;
  IUnknown FAR  *pUnkOuter;
  LPDDRAWI_DIRECTDRAW_LCL  lpDD_lcl;
  ULONG_PTR  dwReserved1;
  ULONG_PTR  dwDDRAWReserved1;
  ULONG_PTR  dwDDRAWReserved2;
  ULONG_PTR  dwDDRAWReserved3;
} DDRAWI_DDRAWPALETTE_LCL;

typedef struct _DDRAWI_DDRAWPALETTE_GBL {
  DWORD  dwRefCnt;
  DWORD  dwFlags;
  LPDDRAWI_DIRECTDRAW_LCL  lpDD_lcl;
  DWORD  dwProcessId;
  LPPALETTEENTRY  lpColorTable;
  union {
     ULONG_PTR  dwReserved1;
     HPALETTE  hHELGDIPalette;
  };
  DWORD  dwDriverReserved;
  DWORD  dwContentsStamp;
  DWORD  dwSaveStamp;
  DWORD  dwHandle;
} DDRAWI_DDRAWPALETTE_GBL;
*/

/*
typedef struct _DDRAWI_DDVIDEOPORT_INT {
  LPVOID  lpVtbl;
  LPDDRAWI_DDVIDEOPORT_LCL  lpLcl;
  LPDDRAWI_DDVIDEOPORT_INT  lpLink;
  DWORD  dwIntRefCnt;
  DWORD  dwFlags;
} DDRAWI_DDVIDEOPORT_INT;

typedef struct _DDRAWI_DDVIDEOPORT_LCL {
  LPDDRAWI_DIRECTDRAW_LCL  lpDD;
  DDVIDEOPORTDESC  ddvpDesc;
  DDVIDEOPORTINFO  ddvpInfo;
  LPDDRAWI_DDRAWSURFACE_INT  lpSurface;
  LPDDRAWI_DDRAWSURFACE_INT  lpVBISurface;
  LPDDRAWI_DDRAWSURFACE_INT *lpFlipInts;
  DWORD  dwNumAutoflip;
  DWORD  dwProcessID;
  DWORD  dwStateFlags;
  DWORD  dwFlags;
  DWORD  dwRefCnt;
  FLATPTR  fpLastFlip;
  ULONG_PTR  dwReserved1;
  ULONG_PTR  dwReserved2;
  HANDLE  hDDVideoPort;
  DWORD  dwNumVBIAutoflip;
  LPDDVIDEOPORTDESC  lpVBIDesc;
  LPDDVIDEOPORTDESC  lpVideoDesc;
  LPDDVIDEOPORTINFO  lpVBIInfo;
  LPDDVIDEOPORTINFO  lpVideoInfo;
  DWORD  dwVBIProcessID;
} DDRAWI_DDVIDEOPORT_LCL;


Surface
typedef struct _DDRAWI_DDRAWSURFACE_GBL {
  DWORD  dwRefCnt;
  DWORD  dwGlobalFlags;
  union {
     LPACCESSRECTLIST  lpRectList;
     DWORD  dwBlockSizeY;
  };
  union {
     LPVMEMHEAP  lpVidMemHeap;
     DWORD  dwBlockSizeX;
  };
  union {
     LPDDRAWI_DIRECTDRAW_GBL lpDD;
     LPVOID   lpDDHandle;
  };
  FLATPTR   fpVidMem;
  union {
     LONG   lPitch;
     DWORD  dwLinearSize;
  };
  WORD    wHeight;
  WORD    wWidth;
  DWORD   dwUsageCount;
  ULONG_PTR   dwReserved1;
  DDPIXELFORMAT   ddpfSurface;
} DDRAWI_DDRAWSURFACE_GBL;

*/


/* This comment info maybe is wrong
   bare in mind I am using logic thinking
   for follow info does not exists in MSDN
   so I am drawing clude how previews stuffs
   works that are document in MSDN/DDK

follow struct should exists ???
DDRAWI_DDVIDEOPORT_GBL
DDRAWI_DDGAMMACONTROL_INT
DDRAWI_DDGAMMACONTROL_LCL
DDRAWI_DDGAMMACONTROL_GBL
DDRAWI_DDCOLORCONTROL_INT
DDRAWI_DDCOLORCONTROL_LCL
DDRAWI_DDCOLORCONTROL_GBL
DDRAWI_KERNEL_INT
DDRAWI_KERNEL_LCL
DDRAWI_KERNEL_GBL
DDRAWI_DDKERNELSURFACE_INT
DDRAWI_DDKERNELSURFACE_LCL
DDRAWI_DDKERNELSURFACE_GBL

follow struct can be easy create
DDRAWI_DDGAMMACONTROL_INT
DDRAWI_DDCOLORCONTROL_INT
DDRAWI_KERNEL_INT

the DDRAWI_DDGAMMACONTROL_INT should looking like this
typedef struct _DDRAWI_DDGAMMACONTROL_INT
{
  LPVOID  lpVtbl;
  LPDDRAWI_DDGAMMACONTROL_LCL  lpLcl;
  LPDDRAWI_DDGAMMACONTROL_INT  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_DDGAMMACONTROL_INT, *LPDDRAWI_DDGAMMACONTROL_INT

how did I got this struct I looked at all other INT struct how they where
build. But it is not 100% sure this one is right untill I/we known how
the DDRAWI_DDGAMMACONTROL_LCL works and  DDRAWI_DDCOLORCONTROL_GBL
our internal struct will look like this
typedef struct _DDRAWI_DDGAMMACONTROL_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_DDGAMMACONTROL_INT, *LPDDRAWI_DDGAMMACONTROL_INT

same goes for DDRAWI_DDCOLORCONTROL_INT

typedef struct DDRAWI_DDCOLORCONTROL_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_DDCOLORCONTROL_INT, *LPDDRAWI_DDCOLORCONTROL_INT
*/

typedef struct DDRAWI_DDCOLORCONTROL_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_DDCOLORCONTROL_INT, *LPDDRAWI_DDCOLORCONTROL_INT;


typedef struct _DDRAWI_DDGAMMACONTROL_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_DDGAMMACONTROL_INT, *LPDDRAWI_DDGAMMACONTROL_INT;

typedef struct _DDRAWI_DDKERNEL_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} DDRAWI_KERNEL_INT, *LPDDRAWI_KERNEL_INT;

typedef struct _DDRAWI_DDKERNELSURFACE_INT
{
  LPVOID  lpVtbl;
  LPVOID  lpLcl;
  LPVOID  lpLink;
  DWORD  dwIntRefCnt;
} _DDRAWI_DDKERNELSURFACE_INT, *LPDDRAWI_DDKERNELSURFACE_INT;

/* now to real info that are for private use and are our own */



/*********** VTables ************/


extern IDirectDrawVtbl				DirectDraw_Vtable;
extern IDirectDraw2Vtbl				DirectDraw2_Vtable;
extern IDirectDraw4Vtbl				DirectDraw4_Vtable;
extern IDirectDraw7Vtbl				DirectDraw7_Vtable;


extern IDirectDrawSurface7Vtbl		DirectDrawSurface7_Vtable;
extern IDirectDrawSurface3Vtbl		DirectDrawSurface3_VTable;

extern IDirectDrawPaletteVtbl		DirectDrawPalette_Vtable;
extern IDirectDrawClipperVtbl		DirectDrawClipper_Vtable;
extern IDirectDrawColorControlVtbl	DirectDrawColorControl_Vtable;
extern IDirectDrawGammaControlVtbl	DirectDrawGammaControl_Vtable;
extern IDirectDrawKernelVtbl        DirectDrawKernel_Vtable;
extern IDirectDrawSurfaceKernelVtbl DirectDrawSurfaceKernel_Vtable;

/********* Prototypes **********/
HRESULT WINAPI Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAWSURFACE7 pAttach);
HRESULT WINAPI Main_DDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7 iface, LPRECT pRect);
HRESULT WINAPI Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface, LPDDSURFACEDESC2 pDDSD);
HRESULT WINAPI Main_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags, LPDDSURFACEDESC2 pDDSD, LPVOID context, LPDDENUMMODESCALLBACK2 callback);
HRESULT WINAPI Main_DDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface, DDSURFACEDESC2 *DDSD, DWORD Flags);


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
DWORD CALLBACK  HelDdSetColorKey(LPDDHAL_DRVSETCOLORKEYDATA lpSetColorKey);
DWORD CALLBACK  HelDdSetMode(LPDDHAL_SETMODEDATA SetMode);
DWORD CALLBACK  HelDdWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank);
DWORD CALLBACK  HelDdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurface);
DWORD CALLBACK  HelDdCreatePalette(LPDDHAL_CREATEPALETTEDATA lpCreatePalette);
DWORD CALLBACK  HelDdGetScanLine(LPDDHAL_GETSCANLINEDATA lpGetScanLine);
DWORD CALLBACK  HelDdSetExclusiveMode(LPDDHAL_SETEXCLUSIVEMODEDATA lpSetExclusiveMode);
DWORD CALLBACK  HelDdFlipToGDISurface(LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurface);

DWORD CALLBACK HelDdSurfAddAttachedSurface(LPDDHAL_ADDATTACHEDSURFACEDATA lpDestroySurface);
DWORD CALLBACK HelDdSurfBlt(LPDDHAL_BLTDATA lpBltData);
DWORD CALLBACK HelDdSurfDestroySurface(LPDDHAL_DESTROYSURFACEDATA lpDestroySurfaceData);
DWORD CALLBACK HelDdSurfFlip(LPDDHAL_FLIPDATA lpFlipData);
DWORD CALLBACK HelDdSurfGetBltStatus(LPDDHAL_GETBLTSTATUSDATA lpGetBltStatusData);
DWORD CALLBACK HelDdSurfGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatusData);
DWORD CALLBACK HelDdSurfLock(LPDDHAL_LOCKDATA lpLockData);
DWORD CALLBACK HelDdSurfreserved4(DWORD *lpPtr);
DWORD CALLBACK HelDdSurfSetClipList(LPDDHAL_SETCLIPLISTDATA lpSetClipListData);
DWORD CALLBACK HelDdSurfSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKeyData);
DWORD CALLBACK HelDdSurfSetOverlayPosition(LPDDHAL_SETOVERLAYPOSITIONDATA lpSetOverlayPositionData);
DWORD CALLBACK HelDdSurfSetPalette(LPDDHAL_SETPALETTEDATA lpSetPaletteData);
DWORD CALLBACK HelDdSurfUnlock(LPDDHAL_UNLOCKDATA lpUnLockData);
DWORD CALLBACK HelDdSurfUpdateOverlay(LPDDHAL_UPDATEOVERLAYDATA lpUpDateOveryLayData);




/* Setting for HEL should be move to ros special reg key ? */

/* setup how much graphic memory should hel be limit, set it now to 64MB */
#define HEL_GRAPHIC_MEMORY_MAX 67108864

/*********** Macros ***********/

/*
   use this macro to close
   down the debuger text complete
   no debuging at all, it will
   crash ms debuger in VS
*/

//#define DX_WINDBG_trace()
//#define DX_STUB
//#define DX_STUB_DD_OK return DD_OK;
//#define DX_STUB_str(x)
//#define DX_WINDBG_trace_res


/*
   Use this macro if you want deboug in visual studio or
   if you have a program to look at the _INT struct from
   ReactOS ddraw.dll or ms ddraw.dll, so you can see what
   value ms are being setup.

   This macro will create allot warings and can not be help when you compile
*/


//#define DX_WINDBG_trace()
//#define DX_STUB
//#define DX_STUB_DD_OK return DD_OK;
//#define DX_STUB_str(x) printf("%s",x);
//#define DX_WINDBG_trace_res

/*
   use this if want doing a trace from a program
   like a game and ReactOS ddraw.dll in windows
   so you can figout what going wrong and what
   api are being call or if it hel or is it hal

   This marco does not create warings when you compile
*/

#define DX_STUB \
{ \
	static BOOL firstcall = TRUE; \
	if (firstcall) \
	{ \
		char buffer[1024]; \
		sprintf ( buffer, "Function %s is not implemented yet (%s:%d)\n", __FUNCTION__,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
		firstcall = FALSE; \
	} \
} \
	return DDERR_UNSUPPORTED;



#define DX_STUB_DD_OK \
{ \
	static BOOL firstcall = TRUE; \
	if (firstcall) \
	{ \
		char buffer[1024]; \
		sprintf ( buffer, "Function %s is not implemented yet (%s:%d)\n", __FUNCTION__,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
		firstcall = FALSE; \
	} \
} \
	return DD_OK;


#define DX_STUB_str(x) \
		{ \
        char buffer[1024]; \
		sprintf ( buffer, "Function %s %s (%s:%d)\n", __FUNCTION__,x,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
        }

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
