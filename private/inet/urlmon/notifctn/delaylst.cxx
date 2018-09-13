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

CDelayListAgent::~CDelayListAgent()
{
#if DBG == 1
    if (_XSortList.GetCount())
    {
        TNotfDebugOut((DEB_TMEMORY, "CDelayListAgent::~CDelayListAgent - the following CPackages have leaked:\n"));
        POSITION pos = _XSortList.GetHeadPosition();

        while (pos)
        {
            CPackage *pPackage = _XSortList.GetNext(pos);
            TNotfAssert((pPackage));
            PPKG_DUMP(pPackage, (DEB_TMEMORY, "~CDelayListAgent"));
        }
    }
#endif
}

//+---------------------------------------------------------------------------
//
//  Method:     CDelayListAgent::RevokePackage
//
//  Synopsis:   revoked a notification from the agent
//
//  Arguments:  [ppackageCookie] --
//              [dwMode] --
//
//  Returns:    E_FAIL always
//
//  History:    1-02-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDelayListAgent::RevokePackage(
                                  PNOTIFICATIONCOOKIE packageCookie,
                                  CPackage          **ppCPackage,
                                  DWORD               dwMode
                                  )
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CDelayListAgent::RevokePackage\n", this));

    // notification can not be revoked from this list
    HRESULT hr = E_FAIL;

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CDelayListAgent::RevokePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDelayListAgent::HandlePackage
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
HRESULT CDelayListAgent::HandlePackage(CPackage    *pCPackage)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CDelayListAgent::HandlePackage\n", this));
    HRESULT hr = E_NOTIMPL;
    CLock lck(_mxs);

    NotfAssert((pCPackage));
    CDelayListAgent *pDelAgent = 0;
    PTASK_TRIGGER pTaskTrigger = pCPackage->GetTaskTrigger();

    /*
    // add this time it is not know where the package will go
    // there might be not threadId
    NotfAssert((   (!pCPackage->GetThreadId() && pCPackage->IsBroadcast())
                 || (pCPackage->GetThreadId() && !pCPackage->IsBroadcast()) ));
    */

    // add it to the notification list
    hr = _XSortList.AddVal(_cElements, pCPackage);
    if (hr == NOERROR)
    {
        TNotfDebugOut((DEB_TFLOW, "%p Added to DelayList CDelayListAgent::HandlePackage\n", pCPackage));

        NotfDebugOut((DEB_SCHEDLST, "%p >>> CDelayListAgent::HandlePackage  Added:[%ld, pCPackage:%p]\n",this, _cElements,pCPackage));
        DumpIID(pCPackage->GetNotificationCookie());

        ADDREF(pCPackage);
        _cElements++;
        hr = SetWakeup();
    }

    NotfAssert(( _cElements == _XSortList.GetCount()));

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CDelayListAgent::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDelayListAgent::SetWakeup
//
//  Synopsis:   sets the wakeup time
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
HRESULT CDelayListAgent::SetWakeup()
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CDelayListAgent::SetWakeup\n", this));
    HRESULT hr = NOERROR;
    CLock lck(_mxs);

    if (!_Timer)
    {
        _Timer = GetGlobalNotfMgr()->SetScheduleTimer(NOTF_DELAY_TIMER,
                                            SCH_DELAY_INTERVAL, 
                                            CDelayListAgent::WakeupProc);

        if (!_Timer)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CDelayListAgent::SetWakeup (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDelayListAgent::WakeupProc
//
//  Synopsis:   called on wakeup to deliver notification from list
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
VOID CALLBACK CDelayListAgent::WakeupProc(HWND hWnd, UINT msg, UINT iWhat, DWORD dwTime)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CDelayListAgent::WakeupProc\n", NULL));
    HRESULT hr = NO_ERROR;

    NotfAssert((msg == WM_TIMER));

    CScheduleAgent *pthis = GetScheduleAgent();
    NotfAssert((pthis));

    hr = pthis->OnWakeup(WT_DELAY);

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CDelayListAgent::WakeupProc (hr:%lx)\n",pthis, hr));
}

//+---------------------------------------------------------------------------
//
//  Method:     CDelayListAgent::OnWakeup
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
HRESULT CDelayListAgent::OnWakeup(WAKEUPTYPE wt, CPackage *pCPackageWakeup)
{
    NotfDebugOut((DEB_SCHEDLST, "%p _IN CDelayListAgent::OnWakeup\n", NULL));
    HRESULT hr = NO_ERROR;
    CLock lck(_mxs);

    NotfAssert(( _XSortList.IsEmpty() == FALSE));
    int cEl = _XSortList.GetCount();
    while (cEl)
    {
        CPackage *pCPackage = _XSortList.RemoveHead();

        NotfDebugOut((DEB_SCHEDLST, "%p >>> CDelayListAgent::OnWakeup Remmoved:[%ld, pCPackage:%p]\n",this, _cElements,pCPackage));
        DumpIID(pCPackage->GetNotificationCookie());

        // Note: the CPackage is addref'd
        // don't release here
        _cElements--;
        cEl--;
        NotfAssert(( _cElements == cEl ));

        hr = _pDeliverAgent->HandlePackage(pCPackage);
        RELEASE(pCPackage);
    }

    NotfAssert(( _XSortList.IsEmpty() ));
    NotfAssert(( _cElements == 0 ));
    {
        CLock lck(_mxs);

        if (_Timer)
        {
            KillTimer(NULL, _Timer);
            _Timer = 0;
        }
    }

    NotfDebugOut((DEB_SCHEDLST, "%p OUT CDelayListAgent::OnWakeup (hr:%lx)\n",this, hr));
    return hr;
}

