/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/stub.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       16/12-2010   Magnus Olsen
 */


#include <dxg_int.h>
#include "dxg_driver.h"

#define stub DPRINT1("UNIMPLEMENT"); /
					return DDERR_UNSUPPORTED;
					

NTSTATUS
NTAPI
DxD3dContextCreate(DWORD x1, DWORD x2, DWORD x3, DWORD x4)
{
	stub;
}

NTSTATUS
NTAPI
DxD3dContextDestroy(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxD3dContextDestroyAll(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxD3dValidateTextureStageState(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxD3dDrawPrimitives2(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetDriverState(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxDdAddAttachedSurface(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdAlphaBlt(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdAttachSurface(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdBeginMoCompFrame)(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdBlt(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdCanCreateSurface(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdCanCreateD3DBuffer)(
{
	stub;
}

NTSTATUS
NTAPI
DxDdColorControl(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdCreateDirectDrawObject(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxDdCreateSurface(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7, DWORD x8)
{
	/* NOTE : DxDdCreateD3DBuffer and DxDdCreateSurface are same */ 
	stub;
}

NTSTATUS
NTAPI
DxDdCreateMoComp(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdCreateSurfaceObject(DWORD x1, DWORD x2, DWORD x2, DWORD x3, DWORD x4, DWORD x5)
{
	stub;
}

NTSTATUS
NTAPI
DxDdDeleteDirectDrawObject(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxDdDeleteSurfaceObject(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxDdDestroyMoComp)(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdDestroySurface(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdDestroyD3DBuffer(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxDdEndMoCompFrame(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdFlip(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5)
{
	stub;
}

NTSTATUS
NTAPI
DxDdFlipToGDISurface(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetAvailDriverMemory(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetBltStatus(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetDC(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetDriverInfo(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetDxHandle(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetFlipStatus(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetInternalMoCompInfo(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetMoCompBuffInfo(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetMoCompGuids(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetMoCompFormats(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetScanLine(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdLock(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdLockD3D(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdQueryDirectDrawObject(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7, DWORD x8, DWORD x9 , DWORD x10 , DWORD x11)
{
	stub;
}

NTSTATUS
NTAPI
DxDdQueryMoCompStatus(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdReenableDirectDrawObject(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdReleaseDC(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxDdRenderMoComp(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdResetVisrgn(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdSetColorKey(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdSetExclusiveMode(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdSetGammaRamp(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdCreateSurfaceEx(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdSetOverlayPosition(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdUnattachSurface(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdUnlock(DWORD x1, DWORD x2)
{
	/* NOTE : DxDdUnlockD3D and DxDdUnlock are same */
	stub;
}

NTSTATUS
NTAPI
DxDdUpdateOverlay(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdWaitForVerticalBlank(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpCanCreateVideoPort(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpColorControl(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpCreateVideoPort(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpDestroyVideoPort(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpFlipVideoPort(DWORD x1, DWORD x2, DWORD x3, DWORD x4)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpGetVideoPortBandwidth(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpGetVideoPortField(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpGetVideoPortFlipStatus(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpGetVideoPortInputFormats(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpGetVideoPortLine(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpGetVideoPortOutputFormats(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpGetVideoPortConnectInfo(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpGetVideoSignalStatus(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpUpdateVideoPort(DWORD x1, DWORD x2, DWORD x3, DWORD x4)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpWaitForVideoPortSync(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpAcquireNotification(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDvpReleaseNotification(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdHeapVidMemAllocAligned(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5)
{
	stub;
}

NTSTATUS
NTAPI
DxDdHeapVidMemFree(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdDisableDirectDraw(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdSuspendDirectDraw(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdResumeDirectDraw(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdDynamicModeChange(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdCloseProcess(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetDirectDrawBound(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdEnableDirectDrawRedirection(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdAllocPrivateUserMem(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdFreePrivateUserMem(DWORD x1, DWORD x2)
{
	stub;
}

NTSTATUS
NTAPI
DxDdSetAccelLevel(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdGetSurfaceLock(DWORD x1)
{
	stub;
}

NTSTATUS
NTAPI
DxDdEnumLockedSurfaceRect(DWORD x1, DWORD x2, DWORD x3)
{
	stub;
}

NTSTATUS
NTAPI
DxDdEnableDirectDraw(PVOID pDev, BOOLEAN Enable)
{

	/* NOTE not finsh yet 
    PGD_DXENGGETHDEVDATA pfDxEngGetHdevData;
    PEDD_DIRECTDRAW_GLOBAL pEDDgpl;

    pfDxEngGetHdevData  = (PGD_DXENGGETHDEVDATA)gpEngFuncs[DXENG_INDEX_DxEngGetHdevData].pfn;

    if ( pfDxEngGetHdevData(pDev, DxEGShDevData_display) )
    {
        pEDDgpl = (PEDD_DIRECTDRAW_GLOBAL) pfDxEngGetHdevData(pDev,DxEGShDevData_eddg);
        pEDDgpl->hDev = (HDEV) pDev;
        pEDDgpl->dhpdev = (PVOID) pfDxEngGetHdevData(pDev, DxEGShDevData_dhpdev);
    }

    return 1;
	*/
	stub;
}


