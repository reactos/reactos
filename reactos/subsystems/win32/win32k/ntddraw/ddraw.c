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

//#define NDEBUG
#include <debug.h>
GDIDEVICE IntGetPrimarySurface(VOID);

/* swtich this off to get rid of all dx debug msg */
#define DX_DEBUG

#define DdHandleTable GdiHandleTable




/************************************************************************/
/* DIRECT DRAW OBJECT                                                   */
/************************************************************************/

BOOL INTERNAL_CALL
DD_Cleanup(PVOID ObjectBody)
{
    PDD_DIRECTDRAW pDirectDraw = (PDD_DIRECTDRAW) ObjectBody;

    DPRINT1("DD_Cleanup\n");

    if (!pDirectDraw)
    {
        return FALSE;
    }

    if (pDirectDraw->Global.dhpdev == NULL)
    {
        return FALSE;
    }

    if (pDirectDraw->DrvDisableDirectDraw == NULL)
    {
        return FALSE;
    }

    pDirectDraw->DrvDisableDirectDraw(pDirectDraw->Global.dhpdev);
    return TRUE;
}


/* Documations how it works in windows and what we should do 
*
 *  HANDLE NtGdiDdCreateDirectDrawObject(HDC hdc)
 *
 *  Use CreateDCW(L”Display”,NULL,NULL,NULL); or some thing  else to create a DC.
.*
 *  A NULL DC are not accpect it  return 0 as error
 *
 *  How the interal works see msdn and ddk that is egunt inforamtions to figour or see reactos
 *  source code. ReactOS interal working diffent here what windows does. 
 */

