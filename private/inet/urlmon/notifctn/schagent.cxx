//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       schagent.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-17-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>

#ifndef xTASK_EVENT_TRIGGER_AT_NET_IDLE
#define xTASK_EVENT_TRIGGER_AT_NET_IDLE   8
#endif

CScheduleAgent      *g_pScheduleAgent   = 0;
CSchedListAgent     *g_pCSchedListAgent = 0;
CDelayListAgent     *g_pCDelayListAgent = 0;
CThrottleListAgent      *g_pCIdleListAgent  = 0;

//+---------------------------------------------------------------------------
//
//  Function:   GetScheduleAgent
//
//  Synopsis:   creates a new schedule agent
//
//  Arguments:  [fAddRef] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CScheduleAgent * GetScheduleAgent(BOOL fAddRef)
{
    NotfDebugOut((DEB_SCHEDLST, "API _IN GetScheduleAgent\n"));
    CScheduleAgent *pDelAgent = NULL;

    if (!g_pCSchedListAgent)
    {
        g_pCSchedListAgent = new CSchedListAgent();
    }

    if (!g_pCDelayListAgent)
    {
        g_pCDelayListAgent = new CDelayListAgent();
    }

    NotfAssert(( GetGlobalNotfMgr() ));
    

    if (!g_pCIdleListAgent && GetGlobalNotfMgr() )
    {
        g_pCIdleListAgent = new CThrottleListAgent(*GetGlobalNotfMgr(), SO_IDLEDATE, PF_WAITING_USER_IDLE | PF_SUSPENDED | PF_RUNNING);
    }


    if (   g_pCSchedListAgent
        && g_pCDelayListAgent
        && g_pCIdleListAgent
        && (!g_pScheduleAgent))
    {
        g_pScheduleAgent = new CScheduleAgent();
        DllAddRef();
    }

    if (g_pScheduleAgent)
    {
        if (fAddRef)
        {
            g_pScheduleAgent->AddRef();
        }
        pDelAgent = g_pScheduleAgent;
    }

    NotfDebugOut((DEB_SCHEDLST, "API OUT GetScheduleAgent (pDelAgent:%p)\n", pDelAgent));
    return pDelAgent;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleAgent::RevokePackage
//
//  Synopsis:   revokes a notification from the scheduler agent
//
//  Arguments:  [ppackageCookie] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:      only scheduled notifications can be revoked!
//
//----------------------------------------------------------------------------
HRESULT CScheduleAgent::RevokePackage(
                                  PNOTIFICATIONCOOKIE packageCookie,
                                  CPackage          **ppCPackage,
                                  DWORD               dwMode
                                  )

{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CScheduleAgent::RevokePackage\n", this));
    HRESULT hr;
    CPackage  * pPkgTmp = NULL;

    if (ppCPackage)
        * ppCPackage = NULL;

    hr = _pSchedListAgent->RevokePackage(packageCookie, ppCPackage, dwMode);

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CScheduleAgent::RevokePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleAgent::HandlePackage
//
//  Synopsis:   finds out package (notification type) and passes it on
//              to the appropriate agent
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    12-16-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleAgent::HandlePackage(CPackage    *pCPackage)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CScheduleAgent::HandlePackage\n", this));
    HRESULT hr = E_FAIL;

    NotfAssert((pCPackage));
    CScheduleAgent *pDelAgent = 0;
    PTASK_TRIGGER pTaskTrigger = pCPackage->GetTaskTrigger();

    if (   pTaskTrigger
        && (pTaskTrigger->TriggerType == TASK_TIME_TRIGGER_NOW) )
    {
        hr = _pDelayListAgent->HandlePackage(pCPackage);
        TNotfDebugOut((DEB_TFLOW, "%p CScheduleAgent::HandlePackage send to DelayList\n", pCPackage));
    }
    else if (!pTaskTrigger)
    {
        if (pCPackage->IsDeliverOnUserIdle())
        {
            //add it to the idl list
            NotfAssert((_pIdleListAgent));
            
            pCPackage->SetSortDate(SO_IDLEDATE, GetUserIdleCounter());
            pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_WAITING_USER_IDLE);
            hr = _pIdleListAgent->HandlePackage(pCPackage);

            TNotfDebugOut((DEB_TFLOW, "%p CScheduleAgent::HandlePackage send to IdleList\n", pCPackage));
        }
        else if (pCPackage->GetDeliverMode() & DM_THROTTLE_MODE )
        {
            //add it to the idl list
            NotfAssert((_pIdleListAgent));
            CPackage *pExistingPkg = NULL;

            HRESULT hr1 = _pIdleListAgent->RevokePackage(&pCPackage->GetNotificationCookie(), 
                                                         &pExistingPkg, 0);
            if (SUCCEEDED(hr1))
            {
                if (pExistingPkg->GetNotificationState() & PF_RUNNING)
                {
                    hr1 = _pIdleListAgent->PrivateDeliverReport(NOTIFICATIONTYPE_TASKS_ABORT, 
                                                                pExistingPkg, 0);
                }
                pExistingPkg->SetNotificationState(
                        pExistingPkg->GetNotificationState() & ~(PF_RUNNING | PF_DELIVERED));
                pExistingPkg->Release();
            }

            pCPackage->SetSortDate(SO_IDLEDATE, GetUserIdleCounter());
            hr = _pIdleListAgent->HandlePackage(pCPackage);
            hr = _pIdleListAgent->OnWakeup(WT_NEWITEM, pCPackage);

            TNotfDebugOut((DEB_TFLOW, "%p CScheduleAgent::HandlePackage send to Throttle\n", pCPackage));
        }
        else
        {
            hr = _pDelayListAgent->HandlePackage(pCPackage);

            TNotfDebugOut((DEB_TFLOW, "%p CScheduleAgent::HandlePackage send to DelayList2\n", pCPackage));
        }

    }
    else
    {
        hr = _pSchedListAgent->HandlePackage(pCPackage);
        TNotfDebugOut((DEB_TFLOW, "%p CScheduleAgent::HandlePackage send to SchedList\n", pCPackage));
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CScheduleAgent::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CScheduleAgent::OnWakeup
//
//  Synopsis:   called on wakeup to process next scheduled notification
//
//  Arguments:  [wt] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CScheduleAgent::OnWakeup(WAKEUPTYPE wt, CPackage *pCPackageWakeup)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CScheduleAgent::OnWakeup\n", NULL));
    HRESULT hr = E_FAIL;

    switch(wt)
    {
    default:
    case WT_NONE :
        break;
    case WT_DELAY:
        hr = _pDelayListAgent->OnWakeup(wt);
        break;
    case WT_SCHED:
        hr = _pSchedListAgent->OnWakeup(wt);
        break;
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CScheduleAgent::OnWakeup (hr:%lx)\n",this, hr));
    return hr;
}
