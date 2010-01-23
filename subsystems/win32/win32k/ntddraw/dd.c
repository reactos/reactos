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
#include <debug.h>

/************************************************************************/
/* NtGdiDdCreateSurface                                                 */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdCreateSurface(HANDLE hDirectDrawLocal,
                     HANDLE *hSurface,
                     DDSURFACEDESC *puSurfaceDescription,
                     DD_SURFACE_GLOBAL *puSurfaceGlobalData,
                     DD_SURFACE_LOCAL *puSurfaceLocalData,
                     DD_SURFACE_MORE *puSurfaceMoreData,
                     PDD_CREATESURFACEDATA puCreateSurfaceData,
                     HANDLE *puhSurface)
{
    PGD_DDCREATESURFACE pfnDdCreateSurface = (PGD_DDCREATESURFACE)gpDxFuncs[DXG_INDEX_DxDdCreateSurface].pfn;
   
    if (pfnDdCreateSurface == NULL)
    {
        DPRINT1("Warning: no pfnDdCreateSurface\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdCreateSurface\n");
    return pfnDdCreateSurface(hDirectDrawLocal,hSurface,puSurfaceDescription,puSurfaceGlobalData,
                              puSurfaceLocalData,puSurfaceMoreData,puCreateSurfaceData,puhSurface);
}

/************************************************************************/
/* NtGdiDdWaitForVerticalBlank                                          */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdWaitForVerticalBlank(HANDLE hDirectDraw,
                            PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData)
{
    PGD_DXDDWAITFORVERTICALBLANK pfnDdWaitForVerticalBlank = (PGD_DXDDWAITFORVERTICALBLANK)gpDxFuncs[DXG_INDEX_DxDdWaitForVerticalBlank].pfn;
   
    if (pfnDdWaitForVerticalBlank == NULL)
    {
        DPRINT1("Warning: no pfnDdWaitForVerticalBlank\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdWaitForVerticalBlank\n");
    return pfnDdWaitForVerticalBlank(hDirectDraw, puWaitForVerticalBlankData);
}

/************************************************************************/
/* NtGdiDdCanCreateSurface                                              */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdCanCreateSurface(HANDLE hDirectDrawLocal,
                        PDD_CANCREATESURFACEDATA puCanCreateSurfaceData)
{
    PGD_DDCANCREATESURFACE pfnDdCanCreateSurface = (PGD_DDCANCREATESURFACE)gpDxFuncs[DXG_INDEX_DxDdCanCreateSurface].pfn;
    
    if (pfnDdCanCreateSurface == NULL)
    {
        DPRINT1("Warning: no pfnDdCanCreateSurface\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys DdCanCreateSurface\n");

    return pfnDdCanCreateSurface(hDirectDrawLocal,puCanCreateSurfaceData);
}

/************************************************************************/
/* NtGdiDdGetScanLine                                                   */
/************************************************************************/
DWORD
APIENTRY 
NtGdiDdGetScanLine(HANDLE hDirectDrawLocal,
                   PDD_GETSCANLINEDATA puGetScanLineData)
{
    PGD_DXDDGETSCANLINE  pfnDdGetScanLine = (PGD_DXDDGETSCANLINE)gpDxFuncs[DXG_INDEX_DxDdGetScanLine].pfn;
   
    if (pfnDdGetScanLine == NULL)
    {
        DPRINT1("Warning: no pfnDdGetScanLine\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdGetScanLine\n");

    return pfnDdGetScanLine(hDirectDrawLocal,puGetScanLineData);
}


/************************************************************************/
/* This is not part of the ddsurface interface but it have              */
/* deal with the surface                                                */
/************************************************************************/

/************************************************************************/
/* NtGdiDdCreateSurfaceEx                                               */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdCreateSurfaceEx(HANDLE hDirectDraw,
                       HANDLE hSurface,
                       DWORD dwSurfaceHandle)
{
    PGD_DXDDCREATESURFACEEX pfnDdCreateSurfaceEx  = (PGD_DXDDCREATESURFACEEX)gpDxFuncs[DXG_INDEX_DxDdCreateSurfaceEx].pfn;
   
    if (pfnDdCreateSurfaceEx == NULL)
    {
        DPRINT1("Warning: no pfnDdCreateSurfaceEx\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdCreateSurfaceEx\n");
    return pfnDdCreateSurfaceEx(hDirectDraw,hSurface,dwSurfaceHandle);

}

