/* 
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw stubs
 * FILE:             subsys/win32k/ntddraw/stubs.c
 * PROGRAMER:        Peter Bajusz (hyp-x@stormregion.com)
 * REVISION HISTORY:
 *       25-10-2003  PB  Created
 */
#include <ddk/ntddk.h>
#include <win32k/win32k.h>

#define NDEBUG
#include <debug.h>


BOOL APIENTRY NtGdiD3dContextCreate(
    HANDLE hDirectDrawLocal,
    HANDLE hSurfColor,
    HANDLE hSurfZ,
    D3DNTHAL_CONTEXTCREATEDATA *pdcci
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiD3dContextDestroy(      
    LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiD3dContextDestroyAll(VOID)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiD3dDrawPrimitives2(      
    HANDLE hCmdBuf,
    HANDLE hVBuf,
    LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    FLATPTR *pfpVidMemCmd,
    DWORD *pdwSizeCmd,
    FLATPTR *pfpVidMemVtx,
    DWORD *pdwSizeVtx
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiD3dValidateTextureStageState(      
    LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdAddAttachedSurface(      
    HANDLE hSurface,
    HANDLE hSurfaceAttached,
    PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdAlphaBlt(VOID)
{
	UNIMPLEMENTED

	return 0;
}

BOOL APIENTRY NtGdiDdAttachSurface(      
    HANDLE hSurfaceFrom,
    HANDLE hSurfaceTo
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdBeginMoCompFrame(      
    HANDLE hMoComp,
    PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdBlt(      
    HANDLE hSurfaceDest,
    HANDLE hSurfaceSrc,
    PDD_BLTDATA puBltData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdCanCreateD3DBuffer(      
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdCanCreateSurface(      
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdColorControl(      
    HANDLE hSurface,
    PDD_COLORCONTROLDATA puColorControlData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdCreateD3DBuffer(      
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    DD_CREATESURFACEDATA *puCreateSurfaceData,
    HANDLE *puhSurface
)
{
	UNIMPLEMENTED

	return 0;
}

HANDLE APIENTRY NtGdiDdCreateDirectDrawObject(      
    HDC hdc
)
{
	UNIMPLEMENTED

	return 0;
}

HANDLE APIENTRY NtGdiDdCreateMoComp(      
    HANDLE hDirectDraw,
    PDD_CREATEMOCOMPDATA puCreateMoCompData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdCreateSurface(      
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    DD_CREATESURFACEDATA *puCreateSurfaceData,
    HANDLE *puhSurface
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdCreateSurfaceEx(      
    HANDLE hDirectDraw,
    HANDLE hSurface,
    DWORD dwSurfaceHandle
)
{
	UNIMPLEMENTED

	return 0;
}

HANDLE APIENTRY NtGdiDdCreateSurfaceObject(      
    HANDLE hDirectDrawLocal,
    HANDLE hSurface,
    PDD_SURFACE_LOCAL puSurfaceLocal,
    PDD_SURFACE_MORE puSurfaceMore,
    PDD_SURFACE_GLOBAL puSurfaceGlobal,
    BOOL bComplete
)
{
	UNIMPLEMENTED

	return 0;
}

BOOL APIENTRY NtGdiDdDeleteDirectDrawObject(      
    HANDLE hDirectDrawLocal
)
{
	UNIMPLEMENTED

	return 0;
}

BOOL APIENTRY NtGdiDdDeleteSurfaceObject(      
    HANDLE hSurface
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdDestroyD3DBuffer(      
    HANDLE hSurface
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdDestroyMoComp(      
    HANDLE hMoComp,
    PDD_DESTROYMOCOMPDATA puBeginFrameData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdDestroySurface(      
    HANDLE hSurface,
    BOOL bRealDestroy
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdEndMoCompFrame(      
    HANDLE hMoComp,
    PDD_ENDMOCOMPFRAMEDATA puEndFrameData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdFlip(      
    HANDLE hSurfaceCurrent,
    HANDLE hSurfaceTarget,
    HANDLE hSurfaceCurrentLeft,
    HANDLE hSurfaceTargetLeft,
    PDD_FLIPDATA puFlipData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdFlipToGDISurface(      
    HANDLE hDirectDraw,
    PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetAvailDriverMemory(      
    HANDLE hDirectDraw,
    PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetBltStatus(      
    HANDLE hSurface,
    PDD_GETBLTSTATUSDATA puGetBltStatusData
)
{
	UNIMPLEMENTED

	return 0;
}

HDC APIENTRY NtGdiDdGetDC(      
    HANDLE hSurface,
    PALETTEENTRY *puColorTable
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetDriverInfo(      
    HANDLE hDirectDraw,
    PDD_GETDRIVERINFODATA puGetDriverInfoData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetDriverState(      
    PDD_GETDRIVERSTATEDATA pdata
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetDxHandle(      
    HANDLE hDirectDraw,
    HANDLE hSurface,
    BOOL bRelease
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetFlipStatus(      
    HANDLE hSurface,
    PDD_GETFLIPSTATUSDATA puGetFlipStatusData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetInternalMoCompInfo(      
    HANDLE hDirectDraw,
    PDD_GETINTERNALMOCOMPDATA puGetInternalData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetMoCompBuffInfo(      
    HANDLE hDirectDraw,
    PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetMoCompFormats(      
    HANDLE hDirectDraw,
    PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetMoCompGuids(      
    HANDLE hDirectDraw,
    PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdGetScanLine(      
    HANDLE hDirectDraw,
    PDD_GETSCANLINEDATA puGetScanLineData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdLock(      
    HANDLE hSurface,
    PDD_LOCKDATA puLockData,
    HDC hdcClip
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdLockD3D(      
    HANDLE hSurface,
    PDD_LOCKDATA puLockData
)
{
	UNIMPLEMENTED

	return 0;
}

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
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdQueryMoCompStatus(      
    HANDLE hMoComp,
    PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
)
{
	UNIMPLEMENTED

	return 0;
}

BOOL APIENTRY NtGdiDdReenableDirectDrawObject(      
    HANDLE hDirectDrawLocal,
    BOOL *pubNewMode
)
{
	UNIMPLEMENTED

	return 0;
}

BOOL APIENTRY NtGdiDdReleaseDC(      
    HANDLE hSurface
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdRenderMoComp(      
    HANDLE hMoComp,
    PDD_RENDERMOCOMPDATA puRenderMoCompData
)
{
	UNIMPLEMENTED

	return 0;
}

BOOL APIENTRY NtGdiDdResetVisrgn(      
    HANDLE hSurface,
    HWND hwnd
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdSetColorKey(      
    HANDLE hSurface,
    PDD_SETCOLORKEYDATA puSetColorKeyData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdSetExclusiveMode(      
    HANDLE hDirectDraw,
    PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
)
{
	UNIMPLEMENTED

	return 0;
}

BOOL APIENTRY NtGdiDdSetGammaRamp(      
    HANDLE hDirectDraw,
    HDC hdc,
    LPVOID lpGammaRamp
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdSetOverlayPosition(      
    HANDLE hSurfaceSource,
    HANDLE hSurfaceDestination,
    PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
)
{
	UNIMPLEMENTED

	return 0;
}

VOID APIENTRY NtGdiDdUnattachSurface(      
    HANDLE hSurface,
    HANDLE hSurfaceAttached
)
{
	UNIMPLEMENTED
}

DWORD APIENTRY NtGdiDdUnlock(      
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdUnlockD3D(      
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdUpdateOverlay(      
    HANDLE hSurfaceDestination,
    HANDLE hSurfaceSource,
    PDD_UPDATEOVERLAYDATA puUpdateOverlayData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD APIENTRY NtGdiDdWaitForVerticalBlank(      
    HANDLE hDirectDraw,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
)
{
	UNIMPLEMENTED

	return 0;
}

/* EOF */
