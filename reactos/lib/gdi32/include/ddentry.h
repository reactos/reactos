
#ifndef __D3D8THK_H
#define __D3D8THK_H

#include <ddk\ddrawint.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL STDCALL NtGdiD3dContextCreate(
    HANDLE hDirectDrawLocal,
    HANDLE hSurfColor,
    HANDLE hSurfZ,
    PD3DNTHAL_CONTEXTCREATEDATA pdcci
);

DWORD STDCALL NtGdiD3dContextDestroy(      
    PD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
);

DWORD STDCALL NtGdiD3dContextDestroyAll(VOID);

DWORD STDCALL NtGdiD3dDrawPrimitives2(      
    HANDLE hCmdBuf,
    HANDLE hVBuf,
    PD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    FLATPTR *pfpVidMemCmd,
    DWORD *pdwSizeCmd,
    FLATPTR *pfpVidMemVtx,
    DWORD *pdwSizeVtx
);

DWORD STDCALL NtGdiD3dValidateTextureStageState(      
    PD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData
);

DWORD STDCALL NtGdiDdAddAttachedSurface(      
    HANDLE hSurface,
    HANDLE hSurfaceAttached,
    PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
);

DWORD STDCALL NtGdiDdAlphaBlt(VOID);

BOOL STDCALL NtGdiDdAttachSurface(      
    HANDLE hSurfaceFrom,
    HANDLE hSurfaceTo
);

DWORD STDCALL NtGdiDdBeginMoCompFrame(      
    HANDLE hMoComp,
    PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
);

DWORD STDCALL NtGdiDdBlt(      
    HANDLE hSurfaceDest,
    HANDLE hSurfaceSrc,
    PDD_BLTDATA puBltData
);

