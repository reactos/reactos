/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/ddraw.c
 * PROGRAMER:        Peter Bajusz (hyp-x@stormregion.com)
 * REVISION HISTORY:
 *       25-10-2003  PB  Created
 */
/* FIXME: FOR THE LOVE OF GOD SOMEONE PLEASE FIX THIS FUCKFEST */
#include <ddk/ntddk.h>
#include <win32k/ntddraw.h>
#include <ddk/winddi.h>
#include <w32k.h>
#include <win32k/win32k.h>
#include <include/intddraw.h>
#include <win32k/gdiobj.h>

#define NDEBUG
#include <debug.h>

/************************************************************************/
/* DIRECT DRAW OBJECT                                                   */
/************************************************************************/

BOOL FASTCALL
DD_Cleanup(PDD_DIRECTDRAW pDD)
{
	pDD->DrvDisableDirectDraw(pDD->Global.dhpdev);
	return TRUE;
}

HANDLE STDCALL NtGdiDdCreateDirectDrawObject(
    HDC hdc
)
{
	DD_CALLBACKS callbacks;
	DD_SURFACECALLBACKS surface_callbacks;
	DD_PALETTECALLBACKS palette_callbacks;

	RtlZeroMemory(&callbacks, sizeof(callbacks));
	callbacks.dwSize = sizeof(callbacks);
	RtlZeroMemory(&surface_callbacks, sizeof(surface_callbacks));
	surface_callbacks.dwSize = sizeof(surface_callbacks);
	RtlZeroMemory(&palette_callbacks, sizeof(palette_callbacks));
	palette_callbacks.dwSize = sizeof(palette_callbacks);

	DC *pDC = DC_LockDc(hdc);
	if (!pDC)
		return NULL;

	if (!pDC->DriverFunctions.EnableDirectDraw)
	{
		// Driver doesn't support DirectDraw
		DC_UnlockDc(pDC);
		return NULL;
	}

	BOOL success = pDC->DriverFunctions.EnableDirectDraw(
		pDC->PDev, &callbacks, &surface_callbacks, &palette_callbacks);

	if (!success)
	{
		// DirectDraw creation failed
		DC_UnlockDc(pDC);
		return NULL;
	}

	HANDLE hDirectDraw = GDIOBJ_AllocObj(GDI_OBJECT_TYPE_DIRECTDRAW);
	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDraw, GDI_OBJECT_TYPE_DIRECTDRAW);

	pDirectDraw->Global.dhpdev = pDC->PDev;
	pDirectDraw->Local.lpGbl = &pDirectDraw->Global;

	pDirectDraw->DrvGetDirectDrawInfo = pDC->DriverFunctions.GetDirectDrawInfo;
	pDirectDraw->DrvDisableDirectDraw = pDC->DriverFunctions.DisableDirectDraw;

	if (callbacks.dwFlags & DDHAL_CB32_CREATESURFACE)
		pDirectDraw->DdCreateSurface = callbacks.CreateSurface;
	if (callbacks.dwFlags & DDHAL_CB32_SETCOLORKEY)
		pDirectDraw->DdDrvSetColorKey = callbacks.SetColorKey;
	if (callbacks.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK)
		pDirectDraw->DdWaitForVerticalBlank = callbacks.WaitForVerticalBlank;
	if (callbacks.dwFlags & DDHAL_CB32_CANCREATESURFACE)
		pDirectDraw->DdCanCreateSurface = callbacks.CanCreateSurface;
	if (callbacks.dwFlags & DDHAL_CB32_CREATEPALETTE)
		pDirectDraw->DdCreatePalette = callbacks.CreatePalette;
	if (callbacks.dwFlags & DDHAL_CB32_GETSCANLINE)
		pDirectDraw->DdGetScanLine = callbacks.GetScanLine;
	if (callbacks.dwFlags & DDHAL_CB32_MAPMEMORY)
		pDirectDraw->DdMapMemory = callbacks.MapMemory;

	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE)
		pDirectDraw->DdDestroySurface = surface_callbacks.DestroySurface;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_FLIP)
		pDirectDraw->DdFlip = surface_callbacks.Flip;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_SETCLIPLIST)
		pDirectDraw->DdSetClipList = surface_callbacks.SetClipList;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_LOCK)
		pDirectDraw->DdLock = surface_callbacks.Lock;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_UNLOCK)
		pDirectDraw->DdUnlock = surface_callbacks.Unlock;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_BLT)
		pDirectDraw->DdBlt = surface_callbacks.Blt;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_SETCOLORKEY)
		pDirectDraw->DdSetColorKey = surface_callbacks.SetColorKey;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE)
		pDirectDraw->DdAddAttachedSurface = surface_callbacks.AddAttachedSurface;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_GETBLTSTATUS)
		pDirectDraw->DdGetBltStatus = surface_callbacks.GetBltStatus;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_GETFLIPSTATUS)
		pDirectDraw->DdGetFlipStatus = surface_callbacks.GetFlipStatus;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY)
		pDirectDraw->DdUpdateOverlay = surface_callbacks.UpdateOverlay;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION)
		pDirectDraw->DdSetOverlayPosition = surface_callbacks.SetOverlayPosition;
	if (surface_callbacks.dwFlags & DDHAL_SURFCB32_SETPALETTE)
		pDirectDraw->DdSetPalette = surface_callbacks.SetPalette;

	if (palette_callbacks.dwFlags & DDHAL_PALCB32_DESTROYPALETTE)
		pDirectDraw->DdDestroyPalette = palette_callbacks.DestroyPalette;
	if (palette_callbacks.dwFlags & DDHAL_PALCB32_SETENTRIES)
		pDirectDraw->DdSetEntries = palette_callbacks.SetEntries;

	GDIOBJ_UnlockObjByPtr(pDirectDraw);
	DC_UnlockDc(pDC);

	return hDirectDraw;
}

