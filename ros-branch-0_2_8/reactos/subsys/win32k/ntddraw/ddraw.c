/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/ddraw.c
 * PROGRAMER:        Peter Bajusz (hyp-x@stormregion.com)
 * REVISION HISTORY:
 *       25-10-2003  PB  Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/************************************************************************/
/* DIRECT DRAW OBJECT                                                   */
/************************************************************************/

BOOL INTERNAL_CALL
DD_Cleanup(PVOID ObjectBody)
{
	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(ObjectBody, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (!pDirectDraw)
		return FALSE;

	pDirectDraw->DrvDisableDirectDraw(pDirectDraw->Global.dhpdev);

	GDIOBJ_UnlockObjByPtr(pDirectDraw);
	return TRUE;
}

HANDLE STDCALL NtGdiDdCreateDirectDrawObject(
    HDC hdc
)
{
	DD_CALLBACKS callbacks;
	DD_SURFACECALLBACKS surface_callbacks;
	DD_PALETTECALLBACKS palette_callbacks;

	RtlZeroMemory(&callbacks, sizeof(DD_CALLBACKS));
	callbacks.dwSize = sizeof(DD_CALLBACKS);
	RtlZeroMemory(&surface_callbacks, sizeof(DD_SURFACECALLBACKS));
	surface_callbacks.dwSize = sizeof(DD_SURFACECALLBACKS);
	RtlZeroMemory(&palette_callbacks, sizeof(DD_PALETTECALLBACKS));
	palette_callbacks.dwSize = sizeof(DD_PALETTECALLBACKS);

	
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
	if (!hDirectDraw)
	{
		/* No more memmory */
		DC_UnlockDc(pDC);
		return NULL;
	}

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDraw, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (!pDirectDraw)
	{
		/* invalid handle */
		DC_UnlockDc(pDC);
		return NULL;
	}
	

	pDirectDraw->Global.dhpdev = pDC->PDev;
	pDirectDraw->Local.lpGbl = &pDirectDraw->Global;

	pDirectDraw->DrvGetDirectDrawInfo = pDC->DriverFunctions.GetDirectDrawInfo;
	pDirectDraw->DrvDisableDirectDraw = pDC->DriverFunctions.DisableDirectDraw;

    /* DD_CALLBACKS setup */	
	RtlMoveMemory(&pDirectDraw->DD, &callbacks, sizeof(DD_CALLBACKS));
	
   	/* DD_SURFACECALLBACKS  setup*/	
	RtlMoveMemory(&pDirectDraw->Surf, &surface_callbacks, sizeof(DD_SURFACECALLBACKS));

	/* DD_PALETTECALLBACKS setup*/	
	RtlMoveMemory(&pDirectDraw->Pal, &surface_callbacks, sizeof(DD_PALETTECALLBACKS));
	
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

	if (pHalInfo)
	{
       RtlMoveMemory(&pDirectDraw->Hal, pHalInfo, sizeof(DD_HALINFO));

	   if (pHalInfo->lpD3DHALCallbacks)
	   {
		 RtlMoveMemory(puD3dCallbacks, pHalInfo->lpD3DHALCallbacks, sizeof(D3DNTHAL_CALLBACKS));		
	   }

	   if (pHalInfo->lpD3DGlobalDriverData)
	   {
		 RtlMoveMemory(puD3dDriverData, pHalInfo->lpD3DGlobalDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA));
	   }
	   if (pHalInfo->lpD3DBufCallbacks)
	   {
		 RtlMoveMemory(puD3dBufferCallbacks, pHalInfo->lpD3DBufCallbacks, sizeof(DD_D3DBUFCALLBACKS));
	   }
       	   	
	 }
        
	GDIOBJ_UnlockObjByPtr(pDirectDraw);

	return TRUE;
}


DWORD STDCALL NtGdiDdGetDriverInfo(
    HANDLE hDirectDrawLocal,
    PDD_GETDRIVERINFODATA puGetDriverInfoData)

{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if   (!(pDirectDraw->Hal.dwFlags & DDHALINFO_GETDRIVERINFOSET))
	     ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
	     ddRVal = pDirectDraw->Hal.GetDriverInfo(puGetDriverInfoData);
   
    GDIOBJ_UnlockObjByPtr(pDirectDraw);

	return ddRVal;
}

/************************************************************************/
/* DD CALLBACKS                                                         */
/* FIXME NtGdiDdCreateSurface we do not call to ddCreateSurface         */
/************************************************************************/

DWORD STDCALL NtGdiDdCreateSurface(
    HANDLE hDirectDrawLocal,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
)
{
	DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
	

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->DD.dwFlags & DDHAL_CB32_CANCREATESURFACE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
	{	  
	   ddRVal = pDirectDraw->DD.CreateSurface(puCreateSurfaceData);	 
	}

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
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->DD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else	
  	    ddRVal = pDirectDraw->DD.WaitForVerticalBlank(puWaitForVerticalBlankData);

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
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->DD.dwFlags & DDHAL_CB32_CANCREATESURFACE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else	
	    ddRVal = pDirectDraw->DD.CanCreateSurface(puCanCreateSurfaceData);

	GDIOBJ_UnlockObjByPtr(pDirectDraw);
	return ddRVal;
}

