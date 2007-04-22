/* $Id: main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include <windows.h>
#include "rosdraw.h"
#include "d3dhal.h"
#include "ddrawgdi.h"

#include "ddrawi.h"
DDRAWI_DIRECTDRAW_GBL ddgbl;
DDRAWI_DDRAWSURFACE_GBL ddSurfGbl;



HRESULT
WINAPI
Create_DirectDraw (LPGUID pGUID,
                   LPDIRECTDRAW* pIface,
                   REFIID id,
                   BOOL ex)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)*pIface;

    DX_WINDBG_trace();

    if (This == NULL)
    {
        /* We do not have a DirectDraw interface, we need alloc it*/
        LPDDRAWI_DIRECTDRAW_INT memThis;

        memThis = DxHeapMemAlloc(sizeof(DDRAWI_DIRECTDRAW_INT));
        This = memThis;
        if (This == NULL)
        {
            if (memThis != NULL) DxHeapMemFree(memThis);
            return DDERR_OUTOFMEMORY;
        }
    }
    else
    {
        /* We got the DirectDraw interface alloc and we need create the link */
        LPDDRAWI_DIRECTDRAW_INT  newThis;
        newThis = DxHeapMemAlloc(sizeof(DDRAWI_DIRECTDRAW_INT));
        if (newThis == NULL)
            return DDERR_OUTOFMEMORY;
        /* we need check the GUID lpGUID what type it is */
        if (pGUID != (LPGUID)DDCREATE_HARDWAREONLY)
        {
            if (pGUID !=NULL)
            {
                This = newThis;
                return DDERR_INVALIDDIRECTDRAWGUID;
            }
        }
        newThis->lpLink = This;
        This = newThis;
    }

    /* Fixme release memory alloc if we fail */
    This->lpLcl = DxHeapMemAlloc(sizeof(DDRAWI_DIRECTDRAW_INT));
    if (This->lpLcl == NULL)
        return DDERR_OUTOFMEMORY;

    *pIface = (LPDIRECTDRAW)This;

    /* Get right interface we whant */
    if (Main_DirectDraw_QueryInterface((LPDIRECTDRAW7)This, id, (void**)&pIface))
    {
        if (StartDirectDraw((LPDIRECTDRAW*)This, pGUID, FALSE) == DD_OK);
            return DD_OK;
    }

    return DDERR_INVALIDPARAMS;
}