BOOL STDCALL NtGdiDdDeleteDirectDrawObject(
    HANDLE hDirectDrawLocal
)
{
	return GDIOBJ_FreeObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
}

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
)
{
	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (!pDirectDraw)
		return FALSE;

	BOOL success = pDirectDraw->DrvGetDirectDrawInfo(
		pDirectDraw->Global.dhpdev,
		pHalInfo,
		puNumHeaps,
		puvmList,
		puNumFourCC,
		puFourCC);

	if (!success)
	{
		GDIOBJ_UnlockObjByPtr(pDirectDraw);
		return FALSE;
	}

	if (pHalInfo->lpD3DHALCallbacks)
	{
		RtlMoveMemory(puD3dCallbacks, pHalInfo->lpD3DHALCallbacks, sizeof(D3DNTHAL_CALLBACKS));
		pDirectDraw->D3dContextCreate = puD3dCallbacks->ContextCreate;
		pDirectDraw->D3dContextDestroy = puD3dCallbacks->ContextDestroy;
	}

	if (pHalInfo->lpD3DGlobalDriverData)
	{
		RtlMoveMemory(puD3dDriverData, pHalInfo->lpD3DGlobalDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA));
	}

	if (pHalInfo->lpD3DBufCallbacks)
	{
		RtlMoveMemory(puD3dBufferCallbacks, pHalInfo->lpD3DBufCallbacks, sizeof(DD_D3DBUFCALLBACKS));
		pDirectDraw->DdCanCreateD3DBuffer = puD3dBufferCallbacks->CanCreateD3DBuffer;
		pDirectDraw->DdCreateD3DBuffer = puD3dBufferCallbacks->CreateD3DBuffer;
		pDirectDraw->DdDestroyD3DBuffer = puD3dBufferCallbacks->DestroyD3DBuffer;
		pDirectDraw->DdLockD3DBuffer = puD3dBufferCallbacks->LockD3DBuffer;
		pDirectDraw->DdUnlockD3DBuffer = puD3dBufferCallbacks->UnlockD3DBuffer;
	}


	GDIOBJ_UnlockObjByPtr(pDirectDraw);

	return TRUE;
}

/************************************************************************/
/* SURFACE OBJECT                                                       */
/************************************************************************/

BOOL FASTCALL
DDSURF_Cleanup(PDD_SURFACE pDDSurf)
{
	//FIXME: implement
	return TRUE;
}

