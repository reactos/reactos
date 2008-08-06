/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntddraw/dd.c
 * PURPOSE:         General  Direct Draw Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtGdiDdGetScanLine(IN HANDLE hDirectDraw,
                   IN OUT PDD_GETSCANLINEDATA puGetScanLineData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdWaitForVerticalBlank(IN HANDLE hDirectDraw,
                            IN OUT PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

HANDLE
APIENTRY
NtGdiDdCreateDirectDrawObject(IN HDC hdc)
{
    UNIMPLEMENTED;
    return NULL;
}

DWORD
APIENTRY
NtGdiDxgGenericThunk(IN ULONG_PTR ulIndex,
                     IN ULONG_PTR ulHandle,
                     IN OUT SIZE_T *pdwSizeOfPtr1,
                     IN OUT  PVOID pvPtr1,
                     IN OUT SIZE_T *pdwSizeOfPtr2,
                     IN OUT  PVOID pvPtr2)
{
    UNIMPLEMENTED;
	return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdGetDriverState(IN OUT PDD_GETDRIVERSTATEDATA pdata)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdColorControl(IN HANDLE hSurface,
                    IN OUT PDD_COLORCONTROLDATA puColorControlData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

HANDLE
APIENTRY
NtGdiDdCreateSurfaceObject(IN HANDLE hDirectDrawLocal,
                           IN HANDLE hSurface,
                           IN PDD_SURFACE_LOCAL puSurfaceLocal,
                           IN PDD_SURFACE_MORE puSurfaceMore,
                           IN PDD_SURFACE_GLOBAL puSurfaceGlobal,
                           IN BOOL bComplete)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiDdDeleteDirectDrawObject(IN HANDLE hDirectDrawLocal)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiDdDeleteSurfaceObject(IN HANDLE hSurface)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiDdQueryDirectDrawObject(IN HANDLE hDirectDrawLocal,
                             OUT PDD_HALINFO pHalInfo,
                             OUT DWORD* pCallBackFlags,
                             OUT OPTIONAL LPD3DNTHAL_CALLBACKS puD3dCallbacks,
                             OUT OPTIONAL LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
                             OUT OPTIONAL PDD_D3DBUFCALLBACKS puD3dBufferCallbacks,
                             OUT OPTIONAL LPDDSURFACEDESC puD3dTextureFormats,
                             OUT DWORD* puNumHeaps,
                             OUT OPTIONAL VIDEOMEMORY* puvmList,
                             OUT DWORD* puNumFourCC,
                             OUT OPTIONAL DWORD* puFourCC)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiDdReenableDirectDrawObject(IN HANDLE hDirectDrawLocal,
                                IN OUT BOOL* pubNewMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtGdiDdGetDriverInfo(IN HANDLE hDirectDraw,
                     IN OUT PDD_GETDRIVERINFODATA puGetDriverInfoData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdGetAvailDriverMemory(IN HANDLE hDirectDraw,
                            IN OUT PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdSetExclusiveMode(IN HANDLE hDirectDraw,
                        IN OUT PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

HDC
APIENTRY
NtGdiDdGetDC(IN HANDLE hSurface,
             IN PALETTEENTRY* puColorTable)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiDdReleaseDC(IN HANDLE hSurface)
{
    UNIMPLEMENTED;
    return FALSE;
}

HANDLE
APIENTRY
NtGdiDdGetDxHandle(IN OPTIONAL HANDLE hDirectDraw,
                   IN OPTIONAL HANDLE hSurface,
                   IN BOOL bRelease)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiDdResetVisrgn(IN HANDLE hSurface,
                   IN HWND hwnd)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiDdSetGammaRamp(IN HANDLE hDirectDraw,
                    IN HDC hdc,
                    IN LPVOID lpGammaRamp)
{
    UNIMPLEMENTED;
	return FALSE;
}
