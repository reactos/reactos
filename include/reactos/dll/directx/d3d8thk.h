
#ifndef __D3D8THK_H
#define __D3D8THK_H

#include <ddrawint.h>
#include <d3dnthal.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI OsThunkD3dContextCreate(
    HANDLE hDirectDrawLocal,
    HANDLE hSurfColor,
    HANDLE hSurfZ,
    LPD3DNTHAL_CONTEXTDESTROYDATA pdcci
);


DWORD WINAPI OsThunkD3dContextDestroy(
    LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
);

DWORD WINAPI OsThunkD3dContextDestroyAll(
    LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
);

DWORD WINAPI OsThunkD3dDrawPrimitives2(
    HANDLE hCmdBuf,
    HANDLE hVBuf,
    LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    FLATPTR *pfpVidMemCmd,
    DWORD *pdwSizeCmd,
    FLATPTR *pfpVidMemVtx,
    DWORD *pdwSizeVtx
);

DWORD WINAPI OsThunkD3dValidateTextureStageState(
    LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData
);


DWORD WINAPI OsThunkDdAddAttachedSurface(
    HANDLE hSurface,
    HANDLE hSurfaceAttached,
    PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
);

DWORD WINAPI OsThunkDdAlphaBlt(VOID);

BOOL WINAPI OsThunkDdAttachSurface(
    HANDLE hSurfaceFrom,
    HANDLE hSurfaceTo
);

DWORD WINAPI OsThunkDdBeginMoCompFrame(
    HANDLE hMoComp,
    PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
);

DWORD WINAPI OsThunkDdBlt(
    HANDLE hSurfaceDest,
    HANDLE hSurfaceSrc,
    PDD_BLTDATA puBltData
);

