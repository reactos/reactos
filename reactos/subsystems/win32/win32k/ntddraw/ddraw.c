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
#include <reactos/drivers/directx/dxg.h>

//#define NDEBUG
#include <debug.h>

/* swtich this off to get rid of all dx debug msg */
#define DX_DEBUG

#define DdHandleTable GdiHandleTable


typedef NTSTATUS (NTAPI *PGD_DXDDSTARTUPDXGRAPHICS) (ULONG, PDRVENABLEDATA, ULONG, PDRVENABLEDATA, PULONG, PEPROCESS);
typedef NTSTATUS (NTAPI *PGD_DXDDCLEANUPDXGRAPHICS) (VOID);
typedef HANDLE (NTAPI *PGD_DDCREATEDIRECTDRAWOBJECT) (HDC hdc);
typedef DWORD (NTAPI *PGD_DDGETDRIVERSTATE)(PDD_GETDRIVERSTATEDATA);
typedef DWORD (NTAPI *PGD_DDALPHABLT)(HANDLE, HANDLE, PDD_BLTDATA);
typedef BOOL (NTAPI *PGD_DDATTACHSURFACE)(HANDLE, HANDLE);
typedef DWORD (NTAPI *PGD_DDCANCREATED3DBUFFER)(HANDLE, PDD_CANCREATESURFACEDATA);
typedef DWORD (NTAPI *PGD_DDCOLORCONTROL)(HANDLE hSurface,PDD_COLORCONTROLDATA puColorControlData);
typedef HANDLE (NTAPI *PGD_DXDDCREATESURFACEOBJECT)(HANDLE, HANDLE, PDD_SURFACE_LOCAL, PDD_SURFACE_MORE, PDD_SURFACE_GLOBAL, BOOL);
typedef BOOL (NTAPI *PGD_DXDDDELETEDIRECTDRAWOBJECT)(HANDLE);
typedef BOOL (NTAPI *PGD_DXDDDELETESURFACEOBJECT)(HANDLE);

typedef DWORD (NTAPI *PGD_DXDDDESTROYD3DBUFFER)(HANDLE);

typedef DWORD (NTAPI *PGD_DXDDFLIPTOGDISURFACE)(HANDLE, PDD_FLIPTOGDISURFACEDATA);
typedef DWORD (NTAPI *PGD_DXDDGETAVAILDRIVERMEMORY)(HANDLE , PDD_GETAVAILDRIVERMEMORYDATA);

/*
typedef DWORD (NTAPI *PGD_DXDDGETBLTSTATUS)(
typedef DWORD (NTAPI *PGD_DXDDGETDC)(
typedef DWORD (NTAPI *PGD_DXDDGETDRIVERINFO)(
typedef DWORD (NTAPI *PGD_DXDDGETDXHANDLE)(
typedef DWORD (NTAPI *PGD_DXDDGETFLIPSTATUS)(
typedef DWORD (NTAPI *PGD_DXDDGETSCANLINE)(
typedef DWORD (NTAPI *PGD_DXDDLOCK)(
typedef DWORD (NTAPI *PGD_DXDDLOCKD3D)(
typedef DWORD (NTAPI *PGD_DXDDQUERYDIRECTDRAWOBJECT)(
typedef DWORD (NTAPI *PGD_DXDDQUERYMOCOMPSTATUS)(
typedef DWORD (NTAPI *PGD_DXDDREENABLEDIRECTDRAWOBJECT)(
typedef DWORD (NTAPI *PGD_DXDDRELEASEDC)(
typedef DWORD (NTAPI *PGD_DXDDRENDERMOCOMP)(
typedef DWORD (NTAPI *PGD_DXDDRESETVISRGN)(
typedef DWORD (NTAPI *PGD_DXDDSETCOLORKEY)(
typedef DWORD (NTAPI *PGD_DXDDSETEXCLUSIVEMODE)(
typedef DWORD (NTAPI *PGD_DXDDSETGAMMARAMP)(
typedef DWORD (NTAPI *PGD_DXDDCREATESURFACEEX)(
typedef DWORD (NTAPI *PGD_DXDDSETOVERLAYPOSITION)(
typedef DWORD (NTAPI *PGD_DXDDUNATTACHSURFACE)(
*/



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
/* DirectX graphic/video driver loading                                 */
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

HANDLE
STDCALL
NtGdiDdCreateMoComp(HANDLE hDirectDraw, PDD_CREATEMOCOMPDATA puCreateMoCompData)
{
    PGD_DXDDCREATEMOCOMP pfnDdCreateMoComp = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCreateMoComp, pfnDdCreateMoComp);

    if (pfnDdCreateMoComp == NULL)
    {
        DPRINT1("Warring no pfnDdCreateMoComp");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCreateMoComp");
    return pfnDdCreateMoComp(hDirectDraw, puCreateMoCompData);
}


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