DWORD STDCALL NtGdiDdGetScanLine(
    HANDLE hDirectDrawLocal,
    PDD_GETSCANLINEDATA puGetScanLineData
)
{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->DD.dwFlags & DDHAL_CB32_GETSCANLINE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else	
	    ddRVal = pDirectDraw->DD.GetScanLine(puGetScanLineData);

	GDIOBJ_UnlockObjByPtr(pDirectDraw);
	return ddRVal;
}



/************************************************************************/
/* Surface CALLBACKS                                                    */
/* FIXME                                                                */
/* NtGdiDdDestroySurface                                                */ 
/************************************************************************/

DWORD STDCALL NtGdiDdDestroySurface(
    HANDLE hSurface,
    BOOL bRealDestroy
)
{	
	DWORD  ddRVal  = DDHAL_DRIVER_NOTHANDLED;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else	
	{
		DD_DESTROYSURFACEDATA DestroySurf; 
	
		/* FIXME 
		 * bRealDestroy 
		 * are we doing right ??
		 */
        DestroySurf.lpDD =  &pDirectDraw->Global;

        DestroySurf.lpDDSurface = hSurface;  // ?
        DestroySurf.DestroySurface = pDirectDraw->Surf.DestroySurface;		
		
        ddRVal = pDirectDraw->Surf.DestroySurface(&DestroySurf); 
	}

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;			
}

DWORD STDCALL NtGdiDdFlip(
    HANDLE hSurfaceCurrent,
    HANDLE hSurfaceTarget,
    HANDLE hSurfaceCurrentLeft,
    HANDLE hSurfaceTargetLeft,
    PDD_FLIPDATA puFlipData
)
{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurfaceTarget, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_FLIP))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Flip(puFlipData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdLock(
    HANDLE hSurface,
    PDD_LOCKDATA puLockData,
    HDC hdcClip
)
{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_LOCK))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Lock(puLockData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdUnlock(
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
)
{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_UNLOCK))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Unlock(puUnlockData);

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
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_BLT))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Blt(puBltData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;
   }

DWORD STDCALL NtGdiDdSetColorKey(
    HANDLE hSurface,
    PDD_SETCOLORKEYDATA puSetColorKeyData
)
{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_SETCOLORKEY))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.SetColorKey(puSetColorKeyData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;	
}


DWORD STDCALL NtGdiDdAddAttachedSurface(
    HANDLE hSurface,
    HANDLE hSurfaceAttached,
    PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
)
{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurfaceAttached, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.AddAttachedSurface(puAddAttachedSurfaceData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;	
}

DWORD STDCALL NtGdiDdGetBltStatus(
    HANDLE hSurface,
    PDD_GETBLTSTATUSDATA puGetBltStatusData
)
{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_GETBLTSTATUS))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.GetBltStatus(puGetBltStatusData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdGetFlipStatus(
    HANDLE hSurface,
    PDD_GETFLIPSTATUSDATA puGetFlipStatusData
)
{
    DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_GETFLIPSTATUS))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.GetFlipStatus(puGetFlipStatusData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdUpdateOverlay(
    HANDLE hSurfaceDestination,
    HANDLE hSurfaceSource,
    PDD_UPDATEOVERLAYDATA puUpdateOverlayData
)
{
	DWORD  ddRVal;

    PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurfaceDestination, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.UpdateOverlay(puUpdateOverlayData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;
}

DWORD STDCALL NtGdiDdSetOverlayPosition(
    HANDLE hSurfaceSource,
    HANDLE hSurfaceDestination,
    PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
)
{
	DWORD  ddRVal;

    PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hSurfaceDestination, GDI_OBJECT_TYPE_DIRECTDRAW);
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.SetOverlayPosition(puSetOverlayPositionData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    return ddRVal;
}


/************************************************************************/
/* SURFACE OBJECT                                                       */
/************************************************************************/

BOOL INTERNAL_CALL
DDSURF_Cleanup(PVOID pDDSurf)
{
	/* FIXME: implement 
	 * PDD_SURFACE pDDSurf = PVOID pDDSurf
	 */
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



/************************************************************************/
/* DIRECT DRAW SURFACR END                                                   */
/************************************************************************/


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

DWORD STDCALL NtGdiDdGetAvailDriverMemory(
    HANDLE hDirectDrawLocal,
    PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData
)
{
	DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);

   // ddRVal = pDirectDraw->DdGetAvailDriverMemory(puGetAvailDriverMemoryData); 
 
	GDIOBJ_UnlockObjByPtr(pDirectDraw);

	return ddRVal;
}




DWORD STDCALL NtGdiDdSetExclusiveMode(
    HANDLE hDirectDraw,
    PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
)
{
	DWORD  ddRVal;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(hDirectDraw, GDI_OBJECT_TYPE_DIRECTDRAW);

    ddRVal = pDirectDraw->DdSetExclusiveMode(puSetExclusiveModeData);

    GDIOBJ_UnlockObjByPtr(pDirectDraw);
    
	return ddRVal;	
}




/* EOF */
