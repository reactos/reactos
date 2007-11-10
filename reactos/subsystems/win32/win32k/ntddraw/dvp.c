
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/dvd.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */


#include <w32k.h>
#include <debug.h>

/************************************************************************/
/* NtGdiDvpCanCreateVideoPort                                           */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpCanCreateVideoPort(HANDLE hDirectDraw,
                           PDD_CANCREATEVPORTDATA puCanCreateVPortData)
{
    PGD_DVPCANCREATEVIDEOPORT pfnDvpCanCreateVideoPort = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpCanCreateVideoPort, pfnDvpCanCreateVideoPort);

    if (pfnDvpCanCreateVideoPort == NULL)
    {
        DPRINT1("Warring no pfnDvpCanCreateVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpCanCreateVideoPort");
    return pfnDvpCanCreateVideoPort(hDirectDraw, puCanCreateVPortData);
}

/************************************************************************/
/* NtGdiDvpColorControl                                                 */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpColorControl(HANDLE hVideoPort,
                     PDD_VPORTCOLORDATA puVPortColorData)
{
    PGD_DVPCOLORCONTROL pfnDvpColorControl = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpColorControl, pfnDvpColorControl);

    if (pfnDvpColorControl == NULL)
    {
        DPRINT1("Warring no pfnDvpColorControl");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpColorControl");
    return pfnDvpColorControl(hVideoPort, puVPortColorData);
}

/************************************************************************/
/* NtGdiDvpCreateVideoPort                                              */
/************************************************************************/
HANDLE
STDCALL
NtGdiDvpCreateVideoPort(HANDLE hDirectDraw,
                        PDD_CREATEVPORTDATA puCreateVPortData)
{
    PGD_DVPCREATEVIDEOPORT pfnDvpCreateVideoPort = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpCreateVideoPort, pfnDvpCreateVideoPort);

    if (pfnDvpCreateVideoPort == NULL)
    {
        DPRINT1("Warring no pfnDvpCreateVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpCreateVideoPort");
    return pfnDvpCreateVideoPort(hDirectDraw, puCreateVPortData);
}

/************************************************************************/
/* NtGdiDvpDestroyVideoPort                                             */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpDestroyVideoPort(HANDLE hVideoPort,
                         PDD_DESTROYVPORTDATA puDestroyVPortData)
{
    PGD_DVPDESTROYVIDEOPORT pfnDvpDestroyVideoPort = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpDestroyVideoPort, pfnDvpDestroyVideoPort);

    if (pfnDvpDestroyVideoPort == NULL)
    {
        DPRINT1("Warring no pfnDvpDestroyVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpDestroyVideoPort");
    return pfnDvpDestroyVideoPort(hVideoPort, puDestroyVPortData);
}

/************************************************************************/
/* NtGdiDvpFlipVideoPort                                                */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpFlipVideoPort(HANDLE hVideoPort,
                      HANDLE hDDSurfaceCurrent,
                      HANDLE hDDSurfaceTarget,
                      PDD_FLIPVPORTDATA puFlipVPortData)
{
    PGD_DVPFLIPVIDEOPORT pfnDvpFlipVideoPort = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpFlipVideoPort, pfnDvpFlipVideoPort);

    if (pfnDvpFlipVideoPort == NULL)
    {
        DPRINT1("Warring no pfnDvpFlipVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpFlipVideoPort");
    return pfnDvpFlipVideoPort(hVideoPort, hDDSurfaceCurrent, hDDSurfaceTarget, puFlipVPortData);
}


/************************************************************************/
/* NtGdiDvpGetVideoPortBandwidth                                        */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortBandwidth(HANDLE hVideoPort,
                              PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData)
{
    PGD_DVPGETVIDEOPORTBANDWITH pfnDvpGetVideoPortBandwidth = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpGetVideoPortBandwidth, pfnDvpGetVideoPortBandwidth);

    if (pfnDvpGetVideoPortBandwidth == NULL)
    {
        DPRINT1("Warring no pfnDvpGetVideoPortBandwidth");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpGetVideoPortBandwidth");
    return pfnDvpGetVideoPortBandwidth(hVideoPort, puGetVPortBandwidthData);
}