DWORD
STDCALL
NtGdiDdAlphaBlt(HANDLE hSurfaceDest,
                HANDLE hSurfaceSrc,
                PDD_BLTDATA puBltData)
{
    PGD_DDALPHABLT pfnDdAlphaBlt = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdAlphaBlt, pfnDdAlphaBlt);

    if (pfnDdAlphaBlt == NULL)
    {
        DPRINT1("Warring no pfnDdAlphaBlt");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdAlphaBlt");
    return pfnDdAlphaBlt(hSurfaceDest,hSurfaceSrc,puBltData);
}


BOOL
STDCALL
NtGdiDdAttachSurface(HANDLE hSurfaceFrom,
                     HANDLE hSurfaceTo
)
{
    PGD_DDATTACHSURFACE pfnDdAttachSurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdAttachSurface, pfnDdAttachSurface);

    if (pfnDdAttachSurface == NULL)
    {
        DPRINT1("Warring no pfnDdAttachSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdAttachSurface");
    return pfnDdAttachSurface(hSurfaceFrom,hSurfaceTo);
}

DWORD
STDCALL
NtGdiDdBeginMoCompFrame(HANDLE hMoComp,
                        PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData)
{
    PGD_DDBEGINMOCOMPFRAME pfnDdBeginMoCompFrame = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdBeginMoCompFrame, pfnDdBeginMoCompFrame);

    if (pfnDdBeginMoCompFrame == NULL)
    {
        DPRINT1("Warring no pfnDdBeginMoCompFrame");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdBeginMoCompFrame");
    return pfnDdBeginMoCompFrame(hMoComp,puBeginFrameData);
}

DWORD
STDCALL
NtGdiDdCanCreateD3DBuffer(HANDLE hDirectDraw,
                          PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
)
{
    PGD_DDCANCREATED3DBUFFER pfnDdCanCreateD3DBuffer = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCanCreateD3DBuffer, pfnDdCanCreateD3DBuffer);

    if (pfnDdCanCreateD3DBuffer == NULL)
    {
        DPRINT1("Warring no pfnDdCanCreateD3DBuffer");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCanCreateD3DBuffer");
    return pfnDdCanCreateD3DBuffer(hDirectDraw,puCanCreateSurfaceData);
}


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

    DPRINT1("Calling on dxg.sys DdCreateSurfaceObject");
    return pfnDdCreateSurfaceObject(hDirectDrawLocal, hSurface, puSurfaceLocal, puSurfaceMore, puSurfaceGlobal, bComplete);
}

BOOL
STDCALL 
NtGdiDdDeleteDirectDrawObject( HANDLE hDirectDrawLocal)
{
    PGD_DXDDDELETEDIRECTDRAWOBJECT pfnDdDeleteDirectDrawObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdDeleteDirectDrawObject, pfnDdDeleteDirectDrawObject);

    if (pfnDdDeleteDirectDrawObject == NULL)
    {
        DPRINT1("Warring no pfnDdDeleteDirectDrawObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdDeleteDirectDrawObject");
    return pfnDdDeleteDirectDrawObject(hDirectDrawLocal);
}

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

DWORD 
STDCALL
NtGdiDdDestroyMoComp(HANDLE hMoComp,
                     PDD_DESTROYMOCOMPDATA puBeginFrameData)
{
    PGD_DXDDDESTROYMOCOMP pfnDxDdDestroyMoComp = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdDestroyMoComp, pfnDxDdDestroyMoComp);

    if (pfnDxDdDestroyMoComp == NULL)
    {
        DPRINT1("Warring no pfnDxDdDestroyMoComp");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DxDdDestroyMoComp");
    return pfnDxDdDestroyMoComp(hMoComp, puBeginFrameData);
}




DWORD
STDCALL
NtGdiDdDestroyD3DBuffer(HANDLE hSurface)
{
    PGD_DXDDDESTROYD3DBUFFER pfnDdDestroyD3DBuffer = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdDestroyD3DBuffer, pfnDdDestroyD3DBuffer);

    if (pfnDdDestroyD3DBuffer == NULL)
    {
        DPRINT1("Warring no pfnDdDestroyD3DBuffer");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdDestroyD3DBuffer");
    return pfnDdDestroyD3DBuffer(hSurface);
}




typedef DWORD (NTAPI *PGD_DXDDENDMOCOMPFRAME)(HANDLE, PDD_ENDMOCOMPFRAMEDATA);
typedef DWORD (NTAPI *PGD_DXDDFLIPTOGDISURFACE)(HANDLE, PDD_FLIPTOGDISURFACEDATA);
typedef DWORD (NTAPI *PGD_DXDDGETAVAILDRIVERMEMORY)(HANDLE , PDD_GETAVAILDRIVERMEMORYDATA);



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






/************************************************************************/
/* DIRECT DRAW SURFACR END                                                   */
/************************************************************************/




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
