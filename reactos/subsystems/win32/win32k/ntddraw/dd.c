/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/dd.c
 * PROGRAMER:        Magnus Olsen (greatlord@reactos.org)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */

#include <w32k.h>

//#define NDEBUG
#include <debug.h>
#include <reactos/drivers/directx/dxg.h>



extern PDRVFN gpDxFuncs;

/* is not longer in use PDD_DESTROYDRIVER  DestroyDriver */
typedef DWORD (NTAPI *PGD_DDCREATESURFACE)(HANDLE, HANDLE *, DDSURFACEDESC *, DD_SURFACE_GLOBAL *, DD_SURFACE_LOCAL *, DD_SURFACE_MORE *, PDD_CREATESURFACEDATA , HANDLE *);
/* see ddsurf.c for  PDD_SETCOLORKEY  SetColorKey; */
/* is not longer in use PDD_SETMODE  SetMode; */
typedef DWORD (NTAPI *PGD_DXDDWAITFORVERTICALBLANK)(HANDLE, PDD_WAITFORVERTICALBLANKDATA);
typedef DWORD (NTAPI *PGD_DDCANCREATESURFACE)(HANDLE hDirectDrawLocal, PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);
/* is not longer in use PDD_CREATEPALETTE  CreatePalette; */
typedef DWORD (NTAPI *PGD_DXDDGETSCANLINE)(HANDLE, PDD_GETSCANLINEDATA);
/* is not longer in use PDD_MAPMEMORY  MapMemory; */

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
/* NtGdiDdCreateSurface                                                 */
/************************************************************************/
DWORD
STDCALL
NtGdiDdCreateSurface(HANDLE hDirectDrawLocal,
                     HANDLE *hSurface,
                     DDSURFACEDESC *puSurfaceDescription,
                     DD_SURFACE_GLOBAL *puSurfaceGlobalData,
                     DD_SURFACE_LOCAL *puSurfaceLocalData,
                     DD_SURFACE_MORE *puSurfaceMoreData,
                     PDD_CREATESURFACEDATA puCreateSurfaceData,
                     HANDLE *puhSurface)
{
    PGD_DDCREATESURFACE pfnDdCreateSurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCreateSurface, pfnDdCreateSurface);

    if (pfnDdCreateSurface == NULL)
    {
        DPRINT1("Warring no pfnDdCreateSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdCreateSurface");
    return pfnDdCreateSurface(hDirectDrawLocal,hSurface,puSurfaceDescription,puSurfaceGlobalData,
                              puSurfaceLocalData,puSurfaceMoreData,puCreateSurfaceData,puhSurface);
}

/************************************************************************/
/* NtGdiDdWaitForVerticalBlank                                          */
/************************************************************************/
DWORD
STDCALL
NtGdiDdWaitForVerticalBlank(HANDLE hDirectDraw,
                            PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData)
{
    PGD_DXDDWAITFORVERTICALBLANK pfnDdWaitForVerticalBlank = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdWaitForVerticalBlank, pfnDdWaitForVerticalBlank);

    if (pfnDdWaitForVerticalBlank == NULL)
    {
        DPRINT1("Warring no pfnDdWaitForVerticalBlank");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdWaitForVerticalBlank");
    return pfnDdWaitForVerticalBlank(hDirectDraw, puWaitForVerticalBlankData);
}

/************************************************************************/
/* NtGdiDdCanCreateSurface                                              */
/************************************************************************/
DWORD
STDCALL
NtGdiDdCanCreateSurface(HANDLE hDirectDrawLocal,
                        PDD_CANCREATESURFACEDATA puCanCreateSurfaceData)
{
    PGD_DDCANCREATESURFACE pfnDdCanCreateSurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCanCreateSurface, pfnDdCanCreateSurface);

    if (pfnDdCanCreateSurface == NULL)
    {
        DPRINT1("Warring no pfnDdCanCreateSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCanCreateSurface");
    return pfnDdCanCreateSurface(hDirectDrawLocal,puCanCreateSurfaceData);
}

/************************************************************************/
/* NtGdiDdGetScanLine                                                   */
/************************************************************************/
DWORD
STDCALL 
NtGdiDdGetScanLine(HANDLE hDirectDrawLocal,
                   PDD_GETSCANLINEDATA puGetScanLineData)
{
    PGD_DXDDGETSCANLINE  pfnDdGetScanLine = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetScanLine, pfnDdGetScanLine);

    if (pfnDdGetScanLine == NULL)
    {
        DPRINT1("Warring no pfnDdGetScanLine");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetScanLine");
    return pfnDdGetScanLine(hDirectDrawLocal,puGetScanLineData);
}


