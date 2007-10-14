/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/ddraw.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
         from 2003 to year 2007
 *       rewrote almost all code Peter did.
 *       only few line are left from him
 */

#include <w32k.h>
#include <reactos/drivers/directx/dxg.h>

//#define NDEBUG
#include <debug.h>

/* swtich this off to get rid of all dx debug msg */
#define DX_DEBUG




typedef DWORD (NTAPI *PGD_DXDDREENABLEDIRECTDRAWOBJECT)(HANDLE, BOOL);
typedef DWORD (NTAPI *PGD_DXDDGETDRIVERINFO)(HANDLE, PDD_GETDRIVERINFODATA);
typedef DWORD (NTAPI *PGD_DXDDGETAVAILDRIVERMEMORY(HANDLE, PDD_GETAVAILDRIVERMEMORYDATA);
typedef DWORD (NTAPI *PGD_DXDDSETEXCLUSIVEMODE)(HANDLE, PDD_SETEXCLUSIVEMODEDATA);
typedef NTSTATUS (NTAPI *PGD_DXDDSTARTUPDXGRAPHICS) (ULONG, PDRVENABLEDATA, ULONG, PDRVENABLEDATA, PULONG, PEPROCESS);
typedef NTSTATUS (NTAPI *PGD_DXDDCLEANUPDXGRAPHICS) (VOID);
typedef HANDLE (NTAPI *PGD_DDCREATEDIRECTDRAWOBJECT) (HDC hdc);
typedef DWORD (NTAPI *PGD_DDGETDRIVERSTATE)(PDD_GETDRIVERSTATEDATA);
typedef DWORD (NTAPI *PGD_DDCOLORCONTROL)(HANDLE hSurface,PDD_COLORCONTROLDATA puColorControlData);
typedef HANDLE (NTAPI *PGD_DXDDCREATESURFACEOBJECT)(HANDLE, HANDLE, PDD_SURFACE_LOCAL, PDD_SURFACE_MORE, PDD_SURFACE_GLOBAL, BOOL);
typedef BOOL (NTAPI *PGD_DXDDDELETEDIRECTDRAWOBJECT)(HANDLE);
typedef BOOL (NTAPI *PGD_DXDDDELETESURFACEOBJECT)(HANDLE);
typedef DWORD (NTAPI *PGD_DXDDFLIPTOGDISURFACE)(HANDLE, PDD_FLIPTOGDISURFACEDATA);
typedef DWORD (NTAPI *PGD_DXDDGETAVAILDRIVERMEMORY)(HANDLE , PDD_GETAVAILDRIVERMEMORYDATA);
typedef BOOL (NTAPI *PGD_DXDDQUERYDIRECTDRAWOBJECT)(HANDLE, DD_HALINFO*, DWORD*,  LPD3DNTHAL_CALLBACKS, LPD3DNTHAL_GLOBALDRIVERDATA,
                                                    PDD_D3DBUFCALLBACKS, LPDDSURFACEDESC, DWORD *, VIDEOMEMORY *, DWORD *, DWORD *);


PGD_DXDDSTARTUPDXGRAPHICS gpfnStartupDxGraphics = NULL;
PGD_DXDDCLEANUPDXGRAPHICS gpfnCleanupDxGraphics = NULL;

DRVFN gaEngFuncs;
PDRVFN gpDxFuncs;
HANDLE ghDxGraphics;
ULONG gdwDirectDrawContext;
ULONG gcEngFuncs;

#define DXG_GET_INDEX_FUNCTION(INDEX, FUNCTION) \
    if (gpDxFuncs) \
    { \
        for (i = 0; i <= DXG_INDEX_DxDdIoctl; i++) \
        { \
            if (gpDxFuncs[i].iFunc == INDEX)  \
            { \
                FUNCTION = (VOID *)gpDxFuncs[i].pfn;  \
                break;  \
            }  \
        } \
    }



/************************************************************************/
/* DirectX graphic/video driver loading and cleanup start here          */
/************************************************************************/
NTSTATUS
STDCALL
DxDdStartupDxGraphics(  ULONG ulc1,
                        PDRVENABLEDATA pDrved1,
                        ULONG ulc2,
                        PDRVENABLEDATA pDrved2,
                        PULONG DDContext,
                        PEPROCESS Proc)
{
    DRVENABLEDATA EngDrv;
    DRVENABLEDATA DXG_API;

    NTSTATUS Status = STATUS_DLL_NOT_FOUND;

    /* FIXME setup of gaEngFuncs driver export list
     * but not in this api, we can add it here tempary until we figout where 
     * no code have been writen for it yet
     */


    /* FIXME ReactOS does not loading the dxapi.sys or import functions from it yet */
    // DxApiGetVersion()

    /* Loading the kernel interface of directx for win32k */
    ghDxGraphics = EngLoadImage(L"drivers\\dxg.sys");
    if (!ghDxGraphics)
    {
        DPRINT1("Warring no dxg.sys in ReactOS");
        return Status;
    }

    /* import DxDdStartupDxGraphics and  DxDdCleanupDxGraphics */
    gpfnStartupDxGraphics = EngFindImageProcAddress(ghDxGraphics,"DxDdStartupDxGraphics");
    gpfnCleanupDxGraphics = EngFindImageProcAddress(ghDxGraphics,"DxDdCleanupDxGraphics");

    if ((gpfnStartupDxGraphics) &&
        (gpfnCleanupDxGraphics))
    {
        /* Setup driver data for activate the dx interface */
        EngDrv.iDriverVersion = DDI_DRIVER_VERSION_NT5_01;
        EngDrv.pdrvfn = &gaEngFuncs;
        EngDrv.c = gcEngFuncs;

        Status = gpfnStartupDxGraphics ( sizeof(DRVENABLEDATA),
                                         &EngDrv,
                                         sizeof(DRVENABLEDATA),
                                         &DXG_API,
                                         &gdwDirectDrawContext,
                                         Proc );
    }

    /* check if we manger loading the data and execute the dxStartupDxGraphics and it susscess */
    if (!NT_SUCCESS(Status))
    {
         gpfnStartupDxGraphics = NULL;
         gpfnCleanupDxGraphics = NULL;
         EngUnloadImage( ghDxGraphics);
         ghDxGraphics = NULL;
         DPRINT1("Warring no init of DirectX graphic interface");
    }
    else
    {
        gpDxFuncs =  DXG_API.pdrvfn;
        DPRINT1("DirectX interface is Activated");
    }

    /* return the status */
    return Status;
}

BOOL
INTERNAL_CALL
DD_Cleanup(PVOID ObjectBody)
{
    PDD_DIRECTDRAW pDirectDraw = (PDD_DIRECTDRAW) ObjectBody;

    DPRINT1("DD_Cleanup\n");

    /* Do not known what the new cleanup code should do at moment */
    return TRUE;
}

BOOL
INTERNAL_CALL
DDSURF_Cleanup(PVOID pDDSurf)
{
    DPRINT1("DDSURF_Cleanup\n");
    /* Do not known what the new cleanup code should do at moment */
    return TRUE;
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

    PGD_DDCREATEDIRECTDRAWOBJECT pfnDdCreateDirectDrawObject = NULL;
    NTSTATUS Status;
    PEPROCESS Proc = NULL;
    INT i=0;

    /* FIXME get the process data */
    /* FIXME this code should be add where the driver being load */
    Status = DxDdStartupDxGraphics(0,NULL,0,NULL,NULL, Proc);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Warring : Fail to statup the directx interface");
        return 0;
    }

    /* This is in correct place */
    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCreateDirectDrawObject, pfnDdCreateDirectDrawObject);

    if (pfnDdCreateDirectDrawObject == NULL)
    {
        DPRINT1("Warring no pfnDdCreateDirectDrawObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCreateDirectDrawObject");
    return pfnDdCreateDirectDrawObject(hdc);

}



