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


/************************************************************************/
/* This is not part of the ddsurface interface but it have              */
/* deal with the surface                                                */
/************************************************************************/

/************************************************************************/
/* NtGdiDdCreateSurfaceEx                                               */
/************************************************************************/
DWORD
STDCALL
NtGdiDdCreateSurfaceEx(HANDLE hDirectDraw,
                       HANDLE hSurface,
                       DWORD dwSurfaceHandle)
{
    PGD_DXDDCREATESURFACEEX pfnDdCreateSurfaceEx  = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCreateSurfaceEx, pfnDdCreateSurfaceEx);

    if (pfnDdCreateSurfaceEx == NULL)
    {
        DPRINT1("Warring no pfnDdCreateSurfaceEx");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdCreateSurfaceEx");
    return pfnDdCreateSurfaceEx(hDirectDraw,hSurface,dwSurfaceHandle);

}

