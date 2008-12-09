/* $Id: main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include "rosdraw.h"

DDRAWI_DIRECTDRAW_GBL ddgbl;
DDRAWI_DDRAWSURFACE_GBL ddSurfGbl;

WCHAR classname[128];
WNDCLASSW wnd_class;


HRESULT WINAPI
Create_DirectDraw (LPGUID pGUID, LPDIRECTDRAW* pIface,
                   REFIID id, BOOL reenable)
{
    LPDDRAWI_DIRECTDRAW_INT This;

    DX_WINDBG_trace();

    if ((IsBadReadPtr(pIface,sizeof(LPDIRECTDRAW))) ||
       (IsBadWritePtr(pIface,sizeof(LPDIRECTDRAW))))
    {
        return DDERR_INVALIDPARAMS;
    }

    This = (LPDDRAWI_DIRECTDRAW_INT)*pIface;

    if ( (IsBadWritePtr(This,sizeof(LPDDRAWI_DIRECTDRAW_INT)) != 0) || 
         (IsBadWritePtr(This->lpLcl,sizeof(LPDDRAWI_DIRECTDRAW_LCL)) != 0) )
    {
        /* We do not have a DirectDraw interface, we need alloc it*/
        LPDDRAWI_DIRECTDRAW_INT memThis;

        DX_STUB_str("1. no linking\n");

        DxHeapMemAlloc(memThis, sizeof(DDRAWI_DIRECTDRAW_INT));
        if (memThis == NULL)
        {
            return DDERR_OUTOFMEMORY;
        }

        This = memThis;

        /* Fixme release memory alloc if we fail */

        DxHeapMemAlloc(This->lpLcl, sizeof(DDRAWI_DIRECTDRAW_LCL));
        if (This->lpLcl == NULL)
        {
            return DDERR_OUTOFMEMORY;
        }
    }
    else
    {
        /* We got the DirectDraw interface alloc and we need create the link */
        LPDDRAWI_DIRECTDRAW_INT  newThis;

        DX_STUB_str("2.linking\n");

        /* step 1.Alloc the new  DDRAWI_DIRECTDRAW_INT for the lnking */
        DxHeapMemAlloc(newThis, sizeof(DDRAWI_DIRECTDRAW_INT));
        if (newThis == NULL)
        {
            return DDERR_OUTOFMEMORY;
        }

        /* step 2 check if it not DDCREATE_HARDWAREONLY we got if so we fail */
        if ((pGUID) && (pGUID != (LPGUID)DDCREATE_HARDWAREONLY))
        {
            if (pGUID !=NULL)
            {
                This = newThis;
                return DDERR_INVALIDDIRECTDRAWGUID;
            }
        }

        /* step 3 do the link the old interface are store in the new one */
        newThis->lpLink = This;

        /* step 4 we need create new local directdraw struct for the new linked interface */
        DxHeapMemAlloc(newThis->lpLcl, sizeof(DDRAWI_DIRECTDRAW_LCL));
        if (newThis->lpLcl == NULL)
        {
            This = newThis;
            return DDERR_OUTOFMEMORY;
        }

        This = newThis;
    }

    This->lpLcl->lpGbl = &ddgbl;

    *pIface = (LPDIRECTDRAW)This;

    /* Get right interface we whant */

    This->lpVtbl = 0;
    if (IsEqualGUID(&IID_IDirectDraw7, id))
    {
        /* DirectDraw7 Vtable */
        This->lpVtbl = &DirectDraw7_Vtable;
        This->lpLcl->dwLocalFlags = This->lpLcl->dwLocalFlags + DDRAWILCL_DIRECTDRAW7;
        *pIface = (LPDIRECTDRAW)&This->lpVtbl;
        Main_DirectDraw_AddRef(This);
    }
    else if (IsEqualGUID(&IID_IDirectDraw4, id))
    {
        /* DirectDraw4 Vtable */
        This->lpVtbl = &DirectDraw4_Vtable;
        *pIface = (LPDIRECTDRAW)&This->lpVtbl;
        Main_DirectDraw_AddRef(This);
    }
    else if (IsEqualGUID(&IID_IDirectDraw2, id))
    {
        /* DirectDraw2 Vtable */
        This->lpVtbl = &DirectDraw2_Vtable;
        *pIface = (LPDIRECTDRAW)&This->lpVtbl;
        Main_DirectDraw_AddRef(This);
    }
    else if (IsEqualGUID(&IID_IDirectDraw, id))
    {
        /* DirectDraw Vtable */
        This->lpVtbl = &DirectDraw_Vtable;
        *pIface = (LPDIRECTDRAW)&This->lpVtbl;
        Main_DirectDraw_AddRef(This);
    }

    if ( This->lpVtbl != 0)
    {
        DX_STUB_str("Got iface\n");

        if (StartDirectDraw((LPDIRECTDRAW)This, pGUID, FALSE) == DD_OK)
        {
            /*
            RtlZeroMemory(&wnd_class, sizeof(wnd_class));
            wnd_class.style = CS_HREDRAW | CS_VREDRAW;
            wnd_class.lpfnWndProc = DefWindowProcW;
            wnd_class.cbClsExtra = 0;
            wnd_class.cbWndExtra = 0;
            wnd_class.hInstance = GetModuleHandleW(0);
            wnd_class.hIcon = 0;
            wnd_class.hCursor = 0;
            wnd_class.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
            wnd_class.lpszMenuName = NULL;
            wnd_class.lpszClassName = classname;
            if(!RegisterClassW(&wnd_class))
            {
                DX_STUB_str("DDERR_GENERIC");
                return DDERR_GENERIC;
            }
            */
            This->lpLcl->hDD = ddgbl.hDD;
            return DD_OK;
        }
    }

    return DDERR_INVALIDPARAMS;
}


