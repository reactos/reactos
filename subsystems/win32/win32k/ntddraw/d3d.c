/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntddraw/d3d.c
 * PURPOSE:         Direct 3D Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtGdiDdCanCreateD3DBuffer(IN HANDLE hDirectDraw,
                          IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiD3dContextCreate(IN HANDLE hDirectDrawLocal,
                      IN HANDLE hSurfColor,
                      IN HANDLE hSurfZ,
                      IN OUT D3DNTHAL_CONTEXTCREATEI *pdcci)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiD3dContextDestroy(IN LPD3DNTHAL_CONTEXTDESTROYDATA pdcdd)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiD3dContextDestroyAll(OUT LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcdad)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdCreateD3DBuffer(IN HANDLE hDirectDraw,
                       IN OUT HANDLE* hSurface,
                       IN OUT DDSURFACEDESC* puSurfaceDescription,
                       IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
                       IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
                       IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
                       IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
                       IN OUT HANDLE* puhSurface)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdDestroyD3DBuffer(IN HANDLE hSurface)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiD3dDrawPrimitives2(IN HANDLE hCmdBuf,
                        IN HANDLE hVBuf,
                        IN OUT LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
                        IN OUT FLATPTR* pfpVidMemCmd,
                        IN OUT DWORD* pdwSizeCmd,
                        IN OUT FLATPTR* pfpVidMemVtx,
                        IN OUT DWORD* pdwSizeVtx)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiD3dValidateTextureStageState(IN OUT LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdLockD3D(IN HANDLE hSurface,
               IN OUT PDD_LOCKDATA puLockData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdUnlockD3D(IN HANDLE hSurface,
                 IN OUT PDD_UNLOCKDATA puUnlockData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}