HANDLE STDCALL NtGdiDdCreateSurfaceObject(
    HANDLE hDirectDrawLocal,
    HANDLE hSurface,
    PDD_SURFACE_LOCAL puSurfaceLocal,
    PDD_SURFACE_MORE puSurfaceMore,
    PDD_SURFACE_GLOBAL puSurfaceGlobal,
    BOOL bComplete
)
{
	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (!pDirectDraw)
		return NULL;

	if (!hSurface)
		hSurface = GDIOBJ_AllocObj(GDI_OBJECT_TYPE_DD_SURFACE);

	PDD_SURFACE pSurface = GDIOBJ_LockObj(hSurface, GDI_OBJECT_TYPE_DD_SURFACE);
        /* FIXME - Handle pSurface == NULL!!!! */

	RtlMoveMemory(&pSurface->Local, puSurfaceLocal, sizeof(DD_SURFACE_LOCAL));
	RtlMoveMemory(&pSurface->More, puSurfaceMore, sizeof(DD_SURFACE_MORE));
	RtlMoveMemory(&pSurface->Global, puSurfaceGlobal, sizeof(DD_SURFACE_GLOBAL));
	pSurface->Local.lpGbl = &pSurface->Global;
	pSurface->Local.lpSurfMore = &pSurface->More;
	pSurface->Local.lpAttachList = NULL;
	pSurface->Local.lpAttachListFrom = NULL;
	pSurface->More.lpVideoPort = NULL;
	// FIXME: figure out how to use this
	pSurface->bComplete = bComplete;

	GDIOBJ_UnlockObjByPtr(pSurface);
	GDIOBJ_UnlockObjByPtr(pDirectDraw);

	return hSurface;
}

BOOL STDCALL NtGdiDdDeleteSurfaceObject(
    HANDLE hSurface
)
{
	return GDIOBJ_FreeObj(hSurface, GDI_OBJECT_TYPE_DD_SURFACE);
}

/*
BOOL STDCALL NtGdiDdAttachSurface(
    HANDLE hSurfaceFrom,
    HANDLE hSurfaceTo
)
{
	PDD_SURFACE pSurfaceFrom = GDIOBJ_LockObj(hSurfaceFrom, GDI_OBJECT_TYPE_DD_SURFACE);
	if (!pSurfaceFrom)
		return FALSE;
	PDD_SURFACE pSurfaceTo = GDIOBJ_LockObj(hSurfaceTo, GDI_OBJECT_TYPE_DD_SURFACE);
	if (!pSurfaceTo)
	{
		GDIOBJ_UnlockObjByPtr(pSurfaceFrom);
		return FALSE;
	}

	if (pSurfaceFrom->Local.lpAttachListFrom)
	{
		pSurfaceFrom->Local.lpAttachListFrom = pSurfaceFrom->AttachListFrom;
	}

	GDIOBJ_UnlockObjByPtr(pSurfaceFrom);
	GDIOBJ_UnlockObjByPtr(pSurfaceTo);
	return TRUE;
}
*/



DWORD STDCALL NtGdiDdGetDriverInfo(
    HANDLE hDirectDrawLocal,
    PDD_GETDRIVERINFODATA puGetDriverInfoData)

{
	DWORD  pdwNumHeaps;
	VIDEOMEMORY  *pvmList = NULL;
    DWORD  pdwNumFourCC;
    DWORD  *pdwFourCC = NULL;
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);


	 ddRVal = pDirectDraw->DrvGetDirectDrawInfo(
                 pDirectDraw->Global.dhpdev,(PDD_HALINFO) puGetDriverInfoData,
                 &pdwNumHeaps, pvmList, &pdwNumFourCC, pdwFourCC);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);

	return ddRVal;
}



DWORD STDCALL NtGdiDdWaitForVerticalBlank(
    HANDLE hDirectDrawLocal,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);

	puWaitForVerticalBlankData->lpDD = pDirectDraw->Local.lpGbl;

	ddRVal = pDirectDraw->DdWaitForVerticalBlank(puWaitForVerticalBlankData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);

	return ddRVal;
}


DWORD STDCALL NtGdiDdCanCreateSurface(
    HANDLE hDirectDrawLocal,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
)
{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);

	puCanCreateSurfaceData->lpDD = pDirectDraw->Local.lpGbl;

	ddRVal = pDirectDraw->DdCanCreateSurface(puCanCreateSurfaceData);

	GDIOBJ_UnlockObjByPtr(pDirectDraw);

	return ddRVal;
}



DWORD STDCALL NtGdiDdBlt(
    HANDLE hSurfaceDest,
    HANDLE hSurfaceSrc,
    PDD_BLTDATA puBltData
)
{
 DWORD  ddRVal;

 PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurfaceDest, GDI_OBJECT_TYPE_DIRECTDRAW);

 puBltData->lpDDDestSurface =  hSurfaceDest;
 puBltData->lpDDSrcSurface  =  hSurfaceSrc;
 puBltData->lpDD = pDirectDraw->Local.lpGbl;

 ddRVal = pDirectDraw->DdBlt(puBltData);

 GDIOBJ_UnlockObjByPtr(pDirectDraw);

 return ddRVal;
}





/* EOF */
