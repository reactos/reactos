/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/ddraw.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */

#include <w32k.h>
#include <debug.h>
#define DXDDRAWDEBUG 1

PGD_DXDDSTARTUPDXGRAPHICS gpfnStartupDxGraphics = NULL;
PGD_DXDDCLEANUPDXGRAPHICS gpfnCleanupDxGraphics = NULL;

/* export from dxeng.c */
extern DRVFN gaEngFuncs;
extern ULONG gcEngFuncs;

DRVFN gpDxFuncs[DXG_INDEX_DxDdIoctl];
HANDLE ghDxGraphics = NULL;
ULONG gdwDirectDrawContext;

EDD_DIRECTDRAW_GLOBAL edd_DdirectDraw_Global;


/************************************************************************/
/* DirectX graphic/video driver enable start here                       */
/************************************************************************/
BOOL
intEnableReactXDriver(PEDD_DIRECTDRAW_GLOBAL pEddgbl, PDC pDC)
{
    PGDIDEVICE pDev = (PGDIDEVICE)pDC->pPDev;
    BOOLEAN success = FALSE;
    DD_GETDRIVERINFODATA GetInfo;

    /*clean up some of the cache entry */
    RtlZeroMemory(pEddgbl,sizeof(EDD_DIRECTDRAW_GLOBAL));

    /* setup hdev for edd_DdirectDraw_Global xp */
    edd_DdirectDraw_Global.hDev   = pDC->pPDev;
    /*FIXME : edd_DdirectDraw_Global.dhpdev   =  (PVOID)pDC->PDev; */

    /* setup EDD_DIRECTDRAW_GLOBAL for pDev xp */
    pDev->pEDDgpl = pEddgbl;

     /* test see if drv got a dx interface or not */
    if  ( ( pDev->DriverFunctions.DisableDirectDraw == NULL) ||
          ( pDev->DriverFunctions.EnableDirectDraw == NULL))
    {
        DPRINT1("Waring : DisableDirectDraw and EnableDirectDraw are NULL, no dx driver \n");
        return FALSE;
    }

    /* Get DirectDraw infomations form the driver 
     * DDK say pvmList, pdwFourCC is always NULL in frist call here 
     * but we get back how many pvmList it whant we should alloc, same 
     * with pdwFourCC.
     */

    if (pDev->DriverFunctions.GetDirectDrawInfo)
    {
        // DPRINT1("if u are using vmware driver and see this msg, please repot this\n");
        DPRINT1("at DrvGetDirectDrawInfo  \n");
        success = pDev->DriverFunctions.GetDirectDrawInfo( pDC->PDev, 
                                                               &pEddgbl->ddHalInfo,
                                                               &pEddgbl->dwNumHeaps,
                                                               NULL,
                                                               &pEddgbl->dwNumFourCC,
                                                               NULL);
        if (!success)
        {
            DPRINT1("DrvGetDirectDrawInfo  frist call fail\n");
            return FALSE;
        }
        DPRINT1(" DrvGetDirectDrawInfo  OK\n");

        /* The driver are not respnose to alloc the memory for pvmList
         * but it is win32k responsible todo, Windows 9x it is gdi32.dll
         */

        if (pEddgbl->dwNumHeaps != 0)
        {
            DPRINT1("Setup pvmList\n");
            pEddgbl->pvmList = (PVIDEOMEMORY) ExAllocatePoolWithTag(PagedPool, pEddgbl->dwNumHeaps * sizeof(VIDEOMEMORY), TAG_DXPVMLIST);
            if (pEddgbl->pvmList == NULL)
            {
                DPRINT1("pvmList memmery alloc fail\n");
                return FALSE;
            }
        }

        /* The driver are not respnose to alloc the memory for pdwFourCC
         * but it is win32k responsible todo, Windows 9x it is gdi32.dll
         */
        if (pEddgbl->dwNumFourCC != 0)
        {
            DPRINT1("Setup pdwFourCC\n");
            pEddgbl->pdwFourCC = (LPDWORD) ExAllocatePoolWithTag(PagedPool, pEddgbl->dwNumFourCC * sizeof(DWORD), TAG_DXFOURCC);

            if (pEddgbl->pdwFourCC == NULL)
            {
                DPRINT1("pdwFourCC memmery alloc fail\n");
                return FALSE;
            }
        }
        success = pDev->DriverFunctions.GetDirectDrawInfo( pDC->PDev,
                                                          &pEddgbl->ddHalInfo,
                                                          &pEddgbl->dwNumHeaps,
                                                          pEddgbl->pvmList,
                                                          &pEddgbl->dwNumFourCC,
                                                          pEddgbl->pdwFourCC);
        if (!success)
        {
            DPRINT1("DrvGetDirectDrawInfo  second call fail\n");
            return FALSE;
        }

        /* We need now convert the DD_HALINFO we got, it can be NT4 driver we 
         * loading ReactOS supporting NT4 and higher to be loading.so we make
         * the HALInfo compatible here so we can easy pass it to gdi32.dll 
         * without converting it later 
         */

        if ((pEddgbl->ddHalInfo.dwSize != sizeof(DD_HALINFO)) && 
            (pEddgbl->ddHalInfo.dwSize != sizeof(DD_HALINFO_V4)))
        {
            DPRINT1(" Fail not vaild driver DD_HALINFO struct found\n");
            return FALSE;
        }

        if (pEddgbl->ddHalInfo.dwSize != sizeof(DD_HALINFO))
        {
            if (pEddgbl->ddHalInfo.dwSize == sizeof(DD_HALINFO_V4))
            {
                /* NT4 Compatible */
                DPRINT1("Got DD_HALINFO_V4 sturct we convert it to DD_HALINFO \n");
                pEddgbl->ddHalInfo.dwSize = sizeof(DD_HALINFO);
                pEddgbl->ddHalInfo.lpD3DGlobalDriverData = NULL;
                pEddgbl->ddHalInfo.lpD3DHALCallbacks = NULL;
                pEddgbl->ddHalInfo.lpD3DBufCallbacks = NULL;
            }
            else
            {
                /* Unknown version found */
                DPRINT1(" Fail : did not get DD_HALINFO size \n");
                return FALSE;
            }
        }
    }

    DPRINT1("Trying EnableDirectDraw the driver\n");

    success = pDev->DriverFunctions.EnableDirectDraw( pDC->PDev, 
                                                      &pEddgbl->ddCallbacks, 
                                                      &pEddgbl->ddSurfaceCallbacks, 
                                                      &pEddgbl->ddPaletteCallbacks);

    if (!success)
    {
        DPRINT1("EnableDirectDraw call fail\n");
        return FALSE;
    }

    GetInfo.dhpdev = pDC->PDev;


    /* Note this check will fail on some nvida drv, it is a bug in their drv not in our code, 
     * we doing proper check if GetDriverInfo exists */
    if  ( ((pEddgbl->ddHalInfo.dwFlags & (DDHALINFO_GETDRIVERINFOSET | DDHALINFO_GETDRIVERINFO2)) != 0) &&
          (pEddgbl->ddHalInfo.GetDriverInfo != NULL) )
    {
        GetInfo.dhpdev = pDC->PDev;
        GetInfo.dwSize = sizeof (DD_GETDRIVERINFODATA);
        GetInfo.dwFlags = 0x00;
        GetInfo.guidInfo = GUID_VideoPortCallbacks;
        GetInfo.lpvData = (PVOID)&pEddgbl->ddVideoPortCallback;
        GetInfo.dwExpectedSize = sizeof (DD_VIDEOPORTCALLBACKS);
        GetInfo.ddRVal = DDERR_GENERIC;
        if ( ( pEddgbl->ddHalInfo.GetDriverInfo (&GetInfo) == DDHAL_DRIVER_NOTHANDLED) || 
             (GetInfo.ddRVal != DD_OK) )
        {
            DPRINT1(" Fail : did not get DD_VIDEOPORTCALLBACKS \n");
        }
        else
        {
            pEddgbl->dwCallbackFlags |= EDDDGBL_VIDEOPORTCALLBACKS;
        }

        /* FIXME fill in videoport caps */
    }
    else
    {
        DPRINT1(" Fail : did not foundpEddgbl->ddHalInfo.GetDriverInfo \n");
    }

    /* Note this check will fail on some nvida drv, it is a bug in their drv not in our code, 
     * we doing proper check if GetDriverInfo exists */
    if  ( ((pEddgbl->ddHalInfo.dwFlags & (DDHALINFO_GETDRIVERINFOSET | DDHALINFO_GETDRIVERINFO2)) != 0) &&
          (pEddgbl->ddHalInfo.GetDriverInfo != NULL) )
    {
        GetInfo.dhpdev = pDC->PDev;
        GetInfo.dwSize = sizeof (DD_GETDRIVERINFODATA);
        GetInfo.dwFlags = 0x00;
        GetInfo.guidInfo = GUID_MiscellaneousCallbacks;
        GetInfo.lpvData = (PVOID)&pEddgbl->ddMiscellanousCallbacks;
        GetInfo.dwExpectedSize = sizeof (DD_MISCELLANEOUSCALLBACKS);
        GetInfo.ddRVal = DDERR_GENERIC;
        if ( ( pEddgbl->ddHalInfo.GetDriverInfo (&GetInfo) == DDHAL_DRIVER_NOTHANDLED) || 
             (GetInfo.ddRVal != DD_OK) )
        {
            DPRINT1(" Fail : did not get DD_MISCELLANEOUSCALLBACKS \n");
        }
        else
        {
            pEddgbl->dwCallbackFlags |= EDDDGBL_MISCCALLBACKS;
        }
    }
    else
    {
        DPRINT1(" Fail : did not foundpEddgbl->ddHalInfo.GetDriverInfo \n");
    }

    /* setup missing data in ddHalInfo */
    //pEddgbl->ddHalInfo.GetDriverInfo = (PVOID)pDev->DriverFunctions.GetDirectDrawInfo;

    /* FIXME : remove this when we are done with debuging of dxg */
    dump_edd_directdraw_global(pEddgbl);
    dump_halinfo(&pEddgbl->ddHalInfo);
    return TRUE;
}

/************************************************************************/
/* DirectX graphic/video driver enable ends here                        */
/************************************************************************/

/************************************************************************/
/* DirectX graphic/video driver loading and cleanup start here          */
/************************************************************************/
NTSTATUS
STDCALL
DxDdStartupDxGraphics(  ULONG ulc1,
                        PDRVENABLEDATA DxEngDrvOld,
                        ULONG ulc2,
                        PDRVENABLEDATA DxgDrvOld,
                        PULONG DirectDrawContext,
                        PEPROCESS Proc)
{
    DRVENABLEDATA DxEngDrv;
    DRVENABLEDATA DxgDrv;

    NTSTATUS Status = STATUS_PROCEDURE_NOT_FOUND;

    /* FIXME setup of gaEngFuncs driver export list
     * but not in this api, we can add it here tempary until we figout where 
     * no code have been writen for it yet
     */


    /* FIXME ReactOS does not loading the dxapi.sys or import functions from it yet */
    // DxApiGetVersion()

    /* Loading the kernel interface of directx for win32k */

    DPRINT1("Warning: trying loading vista dxkrnl.sys\n");
    ghDxGraphics = EngLoadImage(L"\\SystemRoot\\System32\\drivers\\dxkrnl.sys"); 
    if ( ghDxGraphics == NULL)
    {
        DPRINT1("Warning: dxkrnl.sys not found\n");
        /* try loading vista dx kernel */
        DPRINT1("Warning: trying loading xp/2003/reactos dxg.sys\n");
        ghDxGraphics = EngLoadImage(L"\\SystemRoot\\System32\\drivers\\dxg.sys");
    }

    if ( ghDxGraphics == NULL)
    {
        Status = STATUS_DLL_NOT_FOUND;
        DPRINT1("Warning: no ReactX or DirectX kernel driver found\n");
    }
    else
    {
        /* import DxDdStartupDxGraphics and  DxDdCleanupDxGraphics */
        gpfnStartupDxGraphics = EngFindImageProcAddress(ghDxGraphics,"DxDdStartupDxGraphics");
        gpfnCleanupDxGraphics = EngFindImageProcAddress(ghDxGraphics,"DxDdCleanupDxGraphics");

        if ((gpfnStartupDxGraphics) &&
            (gpfnCleanupDxGraphics))
        {
            /* Setup driver data for activate the dx interface */
            DxEngDrv.iDriverVersion = DDI_DRIVER_VERSION_NT5_01;
            DxEngDrv.pdrvfn = &gaEngFuncs;
            DxEngDrv.c = gcEngFuncs;

            Status = gpfnStartupDxGraphics ( sizeof(DRVENABLEDATA),
                                             &DxEngDrv,
                                             sizeof(DRVENABLEDATA),
                                             &DxgDrv,
                                             &gdwDirectDrawContext,
                                             Proc );
        }

        /* check if we manger loading the data and execute the dxStartupDxGraphics and it susscess */
        if (!NT_SUCCESS(Status))
        {
            gpfnStartupDxGraphics = NULL;
            gpfnCleanupDxGraphics = NULL;
            if (ghDxGraphics != NULL)
            {
                EngUnloadImage( ghDxGraphics);
                ghDxGraphics = NULL;
            }
            DPRINT1("Warning: DirectX graphics interface can not be initialized\n");
        }
        else
        {
            /* Sort the drv functions list in index order, this allown us doing, smaller optimze
             * in api that are redirect to dx.sys
             */

            PDRVFN lstDrvFN = DxgDrv.pdrvfn;
            INT t;
            for (t=0;t<=DXG_INDEX_DxDdIoctl;t++)
            {
                gpDxFuncs[lstDrvFN[t].iFunc].iFunc =lstDrvFN[t].iFunc;
                gpDxFuncs[lstDrvFN[t].iFunc].pfn =lstDrvFN[t].pfn;
            }

            DPRINT1("DirectX interface is activated\n");

        }
        /* return the status */
    }

    return Status;
}