/************************************************************************/
/* NtGdiDdGetDriverState                                                */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetDriverState(PDD_GETDRIVERSTATEDATA pdata)
{
    PGD_DDGETDRIVERSTATE pfnDdGetDriverState = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetDriverState, pfnDdGetDriverState);

    if (pfnDdGetDriverState == NULL)
    {
        DPRINT1("Warring no pfnDdGetDriverState");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdGetDriverState");
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
    PGD_DDCOLORCONTROL pfnDdColorControl = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdColorControl, pfnDdColorControl);

    if (pfnDdColorControl == NULL)
    {
        DPRINT1("Warring no pfnDdColorControl");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdColorControl");
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
    PGD_DXDDCREATESURFACEOBJECT pfnDdCreateSurfaceObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCreateSurfaceObject, pfnDdCreateSurfaceObject);

    if (pfnDdCreateSurfaceObject == NULL)
    {
        DPRINT1("Warring no pfnDdCreateSurfaceObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdCreateSurfaceObject");
    return pfnDdCreateSurfaceObject(hDirectDrawLocal, hSurface, puSurfaceLocal, puSurfaceMore, puSurfaceGlobal, bComplete);
}

/************************************************************************/
/* NtGdiDdDeleteDirectDrawObject                                        */
/************************************************************************/
BOOL
STDCALL 
NtGdiDdDeleteDirectDrawObject(HANDLE hDirectDrawLocal)
{
    PGD_DXDDDELETEDIRECTDRAWOBJECT pfnDdDeleteDirectDrawObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdDeleteDirectDrawObject, pfnDdDeleteDirectDrawObject);

    if (pfnDdDeleteDirectDrawObject == NULL)
    {
        DPRINT1("Warring no pfnDdDeleteDirectDrawObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdDeleteDirectDrawObject");
    return pfnDdDeleteDirectDrawObject(hDirectDrawLocal);
}

