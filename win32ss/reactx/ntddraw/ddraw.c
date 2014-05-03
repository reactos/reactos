/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsystems/win32/win32k/ntddraw/ddraw.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */

#include <win32k.h>
#include <debug.h>

PGD_DXDDSTARTUPDXGRAPHICS gpfnStartupDxGraphics = NULL;
PGD_DXDDCLEANUPDXGRAPHICS gpfnCleanupDxGraphics = NULL;

/* export from dxeng.c */
extern DRVFN gaEngFuncs[];
extern ULONG gcEngFuncs;
extern EDD_DIRECTDRAW_GLOBAL edd_DdirectDraw_Global;


DRVFN gpDxFuncs[DXG_INDEX_DxDdIoctl + 1];
HANDLE ghDxGraphics = NULL;
ULONG gdwDirectDrawContext = 0;

#define DXDBG 1

/************************************************************************/
/* DirectX graphic/video driver enable start here                       */
/************************************************************************/
BOOL
intEnableReactXDriver(HDC hdc)
{
    NTSTATUS Status;
    PEPROCESS Proc = NULL;
    PDC pDC = NULL;
    PPDEVOBJ pDev = NULL;
    PGD_DXDDENABLEDIRECTDRAW pfnDdEnableDirectDraw = NULL;
    BOOL success = FALSE;

    /* FIXME: Get the process data */

    /* Do not try load dxg.sys when it have already been load once */
    if (gpfnStartupDxGraphics == NULL)
    {
        Status = DxDdStartupDxGraphics(0,NULL,0,NULL,NULL, Proc);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Warning: Failed to create the directx interface\n");
            return FALSE;
        }
    }

    pDC = DC_LockDc(hdc);
    if (pDC == NULL)
    {
        DPRINT1("Warning: Failed to lock hdc\n");
        return FALSE;
    }

    pDev = pDC->ppdev;

    /* Test and see if drv got a DX interface or not */
    if  ( ( pDev->DriverFunctions.DisableDirectDraw == NULL) ||
          ( pDev->DriverFunctions.EnableDirectDraw == NULL))
    {
        DPRINT1("Waring : DisableDirectDraw and EnableDirectDraw are NULL, no dx driver \n");
    }
    else
    {

        /* Check and see if DX has been enabled or not */
        if ( pDev->pEDDgpl->pvmList == NULL)
        {
            pDev->pEDDgpl->ddCallbacks.dwSize = sizeof(DD_CALLBACKS);
            pDev->pEDDgpl->ddSurfaceCallbacks.dwSize = sizeof(DD_SURFACECALLBACKS);
            pDev->pEDDgpl->ddPaletteCallbacks.dwSize = sizeof(DD_PALETTECALLBACKS);

            pfnDdEnableDirectDraw = (PGD_DXDDENABLEDIRECTDRAW)gpDxFuncs[DXG_INDEX_DxDdEnableDirectDraw].pfn;
            if (pfnDdEnableDirectDraw == NULL)
            {
                DPRINT1("Warning: no pfnDdEnableDirectDraw\n");
            }
            else
            {
                DPRINT1(" call to pfnDdEnableDirectDraw \n ");

                /* Note: it is the hdev struct it wants, not the drv hPDev aka pdc->PDev */
                success = pfnDdEnableDirectDraw(pDC->ppdev, TRUE);

                dump_edd_directdraw_global(pDev->pEDDgpl);
                dump_halinfo(&pDev->pEDDgpl->ddHalInfo);
            }
        }
        else
        {
            DPRINT1(" The dxg.sys and graphic card driver interface is enabled \n ");
            success = TRUE;
        }
    }


    DPRINT1("Return value : 0x%08x\n",success);
    DC_UnlockDc(pDC);
    DPRINT1(" end call to pfnDdEnableDirectDraw \n ");
    return success;
}

/************************************************************************/
/* DirectX graphic/video driver enable ends here                        */
/************************************************************************/

