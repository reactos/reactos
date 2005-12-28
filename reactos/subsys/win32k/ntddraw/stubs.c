/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw stubs
 * FILE:             subsys/win32k/ntddraw/stubs.c
 * PROGRAMER:        Peter Bajusz (hyp-x@stormregion.com)
 * REVISION HISTORY:
 *       25-10-2003  PB  Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>




DWORD STDCALL NtGdiD3dContextCreate(
    HANDLE hDirectDrawLocal,
    HANDLE hSurfColor,
    HANDLE hSurfZ,
    LPD3DNTHAL_CONTEXTCREATEDATA pdcci
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiD3dContextDestroy(
    LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiD3dContextDestroyAll(LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcad)
{
	/* This entry point is not supported on NT5 and ROS */
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiD3dDrawPrimitives2(
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

DWORD STDCALL NtGdiD3dValidateTextureStageState(
    LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData
)
{
	UNIMPLEMENTED

	return 0;
}





BOOL STDCALL NtGdiDdAttachSurface(
    HANDLE hSurfaceFrom,
    HANDLE hSurfaceTo
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiDdBeginMoCompFrame(
    HANDLE hMoComp,
    PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData
)
{
	UNIMPLEMENTED

	return 0;
}


DWORD STDCALL NtGdiDdCanCreateD3DBuffer(
    HANDLE hDirectDraw,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
)
{
	UNIMPLEMENTED

	return 0;
}



DWORD STDCALL NtGdiDdColorControl(
    HANDLE hSurface,
    PDD_COLORCONTROLDATA puColorControlData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiDdCreateD3DBuffer(
    HANDLE hDirectDraw,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
)
{
	UNIMPLEMENTED

	return 0;
}



HANDLE STDCALL NtGdiDdCreateMoComp(
    HANDLE hDirectDraw,
    PDD_CREATEMOCOMPDATA puCreateMoCompData
)
{
	UNIMPLEMENTED

	return 0;
}



DWORD STDCALL NtGdiDdCreateSurfaceEx(
    HANDLE hDirectDraw,
    HANDLE hSurface,
    DWORD dwSurfaceHandle
)
{
	UNIMPLEMENTED

	return 0;
}



DWORD STDCALL NtGdiDdDestroyD3DBuffer(
    HANDLE hSurface
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiDdDestroyMoComp(
    HANDLE hMoComp,
    PDD_DESTROYMOCOMPDATA puBeginFrameData
)
{
	UNIMPLEMENTED

	return 0;
}



DWORD STDCALL NtGdiDdEndMoCompFrame(
    HANDLE hMoComp,
    PDD_ENDMOCOMPFRAMEDATA puEndFrameData
)
{
	UNIMPLEMENTED

	return 0;
}



DWORD STDCALL NtGdiDdFlipToGDISurface(
    HANDLE hDirectDraw,
    PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData
)
{
	UNIMPLEMENTED

	return 0;
}




HDC STDCALL NtGdiDdGetDC(
    HANDLE hSurface,
    PALETTEENTRY *puColorTable
)
{
	UNIMPLEMENTED

	return 0;
}



DWORD STDCALL NtGdiDdGetDriverState(
    PDD_GETDRIVERSTATEDATA pdata
)
{
	UNIMPLEMENTED

	return 0;
}

HANDLE STDCALL NtGdiDdGetDxHandle(
    HANDLE hDirectDraw,
    HANDLE hSurface,
    BOOL bRelease
)
{
	UNIMPLEMENTED

	return 0;
}



DWORD STDCALL NtGdiDdGetInternalMoCompInfo(
    HANDLE hDirectDraw,
    PDD_GETINTERNALMOCOMPDATA puGetInternalData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiDdGetMoCompBuffInfo(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiDdGetMoCompFormats(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiDdGetMoCompGuids(
    HANDLE hDirectDraw,
    PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData
)
{
	UNIMPLEMENTED

	return 0;
}





DWORD STDCALL NtGdiDdLockD3D(
    HANDLE hSurface,
    PDD_LOCKDATA puLockData
)
{
	UNIMPLEMENTED

	return 0;
}


DWORD STDCALL NtGdiDdQueryMoCompStatus(
    HANDLE hMoComp,
    PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData
)
{
	UNIMPLEMENTED

	return 0;
}

BOOL STDCALL NtGdiDdReenableDirectDrawObject(
    HANDLE hDirectDrawLocal,
    BOOL *pubNewMode
)
{
	UNIMPLEMENTED

	return 0;
}

BOOL STDCALL NtGdiDdReleaseDC(
    HANDLE hSurface
)
{
	UNIMPLEMENTED

	return 0;
}

DWORD STDCALL NtGdiDdRenderMoComp(
    HANDLE hMoComp,
    PDD_RENDERMOCOMPDATA puRenderMoCompData
)
{
	UNIMPLEMENTED

	return 0;
}

BOOL STDCALL NtGdiDdResetVisrgn(
    HANDLE hSurface,
    HWND hwnd
)
{
	UNIMPLEMENTED

	return 0;
}





BOOL STDCALL NtGdiDdSetGammaRamp(
    HANDLE hDirectDraw,
    HDC hdc,
    LPVOID lpGammaRamp
)
{
	UNIMPLEMENTED

	return 0;
}



VOID STDCALL NtGdiDdUnattachSurface(
    HANDLE hSurface,
    HANDLE hSurfaceAttached
)
{
	UNIMPLEMENTED
}



DWORD STDCALL NtGdiDdUnlockD3D(
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
)
{
	UNIMPLEMENTED

	return 0;
}




DWORD STDCALL NtGdiDdAlphaBlt(IN HANDLE hSurfaceDest,
                              IN OPTIONAL HANDLE hSurfaceSrc,
                              IN OUT PDD_BLTDATA puBltData)
{
	UNIMPLEMENTED

	return 0;
}
/* EOF */
