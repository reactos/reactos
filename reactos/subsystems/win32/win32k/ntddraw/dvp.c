
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
#include <reactos/drivers/directx/dxg.h>

//#define NDEBUG
#include <debug.h>

/********************************************************************************/
/*                DVP interface from DXG.SYS                                    */
/********************************************************************************/

extern PDRVFN gpDxFuncs;

typedef DWORD (NTAPI *PGD_DXDVPGETVIDEOSIGNALSTATUS)(HANDLE, PDD_GETVPORTSIGNALDATA);
typedef DWORD (NTAPI *PGD_DXDVPUPDATEVIDEOPORT)(HANDLE, HANDLE*, HANDLE*, PDD_UPDATEVPORTDATA);
typedef DWORD (NTAPI *PGD_DXDVPWAITFORVIDEOPORTSYNC)(hVideoPort, puWaitForVPortSyncData);
typedef DWORD (NTAPI *PGD_DXDVPACQUIRENOTIFICATION)(HANDLE, HANDLE*, LPDDVIDEOPORTNOTIFY);
typedef DWORD (NTAPI *PGD_DXDVPRELEASENOTIFICATION)(HANDLE, HANDLE);
typedef DWORD (NTAPI *PGD_DXDVPGETVIDEOPORTFIELD)(HANDLE, PDD_GETVPORTFIELDDATA);



/************************************************************************/
/* NtGdiDvpCanCreateVideoPort                                           */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpCanCreateVideoPort(hDirectDraw,
                           PDD_CANCREATEVPORTDATA puCanCreateVPortData)
{

}

/************************************************************************/
/* NtGdiDvpColorControl                                                 */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpColorControl(HANDLE hVideoPort,
                     PDD_VPORTCOLORDATA puVPortColorData)
{

}

/************************************************************************/
/* NtGdiDvpCreateVideoPort                                              */
/************************************************************************/
HANDLE
STDCALL
NtGdiDvpCreateVideoPort(HANDLE hDirectDraw,
                        PDD_CREATEVPORTDATA puCreateVPortData)
{

}

/************************************************************************/
/* NtGdiDvpDestroyVideoPort                                             */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpDestroyVideoPort(HANDLE hVideoPort,
                         PDD_DESTROYVPORTDATA puDestroyVPortData)
{

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

}


/************************************************************************/
/* NtGdiDvpGetVideoPortBandwidth                                        */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortBandwidth(HANDLE hVideoPort,
                              PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData)
{

}


/************************************************************************/
/* NtGdiDvpGetVideoPortFlipStatus                                       */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortFlipStatus(HANDLE hDirectDraw,
                               PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData)
{

}


/************************************************************************/
/* NtGdiDvpGetVideoPortInputFormats                                     */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortInputFormats(HANDLE hVideoPort,
                                 PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData)
{

}


/************************************************************************/
/* NtGdiDvpGetVideoPortLine                                             */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortLine(HANDLE hVideoPort,
                         PDD_GETVPORTLINEDATA puGetVPortLineData)
{

}


/************************************************************************/
/* NtGdiDvpGetVideoPortOutputFormats                                    */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortOutputFormats(HANDLE hVideoPort,
                                  PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData)
{

}


/************************************************************************/
/* NtGdiDvpGetVideoPortConnectInfo                                      */
/************************************************************************/
DWORD
STDCALL
NtGdiDvpGetVideoPortConnectInfo(HANDLE hDirectDraw,
                                PDD_GETVPORTCONNECTDATA puGetVPortConnectData)
{

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

