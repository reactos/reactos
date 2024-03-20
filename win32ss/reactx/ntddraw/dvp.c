/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             win32ss/reactx/ntddraw/dvp.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */

#include <win32k.h>

// #define NDEBUG
#include <debug.h>

/************************************************************************/
/* NtGdiDvpCanCreateVideoPort                                           */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpCanCreateVideoPort(HANDLE hDirectDraw,
                           PDD_CANCREATEVPORTDATA puCanCreateVPortData)
{
    PGD_DVPCANCREATEVIDEOPORT pfnDvpCanCreateVideoPort = (PGD_DVPCANCREATEVIDEOPORT)gpDxFuncs[DXG_INDEX_DxDvpCanCreateVideoPort].pfn;

    if (pfnDvpCanCreateVideoPort == NULL)
    {
        DPRINT1("Warning: no pfnDvpCanCreateVideoPort\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpCanCreateVideoPort\n");
    return pfnDvpCanCreateVideoPort(hDirectDraw, puCanCreateVPortData);
}

/************************************************************************/
/* NtGdiDvpColorControl                                                 */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpColorControl(HANDLE hVideoPort,
                     PDD_VPORTCOLORDATA puVPortColorData)
{
    PGD_DVPCOLORCONTROL pfnDvpColorControl = (PGD_DVPCOLORCONTROL)gpDxFuncs[DXG_INDEX_DxDvpColorControl].pfn;

    if (pfnDvpColorControl == NULL)
    {
        DPRINT1("Warning: no pfnDvpColorControl\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpColorControl\n");
    return pfnDvpColorControl(hVideoPort, puVPortColorData);
}

/************************************************************************/
/* NtGdiDvpCreateVideoPort                                              */
/************************************************************************/
HANDLE
APIENTRY
NtGdiDvpCreateVideoPort(HANDLE hDirectDraw,
                        PDD_CREATEVPORTDATA puCreateVPortData)
{
    PGD_DVPCREATEVIDEOPORT pfnDvpCreateVideoPort = (PGD_DVPCREATEVIDEOPORT)gpDxFuncs[DXG_INDEX_DxDvpCreateVideoPort].pfn;

    if (pfnDvpCreateVideoPort == NULL)
    {
        DPRINT1("Warning: no pfnDvpCreateVideoPort\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpCreateVideoPort\n");
    return pfnDvpCreateVideoPort(hDirectDraw, puCreateVPortData);
}

/************************************************************************/
/* NtGdiDvpDestroyVideoPort                                             */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpDestroyVideoPort(HANDLE hVideoPort,
                         PDD_DESTROYVPORTDATA puDestroyVPortData)
{
    PGD_DVPDESTROYVIDEOPORT pfnDvpDestroyVideoPort =
        (PGD_DVPDESTROYVIDEOPORT)gpDxFuncs[DXG_INDEX_DxDvpDestroyVideoPort].pfn;

    if (pfnDvpDestroyVideoPort == NULL)
    {
        DPRINT1("Warning: no pfnDvpDestroyVideoPort\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpDestroyVideoPort\n");
    return pfnDvpDestroyVideoPort(hVideoPort, puDestroyVPortData);
}

/************************************************************************/
/* NtGdiDvpFlipVideoPort                                                */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpFlipVideoPort(HANDLE hVideoPort,
                      HANDLE hDDSurfaceCurrent,
                      HANDLE hDDSurfaceTarget,
                      PDD_FLIPVPORTDATA puFlipVPortData)
{
    PGD_DVPFLIPVIDEOPORT pfnDvpFlipVideoPort =
        (PGD_DVPFLIPVIDEOPORT)gpDxFuncs[DXG_INDEX_DxDvpFlipVideoPort].pfn;

    if (pfnDvpFlipVideoPort == NULL)
    {
        DPRINT1("Warning: no pfnDvpFlipVideoPort\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpFlipVideoPort\n");
    return pfnDvpFlipVideoPort(hVideoPort, hDDSurfaceCurrent, hDDSurfaceTarget, puFlipVPortData);
}

/************************************************************************/
/* NtGdiDvpGetVideoPortBandwidth                                        */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpGetVideoPortBandwidth(HANDLE hVideoPort,
                              PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData)
{
    PGD_DVPGETVIDEOPORTBANDWITH pfnDvpGetVideoPortBandwidth = (PGD_DVPGETVIDEOPORTBANDWITH)gpDxFuncs[DXG_INDEX_DxDvpGetVideoPortBandwidth].pfn;

    if (pfnDvpGetVideoPortBandwidth == NULL)
    {
        DPRINT1("Warning: no pfnDvpGetVideoPortBandwidth\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpGetVideoPortBandwidth\n");
    return pfnDvpGetVideoPortBandwidth(hVideoPort, puGetVPortBandwidthData);
}

/************************************************************************/
/* NtGdiDvpGetVideoPortFlipStatus                                       */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpGetVideoPortFlipStatus(HANDLE hDirectDraw,
                               PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData)
{
    PGD_DXDVPGETVIDEOPORTFLIPSTATUS pfnDvpGetVideoPortFlipStatus = (PGD_DXDVPGETVIDEOPORTFLIPSTATUS)gpDxFuncs[DXG_INDEX_DxDvpGetVideoPortFlipStatus].pfn;

    if (pfnDvpGetVideoPortFlipStatus == NULL)
    {
        DPRINT1("Warning: no pfnDvpGetVideoPortFlipStatus\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpGetVideoPortFlipStatus\n");
    return pfnDvpGetVideoPortFlipStatus(hDirectDraw, puGetVPortFlipStatusData);
}

/************************************************************************/
/* NtGdiDvpGetVideoPortInputFormats                                     */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpGetVideoPortInputFormats(HANDLE hVideoPort,
                                 PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData)
{
    PGD_DXDVPGETVIDEOPORTINPUTFORMATS pfnDvpGetVideoPortInputFormats = (PGD_DXDVPGETVIDEOPORTINPUTFORMATS)gpDxFuncs[DXG_INDEX_DxDvpGetVideoPortInputFormats].pfn;

    if (pfnDvpGetVideoPortInputFormats == NULL)
    {
        DPRINT1("Warning: no pfnDvpGetVideoPortInputFormats\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpGetVideoPortInputFormats\n");
    return pfnDvpGetVideoPortInputFormats(hVideoPort, puGetVPortInputFormatData);
}

/************************************************************************/
/* NtGdiDvpGetVideoPortLine                                             */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpGetVideoPortLine(HANDLE hVideoPort,
                         PDD_GETVPORTLINEDATA puGetVPortLineData)
{
    PGD_DXDVPGETVIDEOPORTLINE pfnDvpGetVideoPortLine = (PGD_DXDVPGETVIDEOPORTLINE)gpDxFuncs[DXG_INDEX_DxDvpGetVideoPortLine].pfn;

    if (pfnDvpGetVideoPortLine == NULL)
    {
        DPRINT1("Warning: no pfnDvpGetVideoPortLine\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpGetVideoPortLine\n");
    return pfnDvpGetVideoPortLine(hVideoPort, puGetVPortLineData);
}

/************************************************************************/
/* NtGdiDvpGetVideoPortOutputFormats                                    */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpGetVideoPortOutputFormats(HANDLE hVideoPort,
                                  PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData)
{
    PGD_DXDVPGETVIDEOPORTOUTPUTFORMATS pfnDvpGetVideoPortOutputFormats = (PGD_DXDVPGETVIDEOPORTOUTPUTFORMATS)gpDxFuncs[DXG_INDEX_DxDvpGetVideoPortOutputFormats].pfn;

    if (pfnDvpGetVideoPortOutputFormats == NULL)
    {
        DPRINT1("Warning: no pfnDvpGetVideoPortOutputFormats\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpGetVideoPortOutputFormats\n");
    return pfnDvpGetVideoPortOutputFormats(hVideoPort, puGetVPortOutputFormatData);
}

/************************************************************************/
/* NtGdiDvpGetVideoPortConnectInfo                                      */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpGetVideoPortConnectInfo(HANDLE hDirectDraw,
                                PDD_GETVPORTCONNECTDATA puGetVPortConnectData)
{
    PGD_DXDVPGETVIDEOPORTCONNECTINFO pfnDvpGetVideoPortConnectInfo = (PGD_DXDVPGETVIDEOPORTCONNECTINFO)gpDxFuncs[DXG_INDEX_DxDvpGetVideoPortConnectInfo].pfn;

    if (pfnDvpGetVideoPortConnectInfo == NULL)
    {
        DPRINT1("Warning: no pfnDvpGetVideoPortConnectInfo\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpGetVideoPortConnectInfo\n");
    return pfnDvpGetVideoPortConnectInfo(hDirectDraw, puGetVPortConnectData);
}

/************************************************************************/
/* NtGdiDvpGetVideoSignalStatus                                         */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpGetVideoSignalStatus(HANDLE hVideoPort,
                             PDD_GETVPORTSIGNALDATA puGetVPortSignalData)
{
    PGD_DXDVPGETVIDEOSIGNALSTATUS pfnDvpGetVideoSignalStatus = (PGD_DXDVPGETVIDEOSIGNALSTATUS)gpDxFuncs[DXG_INDEX_DxDvpGetVideoSignalStatus].pfn;

    if (pfnDvpGetVideoSignalStatus == NULL)
    {
        DPRINT1("Warning: no pfnDvpGetVideoSignalStatus\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpGetVideoSignalStatus\n");
    return pfnDvpGetVideoSignalStatus(hVideoPort, puGetVPortSignalData);
}

/************************************************************************/
/* NtGdiDvpUpdateVideoPort                                              */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpUpdateVideoPort(HANDLE hVideoPort,
                        HANDLE* phSurfaceVideo,
                        HANDLE* phSurfaceVbi,
                        PDD_UPDATEVPORTDATA puUpdateVPortData)
{
    PGD_DXDVPUPDATEVIDEOPORT pfnDvpUpdateVideoPort = (PGD_DXDVPUPDATEVIDEOPORT)gpDxFuncs[DXG_INDEX_DxDvpUpdateVideoPort].pfn;

    if (pfnDvpUpdateVideoPort == NULL)
    {
        DPRINT1("Warning: no pfnDvpUpdateVideoPort\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpUpdateVideoPort\n");
    return pfnDvpUpdateVideoPort(hVideoPort, phSurfaceVideo, phSurfaceVbi, puUpdateVPortData);
}

/************************************************************************/
/* NtGdiDvpWaitForVideoPortSync                                         */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpWaitForVideoPortSync(HANDLE hVideoPort,
                             PDD_WAITFORVPORTSYNCDATA puWaitForVPortSyncData)
{
    PGD_DXDVPWAITFORVIDEOPORTSYNC pfnDvpWaitForVideoPortSync = (PGD_DXDVPWAITFORVIDEOPORTSYNC)gpDxFuncs[DXG_INDEX_DxDvpWaitForVideoPortSync].pfn;

    if (pfnDvpWaitForVideoPortSync == NULL)
    {
        DPRINT1("Warning: no pfnDvpWaitForVideoPortSync\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpWaitForVideoPortSync\n");
    return pfnDvpWaitForVideoPortSync(hVideoPort, puWaitForVPortSyncData);
}

/************************************************************************/
/* NtGdiDvpAcquireNotification                                          */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpAcquireNotification(HANDLE hVideoPort,
                            HANDLE* hEvent,
                            LPDDVIDEOPORTNOTIFY pNotify)
{
    PGD_DXDVPACQUIRENOTIFICATION pfnDvpAcquireNotification = (PGD_DXDVPACQUIRENOTIFICATION)gpDxFuncs[DXG_INDEX_DxDvpAcquireNotification].pfn;

    if (pfnDvpAcquireNotification == NULL)
    {
        DPRINT1("Warning: no pfnDvpAcquireNotification\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpAcquireNotification\n");
    return pfnDvpAcquireNotification(hVideoPort, hEvent, pNotify);
}

/************************************************************************/
/* NtGdiDvpReleaseNotification                                          */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpReleaseNotification(HANDLE hVideoPort,
                            HANDLE hEvent)
{
    PGD_DXDVPRELEASENOTIFICATION pfnDvpReleaseNotification = (PGD_DXDVPRELEASENOTIFICATION)gpDxFuncs[DXG_INDEX_DxDvpReleaseNotification].pfn;

    if (pfnDvpReleaseNotification == NULL)
    {
        DPRINT1("Warning: no pfnDvpReleaseNotification\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpReleaseNotification\n");
    return pfnDvpReleaseNotification(hVideoPort, hEvent);
}

/************************************************************************/
/* NtGdiDvpGetVideoPortField                                            */
/************************************************************************/
DWORD
APIENTRY
NtGdiDvpGetVideoPortField(HANDLE hVideoPort,
                          PDD_GETVPORTFIELDDATA puGetVPortFieldData)
{
    PGD_DXDVPGETVIDEOPORTFIELD pfnDvpGetVideoPortField = (PGD_DXDVPGETVIDEOPORTFIELD)gpDxFuncs[DXG_INDEX_DxDvpGetVideoPortField].pfn;

    if (pfnDvpGetVideoPortField == NULL)
    {
        DPRINT1("Warning: no pfnDvpGetVideoPortField\n");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT("Calling dxg.sys pfnDvpGetVideoPortField\n");
    return pfnDvpGetVideoPortField(hVideoPort, puGetVPortFieldData);
}