HANDLE STDCALL 
NtGdiDdCreateDirectDrawObject(HDC hdc)
{
    DD_CALLBACKS callbacks;
    DD_SURFACECALLBACKS surface_callbacks;
    DD_PALETTECALLBACKS palette_callbacks;
    DC *pDC;
    BOOL success;
    HANDLE hDirectDraw;
    PDD_DIRECTDRAW pDirectDraw;

    DPRINT1("NtGdiDdCreateDirectDrawObject\n");

    RtlZeroMemory(&callbacks, sizeof(DD_CALLBACKS));
    callbacks.dwSize = sizeof(DD_CALLBACKS);
    RtlZeroMemory(&surface_callbacks, sizeof(DD_SURFACECALLBACKS));
    surface_callbacks.dwSize = sizeof(DD_SURFACECALLBACKS);
    RtlZeroMemory(&palette_callbacks, sizeof(DD_PALETTECALLBACKS));
    palette_callbacks.dwSize = sizeof(DD_PALETTECALLBACKS);

    /* Create a hdc if we do not have one */
    if (hdc == NULL)
    {
        HDC newHdc = IntGdiCreateDC(NULL,NULL,NULL,NULL,FALSE);
        hdc = newHdc;

        if (hdc == NULL)
        {
            return NULL;
        }
    }
    
    /* Look the hdc to gain the internal struct */

    pDC = DC_LockDc(hdc);
    if (!pDC)
    {
        /* We did fail look here so return NULL */
        return NULL;
    }

    if (pDC->DriverFunctions.EnableDirectDraw == NULL)
    {
        /* Driver doesn't support DirectDraw */

        /*
           Warring ReactOS complain that pDC are not right owner 
           when DC_UnlockDc(pDC) hit, why ? 
         */
        DC_UnlockDc(pDC);
        return NULL;
    }

   /* test see if driver support DirectDraw interface */
    success = pDC->DriverFunctions.EnableDirectDraw(
              pDC->PDev, &callbacks, &surface_callbacks, &palette_callbacks);

    if (!success)
    {
        /* DirectDraw creation failed */
        DPRINT1("DirectDraw creation failed\n"); 
        DC_UnlockDc(pDC);
        return NULL;
    }

    /* We found a DirectDraw interface 
     * Alloc a handler for it
     */
    hDirectDraw = GDIOBJ_AllocObj(DdHandleTable, GDI_OBJECT_TYPE_DIRECTDRAW);
    if (!hDirectDraw)
    {
        /* No more memmory */
        DC_UnlockDc(pDC);
        return NULL;
    }

    /* try look the DirectDraw handler and setup some data later */

    pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDraw, GDI_OBJECT_TYPE_DIRECTDRAW);
    if (!pDirectDraw)
    {
        /* invalid handle */
        DC_UnlockDc(pDC);
        return NULL;
    }

    /* 
     * We are caching all callbacks and some data
     * reason for it, is we do not trust on the kernel/usermode 
     * pointer for the callbacks comes back from user mode.
     * I perfer more safer way todo it, only safe way is 
     * to cache the callbacks pointer, and send back the true 
     * kernel pointer to user mode of the driver api we get back
     * Windows is sending back kernel pointer of the driver we 
     * are doing same, differnt is we also cache it. and only
     * use the cached one.
     */

    /* 
       Getting the PDev bad idea we need the hdc instead 
       if we are doing a dymatic resultions change the 
       Pdev will get lost, we should cache the HDC instead
       in windows, after a resultions change the HAL interface
       need be rebuild from scrach, thanks to this.small problem
       maybe we in ReactOS can found a solvtions on this later
     */

    pDirectDraw->Global.dhpdev = pDC->PDev;
    pDirectDraw->Local.lpGbl = &pDirectDraw->Global;

    pDirectDraw->DrvGetDirectDrawInfo = pDC->DriverFunctions.GetDirectDrawInfo;
    pDirectDraw->DrvDisableDirectDraw = pDC->DriverFunctions.DisableDirectDraw;
    pDirectDraw->EnableDirectDraw = pDC->DriverFunctions.EnableDirectDraw;


    /* DD_CALLBACKS setup */	
    RtlMoveMemory(&pDirectDraw->DD, &callbacks, sizeof(DD_CALLBACKS));

    /* DD_SURFACECALLBACKS  setup*/
    RtlMoveMemory(&pDirectDraw->Surf, &surface_callbacks, sizeof(DD_SURFACECALLBACKS));

    /* DD_PALETTECALLBACKS setup*/	
    RtlMoveMemory(&pDirectDraw->Pal, &palette_callbacks, sizeof(DD_PALETTECALLBACKS));

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    DC_UnlockDc(pDC);

    DPRINT1("DirectDraw return handler 0x%x\n",hDirectDraw); 
    return hDirectDraw;
}



BOOL STDCALL 
NtGdiDdDeleteDirectDrawObject( HANDLE hDirectDrawLocal)
{
    
    DPRINT1("NtGdiDdDeleteDirectDrawObject\n");
    return GDIOBJ_FreeObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
}

