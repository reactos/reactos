
#ifndef __WIN32K_NTDDRAW_H
#define __WIN32K_NTDDRAW_H

#include <windows.h>
#include <ddk/d3dhal.h>

typedef D3DHAL_CONTEXTCREATEDATA D3DNTHAL_CONTEXTCREATEDATA;

BOOL APIENTRY NtGdiD3dContextCreate(
    HANDLE hDirectDrawLocal,
    HANDLE hSurfColor,
    HANDLE hSurfZ,
    D3DNTHAL_CONTEXTCREATEDATA *pdcci
);

DWORD APIENTRY NtGdiD3dContextDestroy(      
    LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
);

DWORD APIENTRY NtGdiD3dContextDestroyAll(VOID);

DWORD APIENTRY NtGdiD3dDrawPrimitives2(      
    HANDLE hCmdBuf,
    HANDLE hVBuf,
    LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    FLATPTR *pfpVidMemCmd,
    DWORD *pdwSizeCmd,
    FLATPTR *pfpVidMemVtx,
    DWORD *pdwSizeVtx
);

DWORD APIENTRY NtGdiD3dValidateTextureStageState(      
    LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData
);

DWORD APIENTRY NtGdiDdAddAttachedSurface(      
    HANDLE hSurface,
    HANDLE hSurfaceAttached,
    PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
);

DWORD APIENTRY NtGdiDdAlphaBlt(VOID);

BOOL APIENTRY NtGdiDdAttachSurface(      
    HANDLE hSurfaceFrom,
    HANDLE hSurfaceTo
);

DWORD APIENTRY NtGdiDdBeginMoCompFrame(      
    HANDLE hMoComp,
    PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
);

DWORD APIENTRY NtGdiDdBlt(      
    HANDLE hSurfaceDest,
    HANDLE hSurfaceSrc,
    PDD_BLTDATA puBltData
);

