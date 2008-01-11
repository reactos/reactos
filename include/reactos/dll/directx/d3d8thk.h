
#ifndef __D3D8THK_H
#define __D3D8THK_H

#include <ddrawint.h>
#include <d3dnthal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME missing PD3DNTHAL_CONTEXTCREATEDATA
BOOL STDCALL OsThunkD3dContextCreate(
    HANDLE hDirectDrawLocal,
    HANDLE hSurfColor,
    HANDLE hSurfZ,
    PD3DNTHAL_CONTEXTCREATEDATA pdcci
);


DWORD STDCALL OsThunkD3dContextDestroy(
    PD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
);

DWORD STDCALL
    OsThunkD3dContextDestroyAll(PD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData);
*/

DWORD STDCALL OsThunkD3dContextDestroyAll(LPVOID);

/* FIXME PD3DNTHAL_DRAWPRIMITIVES2DATA, PD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA missing
DWORD STDCALL OsThunkD3dDrawPrimitives2(
    HANDLE hCmdBuf,
    HANDLE hVBuf,
    PD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    FLATPTR *pfpVidMemCmd,
    DWORD *pdwSizeCmd,
    FLATPTR *pfpVidMemVtx,
    DWORD *pdwSizeVtx
);

DWORD STDCALL OsThunkD3dValidateTextureStageState(
    PD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData
);
*/

DWORD STDCALL OsThunkDdAddAttachedSurface(
    HANDLE hSurface,
    HANDLE hSurfaceAttached,
    PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
);

DWORD STDCALL OsThunkDdAlphaBlt(VOID);

BOOL STDCALL OsThunkDdAttachSurface(
    HANDLE hSurfaceFrom,
    HANDLE hSurfaceTo
);

DWORD STDCALL OsThunkDdBeginMoCompFrame(
    HANDLE hMoComp,
    PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
);

DWORD STDCALL OsThunkDdBlt(
    HANDLE hSurfaceDest,
    HANDLE hSurfaceSrc,
    PDD_BLTDATA puBltData
);

