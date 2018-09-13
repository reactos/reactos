//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       delaylst.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1-02-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>

CThrottleListAgent::~CThrottleListAgent()
{
    PLIST_DUMP(this, (DEB_TLISTS, "CThrottleListAgent::~CThrottleListAgent"));
}

//+---------------------------------------------------------------------------
//
//  Method:     CThrottleListAgent::OnWakeup
//
//  Synopsis:   called on wakeup; also sets the next wakeup time
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
HRESULT CThrottleListAgent::OnWakeup(WAKEUPTYPE wt, CPackage *pCPackageWakeup)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CThrottleListAgent::OnWakeup\n", NULL));
    HRESULT hr = NO_ERROR;
    CLock lck(_mxs);
    CPackage *pCPackage = NULL;
    CGlobalNotfMgr *pGlbNotfMgr = GetGlobalNotfMgr();
      
    if (wt == WT_NEWITEM)
    {
        HRESULT hr1;
        BOOL fOnUserIdle  = (BOOL) (pCPackageWakeup->GetNotificationState() & PF_WAITING_USER_IDLE);

        if ((  !fOnUserIdle) 
            || (fOnUserIdle && (GetGlobalState() & GL_STATE_USERIDLE))
           )
        {
            hr1 = pGlbNotfMgr->AddItemToRunningList(pCPackageWakeup, 0);
            if (hr1 == S_FALSE)
            {
                _cWaiting++;
            }
        }
        else
        {
            TNotfDebugOut((DEB_TFLOW, "%p Deferred for IDLE time in CThrottleListAgent::OnWakeup\n", pCPackageWakeup));
            hr1 = S_FALSE;
        }
                      
        if (hr1 == S_OK)
        {
            pCPackage = pCPackageWakeup;
            ADDREF(pCPackage);

            NotfAssert((pCPackage));
            NotfAssert((pCPackage->GetNotification() ));

            if (!pCPackage->IsReportRequired())
            {
                hr1 = RevokePackage(&pCPackage->GetNotificationCookie(), 0, 0);
            }
            
            // set the notification state to waiting
            pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_RUNNING);

            NotfAssert((pCPackage->GetNotification()));
            
            hr = _pDeliverAgent->HandlePackage(pCPackage);
            NotfAssert((pCPackage->GetNotification()));

            RELEASE(pCPackage);
            pCPackage = NULL;
        }
        else if (hr1 != S_FALSE)
        {
            hr = hr1;
        }
    }
    else if(wt == WT_NEXTITEM)
    {
        HRESULT hr1;
        NOTIFICATIONCOOKIE posCookie = COOKIE_NULL;
        //
        // the list is sorted get the first elements
        //
        hr1 = hr = FindFirstPackage(&posCookie, &pCPackage, EF_NOPERSISTCHECK);

        while (hr1 == NOERROR)
        {
            BOOL fRemoved = FALSE;
            hr1 = E_FAIL;
            
            BOOL fOnUserIdle  = 
                (BOOL) (pCPackage->GetNotificationState() & PF_WAITING_USER_IDLE);

            if (!fOnUserIdle || (GetGlobalState() & GL_STATE_USERIDLE))
            {
                if ( !(pCPackage->GetNotificationState() & PF_RUNNING) )
                {
                    hr1 = pGlbNotfMgr->AddItemToRunningList(pCPackage, 0);
                }
            
                if (hr1 == S_OK)
                {
                    NotfAssert((pCPackage));
                    NotfAssert((pCPackage->GetNotification() ));

                    if (!pCPackage->IsReportRequired())
                    {
                        hr1 = RevokePackage(&pCPackage->GetNotificationCookie(), 0, 0);
                        fRemoved = TRUE;
                    }
                
                    // set the notification state to running
                    pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_RUNNING);

                    NotfAssert((pCPackage->GetNotification()));

                    if (pCPackage->GetNotificationState() & PF_SUSPENDED)
                    {
                        pCPackage->SetNotificationState(pCPackage->GetNotificationState() & ~PF_SUSPENDED);
                        hr = PrivateDeliverReport(NOTIFICATIONTYPE_TASKS_RESUME, pCPackage, 0);
                    }
                    else
                    {
                        // check if this item is not disabled in the task data
                        hr = _pDeliverAgent->HandlePackage(pCPackage);
                        // ignore hresult for now
                    }
                    NotfAssert((pCPackage->GetNotification()));
                }
            }
            RELEASE(pCPackage);
            pCPackage = NULL;
            
            // find the next package
            if (fRemoved)
            {
                hr1 = FindFirstPackage(&posCookie, &pCPackage, EF_NOPERSISTCHECK);
            }
            else
            {
                hr1 = FindNextPackage(&posCookie, &pCPackage, EF_NOPERSISTCHECK);
            }
        }
    
    }
    else if(pCPackageWakeup->GetNotificationID() == NOTIFICATIONTYPE_USER_IDLE_BEGIN) 
    {
        HRESULT hr1;
        NOTIFICATIONCOOKIE posCookie = COOKIE_NULL;

        SetGlobalStateOn(GL_STATE_USERIDLE);
        //
        // the list is sorted get the first elements
        //
        hr1 = hr = FindFirstPackage(&posCookie, &pCPackage, 0);

        while (hr1 == NOERROR)
        {
            BOOL fRemoved = FALSE;

            if (   (pCPackage->GetNotificationState() & PF_WAITING_USER_IDLE)
                && ((hr1 = pGlbNotfMgr->AddItemToRunningList(pCPackage, 0)) == S_OK))
            {
                NotfAssert((pCPackage));
                NotfAssert((pCPackage->GetNotification() ));

                if (!pCPackage->IsReportRequired())
                {
                    hr1 = RevokePackage(&pCPackage->GetNotificationCookie(), 0, 0);
                    fRemoved = TRUE;
                }
                
                // set the notification state to running
                pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_RUNNING);

                NotfAssert((pCPackage->GetNotification()));

                if (pCPackage->GetNotificationState() & PF_SUSPENDED)
                {
                    pCPackage->SetNotificationState(pCPackage->GetNotificationState() & ~PF_SUSPENDED);
                    hr = PrivateDeliverReport(NOTIFICATIONTYPE_TASKS_RESUME, pCPackage, 0);
                }
                else
                {
                    // check if this item is not disabled in the task data
                    hr = _pDeliverAgent->HandlePackage(pCPackage);
                    // ignore hresult for now
                }
                NotfAssert((pCPackage->GetNotification()));
            }
            
            RELEASE(pCPackage);
            pCPackage = NULL;
            
            // find the next package
            if (fRemoved)
            {
                hr1 = FindFirstPackage(&posCookie, &pCPackage, 0);
            }
            else
            {
                hr1 = FindNextPackage(&posCookie, &pCPackage, 0);
            }
        }
    
    }
    else if (pCPackageWakeup->GetNotificationID() == NOTIFICATIONTYPE_USER_IDLE_END)
    {
        // throttle the stuff now
        
        NOTIFICATIONCOOKIE posCookie = COOKIE_NULL;

        SetGlobalStateOff(GL_STATE_USERIDLE);

        //
        // the list is sorted get the first elements
        //
        HRESULT hr1;
        hr1 = hr = FindFirstPackage(&posCookie, &pCPackage, 0);

        while (hr1 == NOERROR)
        {
            BOOL fRemoved = FALSE;
            NotfAssert((pCPackage));

            if (   (pCPackage->GetNotificationState() & PF_WAITING_USER_IDLE) 
                && (pCPackage->GetNotificationState() & PF_RUNNING) )
            {
                NotfAssert((pCPackage->GetNotificationState() & PF_RUNNING));

                //  Give others a chance to run
                hr = pGlbNotfMgr->RemoveItemFromRunningList(pCPackage, 0);
                hr = pGlbNotfMgr->AddItemToRunningList(pCPackage, TL_ADD_TO_WAIT);

                if (pCPackage->IsReportRequired())
                {
                    NotfAssert((pCPackage->GetNotification() ));
                    // set the notification state to waiting
                    pCPackage->SetNotificationState((pCPackage->GetNotificationState() | PF_SUSPENDED) 
                                                     & ~PF_RUNNING);
                    hr = PrivateDeliverReport(NOTIFICATIONTYPE_TASKS_SUSPEND, pCPackage, 0);
                }
            }

            RELEASE(pCPackage);
            pCPackage = NULL;

            // find the next package
            if (fRemoved)
            {
                hr1 = FindFirstPackage(&posCookie, &pCPackage, 0);
            }
            else
            {
                hr1 = FindNextPackage(&posCookie, &pCPackage, 0);
            }
        }
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CThrottleListAgent::OnWakeup (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThrottleListAgent::PrivateDeliverReport
//
//  Synopsis:
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
HRESULT CThrottleListAgent::PrivateDeliverReport(NOTIFICATIONTYPE notfType, CPackage *pCPackage, DWORD dwMode)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CThrottleListAgent::PrivateDeliverReport\n", NULL));
    HRESULT hr = E_FAIL;
    LPNOTIFICATION pNotification = 0;
    
    do 
    {
        if (!pCPackage)
        {
            break;
        }
        
        NOTIFICATIONCOOKIE NotfCookie = COOKIE_NULL;
        pCPackage->GetNotificationCookie(&NotfCookie);

        hr = GetNotificationMgr()->CreateNotification(
                                                notfType
                                                ,0
                                                ,0
                                                ,&pNotification
                                                ,0);
        BREAK_ONERROR(hr);
        
        hr = GetNotificationMgr()->DeliverReport(pNotification, &NotfCookie, 0);
        break;
    } while (TRUE);

    if (pNotification)
    {
        pNotification->Release();
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CThrottleListAgent::PrivateDeliverReport (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThrottleListAgent::HandlePackage
//
//  Synopsis:
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
HRESULT CThrottleListAgent::HandlePackage(CPackage *pCPackage)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CThrottleListAgent::HandlePackage\n", this));

    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage));
    NotfAssert((pCPackage->GetNotification()));
    
    CLock lck(_mxs);
    hr = CSortItemList::HandlePackage(pCPackage);
    
    NotfDebugOut((DEB_SCHEDLST, "%p OUT CThrottleListAgent::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThrottleListAgent::RevokePackage
//
//  Synopsis:   removes the object from the process list
//              and from the global table
//
//  Arguments:  [pCPackage] --
//              [dwMode] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

#define REVOKE_WAITING_ONLY 1

HRESULT CThrottleListAgent::RevokePackage(
                        PNOTIFICATIONCOOKIE packageCookie,
                        CPackage           **ppCPackage,
                        DWORD               dwMode
                        )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CThrottleListAgent::RevokePackage(%d)\n", this, dwMode));
    HRESULT hr = E_FAIL;
    CLock lck(_mxs);
    CGlobalNotfMgr *pGlbNotfMgr = GetGlobalNotfMgr();
    NotfAssert(( pGlbNotfMgr ));
    CPackage  *pCPackageRevoke = 0;
    PNOTIFICATIONTYPE pNotificationType = 0;
 
    hr = CSortItemList::RevokePackage(packageCookie, &pCPackageRevoke, 0);

    if (hr == NOERROR)
    {
        NotfAssert((pCPackageRevoke));
        pNotificationType = pCPackageRevoke->GetNotificationType();
        if (dwMode == REVOKE_WAITING_ONLY)
            hr = pGlbNotfMgr->RemoveItemFromWaitingList(pCPackageRevoke, 0 );
        else
            hr = pGlbNotfMgr->RemoveItemFromRunningList(pCPackageRevoke, (_cWaiting) ? GLB_NO_TRIGGER : 0 );
    }
    
    if (ppCPackage)
    {
        *ppCPackage = pCPackageRevoke;
    }
    else if (pCPackageRevoke)
    {
        RELEASE(pCPackageRevoke);
    }
    
    NotfDebugOut((DEB_SCHEDLST, "%p OUT CThrottleListAgent::RevokePackage (hr:%lx)\n",this, hr));
    return hr;
}

#if DBG == 1
void CThrottleListAgent::Dump(DWORD dwFlags, const char *pszPrefix, HRESULT hr)
{
    if (!(TNotfInfoLevel & dwFlags))
        return;     //  no point in wasting time here

    TNotfDebugOut((dwFlags, "%s 0x%08x\n", pszPrefix, hr));

    POSITION pos = _XSortList.GetHeadPosition();

    while (pos)
    {
        CPackage *pPackage = _XSortList.GetNext(pos);
        TNotfAssert((pPackage));
        PPKG_DUMP(pPackage, (dwFlags, "~CThrottleListAgent"));
    }
}

void DumpRunList(DWORD dwFlags, const char *pszPrefix, HRESULT hr)
{
}

void DumpWaitList(DWORD dwFlags, const char *pszPrefix, HRESULT hr)
{
}

#endif
