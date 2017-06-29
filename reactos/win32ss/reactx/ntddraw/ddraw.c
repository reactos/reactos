/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             win32ss/reactx/ntddraw/ddraw.c
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
        DPRINT1("Warning : DisableDirectDraw and EnableDirectDraw are NULL, no dx driver \n");
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