HRESULT WINAPI
StartDirectDraw(LPDIRECTDRAW* iface, LPGUID lpGuid, BOOL reenable)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DWORD hal_ret = DD_FALSE;
    DWORD hel_ret = DD_FALSE;
    DWORD devicetypes = 0;
    DWORD dwFlags;

    DX_WINDBG_trace();

    /*
     * ddgbl.dwPDevice  is not longer in use in windows 2000 and higher
     * I am using it for device type
     * devicetypes = 1 : both hal and hel are enable
     * devicetypes = 2 : both hal are enable
     * devicetypes = 3 : both hel are enable
     * devicetypes = 4 :loading a guid drv from the register
     */


    if (reenable == FALSE)
    {
        if (This->lpLink == NULL)
        {
            RtlZeroMemory(&ddgbl, sizeof(DDRAWI_DIRECTDRAW_GBL));
            This->lpLcl->lpGbl->dwRefCnt++;
            if (ddgbl.lpDDCBtmp == NULL)
            {
                ddgbl.lpDDCBtmp = (LPDDHAL_CALLBACKS) DxHeapMemAlloc(sizeof(DDHAL_CALLBACKS));
                if (ddgbl.lpDDCBtmp == NULL)
                {
                    DX_STUB_str("Out of memmory");
                    return DD_FALSE;
                }
            }
        }
    }

    if (reenable == FALSE)
    {
        if (lpGuid == NULL)
        {
            devicetypes= 1;

            /* Create HDC for default, hal and hel driver */
            This->lpLcl->hDC =  (ULONG_PTR) GetDC(GetActiveWindow());

            /* cObsolete is undoc in msdn it being use in CreateDCA */
            RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
            RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);
            dwFlags |= DDRAWI_DISPLAYDRV | DDRAWI_GDIDRV;
        }
        else if (lpGuid == (LPGUID) DDCREATE_HARDWAREONLY)
        {
            devicetypes = 2;
            /* Create HDC for default, hal driver */
            This->lpLcl->hDC =  (ULONG_PTR) GetDC(GetActiveWindow());

            /* cObsolete is undoc in msdn it being use in CreateDCA */
            RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
            RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);
            dwFlags |= DDRAWI_DISPLAYDRV | DDRAWI_GDIDRV;
        }
        else if (lpGuid == (LPGUID) DDCREATE_EMULATIONONLY)
        {
            devicetypes = 3;

            /* Create HDC for default, hal and hel driver */
            This->lpLcl->hDC =  (ULONG_PTR) GetDC(GetActiveWindow());

            /* cObsolete is undoc in msdn it being use in CreateDCA */
            RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
            RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);

            dwFlags |= DDRAWI_DISPLAYDRV | DDRAWI_GDIDRV;
        }
        else
        {
            /* FIXME : need getting driver from the GUID that have been pass in from
             * the register. we do not support that yet
             */
             devicetypes = 4;
             This->lpLcl->hDC = (ULONG_PTR) NULL ;
        }

        if ( (HDC)This->lpLcl->hDC == NULL)
        {
            DX_STUB_str("DDERR_OUTOFMEMORY");
            return DDERR_OUTOFMEMORY ;
        }
    }

    This->lpLcl->lpDDCB = ddgbl.lpDDCBtmp;

    /* Startup HEL and HAL */
    This->lpLcl->lpDDCB = This->lpLcl->lpGbl->lpDDCBtmp;
    This->lpLcl->dwProcessId = GetCurrentProcessId();

    switch (devicetypes)
    {
            case 2:
              hal_ret = StartDirectDrawHal(iface, reenable);
              This->lpLcl->lpDDCB->HELDD.dwFlags = 0;
              break;

            case 3:
              hel_ret = StartDirectDrawHel(iface, reenable);
              This->lpLcl->lpDDCB->HALDD.dwFlags = 0;
              break;

            default:
              hal_ret = StartDirectDrawHal(iface, reenable);
              hel_ret = StartDirectDrawHel(iface, reenable);
    }

    if (hal_ret!=DD_OK)
    {
        if (hel_ret!=DD_OK)
        {
            DX_STUB_str("DDERR_NODIRECTDRAWSUPPORT");
            return DDERR_NODIRECTDRAWSUPPORT;
        }
        dwFlags |= DDRAWI_NOHARDWARE;
    }

    if (hel_ret!=DD_OK)
    {
        dwFlags |= DDRAWI_NOEMULATION;

    }
    else
    {
        dwFlags |= DDRAWI_EMULATIONINITIALIZED;
    }

    /* Fill some basic info for Surface */

    This->lpLcl->hDD = ddgbl.hDD;

    return DD_OK;
}