/************************************************************************/
/* DirectX graphic/video driver loading and cleanup starts here         */
/************************************************************************/
NTSTATUS
APIENTRY
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

    /* FIXME: Setup of gaEngFuncs driver export list
     * but not in this api, we can add it here tempary until we figout where
     * no code have been writen for it yet
     */


    /* FIXME: ReactOS does not loading the dxapi.sys or import functions from it yet */
    // DxApiGetVersion()

    /* Loading the kernel interface of DirectX for win32k */

    DPRINT1("Warning: trying loading xp/2003/windows7/reactos dxg.sys\n");
    ghDxGraphics = EngLoadImage(L"\\SystemRoot\\System32\\drivers\\dxg.sys");
    if ( ghDxGraphics == NULL)
    {
        Status = STATUS_DLL_NOT_FOUND;
        DPRINT1("Warning: no ReactX or DirectX kernel driver found\n");
    }
    else
    {
        /* Import DxDdStartupDxGraphics and  DxDdCleanupDxGraphics */
        gpfnStartupDxGraphics = EngFindImageProcAddress(ghDxGraphics,"DxDdStartupDxGraphics");
        gpfnCleanupDxGraphics = EngFindImageProcAddress(ghDxGraphics,"DxDdCleanupDxGraphics");

        if ((gpfnStartupDxGraphics) &&
            (gpfnCleanupDxGraphics))
        {
            /* Setup driver data for activate the dx interface */
            DxEngDrv.iDriverVersion = DDI_DRIVER_VERSION_NT5_01;
            DxEngDrv.pdrvfn = gaEngFuncs;
            DxEngDrv.c = gcEngFuncs;

            Status = gpfnStartupDxGraphics ( sizeof(DRVENABLEDATA),
                                             &DxEngDrv,
                                             sizeof(DRVENABLEDATA),
                                             &DxgDrv,
                                             &gdwDirectDrawContext,
                                             Proc );
        }

        /* Check if we manage loading the data and execute the dxStartupDxGraphics if it is successful */
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
            /* Sort the drv functions list in index order, this allows us doing, smaller optimize
             * in API that are redirect to dx.sys
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
        /* Return the status */
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
APIENTRY
NtGdiDdCreateDirectDrawObject(HDC hdc)
{
    PGD_DDCREATEDIRECTDRAWOBJECT pfnDdCreateDirectDrawObject;

    if (hdc == NULL)
    {
        DPRINT1("Warning: hdc is NULL\n");
        return 0;
    }

    /* FIXME: This should be alloc for each drv and use it from each drv, not global for whole win32k */
    if (intEnableReactXDriver(hdc) == FALSE)
    {
        DPRINT1("Warning: Failed to start the DirectX interface from the graphic driver\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    /* Get the pfnDdCreateDirectDrawObject after we load the drv */
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
* It redirects to dxg.sys in Windows XP/2003, dxkrnl.sys in Vista and is fully implemented in win32k.sys in Windows 2000 and below
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
APIENTRY
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
APIENTRY
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
APIENTRY
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
APIENTRY
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
APIENTRY
NtGdiDdDeleteDirectDrawObject(HANDLE hDirectDrawLocal)
{
    PGD_DXDDDELETEDIRECTDRAWOBJECT pfnDdDeleteDirectDrawObject = (PGD_DXDDDELETEDIRECTDRAWOBJECT)gpDxFuncs[DXG_INDEX_DxDdDeleteDirectDrawObject].pfn;

    if (pfnDdDeleteDirectDrawObject == NULL)
    {
        DPRINT1("Warning: no pfnDdDeleteDirectDrawObject\n");
        return FALSE;
    }

    if (hDirectDrawLocal == NULL)
    {
         DPRINT1("Warning: hDirectDrawLocal is NULL\n");
         return FALSE;
    }

    DPRINT1("hDirectDrawLocal = %p \n", hDirectDrawLocal);
    DPRINT1("Calling dxg.sys pfnDdDeleteDirectDrawObject\n");

    return pfnDdDeleteDirectDrawObject(hDirectDrawLocal);
}

/************************************************************************/
/* NtGdiDdDeleteSurfaceObject                                           */
/************************************************************************/
BOOL
APIENTRY
NtGdiDdDeleteSurfaceObject(HANDLE hSurface)
{
    PGD_DXDDDELETESURFACEOBJECT pfnDdDeleteSurfaceObject = (PGD_DXDDDELETESURFACEOBJECT)gpDxFuncs[DXG_INDEX_DxDdDeleteSurfaceObject].pfn;

    if (pfnDdDeleteSurfaceObject == NULL)
    {
        DPRINT1("Warning: no pfnDdDeleteSurfaceObject\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }
    /* Try and see if the handle is valid */

    DPRINT1("Calling dxg.sys DdDeleteSurfaceObject\n");
    return pfnDdDeleteSurfaceObject(hSurface);
}

/************************************************************************/
/* NtGdiDdQueryDirectDrawObject                                         */
/************************************************************************/
BOOL
APIENTRY
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


    return pfnDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData,
                                      puD3dBufferCallbacks, puD3dTextureFormats, puNumHeaps, puvmList, puNumFourCC, puFourCC);

}


/************************************************************************/
/* NtGdiDdReenableDirectDrawObject                                      */
/************************************************************************/
BOOL
APIENTRY
NtGdiDdReenableDirectDrawObject(HANDLE hDirectDrawLocal,
                                BOOL *pubNewMode)
{
#if DXDBG
    BOOL status = FALSE;
#endif
    PGD_DXDDREENABLEDIRECTDRAWOBJECT pfnDdReenableDirectDrawObject = (PGD_DXDDREENABLEDIRECTDRAWOBJECT)gpDxFuncs[DXG_INDEX_DxDdReenableDirectDrawObject].pfn;

    if (pfnDdReenableDirectDrawObject == NULL)
    {
        DPRINT1("Warning: no pfnDdReenableDirectDrawObject\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdReenableDirectDrawObject\n");

#if DXDBG
    status = pfnDdReenableDirectDrawObject(hDirectDrawLocal, pubNewMode);
    DPRINT1("end Calling dxg.sys pfnDdReenableDirectDrawObject\n");
    DPRINT1("return value : 0x%08x\n", status);
    return status;
#else
    return pfnDdReenableDirectDrawObject(hDirectDrawLocal, pubNewMode);
#endif
}


/************************************************************************/
/* NtGdiDdGetDriverInfo                                                 */
/************************************************************************/
DWORD
APIENTRY
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
APIENTRY
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
APIENTRY
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
APIENTRY
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
APIENTRY
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
APIENTRY
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
APIENTRY
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
APIENTRY
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
APIENTRY
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


/* Internal debug API */
void dump_edd_directdraw_global(EDD_DIRECTDRAW_GLOBAL *pEddgbl)
{
    DPRINT1("0x%08lx 0x000 PEDD_DIRECTDRAW_GLOBAL->dhpdev                                         : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, dhpdev), pEddgbl->dhpdev);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->dwReserved1                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, dwReserved1),pEddgbl->dwReserved1);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->dwReserved2                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, dwReserved2),pEddgbl->dwReserved2);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_000c[0]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_000c[0]),pEddgbl->unk_000c[0]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_000c[1]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_000c[1]),pEddgbl->unk_000c[1]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_000c[2]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_000c[2]),pEddgbl->unk_000c[2]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->cDriverReferences                              : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, cDriverReferences),pEddgbl->cDriverReferences);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_01c                                        : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_01c),pEddgbl->unk_01c);

    DPRINT1("0x%08lx 0x020 PEDD_DIRECTDRAW_GLOBAL->dwCallbackFlags                                : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, dwCallbackFlags),pEddgbl->dwCallbackFlags);

    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_024                                        : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_024),pEddgbl->unk_024);

    DPRINT1("0x%08lx 0x028 PEDD_DIRECTDRAW_GLOBAL->llAssertModeTimeout                            : 0x%llx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, llAssertModeTimeout),pEddgbl->llAssertModeTimeout.QuadPart);
    DPRINT1("0x%08lx 0x030 PEDD_DIRECTDRAW_GLOBAL->dwNumHeaps                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, dwNumHeaps),pEddgbl->dwNumHeaps);
    // VIDEOMEMORY *pvmList;
    DPRINT1("0x%08lx 0x034 PEDD_DIRECTDRAW_GLOBAL->pvmList                                        : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, pvmList),pEddgbl->pvmList);

    DPRINT1("0x%08lx 0x038 PEDD_DIRECTDRAW_GLOBAL->dwNumFourCC                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, dwNumFourCC),pEddgbl->dwNumFourCC);
    DPRINT1("0x%08lx 0x03C PEDD_DIRECTDRAW_GLOBAL->pdwFourCC                                      : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, pdwFourCC),pEddgbl->pdwFourCC);

    // DD_HALINFO ddHalInfo;
    DPRINT1("0x%08lx 0x040 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.dwSize                               : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.dwSize),pEddgbl->ddHalInfo.dwSize);
    DPRINT1("0x%08lx 0x044 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.fpPrimary                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.fpPrimary),pEddgbl->ddHalInfo.vmiData.fpPrimary);
    DPRINT1("0x%08lx 0x048 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.dwFlags                      : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.dwFlags),pEddgbl->ddHalInfo.vmiData.dwFlags);
    DPRINT1("0x%08lx 0x04C PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.dwDisplayWidth               : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.dwDisplayWidth),pEddgbl->ddHalInfo.vmiData.dwDisplayWidth);
    DPRINT1("0x%08lx 0x050 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.dwDisplayHeight              : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.dwDisplayHeight),pEddgbl->ddHalInfo.vmiData.dwDisplayHeight);
    DPRINT1("0x%08lx 0x054 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.lDisplayPitch                : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.lDisplayPitch),pEddgbl->ddHalInfo.vmiData.lDisplayPitch);
    DPRINT1("0x%08lx 0x058 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.ddpfDisplay.dwSize           : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.ddpfDisplay.dwSize),pEddgbl->ddHalInfo.vmiData.ddpfDisplay.dwSize);
    DPRINT1("0x%08lx 0x05C PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.ddpfDisplay.dwFlags          : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.ddpfDisplay.dwFlags),pEddgbl->ddHalInfo.vmiData.ddpfDisplay.dwFlags);
    DPRINT1("0x%08lx 0x060 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.ddpfDisplay.dwFourCC         : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.ddpfDisplay.dwFourCC),pEddgbl->ddHalInfo.vmiData.ddpfDisplay.dwFourCC);
    DPRINT1("0x%08lx 0x064 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.ddpfDisplay.dwRGBBitCount    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.ddpfDisplay.dwRGBBitCount),pEddgbl->ddHalInfo.vmiData.ddpfDisplay.dwRGBBitCount);
    DPRINT1("0x%08lx 0x068 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.ddpfDisplay.dwRBitMask       : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.ddpfDisplay.dwRBitMask),pEddgbl->ddHalInfo.vmiData.ddpfDisplay.dwRBitMask);
    DPRINT1("0x%08lx 0x06C PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.ddpfDisplay.dwGBitMask       : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.ddpfDisplay.dwGBitMask),pEddgbl->ddHalInfo.vmiData.ddpfDisplay.dwGBitMask);
    DPRINT1("0x%08lx 0x070 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.ddpfDisplay.dwBBitMask       : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.ddpfDisplay.dwBBitMask),pEddgbl->ddHalInfo.vmiData.ddpfDisplay.dwGBitMask);
    DPRINT1("0x%08lx 0x074 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.ddpfDisplay.dwRGBAlphaBitMask : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.ddpfDisplay.dwRGBAlphaBitMask),pEddgbl->ddHalInfo.vmiData.ddpfDisplay.dwRGBAlphaBitMask);

    DPRINT1("0x%08lx 0x078 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.dwOffscreenAlign             : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.dwOffscreenAlign),pEddgbl->ddHalInfo.vmiData.dwOffscreenAlign);
    DPRINT1("0x%08lx 0x07C PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.dwOverlayAlign               : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.dwOverlayAlign ),pEddgbl->ddHalInfo.vmiData.dwOverlayAlign);
    DPRINT1("0x%08lx 0x080 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.dwTextureAlign               : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.dwTextureAlign),pEddgbl->ddHalInfo.vmiData.dwTextureAlign);
    DPRINT1("0x%08lx 0x084 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.dwZBufferAlign               : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.dwZBufferAlign),pEddgbl->ddHalInfo.vmiData.dwZBufferAlign);
    DPRINT1("0x%08lx 0x088 PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.dwAlphaAlign                 : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.dwAlphaAlign),pEddgbl->ddHalInfo.vmiData.dwAlphaAlign);
    DPRINT1("0x%08lx 0x08C PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.vmiData.pvPrimary                    : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.vmiData.pvPrimary),pEddgbl->ddHalInfo.vmiData.pvPrimary);
    DPRINT1("0x%08lx 0x08C PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.ddCaps.dwSize                        : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.ddCaps.dwSize),pEddgbl->ddHalInfo.ddCaps.dwSize);
    DPRINT1("0x%08lx 0x08C PEDD_DIRECTDRAW_GLOBAL->ddHalInfo.ddCaps.dwCaps                        : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddHalInfo.ddCaps.dwCaps),pEddgbl->ddHalInfo.ddCaps.dwCaps);



    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[0]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[0]),pEddgbl->unk_1e0[0]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[1]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[1]),pEddgbl->unk_1e0[1]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[2]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[2]),pEddgbl->unk_1e0[2]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[3]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[3]),pEddgbl->unk_1e0[3]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[4]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[4]),pEddgbl->unk_1e0[4]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[5]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[5]),pEddgbl->unk_1e0[5]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[6]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[6]),pEddgbl->unk_1e0[6]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[7]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[7]),pEddgbl->unk_1e0[7]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[8]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[8]),pEddgbl->unk_1e0[8]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[9]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[9]),pEddgbl->unk_1e0[9]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[10]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[10]),pEddgbl->unk_1e0[10]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[11]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[11]),pEddgbl->unk_1e0[11]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[12]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[12]),pEddgbl->unk_1e0[12]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[13]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[13]),pEddgbl->unk_1e0[13]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[14]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[14]),pEddgbl->unk_1e0[14]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[15]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[15]),pEddgbl->unk_1e0[15]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[16]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[16]),pEddgbl->unk_1e0[16]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[17]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[17]),pEddgbl->unk_1e0[17]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[18]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[18]),pEddgbl->unk_1e0[18]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[19]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[19]),pEddgbl->unk_1e0[19]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[20]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[20]),pEddgbl->unk_1e0[20]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[21]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[21]),pEddgbl->unk_1e0[21]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[22]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[22]),pEddgbl->unk_1e0[22]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[23]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[23]),pEddgbl->unk_1e0[23]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[24]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[24]),pEddgbl->unk_1e0[24]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[25]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[25]),pEddgbl->unk_1e0[25]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[26]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[26]),pEddgbl->unk_1e0[26]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[27]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[27]),pEddgbl->unk_1e0[27]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[28]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[28]),pEddgbl->unk_1e0[28]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[29]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[29]),pEddgbl->unk_1e0[29]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[30]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[30]),pEddgbl->unk_1e0[30]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[31]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[31]),pEddgbl->unk_1e0[31]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[32]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[32]),pEddgbl->unk_1e0[32]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[33]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[33]),pEddgbl->unk_1e0[33]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[34]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[34]),pEddgbl->unk_1e0[34]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[35]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[35]),pEddgbl->unk_1e0[35]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[36]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[36]),pEddgbl->unk_1e0[36]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[37]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[37]),pEddgbl->unk_1e0[37]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[38]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[38]),pEddgbl->unk_1e0[38]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[39]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[39]),pEddgbl->unk_1e0[39]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[40]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[40]),pEddgbl->unk_1e0[0]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[41]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[41]),pEddgbl->unk_1e0[41]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[42]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[42]),pEddgbl->unk_1e0[42]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[43]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[43]),pEddgbl->unk_1e0[43]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[44]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[44]),pEddgbl->unk_1e0[44]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[45]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_1e0[45]),pEddgbl->unk_1e0[45]);

    DPRINT1("0x%08lx 0x298 PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.dwSize                             : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.dwSize),pEddgbl->ddCallbacks.dwSize);
    DPRINT1("0x%08lx 0x29C PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.dwFlags                            : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.dwFlags),pEddgbl->ddCallbacks.dwFlags);
    DPRINT1("0x%08lx 0x2A0 PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.DestroyDriver                      : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.DestroyDriver),pEddgbl->ddCallbacks.DestroyDriver);
    DPRINT1("0x%08lx 0x2A4 PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.CreateSurface                      : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.CreateSurface),pEddgbl->ddCallbacks.CreateSurface);
    DPRINT1("0x%08lx 0x2A8 PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.SetColorKey                        : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.SetColorKey),pEddgbl->ddCallbacks.SetColorKey);
    DPRINT1("0x%08lx 0x2AC PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.SetMode                            : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.SetMode),pEddgbl->ddCallbacks.SetMode);
    DPRINT1("0x%08lx 0x2B0 PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.WaitForVerticalBlank               : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.WaitForVerticalBlank),pEddgbl->ddCallbacks.WaitForVerticalBlank);
    DPRINT1("0x%08lx 0x2B4 PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.CanCreateSurface                   : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.CanCreateSurface),pEddgbl->ddCallbacks.CanCreateSurface);
    DPRINT1("0x%08lx 0x2B8 PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.CreatePalette                      : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.CreatePalette),pEddgbl->ddCallbacks.CreatePalette);
    DPRINT1("0x%08lx 0x2BC PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.GetScanLine                        : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.GetScanLine),pEddgbl->ddCallbacks.GetScanLine);
    DPRINT1("0x%08lx 0x2C0 PEDD_DIRECTDRAW_GLOBAL->ddCallbacks.MapMemory                          : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddCallbacks.MapMemory),pEddgbl->ddCallbacks.MapMemory);


    DPRINT1("0x%08lx 0x2C4 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.dwSize                      : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.dwSize),pEddgbl->ddSurfaceCallbacks.dwSize);
    DPRINT1("0x%08lx 0x2C8 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.dwFlags                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.dwFlags),pEddgbl->ddSurfaceCallbacks.dwFlags);
    DPRINT1("0x%08lx 0x2CC PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.DestroySurface              : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.DestroySurface),pEddgbl->ddSurfaceCallbacks.DestroySurface);
    DPRINT1("0x%08lx 0x2D0 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.Flip                        : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.Flip),pEddgbl->ddSurfaceCallbacks.Flip);
    DPRINT1("0x%08lx 0x2D4 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.SetClipList                 : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.SetClipList),pEddgbl->ddSurfaceCallbacks.SetClipList);
    DPRINT1("0x%08lx 0x2D8 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.Lock                        : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.Lock),pEddgbl->ddSurfaceCallbacks.Lock);
    DPRINT1("0x%08lx 0x2DC PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.Unlock                      : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.Unlock),pEddgbl->ddSurfaceCallbacks.Unlock);
    DPRINT1("0x%08lx 0x2E0 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.Blt                         : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.Blt),pEddgbl->ddSurfaceCallbacks.Blt);
    DPRINT1("0x%08lx 0x2E4 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.SetColorKey                 : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.SetColorKey),pEddgbl->ddSurfaceCallbacks.SetColorKey);
    DPRINT1("0x%08lx 0x2E8 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.AddAttachedSurface          : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.AddAttachedSurface),pEddgbl->ddSurfaceCallbacks.AddAttachedSurface);
    DPRINT1("0x%08lx 0x2EC PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.GetBltStatus                : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.GetBltStatus),pEddgbl->ddSurfaceCallbacks.GetBltStatus);
    DPRINT1("0x%08lx 0x2F0 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.GetFlipStatus               : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.GetFlipStatus),pEddgbl->ddSurfaceCallbacks.GetFlipStatus);
    DPRINT1("0x%08lx 0x2F4 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.UpdateOverlay               : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.UpdateOverlay),pEddgbl->ddSurfaceCallbacks.UpdateOverlay);
    DPRINT1("0x%08lx 0x2F8 PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.SetOverlayPosition          : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.SetOverlayPosition),pEddgbl->ddSurfaceCallbacks.SetOverlayPosition);
    DPRINT1("0x%08lx 0x2FC PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks.reserved4                   : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddSurfaceCallbacks.reserved4),pEddgbl->ddSurfaceCallbacks.reserved4);

    DPRINT1("0x%08lx 0x300 PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks.dwSize                      : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddPaletteCallbacks.dwSize),pEddgbl->ddPaletteCallbacks.dwSize);
    DPRINT1("0x%08lx 0x304 PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks.dwFlags                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddPaletteCallbacks.dwFlags),pEddgbl->ddPaletteCallbacks.dwFlags);
    DPRINT1("0x%08lx 0x308 PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks.DestroyPalette              : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddPaletteCallbacks.DestroyPalette),pEddgbl->ddPaletteCallbacks.DestroyPalette);
    DPRINT1("0x%08lx 0x30C PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks.SetEntries                  : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddPaletteCallbacks.SetEntries),pEddgbl->ddPaletteCallbacks.SetEntries);

    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[0]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[0]),pEddgbl->unk_314[0]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[1]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[1]),pEddgbl->unk_314[1]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[2]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[2]),pEddgbl->unk_314[2]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[3]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[3]),pEddgbl->unk_314[3]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[4]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[4]),pEddgbl->unk_314[4]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[5]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[5]),pEddgbl->unk_314[5]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[6]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[6]),pEddgbl->unk_314[6]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[7]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[7]),pEddgbl->unk_314[7]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[8]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[8]),pEddgbl->unk_314[8]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[9]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[9]),pEddgbl->unk_314[9]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[10]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[10]),pEddgbl->unk_314[10]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[11]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[11]),pEddgbl->unk_314[11]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[12]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[12]),pEddgbl->unk_314[12]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[13]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[13]),pEddgbl->unk_314[13]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[14]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[14]),pEddgbl->unk_314[14]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[15]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[15]),pEddgbl->unk_314[15]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[16]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[16]),pEddgbl->unk_314[16]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[17]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[17]),pEddgbl->unk_314[17]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[18]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[18]),pEddgbl->unk_314[18]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[19]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[19]),pEddgbl->unk_314[19]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[20]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[20]),pEddgbl->unk_314[20]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[21]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[21]),pEddgbl->unk_314[21]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[22]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[22]),pEddgbl->unk_314[22]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[23]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[23]),pEddgbl->unk_314[23]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[24]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[24]),pEddgbl->unk_314[24]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[25]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[25]),pEddgbl->unk_314[25]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[26]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[26]),pEddgbl->unk_314[26]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[27]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[27]),pEddgbl->unk_314[27]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[28]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[28]),pEddgbl->unk_314[28]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[29]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[29]),pEddgbl->unk_314[29]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[30]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[30]),pEddgbl->unk_314[30]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[31]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[31]),pEddgbl->unk_314[31]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[32]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[32]),pEddgbl->unk_314[32]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[33]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[33]),pEddgbl->unk_314[33]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[34]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[34]),pEddgbl->unk_314[34]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[35]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[35]),pEddgbl->unk_314[35]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[36]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[36]),pEddgbl->unk_314[36]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[37]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[37]),pEddgbl->unk_314[37]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[38]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[38]),pEddgbl->unk_314[38]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[39]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[39]),pEddgbl->unk_314[39]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[40]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[40]),pEddgbl->unk_314[40]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[41]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[41]),pEddgbl->unk_314[41]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[42]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[42]),pEddgbl->unk_314[42]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[43]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[43]),pEddgbl->unk_314[43]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[44]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[44]),pEddgbl->unk_314[44]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_314[45]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_314[45]),pEddgbl->unk_314[45]);
    // D3DNTHAL_CALLBACKS d3dNtHalCallbacks;
    //DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks                              : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, d3dNtHalCallbacks),pEddgbl->d3dNtHalCallbacks);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_460[0]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_460[0]),pEddgbl->unk_460[0]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_460[1]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_460[1]),pEddgbl->unk_460[1]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_460[2]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_460[2]),pEddgbl->unk_460[2]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_460[3]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_460[3]),pEddgbl->unk_460[3]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_460[4]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_460[4]),pEddgbl->unk_460[4]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_460[5]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_460[5]),pEddgbl->unk_460[5]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_460[6]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_460[6]),pEddgbl->unk_460[6]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_460[7]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_460[7]),pEddgbl->unk_460[7]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_460[8]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_460[8]),pEddgbl->unk_460[8]);
    // D3DNTHAL_CALLBACKS2 d3dNtHalCallbacks2;
    //DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks2                             : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, d3dNtHalCallbacks2),pEddgbl->d3dNtHalCallbacks2);

    DPRINT1("0x%08lx 0x498 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.dwSize                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.dwSize),pEddgbl->ddVideoPortCallback.dwSize);
    DPRINT1("0x%08lx 0x49C PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.dwFlags                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.dwFlags),pEddgbl->ddVideoPortCallback.dwFlags);
    DPRINT1("0x%08lx 0x4A0 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.CanCreateVideoPort         : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.CanCreateVideoPort),pEddgbl->ddVideoPortCallback.CanCreateVideoPort);
    DPRINT1("0x%08lx 0x4A4 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.CreateVideoPort            : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.CreateVideoPort),pEddgbl->ddVideoPortCallback.CreateVideoPort);
    DPRINT1("0x%08lx 0x4A8 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.FlipVideoPort              : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.FlipVideoPort),pEddgbl->ddVideoPortCallback.FlipVideoPort);
    DPRINT1("0x%08lx 0x4AC PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortBandwidth      : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.GetVideoPortBandwidth),pEddgbl->ddVideoPortCallback.GetVideoPortBandwidth);
    DPRINT1("0x%08lx 0x4B0 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortInputFormats   : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.GetVideoPortInputFormats),pEddgbl->ddVideoPortCallback.GetVideoPortInputFormats);
    DPRINT1("0x%08lx 0x4B4 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortOutputFormats  : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.GetVideoPortOutputFormats),pEddgbl->ddVideoPortCallback.GetVideoPortOutputFormats);
    DPRINT1("0x%08lx 0x4B8 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.lpReserved1                : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.lpReserved1),pEddgbl->ddVideoPortCallback.lpReserved1);
    DPRINT1("0x%08lx 0x4BC PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortField          : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.GetVideoPortField),pEddgbl->ddVideoPortCallback.GetVideoPortField);
    DPRINT1("0x%08lx 0x4C0 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortLine           : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.GetVideoPortLine),pEddgbl->ddVideoPortCallback.GetVideoPortLine);
    DPRINT1("0x%08lx 0x4C4 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortConnectInfo    : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.GetVideoPortConnectInfo),pEddgbl->ddVideoPortCallback.GetVideoPortConnectInfo);
    DPRINT1("0x%08lx 0x4C8 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.DestroyVideoPort           : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.DestroyVideoPort),pEddgbl->ddVideoPortCallback.DestroyVideoPort);
    DPRINT1("0x%08lx 0x4CC PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoPortFlipStatus     : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.GetVideoPortFlipStatus),pEddgbl->ddVideoPortCallback.GetVideoPortFlipStatus);
    DPRINT1("0x%08lx 0x4D0 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.UpdateVideoPort            : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.UpdateVideoPort),pEddgbl->ddVideoPortCallback.UpdateVideoPort);
    DPRINT1("0x%08lx 0x4D4 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.WaitForVideoPortSync       : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.WaitForVideoPortSync),pEddgbl->ddVideoPortCallback.WaitForVideoPortSync);
    DPRINT1("0x%08lx 0x4D8 PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.GetVideoSignalStatus       : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.GetVideoSignalStatus),pEddgbl->ddVideoPortCallback.GetVideoSignalStatus);
    DPRINT1("0x%08lx 0x4DC PEDD_DIRECTDRAW_GLOBAL->ddVideoPortCallback.ColorControl               : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddVideoPortCallback.ColorControl),pEddgbl->ddVideoPortCallback.ColorControl);

    DPRINT1("0x%08lx 0x4E0 PEDD_DIRECTDRAW_GLOBAL->ddMiscellanousCallbacks.dwSize                 : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddMiscellanousCallbacks.dwSize),pEddgbl->ddMiscellanousCallbacks.dwSize);
    DPRINT1("0x%08lx 0x4E4 PEDD_DIRECTDRAW_GLOBAL->ddMiscellanousCallbacks.dwFlags                : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddMiscellanousCallbacks.dwFlags),pEddgbl->ddMiscellanousCallbacks.dwFlags);
    DPRINT1("0x%08lx 0x4E8 PEDD_DIRECTDRAW_GLOBAL->ddMiscellanousCallbacks.GetAvailDriverMemory   : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddMiscellanousCallbacks.GetAvailDriverMemory),pEddgbl->ddMiscellanousCallbacks.GetAvailDriverMemory);

    DPRINT1("0x%08lx 0x4EC PEDD_DIRECTDRAW_GLOBAL->ddMiscellanous2Callbacks.dwSize                : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddMiscellanous2Callbacks.dwSize),pEddgbl->ddMiscellanous2Callbacks.dwSize);
    DPRINT1("0x%08lx 0x4F0 PEDD_DIRECTDRAW_GLOBAL->ddMiscellanous2Callbacks.dwFlags               : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddMiscellanous2Callbacks.dwFlags),pEddgbl->ddMiscellanous2Callbacks.dwFlags);
    DPRINT1("0x%08lx 0x4F4 PEDD_DIRECTDRAW_GLOBAL->ddMiscellanous2Callbacks.AlphaBlt              : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddMiscellanous2Callbacks.AlphaBlt),pEddgbl->ddMiscellanous2Callbacks.AlphaBlt);
    DPRINT1("0x%08lx 0x4F8 PEDD_DIRECTDRAW_GLOBAL->ddMiscellanous2Callbacks.CreateSurfaceEx       : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddMiscellanous2Callbacks.CreateSurfaceEx),pEddgbl->ddMiscellanous2Callbacks.CreateSurfaceEx);
    DPRINT1("0x%08lx 0x4FC PEDD_DIRECTDRAW_GLOBAL->ddMiscellanous2Callbacks.GetDriverState        : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddMiscellanous2Callbacks.GetDriverState),pEddgbl->ddMiscellanous2Callbacks.GetDriverState);
    DPRINT1("0x%08lx 0x500 PEDD_DIRECTDRAW_GLOBAL->ddMiscellanous2Callbacks.DestroyDDLocal        : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, ddMiscellanous2Callbacks.DestroyDDLocal),pEddgbl->ddMiscellanous2Callbacks.DestroyDDLocal);

    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[0]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[0]),pEddgbl->unk_504[0]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[1]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[1]),pEddgbl->unk_504[1]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[2]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[2]),pEddgbl->unk_504[2]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[3]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[3]),pEddgbl->unk_504[3]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[4]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[4]),pEddgbl->unk_504[4]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[5]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[5]),pEddgbl->unk_504[5]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[6]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[6]),pEddgbl->unk_504[6]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[7]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[7]),pEddgbl->unk_504[7]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[8]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[8]),pEddgbl->unk_504[8]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_504[9]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_504[9]),pEddgbl->unk_504[9]);

    // D3DNTHAL_CALLBACKS3 d3dNtHalCallbacks3;
    //DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks3                             : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, d3dNtHalCallbacks3),pEddgbl->d3dNtHalCallbacks3);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_544                                        : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_544), pEddgbl->unk_544);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_548                                        : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_548), pEddgbl->unk_548);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[0]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[0]),pEddgbl->unk_54c[0]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[1]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[1]),pEddgbl->unk_54c[1]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[2]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[2]),pEddgbl->unk_54c[2]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[3]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[3]),pEddgbl->unk_54c[3]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[4]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[4]),pEddgbl->unk_54c[4]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[5]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[5]),pEddgbl->unk_54c[5]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[6]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[6]),pEddgbl->unk_54c[6]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[7]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[7]),pEddgbl->unk_54c[7]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[8]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[8]),pEddgbl->unk_54c[8]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[9]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[9]),pEddgbl->unk_54c[9]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[10]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[10]),pEddgbl->unk_54c[10]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[11]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[11]),pEddgbl->unk_54c[11]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[12]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[12]),pEddgbl->unk_54c[12]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[13]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[13]),pEddgbl->unk_54c[13]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[14]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[14]),pEddgbl->unk_54c[14]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[15]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[15]),pEddgbl->unk_54c[15]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[16]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[16]),pEddgbl->unk_54c[16]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[17]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[17]),pEddgbl->unk_54c[17]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[18]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[18]),pEddgbl->unk_54c[18]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[19]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[19]),pEddgbl->unk_54c[19]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[20]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[20]),pEddgbl->unk_54c[20]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[21]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[21]),pEddgbl->unk_54c[21]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_54c[22]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_54c[22]),pEddgbl->unk_54c[22]);
    // EDD_DIRECTDRAW_LOCAL* peDirectDrawLocalList;
    DPRINT1("0x%08lx 0x5A8 PEDD_DIRECTDRAW_GLOBAL->peDirectDrawLocalList                          : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, peDirectDrawLocalList), pEddgbl->peDirectDrawLocalList);
    // EDD_SURFACE* peSurface_LockList;
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->peSurface_LockList                             : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, peSurface_LockList), pEddgbl->peSurface_LockList);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->fl                                             : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, fl), pEddgbl->fl);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->cSurfaceLocks                                  : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, cSurfaceLocks), pEddgbl->cSurfaceLocks);
    // PKEVENT pAssertModeEvent;
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->pAssertModeEvent                               : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, pAssertModeEvent), pEddgbl->pAssertModeEvent);
    // EDD_SURFACE *peSurfaceCurrent;
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->peSurfaceCurrent                               : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, peSurfaceCurrent), pEddgbl->peSurfaceCurrent);
    // EDD_SURFACE *peSurfacePrimary;
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->peSurfacePrimary                               : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, peSurfacePrimary),pEddgbl->peSurfacePrimary);
    DPRINT1("0x%08lx 0x5C4 PEDD_DIRECTDRAW_GLOBAL->bSuspended                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, bSuspended),pEddgbl->bSuspended);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[0]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[0]),pEddgbl->unk_5c8[0]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[1]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[1]),pEddgbl->unk_5c8[1]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[2]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[2]),pEddgbl->unk_5c8[2]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[3]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[3]),pEddgbl->unk_5c8[3]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[4]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[4]),pEddgbl->unk_5c8[4]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[5]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[5]),pEddgbl->unk_5c8[5]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[6]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[6]),pEddgbl->unk_5c8[6]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[7]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[7]),pEddgbl->unk_5c8[7]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[8]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[8]),pEddgbl->unk_5c8[8]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[9]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[9]),pEddgbl->unk_5c8[9]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[10]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[10]),pEddgbl->unk_5c8[10]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_5c8[11]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_5c8[11]),pEddgbl->unk_5c8[11]);
    // RECTL rcbounds;
    //DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->rcbounds                                       : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, rcbounds),pEddgbl->rcbounds);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_608                                        : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_608), pEddgbl->unk_608);
    DPRINT1("0x%08lx 0x60C PEDD_DIRECTDRAW_GLOBAL->hDev                                           : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, hDev), pEddgbl->hDev);

    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[0]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[0]), pEddgbl->unk_610[0]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[1]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[1]), pEddgbl->unk_610[1]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[2]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[2]), pEddgbl->unk_610[2]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[3]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[3]), pEddgbl->unk_610[3]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[4]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[4]), pEddgbl->unk_610[4]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[5]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[5]), pEddgbl->unk_610[5]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[6]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[6]), pEddgbl->unk_610[6]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[7]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[7]), pEddgbl->unk_610[7]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[8]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[8]), pEddgbl->unk_610[8]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[9]                                     : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[9]), pEddgbl->unk_610[9]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[10]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[10]), pEddgbl->unk_610[10]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[11]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[11]), pEddgbl->unk_610[11]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[12]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[12]), pEddgbl->unk_610[12]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[13]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[13]), pEddgbl->unk_610[13]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[14]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[14]), pEddgbl->unk_610[14]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[15]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[15]), pEddgbl->unk_610[15]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[16]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[16]), pEddgbl->unk_610[16]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[17]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[17]), pEddgbl->unk_610[17]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[18]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[18]), pEddgbl->unk_610[18]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[19]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[19]), pEddgbl->unk_610[19]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[20]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[20]), pEddgbl->unk_610[20]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[21]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[21]), pEddgbl->unk_610[21]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[22]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[22]), pEddgbl->unk_610[22]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[23]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[23]), pEddgbl->unk_610[23]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[24]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[24]), pEddgbl->unk_610[24]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[25]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[25]), pEddgbl->unk_610[25]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[26]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[26]), pEddgbl->unk_610[26]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[27]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[27]), pEddgbl->unk_610[27]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[28]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[28]), pEddgbl->unk_610[28]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[29]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[29]), pEddgbl->unk_610[29]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[30]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[30]), pEddgbl->unk_610[30]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[31]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[31]), pEddgbl->unk_610[31]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[32]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[32]), pEddgbl->unk_610[32]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[33]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[33]), pEddgbl->unk_610[33]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[34]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[34]), pEddgbl->unk_610[34]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[35]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[35]), pEddgbl->unk_610[35]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[36]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[36]), pEddgbl->unk_610[36]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[37]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[37]), pEddgbl->unk_610[37]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[38]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[38]), pEddgbl->unk_610[38]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[39]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[39]), pEddgbl->unk_610[39]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[40]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[40]), pEddgbl->unk_610[40]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[41]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[41]), pEddgbl->unk_610[41]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[42]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[42]), pEddgbl->unk_610[42]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[43]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[43]), pEddgbl->unk_610[43]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[44]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[44]), pEddgbl->unk_610[44]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[45]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[45]), pEddgbl->unk_610[45]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[46]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[46]), pEddgbl->unk_610[46]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[47]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[47]), pEddgbl->unk_610[47]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[48]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[48]), pEddgbl->unk_610[48]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[49]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[49]), pEddgbl->unk_610[49]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[50]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[50]), pEddgbl->unk_610[50]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[51]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[51]), pEddgbl->unk_610[51]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[52]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[52]), pEddgbl->unk_610[52]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[53]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[53]), pEddgbl->unk_610[53]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[54]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[54]), pEddgbl->unk_610[54]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[55]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[55]), pEddgbl->unk_610[55]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[56]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[56]), pEddgbl->unk_610[56]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[57]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[57]), pEddgbl->unk_610[57]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[58]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[58]), pEddgbl->unk_610[58]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[59]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[59]), pEddgbl->unk_610[59]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_1e0[60]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[60]), pEddgbl->unk_610[60]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[61]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[61]), pEddgbl->unk_610[61]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_610[62]                                    : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_610[62]), pEddgbl->unk_610[62]);
    DPRINT1("0x%08lx ????? PEDD_DIRECTDRAW_GLOBAL->unk_70C                                        : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_GLOBAL, unk_70C), pEddgbl->unk_70C);
}

