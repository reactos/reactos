//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       notifapi.cxx
//
//  Contents:   helper APIS
//
//  Classes:
//
//  Functions:
//
//  History:    1-19-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>

//+---------------------------------------------------------------------------
//
//  Function:   NotfDeliverNotification
//
//  Synopsis:   Helper API - Delivers a notificaition
//
//  Arguments:  [REFCLSID] --
//              [DELIVERMODE] --
//              [DWORD] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-19-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI NotfDeliverNotification(REFNOTIFICATIONTYPE rNotificationType
                          ,REFCLSID            rClsidDest
                          ,DELIVERMODE         deliverMode
                          ,DWORD               dwReserved
                          )
{
    NotfDebugOut((DEB_MGRGLOBAL,"IN  NotfDeliverNotification\n"));
    HRESULT hr = NOERROR;
    INotificationMgr *pNotificationMgr = 0;

    do
    {
        LPNOTIFICATION pNotfctn = 0;

        hr = CreateNotificationMgr(0, CLSID_StdNotificationMgr, NULL, 
                                   IID_INotificationMgr,(IUnknown **)&pNotificationMgr);
        BREAK_ONERROR(hr);
        
        hr = pNotificationMgr->CreateNotification(
                                     rNotificationType,
                                     (NOTIFICATIONFLAGS)0,
                                     0,
                                     &pNotfctn,
                                     0
                                     );
        BREAK_ONERROR(hr);

        hr = pNotificationMgr->DeliverNotification(
                 pNotfctn,                                  // LPNOTIFICATION   pNotification,
                 rClsidDest,                                // REFDESTID           rNotificationDest,
                 deliverMode,                               // DELIVERMODE         deliverMode,
                 0,                                         // LPNOTIFICATIONSINK  pReportNotfctnSink,     // can be null - see mode
                 0,                                         // LPNOTIFICATIONREPORT *ppNotfctnReport,
                 0                                          // DWORD               dwReserved
                 );


        pNotfctn->Release();

        break;
    } while ( TRUE );

    if (pNotificationMgr)
    {
        pNotificationMgr->Release();
    }

    NotfDebugOut((DEB_MGRGLOBAL,"OUT NotfDeliverNotification(hr:%lx)\n",hr));
    return hr;
}