DWORD STDCALL NtGdiDdCanCreateD3DBuffer(      
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

DWORD STDCALL NtGdiDdCanCreateSurface(      
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
);

DWORD STDCALL NtGdiDdColorControl(      
    HANDLE hSurface,
    PDD_COLORCONTROLDATA puColorControlData
);

DWORD STDCALL NtGdiDdCreateD3DBuffer(      
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
);

HANDLE STDCALL NtGdiDdCreateDirectDrawObject(      
    HDC hdc
);

HANDLE STDCALL NtGdiDdCreateMoComp(      
    HANDLE hDirectDraw,
    PDD_CREATEMOCOMPDATA puCreateMoCompData
);

DWORD STDCALL NtGdiDdCreateSurface(      
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
);

DWORD STDCALL NtGdiDdCreateSurfaceEx(      
    HANDLE hDirectDraw,
    HANDLE hSurface,
    DWORD dwSurfaceHandle
);

HANDLE STDCALL NtGdiDdCreateSurfaceObject(      
    HANDLE hDirectDrawLocal,
    HANDLE hSurface,
    PDD_SURFACE_LOCAL puSurfaceLocal,
    PDD_SURFACE_MORE puSurfaceMore,
    PDD_SURFACE_GLOBAL puSurfaceGlobal,
    BOOL bComplete
);

BOOL STDCALL NtGdiDdDeleteDirectDrawObject(      
    HANDLE hDirectDrawLocal
);

BOOL STDCALL NtGdiDdDeleteSurfaceObject(      
    HANDLE hSurface
);

DWORD STDCALL NtGdiDdDestroyD3DBuffer(      
    HANDLE hSurface
);

DWORD STDCALL NtGdiDdDestroyMoComp(      
    HANDLE hMoComp,
    PDD_DESTROYMOCOMPDATA puBeginFrameData
);

DWORD STDCALL NtGdiDdDestroySurface(      
    HANDLE hSurface,
    BOOL bRealDestroy
);

DWORD STDCALL NtGdiDdEndMoCompFrame(      
    HANDLE hMoComp,
    PDD_ENDMOCOMPFRAMEDATA puEndFrameData
);

DWORD STDCALL NtGdiDdFlip(      
    HANDLE hSurfaceCurrent,
    HANDLE hSurfaceTarget,
    HANDLE hSurfaceCurrentLeft,
    HANDLE hSurfaceTargetLeft,
    PDD_FLIPDATA puFlipData
);

DWORD STDCALL NtGdiDdFlipToGDISurface(      
    HANDLE hDirectDraw,
    PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData
);

DWORD STDCALL NtGdiDdGetAvailDriverMemory(      
    HANDLE hDirectDraw,
    PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
);

DWORD STDCALL NtGdiDdGetBltStatus(      
    HANDLE hSurface,
    PDD_GETBLTSTATUSDATA puGetBltStatusData
);

HDC STDCALL NtGdiDdGetDC(      
    HANDLE hSurface,
    PALETTEENTRY *puColorTable
);

DWORD STDCALL NtGdiDdGetDriverInfo(      
    HANDLE hDirectDraw,
    PDD_GETDRIVERINFODATA puGetDriverInfoData
);

DWORD STDCALL NtGdiDdGetDriverState(      
    PDD_GETDRIVERSTATEDATA pdata
);

DWORD STDCALL NtGdiDdGetDxHandle(      
    HANDLE hDirectDraw,
    HANDLE hSurface,
    BOOL bRelease
);

DWORD STDCALL NtGdiDdGetFlipStatus(      
    HANDLE hSurface,
    PDD_GETFLIPSTATUSDATA puGetFlipStatusData
);

DWORD STDCALL NtGdiDdGetInternalMoCompInfo(      
    HANDLE hDirectDraw,
    PDD_GETINTERNALMOCOMPDATA puGetInternalData
);

DWORD STDCALL NtGdiDdGetMoCompBuffInfo(      
    HANDLE hDirectDraw,
    PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
);

DWORD STDCALL NtGdiDdGetMoCompFormats(      
    HANDLE hDirectDraw,
    PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
);

DWORD STDCALL NtGdiDdGetMoCompGuids(      
    HANDLE hDirectDraw,
    PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData
);

DWORD STDCALL NtGdiDdGetScanLine(      
    HANDLE hDirectDraw,
    PDD_GETSCANLINEDATA puGetScanLineData
);

DWORD STDCALL NtGdiDdLock(      
    HANDLE hSurface,
    PDD_LOCKDATA puLockData,
    HDC hdcClip
);

DWORD STDCALL NtGdiDdLockD3D(      
    HANDLE hSurface,
    PDD_LOCKDATA puLockData
);

BOOL STDCALL NtGdiDdQueryDirectDrawObject(      
    HANDLE hDirectDrawLocal,
    DD_HALINFO *pHalInfo,
    DWORD *pCallBackFlags,
    PD3DNTHAL_CALLBACKS puD3dCallbacks,
    PD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
    PDD_D3DBUFCALLBACKS puD3dBufferCallbacks,
    LPDDSURFACEDESC puD3dTextureFormats,
    DWORD *puNumHeaps,
    VIDEOMEMORY *puvmList,
    DWORD *puNumFourCC,
    DWORD *puFourCC
);

DWORD STDCALL NtGdiDdQueryMoCompStatus(      
    HANDLE hMoComp,
    PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
);

BOOL STDCALL NtGdiDdReenableDirectDrawObject(      
    HANDLE hDirectDrawLocal,
    BOOL *pubNewMode
);

BOOL STDCALL NtGdiDdReleaseDC(      
    HANDLE hSurface
);

DWORD STDCALL NtGdiDdRenderMoComp(      
    HANDLE hMoComp,
    PDD_RENDERMOCOMPDATA puRenderMoCompData
);

BOOL STDCALL NtGdiDdResetVisrgn(      
    HANDLE hSurface,
    HWND hwnd
);

DWORD STDCALL NtGdiDdSetColorKey(      
    HANDLE hSurface,
    PDD_SETCOLORKEYDATA puSetColorKeyData
);

DWORD STDCALL NtGdiDdSetExclusiveMode(      
    HANDLE hDirectDraw,
    PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
);

BOOL STDCALL NtGdiDdSetGammaRamp(      
    HANDLE hDirectDraw,
    HDC hdc,
    LPVOID lpGammaRamp
);

DWORD STDCALL NtGdiDdSetOverlayPosition(      
    HANDLE hSurfaceSource,
    HANDLE hSurfaceDestination,
    PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
);

VOID STDCALL NtGdiDdUnattachSurface(      
    HANDLE hSurface,
    HANDLE hSurfaceAttached
);

DWORD STDCALL NtGdiDdUnlock(      
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
);

DWORD STDCALL NtGdiDdUnlockD3D(      
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
);

DWORD STDCALL NtGdiDdUpdateOverlay(      
    HANDLE hSurfaceDestination,
    HANDLE hSurfaceSource,
    PDD_UPDATEOVERLAYDATA puUpdateOverlayData
);

DWORD STDCALL NtGdiDdWaitForVerticalBlank(      
    HANDLE hDirectDraw,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __D3D8THK_H
