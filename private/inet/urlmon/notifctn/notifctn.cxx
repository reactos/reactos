//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       notifctn.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-21-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>

//
// the NotificationMgr is a process variable
//
CNotificationMgr            *g_pCNotificationMgr        = 0;
// some areas need single access
CMutexSem                    g_mxsCNotfMgr;

#define SZNOTF_DESTINATION  "Destination 0.6"
#define SZNOTF_ROOT         c_pszRegKeyRoot
#define SZNOTF_SCHEDITEMS   "SchedItems 0.6\\"

const CHAR c_pszRegKeyRoot[]          = "Software\\Microsoft\\Windows\\CurrentVersion\\NotificationMgr\\";
const CHAR c_pszRegKeyDestination[]   = "Software\\Microsoft\\Windows\\CurrentVersion\\NotificationMgr\\Destination 0.6\\";
const CHAR c_pszRegKeyPackage[]       = "Software\\Microsoft\\Windows\\CurrentVersion\\NotificationMgr\\Package 0.6\\";
const CHAR c_pszRegKeyRunningItem[]   = "Software\\Microsoft\\Windows\\CurrentVersion\\NotificationMgr\\RunningItem 0.6\\";
const CHAR c_pszRegKeyRunningDest[]   = "Software\\Microsoft\\Windows\\CurrentVersion\\NotificationMgr\\RunningDest 0.6\\";
const CHAR c_pszRegKeyScheduleGroup[] = "Software\\Microsoft\\Windows\\CurrentVersion\\NotificationMgr\\ScheduleGroup 0.6\\";

const CHAR c_pszRegKeyPackageDelete[]       = "Software\\Microsoft\\Windows\\CurrentVersion\\NotificationMgr\\Package 0.6";
const CHAR c_pszRegKeyRunningDestDelete[]   = "Software\\Microsoft\\Windows\\CurrentVersion\\NotificationMgr\\RunningDest 0.6";



extern COleAutDll   g_OleAutDll;
CHAR c_rgszRegKey[SZREGSETTING_MAX] = {0};
const CHAR * c_pszRegKey = c_rgszRegKey;


TASK_TRIGGER TaskTriggerDefDisabled =
{
    sizeof(TASK_TRIGGER)        //  WORD cbTriggerSize;
    ,0      //  WORD Reserved;
    ,1980   //  WORD wBeginYear;
    ,1      //  WORD wBeginMonth;
    ,1      //  WORD wBeginDay;
    ,0      //  WORD wEndYear;
    ,0      //  WORD wEndMonth;
    ,0      //  WORD wEndDay;
    ,0      //  WORD wStartHour;
    ,0      //  WORD wStartMinute;
    ,0      //  DWORD MinutesDuration;
    ,0      //  DWORD MinutesInterval;
    ,TASK_TRIGGER_FLAG_DISABLED   //  DWORD rgFlags;
    ,TASK_TIME_TRIGGER_DAILY      //  TASK_TRIGGER_TYPE TriggerType;
    ,1                            //  TRIGGER_TYPE_UNION Type;
};

PTASK_TRIGGER GetDefTaskTrigger(BOOL fRand)
{
    PTASK_TRIGGER  pTT = (TASK_TRIGGER *)&TaskTriggerDefDisabled;

    if (fRand)
    {
        DWORD dwRandTime = GetRandTime(RANDOM_TIME_START, RANDOM_TIME_END, RANDOM_TIME_INC);
        pTT->wRandomMinutesInterval = RANDOM_TIME_INC;
        pTT->wStartHour             = (UINT)(dwRandTime / 60);
        pTT->wStartMinute           = (UINT)(dwRandTime % 60);
    }
    else
    {
        pTT->wRandomMinutesInterval = 0;
        pTT->wStartHour             = 0;
        pTT->wStartMinute           = 0;
    }

    return pTT;
}