DWORD WINAPI OsThunkDdCanCreateD3DBuffer(
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

DWORD WINAPI OsThunkDdCanCreateSurface(
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

DWORD WINAPI OsThunkDdColorControl(
    HANDLE hSurface,
    PDD_COLORCONTROLDATA puColorControlData
);

DWORD WINAPI OsThunkDdCreateD3DBuffer(
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
);

HANDLE WINAPI OsThunkDdCreateDirectDrawObject(HDC hdc);

HANDLE WINAPI OsThunkDdCreateMoComp(
    HANDLE hDirectDraw,
    PDD_CREATEMOCOMPDATA puCreateMoCompData
);

DWORD WINAPI OsThunkDdCreateSurface(
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
);

DWORD WINAPI OsThunkDdCreateSurfaceEx(
    HANDLE hDirectDraw,
    HANDLE hSurface,
    DWORD dwSurfaceHandle
);

HANDLE WINAPI OsThunkDdCreateSurfaceObject(
    HANDLE hDirectDrawLocal,
    HANDLE hSurface,
    PDD_SURFACE_LOCAL puSurfaceLocal,
    PDD_SURFACE_MORE puSurfaceMore,
    PDD_SURFACE_GLOBAL puSurfaceGlobal,
    BOOL bComplete
);

BOOL WINAPI OsThunkDdDeleteDirectDrawObject(
    HANDLE hDirectDrawLocal
);

BOOL WINAPI OsThunkDdDeleteSurfaceObject(
    HANDLE hSurface
);

DWORD WINAPI OsThunkDdDestroyD3DBuffer(
    HANDLE hSurface
);

DWORD WINAPI OsThunkDdDestroyMoComp(
    HANDLE hMoComp,
    PDD_DESTROYMOCOMPDATA puBeginFrameData
);

DWORD WINAPI OsThunkDdDestroySurface(
    HANDLE hSurface,
    BOOL bRealDestroy
);

DWORD WINAPI OsThunkDdEndMoCompFrame(
    HANDLE hMoComp,
    PDD_ENDMOCOMPFRAMEDATA puEndFrameData
);

DWORD WINAPI OsThunkDdFlip(
    HANDLE hSurfaceCurrent,
    HANDLE hSurfaceTarget,
    HANDLE hSurfaceCurrentLeft,
    HANDLE hSurfaceTargetLeft,
    PDD_FLIPDATA puFlipData
);

DWORD WINAPI OsThunkDdFlipToGDISurface(
    HANDLE hDirectDraw,
    PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData
);

DWORD WINAPI OsThunkDdGetAvailDriverMemory(
    HANDLE hDirectDraw,
    PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
);

DWORD WINAPI OsThunkDdGetBltStatus(
    HANDLE hSurface,
    PDD_GETBLTSTATUSDATA puGetBltStatusData
);

HDC WINAPI OsThunkDdGetDC(
    HANDLE hSurface,
    PALETTEENTRY *puColorTable
);

DWORD WINAPI OsThunkDdGetDriverInfo(
    HANDLE hDirectDraw,
    PDD_GETDRIVERINFODATA puGetDriverInfoData
);

DWORD WINAPI OsThunkDdGetDriverState(
    PDD_GETDRIVERSTATEDATA pdata
);

DWORD WINAPI OsThunkDdGetDxHandle(
    HANDLE hDirectDraw,
    HANDLE hSurface,
    BOOL bRelease
);

DWORD WINAPI OsThunkDdGetFlipStatus(
    HANDLE hSurface,
    PDD_GETFLIPSTATUSDATA puGetFlipStatusData
);

DWORD WINAPI OsThunkDdGetInternalMoCompInfo(
    HANDLE hDirectDraw,
    PDD_GETINTERNALMOCOMPDATA puGetInternalData
);

DWORD WINAPI OsThunkDdGetMoCompBuffInfo(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
);

DWORD WINAPI OsThunkDdGetMoCompFormats(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
);

DWORD WINAPI OsThunkDdGetMoCompGuids(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData
);

DWORD WINAPI OsThunkDdGetScanLine(
    HANDLE hDirectDraw,
    PDD_GETSCANLINEDATA puGetScanLineData
);

DWORD WINAPI OsThunkDdLock(
    HANDLE hSurface,
    PDD_LOCKDATA puLockData,
    HDC hdcClip
);

DWORD WINAPI OsThunkDdLockD3D(
    HANDLE hSurface,
    PDD_LOCKDATA puLockData
);


BOOL WINAPI OsThunkDdQueryDirectDrawObject(
    HANDLE hDirectDrawLocal,
    DD_HALINFO  *pHalInfo,
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


DWORD WINAPI OsThunkDdQueryMoCompStatus(
    HANDLE hMoComp,
    PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
);

BOOL WINAPI OsThunkDdReenableDirectDrawObject(
    HANDLE hDirectDrawLocal,
    BOOL *pubNewMode
);

BOOL WINAPI OsThunkDdReleaseDC(
    HANDLE hSurface
);

DWORD WINAPI OsThunkDdRenderMoComp(
    HANDLE hMoComp,
    PDD_RENDERMOCOMPDATA puRenderMoCompData
);

BOOL WINAPI OsThunkDdResetVisrgn(
    HANDLE hSurface,
    HWND hwnd
);

DWORD WINAPI OsThunkDdSetColorKey(
    HANDLE hSurface,
    PDD_SETCOLORKEYDATA puSetColorKeyData
);

DWORD WINAPI OsThunkDdSetExclusiveMode(
    HANDLE hDirectDraw,
    PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
);

BOOL WINAPI OsThunkDdSetGammaRamp(
    HANDLE hDirectDraw,
    HDC hdc,
    LPVOID lpGammaRamp
);

DWORD WINAPI OsThunkDdSetOverlayPosition(
    HANDLE hSurfaceSource,
    HANDLE hSurfaceDestination,
    PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
);

VOID WINAPI OsThunkDdUnattachSurface(
    HANDLE hSurface,
    HANDLE hSurfaceAttached
);

DWORD WINAPI OsThunkDdUnlock(
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
);

DWORD WINAPI OsThunkDdUnlockD3D(
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
);

DWORD WINAPI OsThunkDdUpdateOverlay(
    HANDLE hSurfaceDestination,
    HANDLE hSurfaceSource,
    PDD_UPDATEOVERLAYDATA puUpdateOverlayData
);

DWORD WINAPI OsThunkDdWaitForVerticalBlank(
    HANDLE hDirectDraw,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __D3D8THK_H