BOOL STDCALL 
NtGdiDdQueryDirectDrawObject(
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
)
{
    DD_HALINFO HalInfo;
    PDD_DIRECTDRAW pDirectDraw;
    BOOL success;


    DPRINT1("NtGdiDdQueryDirectDrawObject\n");

    /* Check for NULL pointer to prevent any one doing a mistake */
    if (hDirectDrawLocal == NULL)
    {
       return FALSE;
    }

    if (pHalInfo == NULL)
    {       
       return FALSE;
    }

    if ( pCallBackFlags == NULL)
    {
       return FALSE;
    }
    
    /* Look the DirectDraw interface */
    pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal,
                                 GDI_OBJECT_TYPE_DIRECTDRAW);

    if (!pDirectDraw)
    {
        /* Fail to Lock DirectDraw handle */
        return FALSE;
    }

    /* lock susseces 
     * rest the pHalInfo size we do not known
     * if we got a NT4 driver or windows 2000/XP/2003 driver yet
     */
    pHalInfo->dwSize = 0;

    /* Getting the request size of all struct at frist call
     * Secound call we getting data back 
     */

    success = pDirectDraw->DrvGetDirectDrawInfo( pDirectDraw->Global.dhpdev,
                                                 &HalInfo, 
                                                 puNumHeaps, puvmList,
                                                 puNumFourCC, puFourCC);

    if (!success)
    {
        /* fail we did not get any DrvGetDirectDrawInfo 
         * so we assume it is simple 2d DirectDraw interface 
         */
        DPRINT1("Driver does not fill the  Fail to get DirectDraw driver info \n");

        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        return FALSE;
    }

    if (HalInfo.dwSize == 0)
    {
        /* some driver does not fill it, they only implement the DrvGetDirectDrawInfo
         * so they work on Windows 2000/XP/2003
         */

        DPRINT1(" Fail for driver does not fill the DD_HALINFO struct \n");
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        return FALSE;
    }

    /* check see if DD_HALINFO is NT4 version or not
     * if it NT4 version we convert it to NT5 version
     */
    if (HalInfo.dwSize != sizeof(DD_HALINFO))
    {
        if (HalInfo.dwSize == sizeof(DD_HALINFO_V4))
        {
            /* NT4 Compatible */
            DPRINT1("Got DD_HALINFO_V4 sturct we convert it to DD_HALINFO \n");
            HalInfo.dwSize = sizeof(DD_HALINFO);
            HalInfo.lpD3DGlobalDriverData = NULL;
            HalInfo.lpD3DHALCallbacks = NULL;
            HalInfo.lpD3DBufCallbacks = NULL;
        }
        else
        {
            /* Unknown version found */
            DPRINT1(" Fail : did not get DD_HALINFO size \n");
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return FALSE;
        }
    }

    /* Copy it to user mode pointer the data */
    RtlMoveMemory(pHalInfo, &HalInfo, sizeof(DD_HALINFO));

    /* rest the flag so we do not need do it later */
    pCallBackFlags[0]=pDirectDraw->DD.dwFlags;
    pCallBackFlags[1]=pDirectDraw->Surf.dwFlags;
    pCallBackFlags[2]=pDirectDraw->Pal.dwFlags;

    DPRINT1("Found DirectDraw CallBack for 2D and 3D Hal\n");

    /* Copy it to the cache */
    RtlMoveMemory(&pDirectDraw->Hal, pHalInfo, sizeof(DD_HALINFO));

    if (pHalInfo->lpD3DGlobalDriverData)
    {
        /* 
             msdn say D3DHAL_GLOBALDRIVERDATA and D3DNTHAL_GLOBALDRIVERDATA 
             are not same but if u compare these in msdn it is exacly same 
         */
       DPRINT1("Found DirectDraw Global DriverData \n");
       RtlMoveMemory(puD3dDriverData, pHalInfo->lpD3DGlobalDriverData,
                     sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    }

    if (pHalInfo->lpD3DHALCallbacks )
    {
        DPRINT1("Found DirectDraw CallBack for 3D Hal\n");
        RtlMoveMemory(puD3dCallbacks, pHalInfo->lpD3DHALCallbacks,
                      sizeof( D3DNTHAL_CALLBACKS ) );
    }

    if (pHalInfo->lpD3DBufCallbacks)
    {
       DPRINT1("Found DirectDraw CallBack for 3D Hal Bufffer  \n");
       /* msdn DDHAL_D3DBUFCALLBACKS = DD_D3DBUFCALLBACKS */
       RtlMoveMemory(puD3dBufferCallbacks, pHalInfo->lpD3DBufCallbacks,
                     sizeof(DD_D3DBUFCALLBACKS));
    }

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return TRUE;
}


DWORD STDCALL NtGdiDdGetDriverInfo(
    HANDLE hDirectDrawLocal,
    PDD_GETDRIVERINFODATA puGetDriverInfoData)