DWORD STDCALL OsThunkDdCanCreateD3DBuffer(
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

DWORD STDCALL OsThunkDdCanCreateSurface(
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

DWORD STDCALL OsThunkDdColorControl(
    HANDLE hSurface,
    PDD_COLORCONTROLDATA puColorControlData
);

DWORD STDCALL OsThunkDdCreateD3DBuffer(
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
);

HANDLE STDCALL OsThunkDdCreateDirectDrawObject(HDC hdc);

HANDLE STDCALL OsThunkDdCreateMoComp(
    HANDLE hDirectDraw,
    PDD_CREATEMOCOMPDATA puCreateMoCompData
);

DWORD STDCALL OsThunkDdCreateSurface(
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
);

DWORD STDCALL OsThunkDdCreateSurfaceEx(
    HANDLE hDirectDraw,
    HANDLE hSurface,
    DWORD dwSurfaceHandle
);

HANDLE STDCALL OsThunkDdCreateSurfaceObject(
    HANDLE hDirectDrawLocal,
    HANDLE hSurface,
    PDD_SURFACE_LOCAL puSurfaceLocal,
    PDD_SURFACE_MORE puSurfaceMore,
    PDD_SURFACE_GLOBAL puSurfaceGlobal,
    BOOL bComplete
);

BOOL STDCALL OsThunkDdDeleteDirectDrawObject(
    HANDLE hDirectDrawLocal
);

BOOL STDCALL OsThunkDdDeleteSurfaceObject(
    HANDLE hSurface
);

DWORD STDCALL OsThunkDdDestroyD3DBuffer(
    HANDLE hSurface
);

DWORD STDCALL OsThunkDdDestroyMoComp(
    HANDLE hMoComp,
    PDD_DESTROYMOCOMPDATA puBeginFrameData
);

DWORD STDCALL OsThunkDdDestroySurface(
    HANDLE hSurface,
    BOOL bRealDestroy
);

DWORD STDCALL OsThunkDdEndMoCompFrame(
    HANDLE hMoComp,
    PDD_ENDMOCOMPFRAMEDATA puEndFrameData
);

DWORD STDCALL OsThunkDdFlip(
    HANDLE hSurfaceCurrent,
    HANDLE hSurfaceTarget,
    HANDLE hSurfaceCurrentLeft,
    HANDLE hSurfaceTargetLeft,
    PDD_FLIPDATA puFlipData
);

DWORD STDCALL OsThunkDdFlipToGDISurface(
    HANDLE hDirectDraw,
    PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData
);

DWORD STDCALL OsThunkDdGetAvailDriverMemory(
    HANDLE hDirectDraw,
    PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
);

DWORD STDCALL OsThunkDdGetBltStatus(
    HANDLE hSurface,
    PDD_GETBLTSTATUSDATA puGetBltStatusData
);

HDC STDCALL OsThunkDdGetDC(
    HANDLE hSurface,
    PALETTEENTRY *puColorTable
);

DWORD STDCALL OsThunkDdGetDriverInfo(
    HANDLE hDirectDraw,
    PDD_GETDRIVERINFODATA puGetDriverInfoData
);

DWORD STDCALL OsThunkDdGetDriverState(
    PDD_GETDRIVERSTATEDATA pdata
);

DWORD STDCALL OsThunkDdGetDxHandle(
    HANDLE hDirectDraw,
    HANDLE hSurface,
    BOOL bRelease
);

DWORD STDCALL OsThunkDdGetFlipStatus(
    HANDLE hSurface,
    PDD_GETFLIPSTATUSDATA puGetFlipStatusData
);

DWORD STDCALL OsThunkDdGetInternalMoCompInfo(
    HANDLE hDirectDraw,
    PDD_GETINTERNALMOCOMPDATA puGetInternalData
);

DWORD STDCALL OsThunkDdGetMoCompBuffInfo(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
);

DWORD STDCALL OsThunkDdGetMoCompFormats(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
);

DWORD STDCALL OsThunkDdGetMoCompGuids(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData
);

DWORD STDCALL OsThunkDdGetScanLine(
    HANDLE hDirectDraw,
    PDD_GETSCANLINEDATA puGetScanLineData
);

DWORD STDCALL OsThunkDdLock(
    HANDLE hSurface,
    PDD_LOCKDATA puLockData,
    HDC hdcClip
);

DWORD STDCALL OsThunkDdLockD3D(
    HANDLE hSurface,
    PDD_LOCKDATA puLockData
);


BOOL STDCALL OsThunkDdQueryDirectDrawObject(
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


DWORD STDCALL OsThunkDdQueryMoCompStatus(
    HANDLE hMoComp,
    PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
);

BOOL STDCALL OsThunkDdReenableDirectDrawObject(
    HANDLE hDirectDrawLocal,
    BOOL *pubNewMode
);

BOOL STDCALL OsThunkDdReleaseDC(
    HANDLE hSurface
);

DWORD STDCALL OsThunkDdRenderMoComp(
    HANDLE hMoComp,
    PDD_RENDERMOCOMPDATA puRenderMoCompData
);

BOOL STDCALL OsThunkDdResetVisrgn(
    HANDLE hSurface,
    HWND hwnd
);

DWORD STDCALL OsThunkDdSetColorKey(
    HANDLE hSurface,
    PDD_SETCOLORKEYDATA puSetColorKeyData
);

DWORD STDCALL OsThunkDdSetExclusiveMode(
    HANDLE hDirectDraw,
    PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
);

BOOL STDCALL OsThunkDdSetGammaRamp(
    HANDLE hDirectDraw,
    HDC hdc,
    LPVOID lpGammaRamp
);

DWORD STDCALL OsThunkDdSetOverlayPosition(
    HANDLE hSurfaceSource,
    HANDLE hSurfaceDestination,
    PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
);

VOID STDCALL OsThunkDdUnattachSurface(
    HANDLE hSurface,
    HANDLE hSurfaceAttached
);

DWORD STDCALL OsThunkDdUnlock(
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
);

DWORD STDCALL OsThunkDdUnlockD3D(
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
);

DWORD STDCALL OsThunkDdUpdateOverlay(
    HANDLE hSurfaceDestination,
    HANDLE hSurfaceSource,
    PDD_UPDATEOVERLAYDATA puUpdateOverlayData
);

DWORD STDCALL OsThunkDdWaitForVerticalBlank(
    HANDLE hDirectDraw,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __D3D8THK_H
