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

CSchedListAgent::~CSchedListAgent()
{
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::RevokePackage
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::RevokePackage(
                                  PNOTIFICATIONCOOKIE packageCookie,
                                  CPackage          **ppCPackage,
                                  DWORD               dwMode
                                  )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::RevokePackage\n", this));
    CLock lck(_mxs);

    NotfAssert((packageCookie && !dwMode));
    
    HRESULT hr = E_FAIL;

    hr = _XSortList.RevokePackage(
                              packageCookie,
                              ppCPackage,
                              dwMode);
    if (hr == NOERROR)
    {
        //
        // fix the wakeup time if waiting
        //
        if (_Timer)
        {
            SetWakeup();
        }
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::RevokePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::HandlePackage
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
HRESULT CSchedListAgent::HandlePackage(CPackage    *pCPackage)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::HandlePackage\n", this));

    HRESULT hr;
    CLock lck(_mxs);

    NotfAssert((pCPackage));
    NotfAssert((pCPackage->GetNotification()));

    PTASK_TRIGGER pTaskTrigger = pCPackage->GetTaskTrigger();

    NotfAssert((pTaskTrigger));
    NotfAssert(( pTaskTrigger->TriggerType != TASK_TIME_TRIGGER_NOW ));

    pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_WAITING);
    if (AreWeTheDefaultProcess() || (!(pCPackage->GetDeliverMode() & DM_DELIVER_DEFAULT_PROCESS)))
    {
        hr = pCPackage->CalcNextRunDate();
        
        if (SUCCEEDED(hr))
        {
            hr = _XSortList.HandlePackage(pCPackage);

            if (!(pTaskTrigger->rgFlags & TASK_TRIGGER_FLAG_DISABLED))
            {
                // set/reset the wakup time
                // setwakup return S_FALSE if there is not timer running
                if (SUCCEEDED(SetWakeup()))
                {
                    hr = S_OK;
                }
            }
        }
    }
    else
    {
        pCPackage->CalcNextRunDate();
        if (SUCCEEDED(g_pGlobalNotfMgr->AddScheduleItem(pCPackage)))
        {
            hr = S_OK;
        }
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::SetWakeup
//
//  Synopsis:   calculates and sets next wakeup date/time
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::SetWakeup()
{
    NotfDebugOut((DEB_SCHEDLST | DEB_TRACE, "%p _IN CSchedListAgent::SetWakeup\n", this));

    #define NOTF_WAKEUP_SOON    1000
    #define MAX_LONG            0x7fffffff

    HRESULT hr;
    CLock lck(_mxs);
    int iMilSec = 0;
    CPackage *pCPackage = NULL;

    //
    //  Find the package which is supposed to run the soonest.  We track
    //  the time of day which we expect the next timer to fire in 
    //  _dateNextRun.
    //
    //  If the package we find is supposed to fire sooner than _dateNextRun,
    //  then we need to adjust the timer, otherwise there's nothing to do.
    //
    //  If everyone is scheduled then we just set our _dateNextRun to it's
    //  initial state (MAX_FILETIME) so that on our next entry we will reset 
    //  the timer.
    //
    //  NOTE: The timer is killed right at the start of OnWakeup() so we start
    //  with a clean slate each time.
    //
    
    CFileTime curDate = GetCurrentDtTime();

    hr = _XSortList.FindFirstSchedulePackage(NULL, &pCPackage, 0);

    if (hr == NOERROR)
    {
        NotfAssert((pCPackage && pCPackage->GetTaskTrigger() ));
        PTASK_TRIGGER pTaskTrigger = pCPackage->GetTaskTrigger();

        CFileTime pkgDate = pCPackage->GetNextRunDate();

        //  If this package is not disabled and this package date is sooner
        //  then our next run date (when the timer is supposed fire) then we
        //  need to adjust the timer.
        if ((pTaskTrigger->TriggerType != TASK_TRIGGER_FLAG_DISABLED) &&
            (pkgDate < _dateNextRun))
        {
            //  This shouldn't happen often in real usage but it happens a
            //  lot in the debugger :-)
            if (pkgDate <= curDate)
            {
                iMilSec = NOTF_WAKEUP_SOON;
                _dateNextRun = curDate + (iMilSec * ONE_MSEC_IN_FILEITME);
            }
            else
            {               
                CFileTime delta = pkgDate - curDate;
                NotfAssert((delta > 0));    //  Things will go bad if this fails!

                //  Get delta into milliseconds with a 1 second fudge factor
                //  for rounding, it's ok to be 1 second late late but NEVER early!

                __int64 i64MilSec = (FileTimeToInt64(delta) / ONE_MSEC_IN_FILEITME) + 1000i64;

                if (i64MilSec > MAX_LONG)
                {
                    iMilSec = MAX_LONG;
                    _dateNextRun = curDate + (MAX_LONG * ONE_MSEC_IN_FILEITME);
                }
                else
                {
                    iMilSec = (int)i64MilSec;
                    _dateNextRun = pkgDate;
                }

            }

            if (_Timer)
            {
                KillTimer(NULL, _Timer);
            }

            NotfAssert((iMilSec > 0));      //  Should never happen...
            if (iMilSec <= 0)
                iMilSec = NOTF_WAKEUP_SOON; //  ...but if it does wakeup soon and pray

            _Timer = GetGlobalNotfMgr()->SetScheduleTimer(NOTF_SCHED_TIMER, 
                                                iMilSec, 
                                                CSchedListAgent::WakeupProc);

            TNotfDebugOut((DEB_TSCHEDULE, "CSchedListAgent::SetWakeup timer is %d\n", iMilSec));
            if (!_Timer)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            TNotfDebugOut((DEB_TSCHEDULE, "CSchedListAgent::SetWakeup: No new wakeup time\n"));
            NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::SetWakeup: No new wakeup time\n", this));
        }
        RELEASE(pCPackage);
    }
    else
    {
        _dateNextRun = MAX_FILETIME;
        TNotfDebugOut((DEB_TSCHEDULE, "CSchedListAgent::SetWakeup: No new timer since nobody's waiting\n"));

        // indicate no timer running
        hr = S_FALSE;
    }
    
    NotfDebugOut((DEB_SCHEDLST | DEB_TRACE, "%p OUT CSchedListAgent::SetWakeup (hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::WakeupProc
//
//  Synopsis:   the wakeup proc
//
//  Arguments:  [hWnd] --
//              [msg] --
//              [iWhat] --
//              [dwTime] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID CALLBACK CSchedListAgent::WakeupProc(HWND hWnd, UINT msg, UINT iWhat, DWORD dwTime)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::WakeupProc\n", NULL));
    HRESULT hr = E_FAIL;

    NotfAssert((msg == WM_TIMER));

    CScheduleAgent *pthis = GetScheduleAgent();
    NotfAssert((pthis));
    if (pthis)
    {
        hr = pthis->OnWakeup(WT_SCHED);
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::WakeupProc (hr:%lx)\n",pthis, hr));
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::OnWakeup
//
//  Synopsis:   called on wakeup - delivers all notification with date smaller
//              then current date/time
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
HRESULT CSchedListAgent::OnWakeup(WAKEUPTYPE wt, CPackage *pCPackageWakeup)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::OnWakeup\n", this));
    HRESULT hr = NOERROR;

    CPackage *pCPackage = NULL,
             *pLastPackage = NULL;
    BOOL fSetWakeup = FALSE;
    NOTIFICATIONCOOKIE PosCookie = COOKIE_NULL;

    // If it's needed it will get reset
    NotfAssert((_Timer));
    KillTimer(NULL, _Timer);
    _Timer = NULL;
    _dateNextRun = MAX_FILETIME;

    if (wt == WT_TIMECHANGE)
    {
        return HandleTimeChange();
    }

    LIST_DUMP(_XSortList, (DEB_TLISTS, "CSchedListAgent::OnWakeup"));

    do
    {
        //
        // the list is sorted get the first elements which is waiting
        //
        hr = _XSortList.FindFirstSchedulePackage(&PosCookie, &pCPackage, 0);
        if (hr != NOERROR)
        {
            break;
        }

        NotfAssert((pCPackage));

        //  A little paranoia...
        NotfAssert((pLastPackage != pCPackage));
        if (pLastPackage == pCPackage)
        {
            //  pCPackage gets Released after the loop
            hr = E_FAIL;
            break;
        }
        pLastPackage = pCPackage;

        CFileTime curdt = GetCurrentDtTime();
       
        if (curdt >=  pCPackage->GetNextRunDate())
        {
            NotfAssert((pCPackage));
            NotfAssert((pCPackage->GetNotification()));

            NotfAssert((   pCPackage->GetTaskTrigger()
                         && (pCPackage->GetTaskTrigger()->TriggerType != TASK_TRIGGER_FLAG_DISABLED) ));

            pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_DELIVERED);
            pCPackage->SetNotificationState(pCPackage->GetNotificationState() & ~PF_WAITING);

            NotfAssert((pCPackage->GetNotification()));
            
            // add the package back to the sched list if
            // this call might return an error
            if (pCPackage->GetTaskTrigger()->TriggerType != TASK_TIME_TRIGGER_ONCE)
            {
                HandlePackage(pCPackage);
            }
                         
            //  Don't let this package mess up with all this fragile status.
            if (pCPackage->IsDeliverIfConnectToInternet() &&
                (GetGlobalNotfMgr()->IsConnectedToInternet() != S_OK)
               )
            {
                pCPackage->SetNotificationState((pCPackage->GetNotificationState() | PF_WAITING) & ~(PF_RUNNING | PF_DELIVERED));
                //  No delivery.
            }
            else if ((pCPackage->GetDeliverMode() & DM_THROTTLE_MODE) ||
                (pCPackage->IsDeliverOnUserIdle()))
            {
                CThrottleListAgent *pThrottleListAgent = GetThrottleListAgent();
                CPackage *pExistingPkg;
                if (SUCCEEDED(pThrottleListAgent->FindPackage(&pCPackage->GetNotificationCookie(),
                                                               &pExistingPkg, 0))) 
                {
                    //  This guy is already running in some capacity so leave him be.
                    TNotfDebugOut((DEB_TFLOW, "%p CSchedListAgent::OnWakeup %p is already throttled\n", pCPackage, pExistingPkg));
                    pCPackage->SetNotificationState(pCPackage->GetNotificationState() & ~PF_DELIVERED);
                    RELEASE(pExistingPkg);
                }
                else
                {
                    pCPackage->SetSortDate(SO_IDLEDATE, GetUserIdleCounter());
                    NotfAssert((pThrottleListAgent));
                    if (pCPackage->IsDeliverOnUserIdle())
                    {
                        pCPackage->SetNotificationState(pCPackage->GetNotificationState() 
                                                        | PF_WAITING_USER_IDLE);
                    }
                    pThrottleListAgent->HandlePackage(pCPackage);
                    hr = pThrottleListAgent->OnWakeup(WT_NEWITEM, pCPackage);

                    TNotfDebugOut((DEB_TFLOW, "%p CSchedListAgent::OnWakeup send to ThrottleAgent\n", pCPackage));
                }
            }
            else
            {
                // check if this item is not disabled in the task data
                pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_RUNNING);
                hr = _pDeliverAgent->HandlePackage(pCPackage);

                TNotfDebugOut((DEB_TFLOW, "%p CSchedListAgent::OnWakeup send to DeliverAgent\n", pCPackage));
            }
        }
        else
        {
            NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::OnWakeup\n", this));
            //exit the loop
            break;
        }
        
        if (pCPackage)
        {
            RELEASE(pCPackage);
            pCPackage = NULL;
        }

    } while (TRUE);

    if (pCPackage)
    {
        RELEASE(pCPackage);
    }
    
    //    
    // fix the wakeup time
    //
    SetWakeup();

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::OnWakeup (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::FindFirstPackage
//
//  Synopsis:   finds the first package in the schedule list
//
//  Arguments:  [packageCookie] -- out parameter used as position pointer
//              [ppCPackage] -- addref object
//              [dwMode] -- ignored for now
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::FindFirstPackage(
                                 PNOTIFICATIONCOOKIE packageCookie,
                                 CPackage          **ppCPackage,
                                 DWORD               dwMode
                                 )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::FindFirstPackage\n", this));
    NotfAssert((packageCookie && ppCPackage));

    HRESULT hr = E_FAIL;

    hr = _XSortList.FindFirstPackage(
                          packageCookie,
                          ppCPackage,
                          dwMode
                          );

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::FindFirstPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::FindNextPackage
//
//  Synopsis:   finds the next package in the list
//
//
//  Arguments:  [packageCookie] -- used a position pointer
//              [ppCPackage] -- in/out parameter - used as position pointer
//              [dwMode] --
//
//  Returns:    NOERROR and the CPackage object
//              E_FAIL  and the packagecookie with CLSID_NULL
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:      THIS IS A BAD ALGORITHM AND NEEEDS TO BE OPTIMIZED!!!
//              DO NOT SEND EMAIL TO JOHANNP - I AM AWARE!
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::FindNextPackage(
                                PNOTIFICATIONCOOKIE packageCookie,
                                CPackage          **ppCPackage,
                                DWORD               dwMode
                                )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::FindNextPackage\n", this));
    NotfAssert((packageCookie && ppCPackage));

    HRESULT hr = E_FAIL;

    hr = _XSortList.FindNextPackage(
                          packageCookie,
                          ppCPackage,
                          dwMode
                          );

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::FindNextPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::GetPackageCount
//
//  Synopsis:   return # of elements in schedule list
//
//  Arguments:  [pCount] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::GetPackageCount(
                                        ULONG          *pCount
                                       )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::GetPackageCount\n", this));
    NotfAssert((pCount));
    HRESULT hr = NOERROR;
    if (pCount)
    {
        *pCount = _XSortList.GetCount();
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::GetPackageCount (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::FindPackage
//
//  Synopsis:   finds a specific item
//
//  Arguments:  [packageCookie] -- the cookie of the item
//              [ppCPackage] -- addref'd object
//              [dwMode] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::FindPackage(
                            PNOTIFICATIONCOOKIE packageCookie,
                            CPackage          **ppCPackage,
                            DWORD               dwMode
                            )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::FindPackage\n", this));
    NotfAssert((packageCookie && ppCPackage));

    HRESULT hr = E_FAIL;

    hr = _XSortList.FindPackage(
                          packageCookie,
                          ppCPackage,
                          dwMode
                          );
    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::FindPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::Load
//
//  Synopsis:
//
//  Arguments:  [packageCookie] --
//              [pStg] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSchedListAgent::Load(
                                   PNOTIFICATIONCOOKIE packageCookie,
                                   IStorage *pStg,
                                   DWORD     dwMode
                                   )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::\n", this));
    NotfAssert((pStg));

    HRESULT hr = E_NOTIMPL;

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent:: (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::Save
//
//  Synopsis:
//
//  Arguments:  [packageCookie] -- the key used to load store the schedule list
//              [pStg] -- the storage
//              [dwMode] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSchedListAgent::Save(
                                   PNOTIFICATIONCOOKIE packageCookie,
                                   IStorage *pStg,
                                   DWORD     dwMode
                                   )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::\n", this));
    NotfAssert((pStg));

    HRESULT hr = NOERROR;

    NotfAssert((FALSE));
    #ifdef _old_

    HRESULT hr1 = NOERROR;
    BOOL fClearDirty = TRUE;
    IStream *pStm = 0;

    POSITION pos = _XSortList.GetHeadPosition();

    while (pos)
    {
        CPackage *pCPackage = _XSortList.GetNext(pos);

        if (pCPackage)
        {
            IPersistStream *pPrstStm = 0;
            LPNOTIFICATION pNotification = pCPackage->GetNotification();
            NotfAssert((pNotification));
            hr1 = pNotification->QueryInterface(IID_IPersistStream,(void **)&pPrstStm);
            if (hr1 == NOERROR)
            {
                hr1 = pPrstStm->Save(pStm, fClearDirty);
                if (hr1 == NOERROR)
                {
                }
            }
        }
        else
        {
            NotfAssert((!pos));
        }
    }
    #endif // _old_

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent:: (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::GetSizeMax
//
//  Synopsis:   calculates the requested size to store the object
//
//  Arguments:  [pcbSize] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CSchedListAgent::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::\n", this));
    NotfAssert((pcbSize));
    HRESULT hr = NOERROR;

    NotfAssert((FALSE));
    #ifdef  _old_

    HRESULT hr1 = NOERROR;

    ULARGE_INTEGER cbSize = {0,0};
    ULONG uSizeTotal = 0;
    ULONG uCount = 0;

    POSITION pos = _XSortList.GetHeadPosition();

    while (pos)
    {
        CPackage *pCPackage = _XSortList.GetNext(pos);

        if (pCPackage)
        {
            uCount++;

            IPersistStream *pPrstStm = 0;
            LPNOTIFICATION pNotification = pCPackage->GetNotification();
            NotfAssert((pNotification));
            hr1 = pNotification->QueryInterface(IID_IPersistStream,(void **)&pPrstStm);
            if (hr1 == NOERROR)
            {
                hr1 = pPrstStm->GetSizeMax(&cbSize);
                if (hr1 == NOERROR)
                {
                    uSizeTotal += cbSize.LowPart;
                }
            }
        }
        else
        {
            NotfAssert((!pos));
        }
    }

    uSizeTotal += (uCount * sizeof(NOTIFICATIONITEM));
    pcbSize->LowPart = uSizeTotal;
    pcbSize->HighPart = 0;

    #endif // _old_

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent:: (hr:%lx)\n",this, hr));
    return hr;
}

//
// group package loopkup

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::FindFirstGroupPackage
//
//  Synopsis:   finds the first package in the schedule list
//
//  Arguments:  [groupCookie] -- out parameter used as position pointer
//              [ppCPackage] -- addref object
//              [dwMode] -- ignored for now
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::FindFirstGroupPackage(
                                 PNOTIFICATIONCOOKIE groupCookie,
                                 CPackage          **ppCPackage,
                                 DWORD               dwMode
                                 )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::FindFirstGroupPackage\n", this));
    NotfAssert((groupCookie && ppCPackage && !dwMode));

    HRESULT hr = E_FAIL;

    do
    {

        CPackage *pCPackage = 0;
        NOTIFICATIONCOOKIE PosCookie = COOKIE_NULL;

        hr = _XSortList.FindFirstPackage(
                                  &PosCookie,
                                  &pCPackage,
                                  dwMode
                                  );
        BREAK_ONERROR(hr);
        BOOL fFound = FALSE;

        while (!fFound && (hr == NOERROR))
        {
            NotfAssert((pCPackage));

            if (   pCPackage
                && pCPackage->GetGroupCookie()
               )
            {
                // ok - now we are at the position we suppost to be
                // get the next one and return it
                *ppCPackage = pCPackage;
                // return the current cookie as the position
                *groupCookie = *pCPackage->GetGroupCookie();
                hr =  NOERROR;
                fFound = TRUE;
            }
            else
            {
                RELEASE(pCPackage);
                pCPackage = 0;

                hr = _XSortList.FindNextPackage(
                                      &PosCookie,
                                      &pCPackage,
                                      dwMode
                                      );
            }
        }

        break;
    } while (TRUE);

    // set the position to null if nothing found
    if (hr != NOERROR)
    {
        *groupCookie = COOKIE_NULL;
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::FindFirstGroupPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::FindNextGroupPackage
//
//  Synopsis:   finds the next package in the list
//
//
//  Arguments:  [groupCookie] -- used a position pointer
//              [ppCPackage] -- in/out parameter - used as position pointer
//              [dwMode] --
//
//  Returns:    NOERROR and the CPackage object
//              E_FAIL  and the groupCookie with CLSID_NULL
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:      THIS IS A BAD ALGORITHM AND NEEEDS TO BE OPTIMIZED!!!
//              DO NOT SEND EMAIL TO JOHANNP - I AM AWARE!
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::FindNextGroupPackage(
                                PNOTIFICATIONCOOKIE groupCookie,
                                CPackage          **ppCPackage,
                                DWORD               dwMode
                                )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::FindNextGroupPackage\n", this));
    NotfAssert((groupCookie && ppCPackage && !dwMode));
    HRESULT hr = E_FAIL;

    do
    {

        CPackage *pCPackage = 0;
        NOTIFICATIONCOOKIE PosCookie = COOKIE_NULL;

        hr = _XSortList.FindFirstPackage(
                                  &PosCookie,
                                  &pCPackage,
                                  dwMode
                                  );
        BREAK_ONERROR(hr);
        BOOL fFound = FALSE;

        while (!fFound && (hr == NOERROR))
        {
            NotfAssert((pCPackage));

            if (   pCPackage
                && pCPackage->GetGroupCookie()
                && (*pCPackage->GetGroupCookie() == *groupCookie)
               )
            {
                // ok - now we are at the position we suppost to be
                // get the next one and return it
                *ppCPackage = pCPackage;
                // return the current cookie as the position
                *groupCookie = *pCPackage->GetGroupCookie();
                hr =  NOERROR;
                fFound = TRUE;
            }
            else
            {
                RELEASE(pCPackage);
                pCPackage = 0;

                hr = _XSortList.FindNextPackage(
                                      &PosCookie,
                                      &pCPackage,
                                      dwMode
                                      );
            }
        }

        break;
    } while (TRUE);

    // set the position to null if nothing found
    if (hr != NOERROR)
    {
        *groupCookie = COOKIE_NULL;
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::FindNextGroupPackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::FindPackage
//
//  Synopsis:   finds a specific item
//
//  Arguments:  [groupCookie] -- the cookie of the item
//              [ppCPackage] -- addref'd object
//              [dwMode] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::FindGroupPackage(
                            PNOTIFICATIONCOOKIE groupCookie,
                            CPackage          **ppCPackage,
                            DWORD               dwMode
                            )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CSchedListAgent::FindGroupPackage\n", this));
    NotfAssert((groupCookie && ppCPackage && !dwMode));

    HRESULT hr = E_FAIL;

    NotfAssert((FALSE));
    #ifdef  _old_

    POSITION pos = _XSortList.GetHeadPosition();
    while (pos)
    {
        CPackage *pCPackage = _XSortList.GetNext(pos);

        if (   pCPackage
            && pCPackage->GetGroupCookie()
            && *pCPackage->GetGroupCookie() == *groupCookie)
        {
            pos = 0;
            ADDREF(pCPackage);
            *ppCPackage = pCPackage;
            hr =  NOERROR;
        }
    }
    #else

    #endif // _old_


    NotfDebugOut((DEB_SCHEDLST, "%p OUT CSchedListAgent::FindGroupPackage (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSchedListAgent::HandleTimeChange
//
//  Synopsis:   finds a specific item
//
//  Arguments:  [dwMode] --
//
//  Returns:
//
//  History:    7-02-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CSchedListAgent::HandleTimeChange(DWORD dwMode)
{
    NotfDebugOut((DEB_SCHEDGRP, "%p _IN CSchedListAgent::HandelTimeChange\n", this));
    HRESULT hr = NOERROR;
    NOTIFICATIONITEM *pnotfItem = 0;

    do
    {
        ULONG cElements = _XSortList.GetCount();

        CEnumNotification *pCEnum = 0;
        hr = CEnumNotification::Create(
                                   &_XSortList
                                  ,EF_NOT_NOTIFICATION
                                  ,&pCEnum
                                  //,0 //EG_GROUPITEMS
                                  //,0 //&_GroupCookie
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

        if (FAILED(hr))
        {
            break;
        }
        hr = NOERROR;

        NotfAssert((cElements >= cFetched));

        for (ULONG i = 0; i < cFetched; i++)
        {

            CPackage *pCPackage = 0;
            
            hr = FindPackage(&(pnotfItem+i)->NotificationCookie, &pCPackage, LM_LOCALCOPY);
            if (hr == NOERROR)
            {
                if (pCPackage->GetTaskTrigger())
                {
                    hr = HandlePackage(pCPackage);
                }
                RELEASE(pCPackage);
            }
        }

        break;
    } while (TRUE);


    if (pnotfItem)
    {
        delete pnotfItem;
    }

    NotfDebugOut((DEB_SCHEDGRP, "%p OUT CSchedListAgent::HandelTimeChange (hr:%lx)\n",this, hr));
    return hr;
}

STDMETHODIMP CSchedListAgent::Resync()
{
    HRESULT hr = S_OK,
            hr1;

    CGlobalNotfMgr *pGlobalNofMgr =  GetGlobalNotfMgr();

    if (!pGlobalNofMgr)
    {
        return E_FAIL;
    }

    CLockSmMutex mutexLock(pGlobalNofMgr->_Smxs);
   
    //  Iterate over all packages in the Registry.  For the items which
    //  already existed we need to FindPackage(), update the properties, 
    //  and then HandlePackage().  For items which are new we need to 
    //  LoadFromPersist then HandlePackage().

    for (ULONG i = 0; i < g_urgScheduleListItemtMax; i ++)
    {
        if ((g_DateLastSync < g_rgSortListItem[i].lastUpdateDate) && 
            (g_rgSortListItem[i].notfCookie != COOKIE_NULL))
        {
            CPackage *pPersistedPkg = NULL;
            CPackage *pExistingPkg = NULL;
            CHAR  *pszValue= StringAFromCLSID(&g_rgSortListItem[i].notfCookie);

            if (pszValue)
            {
                if (SUCCEEDED(FindPackage(&g_rgSortListItem[i].notfCookie, 
                                                &pExistingPkg,
                                                LM_LOCALCOPY)))
                {
                    hr1 = CPackage::LoadFromPersist(c_pszRegKey, pszValue, 0, &pPersistedPkg);
                    if (SUCCEEDED(hr1))
                    {
                        pExistingPkg->SyncTo(pPersistedPkg);

                        if (pExistingPkg->GetTaskTrigger())
                        {
                            hr1 = HandlePackage(pExistingPkg);
                        }
                        pPersistedPkg->Release();
                    }
                    else
                    {
                        hr = hr1;
                    }
                    pExistingPkg->Release();
                }
                else
                {
                    hr1 = CPackage::LoadFromPersist(c_pszRegKey, pszValue, 0, &pPersistedPkg);
                    if (SUCCEEDED(hr1))
                    {
                        if (pPersistedPkg->GetTaskTrigger())
                        {
                            hr = HandlePackage(pPersistedPkg);
                        }
                        pPersistedPkg->Release();
                    }
                    else
                    {
                        hr = hr1;
                    }
                }
                delete pszValue;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }
    }

    g_DateLastSync = GetCurrentDtTime();
    return hr;
}

