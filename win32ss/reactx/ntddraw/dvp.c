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
		DPRINT1("Warning: no pfnDvpCanCreateVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpCanCreateVideoPort");
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
		DPRINT1("Warning: no pfnDvpColorControl");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpColorControl");
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
		DPRINT1("Warning: no pfnDvpCreateVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpCreateVideoPort");
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
    PGD_DVPDESTROYVIDEOPORT pfnDvpDestroyVideoPort  = (PGD_DVPDESTROYVIDEOPORT)gpDxFuncs[DXG_INDEX_DxDvpDestroyVideoPort].pfn;
    
    if (pfnDvpDestroyVideoPort == NULL)
    {
		DPRINT1("Warning: no pfnDvpDestroyVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpDestroyVideoPort");
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
    PGD_DVPFLIPVIDEOPORT pfnDvpFlipVideoPort  = (PGD_DVPFLIPVIDEOPORT)gpDxFuncs[DXG_INDEX_DxDvpFlipVideoPort].pfn;

    if (pfnDvpFlipVideoPort == NULL)
    {
		DPRINT1("Warning: no pfnDvpFlipVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpFlipVideoPort");
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
		DPRINT1("Warning: no pfnDvpGetVideoPortBandwidth");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpGetVideoPortBandwidth");
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
		DPRINT1("Warning: no pfnDvpGetVideoPortFlipStatus");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpGetVideoPortFlipStatus");
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
		DPRINT1("Warning: no pfnDvpGetVideoPortInputFormats");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpGetVideoPortInputFormats");
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
		DPRINT1("Warning: no pfnDvpGetVideoPortLine");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpGetVideoPortLine");
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
		DPRINT1("Warning: no pfnDvpGetVideoPortOutputFormats");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpGetVideoPortOutputFormats");
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
		DPRINT1("Warning: no pfnDvpGetVideoPortConnectInfo");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpGetVideoPortConnectInfo");
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
		DPRINT1("Warning: no pfnDvpGetVideoSignalStatus");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpGetVideoSignalStatus");
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
		DPRINT1("Warning: no pfnDvpUpdateVideoPort");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpUpdateVideoPort");
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
		DPRINT1("Warning: no pfnDvpWaitForVideoPortSync");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpWaitForVideoPortSync");
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
		DPRINT1("Warning: no pfnDvpAcquireNotification");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpAcquireNotification");
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
		DPRINT1("Warning: no pfnDvpReleaseNotification");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpReleaseNotification");
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
		DPRINT1("Warning: no pfnDvpGetVideoPortField");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDvpGetVideoPortField");
    return pfnDvpGetVideoPortField(hVideoPort, puGetVPortFieldData);

}
