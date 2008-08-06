/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntddraw/ddsurf.c
 * PURPOSE:         Direct Draw Surface Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtGdiDdCanCreateSurface(IN HANDLE hDirectDraw,
                        IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdCreateSurface(IN HANDLE hDirectDraw,
                     IN HANDLE* hSurface,
                     IN OUT DDSURFACEDESC* puSurfaceDescription,
                     IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
                     IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
                     IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
                     IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
                     OUT HANDLE* puhSurface)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdCreateSurfaceEx(IN HANDLE hDirectDraw,
                       IN HANDLE hSurface,
                       IN DWORD dwSurfaceHandle)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdDestroySurface(IN HANDLE hSurface,
                      IN BOOL bRealDestroy)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

BOOL
APIENTRY
NtGdiDdAttachSurface(IN HANDLE  hSurfaceFrom,
                     IN HANDLE  hSurfaceTo)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
APIENTRY
NtGdiDdUnattachSurface(IN HANDLE hSurface,
                       IN HANDLE hSurfaceAttached)
{
    UNIMPLEMENTED;
}

DWORD
APIENTRY
NtGdiDdAddAttachedSurface(IN HANDLE hSurface,
                          IN HANDLE hSurfaceAttached,
                          IN OUT PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdFlip(IN HANDLE hSurfaceCurrent,
            IN HANDLE hSurfaceTarget,
            IN HANDLE hSurfaceCurrentLeft,
            IN HANDLE hSurfaceTargetLeft,
            IN OUT PDD_FLIPDATA puFlipData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdFlipToGDISurface(IN HANDLE hDirectDraw,
                        IN OUT PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdLock(IN HANDLE hSurface,
            IN OUT PDD_LOCKDATA puLockData,
            IN HDC hdcClip)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdUnlock(IN HANDLE hSurface,
              IN OUT PDD_UNLOCKDATA puUnlockData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdAlphaBlt(IN HANDLE hSurfaceDest,
                IN OPTIONAL HANDLE hSurfaceSrc,
                IN OUT PDD_BLTDATA puBltData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdBlt(IN HANDLE hSurfaceDest,
           IN HANDLE hSurfaceSrc,
           IN OUT PDD_BLTDATA puBltData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdUpdateOverlay(IN HANDLE hSurfaceDestination,
                     IN HANDLE hSurfaceSource,
                     IN OUT PDD_UPDATEOVERLAYDATA puUpdateOverlayData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdGetBltStatus(IN HANDLE hSurface,
                    IN OUT PDD_GETBLTSTATUSDATA puGetBltStatusData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdGetFlipStatus(IN HANDLE hSurface,
                     IN OUT PDD_GETFLIPSTATUSDATA puGetFlipStatusData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdSetColorKey(IN HANDLE hSurface,
                   IN OUT PDD_SETCOLORKEYDATA puSetColorKeyData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdSetOverlayPosition(IN HANDLE hSurfaceSource,
                          IN HANDLE hSurfaceDestination,
                          IN OUT PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}
