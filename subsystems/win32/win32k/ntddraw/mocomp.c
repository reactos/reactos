/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntddraw/mocomp.c
 * PURPOSE:         Motion Compensation Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtGdiDdBeginMoCompFrame(IN HANDLE hMoComp,
                        IN OUT PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

HANDLE
APIENTRY
NtGdiDdCreateMoComp(IN HANDLE hDirectDraw,
                    IN OUT PDD_CREATEMOCOMPDATA puCreateMoCompData)
{
    UNIMPLEMENTED;
    return NULL;
}

DWORD
APIENTRY
NtGdiDdDestroyMoComp(IN HANDLE hMoComp,
                     IN OUT PDD_DESTROYMOCOMPDATA puDestroyMoCompData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdEndMoCompFrame(IN HANDLE hMoComp,
                      IN OUT PDD_ENDMOCOMPFRAMEDATA puEndFrameData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdGetInternalMoCompInfo(IN HANDLE hDirectDraw,
                             IN OUT PDD_GETINTERNALMOCOMPDATA puGetInternalData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdGetMoCompBuffInfo(IN HANDLE hDirectDraw,
                         IN OUT PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdGetMoCompFormats(IN HANDLE hDirectDraw,
                        IN OUT PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdGetMoCompGuids(IN HANDLE hDirectDraw,
                      IN OUT PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdQueryMoCompStatus(IN HANDLE hMoComp,
                         IN OUT PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}

DWORD
APIENTRY
NtGdiDdRenderMoComp(IN HANDLE hMoComp,
                    IN OUT PDD_RENDERMOCOMPDATA puRenderMoCompData)
{
    UNIMPLEMENTED;
    return DDHAL_DRIVER_NOTHANDLED;
}
