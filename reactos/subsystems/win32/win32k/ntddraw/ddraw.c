/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/ddraw.c
 * PROGRAMER:        Peter Bajusz (hyp-x@stormregion.com)
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       25-10-2003  PB  Created
         from 2003 to year 2007
 *       rewrote almost all code Peter did.
 *       only few line are left from him
 */

#include <w32k.h>

//#define NDEBUG
#include <debug.h>

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

/* code for enable and reanble the drv */
BOOL
intEnableDriver(PDD_DIRECTDRAW pDirectDraw)
{
     BOOL success;
     DD_HALINFO HalInfo;

    /*clean up some of the cache entry */
    RtlZeroMemory(&pDirectDraw->DD,   sizeof(DD_CALLBACKS));
    RtlZeroMemory(&pDirectDraw->Surf, sizeof(DD_SURFACECALLBACKS));
    RtlZeroMemory(&pDirectDraw->Pal,  sizeof(DD_PALETTECALLBACKS));
    RtlZeroMemory(&pDirectDraw->Hal,  sizeof(DD_HALINFO));
    RtlZeroMemory(&HalInfo,  sizeof(DD_HALINFO));
    pDirectDraw->dwNumHeaps =0;
    pDirectDraw->dwNumFourCC = 0;
    pDirectDraw->pdwFourCC = NULL;
    pDirectDraw->pvmList = NULL;

    /* Get DirectDraw infomations form the driver
     * DDK say pvmList, pdwFourCC is always NULL in frist call here
     * but we get back how many pvmList it whant we should alloc, same
     * with pdwFourCC.
     */
    if (pDirectDraw->DrvGetDirectDrawInfo)
    {
        success = pDirectDraw->DrvGetDirectDrawInfo( pDirectDraw->Global.dhpdev,
                                                     &HalInfo,
                                                     &pDirectDraw->dwNumHeaps,
                                                     NULL,
                                                     &pDirectDraw->dwNumFourCC,
                                                     NULL);
        if (!success)
        {
            DPRINT1("DrvGetDirectDrawInfo  frist call fail\n");
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return FALSE;
        }


        /* The driver are not respnose to alloc the memory for pvmList
         * but it is win32k responsible todo, Windows 9x it is gdi32.dll
         */
        if (pDirectDraw->dwNumHeaps != 0)
        {
            DPRINT1("Setup pvmList\n");
            pDirectDraw->pvmList = (PVIDEOMEMORY) ExAllocatePoolWithTag(PagedPool, pDirectDraw->dwNumHeaps * sizeof(VIDEOMEMORY), TAG_DXPVMLIST);
            if (pDirectDraw->pvmList == NULL)
            {
                DPRINT1("pvmList memmery alloc fail\n");
                return FALSE;
            }
        }

        /* The driver are not respnose to alloc the memory for pdwFourCC
         * but it is win32k responsible todo, Windows 9x it is gdi32.dll
         */

        if (pDirectDraw->dwNumFourCC != 0)
        {
            DPRINT1("Setup pdwFourCC\n");
            pDirectDraw->pdwFourCC = (LPDWORD) ExAllocatePoolWithTag(PagedPool, pDirectDraw->dwNumFourCC * sizeof(DWORD), TAG_DXFOURCC);

            if (pDirectDraw->pdwFourCC == NULL)
            {
                DPRINT1("pdwFourCC memmery alloc fail\n");
                return FALSE;
            }
        }
        success = pDirectDraw->DrvGetDirectDrawInfo( pDirectDraw->Global.dhpdev,
                                                     &HalInfo,
                                                     &pDirectDraw->dwNumHeaps,
                                                     pDirectDraw->pvmList,
                                                     &pDirectDraw->dwNumFourCC,
                                                     pDirectDraw->pdwFourCC);
        if (!success)
        {
            DPRINT1("DrvGetDirectDrawInfo  second call fail\n");
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return FALSE;
        }


        /* We need now convert the DD_HALINFO we got, it can be NT4 driver we
         * loading ReactOS supporting NT4 and higher to be loading.so we make
         * the HALInfo compatible here so we can easy pass it to gdi32.dll
         * without converting it later
        */

        if ((HalInfo.dwSize != sizeof(DD_HALINFO)) &&
            (HalInfo.dwSize != sizeof(DD_HALINFO_V4)))
        {
            DPRINT1(" Fail not vaild driver DD_HALINFO struct found\n");
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return FALSE;
        }

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
        RtlCopyMemory(&pDirectDraw->Hal, &HalInfo, sizeof(DD_HALINFO));
    }

    DPRINT1("Trying EnableDirectDraw the driver\n");

    success = pDirectDraw->EnableDirectDraw( pDirectDraw->Global.dhpdev,
                                             &pDirectDraw->DD,
                                             &pDirectDraw->Surf,
                                             &pDirectDraw->Pal);

    if (!success)
    {
        DPRINT1("EnableDirectDraw call fail\n");
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        return FALSE;
    }

    return TRUE;
}