HRESULT WINAPI
StartDirectDrawHel(LPDIRECTDRAW* iface, BOOL reenable)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.CanCreateSurface     = HelDdCanCreateSurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.CreateSurface        = HelDdCreateSurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.CreatePalette        = HelDdCreatePalette;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.DestroyDriver        = HelDdDestroyDriver;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.FlipToGDISurface     = HelDdFlipToGDISurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.GetScanLine          = HelDdGetScanLine;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.SetColorKey          = HelDdSetColorKey;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.SetExclusiveMode     = HelDdSetExclusiveMode;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.SetMode              = HelDdSetMode;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.WaitForVerticalBlank = HelDdWaitForVerticalBlank;

    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.dwFlags =  DDHAL_CB32_CANCREATESURFACE     |
                                          DDHAL_CB32_CREATESURFACE        |
                                          DDHAL_CB32_CREATEPALETTE        |
                                          DDHAL_CB32_DESTROYDRIVER        |
                                          DDHAL_CB32_FLIPTOGDISURFACE     |
                                          DDHAL_CB32_GETSCANLINE          |
                                          DDHAL_CB32_SETCOLORKEY          |
                                          DDHAL_CB32_SETEXCLUSIVEMODE     |
                                          DDHAL_CB32_SETMODE              |
                                          DDHAL_CB32_WAITFORVERTICALBLANK ;

    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.dwSize = sizeof(This->lpLcl->lpDDCB->HELDD);

    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.AddAttachedSurface = HelDdSurfAddAttachedSurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.Blt = HelDdSurfBlt;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.DestroySurface = HelDdSurfDestroySurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.Flip = HelDdSurfFlip;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.GetBltStatus = HelDdSurfGetBltStatus;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.GetFlipStatus = HelDdSurfGetFlipStatus;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.Lock = HelDdSurfLock;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.reserved4 = HelDdSurfreserved4;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.SetClipList = HelDdSurfSetClipList;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.SetColorKey = HelDdSurfSetColorKey;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.SetOverlayPosition = HelDdSurfSetOverlayPosition;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.SetPalette = HelDdSurfSetPalette;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.Unlock = HelDdSurfUnlock;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.UpdateOverlay = HelDdSurfUpdateOverlay;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.dwFlags = DDHAL_SURFCB32_ADDATTACHEDSURFACE |
                                                DDHAL_SURFCB32_BLT                |
                                                DDHAL_SURFCB32_DESTROYSURFACE     |
                                                DDHAL_SURFCB32_FLIP               |
                                                DDHAL_SURFCB32_GETBLTSTATUS       |
                                                DDHAL_SURFCB32_GETFLIPSTATUS      |
                                                DDHAL_SURFCB32_LOCK               |
                                                DDHAL_SURFCB32_RESERVED4          |
                                                DDHAL_SURFCB32_SETCLIPLIST        |
                                                DDHAL_SURFCB32_SETCOLORKEY        |
                                                DDHAL_SURFCB32_SETOVERLAYPOSITION |
                                                DDHAL_SURFCB32_SETPALETTE         |
                                                DDHAL_SURFCB32_UNLOCK             |
                                                DDHAL_SURFCB32_UPDATEOVERLAY;

    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.dwSize = sizeof(This->lpLcl->lpDDCB->HELDDSurface);

    /*
    This->lpLcl->lpDDCB->HELDDPalette.DestroyPalette  = HelDdPalDestroyPalette;
    This->lpLcl->lpDDCB->HELDDPalette.SetEntries = HelDdPalSetEntries;
    This->lpLcl->lpDDCB->HELDDPalette.dwSize = sizeof(This->lpLcl->lpDDCB->HELDDPalette);
    */

    /*
    This->lpLcl->lpDDCB->HELDDExeBuf.CanCreateExecuteBuffer = HelDdExeCanCreateExecuteBuffer;
    This->lpLcl->lpDDCB->HELDDExeBuf.CreateExecuteBuffer = HelDdExeCreateExecuteBuffer;
    This->lpLcl->lpDDCB->HELDDExeBuf.DestroyExecuteBuffer = HelDdExeDestroyExecuteBuffer;
    This->lpLcl->lpDDCB->HELDDExeBuf.LockExecuteBuffer = HelDdExeLockExecuteBuffer;
    This->lpLcl->lpDDCB->HELDDExeBuf.UnlockExecuteBuffer = HelDdExeUnlockExecuteBuffer;
    */

    return DD_OK;
}