/************************************************************************/
/* NtGdiDvpGetVideoPortFlipStatus                                       */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortFlipStatus(HANDLE hDirectDraw,
                               PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData)
{
    PGD_DXDVPGETVIDEOPORTFLIPSTATUS pfnDvpGetVideoPortFlipStatus = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpGetVideoPortFlipStatus, pfnDvpGetVideoPortFlipStatus);

    if (pfnDvpGetVideoPortFlipStatus == NULL)
    {
        DPRINT1("Warring no pfnDvpGetVideoPortFlipStatus");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpGetVideoPortFlipStatus");
    return pfnDvpGetVideoPortFlipStatus(hDirectDraw, puGetVPortFlipStatusData);
}


/************************************************************************/
/* NtGdiDvpGetVideoPortInputFormats                                     */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortInputFormats(HANDLE hVideoPort,
                                 PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData)
{
    PGD_DXDVPGETVIDEOPORTINPUTFORMATS pfnDvpGetVideoPortInputFormats = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpGetVideoPortInputFormats, pfnDvpGetVideoPortInputFormats);

    if (pfnDvpGetVideoPortInputFormats == NULL)
    {
        DPRINT1("Warring no pfnDvpGetVideoPortInputFormats");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpGetVideoPortInputFormats");
    return pfnDvpGetVideoPortInputFormats(hVideoPort, puGetVPortInputFormatData);
}


/************************************************************************/
/* NtGdiDvpGetVideoPortLine                                             */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortLine(HANDLE hVideoPort,
                         PDD_GETVPORTLINEDATA puGetVPortLineData)
{
    PGD_DXDVPGETVIDEOPORTLINE pfnDvpGetVideoPortLine = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpGetVideoPortLine, pfnDvpGetVideoPortLine);

    if (pfnDvpGetVideoPortLine == NULL)
    {
        DPRINT1("Warring no pfnDvpGetVideoPortLine");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpGetVideoPortLine");
    return pfnDvpGetVideoPortLine(hVideoPort, puGetVPortLineData);
}


/************************************************************************/
/* NtGdiDvpGetVideoPortOutputFormats                                    */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortOutputFormats(HANDLE hVideoPort,
                                  PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData)
{
    PGD_DXDVPGETVIDEOPORTOUTPUTFORMATS pfnDvpGetVideoPortOutputFormats = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpGetVideoPortOutputFormats, puGetVPortOutputFormatData);

    if (pfnDvpGetVideoPortOutputFormats == NULL)
    {
        DPRINT1("Warring no pfnDvpGetVideoPortOutputFormats");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpGetVideoPortOutputFormats");
    return pfnDvpGetVideoPortOutputFormats(hVideoPort, puGetVPortOutputFormatData);
 
}


/************************************************************************/
/* NtGdiDvpGetVideoPortConnectInfo                                      */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortConnectInfo(HANDLE hDirectDraw,
                                PDD_GETVPORTCONNECTDATA puGetVPortConnectData)
{
    PGD_DXDVPGETVIDEOPORTCONNECTINFO pfnDvpGetVideoPortConnectInfo = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpGetVideoPortConnectInfo, pfnDvpGetVideoPortConnectInfo);

    if (pfnDvpGetVideoPortConnectInfo == NULL)
    {
        DPRINT1("Warring no pfnDvpGetVideoPortConnectInfo");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpGetVideoPortConnectInfo");
    return pfnDvpGetVideoPortConnectInfo(hDirectDraw, puGetVPortConnectData);
}