{
    DWORD  ddRVal = 0;
    PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal,
                                                GDI_OBJECT_TYPE_DIRECTDRAW);

    DPRINT1("NtGdiDdGetDriverInfo\n");

    if (pDirectDraw == NULL) 
    {
        return DDHAL_DRIVER_NOTHANDLED;
    }

    /* it exsist two version of NtGdiDdGetDriverInfo we need check for both flags */
    if (!(pDirectDraw->Hal.dwFlags & DDHALINFO_GETDRIVERINFOSET))
         ddRVal++;

    if (!(pDirectDraw->Hal.dwFlags & DDHALINFO_GETDRIVERINFO2))
         ddRVal++;

    /* Now we are doing the call to drv DrvGetDriverInfo */
    if   (ddRVal == 2)
    {
        DPRINT1("NtGdiDdGetDriverInfo DDHAL_DRIVER_NOTHANDLED");         
        ddRVal = DDHAL_DRIVER_NOTHANDLED;
    }
    else
    {
        ddRVal = pDirectDraw->Hal.GetDriverInfo(puGetDriverInfoData);
    }

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

    return ddRVal;
}




DWORD STDCALL NtGdiDdUnlock(
    HANDLE hSurface,
    PDD_UNLOCKDATA puUnlockData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdUnlock\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puUnlockData->lpDD;

	/* use our cache version instead */
	puUnlockData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_UNLOCK))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Unlock(puUnlockData);

	/* But back the orignal PDev */
	puUnlockData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);    
	return ddRVal;
}

DWORD STDCALL NtGdiDdBlt(
    HANDLE hSurfaceDest,
    HANDLE hSurfaceSrc,
    PDD_BLTDATA puBltData
)
{
    DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

    PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurfaceDest, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
    DPRINT1("NtGdiDdBlt\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puBltData->lpDD;

	/* use our cache version instead */
	puBltData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_BLT))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.Blt(puBltData);

	/* But back the orignal PDev */
	puBltData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;
   }

DWORD STDCALL NtGdiDdSetColorKey(
    HANDLE hSurface,
    PDD_SETCOLORKEYDATA puSetColorKeyData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdSetColorKey\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puSetColorKeyData->lpDD;

	/* use our cache version instead */
	puSetColorKeyData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_SETCOLORKEY))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.SetColorKey(puSetColorKeyData);

	/* But back the orignal PDev */
	puSetColorKeyData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;	
}


DWORD STDCALL NtGdiDdAddAttachedSurface(
    HANDLE hSurface,
    HANDLE hSurfaceAttached,
    PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurfaceAttached, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdAddAttachedSurface\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puAddAttachedSurfaceData->lpDD;

	/* use our cache version instead */
	puAddAttachedSurfaceData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.AddAttachedSurface(puAddAttachedSurfaceData);

	/* But back the orignal PDev */
	puAddAttachedSurfaceData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;	
}

DWORD STDCALL NtGdiDdGetBltStatus(
    HANDLE hSurface,
    PDD_GETBLTSTATUSDATA puGetBltStatusData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdGetBltStatus\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puGetBltStatusData->lpDD;

	/* use our cache version instead */
	puGetBltStatusData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_GETBLTSTATUS))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.GetBltStatus(puGetBltStatusData);

	/* But back the orignal PDev */
	puGetBltStatusData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdGetFlipStatus(
    HANDLE hSurface,
    PDD_GETFLIPSTATUSDATA puGetFlipStatusData
)
{
    DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdGetFlipStatus\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puGetFlipStatusData->lpDD;

	/* use our cache version instead */
	puGetFlipStatusData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_GETFLIPSTATUS))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.GetFlipStatus(puGetFlipStatusData);

	/* But back the orignal PDev */
	puGetFlipStatusData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;		
}