DWORD APIENTRY NtGdiDdCanCreateD3DBuffer(      
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

DWORD APIENTRY NtGdiDdCanCreateSurface(      
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

DWORD APIENTRY NtGdiDdColorControl(      
    HANDLE hSurface,
    PDD_COLORCONTROLDATA puColorControlData
);

DWORD APIENTRY NtGdiDdCreateD3DBuffer(      
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    DD_CREATESURFACEDATA *puCreateSurfaceData,
    HANDLE *puhSurface
);

HANDLE APIENTRY NtGdiDdCreateDirectDrawObject(      
    HDC hdc
);

HANDLE APIENTRY NtGdiDdCreateMoComp(      
    HANDLE hDirectDraw,
    PDD_CREATEMOCOMPDATA puCreateMoCompData
);

DWORD APIENTRY NtGdiDdCreateSurface(      
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    DD_CREATESURFACEDATA *puCreateSurfaceData,
    HANDLE *puhSurface
);

DWORD APIENTRY NtGdiDdCreateSurfaceEx(      
    HANDLE hDirectDraw,
    HANDLE hSurface,
    DWORD dwSurfaceHandle
);

HANDLE APIENTRY NtGdiDdCreateSurfaceObject(      
    HANDLE hDirectDrawLocal,
    HANDLE hSurface,
    PDD_SURFACE_LOCAL puSurfaceLocal,
    PDD_SURFACE_MORE puSurfaceMore,
    PDD_SURFACE_GLOBAL puSurfaceGlobal,
    BOOL bComplete
);

BOOL APIENTRY NtGdiDdDeleteDirectDrawObject(      
    HANDLE hDirectDrawLocal
);

BOOL APIENTRY NtGdiDdDeleteSurfaceObject(      
    HANDLE hSurface
);

DWORD APIENTRY NtGdiDdDestroyD3DBuffer(      
    HANDLE hSurface
);

DWORD APIENTRY NtGdiDdDestroyMoComp(      
    HANDLE hMoComp,
    PDD_DESTROYMOCOMPDATA puBeginFrameData
);

DWORD APIENTRY NtGdiDdDestroySurface(      
    HANDLE hSurface,
    BOOL bRealDestroy
);

DWORD APIENTRY NtGdiDdEndMoCompFrame(      
    HANDLE hMoComp,
    PDD_ENDMOCOMPFRAMEDATA puEndFrameData
);

DWORD APIENTRY NtGdiDdFlip(      
    HANDLE hSurfaceCurrent,
    HANDLE hSurfaceTarget,
    HANDLE hSurfaceCurrentLeft,
    HANDLE hSurfaceTargetLeft,
    PDD_FLIPDATA puFlipData
);

DWORD APIENTRY NtGdiDdFlipToGDISurface(      
    HANDLE hDirectDraw,
    PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData
);

DWORD APIENTRY NtGdiDdGetAvailDriverMemory(      
    HANDLE hDirectDraw,
    PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
);

DWORD APIENTRY NtGdiDdGetBltStatus(      
    HANDLE hSurface,
    PDD_GETBLTSTATUSDATA puGetBltStatusData
);

HDC APIENTRY NtGdiDdGetDC(      
    HANDLE hSurface,
    PALETTEENTRY *puColorTable
);

DWORD APIENTRY NtGdiDdGetDriverInfo(      
    HANDLE hDirectDraw,
    PDD_GETDRIVERINFODATA puGetDriverInfoData
);

DWORD APIENTRY NtGdiDdGetDriverState(      
    PDD_GETDRIVERSTATEDATA pdata
);

DWORD APIENTRY NtGdiDdGetDxHandle(      
    HANDLE hDirectDraw,
    HANDLE hSurface,
    BOOL bRelease
);

DWORD APIENTRY NtGdiDdGetFlipStatus(      
    HANDLE hSurface,
    PDD_GETFLIPSTATUSDATA puGetFlipStatusData
);

DWORD APIENTRY NtGdiDdGetInternalMoCompInfo(      
    HANDLE hDirectDraw,
    PDD_GETINTERNALMOCOMPDATA puGetInternalData
);

DWORD APIENTRY NtGdiDdGetMoCompBuffInfo(      
    HANDLE hDirectDraw,
    PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
);

DWORD APIENTRY NtGdiDdGetMoCompFormats(      
    HANDLE hDirectDraw,
    PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
);

DWORD APIENTRY NtGdiDdGetMoCompGuids(      
    HANDLE hDirectDraw,
    PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData
);

DWORD APIENTRY NtGdiDdGetScanLine(      
    HANDLE hDirectDraw,
    PDD_GETSCANLINEDATA puGetScanLineData
);

DWORD APIENTRY NtGdiDdLock(      
    HANDLE hSurface,
    PDD_LOCKDATA puLockData,
    HDC hdcClip
);

DWORD APIENTRY NtGdiDdLockD3D(      
    HANDLE hSurface,
    PDD_LOCKDATA puLockData
);

BOOL APIENTRY NtGdiDdQueryDirectDrawObject(      
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

DWORD APIENTRY NtGdiDdQueryMoCompStatus(      
    HANDLE hMoComp,
    PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
);

BOOL APIENTRY NtGdiDdReenableDirectDrawObject(      
    HANDLE hDirectDrawLocal,
    BOOL *pubNewMode
);

BOOL APIENTRY NtGdiDdReleaseDC(      
    HANDLE hSurface
);

DWORD APIENTRY NtGdiDdRenderMoComp(      
    HANDLE hMoComp,
    PDD_RENDERMOCOMPDATA puRenderMoCompData
);

BOOL APIENTRY NtGdiDdResetVisrgn(      
    HANDLE hSurface,
    HWND hwnd
);

DWORD APIENTRY NtGdiDdSetColorKey(      
    HANDLE hSurface,
    PDD_SETCOLORKEYDATA puSetColorKeyData
);

DWORD APIENTRY NtGdiDdSetExclusiveMode(      
    HANDLE hDirectDraw,
    PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
);

BOOL APIENTRY NtGdiDdSetGammaRamp(      
    HANDLE hDirectDraw,
    HDC hdc,
    LPVOID lpGammaRamp
);

DWORD APIENTRY NtGdiDdSetOverlayPosition(      
    HANDLE hSurfaceSource,
    HANDLE hSurfaceDestination,
    PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
);

VOID APIENTRY NtGdiDdUnattachSurface(      
    HANDLE hSurface,
    HANDLE hSurfaceAttached
);

DWORD APIENTRY NtGdiDdUnlock(      
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
);

DWORD APIENTRY NtGdiDdUnlockD3D(      
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
);

DWORD APIENTRY NtGdiDdUpdateOverlay(      
    HANDLE hSurfaceDestination,
    HANDLE hSurfaceSource,
    PDD_UPDATEOVERLAYDATA puUpdateOverlayData
);

DWORD APIENTRY NtGdiDdWaitForVerticalBlank(      
    HANDLE hDirectDraw,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
);


#endif