/************************************************************************/
/* NtGdiDdDeleteSurfaceObject                                           */
/************************************************************************/
BOOL
STDCALL
NtGdiDdDeleteSurfaceObject(HANDLE hSurface)
{
    PGD_DXDDDELETESURFACEOBJECT pfnDdDeleteSurfaceObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdDeleteSurfaceObject, pfnDdDeleteSurfaceObject);

    if (pfnDdDeleteSurfaceObject == NULL)
    {
        DPRINT1("Warring no pfnDdDeleteSurfaceObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdDeleteSurfaceObject");
    return pfnDdDeleteSurfaceObject(hSurface);
}

/************************************************************************/
/* NtGdiDdDeleteSurfaceObject                                           */
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
    PGD_DXDDQUERYDIRECTDRAWOBJECT pfnDdQueryDirectDrawObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdQueryDirectDrawObject, pfnDdQueryDirectDrawObject);

    if (pfnDdQueryDirectDrawObject == NULL)
    {
        DPRINT1("Warring no pfnDdQueryDirectDrawObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdQueryDirectDrawObject");
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
    PGD_DXDDREENABLEDIRECTDRAWOBJECT pfnDdReenableDirectDrawObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdReenableDirectDrawObject, pfnDdReenableDirectDrawObject);

    if (pfnDdReenableDirectDrawObject == NULL)
    {
        DPRINT1("Warring no pfnDdReenableDirectDrawObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdReenableDirectDrawObject");
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
    PGD_DXDDGETDRIVERINFO pfnDdGetDriverInfo = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetDriverInfo, pfnDdGetDriverInfo);

    if (pfnDdGetDriverInfo == NULL)
    {
        DPRINT1("Warring no pfnDdGetDriverInfo");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetDriverInfo");
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
    PGD_DXDDGETAVAILDRIVERMEMORY pfnDdGetAvailDriverMemory = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetAvailDriverMemory, pfnDdGetAvailDriverMemory);

    if (pfnDdGetAvailDriverMemory == NULL)
    {
        DPRINT1("Warring no pfnDdGetAvailDriverMemory");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetAvailDriverMemory");
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
    PGD_DXDDSETEXCLUSIVEMODE pfnDdSetExclusiveMode = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdSetExclusiveMode, pfnDdSetExclusiveMode);

    if (pfnDdSetExclusiveMode == NULL)
    {
        DPRINT1("Warring no pfnDdSetExclusiveMode");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdSetExclusiveMode");
    return pfnDdSetExclusiveMode(hDirectDrawLocal, puGetAvailDriverMemoryData);

}


/************************************************************************/
/* NtGdiDdFlipToGDISurface                                              */
/************************************************************************/
DWORD
STDCALL
NtGdiDdFlipToGDISurface(HANDLE hDirectDraw,
                        PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData)
{
    PGD_DXDDFLIPTOGDISURFACE pfnDdFlipToGDISurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdFlipToGDISurface, pfnDdFlipToGDISurface);

    if (pfnDdFlipToGDISurface == NULL)
    {
        DPRINT1("Warring no pfnDdFlipToGDISurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdFlipToGDISurface");
    return pfnDdFlipToGDISurface(hDirectDrawLocal, puFlipToGDISurfaceData);

}

/************************************************************************/
/* NtGdiDdGetDC                                                         */
/************************************************************************/
HDC
STDCALL
NtGdiDdGetDC(HANDLE hSurface,
             PALETTEENTRY *puColorTable)
{

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

}


/************************************************************************/
/* NtGdiDdReleaseDC                                                     */
/************************************************************************/
BOOL
STDCALL
NtGdiDdReleaseDC(HANDLE hSurface)
{

}

/************************************************************************/
/* NtGdiDdResetVisrgn                                                   */
/************************************************************************/
BOOL
STDCALL
NtGdiDdResetVisrgn(HANDLE hSurface,
                   HWND hwnd)
{

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

}




/* EOF */