DWORD STDCALL NtGdiDdUpdateOverlay(
    HANDLE hSurfaceDestination,
    HANDLE hSurfaceSource,
    PDD_UPDATEOVERLAYDATA puUpdateOverlayData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

    PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurfaceDestination, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
    DPRINT1("NtGdiDdUpdateOverlay\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puUpdateOverlayData->lpDD;

	/* use our cache version instead */
	puUpdateOverlayData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.UpdateOverlay(puUpdateOverlayData);

	/* But back the orignal PDev */
	puUpdateOverlayData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    return ddRVal;
}

DWORD STDCALL NtGdiDdSetOverlayPosition(
    HANDLE hSurfaceSource,
    HANDLE hSurfaceDestination,
    PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

    PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hSurfaceDestination, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
    DPRINT1("NtGdiDdSetOverlayPosition\n");
#endif
	if (pDirectDraw == NULL) 
		return DDHAL_DRIVER_NOTHANDLED;

	/* backup the orignal PDev and info */
	lgpl = puSetOverlayPositionData->lpDD;

	/* use our cache version instead */
	puSetOverlayPositionData->lpDD = &pDirectDraw->Global;

	/* make the call */
	if (!(pDirectDraw->Surf.dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION))
		ddRVal = DDHAL_DRIVER_NOTHANDLED;
	else
        ddRVal = pDirectDraw->Surf.SetOverlayPosition(puSetOverlayPositionData);

	/* But back the orignal PDev */
	puSetOverlayPositionData->lpDD = lgpl;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
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
#ifdef DX_DEBUG
    DPRINT1("DDSURF_Cleanup\n");
#endif
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
	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
    PDD_SURFACE pSurface;
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdCreateSurfaceObject\n");
#endif
	if (!pDirectDraw)
		return NULL;

	if (!hSurface)
		hSurface = GDIOBJ_AllocObj(DdHandleTable, GDI_OBJECT_TYPE_DD_SURFACE);

	pSurface = GDIOBJ_LockObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DD_SURFACE);
	
	if (!pSurface)
	{
		GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
		return NULL;
	}

	pSurface->hDirectDrawLocal = hDirectDrawLocal;

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

	GDIOBJ_UnlockObjByPtr(DdHandleTable, pSurface);
	GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

	return hSurface;
}

BOOL STDCALL NtGdiDdDeleteSurfaceObject(
    HANDLE hSurface
)
{
#ifdef DX_DEBUG
    DPRINT1("NtGdiDdDeleteSurfaceObject\n");
#endif
    /* FIXME add right GDI_OBJECT_TYPE_ for everthing for now 
       we are using same type */
	/* return GDIOBJ_FreeObj(hSurface, GDI_OBJECT_TYPE_DD_SURFACE); */
	return GDIOBJ_FreeObj(DdHandleTable, hSurface, GDI_OBJECT_TYPE_DD_SURFACE);
	
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
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
#ifdef DX_DEBUG
	DPRINT1("NtGdiDdGetAvailDriverMemory\n");
#endif

	/* backup the orignal PDev and info */
	lgpl = puGetAvailDriverMemoryData->lpDD;

	/* use our cache version instead */
	puGetAvailDriverMemoryData->lpDD = &pDirectDraw->Global;

	/* make the call */
   // ddRVal = pDirectDraw->DdGetAvailDriverMemory(puGetAvailDriverMemoryData); 
 
	GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);


	/* But back the orignal PDev */
	puGetAvailDriverMemoryData->lpDD = lgpl;

	return ddRVal;
}




DWORD STDCALL NtGdiDdSetExclusiveMode(
    HANDLE hDirectDraw,
    PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData
)
{
	DWORD  ddRVal;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDraw, GDI_OBJECT_TYPE_DIRECTDRAW);

#ifdef DX_DEBUG
	DPRINT1("NtGdiDdSetExclusiveMode\n");
#endif

	/* backup the orignal PDev and info */
	lgpl = puSetExclusiveModeData->lpDD;

	/* use our cache version instead */
	puSetExclusiveModeData->lpDD = &pDirectDraw->Global;

	/* make the call */
    ddRVal = pDirectDraw->DdSetExclusiveMode(puSetExclusiveModeData);

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

	/* But back the orignal PDev */
	puSetExclusiveModeData->lpDD = lgpl;
    
	return ddRVal;	
}




/* EOF */
