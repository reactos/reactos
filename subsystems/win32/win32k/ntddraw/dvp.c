/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntddraw/dvp.c
 * PURPOSE:         Direct Draw Videoport Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtGdiDvpCanCreateVideoPort(IN HANDLE hDirectDraw,
                           IN OUT PDD_CANCREATEVPORTDATA puCanCreateVPortData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpColorControl(IN HANDLE hVideoPort,
                     IN OUT PDD_VPORTCOLORDATA puVPortColorData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

HANDLE
APIENTRY
NtGdiDvpCreateVideoPort(IN HANDLE hDirectDraw,
    IN OUT PDD_CREATEVPORTDATA puCreateVPortData)
{
    UNIMPLEMENTED;
    return NULL;
}

DWORD
APIENTRY
NtGdiDvpDestroyVideoPort(IN HANDLE hVideoPort,
                         IN OUT PDD_DESTROYVPORTDATA puDestroyVPortData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpFlipVideoPort(IN HANDLE hVideoPort,
                      IN HANDLE hDDSurfaceCurrent,
                      IN HANDLE hDDSurfaceTarget,
                      IN OUT PDD_FLIPVPORTDATA puFlipVPortData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpGetVideoPortBandwidth(IN HANDLE hVideoPort,
                              IN OUT PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpGetVideoPortField(IN HANDLE hVideoPort,
                          IN OUT PDD_GETVPORTFIELDDATA puGetVPortFieldData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpGetVideoPortFlipStatus(IN HANDLE hDirectDraw,
                               IN OUT PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpGetVideoPortInputFormats(IN HANDLE hVideoPort,
                                 IN OUT PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpGetVideoPortLine(IN HANDLE hVideoPort,
                         IN OUT PDD_GETVPORTLINEDATA puGetVPortLineData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpGetVideoPortOutputFormats(IN HANDLE hVideoPort,
                                  IN OUT PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpGetVideoPortConnectInfo(IN HANDLE hDirectDraw,
                                IN OUT PDD_GETVPORTCONNECTDATA puGetVPortConnectData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpGetVideoSignalStatus(IN HANDLE hVideoPort,
                             IN OUT PDD_GETVPORTSIGNALDATA puGetVPortSignalData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpUpdateVideoPort(IN HANDLE hVideoPort,
                        IN HANDLE* phSurfaceVideo,
                        IN HANDLE* phSurfaceVbi,
                        IN OUT PDD_UPDATEVPORTDATA puUpdateVPortData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpWaitForVideoPortSync(IN HANDLE hVideoPort,
                             IN OUT PDD_WAITFORVPORTSYNCDATA puWaitForVPortSyncData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDvpAcquireNotification(IN HANDLE hVideoPort,
                            IN OUT HANDLE* hEvent,
                            IN LPDDVIDEOPORTNOTIFY pNotify)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}


DWORD
APIENTRY
NtGdiDvpReleaseNotification(IN HANDLE hVideoPort,
                            IN HANDLE hEvent)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}
