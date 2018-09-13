//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       schgroup.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>

//+---------------------------------------------------------------------------
//
//  Function:   IsPredefinedScheduleGroup
//
//  Synopsis:
//
//  Arguments:  [rGroupCookie] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsPredefinedScheduleGroup(REFNOTIFICATIONCOOKIE rGroupCookie)
{
    return (   (rGroupCookie == NOTFCOOKIE_SCHEDULE_GROUP_DAILY )
            || (rGroupCookie == NOTFCOOKIE_SCHEDULE_GROUP_WEEKLY)
            || (rGroupCookie == NOTFCOOKIE_SCHEDULE_GROUP_MONTHLY)
            || (rGroupCookie == NOTFCOOKIE_SCHEDULE_GROUP_MANUAL)
           );
}


//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::CreateDefScheduleGroup
//
//  Synopsis:
//
//  Arguments:  [rGroupCookie] --
//              [TriggerType] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::CreateDefScheduleGroup(REFNOTIFICATIONCOOKIE rGroupCookie,
                                               PTASK_TRIGGER pTaskTrigger, LPCWSTR pwzName)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::CreateDefScheduleGroup\n",NULL));
    HRESULT hr = E_INVALIDARG;
    CScheduleGroup *pCSchedGroup = 0;


    do
    {
        SCHEDULEGROUPITEM ScheduleGroupItem;
        DWORD grfGroupCreateFlags = 0;

        pCSchedGroup = new CScheduleGroup(GT_STATIC);

        if (!pCSchedGroup)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        memset(&ScheduleGroupItem, 0, sizeof(SCHEDULEGROUPITEM));
        ScheduleGroupItem.cbSize      = sizeof(SCHEDULEGROUPITEM);
        ScheduleGroupItem.cElements   = 0;
        ScheduleGroupItem.GroupCookie = rGroupCookie;
        ScheduleGroupItem.grpState    = GS_Initialized;
        ScheduleGroupItem.grfGroupMode= 0;
        ScheduleGroupItem.TaskTrigger = *pTaskTrigger;
        ScheduleGroupItem.GroupType   = GT_STATIC;
        ScheduleGroupItem.GroupInfo.cbSize = sizeof(GROUPINFO);
        ScheduleGroupItem.GroupInfo.pwzGroupname = (pwzName) ? OLESTRDuplicate(pwzName) : 0;

        hr = pCSchedGroup->SetScheduleGroupItem(&ScheduleGroupItem, TRUE);
        BREAK_ONERROR(hr);

        // add the scheduled group to the group list

        hr = pCSchedGroup->SaveToPersist(c_pszRegKeyScheduleGroup);


        break;
    } while (TRUE);


    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::CreateDefScheduleGroup(hr:%lx)\n", pCSchedGroup, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::Create
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//              [ppCSchedGroup] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-27-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::CreateForEnum( CPackage  *pCPackage,
                                       CScheduleGroup    **ppCSchedGroup,
                                       DWORD dwReserved)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::CreateForEnum\n", NULL));
    HRESULT hr = NOERROR;
    CScheduleGroup *pCSchGroup = 0;

    do
    {
        if (   !ppCSchedGroup
            || !pCPackage->GetGroupCookie())
        {
            hr = E_INVALIDARG;
            break;
        }
        PNOTIFICATIONCOOKIE pGroupCookie = pCPackage->GetGroupCookie();

        //
        // create an empty group - the group is not scheduled yet
        //
        pCSchGroup = new CScheduleGroup(pGroupCookie,GS_Running);
        if (!pCSchGroup)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        if (pCPackage->GetTaskTrigger())
        {
            pCSchGroup->_TaskTrigger = *pCPackage->GetTaskTrigger();
        }
        if (pCPackage->GetTaskData())
        {
            pCSchGroup->_TaskData = *pCPackage->GetTaskData();
        }

        *ppCSchedGroup = pCSchGroup;
        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::CreateForEnum (hr:%lx)\n", pCSchGroup,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::Create
//
//  Synopsis:   creates an new group
//
//  Arguments:  [grfGroupCreateFlags] --
//              [ppCSchedGroup] --
//              [pGroupCookie] --
//
//  Returns:
//
//  History:    1-23-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::Create(DWORD  grfGroupCreateFlags,
                               CScheduleGroup **ppCSchedGroup,
                               PNOTIFICATIONCOOKIE pGroupCookie,
                               DWORD dwReserved)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::Create\n", NULL));
    HRESULT hr = NOERROR;
    CScheduleGroup *pCSchGroup = 0;

    do
    {
        if (!ppCSchedGroup || !pGroupCookie)
        {
            hr = E_INVALIDARG;
            break;
        }
        //
        // create an empty group - the group is not scheduled yet
        //
        pCSchGroup = new CScheduleGroup(grfGroupCreateFlags
                                       ,pGroupCookie
                                       ,GetDefTaskTrigger());
        if (!pCSchGroup)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        //
        //
        pCSchGroup->_GroupCookie = pCSchGroup->_CGroupCookie;
        *pGroupCookie = pCSchGroup->_CGroupCookie;

        // persit the new schedule group
        hr = pCSchGroup->SaveToPersist(c_pszRegKeyScheduleGroup);

        *ppCSchedGroup = pCSchGroup;
        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::Create (hr:%lx)\n", pCSchGroup,hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::QueryInterface
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
STDMETHODIMP CScheduleGroup::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_IScheduleGroup))
    {
        *ppvObj = (IScheduleGroup *)this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CScheduleGroup::AddRef
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
STDMETHODIMP_(ULONG) CScheduleGroup::AddRef(void)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::AddRef\n", this));

    LONG lRet = ++_CRefs;

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CScheduleGroup::Release
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
STDMETHODIMP_(ULONG) CScheduleGroup::Release(void)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::Release\n", this));
    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::SetAttributes
//
//  Synopsis:   set and changes the attributes of the group
//
//  Arguments:  [pTaskTrigger] --
//              [grfTasktrigger] --
//              [pGroupCookie] --
//              [grfGroupMode] --
//
//  Returns:
//
//  History:    1-23-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::SetAttributes(
    // new  task trigger
    PTASK_TRIGGER       pTaskTrigger,
    // the flags which go with the TaskTrigger
    PTASK_DATA          pTaskData,
    // new group cookie if sequential
    PNOTIFICATIONCOOKIE pGroupCookie,
    // group info 
    PGROUPINFO          pGroupInfo,
    // sequential or parallel
    GROUPMODE           grfGroupMode
    )
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::SetAttributes\n", this));
    HRESULT hr = NOERROR;

    do
    {
        if (   !pTaskTrigger
            || pTaskData        //  We haven't decided how to handle this so fail
            || (grfGroupMode & GM_GROUP_SEQUENTIAL)
           )
        {
            // need TaskTrigger and not sequential mode yet
            hr = E_INVALIDARG;
            break;
        }

        // validate new TaskTrigger
        if(FAILED(hr = ValidateTrigger(pTaskTrigger)))
        {
            break;
        }

        if (    pTaskTrigger
            && !(pTaskTrigger->rgFlags & TASK_TRIGGER_FLAG_DISABLED))
        {
            CFileTime date;

            pTaskTrigger->Reserved2 = 0;
            // get the run date
            hr = CheckNextRunDate(pTaskTrigger, NULL, &date);
        }
        BREAK_ONERROR(hr);

        if (!IsInitialized())
        {
            hr = InitAttributes(
                               pTaskTrigger,
                               NULL,
                               pGroupCookie,
                               pGroupInfo,
                               grfGroupMode);
        }
        else
        {

            hr = ChangeAttributes(
                               pTaskTrigger,
                               NULL,
                               pGroupCookie,
                               pGroupInfo,
                               grfGroupMode);
        }
        BREAK_ONERROR(hr);
        hr = SaveToPersist(c_pszRegKeyScheduleGroup);

        PostSyncDefProcNotifications();

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::SetAttributes (hr:%lx)\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::GetAttributes
//
//  Synopsis:   retrieves the current attributes
//
//  Arguments:  [pTaskTrigger] --
//              [pgrfTasktrigger] --
//              [pGroupCookie] --
//              [pgrfGroupMode] --
//              [pElements] --
//
//  Returns:
//
//  History:    1-23-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::GetAttributes(
    // new  task trigger
    PTASK_TRIGGER       pTaskTrigger,
    // the flags which go with the TaskTrigger
    PTASK_DATA          pTaskData,
    // new group cookie if sequential
    PNOTIFICATIONCOOKIE pGroupCookie,
    // group info 
    PGROUPINFO          pGroupInfo,
    // sequential or parallel
    GROUPMODE          *pgrfGroupMode,
    // number of elements in group
    LONG               *pElements
    )
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::GetAttributes\n", this));
    HRESULT hr;

    if (IS_BAD_WRITEPTR(pTaskTrigger, sizeof(TASK_TRIGGER)) ||
        pTaskData ||
        IS_BAD_WRITEPTR(pGroupCookie, sizeof(NOTIFICATIONCOOKIE)) ||
        IS_BAD_WRITEPTR(pGroupInfo, sizeof(GROUPINFO)) ||
        IS_BAD_WRITEPTR(pgrfGroupMode, sizeof(GROUPMODE)) ||
        IS_BAD_WRITEPTR(pElements, sizeof(LONG))
       )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = RetrieveAttributes(pTaskTrigger,
                                pTaskData,
                                pGroupCookie,
                                pGroupInfo,
                                pgrfGroupMode,
                                pElements);
    }

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::GetAttributes(hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::AddNotification
//
//  Synopsis:   adds a notification to an existing group
//
//  Arguments:  [pNotification] --
//              [rNotificationDest] --
//              [deliverMode] --
//              [pClsidSender] --
//              [pReportNotfctnSink] --
//              [ppNotfctnReport] --
//              [pNotificationCookie] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-23-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::AddNotification(
                // the notificationid and object
    LPNOTIFICATION      pNotification,
                // destination
    REFCLSID            rNotificationDest,
                // deliver mode - group and schedule data
    DELIVERMODE         deliverMode,
                // info about who the sender
    LPCLSID             pClsidSender,           // class of sender can be NULL
    LPNOTIFICATIONSINK  pReportNotfctnSink,     // can be null - see mode
                //the cookie of the new notification
    LPNOTIFICATIONREPORT *ppNotfctnReport,
                //the cookie of the new notification
    PNOTIFICATIONCOOKIE pNotificationCookie,
                // reserved dword
    PTASK_DATA          pTaskData
    )
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::AddNotification\n", this));
    HRESULT hr = E_NOTIMPL;
    SCHEDULEGROUPITEM scheduleGroupItem;
    CScheduleGroup *pSchedGrp = NULL;

    do
    {
        if (!pNotification)
        {
            // need notification, TaskTrigger and can not be delivered imemediately
            hr = E_INVALIDARG;
            break;
        }

        // validate task data
        if(FAILED(hr = ValidateTaskData(pTaskData)))
        {
            break;
        }

        // check if initialized
        if (!IsInitialized())
        {
            hr =  E_FAIL;
            break;
        }

        hr = CScheduleGroup::LoadPersistedGroup(c_pszRegKeyScheduleGroup, 
                                                &_GroupCookie, 0, &pSchedGrp);

        if (hr != NOERROR)
        {
            break;
        }

        scheduleGroupItem.cbSize = sizeof(SCHEDULEGROUPITEM);
        if (FAILED(pSchedGrp->GetScheduleGroupItem(&scheduleGroupItem, FALSE)) ||
            FAILED(SetScheduleGroupItem(&scheduleGroupItem, FALSE)))
        {
            hr = E_FAIL;
            break;
        }

        if (deliverMode & DM_THROTTLE_MODE)
        {
            deliverMode |= DM_NEED_COMPLETIONREPORT;
            // need a notification sink
            //
            if (!pReportNotfctnSink)
            {
                CNotificationMgr *pCNotfMgr = GetNotificationMgr();

                NotfAssert((pCNotfMgr));

                // used the default one
                pReportNotfctnSink = (INotificationSink *)pCNotfMgr;
                pReportNotfctnSink->AddRef();
            }
        }



        CPackage *pCPackage = 0;
        hr = CPackage::CreateDeliver(
                                pClsidSender,       // class of sender
                                pReportNotfctnSink, // can be null - see mode
                                pNotification,
                                deliverMode,
                                (PNOTIFICATIONCOOKIE)&_CGroupCookie,     //pGroupCookie,
                                &_TaskTrigger,      //pTaskTrigger,
                                (pTaskData) ? pTaskData : &_TaskData,
                                rNotificationDest,
                                &pCPackage,
                                PF_SCHEDULED
                                );

        if (hr != NOERROR)
        {
            break;
        }

        NotfAssert((pCPackage));

        if (pNotificationCookie)
        {
            *pNotificationCookie = pCPackage->GetNotificationCookie();
        }

        if (ppNotfctnReport)
        {
            *ppNotfctnReport = pCPackage->GetNotificationReportSender(TRUE);
            NotfAssert(( *ppNotfctnReport ));
        }

        if (hr == NOERROR)
        {
            hr = _SchedListAgent.HandlePackage(pCPackage);
        }

        GetGlobalNotfMgr()->SetLastUpdateDate(pCPackage);
        // release the package
        RELEASE(pCPackage);
        pCPackage = 0;

        NotfAssert((!pCPackage));
        if (deliverMode & DM_DELIVER_DEFAULT_PROCESS)
        {
            PostSyncDefProcNotifications();
        }

        break;
    } while ( TRUE );

    if (pSchedGrp)
    {
        pSchedGrp->Release();
    }

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::AddNotification (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::FindNotification
//
//  Synopsis:   finds a notification in a group
//
//  Arguments:  [pNotificatioCookie] --
//              [pNotificationItem] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-23-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::FindNotification(
    // the notification cookie
    PNOTIFICATIONCOOKIE pNotificatioCookie,
    // the new object just created
    PNOTIFICATIONITEM   pNotificationItem,
    DWORD               dwMode
    )
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::FindNotification\n", this));
    HRESULT hr = E_INVALIDARG;

    do
    {
        if (   !pNotificatioCookie
            || !pNotificationItem
            )
        {
            break;
        }

        CPackage *pCPackage = 0;
        hr = _SchedListAgent.FindPackage(
                                     pNotificatioCookie
                                     ,&pCPackage
                                     ,0
                                     );
        if (hr != NOERROR)
        {
            break;
        }
        NotfAssert((pCPackage));

        PNOTIFICATIONCOOKIE pGrpCookie =  pCPackage->GetGroupCookie();

        if (pGrpCookie)
        {
            hr = pCPackage->GetNotificationItem(pNotificationItem,(ENUM_FLAGS)dwMode);
        }
        else
        {
            hr = E_FAIL;
        }

        RELEASE(pCPackage);

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::FindNotification (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::RevokeNotification
//
//  Synopsis:   revokes a particular item from a group
//
//  Arguments:  [pNotificationCookie] --
//              [pNotificationItem] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-23-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::RevokeNotification(
    PNOTIFICATIONCOOKIE      pNotificationCookie,
    PNOTIFICATIONITEM        pNotificationItem,
    DWORD                    dwMode
    )
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::RevokeNotification\n", this));
    HRESULT hr = E_INVALIDARG;

    CPackage *pFoundPackage = NULL, 
             *pSLARevokedPackage = NULL,
             *pPkgTmp = NULL;

    do
    {
        if (!pNotificationCookie)
        {
            break;
        }

        CPackage *pCPackage = 0;
        hr = _SchedListAgent.FindPackage(
                                     pNotificationCookie
                                     ,&pFoundPackage
                                     ,0
                                     );
        BREAK_ONERROR(hr);

        if (pNotificationItem)
        {
            hr = _SchedListAgent.RevokePackage(pNotificationCookie, 
                                               &pSLARevokedPackage, 
                                               dwMode);
            if (hr == NOERROR)
            {
                NotfAssert((pSLARevokedPackage));
                hr = pSLARevokedPackage->GetNotificationItem(pNotificationItem,
                                                             (ENUM_FLAGS)dwMode);
                RELEASE(pSLARevokedPackage);
            }
        }
        else
        {
            hr = _SchedListAgent.RevokePackage(pNotificationCookie,NULL,dwMode);
        }

        RELEASE(pFoundPackage);

        HRESULT hr1 = GetThrottleListAgent()->FindPackage(pNotificationCookie, &pPkgTmp, 0);
        if (SUCCEEDED(hr1) && pPkgTmp)
        {
            DWORD dwState = pPkgTmp->GetNotificationState();
            pPkgTmp->SetNotificationState(dwState | PF_REVOKED);
            RELEASE(pPkgTmp);
        }

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::RevokeNotification (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::GetEnumNotification
//
//  Synopsis:   provides an enumerator over the items in this group
//
//  Arguments:  [grfFlags] --
//              [ppEnumNotification] --
//
//  Returns:
//
//  History:    1-23-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::GetEnumNotification(
    // flags which items to enumerate
    DWORD                    grfFlags,
    LPENUMNOTIFICATION      *ppEnumNotification
    )
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::GetEnumNotification\n", this));

    HRESULT hr = E_FAIL;

    do
    {
        if (!ppEnumNotification)
        {
            hr = E_INVALIDARG;
            break;
        }

        *ppEnumNotification = 0;

        CEnumNotification *pCEnum = 0;
        hr = CEnumNotification::Create(
                                   &_SchedListAgent
                                  ,(ENUM_FLAGS)grfFlags
                                  ,&pCEnum
                                  ,EG_GROUPITEMS
                                  ,&_GroupCookie
                                  );

        if (hr != NOERROR)
        {
            break;
        }
        *ppEnumNotification = (IEnumNotification *)pCEnum;

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::GetEnumNotification (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::InitAttributes
//
//  Synopsis:
//
//  Arguments:  [pTaskTrigger] --
//              [pTaskData] --
//              [pGroupCookie] --
//              [grfGroupMode] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::InitAttributes(PTASK_TRIGGER       pTaskTrigger,
                                       PTASK_DATA          pTaskData,
                                       PNOTIFICATIONCOOKIE pGroupCookie,
                                       PGROUPINFO          pGroupInfo,
                                       GROUPMODE           grfGroupMode)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::InitAttributes\n", this));
    NotfAssert((pTaskTrigger));
    HRESULT hr = NOERROR;

    do
    {
        if (   !pTaskTrigger
            || (grfGroupMode & GM_GROUP_SEQUENTIAL)
           )
        {
            // no sequential mode yet
            hr = E_INVALIDARG;
            break;
        }

        if (pTaskData)
        {
            _TaskData   = *pTaskData;
        }

        _TaskTrigger    = *pTaskTrigger;
        _grfGroupMode   = grfGroupMode;
        _grpState = GS_Initialized;

        if (pGroupInfo)
        {
            if (!IsStaticGroup())
            {
                delete [] _GroupInfo.pwzGroupname;
                _GroupInfo.pwzGroupname = (pGroupInfo->pwzGroupname) ? OLESTRDuplicate(pGroupInfo->pwzGroupname) : 0;
            }
        }

        // persit the new schedule group
        hr = SaveToPersist(c_pszRegKeyScheduleGroup);

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::InitAttributes (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::ChangeAttributes
//
//  Synopsis:
//
//  Arguments:  [pTaskTrigger] --
//              [pTaskData] --
//              [pGroupCookie] --
//              [grfGroupMode] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::ChangeAttributes(PTASK_TRIGGER       pTaskTrigger,
                                         PTASK_DATA          pTaskData,
                                         PNOTIFICATIONCOOKIE pGroupCookie,
                                         PGROUPINFO          pGroupInfo,
                                         GROUPMODE           grfGroupMode)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::ChangeAttributes\n", this));
    HRESULT hr = NOERROR;
    NOTIFICATIONITEM *pnotfItem = 0;

    do
    {
        if ( grfGroupMode & GM_GROUP_SEQUENTIAL)
        {
            // not sequential mode yet
            hr = E_INVALIDARG;
            break;
        }

        ULONG cElements = 0;
        hr = _SchedListAgent.GetPackageCount(&cElements);

        if (cElements == 0)
        {
            if (pTaskTrigger)
            {
                hr = InitAttributes(pTaskTrigger, pTaskData, pGroupCookie, pGroupInfo, grfGroupMode);
            }

            // nothing to do - no items
            break;
        }

        CEnumNotification *pCEnum = 0;
        hr = CEnumNotification::Create(
                                   &_SchedListAgent
                                  ,EF_NOT_NOTIFICATION
                                  ,&pCEnum
                                  ,EG_GROUPITEMS
                                  ,&_GroupCookie
                                  );

        BREAK_ONERROR(hr);

        pnotfItem = new NOTIFICATIONITEM [cElements];

        if (!pnotfItem)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        ULONG cFetched = 0;

        hr = pCEnum->Reset();
        hr = pCEnum->Next(cElements, pnotfItem, &cFetched);
        pCEnum->Release();

        if (FAILED(hr))
        {
            break;
        }
        hr = NOERROR;

        NotfAssert((cElements >= cFetched));

        if (pTaskTrigger)
        {
            _TaskTrigger    = *pTaskTrigger;
        }
        if (pTaskData)
        {
            _TaskData  = *pTaskData;
        }
        _grfGroupMode    = grfGroupMode;


        if (!IsStaticGroup())
        {

            delete [] _GroupInfo.pwzGroupname;
            _GroupInfo.pwzGroupname = 0;
            
            if (pGroupInfo && pGroupInfo->pwzGroupname)
            {
                _GroupInfo.pwzGroupname = OLESTRDuplicate(pGroupInfo->pwzGroupname);
            }
        }

        for (ULONG i = 0; i < cFetched; i++)
        {

            CPackage *pCPackage = 0;
            hr = _SchedListAgent.FindPackage(
                                  &(pnotfItem+i)->NotificationCookie
                                 ,&pCPackage
                                 ,LM_LOCALCOPY
                                 );
            if (FAILED(hr) || (!(pCPackage->GetDeliverMode() & DM_DELIVER_DEFAULT_PROCESS)))
            {
                if (pCPackage)
                {
                    RELEASE(pCPackage);
                }
                hr = _SchedListAgent.RevokePackage(
                                      &(pnotfItem+i)->NotificationCookie
                                     ,&pCPackage
                                     ,0
                                     );
            }

            if (hr == NOERROR)
            {
                if (pTaskTrigger)
                {
                    pCPackage->SetTaskTrigger(&_TaskTrigger, NULL);
                    hr = _SchedListAgent.HandlePackage(pCPackage);
                    GetGlobalNotfMgr()->SetLastUpdateDate(pCPackage);
                }
                RELEASE(pCPackage);
            }
        }

        BREAK_ONERROR(hr);
        
        // persit the new schedule group
        hr = SaveToPersist(c_pszRegKeyScheduleGroup);

        break;
    } while (TRUE);


    if (pnotfItem)
    {
        delete pnotfItem;
    }

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::ChangeAttributes (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::RetrieveAttributes
//
//  Synopsis:
//
//  Arguments:  [pTaskTrigger] --
//              [pTaskData] --
//              [pGroupCookie] --
//              [grfGroupMode] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::RetrieveAttributes(
        PTASK_TRIGGER       pTaskTrigger,
        PTASK_DATA          pTaskData,
        PNOTIFICATIONCOOKIE pGroupCookie,
        PGROUPINFO          pGroupInfo,
        GROUPMODE          *pgrfGroupMode,
        LONG               *pElements
        )
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::RetrieveAttributes\n", this));
    HRESULT hr = NOERROR;
    

    if (pTaskTrigger)
    {
        *pTaskTrigger = _TaskTrigger;
    }

    if (pTaskData)
    {
        *pTaskData  = _TaskData;
    }

    if (pGroupCookie)
    {
        *pGroupCookie     = _GroupCookie;
    }
    if (pgrfGroupMode)
    {
        *pgrfGroupMode    = _grfGroupMode;
    }
    if (pGroupInfo)
    {
        *pGroupInfo = _GroupInfo;
        pGroupInfo->pwzGroupname = (_GroupInfo.pwzGroupname) ? OLESTRDuplicate(_GroupInfo.pwzGroupname) : 0;
    }

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::RetrieveAttributes (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::RemovePersist
//
//  Synopsis:   remove a persistance package form the registry
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::RemovePersist(LPCSTR pszWhere,
                                      LPSTR pszSubKeyIn,
                                      DWORD dwMode)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::RemovePersist\n", NULL));
    HRESULT hr = NOERROR;
    NotfAssert((pszWhere));
    // remove the peristed package from the stream

    LPSTR       pszRegKey = 0;
    LPSTR       pszSubKey = 0;

    do
    {
        if (!pszWhere)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (pszSubKeyIn == 0)
        {
            pszSubKey = StringAFromCLSID( _CGroupCookie );
        }
        else
        {
            pszSubKey = pszSubKeyIn;
        }

        if (!pszSubKey)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        {
            long    lRes;
            DWORD   dwDisposition, dwType, dwSize;
            HKEY    hKey;
            char szKeyToDelete[1024];

            strcpy(szKeyToDelete, pszWhere);

            lRes = RegCreateKeyEx(HKEY_CURRENT_USER,szKeyToDelete,0,NULL,0,HKEY_READ_WRITE_ACCESS,
                            NULL,&hKey,&dwDisposition);

            if(lRes == ERROR_SUCCESS)
            {
                strcpy(szKeyToDelete, pszSubKey);
                lRes = RegDeleteValue(hKey, szKeyToDelete);
            }

            if (lRes != ERROR_SUCCESS)
            {
                hr = E_FAIL;
            }
            
            if (hKey)
            {
                RegCloseKey(hKey);
            }

        }

        if (pszRegKey)
        {
            delete pszRegKey;
        }
        if (pszSubKey && !pszSubKeyIn)
        {
            delete pszSubKey;
        }

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::RemovePersist (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::SaveToPersist
//
//  Synopsis:   saves a peristance package to the registry
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::SaveToPersist(LPCSTR pszWhere,
                                      LPSTR pszSubKeyIn,
                                      DWORD dwMode)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::SaveToPersist\n", NULL));
    HRESULT hr = E_INVALIDARG;
    NotfAssert((pszWhere));
    // save the package

    CRegStream *pRegStm = 0;
    LPSTR       pszRegKey = 0;
    LPSTR       pszSubKey = 0;

    do
    {
        // BUBUG: need to save the clsid of the current process
        //CLSID clsid = CLSID_StdNotificationMgr;
        //pszRegKey = StringAFromCLSID( &clsid );
        if (pszSubKeyIn == 0)
        {
            pszSubKey = StringAFromCLSID( _CGroupCookie );
        }
        else
        {
            pszSubKey = pszSubKeyIn;
        }

        if (pszSubKey)
        {
            pRegStm = new CRegStream(HKEY_CURRENT_USER, pszWhere,pszSubKey, TRUE);
        }

        if (pszRegKey)
        {
            delete pszRegKey;
        }
        if (pszSubKey && !pszSubKeyIn)
        {
            delete pszSubKey;
        }

        if (!pRegStm)
        {
           hr = E_OUTOFMEMORY;
           break;
        }

        IStream *pStm = 0;
        hr = pRegStm->GetStream(&pStm);
        if (hr != NOERROR)
        {

            delete pRegStm;
            break;
        }

        // save the item and the notification
        hr = Save(pStm, TRUE);
        BREAK_ONERROR(hr);

        if (pStm)
        {
            pStm->Release();
        }

        if (pRegStm)
        {
            pRegStm->SetDirty();
            delete pRegStm;
        }

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::SaveToPersist (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::LoadFromPersist
//
//  Synopsis:   loads a persistence package form the registry
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::LoadFromPersist(LPCSTR pszWhere,
                                        LPSTR pszSubKey,
                                        DWORD dwMode,
                                        CScheduleGroup **ppCScheduleGroup)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::LoadFromPersist\n", NULL));
    HRESULT hr = E_INVALIDARG;
    NotfAssert((pszWhere));
    // save the package

    CRegStream *pRegStm = 0;
    CScheduleGroup *pCPkg = 0;

    do
    {
        if (   !pszWhere
            || !pszSubKey
            || !ppCScheduleGroup )

        {
            break;
        }

        hr = RegIsPersistedValue(HKEY_CURRENT_USER, pszWhere, pszSubKey);
        BREAK_ONERROR(hr);

        pCPkg = new CScheduleGroup();

        if (!pCPkg)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        pRegStm = new CRegStream(HKEY_CURRENT_USER, pszWhere, pszSubKey, FALSE);

        if (!pRegStm)
        {
           hr = E_OUTOFMEMORY;
           break;
        }

        IStream *pStm = 0;
        hr = pRegStm->GetStream(&pStm);
        if (hr != NOERROR)
        {
            delete pRegStm;
            break;
        }

        // save the item and the notification
        hr = pCPkg->Load(pStm);
        BREAK_ONERROR(hr);

        if (pStm)
        {
            pStm->Release();
        }

        if (pRegStm)
        {
            delete pRegStm;
        }

        *ppCScheduleGroup = pCPkg;

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::LoadFromPersist (hr:%lx)\n",pCPkg, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::LoadPersistedGroup
//
//  Synopsis:   loads a persisted group
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleGroup::LoadPersistedGroup(
                       LPCSTR               pszWhere,
                       PNOTIFICATIONCOOKIE  pNotfCookie,
                       DWORD                dwMode,
                       CScheduleGroup **ppCScheduleGroup)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::LoadPersistedGroup\n", NULL));
    HRESULT hr = E_FAIL;
    LPSTR szPackageSubKey = 0;

    if (pNotfCookie)
    {
        szPackageSubKey = StringAFromCLSID( pNotfCookie);
    }
    if (szPackageSubKey)
    {
        hr = LoadFromPersist(pszWhere,szPackageSubKey,
                             dwMode,ppCScheduleGroup);

        delete [] szPackageSubKey;                             
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::LoadPersistedGroup (hr:%lx)\n", NULL, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::GetClassID
//
//  Synopsis:
//
//  Arguments:  [pClassID] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::GetClassID (CLSID *pClassID)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::GetClassID\n", this));
    HRESULT hr = NOERROR;

    *pClassID = CLSID_StdNotificationMgr;

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::GetClassID (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::IsDirty
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::IsDirty(void)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::\n", this));
    HRESULT hr = NOERROR;

    hr = (_fDirty) ? S_OK : S_FALSE;

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup:: (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::Load
//
//  Synopsis:
//
//  Arguments:  [pStm] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::Load(IStream *pStm)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::Load\n", this));
    HRESULT hr = NOERROR;
    NotfAssert((pStm));

    SCHEDULEGROUPITEM scheduleGroupItem;
    DWORD cbSaved;
    CLSID clsid = CLSID_NULL;

    do
    {
        scheduleGroupItem.cbSize = sizeof(SCHEDULEGROUPITEM);
        // read the notification item
        hr =  pStm->Read(&scheduleGroupItem, sizeof(SCHEDULEGROUPITEM), &cbSaved);
        BREAK_ONERROR(hr);
        NotfAssert(( (sizeof(SCHEDULEGROUPITEM) == cbSaved) || (sizeof(SCHEDULEGROUPITEM) - sizeof(GROUPINFO) == cbSaved) ));

        DWORD cblen = 0;
        // old groupitem did not have the group info
        if  (sizeof(SCHEDULEGROUPITEM) == cbSaved)
        {
            hr =  pStm->Read(&cblen, sizeof(DWORD), &cbSaved);
            NotfAssert((sizeof(DWORD) == cbSaved));
        }

        if (cblen)
        {
            scheduleGroupItem.GroupInfo.pwzGroupname = new WCHAR [cblen];
            if (scheduleGroupItem.GroupInfo.pwzGroupname)
            {
                hr =  pStm->Read((void *)scheduleGroupItem.GroupInfo.pwzGroupname, cblen * sizeof(WCHAR), &cbSaved);
                NotfAssert(( (cblen * sizeof(WCHAR)) == cbSaved));
            }
        }

        hr = SetScheduleGroupItem(&scheduleGroupItem, TRUE);

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::Load (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::Save
//
//  Synopsis:
//
//  Arguments:  [BOOL] --
//              [fClearDirty] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::Save(IStream *pStm,BOOL fClearDirty)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::Save\n", this));
    NotfAssert((pStm));
    HRESULT hr = NOERROR;

    SCHEDULEGROUPITEM       scheduleGroupItem;
    DWORD cbSaved;

    do
    {
        scheduleGroupItem.cbSize = sizeof(SCHEDULEGROUPITEM);
        // get and write the notification item
        hr = GetScheduleGroupItem(&scheduleGroupItem, TRUE);
        BREAK_ONERROR(hr);
        hr =  pStm->Write(&scheduleGroupItem, sizeof(SCHEDULEGROUPITEM), &cbSaved);
        BREAK_ONERROR(hr);
        NotfAssert(( sizeof(SCHEDULEGROUPITEM) == cbSaved));

        DWORD cblen = (scheduleGroupItem.GroupInfo.pwzGroupname) ? wcslen(scheduleGroupItem.GroupInfo.pwzGroupname) + 1 : 0;
        hr =  pStm->Write(&cblen, sizeof(DWORD), &cbSaved);
        NotfAssert((sizeof(DWORD) == cbSaved));

        if (cblen)
        {
            cblen *= sizeof(WCHAR);
            hr =  pStm->Write(scheduleGroupItem.GroupInfo.pwzGroupname, cblen, &cbSaved);
            NotfAssert((cblen == cbSaved));
        }
        
        _fDirty = FALSE;

        break;
    } while (TRUE);

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::Save (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleGroup::GetSizeMax
//
//  Synopsis:
//
//  Arguments:  [pcbSize] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CScheduleGroup::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::GetSizeMax\n", this));
    HRESULT hr = NOERROR;

    IPersistStream *pPrstStm = 0;

    pcbSize->LowPart += sizeof(SCHEDULEGROUPITEM);
    pcbSize->HighPart = 0;

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CScheduleGroup::GetSizeMax (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CScheduleGroup::UnPersistScheduleGroups
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
HRESULT CScheduleGroup::UnPersistScheduleGroups(
                                                LPCSTR pszWhere,
                                                LPSTR pszSubKeyIn,
                                                DWORD dwMode,
                                                CScheduleGroup *prgCSchdGrp[],
                                                ULONG cElements,
                                                ULONG *pcElementFilled
                                                )
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CScheduleGroup::UnPersistScheduleItem\n",NULL));
    HRESULT hr = E_INVALIDARG;
    CScheduleGroup *pCSchGrp = 0;
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

                hr = CScheduleGroup::LoadFromPersist(pszWhere,szValue, 0, &pCSchGrp);
                
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

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT UnPersistScheduleItem(hr:%lx)\n", NULL, hr));
    return hr;
}