/* NtGdiDdCreateDirectDrawObject is finish and works as it should
 * it maybe have some memory leack or handler leack in this code
 * if you found any case you maybe think it leacks the handler
 * or memory please tell me, before you start fixing the code
 * Magnus Olsen
 */
HANDLE STDCALL
NtGdiDdCreateDirectDrawObject(HDC hdc)
{
    DC *pDC;
    HANDLE hDirectDraw;
    PDD_DIRECTDRAW pDirectDraw;

    /* Create a hdc if we do not have one */
    if (hdc == NULL)
    {
       return NULL;
    }

    /* Look the hdc to gain the internal struct */
    pDC = DC_LockDc(hdc);
    if (!pDC)
    {
        return NULL;
    }

    /* test see if drv got a dx interface or not */
    if  ( ( pDC->DriverFunctions.DisableDirectDraw == NULL) ||
          ( pDC->DriverFunctions.EnableDirectDraw == NULL))
    {
        DC_UnlockDc(pDC);
        return NULL;
    }

    /* alloc and lock  the stucrt */
    hDirectDraw = GDIOBJ_AllocObj(DdHandleTable, GDI_OBJECT_TYPE_DIRECTDRAW);
    if (!hDirectDraw)
    {
        /* No more memmory */
        DC_UnlockDc(pDC);
        return NULL;
    }

    pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDraw, GDI_OBJECT_TYPE_DIRECTDRAW);
    if (!pDirectDraw)
    {
        /* invalid handle */
        DC_UnlockDc(pDC);
        return NULL;
    }

    /* setup the internal stuff */
    pDirectDraw->Global.dhpdev = pDC->PDev;
    pDirectDraw->Local.lpGbl = &pDirectDraw->Global;

    pDirectDraw->DrvGetDirectDrawInfo = pDC->DriverFunctions.GetDirectDrawInfo;
    pDirectDraw->DrvDisableDirectDraw = pDC->DriverFunctions.DisableDirectDraw;
    pDirectDraw->EnableDirectDraw = pDC->DriverFunctions.EnableDirectDraw;

    if (intEnableDriver(pDirectDraw) == FALSE)
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        DC_UnlockDc(pDC);
        return NULL;
    }

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    DC_UnlockDc(pDC);
    return hDirectDraw;
}