/************************************************************************/
/* DirectX graphic/video driver loading cleanup ends here               */
/************************************************************************/

/************************************************************************/
/* NtGdiDdCreateDirectDrawObject                                        */
/************************************************************************/
HANDLE
STDCALL 
NtGdiDdCreateDirectDrawObject(HDC hdc)
{
    PGD_DDCREATEDIRECTDRAWOBJECT pfnDdCreateDirectDrawObject;
    NTSTATUS Status;
    PEPROCESS Proc = NULL;
    PDC pDC;

    if (hdc == NULL)
    {
        DPRINT1("Warning : hdc is NULL\n");
        return 0;
    }

    /* FIXME this code should be add where the driver being load */
    pDC = DC_LockDc(hdc);
    if (pDC == NULL)
    {
        DPRINT1("Warning : Failed to lock hdc\n");
        return 0;
    }

    /* FIXME This should be alloc for each drv and use it from each drv, not global for whole win32k */
    if (intEnableReactXDriver(&edd_DdirectDraw_Global, pDC) == FALSE)
    {
        DC_UnlockDc(pDC);
        DPRINT1("Warning : Failed to start the directx interface from the graphic driver\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DC_UnlockDc(pDC);

    /* FIXME get the process data */
    /* FIXME this code should be add where the driver being load */
    Status = DxDdStartupDxGraphics(0,NULL,0,NULL,NULL, Proc);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Warning : Failed to create the directx interface\n");
        return 0;
    }

    /* get the pfnDdCreateDirectDrawObject after we load the drv */
    pfnDdCreateDirectDrawObject = (PGD_DDCREATEDIRECTDRAWOBJECT)gpDxFuncs[DXG_INDEX_DxDdCreateDirectDrawObject].pfn;
  
    if (pfnDdCreateDirectDrawObject == NULL)
    {
        DPRINT1("Warning: no pfnDdCreateDirectDrawObject\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys DdCreateDirectDrawObject\n");

    return pfnDdCreateDirectDrawObject(hdc);
}

/*++
* @name NtGdiDxgGenericThunk
* @implemented
*
* The function NtGdiDxgGenericThunk redirects DirectX calls to another function.
* It redirects to dxg.sys in windows XP/2003, dxkrnl.sys in vista and is fully implemented in win32k.sys in windows 2000 and below
*
* @param ULONG_PTR ulIndex
* The functions we want to redirect
*
* @param ULONG_PTR ulHandle
* Unknown
*
* @param SIZE_T *pdwSizeOfPtr1
* Unknown
*
* @param PVOID pvPtr1
* Unknown
*
* @param SIZE_T *pdwSizeOfPtr2
* Unknown
*
* @param PVOID pvPtr2
* Unknown
*
* @return 
* Always returns DDHAL_DRIVER_NOTHANDLED
*
* @remarks.
* dxg.sys NtGdiDxgGenericThunk calls are redirected to dxg.sys
* This function is no longer used but is still present in Windows NT 2000/XP/2003.
*
*--*/
DWORD
STDCALL
NtGdiDxgGenericThunk(ULONG_PTR ulIndex,
                     ULONG_PTR ulHandle,
                     SIZE_T *pdwSizeOfPtr1,
                     PVOID pvPtr1,
                     SIZE_T *pdwSizeOfPtr2,
                     PVOID pvPtr2)
{
    PGD_DXGENERICTRUNK pfnDxgGenericThunk = (PGD_DXGENERICTRUNK)gpDxFuncs[DXG_INDEX_DxDxgGenericThunk].pfn;

    if (pfnDxgGenericThunk == NULL)
    {
        DPRINT1("Warning: no pfnDxgGenericThunk\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDxgGenericThunk\n");
    return pfnDxgGenericThunk(ulIndex, ulHandle, pdwSizeOfPtr1, pvPtr1, pdwSizeOfPtr2, pvPtr2);
}

/************************************************************************/
/* NtGdiDdGetDriverState                                                */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetDriverState(PDD_GETDRIVERSTATEDATA pdata)
{
    PGD_DDGETDRIVERSTATE pfnDdGetDriverState = (PGD_DDGETDRIVERSTATE)gpDxFuncs[DXG_INDEX_DxDdGetDriverState].pfn;

    if (pfnDdGetDriverState == NULL)
    {
        DPRINT1("Warning: no pfnDdGetDriverState\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys DdGetDriverState\n");
    return pfnDdGetDriverState(pdata);
}

/************************************************************************/
/* NtGdiDdColorControl                                                  */
/************************************************************************/
DWORD
STDCALL
NtGdiDdColorControl(HANDLE hSurface,
                    PDD_COLORCONTROLDATA puColorControlData)
{
    PGD_DDCOLORCONTROL pfnDdColorControl = (PGD_DDCOLORCONTROL)gpDxFuncs[DXG_INDEX_DxDdColorControl].pfn;
       
    if (pfnDdColorControl == NULL)
    {
        DPRINT1("Warning: no pfnDdColorControl\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys DdColorControl\n");
    return pfnDdColorControl(hSurface,puColorControlData);
}

/************************************************************************/
/* NtGdiDdCreateSurfaceObject                                           */
/************************************************************************/
HANDLE
STDCALL
NtGdiDdCreateSurfaceObject(HANDLE hDirectDrawLocal,
                           HANDLE hSurface,
                           PDD_SURFACE_LOCAL puSurfaceLocal,
                           PDD_SURFACE_MORE puSurfaceMore,
                           PDD_SURFACE_GLOBAL puSurfaceGlobal,
                           BOOL bComplete
)
{
    PGD_DXDDCREATESURFACEOBJECT pfnDdCreateSurfaceObject = (PGD_DXDDCREATESURFACEOBJECT)gpDxFuncs[DXG_INDEX_DxDdCreateSurfaceObject].pfn;
  
    if (pfnDdCreateSurfaceObject == NULL)
    {
        DPRINT1("Warning: no pfnDdCreateSurfaceObject\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdCreateSurfaceObject\n");
    return pfnDdCreateSurfaceObject(hDirectDrawLocal, hSurface, puSurfaceLocal, puSurfaceMore, puSurfaceGlobal, bComplete);
}

/************************************************************************/
/* NtGdiDdDeleteDirectDrawObject                                        */
/************************************************************************/
BOOL
STDCALL 
NtGdiDdDeleteDirectDrawObject(HANDLE hDirectDrawLocal)
{
    PGD_DXDDDELETEDIRECTDRAWOBJECT pfnDdDeleteDirectDrawObject = (PGD_DXDDDELETEDIRECTDRAWOBJECT)gpDxFuncs[DXG_INDEX_DxDdDeleteDirectDrawObject].pfn;

    if (pfnDdDeleteDirectDrawObject == NULL)
    {
        DPRINT1("Warning: no pfnDdDeleteDirectDrawObject\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    if (hDirectDrawLocal == NULL)
    {
         DPRINT1("Warning: hDirectDrawLocal is NULL\n");
         return DDHAL_DRIVER_HANDLED;
    }

    DPRINT1("hDirectDrawLocal = %lx \n",hDirectDrawLocal);
    DPRINT1("Calling dxg.sys pfnDdDeleteDirectDrawObject\n");

    return pfnDdDeleteDirectDrawObject(hDirectDrawLocal);
}

/************************************************************************/
/* NtGdiDdDeleteSurfaceObject                                           */
/************************************************************************/
BOOL
STDCALL
NtGdiDdDeleteSurfaceObject(HANDLE hSurface)
{
    PGD_DXDDDELETESURFACEOBJECT pfnDdDeleteSurfaceObject = (PGD_DXDDDELETESURFACEOBJECT)gpDxFuncs[DXG_INDEX_DxDdDeleteSurfaceObject].pfn;

    if (pfnDdDeleteSurfaceObject == NULL)
    {
        DPRINT1("Warning: no pfnDdDeleteSurfaceObject\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }
    /* try see if the handle is vaidl */

    DPRINT1("Calling dxg.sys DdDeleteSurfaceObject\n");
    return pfnDdDeleteSurfaceObject(hSurface);
}

/************************************************************************/
/* NtGdiDdQueryDirectDrawObject                                         */
/************************************************************************/
BOOL
STDCALL 
NtGdiDdQueryDirectDrawObject(HANDLE hDirectDrawLocal,
                             DD_HALINFO  *pHalInfo,
                             DWORD *pCallBackFlags,
                             LPD3DNTHAL_CALLBACKS puD3dCallbacks,
                             LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
                             PDD_D3DBUFCALLBACKS puD3dBufferCallbacks,
                             LPDDSURFACEDESC puD3dTextureFormats,
                             DWORD *puNumHeaps,
                             VIDEOMEMORY *puvmList,
                             DWORD *puNumFourCC,
                             DWORD *puFourCC)
{
    PGD_DXDDQUERYDIRECTDRAWOBJECT pfnDdQueryDirectDrawObject = (PGD_DXDDQUERYDIRECTDRAWOBJECT)gpDxFuncs[DXG_INDEX_DxDdQueryDirectDrawObject].pfn;
   
    if (pfnDdQueryDirectDrawObject == NULL)
    {
        DPRINT1("Warning: no pfnDdQueryDirectDrawObject\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdQueryDirectDrawObject\n");

    dump_edd_directdraw_global(&edd_DdirectDraw_Global);
    dump_edd_directdraw_local(edd_DdirectDraw_Global.peDirectDrawLocalList);

    return pfnDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, 
                                      puD3dBufferCallbacks, puD3dTextureFormats, puNumHeaps, puvmList, puNumFourCC, puFourCC);
}


/************************************************************************/
/* NtGdiDdReenableDirectDrawObject                                      */
/************************************************************************/
BOOL
STDCALL
NtGdiDdReenableDirectDrawObject(HANDLE hDirectDrawLocal,
                                BOOL *pubNewMode)
{
    PGD_DXDDREENABLEDIRECTDRAWOBJECT pfnDdReenableDirectDrawObject = (PGD_DXDDREENABLEDIRECTDRAWOBJECT)gpDxFuncs[DXG_INDEX_DxDdReenableDirectDrawObject].pfn;
  
    if (pfnDdReenableDirectDrawObject == NULL)
    {
        DPRINT1("Warning: no pfnDdReenableDirectDrawObject\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdReenableDirectDrawObject\n");
    return pfnDdReenableDirectDrawObject(hDirectDrawLocal, pubNewMode);
}


/************************************************************************/
/* NtGdiDdGetDriverInfo                                                 */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetDriverInfo(HANDLE hDirectDrawLocal,
                     PDD_GETDRIVERINFODATA puGetDriverInfoData)

{
    PGD_DXDDGETDRIVERINFO pfnDdGetDriverInfo = (PGD_DXDDGETDRIVERINFO)gpDxFuncs[DXG_INDEX_DxDdGetDriverInfo].pfn;
    
    if (pfnDdGetDriverInfo == NULL)
    {
        DPRINT1("Warning: no pfnDdGetDriverInfo\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdGetDriverInfo\n");
    return pfnDdGetDriverInfo(hDirectDrawLocal, puGetDriverInfoData);
}


/************************************************************************/
/* NtGdiDdGetAvailDriverMemory                                          */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetAvailDriverMemory(HANDLE hDirectDrawLocal,
                            PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData)
{
    PGD_DXDDGETAVAILDRIVERMEMORY pfnDdGetAvailDriverMemory = (PGD_DXDDGETAVAILDRIVERMEMORY)gpDxFuncs[DXG_INDEX_DxDdGetAvailDriverMemory].pfn;
   
    if (pfnDdGetAvailDriverMemory == NULL)
    {
        DPRINT1("Warning: no pfnDdGetAvailDriverMemory\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdGetAvailDriverMemory\n");
    return pfnDdGetAvailDriverMemory(hDirectDrawLocal, puGetAvailDriverMemoryData);
}


/************************************************************************/
/* NtGdiDdSetExclusiveMode                                              */
/************************************************************************/

DWORD
STDCALL
NtGdiDdSetExclusiveMode(HANDLE hDirectDraw,
                        PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData)
{
    PGD_DXDDSETEXCLUSIVEMODE pfnDdSetExclusiveMode = (PGD_DXDDSETEXCLUSIVEMODE)gpDxFuncs[DXG_INDEX_DxDdSetExclusiveMode].pfn;
      
    if (pfnDdSetExclusiveMode == NULL)
    {
        DPRINT1("Warning: no pfnDdSetExclusiveMode\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdSetExclusiveMode\n");
    return pfnDdSetExclusiveMode(hDirectDraw, puSetExclusiveModeData);

}


/************************************************************************/
/* NtGdiDdFlipToGDISurface                                              */
/************************************************************************/
DWORD
STDCALL
NtGdiDdFlipToGDISurface(HANDLE hDirectDraw,
                        PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData)
{
    PGD_DXDDFLIPTOGDISURFACE pfnDdFlipToGDISurface = (PGD_DXDDFLIPTOGDISURFACE)gpDxFuncs[DXG_INDEX_DxDdFlipToGDISurface].pfn;
   
    if (pfnDdFlipToGDISurface == NULL)
    {
        DPRINT1("Warning: no pfnDdFlipToGDISurface\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdFlipToGDISurface\n");
    return pfnDdFlipToGDISurface(hDirectDraw, puFlipToGDISurfaceData);

}

/************************************************************************/
/* NtGdiDdGetDC                                                         */
/************************************************************************/
HDC
STDCALL
NtGdiDdGetDC(HANDLE hSurface,
             PALETTEENTRY *puColorTable)
{
    PGD_DDGETDC pfnDdGetDC = (PGD_DDGETDC)gpDxFuncs[DXG_INDEX_DxDdGetDC].pfn;
    
    if (pfnDdGetDC == NULL)
    {
        DPRINT1("Warning: no pfnDdGetDC\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdGetDC\n");
    return pfnDdGetDC(hSurface, puColorTable);
}

/************************************************************************/
/* NtGdiDdGetDxHandle                                                   */
/************************************************************************/
HANDLE
STDCALL
NtGdiDdGetDxHandle(HANDLE hDirectDraw,
                   HANDLE hSurface,
                   BOOL bRelease)
{    
    PGD_DDGETDXHANDLE pfnDdGetDxHandle = (PGD_DDGETDXHANDLE)gpDxFuncs[DXG_INDEX_DxDdGetDxHandle].pfn;
    
    if (pfnDdGetDxHandle == NULL)
    {
        DPRINT1("Warning: no pfnDdGetDxHandle\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdGetDxHandle\n");
    return pfnDdGetDxHandle(hDirectDraw, hSurface, bRelease);
}


/************************************************************************/
/* NtGdiDdReleaseDC                                                     */
/************************************************************************/
BOOL
STDCALL
NtGdiDdReleaseDC(HANDLE hSurface)
{
    PGD_DDRELEASEDC pfnDdReleaseDC = (PGD_DDRELEASEDC)gpDxFuncs[DXG_INDEX_DxDdReleaseDC].pfn;
   
    if (pfnDdReleaseDC == NULL)
    {
        DPRINT1("Warning: no pfnDdReleaseDC\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdReleaseDC\n");
    return pfnDdReleaseDC(hSurface);
}

/************************************************************************/
/* NtGdiDdResetVisrgn                                                   */
/************************************************************************/
BOOL
STDCALL
NtGdiDdResetVisrgn(HANDLE hSurface,
                   HWND hwnd)
{

    PGD_DDRESTVISRGN pfnDdResetVisrgn = (PGD_DDRESTVISRGN)gpDxFuncs[DXG_INDEX_DxDdResetVisrgn].pfn;
   
    if (pfnDdResetVisrgn == NULL)
    {
        DPRINT1("Warning: no pfnDdResetVisrgn\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdResetVisrgn\n");
    return pfnDdResetVisrgn(hSurface, hwnd);
}

/************************************************************************/
/* NtGdiDdSetGammaRamp                                                  */
/************************************************************************/
BOOL
STDCALL
NtGdiDdSetGammaRamp(HANDLE hDirectDraw,
                    HDC hdc,
                    LPVOID lpGammaRamp)
{
    PGD_DDSETGAMMARAMP pfnDdSetGammaRamp = (PGD_DDSETGAMMARAMP)gpDxFuncs[DXG_INDEX_DxDdSetGammaRamp].pfn;
   
    if (pfnDdSetGammaRamp == NULL)
    {
        DPRINT1("Warning: no pfnDdSetGammaRamp\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdSetGammaRamp\n");
    return pfnDdSetGammaRamp(hDirectDraw, hdc, lpGammaRamp);
}


/* internal debug api */
void dump_edd_directdraw_global(EDD_DIRECTDRAW_GLOBAL *pEddgbl)
{
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dhpdev                                         : 0x%08lx\n",(((DWORD)&pEddgbl->dhpdev) - (DWORD)pEddgbl), pEddgbl->dhpdev);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dwReserved1                                    : 0x%08lx\n",(((DWORD)&pEddgbl->dwReserved1) - (DWORD)pEddgbl),pEddgbl->dwReserved1);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dwReserved2                                    : 0x%08lx\n",(((DWORD)&pEddgbl->dwReserved2) - (DWORD)pEddgbl),pEddgbl->dwReserved2);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_000c[0]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_000c[0]) - (DWORD)pEddgbl),pEddgbl->unk_000c[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_000c[1]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_000c[1]) - (DWORD)pEddgbl),pEddgbl->unk_000c[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_000c[2]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_000c[2]) - (DWORD)pEddgbl),pEddgbl->unk_000c[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->cDriverReferences                              : 0x%08lx\n",(((DWORD)&pEddgbl->cDriverReferences) - (DWORD)pEddgbl),pEddgbl->cDriverReferences);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_01c                                        : 0x%08lx\n",(((DWORD)&pEddgbl->unk_01c) - (DWORD)pEddgbl),pEddgbl->unk_01c);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dwCallbackFlags                                : 0x%08lx\n",(((DWORD)&pEddgbl->dwCallbackFlags) - (DWORD)pEddgbl),pEddgbl->dwCallbackFlags);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_024                                        : 0x%08lx\n",(((DWORD)&pEddgbl->unk_024) - (DWORD)pEddgbl),pEddgbl->unk_024);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->llAssertModeTimeout                            : 0x%x\n",(((DWORD)&pEddgbl->llAssertModeTimeout) - (DWORD)pEddgbl),pEddgbl->llAssertModeTimeout);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dwNumHeaps                                     : 0x%08lx\n",(((DWORD)&pEddgbl->dwNumHeaps) - (DWORD)pEddgbl),pEddgbl->dwNumHeaps);
    // VIDEOMEMORY *pvmList;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->pvmList                                        : 0x%08lx\n",(((DWORD)&pEddgbl->pvmList) - (DWORD)pEddgbl),pEddgbl->pvmList);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dwNumFourCC                                    : 0x%08lx\n",(((DWORD)&pEddgbl->dwNumFourCC) - (DWORD)pEddgbl),pEddgbl->dwNumFourCC);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->pdwFourCC                                      : 0x%08lx\n",(((DWORD)&pEddgbl->pdwFourCC) - (DWORD)pEddgbl),pEddgbl->pdwFourCC);
    // DD_HALINFO ddHalInfo;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddHalInfo                                      : 0x%08lx\n",(((DWORD)&pEddgbl->ddHalInfo) - (DWORD)pEddgbl),pEddgbl->ddHalInfo);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[0]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[0]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[1]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[1]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[2]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[2]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[3]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[3]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[3]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[4]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[4]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[4]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[5]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[5]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[5]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[6]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[6]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[6]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[7]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[7]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[7]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[8]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[8]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[8]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[9]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[9]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[9]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[10]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[10]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[10]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[11]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[11]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[11]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[12]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[12]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[12]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[13]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[13]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[13]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[14]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[14]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[14]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[15]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[15]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[15]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[16]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[16]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[16]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[17]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[17]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[17]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[18]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[18]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[18]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[19]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[19]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[19]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[20]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[20]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[20]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[21]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[21]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[21]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[22]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[22]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[22]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[23]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[23]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[23]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[24]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[24]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[24]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[25]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[25]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[25]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[26]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[26]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[26]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[27]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[27]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[27]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[28]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[28]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[28]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[29]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[29]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[29]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[30]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[30]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[30]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[31]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[31]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[31]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[32]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[32]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[32]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[33]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[33]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[33]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[34]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[34]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[34]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[35]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[35]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[35]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[36]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[36]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[36]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[37]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[37]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[37]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[38]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[38]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[38]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[39]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[39]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[39]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[40]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[40]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[41]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[41]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[41]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[42]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[42]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[42]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[43]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[43]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[43]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[44]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[44]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[44]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[45]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_1e0[45]) - (DWORD)pEddgbl),pEddgbl->unk_1e0[45]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.dwSize                             : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.dwSize) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.dwSize);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.dwFlags                            : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.dwFlags) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.dwFlags);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.DestroyDriver                      : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.DestroyDriver) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.DestroyDriver);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.CreateSurface                      : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.CreateSurface) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.CreateSurface);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.SetColorKey                        : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.SetColorKey) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.SetColorKey);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.SetMode                            : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.SetMode) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.SetMode);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.WaitForVerticalBlank               : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.WaitForVerticalBlank) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.WaitForVerticalBlank);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.CanCreateSurface                   : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.CanCreateSurface) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.CanCreateSurface);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.CreatePalette                      : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.CreatePalette) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.CreatePalette);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.GetScanLine                        : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.GetScanLine) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.GetScanLine);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.MapMemory                          : 0x%08lx\n",(((DWORD)&pEddgbl->ddCallbacks.MapMemory) - (DWORD)pEddgbl),pEddgbl->ddCallbacks.MapMemory);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.dwSize                      : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.dwSize) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.dwSize);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.dwFlags                     : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.dwFlags) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.dwFlags);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.DestroySurface              : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.DestroySurface) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.DestroySurface);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.Flip                        : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.Flip) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.Flip);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.SetClipList                 : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.SetClipList) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.SetClipList);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.Lock                        : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.Lock) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.Lock);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.Unlock                      : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.Unlock) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.Unlock);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.Blt                         : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.Blt) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.Blt);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.SetColorKey                 : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.SetColorKey) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.SetColorKey);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.AddAttachedSurface          : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.AddAttachedSurface) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.AddAttachedSurface);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.GetBltStatus                : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.GetBltStatus) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.GetBltStatus);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.GetFlipStatus               : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.GetFlipStatus) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.GetFlipStatus);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.UpdateOverlay               : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.UpdateOverlay) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.UpdateOverlay);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.SetOverlayPosition          : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.SetOverlayPosition) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.SetOverlayPosition);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.reserved4                   : 0x%08lx\n",(((DWORD)&pEddgbl->ddSurfaceCallbacks.reserved4) - (DWORD)pEddgbl),pEddgbl->ddSurfaceCallbacks.reserved4);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks.dwSize                      : 0x%08lx\n",(((DWORD)&pEddgbl->ddPaletteCallbacks.dwSize) - (DWORD)pEddgbl),pEddgbl->ddPaletteCallbacks.dwSize);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks.dwFlags                     : 0x%08lx\n",(((DWORD)&pEddgbl->ddPaletteCallbacks.dwFlags) - (DWORD)pEddgbl),pEddgbl->ddPaletteCallbacks.dwFlags);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks.DestroyPalette              : 0x%08lx\n",(((DWORD)&pEddgbl->ddPaletteCallbacks.DestroyPalette) - (DWORD)pEddgbl),pEddgbl->ddPaletteCallbacks.DestroyPalette);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks.SetEntries                  : 0x%08lx\n",(((DWORD)&pEddgbl->ddPaletteCallbacks.SetEntries) - (DWORD)pEddgbl),pEddgbl->ddPaletteCallbacks.SetEntries);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[0]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[0]) - (DWORD)pEddgbl),pEddgbl->unk_314[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[1]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[1]) - (DWORD)pEddgbl),pEddgbl->unk_314[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[2]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[2]) - (DWORD)pEddgbl),pEddgbl->unk_314[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[3]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[3]) - (DWORD)pEddgbl),pEddgbl->unk_314[3]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[4]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[4]) - (DWORD)pEddgbl),pEddgbl->unk_314[4]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[5]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[5]) - (DWORD)pEddgbl),pEddgbl->unk_314[5]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[6]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[6]) - (DWORD)pEddgbl),pEddgbl->unk_314[6]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[7]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[7]) - (DWORD)pEddgbl),pEddgbl->unk_314[7]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[8]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[8]) - (DWORD)pEddgbl),pEddgbl->unk_314[8]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[9]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[9]) - (DWORD)pEddgbl),pEddgbl->unk_314[9]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[10]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[10]) - (DWORD)pEddgbl),pEddgbl->unk_314[10]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[11]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[11]) - (DWORD)pEddgbl),pEddgbl->unk_314[11]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[12]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[12]) - (DWORD)pEddgbl),pEddgbl->unk_314[12]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[13]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[13]) - (DWORD)pEddgbl),pEddgbl->unk_314[13]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[14]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[14]) - (DWORD)pEddgbl),pEddgbl->unk_314[14]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[15]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[15]) - (DWORD)pEddgbl),pEddgbl->unk_314[15]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[16]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[16]) - (DWORD)pEddgbl),pEddgbl->unk_314[16]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[17]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[17]) - (DWORD)pEddgbl),pEddgbl->unk_314[17]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[18]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[18]) - (DWORD)pEddgbl),pEddgbl->unk_314[18]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[19]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[19]) - (DWORD)pEddgbl),pEddgbl->unk_314[19]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[20]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[20]) - (DWORD)pEddgbl),pEddgbl->unk_314[20]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[21]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[21]) - (DWORD)pEddgbl),pEddgbl->unk_314[21]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[22]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[22]) - (DWORD)pEddgbl),pEddgbl->unk_314[22]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[23]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[23]) - (DWORD)pEddgbl),pEddgbl->unk_314[23]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[24]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[24]) - (DWORD)pEddgbl),pEddgbl->unk_314[24]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[25]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[25]) - (DWORD)pEddgbl),pEddgbl->unk_314[25]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[26]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[26]) - (DWORD)pEddgbl),pEddgbl->unk_314[26]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[27]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[27]) - (DWORD)pEddgbl),pEddgbl->unk_314[27]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[28]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[28]) - (DWORD)pEddgbl),pEddgbl->unk_314[28]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[29]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[29]) - (DWORD)pEddgbl),pEddgbl->unk_314[29]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[30]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[30]) - (DWORD)pEddgbl),pEddgbl->unk_314[30]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[31]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[31]) - (DWORD)pEddgbl),pEddgbl->unk_314[31]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[32]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[32]) - (DWORD)pEddgbl),pEddgbl->unk_314[32]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[33]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[33]) - (DWORD)pEddgbl),pEddgbl->unk_314[33]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[34]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[34]) - (DWORD)pEddgbl),pEddgbl->unk_314[34]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[35]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[35]) - (DWORD)pEddgbl),pEddgbl->unk_314[35]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[36]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[36]) - (DWORD)pEddgbl),pEddgbl->unk_314[36]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[37]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[37]) - (DWORD)pEddgbl),pEddgbl->unk_314[37]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[38]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[38]) - (DWORD)pEddgbl),pEddgbl->unk_314[38]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[39]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[39]) - (DWORD)pEddgbl),pEddgbl->unk_314[39]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[40]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[40]) - (DWORD)pEddgbl),pEddgbl->unk_314[40]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[41]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[41]) - (DWORD)pEddgbl),pEddgbl->unk_314[41]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[42]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[42]) - (DWORD)pEddgbl),pEddgbl->unk_314[42]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[43]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[43]) - (DWORD)pEddgbl),pEddgbl->unk_314[43]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[44]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[44]) - (DWORD)pEddgbl),pEddgbl->unk_314[44]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_314[45]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_314[45]) - (DWORD)pEddgbl),pEddgbl->unk_314[45]);
    // D3DNTHAL_CALLBACKS d3dNtHalCallbacks;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks                              : 0x%08lx\n",(((DWORD)&pEddgbl->d3dNtHalCallbacks) - (DWORD)pEddgbl),pEddgbl->d3dNtHalCallbacks);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_460[0]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_460[0]) - (DWORD)pEddgbl),pEddgbl->unk_460[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_460[1]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_460[1]) - (DWORD)pEddgbl),pEddgbl->unk_460[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_460[2]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_460[2]) - (DWORD)pEddgbl),pEddgbl->unk_460[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_460[3]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_460[3]) - (DWORD)pEddgbl),pEddgbl->unk_460[3]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_460[4]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_460[4]) - (DWORD)pEddgbl),pEddgbl->unk_460[4]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_460[5]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_460[5]) - (DWORD)pEddgbl),pEddgbl->unk_460[5]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_460[6]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_460[6]) - (DWORD)pEddgbl),pEddgbl->unk_460[6]);
    // D3DNTHAL_CALLBACKS2 d3dNtHalCallbacks2;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks2                             : 0x%08lx\n",(((DWORD)&pEddgbl->d3dNtHalCallbacks2) - (DWORD)pEddgbl),pEddgbl->d3dNtHalCallbacks2);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.dwSize                     : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.dwSize) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.dwSize);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.dwFlags                    : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.dwFlags) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.dwFlags);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.CanCreateVideoPort         : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.CanCreateVideoPort) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.CanCreateVideoPort);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.CreateVideoPort            : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.CreateVideoPort) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.CreateVideoPort);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.FlipVideoPort              : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.FlipVideoPort) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.FlipVideoPort);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortBandwidth      : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.GetVideoPortBandwidth) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.GetVideoPortBandwidth);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortInputFormats   : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.GetVideoPortInputFormats) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.GetVideoPortInputFormats);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortOutputFormats  : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.GetVideoPortOutputFormats) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.GetVideoPortOutputFormats);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.lpReserved1                : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.lpReserved1) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.lpReserved1);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortField          : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.GetVideoPortField) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.GetVideoPortField);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortLine           : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.GetVideoPortLine) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.GetVideoPortLine);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortConnectInfo    : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.GetVideoPortConnectInfo) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.GetVideoPortConnectInfo);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.DestroyVideoPort           : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.DestroyVideoPort) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.DestroyVideoPort);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortFlipStatus     : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.GetVideoPortFlipStatus) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.GetVideoPortFlipStatus);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.UpdateVideoPort            : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.UpdateVideoPort) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.UpdateVideoPort);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.WaitForVideoPortSync       : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.WaitForVideoPortSync) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.WaitForVideoPortSync);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoSignalStatus       : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.GetVideoSignalStatus) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.GetVideoSignalStatus);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.ColorControl               : 0x%08lx\n",(((DWORD)&pEddgbl->ddVideoPortCallback.ColorControl) - (DWORD)pEddgbl),pEddgbl->ddVideoPortCallback.ColorControl);

    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddMiscellanousCallbacks.dwSize                 : 0x%08lx\n",(((DWORD)&pEddgbl->ddMiscellanousCallbacks.dwSize) - (DWORD)pEddgbl),pEddgbl->ddMiscellanousCallbacks.dwSize);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddMiscellanousCallbacks.dwFlags                : 0x%08lx\n",(((DWORD)&pEddgbl->ddMiscellanousCallbacks.dwFlags) - (DWORD)pEddgbl),pEddgbl->ddMiscellanousCallbacks.dwFlags);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddMiscellanousCallbacks.GetAvailDriverMemory   : 0x%08lx\n",(((DWORD)&pEddgbl->ddMiscellanousCallbacks.GetAvailDriverMemory) - (DWORD)pEddgbl),pEddgbl->ddMiscellanousCallbacks.GetAvailDriverMemory);

    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[0]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[0]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[1]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[1]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[2]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[2]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[3]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[3]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[3]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[4]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[4]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[4]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[5]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[5]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[5]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[6]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[6]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[6]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[7]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[7]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[7]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[8]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[8]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[8]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[9]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[9]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[9]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[10]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[10]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[10]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[11]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[11]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[11]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[12]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[12]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[12]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[13]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[13]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[13]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[14]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[14]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[14]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[15]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[15]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[15]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[16]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[16]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[16]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_4ec[17]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_4ec[17]) - (DWORD)pEddgbl),pEddgbl->unk_4ec[17]);
    // D3DNTHAL_CALLBACKS3 d3dNtHalCallbacks3;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks3                             : 0x%08lx\n",(((DWORD)&pEddgbl->d3dNtHalCallbacks3) - (DWORD)pEddgbl),pEddgbl->d3dNtHalCallbacks3);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_544                                        : 0x%08lx\n",(((DWORD)&pEddgbl->unk_544) - (DWORD)pEddgbl), pEddgbl->unk_544);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_548                                        : 0x%08lx\n",(((DWORD)&pEddgbl->unk_548) - (DWORD)pEddgbl), pEddgbl->unk_548);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[0]                                     : 0x%08lx\n",( ( (DWORD)&pEddgbl->unk_54c[0] ) - (DWORD)pEddgbl),pEddgbl->unk_54c[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[1]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[1]) - (DWORD)pEddgbl),pEddgbl->unk_54c[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[2]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[2]) - (DWORD)pEddgbl),pEddgbl->unk_54c[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[3]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[3]) - (DWORD)pEddgbl),pEddgbl->unk_54c[3]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[4]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[4]) - (DWORD)pEddgbl),pEddgbl->unk_54c[4]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[5]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[5]) - (DWORD)pEddgbl),pEddgbl->unk_54c[5]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[6]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[6]) - (DWORD)pEddgbl),pEddgbl->unk_54c[6]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[7]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[7]) - (DWORD)pEddgbl),pEddgbl->unk_54c[7]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[8]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[8]) - (DWORD)pEddgbl),pEddgbl->unk_54c[8]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[9]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[9]) - (DWORD)pEddgbl),pEddgbl->unk_54c[9]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[10]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[10]) - (DWORD)pEddgbl),pEddgbl->unk_54c[10]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[11]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[11]) - (DWORD)pEddgbl),pEddgbl->unk_54c[11]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[12]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[12]) - (DWORD)pEddgbl),pEddgbl->unk_54c[12]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[13]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[13]) - (DWORD)pEddgbl),pEddgbl->unk_54c[13]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[14]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[14]) - (DWORD)pEddgbl),pEddgbl->unk_54c[14]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[15]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[15]) - (DWORD)pEddgbl),pEddgbl->unk_54c[15]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[16]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[16]) - (DWORD)pEddgbl),pEddgbl->unk_54c[16]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[17]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[17]) - (DWORD)pEddgbl),pEddgbl->unk_54c[17]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[18]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[18]) - (DWORD)pEddgbl),pEddgbl->unk_54c[18]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[19]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[19]) - (DWORD)pEddgbl),pEddgbl->unk_54c[19]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[20]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[20]) - (DWORD)pEddgbl),pEddgbl->unk_54c[20]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[21]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[21]) - (DWORD)pEddgbl),pEddgbl->unk_54c[21]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_54c[22]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_54c[22]) - (DWORD)pEddgbl),pEddgbl->unk_54c[22]);    
    // EDD_DIRECTDRAW_LOCAL* peDirectDrawLocalList;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->peDirectDrawLocalList                          : 0x%08lx\n",(((DWORD)&pEddgbl->peDirectDrawLocalList) - (DWORD)pEddgbl), pEddgbl->peDirectDrawLocalList);
    // EDD_SURFACE* peSurface_LockList;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->peSurface_LockList                             : 0x%08lx\n",(((DWORD)&pEddgbl->peSurface_LockList) - (DWORD)pEddgbl), pEddgbl->peSurface_LockList);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->fl                                             : 0x%08lx\n",(((DWORD)&pEddgbl->fl) - (DWORD)pEddgbl), pEddgbl->fl);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->cSurfaceLocks                                  : 0x%08lx\n",(((DWORD)&pEddgbl->cSurfaceLocks) - (DWORD)pEddgbl), pEddgbl->cSurfaceLocks);
    // PKEVENT pAssertModeEvent;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->pAssertModeEvent                               : 0x%08lx\n",(((DWORD)&pEddgbl->pAssertModeEvent) - (DWORD)pEddgbl), pEddgbl->pAssertModeEvent);
    // EDD_SURFACE *peSurfaceCurrent;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->peSurfaceCurrent                               : 0x%08lx\n",(((DWORD)&pEddgbl->peSurfaceCurrent) - (DWORD)pEddgbl), pEddgbl->peSurfaceCurrent);
    // EDD_SURFACE *peSurfacePrimary;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->peSurfacePrimary                               : 0x%08lx\n",(((DWORD)&pEddgbl->peSurfacePrimary) - (DWORD)pEddgbl),pEddgbl->peSurfacePrimary);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->bSuspended                                     : 0x%08lx\n",(((DWORD)&pEddgbl->bSuspended) - (DWORD)pEddgbl),pEddgbl->bSuspended);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[0]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[0]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[1]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[1]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[2]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[2]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[3]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[3]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[3]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[4]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[4]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[4]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[5]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[5]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[5]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[6]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[6]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[6]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[7]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[7]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[7]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[8]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[8]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[8]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[9]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[9]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[9]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[10]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[10]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[10]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5c8[11]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5c8[11]) - (DWORD)pEddgbl),pEddgbl->unk_5c8[11]);
    // RECTL rcbounds;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->rcbounds                                       : 0x%08lx\n",(((DWORD)&pEddgbl->rcbounds) - (DWORD)pEddgbl),pEddgbl->rcbounds);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_608                                        : 0x%08lx\n",(((DWORD)&pEddgbl->unk_608) - (DWORD)pEddgbl), pEddgbl->unk_608);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->hDev                                           : 0x%08lx\n",(((DWORD)&pEddgbl->hDev) - (DWORD)pEddgbl), pEddgbl->hDev);

    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks                             : 0x%08lx\n",(((DWORD)&pEddgbl->ddPaletteCallbacks) - (DWORD)pEddgbl), pEddgbl->ddPaletteCallbacks);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[0]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[0]) - (DWORD)pEddgbl), pEddgbl->unk_610[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[1]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[1]) - (DWORD)pEddgbl), pEddgbl->unk_610[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[2]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[2]) - (DWORD)pEddgbl), pEddgbl->unk_610[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[3]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[3]) - (DWORD)pEddgbl), pEddgbl->unk_610[3]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[4]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[4]) - (DWORD)pEddgbl), pEddgbl->unk_610[4]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[5]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[5]) - (DWORD)pEddgbl), pEddgbl->unk_610[5]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[6]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[6]) - (DWORD)pEddgbl), pEddgbl->unk_610[6]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[7]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[7]) - (DWORD)pEddgbl), pEddgbl->unk_610[7]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[8]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[8]) - (DWORD)pEddgbl), pEddgbl->unk_610[8]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[9]                                     : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[9]) - (DWORD)pEddgbl), pEddgbl->unk_610[9]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[10]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[10]) - (DWORD)pEddgbl), pEddgbl->unk_610[10]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[11]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[11]) - (DWORD)pEddgbl), pEddgbl->unk_610[11]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[12]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[12]) - (DWORD)pEddgbl), pEddgbl->unk_610[12]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[13]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[13]) - (DWORD)pEddgbl), pEddgbl->unk_610[13]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[14]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[14]) - (DWORD)pEddgbl), pEddgbl->unk_610[14]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[15]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[15]) - (DWORD)pEddgbl), pEddgbl->unk_610[15]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[16]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[16]) - (DWORD)pEddgbl), pEddgbl->unk_610[16]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[17]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[17]) - (DWORD)pEddgbl), pEddgbl->unk_610[17]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[18]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[18]) - (DWORD)pEddgbl), pEddgbl->unk_610[18]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[19]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[19]) - (DWORD)pEddgbl), pEddgbl->unk_610[19]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[20]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[20]) - (DWORD)pEddgbl), pEddgbl->unk_610[20]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[21]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[21]) - (DWORD)pEddgbl), pEddgbl->unk_610[21]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[22]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[22]) - (DWORD)pEddgbl), pEddgbl->unk_610[22]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[23]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[23]) - (DWORD)pEddgbl), pEddgbl->unk_610[23]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[24]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[24]) - (DWORD)pEddgbl), pEddgbl->unk_610[24]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[25]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[25]) - (DWORD)pEddgbl), pEddgbl->unk_610[25]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[26]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[26]) - (DWORD)pEddgbl), pEddgbl->unk_610[26]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[27]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[27]) - (DWORD)pEddgbl), pEddgbl->unk_610[27]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[28]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[28]) - (DWORD)pEddgbl), pEddgbl->unk_610[28]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[29]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[29]) - (DWORD)pEddgbl), pEddgbl->unk_610[29]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[30]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[30]) - (DWORD)pEddgbl), pEddgbl->unk_610[30]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[31]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[31]) - (DWORD)pEddgbl), pEddgbl->unk_610[31]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[32]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[32]) - (DWORD)pEddgbl), pEddgbl->unk_610[32]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[33]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[33]) - (DWORD)pEddgbl), pEddgbl->unk_610[33]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[34]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[34]) - (DWORD)pEddgbl), pEddgbl->unk_610[34]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[35]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[35]) - (DWORD)pEddgbl), pEddgbl->unk_610[35]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[36]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[36]) - (DWORD)pEddgbl), pEddgbl->unk_610[36]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[37]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[37]) - (DWORD)pEddgbl), pEddgbl->unk_610[37]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[38]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[38]) - (DWORD)pEddgbl), pEddgbl->unk_610[38]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[39]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[39]) - (DWORD)pEddgbl), pEddgbl->unk_610[39]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[40]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[40]) - (DWORD)pEddgbl), pEddgbl->unk_610[40]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[41]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[41]) - (DWORD)pEddgbl), pEddgbl->unk_610[41]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[42]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[42]) - (DWORD)pEddgbl), pEddgbl->unk_610[42]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[43]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[43]) - (DWORD)pEddgbl), pEddgbl->unk_610[43]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[44]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[44]) - (DWORD)pEddgbl), pEddgbl->unk_610[44]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[45]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[45]) - (DWORD)pEddgbl), pEddgbl->unk_610[45]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[46]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[46]) - (DWORD)pEddgbl), pEddgbl->unk_610[46]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[47]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[47]) - (DWORD)pEddgbl), pEddgbl->unk_610[47]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[48]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[48]) - (DWORD)pEddgbl), pEddgbl->unk_610[48]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[49]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[49]) - (DWORD)pEddgbl), pEddgbl->unk_610[49]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[50]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[50]) - (DWORD)pEddgbl), pEddgbl->unk_610[50]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[51]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[51]) - (DWORD)pEddgbl), pEddgbl->unk_610[51]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[52]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[52]) - (DWORD)pEddgbl), pEddgbl->unk_610[52]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[53]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[53]) - (DWORD)pEddgbl), pEddgbl->unk_610[53]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[54]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[54]) - (DWORD)pEddgbl), pEddgbl->unk_610[54]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[55]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[55]) - (DWORD)pEddgbl), pEddgbl->unk_610[55]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[56]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[56]) - (DWORD)pEddgbl), pEddgbl->unk_610[56]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[57]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[57]) - (DWORD)pEddgbl), pEddgbl->unk_610[57]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[58]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[58]) - (DWORD)pEddgbl), pEddgbl->unk_610[58]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[59]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[59]) - (DWORD)pEddgbl), pEddgbl->unk_610[59]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_1e0[60]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[60]) - (DWORD)pEddgbl), pEddgbl->unk_610[60]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[61]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[61]) - (DWORD)pEddgbl), pEddgbl->unk_610[61]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_610[62]                                    : 0x%08lx\n",(((DWORD)&pEddgbl->unk_610[62]) - (DWORD)pEddgbl), pEddgbl->unk_610[62]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_70C                                        : 0x%08lx\n",(((DWORD)&pEddgbl->unk_70C) - (DWORD)pEddgbl), pEddgbl->unk_70C);
}

void dump_edd_directdraw_local(PEDD_DIRECTDRAW_LOCAL pEddlcl)
{
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->pobj                     : 0x%08lx\n",(((DWORD)&pEddlcl->pobj) - (DWORD)pEddlcl), pEddlcl->pobj);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->peDirectDrawGlobal       : 0x%08lx\n",(((DWORD)&pEddlcl->peDirectDrawGlobal) - (DWORD)pEddlcl), pEddlcl->peDirectDrawGlobal);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->peSurface_DdList         : 0x%08lx\n",(((DWORD)&pEddlcl->peSurface_DdList) - (DWORD)pEddlcl), pEddlcl->peSurface_DdList);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_018                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_018) - (DWORD)pEddlcl), pEddlcl->unk_018);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_01c                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_01c) - (DWORD)pEddlcl), pEddlcl->unk_01c);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_020                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_020) - (DWORD)pEddlcl), pEddlcl->unk_020);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->peDirectDrawGlobal2      : 0x%08lx\n",(((DWORD)&pEddlcl->peDirectDrawGlobal2) - (DWORD)pEddlcl), pEddlcl->peDirectDrawGlobal2);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->fpProcess                : 0x%08lx\n",(((DWORD)&pEddlcl->fpProcess) - (DWORD)pEddlcl), pEddlcl->fpProcess);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->fl                       : 0x%08lx\n",(((DWORD)&pEddlcl->fl) - (DWORD)pEddlcl), pEddlcl->fl);

    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->peDirectDrawLocal_prev   : 0x%08lx\n",(((DWORD)&pEddlcl->peDirectDrawLocal_prev) - (DWORD)pEddlcl), pEddlcl->peDirectDrawLocal_prev);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->Process                  : 0x%08lx\n",(((DWORD)&pEddlcl->Process) - (DWORD)pEddlcl), pEddlcl->Process);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_038                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_038) - (DWORD)pEddlcl), pEddlcl->unk_038);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->UniqueProcess            : 0x%08lx\n",(((DWORD)&pEddlcl->UniqueProcess) - (DWORD)pEddlcl), pEddlcl->UniqueProcess);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_040                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_040) - (DWORD)pEddlcl), pEddlcl->unk_040);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_044                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_044) - (DWORD)pEddlcl), pEddlcl->unk_044);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_048                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_048) - (DWORD)pEddlcl), pEddlcl->unk_048);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_04C                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_04C) - (DWORD)pEddlcl), pEddlcl->unk_04C);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_050                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_050) - (DWORD)pEddlcl), pEddlcl->unk_050);

    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_050                  : 0x%08lx\n",(((DWORD)&pEddlcl->unk_050) - (DWORD)pEddlcl), pEddlcl->unk_050);
}

void
dump_halinfo(DD_HALINFO *pHalInfo)
{
    if (pHalInfo->dwSize == sizeof(DD_HALINFO_V4))
    {
        DD_HALINFO_V4 *pHalInfo4 = (DD_HALINFO_V4 *) pHalInfo;
        int t;

        DPRINT1("DD_HALINFO Version NT4 found \n");
        DPRINT1(" pHalInfo4->dwSize                                  : 0x%08lx\n",(long)pHalInfo4->dwSize);
        DPRINT1(" pHalInfo4->vmiData->fpPrimary                      : 0x%08lx\n",(long)pHalInfo4->vmiData.fpPrimary);
        DPRINT1(" pHalInfo4->vmiData->dwFlags                        : 0x%08lx\n",(long)pHalInfo4->vmiData.dwFlags);
        DPRINT1(" pHalInfo4->vmiData->dwDisplayWidth                 : 0x%08lx\n",(long)pHalInfo4->vmiData.dwDisplayWidth);
        DPRINT1(" pHalInfo4->vmiData->dwDisplayHeight                : 0x%08lx\n",(long)pHalInfo4->vmiData.dwDisplayHeight);
        DPRINT1(" pHalInfo4->vmiData->lDisplayPitch                  : 0x%08lx\n",(long)pHalInfo4->vmiData.lDisplayPitch);

        DPRINT1(" pHalInfo4->vmiData->ddpfDisplay.dwSize             : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwSize);
        DPRINT1(" pHalInfo4->vmiData->ddpfDisplay.dwFlags            : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwFlags);
        DPRINT1(" pHalInfo4->vmiData->ddpfDisplay.dwFourCC           : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwFourCC);
        DPRINT1(" pHalInfo4->vmiData->ddpfDisplay.dwRGBBitCount      : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwRGBBitCount);
        DPRINT1(" pHalInfo4->vmiData->ddpfDisplay.dwRBitMask         : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwRBitMask);
        DPRINT1(" pHalInfo4->vmiData->ddpfDisplay.dwGBitMask         : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwGBitMask);
        DPRINT1(" pHalInfo4->vmiData->ddpfDisplay.dwBBitMask         : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwBBitMask);
        DPRINT1(" pHalInfo4->vmiData->ddpfDisplay.dwRGBAlphaBitMask  : 0x%08lx\n",(long)pHalInfo4->vmiData.ddpfDisplay.dwRGBAlphaBitMask);

        DPRINT1(" pHalInfo4->vmiData->dwOffscreenAlign               : 0x%08lx\n",(long)pHalInfo4->vmiData.dwOffscreenAlign);
        DPRINT1(" pHalInfo4->vmiData->dwOverlayAlign                 : 0x%08lx\n",(long)pHalInfo4->vmiData.dwOverlayAlign);
        DPRINT1(" pHalInfo4->vmiData->dwTextureAlign                 : 0x%08lx\n",(long)pHalInfo4->vmiData.dwTextureAlign);
        DPRINT1(" pHalInfo4->vmiData->dwZBufferAlign                 : 0x%08lx\n",(long)pHalInfo4->vmiData.dwZBufferAlign);
        DPRINT1(" pHalInfo4->vmiData->dwAlphaAlign                   : 0x%08lx\n",(long)pHalInfo4->vmiData.dwAlphaAlign);
        DPRINT1(" pHalInfo4->vmiData->pvPrimary                      : 0x%08lx\n",(long)pHalInfo4->vmiData.pvPrimary);

        DPRINT1(" pHalInfo4->ddCaps.dwSize                           : 0x%08lx\n",pHalInfo4->ddCaps.dwSize);
        DPRINT1(" pHalInfo4->ddCaps.dwCaps                           : 0x%08lx\n",pHalInfo4->ddCaps.dwCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwCaps2                          : 0x%08lx\n",pHalInfo4->ddCaps.dwCaps2);
        DPRINT1(" pHalInfo4->ddCaps.dwCKeyCaps                       : 0x%08lx\n",pHalInfo4->ddCaps.dwCKeyCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwFXCaps                         : 0x%08lx\n",pHalInfo4->ddCaps.dwFXCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwFXAlphaCaps                    : 0x%08lx\n",pHalInfo4->ddCaps.dwFXAlphaCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwPalCaps                        : 0x%08lx\n",pHalInfo4->ddCaps.dwPalCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwSVCaps                         : 0x%08lx\n",pHalInfo4->ddCaps.dwSVCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwAlphaBltConstBitDepths         : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaBltConstBitDepths);
        DPRINT1(" pHalInfo4->ddCaps.dwAlphaBltPixelBitDepths         : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaBltPixelBitDepths);
        DPRINT1(" pHalInfo4->ddCaps.dwAlphaBltSurfaceBitDepths       : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaBltSurfaceBitDepths);
        DPRINT1(" pHalInfo4->ddCaps.dwAlphaOverlayConstBitDepths     : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaOverlayConstBitDepths);
        DPRINT1(" pHalInfo4->ddCaps.dwAlphaOverlayPixelBitDepths     : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaOverlayPixelBitDepths);
        DPRINT1(" pHalInfo4->ddCaps.dwAlphaOverlaySurfaceBitDepths   : 0x%08lx\n",pHalInfo4->ddCaps.dwAlphaOverlaySurfaceBitDepths);
        DPRINT1(" pHalInfo4->ddCaps.dwZBufferBitDepths               : 0x%08lx\n",pHalInfo4->ddCaps.dwZBufferBitDepths);
        DPRINT1(" pHalInfo4->ddCaps.dwVidMemTotal                    : 0x%08lx\n",pHalInfo4->ddCaps.dwVidMemTotal);
        DPRINT1(" pHalInfo4->ddCaps.dwVidMemFree                     : 0x%08lx\n",pHalInfo4->ddCaps.dwVidMemFree);
        DPRINT1(" pHalInfo4->ddCaps.dwMaxVisibleOverlays             : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxVisibleOverlays);
        DPRINT1(" pHalInfo4->ddCaps.dwCurrVisibleOverlays            : 0x%08lx\n",pHalInfo4->ddCaps.dwCurrVisibleOverlays);
        DPRINT1(" pHalInfo4->ddCaps.dwNumFourCCCodes                 : 0x%08lx\n",pHalInfo4->ddCaps.dwNumFourCCCodes);
        DPRINT1(" pHalInfo4->ddCaps.dwAlignBoundarySrc               : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignBoundarySrc);
        DPRINT1(" pHalInfo4->ddCaps.dwAlignSizeSrc                   : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignSizeSrc);
        DPRINT1(" pHalInfo4->ddCaps.dwAlignBoundaryDes               : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignBoundaryDest);
        DPRINT1(" pHalInfo4->ddCaps.dwAlignSizeDest                  : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignSizeDest);
        DPRINT1(" pHalInfo4->ddCaps.dwAlignStrideAlign               : 0x%08lx\n",pHalInfo4->ddCaps.dwAlignStrideAlign);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        DPRINT1(" pHalInfo4->ddCaps.dwRops[0x%04x]                   : 0x%08lx\n",t,pHalInfo4->ddCaps.dwRops[t]);
        }
        DPRINT1(" pHalInfo4->ddCaps.ddsCaps.dwCaps                   : 0x%08lx\n",pHalInfo4->ddCaps.ddsCaps.dwCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwMinOverlayStretch              : 0x%08lx\n",pHalInfo4->ddCaps.dwMinOverlayStretch);
        DPRINT1(" pHalInfo4->ddCaps.dwMaxOverlayStretch              : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxOverlayStretch);
        DPRINT1(" pHalInfo4->ddCaps.dwMinLiveVideoStretch            : 0x%08lx\n",pHalInfo4->ddCaps.dwMinLiveVideoStretch);
        DPRINT1(" pHalInfo4->ddCaps.dwMaxLiveVideoStretch            : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxLiveVideoStretch);
        DPRINT1(" pHalInfo4->ddCaps.dwMinHwCodecStretch              : 0x%08lx\n",pHalInfo4->ddCaps.dwMinHwCodecStretch);
        DPRINT1(" pHalInfo4->ddCaps.dwMaxHwCodecStretch              : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxHwCodecStretch);
        DPRINT1(" pHalInfo4->ddCaps.dwReserved1                      : 0x%08lx\n",pHalInfo4->ddCaps.dwReserved1);
        DPRINT1(" pHalInfo4->ddCaps.dwReserved2                      : 0x%08lx\n",pHalInfo4->ddCaps.dwReserved2);
        DPRINT1(" pHalInfo4->ddCaps.dwReserved3                      : 0x%08lx\n",pHalInfo4->ddCaps.dwReserved3);
        DPRINT1(" pHalInfo4->ddCaps.dwSVBCaps                        : 0x%08lx\n",pHalInfo4->ddCaps.dwSVBCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwSVBCKeyCaps                    : 0x%08lx\n",pHalInfo4->ddCaps.dwSVBCKeyCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwSVBFXCaps                      : 0x%08lx\n",pHalInfo4->ddCaps.dwSVBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        DPRINT1(" pHalInfo4->ddCaps.dwSVBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo4->ddCaps.dwSVBRops[t]);
        }
        DPRINT1(" pHalInfo4->ddCaps.dwVSBCaps                        : 0x%08lx\n",pHalInfo4->ddCaps.dwVSBCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwVSBCKeyCaps                    : 0x%08lx\n",pHalInfo4->ddCaps.dwVSBCKeyCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwVSBFXCaps                      : 0x%08lx\n",pHalInfo4->ddCaps.dwVSBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        DPRINT1(" pHalInfo4->ddCaps.dwVSBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo4->ddCaps.dwVSBRops[t]);
        }
        DPRINT1(" pHalInfo4->ddCaps.dwSSBCaps                        : 0x%08lx\n",pHalInfo4->ddCaps.dwSSBCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwSSBCKeyCa                      : 0x%08lx\n",pHalInfo4->ddCaps.dwSSBCKeyCaps);
        DPRINT1(" pHalInfo4->ddCaps.dwSSBFXCaps                      : 0x%08lx\n",pHalInfo4->ddCaps.dwSSBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        DPRINT1(" pHalInfo4->ddCaps.dwSSBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo4->ddCaps.dwSSBRops[t]);
        }

        DPRINT1(" pHalInfo4->ddCaps.dwMaxVideoPorts                  : 0x%08lx\n",pHalInfo4->ddCaps.dwMaxVideoPorts);
        DPRINT1(" pHalInfo4->ddCaps.dwCurrVideoPorts                 : 0x%08lx\n",pHalInfo4->ddCaps.dwCurrVideoPorts);
        DPRINT1(" pHalInfo4->ddCaps.dwSVBCaps2                       : 0x%08lx\n",pHalInfo4->ddCaps.dwSVBCaps2);


        DPRINT1(" pHalInfo4->GetDriverInfo                           : 0x%08lx\n",(long)pHalInfo4->GetDriverInfo);
        DPRINT1(" pHalInfo4->dwFlags                                 : 0x%08lx\n",(long)pHalInfo4->dwFlags);

    }
    else if (pHalInfo->dwSize == sizeof(DD_HALINFO))
    {
        int t;
        UINT flag;
        INT count=0;
        
        DPRINT1("DD_HALINFO Version NT 2000/XP/2003 found \n");
        DPRINT1(" pHalInfo->dwSize                                  : 0x%08lx\n",(long)pHalInfo->dwSize);

        DPRINT1(" pHalInfo->vmiData->fpPrimary                      : 0x%08lx\n",(long)pHalInfo->vmiData.fpPrimary);
        DPRINT1(" pHalInfo->vmiData->dwFlags                        : 0x%08lx\n",(long)pHalInfo->vmiData.dwFlags);
        DPRINT1(" pHalInfo->vmiData->dwDisplayWidth                 : 0x%08lx\n",(long)pHalInfo->vmiData.dwDisplayWidth);
        DPRINT1(" pHalInfo->vmiData->dwDisplayHeight                : 0x%08lx\n",(long)pHalInfo->vmiData.dwDisplayHeight);
        DPRINT1(" pHalInfo->vmiData->lDisplayPitch                  : 0x%08lx\n",(long)pHalInfo->vmiData.lDisplayPitch);

        DPRINT1(" pHalInfo->vmiData->ddpfDisplay.dwSize             : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwSize);
        DPRINT1(" pHalInfo->vmiData->ddpfDisplay.dwFlags            : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwFlags);
        DPRINT1(" pHalInfo->vmiData->ddpfDisplay.dwFourCC           : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwFourCC);
        DPRINT1(" pHalInfo->vmiData->ddpfDisplay.dwRGBBitCount      : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount);
        DPRINT1(" pHalInfo->vmiData->ddpfDisplay.dwRBitMask         : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwRBitMask);
        DPRINT1(" pHalInfo->vmiData->ddpfDisplay.dwGBitMask         : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwGBitMask);
        DPRINT1(" pHalInfo->vmiData->ddpfDisplay.dwBBitMask         : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwBBitMask);
        DPRINT1(" pHalInfo->vmiData->ddpfDisplay.dwRGBAlphaBitMask  : 0x%08lx\n",(long)pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask);


        DPRINT1(" pHalInfo->vmiData->dwOffscreenAlign               : 0x%08lx\n",(long)pHalInfo->vmiData.dwOffscreenAlign);
        DPRINT1(" pHalInfo->vmiData->dwOverlayAlign                 : 0x%08lx\n",(long)pHalInfo->vmiData.dwOverlayAlign);
        DPRINT1(" pHalInfo->vmiData->dwTextureAlign                 : 0x%08lx\n",(long)pHalInfo->vmiData.dwTextureAlign);
        DPRINT1(" pHalInfo->vmiData->dwZBufferAlign                 : 0x%08lx\n",(long)pHalInfo->vmiData.dwZBufferAlign);
        DPRINT1(" pHalInfo->vmiData->dwAlphaAlign                   : 0x%08lx\n",(long)pHalInfo->vmiData.dwAlphaAlign);
        DPRINT1(" pHalInfo->vmiData->pvPrimary                      : 0x%08lx\n",(long)pHalInfo->vmiData.pvPrimary);

        DPRINT1(" pHalInfo->ddCaps.dwSize                           : 0x%08lx\n",pHalInfo->ddCaps.dwSize);
        DPRINT1(" pHalInfo->ddCaps.dwCaps                           : ");
        flag = pHalInfo->ddCaps.dwCaps;
        count = 0;
        checkflag(flag,DDCAPS_3D,"DDCAPS_3D");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYDEST,"DDCAPS_ALIGNBOUNDARYDEST");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYSRC,"DDCAPS_ALIGNBOUNDARYSRC");
        checkflag(flag,DDCAPS_ALIGNSIZEDEST,"DDCAPS_ALIGNSIZEDEST");
        checkflag(flag,DDCAPS_ALIGNSIZESRC,"DDCAPS_ALIGNSIZESRC");
        checkflag(flag,DDCAPS_ALIGNSTRIDE,"DDCAPS_ALIGNSTRIDE");
        checkflag(flag,DDCAPS_ALPHA,"DDCAPS_ALPHA");
        checkflag(flag,DDCAPS_BANKSWITCHED,"DDCAPS_BANKSWITCHED");
        checkflag(flag,DDCAPS_BLT,"DDCAPS_BLT");
        checkflag(flag,DDCAPS_BLTCOLORFILL,"DDCAPS_BLTCOLORFILL");
        checkflag(flag,DDCAPS_BLTDEPTHFILL,"DDCAPS_BLTDEPTHFILL");
        checkflag(flag,DDCAPS_BLTFOURCC,"DDCAPS_BLTFOURCC");
        checkflag(flag,DDCAPS_BLTQUEUE,"DDCAPS_BLTQUEUE");
        checkflag(flag,DDCAPS_BLTSTRETCH,"DDCAPS_BLTSTRETCH");
        checkflag(flag,DDCAPS_CANBLTSYSMEM,"DDCAPS_CANBLTSYSMEM");
        checkflag(flag,DDCAPS_CANCLIP,"DDCAPS_CANCLIP");
        checkflag(flag,DDCAPS_CANCLIPSTRETCHED,"DDCAPS_CANCLIPSTRETCHED");
        checkflag(flag,DDCAPS_COLORKEY,"DDCAPS_COLORKEY");
        checkflag(flag,DDCAPS_COLORKEYHWASSIST,"DDCAPS_COLORKEYHWASSIST");
        checkflag(flag,DDCAPS_GDI,"DDCAPS_GDI");
        checkflag(flag,DDCAPS_NOHARDWARE,"DDCAPS_NOHARDWARE");
        checkflag(flag,DDCAPS_OVERLAY,"DDCAPS_OVERLAY");
        checkflag(flag,DDCAPS_OVERLAYCANTCLIP,"DDCAPS_OVERLAYCANTCLIP");
        checkflag(flag,DDCAPS_OVERLAYFOURCC,"DDCAPS_OVERLAYFOURCC");
        checkflag(flag,DDCAPS_OVERLAYSTRETCH,"DDCAPS_OVERLAYSTRETCH");
        checkflag(flag,DDCAPS_PALETTE,"DDCAPS_PALETTE");
        checkflag(flag,DDCAPS_PALETTEVSYNC,"DDCAPS_PALETTEVSYNC");
        checkflag(flag,DDCAPS_READSCANLINE,"DDCAPS_READSCANLINE");
        checkflag(flag,DDCAPS_STEREOVIEW,"DDCAPS_STEREOVIEW");
        checkflag(flag,DDCAPS_VBI,"DDCAPS_VBI");
        checkflag(flag,DDCAPS_ZBLTS,"DDCAPS_ZBLTS");
        checkflag(flag,DDCAPS_ZOVERLAYS,"DDCAPS_ZOVERLAYS");
        endcheckflag(flag,"pHalInfo->ddCaps.dwCaps");

        DPRINT1(" pHalInfo->ddCaps.dwCaps2                          : ");
        flag = pHalInfo->ddCaps.dwCaps2;
        count = 0;
        checkflag(flag,DDCAPS2_AUTOFLIPOVERLAY,"DDCAPS2_AUTOFLIPOVERLAY");
        checkflag(flag,DDCAPS2_CANAUTOGENMIPMAP,"DDCAPS2_CANAUTOGENMIPMAP");
        checkflag(flag,DDCAPS2_CANBOBHARDWARE,"DDCAPS2_CANBOBHARDWARE");
        checkflag(flag,DDCAPS2_CANBOBINTERLEAVED,"DDCAPS2_CANBOBINTERLEAVED");
        checkflag(flag,DDCAPS2_CANBOBNONINTERLEAVED,"DDCAPS2_CANBOBNONINTERLEAVED");
        checkflag(flag,DDCAPS2_CANCALIBRATEGAMMA,"DDCAPS2_CANCALIBRATEGAMMA");
        checkflag(flag,DDCAPS2_CANDROPZ16BIT,"DDCAPS2_CANDROPZ16BIT");
        checkflag(flag,DDCAPS2_CANFLIPODDEVEN,"DDCAPS2_CANFLIPODDEVEN");
        checkflag(flag,DDCAPS2_CANMANAGERESOURCE,"DDCAPS2_CANMANAGERESOURCE");
        checkflag(flag,DDCAPS2_CANMANAGETEXTURE,"DDCAPS2_CANMANAGETEXTURE");

        checkflag(flag,DDCAPS2_CANRENDERWINDOWED,"DDCAPS2_CANRENDERWINDOWED");
        checkflag(flag,DDCAPS2_CERTIFIED,"DDCAPS2_CERTIFIED");
        checkflag(flag,DDCAPS2_COLORCONTROLOVERLAY,"DDCAPS2_COLORCONTROLOVERLAY");
        checkflag(flag,DDCAPS2_COLORCONTROLPRIMARY,"DDCAPS2_COLORCONTROLPRIMARY");
        checkflag(flag,DDCAPS2_COPYFOURCC,"DDCAPS2_COPYFOURCC");
        checkflag(flag,DDCAPS2_FLIPINTERVAL,"DDCAPS2_FLIPINTERVAL");
        checkflag(flag,DDCAPS2_FLIPNOVSYNC,"DDCAPS2_FLIPNOVSYNC");
        checkflag(flag,DDCAPS2_NO2DDURING3DSCENE,"DDCAPS2_NO2DDURING3DSCENE");
        checkflag(flag,DDCAPS2_NONLOCALVIDMEM,"DDCAPS2_NONLOCALVIDMEM");
        checkflag(flag,DDCAPS2_NONLOCALVIDMEMCAPS,"DDCAPS2_NONLOCALVIDMEMCAPS");
        checkflag(flag,DDCAPS2_NOPAGELOCKREQUIRED,"DDCAPS2_NOPAGELOCKREQUIRED");
        checkflag(flag,DDCAPS2_PRIMARYGAMMA,"DDCAPS2_PRIMARYGAMMA");
        checkflag(flag,DDCAPS2_VIDEOPORT,"DDCAPS2_VIDEOPORT");
        checkflag(flag,DDCAPS2_WIDESURFACES,"DDCAPS2_WIDESURFACES");
        endcheckflag(flag,"pHalInfo->ddCaps.dwCaps2");

        DPRINT1(" pHalInfo->ddCaps.dwCKeyCaps                       : ");
        flag = pHalInfo->ddCaps.dwCKeyCaps;
        count = 0;
        checkflag(flag,DDCKEYCAPS_DESTBLT,"DDCKEYCAPS_DESTBLT");
        checkflag(flag,DDCKEYCAPS_DESTBLTCLRSPACE,"DDCKEYCAPS_DESTBLTCLRSPACE");
        checkflag(flag,DDCKEYCAPS_DESTBLTCLRSPACEYUV,"DDCKEYCAPS_DESTBLTCLRSPACEYUV");
        checkflag(flag,DDCKEYCAPS_DESTBLTYUV,"DDCKEYCAPS_DESTBLTYUV");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAY,"DDCKEYCAPS_DESTOVERLAY");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAYCLRSPACE,"DDCKEYCAPS_DESTOVERLAYCLRSPACE");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV,"DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAYONEACTIVE,"DDCKEYCAPS_DESTOVERLAYONEACTIVE");
        checkflag(flag,DDCKEYCAPS_DESTOVERLAYYUV,"DDCKEYCAPS_DESTOVERLAYYUV");
        checkflag(flag,DDCKEYCAPS_NOCOSTOVERLAY,"DDCKEYCAPS_NOCOSTOVERLAY");
        checkflag(flag,DDCKEYCAPS_SRCBLT,"DDCKEYCAPS_SRCBLT");
        checkflag(flag,DDCKEYCAPS_SRCBLTCLRSPACE,"DDCKEYCAPS_SRCBLTCLRSPACE");
        checkflag(flag,DDCKEYCAPS_SRCBLTCLRSPACEYUV,"DDCKEYCAPS_SRCBLTCLRSPACEYUV");
        checkflag(flag,DDCKEYCAPS_SRCBLTYUV,"DDCKEYCAPS_SRCBLTYUV");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAY,"DDCKEYCAPS_SRCOVERLAY");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAYCLRSPACE,"DDCKEYCAPS_SRCOVERLAYCLRSPACE");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV,"DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAYONEACTIVE,"DDCKEYCAPS_SRCOVERLAYONEACTIVE");
        checkflag(flag,DDCKEYCAPS_SRCOVERLAYYUV,"DDCKEYCAPS_SRCOVERLAYYUV");
        endcheckflag(flag,"pHalInfo->ddCaps.dwCKeyCaps");

        DPRINT1(" pHalInfo->ddCaps.dwFXCaps                         : ");
        flag = pHalInfo->ddCaps.dwFXCaps;
        count = 0;
        checkflag(flag,DDFXCAPS_BLTARITHSTRETCHY,"DDFXCAPS_BLTARITHSTRETCHY");
        checkflag(flag,DDFXCAPS_BLTARITHSTRETCHYN,"DDFXCAPS_BLTARITHSTRETCHYN");
        checkflag(flag,DDFXCAPS_BLTMIRRORLEFTRIGHT,"DDFXCAPS_BLTMIRRORLEFTRIGHT");
        checkflag(flag,DDFXCAPS_BLTMIRRORUPDOWN,"DDFXCAPS_BLTMIRRORUPDOWN");
        checkflag(flag,DDFXCAPS_BLTROTATION,"DDFXCAPS_BLTROTATION");
        checkflag(flag,DDFXCAPS_BLTROTATION90,"DDFXCAPS_BLTROTATION90");
        checkflag(flag,DDFXCAPS_BLTSHRINKX,"DDFXCAPS_BLTSHRINKX");
        checkflag(flag,DDFXCAPS_BLTSHRINKXN,"DDFXCAPS_BLTSHRINKXN");
        checkflag(flag,DDFXCAPS_BLTSHRINKY,"DDFXCAPS_BLTSHRINKY");
        checkflag(flag,DDFXCAPS_BLTSHRINKYN,"DDFXCAPS_BLTSHRINKYN");
        checkflag(flag,DDFXCAPS_BLTSTRETCHX,"DDFXCAPS_BLTSTRETCHX");
        checkflag(flag,DDFXCAPS_BLTSTRETCHXN,"DDFXCAPS_BLTSTRETCHXN");
        checkflag(flag,DDFXCAPS_BLTSTRETCHY,"DDFXCAPS_BLTSTRETCHY");
        checkflag(flag,DDFXCAPS_BLTSTRETCHYN,"DDFXCAPS_BLTSTRETCHYN");
        checkflag(flag,DDFXCAPS_OVERLAYARITHSTRETCHY,"DDFXCAPS_OVERLAYARITHSTRETCHY");
        checkflag(flag,DDFXCAPS_OVERLAYARITHSTRETCHYN,"DDFXCAPS_OVERLAYARITHSTRETCHYN");
        checkflag(flag,DDFXCAPS_OVERLAYMIRRORLEFTRIGHT,"DDFXCAPS_OVERLAYMIRRORLEFTRIGHT");
        checkflag(flag,DDFXCAPS_OVERLAYMIRRORUPDOWN,"DDFXCAPS_OVERLAYMIRRORUPDOWN");
        checkflag(flag,DDFXCAPS_OVERLAYSHRINKX,"DDFXCAPS_OVERLAYSHRINKX");
        checkflag(flag,DDFXCAPS_OVERLAYSHRINKXN,"DDFXCAPS_OVERLAYSHRINKXN");
        checkflag(flag,DDFXCAPS_OVERLAYSHRINKY,"DDFXCAPS_OVERLAYSHRINKY");
        checkflag(flag,DDFXCAPS_OVERLAYSHRINKYN,"DDFXCAPS_OVERLAYSHRINKYN");
        checkflag(flag,DDFXCAPS_OVERLAYSTRETCHX,"DDFXCAPS_OVERLAYSTRETCHX");
        checkflag(flag,DDFXCAPS_OVERLAYSTRETCHX,"DDFXCAPS_OVERLAYSTRETCHX");
        checkflag(flag,DDFXCAPS_OVERLAYSTRETCHY,"DDFXCAPS_OVERLAYSTRETCHY");
        checkflag(flag,DDFXCAPS_OVERLAYSTRETCHYN,"DDFXCAPS_OVERLAYSTRETCHYN");
        endcheckflag(flag,"pHalInfo->ddCaps.dwFXCaps");

        DPRINT1(" pHalInfo->ddCaps.dwFXAlphaCaps                    : 0x%08lx\n",pHalInfo->ddCaps.dwFXAlphaCaps);
        DPRINT1(" pHalInfo->ddCaps.dwPalCaps                        : 0x%08lx\n",pHalInfo->ddCaps.dwPalCaps);

        DPRINT1(" pHalInfo->ddCaps.dwSVCaps                         : ");        
        flag = pHalInfo->ddCaps.dwSVCaps;
        count = 0;
        checkflag(flag,DDSVCAPS_ENIGMA,"DDSVCAPS_ENIGMA");
        checkflag(flag,DDSVCAPS_FLICKER,"DDSVCAPS_FLICKER");
        checkflag(flag,DDSVCAPS_REDBLUE,"DDSVCAPS_REDBLUE");
        checkflag(flag,DDSVCAPS_SPLIT,"DDSVCAPS_SPLIT");
        endcheckflag(flag,"pHalInfo->ddCaps.dwSVCaps");

        DPRINT1(" pHalInfo->ddCaps.dwAlphaBltConstBitDepths         : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaBltConstBitDepths);
        DPRINT1(" pHalInfo->ddCaps.dwAlphaBltPixelBitDepths         : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaBltPixelBitDepths);
        DPRINT1(" pHalInfo->ddCaps.dwAlphaBltSurfaceBitDepths       : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaBltSurfaceBitDepths);
        DPRINT1(" pHalInfo->ddCaps.dwAlphaOverlayConstBitDepths     : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaOverlayConstBitDepths);
        DPRINT1(" pHalInfo->ddCaps.dwAlphaOverlayPixelBitDepths     : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaOverlayPixelBitDepths);
        DPRINT1(" pHalInfo->ddCaps.dwAlphaOverlaySurfaceBitDepths   : 0x%08lx\n",pHalInfo->ddCaps.dwAlphaOverlaySurfaceBitDepths);
        DPRINT1(" pHalInfo->ddCaps.dwZBufferBitDepths               : 0x%08lx\n",pHalInfo->ddCaps.dwZBufferBitDepths);
        DPRINT1(" pHalInfo->ddCaps.dwVidMemTotal                    : 0x%08lx\n",pHalInfo->ddCaps.dwVidMemTotal);
        DPRINT1(" pHalInfo->ddCaps.dwVidMemFree                     : 0x%08lx\n",pHalInfo->ddCaps.dwVidMemFree);
        DPRINT1(" pHalInfo->ddCaps.dwMaxVisibleOverlays             : 0x%08lx\n",pHalInfo->ddCaps.dwMaxVisibleOverlays);
        DPRINT1(" pHalInfo->ddCaps.dwCurrVisibleOverlays            : 0x%08lx\n",pHalInfo->ddCaps.dwCurrVisibleOverlays);
        DPRINT1(" pHalInfo->ddCaps.dwNumFourCCCodes                 : 0x%08lx\n",pHalInfo->ddCaps.dwNumFourCCCodes);
        DPRINT1(" pHalInfo->ddCaps.dwAlignBoundarySrc               : 0x%08lx\n",pHalInfo->ddCaps.dwAlignBoundarySrc);
        DPRINT1(" pHalInfo->ddCaps.dwAlignSizeSrc                   : 0x%08lx\n",pHalInfo->ddCaps.dwAlignSizeSrc);
        DPRINT1(" pHalInfo->ddCaps.dwAlignBoundaryDes               : 0x%08lx\n",pHalInfo->ddCaps.dwAlignBoundaryDest);
        DPRINT1(" pHalInfo->ddCaps.dwAlignSizeDest                  : 0x%08lx\n",pHalInfo->ddCaps.dwAlignSizeDest);
        DPRINT1(" pHalInfo->ddCaps.dwAlignStrideAlign               : 0x%08lx\n",pHalInfo->ddCaps.dwAlignStrideAlign);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        DPRINT1(" pHalInfo->ddCaps.dwRops[0x%04x]                   : 0x%08lx\n",t,pHalInfo->ddCaps.dwRops[t]);
        }
        DPRINT1(" pHalInfo->ddCaps.ddsCaps.dwCaps                   : ");
        flag = pHalInfo->ddCaps.ddsCaps.dwCaps;
        count = 0;
        checkflag(flag,DDSCAPS_3DDEVICE,"DDSCAPS_3DDEVICE");
        checkflag(flag,DDSCAPS_ALLOCONLOAD,"DDSCAPS_ALLOCONLOAD");
        checkflag(flag,DDSCAPS_ALPHA,"DDSCAPS_ALPHA");
        checkflag(flag,DDSCAPS_BACKBUFFER,"DDSCAPS_BACKBUFFER");
        checkflag(flag,DDSCAPS_COMPLEX,"DDSCAPS_COMPLEX");
        checkflag(flag,DDSCAPS_EXECUTEBUFFER,"DDSCAPS_EXECUTEBUFFER");
        checkflag(flag,DDSCAPS_FLIP,"DDSCAPS_FLIP");
        checkflag(flag,DDSCAPS_FRONTBUFFER,"DDSCAPS_FRONTBUFFER");
        checkflag(flag,DDSCAPS_HWCODEC,"DDSCAPS_HWCODEC");
        checkflag(flag,DDSCAPS_LIVEVIDEO,"DDSCAPS_LIVEVIDEO");
        checkflag(flag,DDSCAPS_LOCALVIDMEM,"DDSCAPS_LOCALVIDMEM");
        checkflag(flag,DDSCAPS_MIPMAP,"DDSCAPS_MIPMAP");
        checkflag(flag,DDSCAPS_MODEX,"DDSCAPS_MODEX");
        checkflag(flag,DDSCAPS_NONLOCALVIDMEM,"DDSCAPS_NONLOCALVIDMEM");
        checkflag(flag,DDSCAPS_OFFSCREENPLAIN,"DDSCAPS_OFFSCREENPLAIN");
        checkflag(flag,DDSCAPS_OVERLAY,"DDSCAPS_OVERLAY");
        checkflag(flag,DDSCAPS_OPTIMIZED,"DDSCAPS_OPTIMIZED");
        checkflag(flag,DDSCAPS_OWNDC,"DDSCAPS_OWNDC");
        checkflag(flag,DDSCAPS_PALETTE,"DDSCAPS_PALETTE");
        checkflag(flag,DDSCAPS_PRIMARYSURFACE,"DDSCAPS_PRIMARYSURFACE");
        checkflag(flag,DDSCAPS_PRIMARYSURFACELEFT,"DDSCAPS_PRIMARYSURFACELEFT");
        checkflag(flag,DDSCAPS_STANDARDVGAMODE,"DDSCAPS_STANDARDVGAMODE");
        checkflag(flag,DDSCAPS_SYSTEMMEMORY,"DDSCAPS_SYSTEMMEMORY");
        checkflag(flag,DDSCAPS_TEXTURE,"DDSCAPS_TEXTURE");
        checkflag(flag,DDSCAPS_VIDEOMEMORY,"DDSCAPS_VIDEOMEMORY");
        checkflag(flag,DDSCAPS_VIDEOPORT,"DDSCAPS_VIDEOPORT");
        checkflag(flag,DDSCAPS_VISIBLE,"DDSCAPS_VISIBLE");
        checkflag(flag,DDSCAPS_WRITEONLY,"DDSCAPS_WRITEONLY");
        checkflag(flag,DDSCAPS_ZBUFFER,"DDSCAPS_ZBUFFER");
        endcheckflag(flag,"pHalInfo->ddCaps.ddsCaps.dwCaps");

        DPRINT1(" pHalInfo->ddCaps.dwMinOverlayStretch              : 0x%08lx\n",pHalInfo->ddCaps.dwMinOverlayStretch);
        DPRINT1(" pHalInfo->ddCaps.dwMaxOverlayStretch              : 0x%08lx\n",pHalInfo->ddCaps.dwMaxOverlayStretch);
        DPRINT1(" pHalInfo->ddCaps.dwMinLiveVideoStretch            : 0x%08lx\n",pHalInfo->ddCaps.dwMinLiveVideoStretch);
        DPRINT1(" pHalInfo->ddCaps.dwMaxLiveVideoStretch            : 0x%08lx\n",pHalInfo->ddCaps.dwMaxLiveVideoStretch);
        DPRINT1(" pHalInfo->ddCaps.dwMinHwCodecStretch              : 0x%08lx\n",pHalInfo->ddCaps.dwMinHwCodecStretch);
        DPRINT1(" pHalInfo->ddCaps.dwMaxHwCodecStretch              : 0x%08lx\n",pHalInfo->ddCaps.dwMaxHwCodecStretch);
        DPRINT1(" pHalInfo->ddCaps.dwReserved1                      : 0x%08lx\n",pHalInfo->ddCaps.dwReserved1);
        DPRINT1(" pHalInfo->ddCaps.dwReserved2                      : 0x%08lx\n",pHalInfo->ddCaps.dwReserved2);
        DPRINT1(" pHalInfo->ddCaps.dwReserved3                      : 0x%08lx\n",pHalInfo->ddCaps.dwReserved3);
        
        DPRINT1(" pHalInfo->ddCaps.dwSVBCaps                        : ");
        flag = pHalInfo->ddCaps.dwSVBCaps;
        count = 0;   
        checkflag(flag,DDCAPS_3D,"DDCAPS_3D");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYDEST,"DDCAPS_ALIGNBOUNDARYDEST");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYSRC,"DDCAPS_ALIGNBOUNDARYSRC");
        checkflag(flag,DDCAPS_ALIGNSIZEDEST,"DDCAPS_ALIGNSIZEDEST");
        checkflag(flag,DDCAPS_ALIGNSIZESRC,"DDCAPS_ALIGNSIZESRC");
        checkflag(flag,DDCAPS_ALIGNSTRIDE,"DDCAPS_ALIGNSTRIDE");
        checkflag(flag,DDCAPS_ALPHA,"DDCAPS_ALPHA");
        checkflag(flag,DDCAPS_BANKSWITCHED,"DDCAPS_BANKSWITCHED");
        checkflag(flag,DDCAPS_BLT,"DDCAPS_BLT");
        checkflag(flag,DDCAPS_BLTCOLORFILL,"DDCAPS_BLTCOLORFILL");
        checkflag(flag,DDCAPS_BLTDEPTHFILL,"DDCAPS_BLTDEPTHFILL");
        checkflag(flag,DDCAPS_BLTFOURCC,"DDCAPS_BLTFOURCC");
        checkflag(flag,DDCAPS_BLTQUEUE,"DDCAPS_BLTQUEUE");
        checkflag(flag,DDCAPS_BLTSTRETCH,"DDCAPS_BLTSTRETCH");
        checkflag(flag,DDCAPS_CANBLTSYSMEM,"DDCAPS_CANBLTSYSMEM");
        checkflag(flag,DDCAPS_CANCLIP,"DDCAPS_CANCLIP");
        checkflag(flag,DDCAPS_CANCLIPSTRETCHED,"DDCAPS_CANCLIPSTRETCHED");
        checkflag(flag,DDCAPS_COLORKEY,"DDCAPS_COLORKEY");
        checkflag(flag,DDCAPS_COLORKEYHWASSIST,"DDCAPS_COLORKEYHWASSIST");
        checkflag(flag,DDCAPS_GDI,"DDCAPS_GDI");
        checkflag(flag,DDCAPS_NOHARDWARE,"DDCAPS_NOHARDWARE");
        checkflag(flag,DDCAPS_OVERLAY,"DDCAPS_OVERLAY");
        checkflag(flag,DDCAPS_OVERLAYCANTCLIP,"DDCAPS_OVERLAYCANTCLIP");
        checkflag(flag,DDCAPS_OVERLAYFOURCC,"DDCAPS_OVERLAYFOURCC");
        checkflag(flag,DDCAPS_OVERLAYSTRETCH,"DDCAPS_OVERLAYSTRETCH");
        checkflag(flag,DDCAPS_PALETTE,"DDCAPS_PALETTE");
        checkflag(flag,DDCAPS_PALETTEVSYNC,"DDCAPS_PALETTEVSYNC");
        checkflag(flag,DDCAPS_READSCANLINE,"DDCAPS_READSCANLINE");
        checkflag(flag,DDCAPS_STEREOVIEW,"DDCAPS_STEREOVIEW");
        checkflag(flag,DDCAPS_VBI,"DDCAPS_VBI");
        checkflag(flag,DDCAPS_ZBLTS,"DDCAPS_ZBLTS");
        checkflag(flag,DDCAPS_ZOVERLAYS,"DDCAPS_ZOVERLAYS");
        endcheckflag(flag,"pHalInfo->ddCaps.dwSVBCaps");

        DPRINT1(" pHalInfo->ddCaps.dwSVBCKeyCaps                    : 0x%08lx\n",pHalInfo->ddCaps.dwSVBCKeyCaps);
        DPRINT1(" pHalInfo->ddCaps.dwSVBFXCaps                      : 0x%08lx\n",pHalInfo->ddCaps.dwSVBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        DPRINT1(" pHalInfo->ddCaps.dwSVBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo->ddCaps.dwSVBRops[t]);
        }

        DPRINT1(" pHalInfo->ddCaps.dwVSBCaps                        : ");
        flag = pHalInfo->ddCaps.dwVSBCaps;
        count = 0;   
        checkflag(flag,DDCAPS_3D,"DDCAPS_3D");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYDEST,"DDCAPS_ALIGNBOUNDARYDEST");
        checkflag(flag,DDCAPS_ALIGNBOUNDARYSRC,"DDCAPS_ALIGNBOUNDARYSRC");
        checkflag(flag,DDCAPS_ALIGNSIZEDEST,"DDCAPS_ALIGNSIZEDEST");
        checkflag(flag,DDCAPS_ALIGNSIZESRC,"DDCAPS_ALIGNSIZESRC");
        checkflag(flag,DDCAPS_ALIGNSTRIDE,"DDCAPS_ALIGNSTRIDE");
        checkflag(flag,DDCAPS_ALPHA,"DDCAPS_ALPHA");
        checkflag(flag,DDCAPS_BANKSWITCHED,"DDCAPS_BANKSWITCHED");
        checkflag(flag,DDCAPS_BLT,"DDCAPS_BLT");
        checkflag(flag,DDCAPS_BLTCOLORFILL,"DDCAPS_BLTCOLORFILL");
        checkflag(flag,DDCAPS_BLTDEPTHFILL,"DDCAPS_BLTDEPTHFILL");
        checkflag(flag,DDCAPS_BLTFOURCC,"DDCAPS_BLTFOURCC");
        checkflag(flag,DDCAPS_BLTQUEUE,"DDCAPS_BLTQUEUE");
        checkflag(flag,DDCAPS_BLTSTRETCH,"DDCAPS_BLTSTRETCH");
        checkflag(flag,DDCAPS_CANBLTSYSMEM,"DDCAPS_CANBLTSYSMEM");
        checkflag(flag,DDCAPS_CANCLIP,"DDCAPS_CANCLIP");
        checkflag(flag,DDCAPS_CANCLIPSTRETCHED,"DDCAPS_CANCLIPSTRETCHED");
        checkflag(flag,DDCAPS_COLORKEY,"DDCAPS_COLORKEY");
        checkflag(flag,DDCAPS_COLORKEYHWASSIST,"DDCAPS_COLORKEYHWASSIST");
        checkflag(flag,DDCAPS_GDI,"DDCAPS_GDI");
        checkflag(flag,DDCAPS_NOHARDWARE,"DDCAPS_NOHARDWARE");
        checkflag(flag,DDCAPS_OVERLAY,"DDCAPS_OVERLAY");
        checkflag(flag,DDCAPS_OVERLAYCANTCLIP,"DDCAPS_OVERLAYCANTCLIP");
        checkflag(flag,DDCAPS_OVERLAYFOURCC,"DDCAPS_OVERLAYFOURCC");
        checkflag(flag,DDCAPS_OVERLAYSTRETCH,"DDCAPS_OVERLAYSTRETCH");
        checkflag(flag,DDCAPS_PALETTE,"DDCAPS_PALETTE");
        checkflag(flag,DDCAPS_PALETTEVSYNC,"DDCAPS_PALETTEVSYNC");
        checkflag(flag,DDCAPS_READSCANLINE,"DDCAPS_READSCANLINE");
        checkflag(flag,DDCAPS_STEREOVIEW,"DDCAPS_STEREOVIEW");
        checkflag(flag,DDCAPS_VBI,"DDCAPS_VBI");
        checkflag(flag,DDCAPS_ZBLTS,"DDCAPS_ZBLTS");
        checkflag(flag,DDCAPS_ZOVERLAYS,"DDCAPS_ZOVERLAYS");
        endcheckflag(flag,"pHalInfo->ddCaps.dwVSBCaps");

        DPRINT1(" pHalInfo->ddCaps.dwVSBCKeyCaps                    : 0x%08lx\n",pHalInfo->ddCaps.dwVSBCKeyCaps);
        DPRINT1(" pHalInfo->ddCaps.dwVSBFXCaps                      : 0x%08lx\n",pHalInfo->ddCaps.dwVSBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        DPRINT1(" pHalInfo->ddCaps.dwVSBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo->ddCaps.dwVSBRops[t]);
        }
        DPRINT1(" pHalInfo->ddCaps.dwSSBCaps                        : 0x%08lx\n",pHalInfo->ddCaps.dwSSBCaps);
        DPRINT1(" pHalInfo->ddCaps.dwSSBCKeyCa                      : 0x%08lx\n",pHalInfo->ddCaps.dwSSBCKeyCaps);
        DPRINT1(" pHalInfo->ddCaps.dwSSBFXCaps                      : 0x%08lx\n",pHalInfo->ddCaps.dwSSBFXCaps);
        for (t=0;t<DD_ROP_SPACE;t++)
        {
        DPRINT1(" pHalInfo->ddCaps.dwSSBRops[0x%04x]                : 0x%08lx\n",t,pHalInfo->ddCaps.dwSSBRops[t]);
        }

        DPRINT1(" pHalInfo->GetDriverInfo                           : 0x%08lx\n",(long)pHalInfo->GetDriverInfo);
        DPRINT1(" pHalInfo->dwFlags                                 : ");

        flag = pHalInfo->dwFlags;
        count = 0;
        checkflag(flag,DDHALINFO_ISPRIMARYDISPLAY,"DDHALINFO_ISPRIMARYDISPLAY");
        checkflag(flag,DDHALINFO_MODEXILLEGAL,"DDHALINFO_MODEXILLEGAL");
        checkflag(flag,DDHALINFO_GETDRIVERINFOSET,"DDHALINFO_GETDRIVERINFOSET");
        checkflag(flag,DDHALINFO_GETDRIVERINFO2,"DDHALINFO_GETDRIVERINFO2");
        endcheckflag(flag,"pHalInfo->dwFlags");

        DPRINT1(" pHalInfo->lpD3DGlobalDriverData                   : 0x%08lx\n",(long)pHalInfo->lpD3DGlobalDriverData);
        DPRINT1(" pHalInfo->lpD3DHALCallbacks                       : 0x%08lx\n",(long)pHalInfo->lpD3DHALCallbacks);
        DPRINT1(" pHalInfo->lpD3DBufCallbacks                       : 0x%08lx\n",(long)pHalInfo->lpD3DBufCallbacks);
    }
    else
    {
        if (pHalInfo->dwSize !=0)
        {
            DPRINT1("unkonwn dwSize DD_HALINFO : the size found is 0x%08lx\n",pHalInfo->dwSize);
        }
        else
        {
            DPRINT1("none pHalInfo from the driver 0x%08lx\n",pHalInfo->dwSize);
        }
    }
}

/* EOF */