//+---------------------------------------------------------------------------
//
//  Function:   NotificationMgrUnInit
//
//  Synopsis:   uninitializes the notification Mgr 
//
//  Arguments:  
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:      called on DLL_PROCESSDETACH
//
//----------------------------------------------------------------------------
void NotificationMgrUnInit()
{
    DUMP_ALLOC_TRACK();
    if (g_pCSchedListAgent)
    {
        g_pCSchedListAgent->Uninitialize();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateNotificationMgr
//
//  Synopsis:   creates a notification mgr for a process
//
//  Arguments:  [dwId] --
//              [rclsid] --
//              [pUnkOuter] --
//              [riid] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CreateNotificationMgr(DWORD dwId, REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk)
{
    NotfDebugOut((DEB_MGR, "API _IN CreateNotificationMgr\n"));
    HRESULT hr = NOERROR;
    CLock lck(g_mxsCNotfMgr);

    INotificationMgr *pNotificationMgr= 0;
    CDeliverAgentStart  * pDelAgent = GetDeliverAgentStart();
    CScheduleAgent      * pSchAgent = GetScheduleAgent();

    do
    {
        if (!pDelAgent || !pSchAgent)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        if (pUnkOuter)
        {
            hr = CLASS_E_NOAGGREGATION;
            break;
        }

        if (g_pCNotificationMgr == 0)
        {
            g_pCNotificationMgr = new CNotificationMgr(*pDelAgent, *pSchAgent);

            if (g_pCNotificationMgr)
            {
                HRESULT hr1;
                // make sure the default items are avaiable
                g_pCNotificationMgr->CreateDefaultScheduleGroups(0);
                // make sure destination are registered
                g_pCNotificationMgr->UnPersistDestinationItem(0,0,0);
                // now reload all the peristed packages
                hr1 = g_pCNotificationMgr->UnPersistScheduleItem(0,0,0);
            }
        }

        if (g_pCNotificationMgr)
        {
            pNotificationMgr = g_pCNotificationMgr;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        BREAK_ONERROR(hr);

        IScheduleGroup *pSchGroup = 0;
        NOTIFICATIONCOOKIE GroupCookie;

        hr = g_pCNotificationMgr->QueryInterface(riid, (LPVOID *)ppUnk);

        break;

    } while (TRUE);


    NotfDebugOut((DEB_MGR, "API OUT CreateNotificationMgr(hr:%lx)\n", hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotificationMgr::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_INotificationMgr) )
    {
        *ppvObj = this;
        AddRef();
    }
    else if (riid == IID_INotificationProcessMgr0)
    {
        *ppvObj = (INotificationProcessMgr0 *)this;
        AddRef();
    }
    else if (riid == IID_INotificationSink)
    {
        *ppvObj = (INotificationSink *)this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotificationMgr::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotificationMgr::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNotificationMgr::AddRef(void)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotificationMgr::AddRef\n", this));

    LONG lRet = ++_CRefs;

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotificationMgr::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotificationMgr::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNotificationMgr::Release(void)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotificationMgr::Release\n", this));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        // this is global
        //delete this;
    }
    // the refcount of this object should never go to zero
    NotfAssert((lRet > 0));

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotificationMgr::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::Register
//
//  Synopsis:   registers ID for a given classId or INotificationSink
//              An ID can only be registered ONCE per ClassID.
//
//  Arguments:  [pNotfctnSink] --
//              [rNotificationDest] --
//              [asMode] --
//              [cNotifications] --
//              [pNotificationIDs] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    12-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:      BUGBUG: need to change implementation to register an ID
//                      once per class if classid is provided!
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::RegisterNotificationSink(
    LPNOTIFICATIONSINK  pNotfctnSink,              // can be null - see mode
    LPCLSID             pDestID,
    NOTFSINKMODE        asMode,
    ULONG               cNotifications,
    PNOTIFICATIONTYPE   pNotificationIDs,
    PNOTIFICATIONCOOKIE pRegisterCookie,
    DWORD               dwReserved
    )
{

    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::RegisterNotificationSink\n", this));
    HRESULT hr = NOERROR;

    if (    !pRegisterCookie
         || (!pDestID && !pNotificationIDs)
         || (   !cNotifications
             || !pNotificationIDs)
       )
    {
        // sink can be registered for completion reports
        hr = E_INVALIDARG;
    }
    else
    {
        hr = _rDeliverAgent.Register(
                                pNotfctnSink,
                                pDestID,
                                asMode,
                                cNotifications,
                                pNotificationIDs,
                                pRegisterCookie,
                                dwReserved
                                );
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::RegisterNotificationSink (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::Unregister
//
//  Synopsis:
//
//  Arguments:  [rNotificationDest] --
//              [cNotifications] --
//              [pNotificationIDs] --
//
//  Returns:
//
//  History:    12-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::UnregisterNotificationSink(
    PNOTIFICATIONCOOKIE     pRegisterCookie
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::UnregisterNotificationSink\n", this));
    HRESULT hr = NOERROR;

    if (!pRegisterCookie)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = _rDeliverAgent.Unregister(pRegisterCookie);
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::UnregisterNotificationSink (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::CreateNotification
//
//  Synopsis:   creates an empty notification object
//
//  Arguments:  [rNotificationType] --
//              [NotificationFlags] --
//              [pUnkOuter] --
//              [ppNotification] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::CreateNotification(
    // the type of notification
    REFNOTIFICATIONTYPE rNotificationType,
    // creation flags
    NOTIFICATIONFLAGS   NotificationFlags,
    // the pUnkOuter for aggregation
    LPUNKNOWN           pUnkOuter,
    // the new object just created
    LPNOTIFICATION     *ppNotification,
    DWORD               dwReserved
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::CreateNotification\n", this));
    HRESULT hr = NOERROR;

    NotfAssert((ppNotification));

    if (!ppNotification)
    {
        hr = E_INVALIDARG;
    }
    else if (pUnkOuter)
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    else
    {
        CNotfctnObj *pPropMap = 0;
        *ppNotification = 0;
        NOTIFICATIONTYPE nofctnType = rNotificationType;

        hr = CNotfctnObj::Create((PNOTIFICATIONTYPE) &nofctnType, &pPropMap);
        if (hr == NOERROR)
        {
            *ppNotification = pPropMap;
        }
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::CreateNotification (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::FindNotification
//
//  Synopsis:   find a scheduled  notification form the schedule agent
//
//  Arguments:  [pNotificatioCookie] --
//              [pNotificationItem] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::FindNotification(
    // the notification cookie
    PNOTIFICATIONCOOKIE pNotificatioCookie,
    // the new object just created
    PNOTIFICATIONITEM   pNotificationItem,
    DWORD               dwMode
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::FindNotification\n", this));
    HRESULT hr = E_INVALIDARG;

    do
    {
        if (   !pNotificatioCookie
            || !pNotificationItem
            )
        {
            break;
        }

        CSchedListAgent *pCSchAgent = _rScheduleAgent.GetSchedListAgent();
        if (!pCSchAgent)
        {
            hr = E_FAIL;
            break;
        }
        CPackage *pCPackage = 0;
        hr = pCSchAgent->FindPackage(
                                     pNotificatioCookie
                                     ,&pCPackage
                                     ,0
                                     );
        if (hr != NOERROR)
        {
            break;
        }
        NotfAssert((pCPackage));

        hr = pCPackage->GetNotificationItem(pNotificationItem,(ENUM_FLAGS)dwMode);

        RELEASE(pCPackage);

        break;
    } while (TRUE);

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::FindNotification (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::DeliverNotification
//
//  Synopsis:
//
//  Arguments:  [pNotification] --
//              [rNotificationDest] --
//              [deliverMode] --
//              [pReportNotfctnSink] --
//              [ppNotfctnReport] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::DeliverNotification(
                    // the notificationid and object
                    LPNOTIFICATION      pNotification,
                    // destination
                    REFCLSID            rNotificationDest,
                    // deliver mode - group and schedule data
                    DELIVERMODE         deliverMode,
                    // info about who the sender
                    LPNOTIFICATIONSINK  pReportNotfctnSink,     // can be null - see mode
                    // the notification update interface
                    LPNOTIFICATIONREPORT *ppNotfctnReport,
                    //DWORD               dwReserved
                    PTASK_DATA          pTaskData
                    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::DeliverNotification (DELIVERMODE:%lX)\n", this, deliverMode));
    HRESULT hr = E_INVALIDARG;


    do
    {    
        if (   (deliverMode & ~DELIVERMODE_ALL)  
            || IS_BAD_NULL_INTERFACE(pNotification)
            || IS_BAD_INTERFACE(pReportNotfctnSink)
            || IS_BAD_WRITEPTR(ppNotfctnReport, sizeof(LPNOTIFICATIONREPORT))
           )
        {
            break;
        }

        // Validate task data
        if(FAILED(hr = ValidateTaskData(pTaskData)))
        {
            break;
        }

        if ((deliverMode & DM_DELIVER_DEFAULT_PROCESS)  && (!IsThereADefaultProcess()))
        {
            hr = E_FAIL;
            break;
        }

        CPackage *pCPackage = 0;
        hr = CPackage::CreateDeliver(
                                NULL,                    // class of sender
                                pReportNotfctnSink,      // can be null - see mode
                                pNotification,
                                deliverMode,
                                0, //pGroupCookie,
                                0, //pTasktrigger,
                                pTaskData,
                                rNotificationDest,
                                &pCPackage,
                                PF_DELIVERED
                                );

        BREAK_ONERROR(hr)
#if 0
        if (hr == S_FALSE)      //  Is running already.
        {
            hr = E_FAIL;
            break;
        }
#endif

        NotfAssert((pCPackage));

        if (   (ppNotfctnReport)
            && pCPackage->IsReportRequired() )
        {
            *ppNotfctnReport = pCPackage->GetNotificationReportSender(TRUE);
            NotfAssert(( *ppNotfctnReport ));
        }

        if (deliverMode & (DM_DELIVER_DELAYED | DM_THROTTLE_MODE | DM_DELIVER_LAST_DELAYED) )
        {
            hr = _rScheduleAgent.HandlePackage(pCPackage);
        }
        else
        {
            hr = _rDeliverAgent.HandlePackage(pCPackage);
        }

        // release the package
        RELEASE(pCPackage);
        pCPackage = 0;

        break;
    } while (TRUE);


    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::DeliverNotification (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::ScheduleNotification
//
//  Synopsis:
//
//  Arguments:  [pNotification] --
//              [rNotificationDest] --
//              [pTasktrigger] --
//              [pTaskData] --
//              [deliverMode] --
//              [pClsidSender] --
//              [pReportNotfctnSink] --
//              [ppNotfctnReport] --
//              [pNotificationCookie] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::ScheduleNotification(
                // the notificationid and object
                LPNOTIFICATION      pNotification,
                // destination
                REFCLSID            rNotificationDest,
                // deliver mode - group and schedule data
                PTASK_TRIGGER       pTasktrigger,
                // flags which go with the TaskTrigger
                PTASK_DATA          pTaskData,
                // the mode how it should be delivered
                DELIVERMODE         deliverMode,
                // info about who the sender
                LPCLSID             pClsidSender,           // class of sender can be NULL
                LPNOTIFICATIONSINK  pReportNotfctnSink,     // can be null - see mode
                // the notification update interface
                LPNOTIFICATIONREPORT *ppNotfctnReport,
                //the cookie of the new notification
                PNOTIFICATIONCOOKIE pNotificationCookie,
                // reserved dword
                DWORD               dwReserved
                )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::ScheduleNotification\n", this));
    HRESULT hr;

    if (   !pNotification
        || !pTasktrigger
        || (deliverMode & ~DELIVERMODE_ALL)
       )
    {
        // need notification, TaskTrigger and can not be delivered imemediately
        hr = E_INVALIDARG;
    }
    else
    {

        do
        {
            // validate task trigger
            if(FAILED(hr = ValidateTrigger(pTasktrigger)))
            {
                break;
            }

            // validate task data
            if(FAILED(hr = ValidateTaskData(pTaskData)))
            {
                break;
            }

            if (deliverMode & DM_THROTTLE_MODE)
            {
                deliverMode |= DM_NEED_COMPLETIONREPORT;
                // need a notification sink
                //
                if (!pReportNotfctnSink)
                {
                    // used the default one
                    pReportNotfctnSink = (INotificationSink *)this;
                    pReportNotfctnSink->AddRef();
                }
            }

            CPackage *pCPackage = 0;
            hr = CPackage::CreateDeliver(
                                    pClsidSender,            // class of sender
                                    pReportNotfctnSink,      // can be null - see mode
                                    pNotification,
                                    deliverMode,
                                    0,              //pGroupCookie,
                                    pTasktrigger,   //pTasktrigger,
                                    pTaskData,
                                    rNotificationDest,
                                    &pCPackage,
                                    PF_SCHEDULED
                                    );
            if (hr != NOERROR)
            {
                break;
            }

            NotfAssert((pCPackage));
            //need to have a valid task trigger now
            NotfAssert((pCPackage->GetTaskTrigger()));

            if (pNotificationCookie)
            {
                *pNotificationCookie = pCPackage->GetNotificationCookie();
            }

            if (   (ppNotfctnReport)
                && pCPackage->IsReportRequired() )
            {
                *ppNotfctnReport = pCPackage->GetNotificationReportSender(TRUE);
                NotfAssert(( *ppNotfctnReport ));
            }

            if (hr == NOERROR)
            {
                hr = _rScheduleAgent.HandlePackage(pCPackage);
            }

            if (deliverMode & DM_DELIVER_DEFAULT_PROCESS)
            {
                PostSyncDefProcNotifications();
            }
            GetGlobalNotfMgr()->SetLastUpdateDate(pCPackage);

            // release the package
            RELEASE(pCPackage);
            pCPackage = 0;

            NotfAssert((!pCPackage));

            break;
        } while ( TRUE );

    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::ScheduleNotification (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::UpdateNotification
//
//  Synopsis:
//
//  Arguments:  [pNotification] --
//              [rNotificationDest] --
//              [pTasktrigger] --
//              [pTaskData] --
//              [deliverMode] --
//              [pClsidSender] --
//              [pReportNotfctnSink] --
//              [ppNotfctnReport] --
//              [pNotificationCookie] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::UpdateNotification(
                //the cookie of the new notification
                PNOTIFICATIONCOOKIE pNotificationCookie,
                // deliver mode - group and schedule data
                PTASK_TRIGGER       pTasktrigger,
                // flags which go with the TaskTrigger
                PTASK_DATA          pTaskData,
                // the mode how it should be delivered
                DELIVERMODE         deliverMode,
                // info about who the sender
                // reserved dword
                DWORD               dwReserved
                )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::UpdateNotification\n", this));
    HRESULT hr = E_NOTIMPL;

    do
    {
        if (   IS_BAD_NULL_READPTR(pNotificationCookie, sizeof(NOTIFICATIONCOOKIE))
            || IS_BAD_NULL_READPTR(pTasktrigger, sizeof(TASK_TRIGGER))
            || (deliverMode & ~DELIVERMODE_ALL)
            || IS_BAD_READPTR(pTaskData, sizeof(TASK_DATA))
           )
        {
            // need notification, TaskTrigger and can not be delivered imemediately
            hr = E_INVALIDARG;
            break;
        }

        // validate task trigger
        if(FAILED(hr = ValidateTrigger(pTasktrigger)))
        {
            break;
        }

        // validate task data
        if(FAILED(hr = ValidateTaskData(pTaskData)))
        {
            break;
        }

        CPackage *pCPackage = 0;
        hr = _rScheduleAgent.FindPackage(
                              pNotificationCookie
                             ,&pCPackage
                             ,LM_LOCALCOPY
                             );
        if (FAILED(hr) || (!(pCPackage->GetDeliverMode() & DM_DELIVER_DEFAULT_PROCESS)))
        {
            if (pCPackage)
            {
                RELEASE(pCPackage);
            }
            hr = _rScheduleAgent.RevokePackage(
                                  pNotificationCookie
                                 ,&pCPackage
                                 ,0
                                 );
        }

        if (hr != NOERROR)
        {
            TNotfAssert((!pCPackage));
            break;
        }

        pCPackage->SetTaskTrigger(pTasktrigger, pTaskData);

        pCPackage->SetDeliverMode(deliverMode);

        pCPackage->ClearGroupCookie();

        hr = _rScheduleAgent.HandlePackage(pCPackage);

        if (deliverMode & DM_DELIVER_DEFAULT_PROCESS)
        {
            PostSyncDefProcNotifications();
        }
        GetGlobalNotfMgr()->SetLastUpdateDate(pCPackage);

        RELEASE(pCPackage);

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::UpdateNotification (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::RevokeNotification
//
//  Synopsis:
//
//  Arguments:  [ppackageCookie] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::RevokeNotification(
                        PNOTIFICATIONCOOKIE     pNotificationCookie,
                        PNOTIFICATIONITEM       pNotificationItem,
                        DWORD                   dwMode
                        )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::RevokeNotification\n", this));
    HRESULT hr = E_NOTIMPL;

    CPackage *pCPackage = NULL,
             *pPkgTmp = NULL;

    if (   (pNotificationItem)
        && (pNotificationItem->cbSize < sizeof(NOTIFICATIONITEM)))
    {
        hr = E_INVALIDARG;
    }
    else if (pNotificationItem)
    {
        hr = _rScheduleAgent.RevokePackage(pNotificationCookie,&pCPackage,dwMode);
        if (hr == NOERROR)
        {
            NotfAssert((pCPackage));
            hr = pCPackage->GetNotificationItem(pNotificationItem,(ENUM_FLAGS)dwMode);
            RELEASE(pCPackage);
        }
        else
        {
            NotfAssert((!pCPackage));
        }
    }
    else
    {
        hr = _rScheduleAgent.RevokePackage(pNotificationCookie, NULL,dwMode);
    }
    
    HRESULT hr1 = GetThrottleListAgent()->FindPackage(pNotificationCookie, &pPkgTmp, 0);
    if (SUCCEEDED(hr1) && pPkgTmp)
    {
        DWORD dwState = pPkgTmp->GetNotificationState();
        pPkgTmp->SetNotificationState(dwState | PF_REVOKED);
        RELEASE(pPkgTmp);
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::RevokeNotification (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::GetEnumNotification
//
//  Synopsis:
//
//  Arguments:  [pEnumNotification] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::GetEnumNotification(
                        DWORD                   grfFlags,
                        LPENUMNOTIFICATION     *ppEnumNotification
                        )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::RevokeItem\n", this));

    HRESULT hr = E_FAIL;

    do
    {
        if (!ppEnumNotification)
        {
            hr = E_INVALIDARG;
            break;
        }

        *ppEnumNotification = 0;

        CListAgent *pCListAgent = 0;
        if (   (grfFlags & EF_NOTIFICATION_THROTTLED)
            || (grfFlags & EF_NOTIFICATION_SUSPENDED) )
        {
            pCListAgent = GetThrottleListAgent();
        }
        else
        {
            pCListAgent = _rScheduleAgent.GetSchedListAgent();
        }
        // should never happen
        NotfAssert((pCListAgent));
        if (!pCListAgent)
        {
            break;
        }

        CEnumNotification *pCEnum = 0;
        hr = CEnumNotification::Create(
                                   pCListAgent
                                  ,(ENUM_FLAGS)grfFlags
                                  ,&pCEnum
                                  );

        if (hr != NOERROR)
        {
            break;
        }
        *ppEnumNotification = (IEnumNotification *)pCEnum;

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::RevokeItem (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::CreateScheduleGroup
//
//  Synopsis:   creates a new schedule group - the group is empty
//
//  Arguments:  [grfGroupCreateFlags] --
//              [ppSchGroup] --
//              [pGroupCookie] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::CreateScheduleGroup(
    // the group creation flags
    DWORD               grfGroupCreateFlags,
    // the new group
    LPSCHEDULEGROUP    *ppSchGroup,
    // the cookie of the group
    PNOTIFICATIONCOOKIE pGroupCookie,
    DWORD               dwReserved
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::CreateScheduleGroup\n", this));
    HRESULT hr = E_INVALIDARG;

    do
    {
        if (!ppSchGroup || !pGroupCookie)
        {
            break;
        }

        CScheduleGroup *pCSchedGroup = 0;

        hr = CScheduleGroup::Create(
                                 grfGroupCreateFlags
                                ,&pCSchedGroup
                                ,pGroupCookie
                                ,dwReserved
                                );
        if (hr != NOERROR)
        {
            break;
        }

        *ppSchGroup = (IScheduleGroup *)pCSchedGroup;

        break;
    } while (TRUE);

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::CreateScheduleGroup (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::FindScheduleGroup
//
//  Synopsis:   finds an existing group
//
//  Arguments:  [pGroupCookie] --
//              [ppSchGroup] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::FindScheduleGroup(
    PNOTIFICATIONCOOKIE pGroupCookie,
    LPSCHEDULEGROUP    *ppSchGroup,
    // the cookie of the group
    DWORD               dwReserved
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::FindScheduleGroup\n", this));

    HRESULT hr = E_INVALIDARG;

    CScheduleGroup *pCSchedGroup = 0;

    do
    {
        if (   !pGroupCookie
            || !ppSchGroup
            )
        {
            break;
        }

        hr = CScheduleGroup::LoadPersistedGroup(
                                 c_pszRegKeyScheduleGroup
                                ,pGroupCookie
                                ,0
                                ,&pCSchedGroup
                                );
        BREAK_ONERROR(hr);

        *ppSchGroup = (IScheduleGroup *)pCSchedGroup;

        // the group is addref'd
        pCSchedGroup = 0;

        break;
    } while (TRUE);

    if (pCSchedGroup)
    {
        pCSchedGroup->Release();
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::FindScheduleGroup (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::RevokeScheduleGroup
//
//  Synopsis:   revokes an entire group from the scheduler
//
//  Arguments:  [pGroupCookie] --
//              [ppSchGroup] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::RevokeScheduleGroup(
    // cookie of group to be revoked
    PNOTIFICATIONCOOKIE pGroupCookie,
    LPSCHEDULEGROUP    *ppSchGroup,
    DWORD               dwReserved
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::RevokeScheduleGroup\n", this));
    HRESULT hr = E_INVALIDARG;

    do
    {
        if (!pGroupCookie)
        {
            break;
        }

        if (IsPredefinedScheduleGroup(*pGroupCookie))
        {
            // can not revoke a predefined group
            break;
        }

        CScheduleGroup *pCSchedGroup = 0;

        hr = CScheduleGroup::LoadPersistedGroup(
                                 c_pszRegKeyScheduleGroup
                                ,pGroupCookie
                                ,0
                                ,&pCSchedGroup
                                );
        BREAK_ONERROR(hr);

        hr = pCSchedGroup->ChangeAttributes(NULL        // PTASK_TRIGGER       pTaskTrigger,
                                         ,0             // DWORD               pTaskData,
                                         ,pGroupCookie  // PNOTIFICATIONCOOKIE pGroupCookie,
                                         ,0             // PGROUPINFO          pGroupInfo
                                         ,(GROUPMODE)0  // GROUPMODE           grfGroupMode)
                                         );
        // ignore error

        hr = pCSchedGroup->RemovePersist(c_pszRegKeyScheduleGroup);
        BREAK_ONERROR(hr);

        if (ppSchGroup)
        {
            *ppSchGroup = pCSchedGroup;
        }
        else
        {
            pCSchedGroup->Release();
        }

        break;
    } while (TRUE);


    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::RevokeScheduleGroup (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::GetEnumScheduleGroup
//
//  Synopsis:   return an enumerator over the groups in the scheduler
//
//  Arguments:  [grfFlags] --
//              [ppEnumNotification] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::GetEnumScheduleGroup(
    DWORD                    grfFlags,
    LPENUMSCHEDULEGROUP     *ppEnumScheduleGroup
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::GetEnumScheduleGroup\n", this));
    HRESULT hr = E_FAIL;

    do
    {
        if (!ppEnumScheduleGroup)
        {
            hr = E_INVALIDARG;
            break;
        }

        CSchedListAgent *pCSchAgent = _rScheduleAgent.GetSchedListAgent();
        // should never happen
        NotfAssert((pCSchAgent));
        if (!pCSchAgent)
        {
            break;
        }

        CEnumSchedGroup *pCEnum = 0;
        hr = CEnumSchedGroup::Create(
                                   pCSchAgent
                                  ,(ENUM_FLAGS)grfFlags
                                  ,&pCEnum
                                  );

        if (hr != NOERROR)
        {
            break;
        }
        *ppEnumScheduleGroup = pCEnum;

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::GetEnumScheduleGroup (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::SetMode
//
//  Synopsis:
//
//  Arguments:  [rClsID] --
//              [NotificationMgrMode] --
//              [pClsIDPre] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::SetMode(
    // the clsid of the process
    REFCLSID                rClsID,
    // initialization mode
    NOTIFICATIONMGRMODE     NotificationMgrMode,
    // out previous default notification mgr
    LPCLSID                *pClsIDPre,
    // a reserved again
    DWORD                   dwReserved
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::SetMode\n", this));
    HRESULT hr = E_FAIL;

    do
    {
        if (   (NotificationMgrMode != NM_DEFAULT_PROCESS)
            || (rClsID == CLSID_NULL)
            || dwReserved)
        {
            hr = E_INVALIDARG;
            break;
        }
        if (g_pGlobalNotfMgr)
        {
            CDestinationPort cDefDest(GetThreadNotificationWnd());
            hr = g_pGlobalNotfMgr->SetDefaultDestinationPort(cDefDest, 0);
            HRESULT hr1 = g_pCNotificationMgr->UnPersistScheduleItem(0,0,0);
        }

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::SetMode (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::OnNotification
//
//  Synopsis:   delivers a notification without scheduling it
//
//  Arguments:  [pNotification] --
//              [pRunningNotfCookie] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:      used internally
//
//----------------------------------------------------------------------------
STDMETHODIMP  CNotificationMgr::OnNotification(
    // the notification object itself
    LPNOTIFICATION          pNotification,
    // the report sink if - can be NULL
    LPNOTIFICATIONREPORT    pNotfctnReport,
    DWORD                   dwReserved
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::OnNotification\n", this));

    HRESULT hr = E_FAIL;

    if (   !pNotification
        )
    {
        hr = E_INVALIDARG;
    }
    else
    {

    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::OnNotification (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::DeliverReport
//
//  Synopsis:   delivers a notification without scheduling it
//
//  Arguments:  [pNotification] --
//              [pRunningNotfCookie] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::DeliverReport(
    // the notification object itself
    LPNOTIFICATION          pNotification,
    // the cookie of the object the notification is targeted too
    PNOTIFICATIONCOOKIE     pRunningNotfCookie,
    DWORD                   dwReserved
    )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::DeliverReport\n", this));

    HRESULT hr = E_FAIL;
    CPackage *pCPackage = 0;
    CPackage *pCPackageRunning = 0;

    do
    {

        if (   !pNotification
            || !pRunningNotfCookie
            || *pRunningNotfCookie == COOKIE_NULL)
        {
            hr = E_INVALIDARG;
            break;
        }

        CNotificationReport *pRprtNew = 0;
        BOOL fLocal = TRUE;
        
        // find the package in the throttle list if local 
        CThrottleListAgent *pThrottleListAgent = GetThrottleListAgent();
        CSchedListAgent *pCSchAgent = _rScheduleAgent.GetSchedListAgent();

        if (!pCSchAgent || !pThrottleListAgent) {
            hr = E_FAIL;
            break;
        }

        NotfAssert((pThrottleListAgent));
        hr = pThrottleListAgent->FindPackage(
                                     pRunningNotfCookie
                                     ,&pCPackageRunning
                                     ,LM_LOCALCOPY
                                     );

        if (hr == NOERROR)  {
            if(!pCPackageRunning->GetNotificationSinkDest())    {
                DWORD   dwState = 0;
                dwState = pCPackageRunning->GetNotificationState();


                NOTIFICATIONTYPE   ntType;
                hr = pNotification->GetNotificationInfo(&ntType,
                                                        NULL, NULL,
                                                        NULL, NULL);
                if ((hr == NOERROR) && (ntType == NOTIFICATIONTYPE_TASKS_ABORT))
                {
                    if (!(dwState & (PF_RUNNING | PF_SUSPENDED)))  {
                        dwState &= ~PF_DELIVERED;
                        //
                        // deliver the completion report notification to the sender
                        //
                        if (    pCPackageRunning->GetNotificationReport()
                            &&  (pCPackageRunning->GetDeliverMode() & (DM_NEED_COMPLETIONREPORT | DM_NEED_PROGRESSREPORT)) ) 
                        {
                            LPNOTIFICATION pNotification = 0;
                            
                            hr = GetNotificationMgr()->CreateNotification(
                                             NOTIFICATIONTYPE_TASKS_COMPLETED
                                            ,0
                                            ,0
                                            ,&pNotification
                                            ,0);
                            if (hr == NOERROR)
                            {
                                WriteGUID(pNotification, WZ_COOKIE, pRunningNotfCookie);

                                pCPackageRunning->GetNotificationReport()->DeliverUpdate(pNotification, 0 , 0);
                                pNotification->Release();
                                pNotification = NULL;
                            }
                        } 

                        hr = pThrottleListAgent->RevokePackage(
                                                        pRunningNotfCookie,
                                                        0,
                                                        1);
                        //  BUGBUG.
                        //  Should we break or continue (so we can delete the
                        //  globally running one)?
                        RELEASE(pCPackageRunning);
                        pCPackageRunning = NULL;
                        break;
                    }
                }
                RELEASE(pCPackageRunning);
                pCPackageRunning = NULL;
                hr = E_FAIL;
            }
        }

        if (FAILED(hr))
        {
            hr = pCSchAgent->FindPackage(
                                         pRunningNotfCookie
                                         ,&pCPackageRunning
                                         ,LM_LOCALCOPY
                                         );
        }                                         
        
        if (   (hr == NOERROR)
            && (!pCPackageRunning->GetNotificationSinkDest() ))
        {
            RELEASE(pCPackageRunning);
            pCPackageRunning = 0;
            hr = E_FAIL;
        }
        
        if (FAILED(hr))
        {
            fLocal = FALSE;
            hr = pCSchAgent->FindPackage(
                                         pRunningNotfCookie
                                         ,&pCPackageRunning
                                         ,0
                                         );

        }

        if (hr != NOERROR)  {
            break;
        }

        NotfAssert((pCPackageRunning));

        if (!fLocal)    {
            CDestinationPort & rCDest = pCPackageRunning->GetCDestPort();
            NotfAssert(( rCDest.GetPort() != GetThreadNotificationWnd() ));
            if (!IsWindow(rCDest.GetPort()))    {
                RELEASE(pCPackageRunning);
                pCPackageRunning = 0;
                hr = E_FAIL;
                break;
            }
        }

        if ( !(pCPackageRunning->GetNotificationState() & (PF_RUNNING | PF_SUSPENDED) ) )
        {
            // not running or supended - nothing to do
            hr = E_FAIL;
            break;
        }
        NotfAssert((pCPackageRunning));
        
        //  We should get the Sender!
        pRprtNew = pCPackageRunning->GetNotificationReportSender();

        DWORD dwNotfState = pCPackageRunning->GetNotificationState();
        //  Set the XProcess flag;
        if (!fLocal)    {
            pCPackageRunning->SetNotificationState(dwNotfState | PF_CROSSPROCESS);
        }

        BREAK_ONERROR(hr);

        if (!pRprtNew)
        {
            hr = E_FAIL;
            break;
        }

        hr = CPackage::CreateUpdate(
                                pRprtNew,
                                pNotification,
                                0, //deliverMode,
                                dwReserved,
                                &pCPackage
                                );


        if (hr == NOERROR)
        {
            NotfAssert((pCPackage));

            if (fLocal)
            {
                INotificationSink *pNotfSinkDest = pCPackageRunning->GetNotificationSinkDest();
                //NotfAssert((pNotfSinkDest));
                pCPackage->SetNotificationSinkDest(pNotfSinkDest);
            } else  {
                dwNotfState = pCPackage->GetNotificationState()|PF_CROSSPROCESS;
                pCPackage->SetNotificationState(dwNotfState);
                pCPackage->SetDestID(pCPackageRunning->GetDestID());
                CDestinationPort &rCDest = pCPackageRunning->GetCDestPort();
                pCPackage->SetDestPort(rCDest.GetPort(), rCDest.GetDestThreadId() );
            }

            RELEASE(pCPackageRunning);
            pCPackageRunning = 0;

            hr = _rDeliverAgent.HandlePackage(pCPackage);

            RELEASE(pCPackage);
            pCPackage = 0;
        }
        else
        {
            RELEASE(pCPackageRunning);
            pCPackageRunning = 0;
        }
        NotfAssert((!pCPackage));

        break;
    }
    while (TRUE);

    if (pCPackageRunning)
    {
        RELEASE(pCPackageRunning);
    }

    if (pCPackage)
    {
        RELEASE(pCPackage);
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::DeliverReport (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::RegisterThrottleNotificationType
//
//  Synopsis:
//
//  Arguments:  [cItems] --
//              [pThrottleItems] --
//              [pcItemsOut] --
//              [ppThrottleItemsOut] --
//              [dwMode] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationMgr::RegisterThrottleNotificationType(
        ULONG               cItems,
        PTHROTTLEITEM       pThrottleItems,
        ULONG              *pcItemsOut,
        PTHROTTLEITEM      *ppThrottleItemsOut,
        DWORD               dwMode,
        DWORD               dwReserved
        )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::RegisterThrottleNotificationType\n", this));

    HRESULT hr = E_FAIL;
    CPackage *pCPackage = 0;
    CPackage *pCPackageRunning = 0;

    do
    {

        if (   !cItems
            || !pThrottleItems
            || dwMode
            || dwReserved
            )
        {
            hr = E_INVALIDARG;
            break;
        }

        NotfAssert(( GetGlobalNotfMgr() ));

        hr = GetGlobalNotfMgr()->RegisterThrottleNotificationType(
                                                            cItems,
                                                            pThrottleItems,
                                                            pcItemsOut,
                                                            ppThrottleItemsOut,
                                                            dwMode,
                                                            dwReserved);

        break;
    }
    while (TRUE);


    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::RegisterThrottleNotificationType (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotificationMgr::UnPersistScheduleItem
//
//  Synopsis:
//
//  Arguments:  [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CNotificationMgr::UnPersistScheduleItem(
                                                LPCSTR pszWhere,
                                                LPSTR pszSubKeyIn,
                                                DWORD dwMode)
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::UnPersistScheduleItem\n",this));
    HRESULT hr = E_INVALIDARG;
    CPackage *pCPkg = 0;
    HKEY      hKey = 0;

    // construct the new location
    // this location is used to save new items
    //strcpy((LPSTR)c_pszRegKey,SZNOTF_ROOT);
    //strcat((LPSTR)c_pszRegKey,SZNOTF_SCHEDITEMS);

    if (c_rgszRegKey[0] == '\0')
    {
        strcpy(c_rgszRegKey,SZNOTF_ROOT);
        strcat(c_rgszRegKey,SZNOTF_SCHEDITEMS);
        c_pszRegKey = c_rgszRegKey;
    }

    if ((GetGlobalNotfMgr()->GetProcessId() > 1) && !AreWeTheDefaultProcess())
    {
        // nothing to do - items already reloaded
    }
    else do
    {
        // now reload all the peristed packages

        long  lRes;
        DWORD dwDisposition, dwIndex = 0;
        DWORD dwValueLen = SZREGVALUE_MAX;
        CHAR  szLocation[SZREGSETTING_MAX];
        CHAR  szValue[SZREGVALUE_MAX];

        strcpy(szLocation,SZNOTF_ROOT);
        strcat(szLocation,SZNOTF_SCHEDITEMS);


        lRes = RegCreateKeyEx(HKEY_CURRENT_USER, c_pszRegKey, 0, NULL, 0,
                              HKEY_READ_WRITE_ACCESS, NULL, &hKey, &dwDisposition);

        // loop over all elements and schedule the elements
        while ((lRes == ERROR_SUCCESS) && (ERROR_NO_MORE_ITEMS != lRes))
        {
            DWORD dwType;
            DWORD dwNameLen;

            dwNameLen = SZREGVALUE_MAX;
            lRes = RegEnumKey(hKey, dwIndex, szValue, dwNameLen);
            dwIndex++;

            if ((lRes == ERROR_SUCCESS) && (ERROR_NO_MORE_ITEMS != lRes))
            {
                NotfAssert((pCPkg == 0));
                hr = CPackage::LoadFromPersist(szLocation,szValue, 0, &pCPkg);
                if (hr == NOERROR)
                {
                    NotfAssert((pCPkg));
                    hr = E_FAIL;

                    // update state
                    if (   (pCPkg->GetNotificationState() & PF_WAITING)
                        && (SUCCEEDED(ValidateTrigger(pCPkg->GetTaskTrigger())))
                        && ( (pCPkg->GetPackageType() != PT_REPORT_TO_DEST) && (pCPkg->GetPackageType() != PT_REPORT_TO_SENDER)) )
                    {
                        pCPkg->SetNotificationState(PF_SCHEDULED | PF_WAITING | PF_READY);

                        //  Nuke the running cookie and dest port.
                        pCPkg->SetRunningCookie(NULL);
                        pCPkg->SetDestPort(NULL, 0);
                        pCPkg->CalcNextRunDate();
                        hr = _rScheduleAgent.HandlePackage(pCPkg);
                        PPKG_DUMP(pCPkg, (DEB_TFLOW | DEB_TMEMORY, "Created in CNotificationMgr::UnPersistScheduleItem", NOERROR));
                    }
                }
                //
                // delete the item in case of error
                //
                if (hr != NOERROR)
                {
                    char szKeyToDelete[1024];
                    strcpy(szKeyToDelete, szLocation);
                    strcat(szKeyToDelete, szValue);
                    lRes = RegDeleteKey(HKEY_CURRENT_USER,szKeyToDelete);
                    if (lRes == ERROR_SUCCESS)
                    {
                        dwIndex--;
                    }
                }
            }

            if (pCPkg)
            {
                RELEASE(pCPkg);
                pCPkg = 0;
            }

        } // end while keys

        break;
    } while (TRUE);

    if (pCPkg)
    {
        RELEASE(pCPkg);
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    NotfDebugOut((DEB_MGR, "%p OUT UnPersistScheduleItem(hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::::ReadRegSetting
//
//  Synopsis:
//
//  Arguments:  [pwzMime] --
//              [pclsid] --
//              [pcClsIds] --
//
//  Returns:
//
//  History:    11-20-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CNotificationMgr::ReadRegSetting(LPCSTR pszRoot, LPCSTR pszKey,
                                         LPSTR pszValue, DWORD dwSize,CLSID *pclsid)
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::::ReadRegSetting (pszRoot:%s)\n", this,pszRoot));
    HRESULT hr = E_FAIL;
    NotfAssert((pszRoot && pszKey));

    HKEY hKey = NULL;
    char szValue[SZREGVALUE_MAX];
    DWORD dwValueLen = SZREGVALUE_MAX;


    if (RegOpenKeyEx(HKEY_CURRENT_USER, pszRoot, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwType;
        if (RegQueryValueEx(hKey, pszKey, NULL, &dwType, (LPBYTE)szValue, &dwValueLen) == ERROR_SUCCESS)
        {
            if (pszValue && (dwValueLen <= dwSize))
            {
                strcpy(pszValue,szValue);
                hr = NOERROR;
            }
            if (pclsid)
            {
                hr = CLSIDFromStringA(szValue, pclsid);
            }
        }

        RegCloseKey(hKey);
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::ReadRegSetting (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::WriteRegSetting
//
//  Synopsis:
//
//  Arguments:  [pszRoot] --
//              [pszKey] --
//              [pszValue] --
//              [dwSize] --
//              [pclsid] --
//
//  Returns:
//
//  History:    1-18-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CNotificationMgr::WriteRegSetting(LPCSTR pszRoot, LPCSTR pszKey,
                                          LPSTR pszValue, DWORD dwSize, CLSID *pclsid)
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::::WriteRegSetting (pszRoot:%s)\n", this,pszRoot));
    HRESULT hr = E_FAIL;
    NotfAssert((pszRoot && pszKey));
    DWORD dwDisposition;


    HKEY hKey = NULL;
    char szValue[SZREGVALUE_MAX];
    DWORD dwValueLen = SZREGVALUE_MAX;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, pszRoot, 0, NULL, 0, HKEY_READ_WRITE_ACCESS,NULL, &hKey,&dwDisposition) == ERROR_SUCCESS)
    {
        DWORD dwError = 1;
        if (pszValue)
        {
            dwError = RegSetValueEx(hKey, pszKey, NULL, REG_SZ,(const BYTE *)pszValue, dwSize);
        }
        else if (pclsid)
        {
            pszValue = StringAFromCLSID(pclsid);
            if (pszValue)
            {
                dwSize =  strlen(pszValue);
                dwError = RegSetValueEx(hKey, pszKey, NULL, REG_SZ,(const BYTE *)pszValue, dwSize);
                delete pszValue;
            }
        }
        if (dwError == ERROR_SUCCESS)
        {
            hr = NOERROR;
        }

        RegCloseKey(hKey);
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::WriteRegSetting (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::DeletRegSetting
//
//  Synopsis:
//
//  Arguments:  [pszRoot] --
//              [LPSTR] --
//              [pszValue] --
//
//  Returns:
//
//  History:    1-19-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CNotificationMgr::DeletRegSetting(LPCSTR pszRoot, LPCSTR pszKey,LPSTR pszValue)
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::DeletRegSetting (pszRoot:%s)\n", this,pszRoot));
    HRESULT hr = E_FAIL;
    HKEY hKey = NULL;
    DWORD   dwDisposition;

    NotfAssert((pszRoot && pszKey));
    if (RegCreateKeyEx(HKEY_CURRENT_USER,pszRoot,0,NULL,0,HKEY_READ_WRITE_ACCESS,
                        NULL,&hKey,&dwDisposition) == ERROR_SUCCESS)
    {
        if (RegDeleteKey(hKey, pszKey) == ERROR_SUCCESS)
        {
            hr = NOERROR;
        }

        RegCloseKey(hKey);
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::DeletRegSetting (hr:%lx)\n",this, hr));
    return hr;
}
#ifdef _unused_
//+---------------------------------------------------------------------------
//
//  Function:   UnPersistScheduleItem
//
//  Synopsis:
//
//  Arguments:  [pszStr] --
//              [pNotMgr] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT UnPersistScheduleItem(LPCSTR pszStr, CNotificationMgr *pNotMgr, DWORD dwMode)
{
    NotfDebugOut((DEB_MGR, "API _IN UnPersistScheduleItem\n"));
    HRESULT hr = E_INVALIDARG;
    CPackage *pCPkg = 0;
    HKEY            hKey = 0;
    const cbStrKeyLen = 256;

    do
    {
        if (   !pszStr
               || !pNotMgr)
        {
            BREAK_ONERROR(hr);
        }


        long   lRes, lCookie, lCount=0;
        DWORD  dwDisposition, dwIndex = 0, dwType, dwNameLen;
        CHAR   pszValueName[cbStrKeyLen];

        lRes = RegCreateKeyEx(HKEY_CURRENT_USER, c_pszRegKey, 0, NULL, 0,
                              HKEY_READ_WRITE_ACCESS, NULL, &hKey, &dwDisposition);

        if (lRes != ERROR_SUCCESS)
        {
            hKey = 0;
            break;
        }

        while (ERROR_NO_MORE_ITEMS != lRes)
        {
            dwNameLen = cbStrKeyLen;
            lRes = RegEnumValue(hKey, dwIndex, pszValueName, &dwNameLen,
                                NULL, &dwType, NULL, NULL);
            dwIndex++;

            if (ERROR_NO_MORE_ITEMS != lRes)
            {
                NotfAssert((pCPkg == 0));

                hr = CPackage::LoadFromPersist(c_pszRegKey,pszValueName, 0, &pCPkg);
                BREAK_ONERROR(hr);

                NOTIFICATIONITEM nfItem;
                memset(&nfItem, 0, sizeof(NOTIFICATIONITEM));
                nfItem.cbSize = sizeof(NOTIFICATIONITEM);

                hr = pCPkg->GetNotificationItem(&nfItem);

                BREAK_ONERROR(hr);

                hr = pNotMgr->ScheduleNotification(
                        nfItem.pNotification                        // LPNOTIFICATION   pNotification,
                       ,nfItem.clsidDest                            // REFDESTID           rNotificationDest,
                       ,&nfItem.TaskTrigger                         // PTASK_TRIGGER       pTasktrigger,
                       ,&nfItem.TaskData                       // DWORD               pTaskData,
                       ,pCPkg->GetDeliverMode()                     // DELIVERMODE         deliverMode,
                       ,&nfItem.clsidSender                         // LPCLSID             pClsidSender,           // class of sender can be NUL
                       ,0                                           // LPNOTIFICATIONSINK  pReportNotfctnSink,     // can be null - see mode
                       ,0                                           // LPNOTIFICATIONREPORT *ppNotfctnReport,
                       ,0                                           // PNOTIFICATIONCOOKIE pNotificationCookie,
                       ,0                                           // DWORD               dwReserved
                       );
                if (nfItem.pNotification)
                {
                    nfItem.pNotification->Release();
                }
            }

            if (pCPkg)
            {
                RELEASE(pCPkg);
                pCPkg = 0;
            }

        } // end while keys

        break;
    } while (TRUE);

    if (pCPkg)
    {
        RELEASE(pCPkg);
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    NotfDebugOut((DEB_MGR, "API OUT UnPersistScheduleItem(hr:%lx)\n", hr));
    return hr;
}
#endif // _unused_

//+---------------------------------------------------------------------------
//
//  Function:   CNotificationMgr::UnPersistDestinationItem
//
//  Synopsis:
//
//  Arguments:  [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CNotificationMgr::UnPersistDestinationItem(
                                                   LPCSTR pszWhere,
                                                   LPSTR pszSubKeyIn,
                                                   DWORD dwMode)
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::UnPersistDestinationItem\n",this));
    HRESULT hr = NOERROR;
    CDestination *pCDest = 0;
    HKEY      hKey = 0;
    DWORD dwValueLen = SZREGVALUE_MAX;
    char szValue[SZREGVALUE_MAX];

    do
    {
        long   lRes;
        DWORD  dwDisposition, dwIndex = 0;
        CHAR   szLocation[SZREGSETTING_MAX];

        // construct the new location
        strcpy(szLocation,SZNOTF_ROOT);
        strcat(szLocation,SZNOTF_DESTINATION);
        strcat(szLocation,"\\");

        lRes = RegCreateKeyEx(HKEY_CURRENT_USER, szLocation, 0, NULL, 0,
                              HKEY_READ_WRITE_ACCESS, NULL, &hKey, &dwDisposition);


        // loop over all elements and pass on the items to the deliver agent
        while ((lRes == ERROR_SUCCESS) && (ERROR_NO_MORE_ITEMS != lRes))
        {
            DWORD dwType;
            DWORD dwNameLen;

            dwNameLen = SZREGVALUE_MAX;
            lRes = RegEnumValue(hKey, dwIndex, szValue, &dwNameLen,
                                NULL, &dwType, NULL, NULL);
            dwIndex++;

            if ((lRes == ERROR_SUCCESS) && (ERROR_NO_MORE_ITEMS != lRes))
            {
                NotfAssert((pCDest == 0));

                hr = CDestination::LoadFromPersist(szLocation, szValue, 0, &pCDest);

                if (hr == NOERROR)
                {
                    NotfAssert((pCDest));

                    DESTINATIONDATA destData;

                    pCDest->GetDestinationData(&destData);

                    NotfAssert(( pCDest->GetNotfTypes() ));

                    //
                    // turn of the persist flag - no need to repersist again
                    //
                    destData.NotfctnSinkMode &= ~NM_PERMANENT;

                    hr = _rDeliverAgent.Register(
                                             0   //pNotfctnSink,
                                            ,&destData.NotificationDest  //pDestID,
                                            ,destData.NotfctnSinkMode    //asMode,
                                            ,destData.cNotifications    //cNotifications,
                                            ,pCDest->GetNotfTypes()     //pNotificationIDs,
                                            ,&destData.RegisterCookie    //pRegisterCookie,
                                            ,destData.dwReserved        //dwReserved
                                            );

                }

            }

            if (pCDest)
            {
                pCDest->Release();
                pCDest = 0;
            }

        } // end while keys

        break;
    } while (TRUE);

    if (pCDest)
    {
        pCDest->Release();
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::UnPersistDestinationItem(hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationMgr::CreateDefaultScheduleGroups
//
//  Synopsis:   creates the default schedule groups
//
//  Arguments:  [dwMode] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CNotificationMgr::CreateDefaultScheduleGroups(DWORD dwMode)
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::CreateDefaultScheduleGroups\n",this));
    HRESULT hr = NOERROR;
    TASK_TRIGGER TaskTrigger;
    IScheduleGroup *pSchGroup = 0;
    // make sur the default items are avaiable

    HRESULT hr1 = FindScheduleGroup(
                        (LPCLSID)&NOTFCOOKIE_SCHEDULE_GROUP_DAILY,
                        &pSchGroup,
                        0
                       );
    if (hr1 != NOERROR)
    {
        TaskTrigger = *GetDefTaskTrigger(TRUE);

        TaskTrigger.rgFlags = 0;

        // TaskTrigger.TriggerType = TASK_TIME_TRIGGER_DAILY;
        // TaskTrigger.Type.Daily.DaysInterval = 1;
        NotfAssert((TaskTrigger.TriggerType == TASK_TIME_TRIGGER_DAILY));
        NotfAssert((TaskTrigger.Type.Daily.DaysInterval == 1));

        LPWSTR pwzName = GetResourceString(ID_SCHEDULE_GROUP_DAILY);
        if (pwzName)
        {
            hr1 = CScheduleGroup::CreateDefScheduleGroup(
                            (REFIID)NOTFCOOKIE_SCHEDULE_GROUP_DAILY,
                            &TaskTrigger,
                            //SCHEDULE_GROUP_NAME_DAILY
                            pwzName
                            );
        }
        delete pwzName;                        
    }
    else
    {
        NotfAssert((pSchGroup));
        pSchGroup->Release();
    }
    hr1 = FindScheduleGroup(
                        (LPCLSID)&NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY,
                        &pSchGroup,
                        0
                       );

    if (hr1 != NOERROR)
    {
        TaskTrigger = *GetDefTaskTrigger(TRUE);

        TaskTrigger.rgFlags = 0;
        TaskTrigger.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
        TaskTrigger.Type.Weekly.WeeksInterval = 1;
        TaskTrigger.Type.Weekly.rgfDaysOfTheWeek = TASK_MONDAY;

        LPWSTR pwzName = GetResourceString(ID_SCHEDULE_GROUP_WEEKLY);
        if (pwzName)
        {
            hr1 = CScheduleGroup::CreateDefScheduleGroup(
                        (REFIID)NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY,
                        &TaskTrigger,
                        pwzName
                        );
        }
        delete pwzName;                        
    }
    else
    {
        NotfAssert((pSchGroup));
        pSchGroup->Release();
    }

    hr1 = FindScheduleGroup(
                        (LPCLSID)&NOTFCOOKIE_SCHEDULE_GROUP_MONTHLY,
                        &pSchGroup,
                        0
                        );

    if (hr1 != NOERROR)
    {
        TaskTrigger = *GetDefTaskTrigger(TRUE);

        TaskTrigger.rgFlags = 0;
        TaskTrigger.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
        TaskTrigger.Type.MonthlyDate.rgfDays = 0x00000001;
        TaskTrigger.Type.MonthlyDate.rgfMonths = 0x0FFF;     // TASK_JANUARY -> TASK_DECEMBER

        LPWSTR pwzName = GetResourceString(ID_SCHEDULE_GROUP_MONTHLY);
        if (pwzName)
        {

            hr1 = CScheduleGroup::CreateDefScheduleGroup(
                        (REFIID)NOTFCOOKIE_SCHEDULE_GROUP_MONTHLY,
                        &TaskTrigger,
                        pwzName
                        );
        }
        delete pwzName;                        
    }
    else
    {
        NotfAssert((pSchGroup));
        pSchGroup->Release();
    }


    hr1 = FindScheduleGroup(
                        (LPCLSID)&NOTFCOOKIE_SCHEDULE_GROUP_MANUAL,
                        &pSchGroup,
                        0
                        );

    if (hr1 != NOERROR)
    {
        TaskTrigger = *GetDefTaskTrigger();

        TaskTrigger.rgFlags = TASK_TRIGGER_FLAG_DISABLED;
        TaskTrigger.TriggerType = (TASK_TRIGGER_TYPE)TASK_TRIGGER_FLAG_DISABLED;

        LPWSTR pwzName = GetResourceString(ID_SCHEDULE_GROUP_MANUAL);
        if (pwzName)
        {
            hr1 = CScheduleGroup::CreateDefScheduleGroup(
                                (REFIID)NOTFCOOKIE_SCHEDULE_GROUP_MANUAL,
                                &TaskTrigger,
                                pwzName
                                );
        }
        delete pwzName;                        
    }
    else
    {
        NotfAssert((pSchGroup));
        pSchGroup->Release();
    }


    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::CreateDefaultScheduleGroups(hr:%lx)\n", this, hr));
    return hr;
}

HRESULT CNotificationMgr::UnPersistRunningItems(
                                                LPCSTR pszWhere,
                                                LPSTR pszSubKeyIn,
                                                DWORD dwMode,
                                                CPackage *prgCSchdGrp[],
                                                ULONG cElements,
                                                ULONG *pcElementFilled
                                                )
{
    NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::UnPersistScheduleItem\n",NULL));
    HRESULT hr = E_INVALIDARG;
    CPackage *pCSchGrp = 0;
    HKEY      hKey = 0;
    if (!pszWhere)
    {
        pszWhere = c_pszRegKeyScheduleGroup;
    }

    do
    {
        // now reload all the peristed packages
        long  lRes;
        DWORD dwDisposition, dwIndex = 0;
        ULONG i = 0;
        DWORD dwValueLen = SZREGVALUE_MAX;
        CHAR  szLocation[SZREGSETTING_MAX];
        CHAR  szValue[SZREGVALUE_MAX];

        lRes = RegCreateKeyEx(HKEY_CURRENT_USER, pszWhere, 0, NULL, 0,
                              HKEY_READ_WRITE_ACCESS, NULL, &hKey, &dwDisposition);

        // loop over all elements and schedule the elements
        while ((lRes == ERROR_SUCCESS) && (ERROR_NO_MORE_ITEMS != lRes))
        {
            DWORD dwType;
            DWORD dwNameLen;

            dwNameLen = SZREGVALUE_MAX;
            lRes = RegEnumValue(hKey, dwIndex, szValue, &dwNameLen,
                                NULL, &dwType, NULL, NULL);
            dwIndex++;

            if ((lRes == ERROR_SUCCESS) && (ERROR_NO_MORE_ITEMS != lRes))
            {
                NotfAssert((pCSchGrp == 0));

                hr = CPackage::LoadFromPersist(pszWhere,szValue, 0, &pCSchGrp);

                if (hr == NOERROR)
                {
                    NotfAssert((pCSchGrp));
                    prgCSchdGrp[i] = pCSchGrp;
                    i++;
                    pCSchGrp = 0;
                }
            }

            if (pCSchGrp)
            {
                pCSchGrp->Release();
                pCSchGrp = 0;
            }

        } // end while keys

        *pcElementFilled = i;

        break;
    } while (TRUE);

    if (pCSchGrp)
    {
        pCSchGrp->Release();
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::UnPersistScheduleItem(hr:%lx)\n", NULL, hr));
    return hr;
}