/************************************************************************/
/* NtGdiDvpGetVideoSignalStatus                                         */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoSignalStatus(HANDLE hVideoPort,
                             PDD_GETVPORTSIGNALDATA puGetVPortSignalData)
{
    PGD_DXDVPGETVIDEOSIGNALSTATUS pfnDvpGetVideoSignalStatus = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpGetVideoSignalStatus, pfnDvpGetVideoSignalStatus);

    if (pfnDvpGetVideoSignalStatus == NULL)
    {
        DPRINT1("Warring no pfnDvpGetVideoSignalStatus");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpGetVideoSignalStatus");
    return pfnDvpGetVideoSignalStatus(hVideoPort, puGetVPortSignalData);

}


/************************************************************************/
/* NtGdiDvpUpdateVideoPort                                              */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpUpdateVideoPort(HANDLE hVideoPort,
                        HANDLE* phSurfaceVideo,
                        HANDLE* phSurfaceVbi,
                        PDD_UPDATEVPORTDATA puUpdateVPortData)
{
    PGD_DXDVPUPDATEVIDEOPORT pfnDvpUpdateVideoPort = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpUpdateVideoPort, pfnDvpUpdateVideoPort);

    if (pfnDvpUpdateVideoPort == NULL)
    {
        DPRINT1("Warring no pfnDvpUpdateVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpUpdateVideoPort");
    return pfnDvpUpdateVideoPort(hVideoPort, phSurfaceVideo, phSurfaceVbi, puUpdateVPortData);

}


/************************************************************************/
/* NtGdiDvpWaitForVideoPortSync                                         */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpWaitForVideoPortSync(HANDLE hVideoPort,
                             PDD_WAITFORVPORTSYNCDATA puWaitForVPortSyncData)
{
    PGD_DXDVPWAITFORVIDEOPORTSYNC pfnDvpWaitForVideoPortSync = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpWaitForVideoPortSync, pfnDvpWaitForVideoPortSync);

    if (pfnDvpWaitForVideoPortSync == NULL)
    {
        DPRINT1("Warring no pfnDvpWaitForVideoPortSync");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpWaitForVideoPortSync");
    return pfnDvpWaitForVideoPortSync(hVideoPort, puWaitForVPortSyncData);
}


/************************************************************************/
/* NtGdiDvpAcquireNotification                                          */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpAcquireNotification(HANDLE hVideoPort,
                            HANDLE* hEvent,
                            LPDDVIDEOPORTNOTIFY pNotify)
{
    PGD_DXDVPACQUIRENOTIFICATION pfnDvpAcquireNotification = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpAcquireNotification, pfnDvpAcquireNotification);

    if (pfnDvpAcquireNotification == NULL)
    {
        DPRINT1("Warring no pfnDvpAcquireNotification");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpAcquireNotification");
    return pfnDvpAcquireNotification(hVideoPort, hEvent, pNotify);
}


/************************************************************************/
/* NtGdiDvpReleaseNotification                                          */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpReleaseNotification(HANDLE hVideoPort,
                            HANDLE hEvent)
{
    PGD_DXDVPRELEASENOTIFICATION pfnDvpReleaseNotification = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpReleaseNotification, pfnDvpReleaseNotification);

    if (pfnDvpReleaseNotification == NULL)
    {
        DPRINT1("Warring no pfnDvpReleaseNotification");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpReleaseNotification");
    return pfnDvpReleaseNotification(hVideoPort, hEvent);

}


/************************************************************************/
/* NtGdiDvpGetVideoPortField                                            */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortField(HANDLE hVideoPort,
                          PDD_GETVPORTFIELDDATA puGetVPortFieldData)
{
    PGD_DXDVPGETVIDEOPORTFIELD pfnDvpGetVideoPortField = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDvpGetVideoPortField, pfnDvpGetVideoPortField);

    if (pfnDvpGetVideoPortField == NULL)
    {
        DPRINT1("Warring no pfnDvpGetVideoPortField");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDvpGetVideoPortField");
    return pfnDvpGetVideoPortField(hVideoPort, puGetVPortFieldData);

}