HRESULT WINAPI
StartDirectDrawHal(LPDIRECTDRAW* iface, BOOL reenable)
{


    LPDWORD mpFourCC;
    DDHALINFO mHALInfo;
    BOOL newmode = FALSE;
    LPDDSURFACEDESC mpTextures;
    D3DHAL_CALLBACKS mD3dCallbacks;
    D3DHAL_GLOBALDRIVERDATA mD3dDriverData;
    DDHAL_DDEXEBUFCALLBACKS mD3dBufferCallbacks;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;


    RtlZeroMemory(&mHALInfo, sizeof(DDHALINFO));
    RtlZeroMemory(&mD3dCallbacks, sizeof(D3DHAL_CALLBACKS));
    RtlZeroMemory(&mD3dDriverData, sizeof(D3DHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&mD3dBufferCallbacks, sizeof(DDHAL_DDEXEBUFCALLBACKS));

    if (reenable == FALSE)
    {
        ddgbl.lpDDCBtmp = DxHeapMemAlloc(sizeof(DDHAL_CALLBACKS));
        if ( ddgbl.lpDDCBtmp == NULL)
        {
            return DD_FALSE;
        }
    }
    else
    {
        RtlZeroMemory(ddgbl.lpDDCBtmp,sizeof(DDHAL_CALLBACKS));
    }

    /*
     *  Startup DX HAL step one of three
     */
    if (!DdCreateDirectDrawObject(This->lpLcl->lpGbl, (HDC)This->lpLcl->hDC))
    {
       DxHeapMemFree(ddgbl.lpDDCBtmp);
       return DD_FALSE;
    }

    /* Some card disable the dx after it have been created so
     * we are force reanble it
     */
    if (!DdReenableDirectDrawObject(This->lpLcl->lpGbl, &newmode))
    {
      DxHeapMemFree(ddgbl.lpDDCBtmp);
      return DD_FALSE;
    }

    if (!DdQueryDirectDrawObject(This->lpLcl->lpGbl,
                                 &mHALInfo,
                                 &ddgbl.lpDDCBtmp->HALDD,
                                 &ddgbl.lpDDCBtmp->HALDDSurface,
                                 &ddgbl.lpDDCBtmp->HALDDPalette,
                                 &mD3dCallbacks,
                                 &mD3dDriverData,
                                 &mD3dBufferCallbacks,
                                 NULL,
                                 NULL,
                                 NULL))
    {
      DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);
      DxHeapMemFree(ddgbl.lpDDCBtmp);
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    /* Alloc mpFourCC */
    mpFourCC = NULL;
    if (mHALInfo.ddCaps.dwNumFourCCCodes)
    {
        mpFourCC = (DWORD *) DxHeapMemAlloc(sizeof(DWORD) * mHALInfo.ddCaps.dwNumFourCCCodes);
        if (mpFourCC == NULL)
        {
            DxHeapMemFree(ddgbl.lpDDCBtmp);
            // FIXME Close DX fristcall and second call
            return DD_FALSE;
        }
    }

    /* Alloc mpTextures */
    mpTextures = NULL;

    if (mD3dDriverData.dwNumTextureFormats)
    {
        mpTextures = (DDSURFACEDESC*) DxHeapMemAlloc(sizeof(DDSURFACEDESC) * mD3dDriverData.dwNumTextureFormats);
        if (mpTextures == NULL)
        {
            DxHeapMemFree( mpFourCC);
            DxHeapMemFree(ddgbl.lpDDCBtmp);
            // FIXME Close DX fristcall and second call
        }
      return DD_FALSE;
    }


    /* Get all basic data from the driver */
    if (!DdQueryDirectDrawObject(
                                 This->lpLcl->lpGbl,
                                 &mHALInfo,
                                 &ddgbl.lpDDCBtmp->HALDD,
                                 &ddgbl.lpDDCBtmp->HALDDSurface,
                                 &ddgbl.lpDDCBtmp->HALDDPalette,
                                 &mD3dCallbacks,
                                 &mD3dDriverData,
                                 &ddgbl.lpDDCBtmp->HALDDExeBuf,
                                 (DDSURFACEDESC*)mpTextures,
                                 mpFourCC,
                                 NULL))
    {
        DxHeapMemFree( mpFourCC);
        DxHeapMemFree( mpTextures);
        DxHeapMemFree(ddgbl.lpDDCBtmp);
        // FIXME Close DX fristcall and second call
        return DD_FALSE;
    }

    memcpy(&ddgbl.vmiData, &mHALInfo.vmiData,sizeof(VIDMEMINFO));
    memcpy(&ddgbl.ddCaps,  &mHALInfo.ddCaps,sizeof(DDCORECAPS));

    This->lpLcl->lpGbl->dwNumFourCC = mHALInfo.ddCaps.dwNumFourCCCodes;
    This->lpLcl->lpGbl->lpdwFourCC = mpFourCC;

    This->lpLcl->lpGbl->dwMonitorFrequency = mHALInfo.dwMonitorFrequency;
    This->lpLcl->lpGbl->dwModeIndex        = mHALInfo.dwModeIndex;
    This->lpLcl->lpGbl->dwNumModes         = mHALInfo.dwNumModes;
    This->lpLcl->lpGbl->lpModeInfo         = mHALInfo.lpModeInfo;
    This->lpLcl->lpGbl->hInstance          = mHALInfo.hInstance;
    This->lpLcl->lpGbl->lp16DD = This->lpLcl->lpGbl;

    /* FIXME convert mpTextures to DDHALMODEINFO */
    DxHeapMemFree( mpTextures);

    /* FIXME D3D setup mD3dCallbacks and mD3dDriverData */
    return DD_OK;
}
