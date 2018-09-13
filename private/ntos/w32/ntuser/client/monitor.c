/****************************** Module Header ******************************\
* Module Name: monitor.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager client side DDESPY monitoring functions.
*
* Created: 11/20/91 Sanford Staab
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


//
// Other monitor events are initiated directly from SetLastDDEMLError()
// and DoCallback().
//

/***************************************************************************\
* MonitorStringHandle
*
* Description:
* Launches a string handle monitor event. This function should be
* invoked via the MONHSZ() macro so as not to slow down things much
* when no DDESpy is running.
*
* History:
* 11-26-91 sanfords Created.
\***************************************************************************/
VOID MonitorStringHandle(
PCL_INSTANCE_INFO pcii,
HSZ hsz, // local atom
DWORD fsAction)
{
    WCHAR szT[256];
    PEVENT_PACKET pep;
    DWORD cchString;

    CheckDDECritIn;

    UserAssert(pcii->MonitorFlags & MF_HSZ_INFO);

    if (!(cchString = GetAtomName(LATOM_FROM_HSZ(hsz), szT,
            sizeof(szT) / sizeof(WCHAR)))) {
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        return ;
    }
    cchString++;
    pep = (PEVENT_PACKET)DDEMLAlloc(sizeof(EVENT_PACKET) - sizeof(DWORD) +
            sizeof(MONHSZSTRUCT) + cchString * sizeof(WCHAR));
    if (pep == NULL) {
        SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
        return ;
    }

    pep->EventType =    MF_HSZ_INFO;
    pep->fSense =       TRUE;
    pep->cbEventData =  (WORD)(sizeof(MONHSZSTRUCT) + cchString * sizeof(WCHAR));

#define phszs ((MONHSZSTRUCT *)&pep->Data)
    phszs->cb =      sizeof(MONHSZSTRUCT);
    phszs->fsAction = fsAction;
    phszs->dwTime =  NtGetTickCount();
    phszs->hsz = hsz;
    phszs->hTask = (HANDLE)LongToHandle( pcii->tid );
    // phszs->wReserved = 0; // zero init.
    wcscpy(phszs->str, szT);

    LeaveDDECrit;
    Event(pep);
    EnterDDECrit;
#undef phszs
}




/***************************************************************************\
* MonitorLink
*
* Description:
* Launches a link monitor event. This function should be
* invoked via the MONLINK() macro so as not to slow down things much
* when no DDESpy is running.
*
* History:
* 11-26-91 sanfords Created.
\***************************************************************************/
VOID MonitorLink(
PCL_INSTANCE_INFO pcii,
BOOL fEstablished,
BOOL fNoData,
LATOM aService,
LATOM aTopic,
GATOM aItem,
WORD wFmt,
BOOL fServer,
HCONV hConvServer,
HCONV hConvClient)
{
    PEVENT_PACKET pep;

    CheckDDECritIn;

    UserAssert(pcii->MonitorFlags & MF_LINKS);

    pep = (PEVENT_PACKET)DDEMLAlloc(sizeof(EVENT_PACKET) - sizeof(DWORD) +
            sizeof(MONLINKSTRUCT));
    if (pep == NULL) {
        SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
        return ;
    }

    pep->EventType =    MF_LINKS;
    pep->fSense =       TRUE;
    pep->cbEventData =  sizeof(MONLINKSTRUCT);

#define pls ((MONLINKSTRUCT *)&pep->Data)
    pls->cb =           sizeof(MONLINKSTRUCT);
    pls->dwTime =       NtGetTickCount();
    pls->hTask =        (HANDLE)LongToHandle( pcii->tid );
    pls->fEstablished = fEstablished;
    pls->fNoData =      fNoData;

    // use global atoms here - these need to be changed to local atoms before
    // the callbacks to the ddespy apps.

    pls->hszSvc =       (HSZ)LocalToGlobalAtom(aService);
    pls->hszTopic =     (HSZ)LocalToGlobalAtom(aTopic);
    IncGlobalAtomCount(aItem);
    pls->hszItem =      (HSZ)aItem;

    pls->wFmt =         wFmt;
    pls->fServer =      fServer;
    pls->hConvServer =  hConvServer;
    pls->hConvClient =  hConvClient;

    LeaveDDECrit;
    Event(pep);
    EnterDDECrit;

    GlobalDeleteAtom((ATOM)(ULONG_PTR)pls->hszSvc);
    GlobalDeleteAtom((ATOM)(ULONG_PTR)pls->hszTopic);
    GlobalDeleteAtom(aItem);
#undef pls
}




/***************************************************************************\
* MonitorConv
*
* Description:
* Launches a conversation monitor event. This function should be
* invoked via the MONCONV() macro so as not to slow down things much
* when no DDESpy is running.
*
* History:
* 11-26-91 sanfords Created.
* 5-8-92   sanfords Since the hConv's mean nothing outside this process,
*                   the hConv fields now hold hwnds.  This lets DDESPY
*                   tie together connect and disconnect events from each
*                   side.
\***************************************************************************/
VOID MonitorConv(
PCONV_INFO pcoi,
BOOL fConnect)
{
    PEVENT_PACKET pep;

    CheckDDECritIn;

    UserAssert(pcoi->pcii->MonitorFlags & MF_CONV);

    pep = (PEVENT_PACKET)DDEMLAlloc(sizeof(EVENT_PACKET) - sizeof(DWORD) +
            sizeof(MONCONVSTRUCT));
    if (pep == NULL) {
        SetLastDDEMLError(pcoi->pcii, DMLERR_MEMORY_ERROR);
        return ;
    }

    pep->EventType =    MF_CONV;
    pep->fSense =       TRUE;
    pep->cbEventData =  sizeof(MONCONVSTRUCT);

#define pcs ((MONCONVSTRUCT *)&pep->Data)
    pcs->cb =           sizeof(MONCONVSTRUCT);
    pcs->fConnect =     fConnect;
    pcs->dwTime =       NtGetTickCount();
    pcs->hTask =        (HANDLE)LongToHandle( pcoi->pcii->tid );
    pcs->hszSvc =       (HSZ)LocalToGlobalAtom(pcoi->laService);
    pcs->hszTopic =     (HSZ)LocalToGlobalAtom(pcoi->laTopic);
    if (pcoi->state & ST_CLIENT) {
        pcs->hConvClient =  (HCONV)pcoi->hwndConv;
        pcs->hConvServer =  (HCONV)pcoi->hwndPartner;
    } else {
        pcs->hConvClient =  (HCONV)pcoi->hwndPartner;
        pcs->hConvServer =  (HCONV)pcoi->hwndConv;
    }

    LeaveDDECrit;
    Event(pep);
    EnterDDECrit;

    GlobalDeleteAtom((ATOM)(ULONG_PTR)pcs->hszSvc);
    GlobalDeleteAtom((ATOM)(ULONG_PTR)pcs->hszTopic);
#undef pcs
}
