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
    HANDLE DxHandle;

    if (hdc == NULL)
    {
        DPRINT1("Warning : hdc is NULL\n");
        return 0;
    }

    /* FIXME get the process data */
    /* FIXME this code should be add where the driver being load */
    Status = DxDdStartupDxGraphics(0,NULL,0,NULL,NULL, Proc);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Warning : Failed to create the directx interface\n");
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
    ((PGDIDEVICE)pDC->pPDev)->pEDDgpl = &edd_DdirectDraw_Global;
    RtlZeroMemory(&edd_DdirectDraw_Global,sizeof(EDD_DIRECTDRAW_GLOBAL));



    /* setup hdev for edd_DdirectDraw_Global xp */
    edd_DdirectDraw_Global.hPDev = (PVOID)pDC->pPDev;

    DC_UnlockDc(pDC);

    /* get the pfnDdCreateDirectDrawObject after we load the drv */
    pfnDdCreateDirectDrawObject = (PGD_DDCREATEDIRECTDRAWOBJECT)gpDxFuncs[DXG_INDEX_DxDdCreateDirectDrawObject].pfn;
  
    if (pfnDdCreateDirectDrawObject == NULL)
    {
        DPRINT1("Warning: no pfnDdCreateDirectDrawObject\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys DdCreateDirectDrawObject\n");
    DxHandle = pfnDdCreateDirectDrawObject(hdc);

#if DXDDRAWDEBUG
    dump_edd_directdraw_global(&edd_DdirectDraw_Global);
#endif

    return DxHandle;
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
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dhpdev                  : 0x%08lx\n",(((DWORD)&pEddgbl->dhpdev) - (DWORD)pEddgbl), pEddgbl->dhpdev);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dwReserved1             : 0x%08lx\n",(((DWORD)&pEddgbl->dwReserved1) - (DWORD)pEddgbl),pEddgbl->dwReserved1);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dwReserved2             : 0x%08lx\n",(((DWORD)&pEddgbl->dwReserved2) - (DWORD)pEddgbl),pEddgbl->dwReserved2);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_000c[0]             : 0x%08lx\n",(((DWORD)&pEddgbl->unk_000c) - (DWORD)pEddgbl),pEddgbl->unk_000c[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_000c[1]             : 0x%08lx\n",(((DWORD)&pEddgbl->unk_000c) - (DWORD)pEddgbl),pEddgbl->unk_000c[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_000c[2]             : 0x%08lx\n",(((DWORD)&pEddgbl->unk_000c) - (DWORD)pEddgbl),pEddgbl->unk_000c[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->cDriverReferences       : 0x%08lx\n",(((DWORD)&pEddgbl->cDriverReferences) - (DWORD)pEddgbl),pEddgbl->cDriverReferences);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_01c[0]              : 0x%08lx\n",(((DWORD)&pEddgbl->unk_01c) - (DWORD)pEddgbl),pEddgbl->unk_01c[0]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_01c[1]              : 0x%08lx\n",(((DWORD)&pEddgbl->unk_01c) - (DWORD)pEddgbl),pEddgbl->unk_01c[1]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_01c[2]              : 0x%08lx\n",(((DWORD)&pEddgbl->unk_01c) - (DWORD)pEddgbl),pEddgbl->unk_01c[2]);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->llAssertModeTimeout     : 0x%llx\n",(((DWORD)&pEddgbl->llAssertModeTimeout) - (DWORD)pEddgbl),pEddgbl->llAssertModeTimeout);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->dwNumHeaps              : 0x%08lx\n",(((DWORD)&pEddgbl->dwNumHeaps) - (DWORD)pEddgbl),pEddgbl->dwNumHeaps);
    // VIDEOMEMORY *pvmList;
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->pvmList                 : 0x%08lx\n",(((DWORD)&pEddgbl->pvmList) - (DWORD)pEddgbl),pEddgbl->pvmList);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->dwNumFourCC             : 0x%08lx\n",pEddgbl->dwNumFourCC);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->pdwFourCC               : 0x%08lx\n",pEddgbl->pdwFourCC);
    // DD_HALINFO ddHalInfo;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->ddHalInfo               : 0x%08lx\n",pEddgbl->ddHalInfo);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[0]              : 0x%08lx\n",pEddgbl->unk_1e0[0]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[1]              : 0x%08lx\n",pEddgbl->unk_1e0[1]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[2]              : 0x%08lx\n",pEddgbl->unk_1e0[2]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[3]              : 0x%08lx\n",pEddgbl->unk_1e0[3]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[4]              : 0x%08lx\n",pEddgbl->unk_1e0[4]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[5]              : 0x%08lx\n",pEddgbl->unk_1e0[5]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[6]              : 0x%08lx\n",pEddgbl->unk_1e0[6]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[7]              : 0x%08lx\n",pEddgbl->unk_1e0[7]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[8]              : 0x%08lx\n",pEddgbl->unk_1e0[8]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[9]              : 0x%08lx\n",pEddgbl->unk_1e0[9]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[10]             : 0x%08lx\n",pEddgbl->unk_1e0[10]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[11]             : 0x%08lx\n",pEddgbl->unk_1e0[11]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[12]             : 0x%08lx\n",pEddgbl->unk_1e0[12]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[13]             : 0x%08lx\n",pEddgbl->unk_1e0[13]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[14]             : 0x%08lx\n",pEddgbl->unk_1e0[14]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[15]             : 0x%08lx\n",pEddgbl->unk_1e0[15]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[16]             : 0x%08lx\n",pEddgbl->unk_1e0[16]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[17]             : 0x%08lx\n",pEddgbl->unk_1e0[17]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[18]             : 0x%08lx\n",pEddgbl->unk_1e0[18]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[19]             : 0x%08lx\n",pEddgbl->unk_1e0[19]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[20]             : 0x%08lx\n",pEddgbl->unk_1e0[20]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[21]             : 0x%08lx\n",pEddgbl->unk_1e0[21]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[22]             : 0x%08lx\n",pEddgbl->unk_1e0[22]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[23]             : 0x%08lx\n",pEddgbl->unk_1e0[23]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[24]             : 0x%08lx\n",pEddgbl->unk_1e0[24]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[25]             : 0x%08lx\n",pEddgbl->unk_1e0[25]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[26]             : 0x%08lx\n",pEddgbl->unk_1e0[26]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[27]             : 0x%08lx\n",pEddgbl->unk_1e0[27]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[28]             : 0x%08lx\n",pEddgbl->unk_1e0[28]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[29]             : 0x%08lx\n",pEddgbl->unk_1e0[29]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[30]             : 0x%08lx\n",pEddgbl->unk_1e0[30]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[31]             : 0x%08lx\n",pEddgbl->unk_1e0[31]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[32]             : 0x%08lx\n",pEddgbl->unk_1e0[32]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[33]             : 0x%08lx\n",pEddgbl->unk_1e0[33]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[34]             : 0x%08lx\n",pEddgbl->unk_1e0[34]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[35]             : 0x%08lx\n",pEddgbl->unk_1e0[35]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[36]             : 0x%08lx\n",pEddgbl->unk_1e0[36]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[37]             : 0x%08lx\n",pEddgbl->unk_1e0[37]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[38]             : 0x%08lx\n",pEddgbl->unk_1e0[38]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[39]             : 0x%08lx\n",pEddgbl->unk_1e0[39]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[40]             : 0x%08lx\n",pEddgbl->unk_1e0[0]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[41]             : 0x%08lx\n",pEddgbl->unk_1e0[41]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[42]             : 0x%08lx\n",pEddgbl->unk_1e0[42]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[43]             : 0x%08lx\n",pEddgbl->unk_1e0[43]);
    // DD_CALLBACKS ddCallbacks;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->ddCallbacks             : 0x%08lx\n",pEddgbl->ddCallbacks);
    // DD_SURFACECALLBACKS ddSurfaceCallbacks;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->ddSurfaceCallbacks      : 0x%08lx\n",pEddgbl->ddSurfaceCallbacks);
    // DD_PALETTECALLBACKS ddPaletteCallbacks;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks      : 0x%08lx\n",pEddgbl->ddPaletteCallbacks);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[0]              : 0x%08lx\n",pEddgbl->unk_314[0]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[1]              : 0x%08lx\n",pEddgbl->unk_314[1]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[2]              : 0x%08lx\n",pEddgbl->unk_314[2]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[3]              : 0x%08lx\n",pEddgbl->unk_314[3]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[4]              : 0x%08lx\n",pEddgbl->unk_314[4]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[5]              : 0x%08lx\n",pEddgbl->unk_314[5]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[6]              : 0x%08lx\n",pEddgbl->unk_314[6]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[7]              : 0x%08lx\n",pEddgbl->unk_314[7]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[8]              : 0x%08lx\n",pEddgbl->unk_314[8]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[9]              : 0x%08lx\n",pEddgbl->unk_314[9]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[10]             : 0x%08lx\n",pEddgbl->unk_314[10]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[11]             : 0x%08lx\n",pEddgbl->unk_314[11]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[12]             : 0x%08lx\n",pEddgbl->unk_314[12]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[13]             : 0x%08lx\n",pEddgbl->unk_314[13]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[14]             : 0x%08lx\n",pEddgbl->unk_314[14]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[15]             : 0x%08lx\n",pEddgbl->unk_314[15]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[16]             : 0x%08lx\n",pEddgbl->unk_314[16]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[17]             : 0x%08lx\n",pEddgbl->unk_314[17]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[18]             : 0x%08lx\n",pEddgbl->unk_314[18]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[19]             : 0x%08lx\n",pEddgbl->unk_314[19]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[20]             : 0x%08lx\n",pEddgbl->unk_314[20]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[21]             : 0x%08lx\n",pEddgbl->unk_314[21]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[22]             : 0x%08lx\n",pEddgbl->unk_314[22]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[23]             : 0x%08lx\n",pEddgbl->unk_314[23]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[24]             : 0x%08lx\n",pEddgbl->unk_314[24]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[25]             : 0x%08lx\n",pEddgbl->unk_314[25]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[26]             : 0x%08lx\n",pEddgbl->unk_314[26]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[27]             : 0x%08lx\n",pEddgbl->unk_314[27]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[28]             : 0x%08lx\n",pEddgbl->unk_314[28]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[29]             : 0x%08lx\n",pEddgbl->unk_314[29]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[30]             : 0x%08lx\n",pEddgbl->unk_314[30]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[31]             : 0x%08lx\n",pEddgbl->unk_314[31]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[32]             : 0x%08lx\n",pEddgbl->unk_314[32]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[33]             : 0x%08lx\n",pEddgbl->unk_314[33]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[34]             : 0x%08lx\n",pEddgbl->unk_314[34]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[35]             : 0x%08lx\n",pEddgbl->unk_314[35]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[36]             : 0x%08lx\n",pEddgbl->unk_314[36]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[37]             : 0x%08lx\n",pEddgbl->unk_314[37]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[38]             : 0x%08lx\n",pEddgbl->unk_314[38]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[39]             : 0x%08lx\n",pEddgbl->unk_314[39]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[40]             : 0x%08lx\n",pEddgbl->unk_314[40]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[41]             : 0x%08lx\n",pEddgbl->unk_314[41]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[42]             : 0x%08lx\n",pEddgbl->unk_314[42]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[43]             : 0x%08lx\n",pEddgbl->unk_314[43]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[44]             : 0x%08lx\n",pEddgbl->unk_314[44]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[45]             : 0x%08lx\n",pEddgbl->unk_314[45]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[46]             : 0x%08lx\n",pEddgbl->unk_314[46]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_314[47]             : 0x%08lx\n",pEddgbl->unk_314[47]);
    // D3DNTHAL_CALLBACKS d3dNtHalCallbacks;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks       : 0x%08lx\n",pEddgbl->d3dNtHalCallbacks);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_460[0]              : 0x%08lx\n",pEddgbl->unk_460[0]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_460[1]              : 0x%08lx\n",pEddgbl->unk_460[1]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_460[2]              : 0x%08lx\n",pEddgbl->unk_460[2]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_460[3]              : 0x%08lx\n",pEddgbl->unk_460[3]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_460[4]              : 0x%08lx\n",pEddgbl->unk_460[4]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_460[5]              : 0x%08lx\n",pEddgbl->unk_460[5]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_460[6]              : 0x%08lx\n",pEddgbl->unk_460[6]);
    // D3DNTHAL_CALLBACKS2 d3dNtHalCallbacks2;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks2      : 0x%08lx\n",pEddgbl->d3dNtHalCallbacks2);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[0]              : 0x%08lx\n",pEddgbl->unk_498[0]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[1]              : 0x%08lx\n",pEddgbl->unk_498[1]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[2]              : 0x%08lx\n",pEddgbl->unk_498[2]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[3]              : 0x%08lx\n",pEddgbl->unk_498[3]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[4]              : 0x%08lx\n",pEddgbl->unk_498[4]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[5]              : 0x%08lx\n",pEddgbl->unk_498[5]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[6]              : 0x%08lx\n",pEddgbl->unk_498[6]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[7]              : 0x%08lx\n",pEddgbl->unk_498[7]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[8]              : 0x%08lx\n",pEddgbl->unk_498[8]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[9]              : 0x%08lx\n",pEddgbl->unk_498[9]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[10]             : 0x%08lx\n",pEddgbl->unk_498[10]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[11]             : 0x%08lx\n",pEddgbl->unk_498[11]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[12]             : 0x%08lx\n",pEddgbl->unk_498[12]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[13]             : 0x%08lx\n",pEddgbl->unk_498[13]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[14]             : 0x%08lx\n",pEddgbl->unk_498[14]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[15]             : 0x%08lx\n",pEddgbl->unk_498[15]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[16]             : 0x%08lx\n",pEddgbl->unk_498[16]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_498[17]             : 0x%08lx\n",pEddgbl->unk_498[17]);
    // DD_MISCELLANEOUSCALLBACKS ddMiscellanousCallbacks;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks3      : 0x%08lx\n",pEddgbl->d3dNtHalCallbacks3);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[0]              : 0x%08lx\n",pEddgbl->unk_4ec[0]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[1]              : 0x%08lx\n",pEddgbl->unk_4ec[1]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[2]              : 0x%08lx\n",pEddgbl->unk_4ec[2]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[3]              : 0x%08lx\n",pEddgbl->unk_4ec[3]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[4]              : 0x%08lx\n",pEddgbl->unk_4ec[4]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[5]              : 0x%08lx\n",pEddgbl->unk_4ec[5]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[6]              : 0x%08lx\n",pEddgbl->unk_4ec[6]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[7]              : 0x%08lx\n",pEddgbl->unk_4ec[7]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[8]              : 0x%08lx\n",pEddgbl->unk_4ec[8]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[9]              : 0x%08lx\n",pEddgbl->unk_4ec[9]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[10]             : 0x%08lx\n",pEddgbl->unk_4ec[10]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[11]             : 0x%08lx\n",pEddgbl->unk_4ec[11]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[12]             : 0x%08lx\n",pEddgbl->unk_4ec[12]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[13]             : 0x%08lx\n",pEddgbl->unk_4ec[13]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[14]             : 0x%08lx\n",pEddgbl->unk_4ec[14]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[15]             : 0x%08lx\n",pEddgbl->unk_4ec[15]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[16]             : 0x%08lx\n",pEddgbl->unk_4ec[16]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_4ec[17]             : 0x%08lx\n",pEddgbl->unk_4ec[17]);
    // D3DNTHAL_CALLBACKS3 d3dNtHalCallbacks3;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->d3dNtHalCallbacks3      : 0x%08lx\n",pEddgbl->d3dNtHalCallbacks3);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[0]              : 0x%08lx\n",pEddgbl->unk_54c[0]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[1]              : 0x%08lx\n",pEddgbl->unk_54c[1]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[2]              : 0x%08lx\n",pEddgbl->unk_54c[2]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[3]              : 0x%08lx\n",pEddgbl->unk_54c[3]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[4]              : 0x%08lx\n",pEddgbl->unk_54c[4]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[5]              : 0x%08lx\n",pEddgbl->unk_54c[5]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[6]              : 0x%08lx\n",pEddgbl->unk_54c[6]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[7]              : 0x%08lx\n",pEddgbl->unk_54c[7]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[8]              : 0x%08lx\n",pEddgbl->unk_54c[8]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[9]              : 0x%08lx\n",pEddgbl->unk_54c[9]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[10]             : 0x%08lx\n",pEddgbl->unk_54c[10]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[11]             : 0x%08lx\n",pEddgbl->unk_54c[11]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[12]             : 0x%08lx\n",pEddgbl->unk_54c[12]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[13]             : 0x%08lx\n",pEddgbl->unk_54c[13]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[14]             : 0x%08lx\n",pEddgbl->unk_54c[14]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[15]             : 0x%08lx\n",pEddgbl->unk_54c[15]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[16]             : 0x%08lx\n",pEddgbl->unk_54c[16]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[17]             : 0x%08lx\n",pEddgbl->unk_54c[17]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[18]             : 0x%08lx\n",pEddgbl->unk_54c[18]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[19]             : 0x%08lx\n",pEddgbl->unk_54c[19]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[20]             : 0x%08lx\n",pEddgbl->unk_54c[20]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[21]             : 0x%08lx\n",pEddgbl->unk_54c[21]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_54c[22]             : 0x%08lx\n",pEddgbl->unk_54c[22]);
    // EDD_DIRECTDRAW_LOCAL* peDirectDrawLocalList;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->peDirectDrawLocalList   : 0x%08lx\n",pEddgbl->peDirectDrawLocalList);
    // EDD_SURFACE* peSurface_LockList;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->peSurface_LockList      : 0x%08lx\n",pEddgbl->peSurface_LockList);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->fl                      : 0x%08lx\n",pEddgbl->fl);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->cSurfaceLocks           : 0x%08lx\n",pEddgbl->cSurfaceLocks);
    // PKEVENT pAssertModeEvent;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->pAssertModeEvent        : 0x%08lx\n",pEddgbl->pAssertModeEvent);
    // EDD_SURFACE *peSurfaceCurrent;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->peSurfaceCurrent        : 0x%08lx\n",pEddgbl->peSurfaceCurrent);
    // EDD_SURFACE *peSurfacePrimary;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->peSurfacePrimary        : 0x%08lx\n",pEddgbl->peSurfacePrimary);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->bSuspended              : 0x%08lx\n",pEddgbl->bSuspended);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[0]              : 0x%08lx\n",pEddgbl->unk_5c8[0]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[1]              : 0x%08lx\n",pEddgbl->unk_5c8[1]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[2]              : 0x%08lx\n",pEddgbl->unk_5c8[2]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[3]              : 0x%08lx\n",pEddgbl->unk_5c8[3]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[4]              : 0x%08lx\n",pEddgbl->unk_5c8[4]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[5]              : 0x%08lx\n",pEddgbl->unk_5c8[5]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[6]              : 0x%08lx\n",pEddgbl->unk_5c8[6]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[7]              : 0x%08lx\n",pEddgbl->unk_5c8[7]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[8]              : 0x%08lx\n",pEddgbl->unk_5c8[8]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[9]              : 0x%08lx\n",pEddgbl->unk_5c8[9]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[10]             : 0x%08lx\n",pEddgbl->unk_5c8[10]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_5c8[11]             : 0x%08lx\n",pEddgbl->unk_5c8[11]);
    // RECTL rcbounds;
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->rcbounds                : 0x%08lx\n",pEddgbl->rcbounds);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_5FC                 : 0x%08lx\n",(((DWORD)&pEddgbl->unk_5FC) - (DWORD)pEddgbl), pEddgbl->unk_5FC);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->unk_600                 : 0x%08lx\n",(((DWORD)&pEddgbl->unk_600) - (DWORD)pEddgbl), pEddgbl->unk_600);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->hDev                    : 0x%08lx\n",(((DWORD)&pEddgbl->hDev) - (DWORD)pEddgbl), pEddgbl->hDev);
    DPRINT1("0x%08lx PEDD_DIRECTDRAW_GLOBAL->hPDev                   : 0x%08lx\n",(((DWORD)&pEddgbl->hPDev) - (DWORD)pEddgbl), pEddgbl->hPDev);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->ddPaletteCallbacks      : 0x%08lx\n",pEddgbl->ddPaletteCallbacks);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[0]              : 0x%08lx\n",pEddgbl->unk_610[0]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[1]              : 0x%08lx\n",pEddgbl->unk_610[1]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[2]              : 0x%08lx\n",pEddgbl->unk_610[2]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[3]              : 0x%08lx\n",pEddgbl->unk_610[3]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[4]              : 0x%08lx\n",pEddgbl->unk_610[4]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[5]              : 0x%08lx\n",pEddgbl->unk_610[5]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[6]              : 0x%08lx\n",pEddgbl->unk_610[6]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[7]              : 0x%08lx\n",pEddgbl->unk_610[7]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[8]              : 0x%08lx\n",pEddgbl->unk_610[8]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[9]              : 0x%08lx\n",pEddgbl->unk_610[9]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[10]             : 0x%08lx\n",pEddgbl->unk_610[10]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[11]             : 0x%08lx\n",pEddgbl->unk_610[11]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[12]             : 0x%08lx\n",pEddgbl->unk_610[12]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[13]             : 0x%08lx\n",pEddgbl->unk_610[13]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[14]             : 0x%08lx\n",pEddgbl->unk_610[14]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[15]             : 0x%08lx\n",pEddgbl->unk_610[15]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[16]             : 0x%08lx\n",pEddgbl->unk_610[16]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[17]             : 0x%08lx\n",pEddgbl->unk_610[17]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[18]             : 0x%08lx\n",pEddgbl->unk_610[18]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[19]             : 0x%08lx\n",pEddgbl->unk_610[19]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[20]             : 0x%08lx\n",pEddgbl->unk_610[20]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[21]             : 0x%08lx\n",pEddgbl->unk_610[21]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[22]             : 0x%08lx\n",pEddgbl->unk_610[22]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[23]             : 0x%08lx\n",pEddgbl->unk_610[23]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[24]             : 0x%08lx\n",pEddgbl->unk_610[24]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[25]             : 0x%08lx\n",pEddgbl->unk_610[25]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[26]             : 0x%08lx\n",pEddgbl->unk_610[26]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[27]             : 0x%08lx\n",pEddgbl->unk_610[27]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[28]             : 0x%08lx\n",pEddgbl->unk_610[28]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[29]             : 0x%08lx\n",pEddgbl->unk_610[29]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[30]             : 0x%08lx\n",pEddgbl->unk_610[30]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[31]             : 0x%08lx\n",pEddgbl->unk_610[31]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[32]             : 0x%08lx\n",pEddgbl->unk_610[32]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[33]             : 0x%08lx\n",pEddgbl->unk_610[33]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[34]             : 0x%08lx\n",pEddgbl->unk_610[34]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[35]             : 0x%08lx\n",pEddgbl->unk_610[35]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[36]             : 0x%08lx\n",pEddgbl->unk_610[36]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[37]             : 0x%08lx\n",pEddgbl->unk_610[37]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[38]             : 0x%08lx\n",pEddgbl->unk_610[38]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[39]             : 0x%08lx\n",pEddgbl->unk_610[39]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[40]             : 0x%08lx\n",pEddgbl->unk_610[40]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[41]             : 0x%08lx\n",pEddgbl->unk_610[41]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[42]             : 0x%08lx\n",pEddgbl->unk_610[42]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[43]             : 0x%08lx\n",pEddgbl->unk_610[43]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[44]             : 0x%08lx\n",pEddgbl->unk_610[44]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[45]             : 0x%08lx\n",pEddgbl->unk_610[45]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[46]             : 0x%08lx\n",pEddgbl->unk_610[46]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[47]             : 0x%08lx\n",pEddgbl->unk_610[47]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[48]             : 0x%08lx\n",pEddgbl->unk_610[48]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[49]             : 0x%08lx\n",pEddgbl->unk_610[49]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[50]             : 0x%08lx\n",pEddgbl->unk_610[50]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[51]             : 0x%08lx\n",pEddgbl->unk_610[51]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[52]             : 0x%08lx\n",pEddgbl->unk_610[52]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[53]             : 0x%08lx\n",pEddgbl->unk_610[53]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[54]             : 0x%08lx\n",pEddgbl->unk_610[54]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[55]             : 0x%08lx\n",pEddgbl->unk_610[55]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[56]             : 0x%08lx\n",pEddgbl->unk_610[56]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[57]             : 0x%08lx\n",pEddgbl->unk_610[57]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[58]             : 0x%08lx\n",pEddgbl->unk_610[58]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[59]             : 0x%08lx\n",pEddgbl->unk_610[59]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_1e0[60]             : 0x%08lx\n",pEddgbl->unk_610[60]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[61]             : 0x%08lx\n",pEddgbl->unk_610[61]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_610[62]             : 0x%08lx\n",pEddgbl->unk_610[62]);
    DPRINT1("PEDD_DIRECTDRAW_GLOBAL->unk_70C                 : 0x%08lx\n",pEddgbl->unk_70C);
}

/* EOF */
