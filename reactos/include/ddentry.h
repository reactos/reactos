/*
 *  DirectDraw GDI32.dll interface definitions
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <ddk/ddrawi.h>
#include <ddk/winddi.h>
#include <ddk/d3dhal.h>
BOOL STDCALL DdCreateDirectDrawObject( 
LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
HDC hdc
);
BOOL STDCALL DdQueryDirectDrawObject( 
LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
LPDDHALINFO pHalInfo,
LPDDHAL_DDCALLBACKS pDDCallbacks,
LPDDHAL_DDSURFACECALLBACKS pDDSurfaceCallbacks,
LPDDHAL_DDPALETTECALLBACKS pDDPaletteCallbacks,
LPD3DHAL_CALLBACKS pD3dCallbacks,
LPD3DHAL_GLOBALDRIVERDATA pD3dDriverData,
LPDDHAL_DDEXEBUFCALLBACKS pD3dBufferCallbacks,
LPDDSURFACEDESC pD3dTextureFormats,
LPDWORD pdwFourCC,
LPVIDMEM pvmList
);
BOOL STDCALL DdDeleteDirectDrawObject( 
LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal
);
BOOL STDCALL DdCreateSurfaceObject( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
BOOL bPrimarySurface
);
BOOL STDCALL DdDeleteSurfaceObject( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal
);
BOOL STDCALL DdResetVisrgn( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
HWND hWnd
);
BOOL STDCALL DdGetDC( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
LPPALETTEENTRY pColorTable
);
BOOL STDCALL DdReleaseDC( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal
);
HBITMAP STDCALL DdCreateDIBSection( 
HDC hdc,
CONST BITMAPINFO *pbmi,
UINT iUsage,
VOID **ppvBits,
HANDLE hSectionApp,
DWORD dwOffset
);
BOOL STDCALL DdReenableDirectDrawObject( 
LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
BOOL *pbNewMode
);
BOOL STDCALL DdAttachSurface( 
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceFrom,
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceTo
);
VOID STDCALL DdUnattachSurface( 
LPDDRAWI_DDRAWSURFACE_LCL pSurface,
LPDDRAWI_DDRAWSURFACE_LCL pSurfaceAttached
);
ULONG STDCALL DdQueryDisplaySettingsUniqueness(VOID);
HANDLE STDCALL DdGetDxHandle( 
LPDDRAWI_DIRECTDRAW_LCL pDDraw,
LPDDRAWI_DDRAWSURFACE_LCL pSurface,
BOOL bRelease
);
BOOL STDCALL DdSetGammaRamp( 
LPDDRAWI_DIRECTDRAW_LCL pDDraw,
HDC hdc,
LPVOID lpGammaRamp
);
DWORD STDCALL DdSwapTextureHandles( 
LPDDRAWI_DIRECTDRAW_LCL pDDraw,
LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl1,
LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl2
);
DWORD STDCALL DxgGenericThunk(ULONG_PTR ulIndex,
ULONG_PTR ulHandle,
SIZE_T *pdwSizeOfPtr1,
PVOID pvPtr1,
SIZE_T *pdwSizeOfPtr2,
PVOID pvPtr2);
BOOL STDCALL D3DContextCreate( 
HANDLE hDirectDrawLocal,
HANDLE hSurfColor,
HANDLE hSurfZ,
D3DNTHAL_CONTEXTCREATEI *pdcci
);
DWORD STDCALL D3DContextDestroy( 
LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
);
DWORD STDCALL D3DContextDestroyAll(VOID);
DWORD STDCALL D3DValidateTextureStageState( 
LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData
);
DWORD STDCALL D3DDrawPrimitives2( 
HANDLE hCmdBuf,
HANDLE hVBuf,
LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
FLATPTR *pfpVidMemCmd,
DWORD *pdwSizeCmd,
FLATPTR *pfpVidMemVtx,
DWORD *pdwSizeVtx
);
DWORD STDCALL D3DGetDriverState( 
PDD_GETDRIVERSTATEDATA pdata
);
DWORD STDCALL DdAddAttachedSurface( 
HANDLE hSurface,
HANDLE hSurfaceAttached,
PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
);
DWORD STDCALL DdAlphaBlt(
HANDLE hSurfaceDest, 
HANDLE hSurfaceSrc,
PDD_BLTDATA puBltData);
BOOL STDCALL DdDdAttachSurface( /*rename it so it doesnt conflict */
HANDLE hSurfaceFrom,
HANDLE hSurfaceTo
);
DWORD STDCALL DdBeginMoCompFrame( 
HANDLE hMoComp,
PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
);
DWORD STDCALL DdBlt( 
HANDLE hSurfaceDest,
HANDLE hSurfaceSrc,
PDD_BLTDATA puBltData
);
DWORD STDCALL DdCanCreateSurface( 
HANDLE hDirectDraw,
PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);
DWORD STDCALL DdCanCreateD3DBuffer( 
HANDLE hDirectDraw,
PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);
DWORD STDCALL DdColorControl( 
HANDLE hSurface,
PDD_COLORCONTROLDATA puColorControlData
);
HANDLE STDCALL DdDdCreateDirectDrawObject( /*rename it so it doesnt conflict */
HDC hdc
);
DWORD STDCALL DdCreateSurface( 
HANDLE hDirectDraw,
HANDLE *hSurface,
DDSURFACEDESC *puSurfaceDescription,
DD_SURFACE_GLOBAL *puSurfaceGlobalData,
DD_SURFACE_LOCAL *puSurfaceLocalData,
DD_SURFACE_MORE *puSurfaceMoreData,
DD_CREATESURFACEDATA *puCreateSurfaceData,
HANDLE *puhSurface
);
DWORD STDCALL DdCreateD3DBuffer( 
HANDLE hDirectDraw,
HANDLE *hSurface,
DDSURFACEDESC *puSurfaceDescription,
DD_SURFACE_GLOBAL *puSurfaceGlobalData,
DD_SURFACE_LOCAL *puSurfaceLocalData,
DD_SURFACE_MORE *puSurfaceMoreData,
DD_CREATESURFACEDATA *puCreateSurfaceData,
HANDLE *puhSurface
);
HANDLE STDCALL DdCreateMoComp( 
HANDLE hDirectDraw,
PDD_CREATEMOCOMPDATA puCreateMoCompData
);
HANDLE STDCALL DdDdCreateSurfaceObject( /*rename it so it doesnt conflict */
HANDLE hDirectDrawLocal,
HANDLE hSurface,
PDD_SURFACE_LOCAL puSurfaceLocal,
PDD_SURFACE_MORE puSurfaceMore,
PDD_SURFACE_GLOBAL puSurfaceGlobal,
BOOL bComplete
);
BOOL STDCALL DdDdDeleteDirectDrawObject( /*rename it so it doesnt conflict */
HANDLE hDirectDrawLocal
);
BOOL STDCALL DdDdDeleteSurfaceObject( /*rename it so it doesnt conflict */
HANDLE hSurface
);
DWORD STDCALL DdDestroyMoComp( 
HANDLE hMoComp,
PDD_DESTROYMOCOMPDATA puBeginFrameData
);
DWORD STDCALL DdDestroySurface( 
HANDLE hSurface,
BOOL bRealDestroy
);
DWORD STDCALL DdDestroyD3DBuffer( 
HANDLE hSurface
);
DWORD STDCALL DdEndMoCompFrame( 
HANDLE hMoComp,
PDD_ENDMOCOMPFRAMEDATA puEndFrameData
);
DWORD STDCALL DdFlip( 
HANDLE hSurfaceCurrent,
HANDLE hSurfaceTarget,
HANDLE hSurfaceCurrentLeft,
HANDLE hSurfaceTargetLeft,
PDD_FLIPDATA puFlipData
);
DWORD STDCALL DdFlipToGDISurface( 
HANDLE hDirectDraw,
PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData
);
DWORD STDCALL DdGetAvailDriverMemory( 
HANDLE hDirectDraw,
PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
);
DWORD STDCALL DdGetBltStatus( 
HANDLE hSurface,
PDD_GETBLTSTATUSDATA puGetBltStatusData
);
HDC STDCALL DdDdGetDC( /*rename it so it doesnt conflict */
HANDLE hSurface,
PALETTEENTRY *puColorTable
);
DWORD STDCALL DdGetDriverInfo( 
HANDLE hDirectDraw,
PDD_GETDRIVERINFODATA puGetDriverInfoData
);
DWORD STDCALL DdDdGetDxHandle( /*rename it so it doesnt conflict */
HANDLE hDirectDraw,
HANDLE hSurface,
BOOL bRelease
);
DWORD STDCALL DdGetFlipStatus( 
HANDLE hSurface,
PDD_GETFLIPSTATUSDATA puGetFlipStatusData
);
DWORD STDCALL DdGetInternalMoCompInfo( 
HANDLE hDirectDraw,
PDD_GETINTERNALMOCOMPDATA puGetInternalData
);
DWORD STDCALL DdGetMoCompBuffInfo( 
HANDLE hDirectDraw,
PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
);
DWORD STDCALL DdGetMoCompGuids( 
HANDLE hDirectDraw,
PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData
);
DWORD STDCALL DdGetMoCompFormats( 
HANDLE hDirectDraw,
PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
);
DWORD STDCALL DdGetScanLine( 
HANDLE hDirectDraw,
PDD_GETSCANLINEDATA puGetScanLineData
);
DWORD STDCALL DdLock( 
HANDLE hSurface,
PDD_LOCKDATA puLockData,
HDC hdcClip
);
DWORD STDCALL DdLockD3D( 
HANDLE hSurface,
PDD_LOCKDATA puLockData
);
BOOL STDCALL DdDdQueryDirectDrawObject(  /*rename it so it doesnt conflict */
HANDLE hDirectDrawLocal,
DD_HALINFO *pHalInfo,
DWORD *pCallBackFlags,
LPD3DNTHAL_CALLBACKS puD3dCallbacks,
LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
PDD_D3DBUFCALLBACKS puD3dBufferCallbacks,
LPDDSURFACEDESC puD3dTextureFormats,
DWORD *puNumHeaps,
VIDEOMEMORY *puvmList,
DWORD *puNumFourCC,
DWORD *puFourCC
);
DWORD STDCALL DdQueryMoCompStatus( 
HANDLE hMoComp,
PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
);
BOOL STDCALL DdDdReenableDirectDrawObject( /*rename it so it doesnt conflict */
HANDLE hDirectDrawLocal,
BOOL *pubNewMode
);
BOOL STDCALL DdDdReleaseDC( /*rename it so it doesnt conflict */
HANDLE hSurface
);
DWORD STDCALL DdRenderMoComp( 
HANDLE hMoComp,
PDD_RENDERMOCOMPDATA puRenderMoCompData
);
BOOL STDCALL DdDdResetVisrgn( /*rename it so it doesnt conflict */
HANDLE hSurface,
HWND hwnd
);
DWORD STDCALL DdSetColorKey( 
HANDLE hSurface,
PDD_SETCOLORKEYDATA puSetColorKeyData
);
DWORD STDCALL DdSetExclusiveMode( 
HANDLE hDirectDraw,
PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
);
BOOL STDCALL DdDdSetGammaRamp( /*rename it so it doesnt conflict */
HANDLE hDirectDraw,
HDC hdc,
LPVOID lpGammaRamp
);
DWORD STDCALL DdCreateSurfaceEx( 
HANDLE hDirectDraw,
HANDLE hSurface,
DWORD dwSurfaceHandle
);
DWORD STDCALL DdSetOverlayPosition( 
HANDLE hSurfaceSource,
HANDLE hSurfaceDestination,
PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
);
VOID STDCALL DdDdUnattachSurface( /*rename it so it doesnt conflict */
HANDLE hSurface,
HANDLE hSurfaceAttached
);
DWORD STDCALL DdUnlock( 
HANDLE hSurface,
PDD_UNLOCKDATA puUnlockData
);
DWORD STDCALL DdUnlockD3D( 
HANDLE hSurface,
PDD_UNLOCKDATA puUnlockData
);
DWORD STDCALL DdUpdateOverlay( 
HANDLE hSurfaceDestination,
HANDLE hSurfaceSource,
PDD_UPDATEOVERLAYDATA puUpdateOverlayData
);
DWORD STDCALL DdWaitForVerticalBlank( 
HANDLE hDirectDraw,
PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
);