/* NtGdiDdCreateDirectDrawObject is finish and works as it should
 * it maybe have some memory leack or handler leack in this code
 * if you found any case you maybe think it leacks the handler
 * or memory please tell me, before you start fixing the code
 * Magnus Olsen
 */

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
    PDD_DIRECTDRAW pDirectDraw;
    NTSTATUS Status = FALSE;
    BOOL Ret=FALSE;
    LPD3DNTHAL_GLOBALDRIVERDATA pD3dDriverData;

    if (hDirectDrawLocal == NULL)
    {
       return FALSE;
    }

    pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal,
                                 GDI_OBJECT_TYPE_DIRECTDRAW);
    if (!pDirectDraw)
    {
        return FALSE;
    }

    /*
     * Get pHalInfo
     */
    if (pHalInfo != NULL)
    {
        _SEH_TRY
        {
            ProbeForWrite(pHalInfo,  sizeof(DD_HALINFO), 1);
            RtlCopyMemory(pHalInfo,&pDirectDraw->Hal, sizeof(DD_HALINFO));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return FALSE;
        }
    }
    else
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        return FALSE;
    }

    /*
     * Get pCallBackFlags
     */
    if (pCallBackFlags != NULL)
    {
        _SEH_TRY
        {
            ProbeForWrite(pCallBackFlags,  sizeof(DWORD)*3, 1);
            pCallBackFlags[0]=pDirectDraw->DD.dwFlags;
            pCallBackFlags[1]=pDirectDraw->Surf.dwFlags;
            pCallBackFlags[2]=pDirectDraw->Pal.dwFlags;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return FALSE;
        }
    }
    else
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        return FALSE;
    }

    /*
     * Get puD3dCallbacks
     */
    if ((puD3dCallbacks) &&
        (pDirectDraw->Hal.lpD3DHALCallbacks))
    {
        _SEH_TRY
        {
            ProbeForWrite(puD3dCallbacks,  sizeof(D3DNTHAL_CALLBACKS), 1);
            RtlCopyMemory( puD3dCallbacks, pDirectDraw->Hal.lpD3DHALCallbacks, sizeof( D3DNTHAL_CALLBACKS ) );
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return FALSE;
        }
    }
    else
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        return FALSE;
    }

    /*
     * Get lpD3DGlobalDriverData
     */
    if ((puD3dDriverData) &&
        (pDirectDraw->Hal.lpD3DGlobalDriverData != NULL))
    {
        /* Get D3dDriverData */
        _SEH_TRY
        {
            ProbeForWrite(puD3dDriverData,  sizeof(D3DNTHAL_GLOBALDRIVERDATA), 1);
            RtlCopyMemory( puD3dDriverData, pDirectDraw->Hal.lpD3DGlobalDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return FALSE;
        }

        /* Get TextureFormats */
        pD3dDriverData =pDirectDraw->Hal.lpD3DGlobalDriverData;
        if ((puD3dTextureFormats) &&
            (pD3dDriverData->dwNumTextureFormats>0) &&
            (pD3dDriverData->lpTextureFormats))
        {
            DWORD Size = sizeof(DDSURFACEDESC) * pD3dDriverData->dwNumTextureFormats;
            _SEH_TRY
            {
                ProbeForWrite(puD3dTextureFormats, Size, 1);
                RtlCopyMemory( puD3dTextureFormats, pD3dDriverData->lpTextureFormats, Size);
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            if(!NT_SUCCESS(Status))
            {
                SetLastNtError(Status);
                GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
                return FALSE;
            }
        }
    }
    else
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        return FALSE;
    }

    /*Get D3dBufferCallbacks */
    if ( (puD3dBufferCallbacks) &&
         (pDirectDraw->Hal.lpD3DBufCallbacks))
    {
        _SEH_TRY
        {
            ProbeForWrite(puD3dBufferCallbacks,  sizeof(DD_D3DBUFCALLBACKS), 1);
            RtlCopyMemory( puD3dBufferCallbacks, pDirectDraw->Hal.lpD3DBufCallbacks, sizeof(DD_D3DBUFCALLBACKS));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            return FALSE;
        }
    }
    else
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        return FALSE;
    }

    /* Get puNumFourCC and dwNumFourCC */
    _SEH_TRY
    {
        ProbeForWrite(puNumFourCC, sizeof(DWORD), 1);
        *puNumFourCC = pDirectDraw->dwNumFourCC;

        if ((pDirectDraw->pdwFourCC) &&
            (puFourCC))
        {
            ProbeForWrite(puFourCC, sizeof(DWORD) * pDirectDraw->dwNumFourCC, 1);
            RtlCopyMemory( puFourCC, pDirectDraw->pdwFourCC, sizeof(DWORD) * pDirectDraw->dwNumFourCC);
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
    }
    else
    {
        Ret = TRUE;
    }
    return Ret;
}


BOOL STDCALL
NtGdiDdDeleteDirectDrawObject( HANDLE hDirectDrawLocal)
{
    DPRINT1("NtGdiDdDeleteDirectDrawObject\n");
    if (hDirectDrawLocal == NULL)
    {
        return FALSE;
    }

    return GDIOBJ_FreeObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
}


BOOL STDCALL NtGdiDdReenableDirectDrawObject(
    HANDLE hDirectDrawLocal,
    BOOL *pubNewMode
)
{
    BOOL Ret=FALSE;
    NTSTATUS Status = FALSE;
    PDD_DIRECTDRAW pDirectDraw;

    if (hDirectDrawLocal == NULL)
    {
       return Ret;
    }

    pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal,
                                 GDI_OBJECT_TYPE_DIRECTDRAW);

    if (!pDirectDraw)
    {
        return Ret;
    }

    /*
     * FIXME detect mode change thic code maybe are not correct
     * if we call on intEnableDriver it will cause some memory leak
     * we need free the alloc memory before we call on it
     */
    Ret = intEnableDriver(pDirectDraw);

    _SEH_TRY
    {
        ProbeForWrite(pubNewMode, sizeof(BOOL), 1);
        *pubNewMode = Ret;
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return Ret;
    }

    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

    return Ret;

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