void dump_edd_directdraw_local(PEDD_DIRECTDRAW_LOCAL pEddlcl)
{
    //DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->pobj                     : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, pobj), pEddlcl->pobj);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->peDirectDrawGlobal       : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, peDirectDrawGlobal), pEddlcl->peDirectDrawGlobal);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->peSurface_DdList         : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, peSurface_DdList), pEddlcl->peSurface_DdList);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_018                  : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_018), pEddlcl->unk_018);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_01c                  : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_01c), pEddlcl->unk_01c);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_020                  : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_020), pEddlcl->unk_020);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->peDirectDrawGlobal2      : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, peDirectDrawGlobal2), pEddlcl->peDirectDrawGlobal2);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->fpProcess                : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, fpProcess), pEddlcl->fpProcess);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->fl                       : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, fl), pEddlcl->fl);

    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->peDirectDrawLocal_prev   : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, peDirectDrawLocal_prev), pEddlcl->peDirectDrawLocal_prev);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->Process                  : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, Process), pEddlcl->Process);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_038                  : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_038), pEddlcl->unk_038);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->UniqueProcess            : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, UniqueProcess), pEddlcl->UniqueProcess);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_040                  : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_040), pEddlcl->unk_040);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_044                  : 0x%p\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_044), pEddlcl->unk_044);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_048                  : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_048), pEddlcl->unk_048);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_04C                  : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_04C), pEddlcl->unk_04C);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_050                  : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_050), pEddlcl->unk_050);

    DPRINT1("0x%08lx PEDD_DIRECTDRAW_LOCAL->unk_050                  : 0x%08lx\n",FIELD_OFFSET(EDD_DIRECTDRAW_LOCAL, unk_050), pEddlcl->unk_050);
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
        DPRINT1(" pHalInfo4->vmiData->pvPrimary                      : 0x%p\n",pHalInfo4->vmiData.pvPrimary);

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


        DPRINT1(" pHalInfo4->GetDriverInfo                           : 0x%p\n",pHalInfo4->GetDriverInfo);
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
        DPRINT1(" pHalInfo->vmiData->pvPrimary                      : 0x%p\n",pHalInfo->vmiData.pvPrimary);

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

        DPRINT1(" pHalInfo->GetDriverInfo                           : 0x%p\n",pHalInfo->GetDriverInfo);
        DPRINT1(" pHalInfo->dwFlags                                 : ");

        flag = pHalInfo->dwFlags;
        count = 0;
        checkflag(flag,DDHALINFO_ISPRIMARYDISPLAY,"DDHALINFO_ISPRIMARYDISPLAY");
        checkflag(flag,DDHALINFO_MODEXILLEGAL,"DDHALINFO_MODEXILLEGAL");
        checkflag(flag,DDHALINFO_GETDRIVERINFOSET,"DDHALINFO_GETDRIVERINFOSET");
        checkflag(flag,DDHALINFO_GETDRIVERINFO2,"DDHALINFO_GETDRIVERINFO2");
        endcheckflag(flag,"pHalInfo->dwFlags");

        DPRINT1(" pHalInfo->lpD3DGlobalDriverData                   : 0x%p\n",pHalInfo->lpD3DGlobalDriverData);
        DPRINT1(" pHalInfo->lpD3DHALCallbacks                       : 0x%p\n",pHalInfo->lpD3DHALCallbacks);
        DPRINT1(" pHalInfo->lpD3DBufCallbacks                       : 0x%p\n",pHalInfo->lpD3DBufCallbacks);
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