HRESULT WINAPI
StartDirectDraw(LPDIRECTDRAW iface, LPGUID lpGuid, BOOL reenable)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DWORD hal_ret = DD_FALSE;
    DWORD hel_ret = DD_FALSE;
    DWORD devicetypes = 0;
    DWORD dwFlags = 0;


    DX_WINDBG_trace();


    /*
     * ddgbl.dwPDevice  is not longer in use in windows 2000 and higher
     * I am using it for device type
     * devicetypes = 1 : both hal and hel are enable
     * devicetypes = 2 : both hal are enable
     * devicetypes = 3 : both hel are enable
     * devicetypes = 4 :loading a guid drv from the register
     */

    ddgbl.lpDriverHandle = &ddgbl;
    ddgbl.hDDVxd = -1;

    if (reenable == FALSE)
    {
        if ((!IsBadReadPtr(This->lpLink,sizeof(LPDIRECTDRAW))) && (This->lpLink == NULL))
        {
            RtlZeroMemory(&ddgbl, sizeof(DDRAWI_DIRECTDRAW_GBL));
            This->lpLcl->lpGbl->dwRefCnt++;
            if (ddgbl.lpDDCBtmp == NULL)
            {
                // LPDDHAL_CALLBACKS
                DxHeapMemAlloc( ddgbl.lpDDCBtmp , sizeof(DDHAL_CALLBACKS));
                if (ddgbl.lpDDCBtmp == NULL)
                {
                    DX_STUB_str("Out of memmory\n");
                    return DD_FALSE;
                }
            }
        }

        DxHeapMemAlloc(ddgbl.lpModeInfo, sizeof(DDHALMODEINFO));
        if (!ddgbl.lpModeInfo)
        {
            return DDERR_OUTOFMEMORY;
        }

    }
    /* Windows handler are by set of SetCooperLevel
     * so do not set it
     */

    if (reenable == FALSE)
    {
        if (lpGuid == NULL)
        {
            devicetypes= 1;

            /* Create HDC for default, hal and hel driver */
            // This->lpLcl->hWnd = (ULONG_PTR) GetActiveWindow();
            This->lpLcl->hDC = (ULONG_PTR)CreateDCA("DISPLAY",NULL,NULL,NULL);

            /* cObsolete is undoc in msdn it being use in CreateDCA */
            RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
            RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);
            dwFlags |= DDRAWI_DISPLAYDRV | DDRAWI_GDIDRV;
        }
        else if (lpGuid == (LPGUID) DDCREATE_HARDWAREONLY)
        {
            devicetypes = 2;
            /* Create HDC for default, hal driver */
            // This->lpLcl->hWnd =(ULONG_PTR) GetActiveWindow();
            This->lpLcl->hDC = (ULONG_PTR)CreateDCA("DISPLAY",NULL,NULL,NULL);

            /* cObsolete is undoc in msdn it being use in CreateDCA */
            RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
            RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);
            dwFlags |= DDRAWI_DISPLAYDRV | DDRAWI_GDIDRV;
        }
        else if (lpGuid == (LPGUID) DDCREATE_EMULATIONONLY)
        {
            devicetypes = 3;

            /* Create HDC for default, hal and hel driver */
            //This->lpLcl->hWnd = (ULONG_PTR) GetActiveWindow();
            This->lpLcl->hDC = (ULONG_PTR)CreateDCA("DISPLAY",NULL,NULL,NULL);

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
             //This->lpLcl->hDC = (ULONG_PTR) NULL ;
             //This->lpLcl->hDC = (ULONG_PTR)CreateDCA("DISPLAY",NULL,NULL,NULL);
        }

        /*
        if ( (HDC)This->lpLcl->hDC == NULL)
        {
            DX_STUB_str("DDERR_OUTOFMEMORY\n");
            return DDERR_OUTOFMEMORY ;
        }
        */
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
              break;
    }

    if (hal_ret!=DD_OK)
    {
        if (hel_ret!=DD_OK)
        {
            DX_STUB_str("DDERR_NODIRECTDRAWSUPPORT\n");
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
    This->lpLcl->lpGbl->dwFlags =  This->lpLcl->lpGbl->dwFlags | dwFlags | DDRAWI_ATTACHEDTODESKTOP;
    This->lpLcl->lpDDCB = This->lpLcl->lpGbl->lpDDCBtmp;
    This->lpLcl->hDD = ddgbl.hDD;

    ddgbl.rectDevice.top = 0;
    ddgbl.rectDevice.left = 0;
    ddgbl.rectDevice.right = ddgbl.vmiData.dwDisplayWidth;
    ddgbl.rectDevice.bottom = ddgbl.vmiData.dwDisplayHeight;

    ddgbl.rectDesktop.top = 0;
    ddgbl.rectDesktop.left = 0;
    ddgbl.rectDesktop.right = ddgbl.vmiData.dwDisplayWidth;
    ddgbl.rectDesktop.bottom = ddgbl.vmiData.dwDisplayHeight;

    ddgbl.dwMonitorFrequency = GetDeviceCaps(GetWindowDC(NULL),VREFRESH);
    ddgbl.lpModeInfo->dwWidth      = ddgbl.vmiData.dwDisplayWidth;
    ddgbl.lpModeInfo->dwHeight     = ddgbl.vmiData.dwDisplayHeight;
    ddgbl.lpModeInfo->dwBPP        = ddgbl.vmiData.ddpfDisplay.dwRGBBitCount;
    ddgbl.lpModeInfo->lPitch       = ddgbl.vmiData.lDisplayPitch;
    ddgbl.lpModeInfo->wRefreshRate = ddgbl.dwMonitorFrequency;
    ddgbl.lpModeInfo->dwRBitMask = ddgbl.vmiData.ddpfDisplay.dwRBitMask;
    ddgbl.lpModeInfo->dwGBitMask = ddgbl.vmiData.ddpfDisplay.dwGBitMask;
    ddgbl.lpModeInfo->dwBBitMask = ddgbl.vmiData.ddpfDisplay.dwBBitMask;
    ddgbl.lpModeInfo->dwAlphaBitMask = ddgbl.vmiData.ddpfDisplay.dwRGBAlphaBitMask;

    return DD_OK;
}


HRESULT WINAPI
StartDirectDrawHel(LPDIRECTDRAW iface, BOOL reenable)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    if (reenable == FALSE)
    {
        if (ddgbl.lpDDCBtmp == NULL)
        {
            DxHeapMemAlloc(ddgbl.lpDDCBtmp, sizeof(DDHAL_CALLBACKS));
            if ( ddgbl.lpDDCBtmp == NULL)
            {
                return DD_FALSE;
            }
        }
    }

    ddgbl.lpDDCBtmp->HELDD.CanCreateSurface     = HelDdCanCreateSurface;
    ddgbl.lpDDCBtmp->HELDD.CreateSurface        = HelDdCreateSurface;
    ddgbl.lpDDCBtmp->HELDD.CreatePalette        = HelDdCreatePalette;
    ddgbl.lpDDCBtmp->HELDD.DestroyDriver        = HelDdDestroyDriver;
    ddgbl.lpDDCBtmp->HELDD.FlipToGDISurface     = HelDdFlipToGDISurface;
    ddgbl.lpDDCBtmp->HELDD.GetScanLine          = HelDdGetScanLine;
    ddgbl.lpDDCBtmp->HELDD.SetColorKey          = HelDdSetColorKey;
    ddgbl.lpDDCBtmp->HELDD.SetExclusiveMode     = HelDdSetExclusiveMode;
    ddgbl.lpDDCBtmp->HELDD.SetMode              = HelDdSetMode;
    ddgbl.lpDDCBtmp->HELDD.WaitForVerticalBlank = HelDdWaitForVerticalBlank;

    ddgbl.lpDDCBtmp->HELDD.dwFlags =  DDHAL_CB32_CANCREATESURFACE     |
                                          DDHAL_CB32_CREATESURFACE        |
                                          DDHAL_CB32_CREATEPALETTE        |
                                          DDHAL_CB32_DESTROYDRIVER        |
                                          DDHAL_CB32_FLIPTOGDISURFACE     |
                                          DDHAL_CB32_GETSCANLINE          |
                                          DDHAL_CB32_SETCOLORKEY          |
                                          DDHAL_CB32_SETEXCLUSIVEMODE     |
                                          DDHAL_CB32_SETMODE              |
                                          DDHAL_CB32_WAITFORVERTICALBLANK ;

    ddgbl.lpDDCBtmp->HELDD.dwSize = sizeof(This->lpLcl->lpDDCB->HELDD);

    ddgbl.lpDDCBtmp->HELDDSurface.AddAttachedSurface = HelDdSurfAddAttachedSurface;
    ddgbl.lpDDCBtmp->HELDDSurface.Blt = HelDdSurfBlt;
    ddgbl.lpDDCBtmp->HELDDSurface.DestroySurface = HelDdSurfDestroySurface;
    ddgbl.lpDDCBtmp->HELDDSurface.Flip = HelDdSurfFlip;
    ddgbl.lpDDCBtmp->HELDDSurface.GetBltStatus = HelDdSurfGetBltStatus;
    ddgbl.lpDDCBtmp->HELDDSurface.GetFlipStatus = HelDdSurfGetFlipStatus;
    ddgbl.lpDDCBtmp->HELDDSurface.Lock = HelDdSurfLock;
    ddgbl.lpDDCBtmp->HELDDSurface.reserved4 = HelDdSurfreserved4;
    ddgbl.lpDDCBtmp->HELDDSurface.SetClipList = HelDdSurfSetClipList;
    ddgbl.lpDDCBtmp->HELDDSurface.SetColorKey = HelDdSurfSetColorKey;
    ddgbl.lpDDCBtmp->HELDDSurface.SetOverlayPosition = HelDdSurfSetOverlayPosition;
    ddgbl.lpDDCBtmp->HELDDSurface.SetPalette = HelDdSurfSetPalette;
    ddgbl.lpDDCBtmp->HELDDSurface.Unlock = HelDdSurfUnlock;
    ddgbl.lpDDCBtmp->HELDDSurface.UpdateOverlay = HelDdSurfUpdateOverlay;
    ddgbl.lpDDCBtmp->HELDDSurface.dwFlags = DDHAL_SURFCB32_ADDATTACHEDSURFACE |
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

    ddgbl.lpDDCBtmp->HELDDSurface.dwSize = sizeof(This->lpLcl->lpDDCB->HELDDSurface);

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
StartDirectDrawHal(LPDIRECTDRAW iface, BOOL reenable)
{
    LPDWORD mpFourCC = NULL;
    DDHALINFO mHALInfo;
    BOOL newmode = FALSE;
    LPDDSURFACEDESC mpTextures;
    D3DHAL_CALLBACKS mD3dCallbacks;
    D3DHAL_GLOBALDRIVERDATA mD3dDriverData;
    DDHAL_DDEXEBUFCALLBACKS mD3dBufferCallbacks;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DDHAL_GETDRIVERINFODATA DdGetDriverInfo = { 0 };

    DX_WINDBG_trace();

    RtlZeroMemory(&mHALInfo, sizeof(DDHALINFO));
    RtlZeroMemory(&mD3dCallbacks, sizeof(D3DHAL_CALLBACKS));
    RtlZeroMemory(&mD3dDriverData, sizeof(D3DHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&mD3dBufferCallbacks, sizeof(DDHAL_DDEXEBUFCALLBACKS));

    if (reenable == FALSE)
    {
        if (ddgbl.lpDDCBtmp == NULL)
        {
            DxHeapMemAlloc(ddgbl.lpDDCBtmp, sizeof(DDHAL_CALLBACKS));
            if ( ddgbl.lpDDCBtmp == NULL)
            {
                return DD_FALSE;
            }
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
                                 mpFourCC,
                                 NULL))
    {
      DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);
      DxHeapMemFree(ddgbl.lpDDCBtmp);
      // FIXME Close DX first and second call
      return DD_FALSE;
    }

    /* Alloc mpFourCC */
    if (This->lpLcl->lpGbl->lpdwFourCC != NULL)
    {
        DxHeapMemFree(This->lpLcl->lpGbl->lpdwFourCC);
    }

    if (mHALInfo.ddCaps.dwNumFourCCCodes > 0 )
    {

        DxHeapMemAlloc(mpFourCC, sizeof(DWORD) * (mHALInfo.ddCaps.dwNumFourCCCodes + 2));

        if (mpFourCC == NULL)
        {
            DxHeapMemFree(ddgbl.lpDDCBtmp);
            // FIXME Close DX first and second call
            return DD_FALSE;
        }
    }

    /* Alloc mpTextures */
#if 0
    DX_STUB_str("1 Here\n");

    if (This->lpLcl->lpGbl->texture != NULL)
    {
        DxHeapMemFree(This->lpLcl->lpGbl->texture;
    }

    mpTextures = NULL;
    if (mD3dDriverData.dwNumTextureFormats > 0)
    {
        mpTextures = (DDSURFACEDESC*) DxHeapMemAlloc(sizeof(DDSURFACEDESC) * mD3dDriverData.dwNumTextureFormats);
        if (mpTextures == NULL)
        {
            DxHeapMemFree(mpFourCC);
            DxHeapMemFree(ddgbl.lpDDCBtmp);
            // FIXME Close DX first and second call
        }
    }

    DX_STUB_str("2 Here\n");

#else
      mpTextures = NULL;
#endif


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
        DxHeapMemFree(mpFourCC);
        DxHeapMemFree(mpTextures);
        DxHeapMemFree(ddgbl.lpDDCBtmp);
        // FIXME Close DX first and second call
        return DD_FALSE;
    }

    memcpy(&ddgbl.vmiData, &mHALInfo.vmiData,sizeof(VIDMEMINFO));


    memcpy(&ddgbl.ddCaps,  &mHALInfo.ddCaps,sizeof(DDCORECAPS));

    This->lpLcl->lpGbl->dwNumFourCC        = mHALInfo.ddCaps.dwNumFourCCCodes;
    This->lpLcl->lpGbl->lpdwFourCC         = mpFourCC;
    // This->lpLcl->lpGbl->dwMonitorFrequency = mHALInfo.dwMonitorFrequency;     // 0
    This->lpLcl->lpGbl->dwModeIndex        = mHALInfo.dwModeIndex;
    // This->lpLcl->lpGbl->dwNumModes         = mHALInfo.dwNumModes;
    // This->lpLcl->lpGbl->lpModeInfo         = mHALInfo.lpModeInfo;

    /* FIXME convert mpTextures to DDHALMODEINFO */
    // DxHeapMemFree( mpTextures);

    /* FIXME D3D setup mD3dCallbacks and mD3dDriverData */




    if (mHALInfo.dwFlags & DDHALINFO_GETDRIVERINFOSET)
    {
        DdGetDriverInfo.dwSize = sizeof (DDHAL_GETDRIVERINFODATA);
        DdGetDriverInfo.guidInfo = GUID_MiscellaneousCallbacks;
        DdGetDriverInfo.lpvData = (PVOID)&ddgbl.lpDDCBtmp->HALDDMiscellaneous;
        DdGetDriverInfo.dwExpectedSize = sizeof (DDHAL_DDMISCELLANEOUSCALLBACKS);

        if(mHALInfo.GetDriverInfo (&DdGetDriverInfo) == DDHAL_DRIVER_NOTHANDLED || DdGetDriverInfo.ddRVal != DD_OK)
        {
            DxHeapMemFree(mpFourCC);
            DxHeapMemFree(mpTextures);
            DxHeapMemFree(ddgbl.lpDDCBtmp);
            // FIXME Close DX fristcall and second call
            return DD_FALSE;
        }

        RtlZeroMemory(&DdGetDriverInfo, sizeof(DDHAL_GETDRIVERINFODATA));
        DdGetDriverInfo.dwSize = sizeof (DDHAL_GETDRIVERINFODATA);
        DdGetDriverInfo.guidInfo = GUID_Miscellaneous2Callbacks;

        /* FIXME
        DdGetDriverInfo.lpvData = (PVOID)&ddgbl.lpDDCBtmp->HALDDMiscellaneous;
        DdGetDriverInfo.dwExpectedSize = sizeof (DDHAL_DDMISCELLANEOUS2CALLBACKS);

        if(mHALInfo.GetDriverInfo (&DdGetDriverInfo) == DDHAL_DRIVER_NOTHANDLED || DdGetDriverInfo.ddRVal != DD_OK)
        {
            DxHeapMemFree(mpFourCC);
            DxHeapMemFree(mpTextures);
            DxHeapMemFree(ddgbl.lpDDCBtmp);
            // FIXME Close DX fristcall and second call
            return DD_FALSE;
        }
        DD_MISCELLANEOUS2CALLBACKS
        {
            DWORD                dwSize;
            DWORD                dwFlags;
            PDD_ALPHABLT         AlphaBlt;  // unsuse acoding msdn and always set to NULL
            PDD_CREATESURFACEEX  CreateSurfaceEx;
            PDD_GETDRIVERSTATE   GetDriverState;
            PDD_DESTROYDDLOCAL   DestroyDDLocal;
        }
          DDHAL_MISC2CB32_CREATESURFACEEX
          DDHAL_MISC2CB32_GETDRIVERSTATE
          DDHAL_MISC2CB32_DESTROYDDLOCAL
        */
    }

    if (mHALInfo.dwFlags & DDHALINFO_GETDRIVERINFO2)
    {
        This->lpLcl->lpGbl->dwFlags = This->lpLcl->lpGbl->dwFlags | DDRAWI_DRIVERINFO2;
    }


    return DD_OK;
}
