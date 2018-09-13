//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       NotfMgr.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>
//
//
// dll shared data
//
#pragma data_seg("SharedData")

ULONG       g_dwCurrentCookie = 0;
ULONG       g_dwCurrentPackage = 0;
ULONG       g_dwCurrentUserIdle = 0;
ULONG       g_dwGlobalState = 0;
CFileTime   g_dateLastSchedListChange = 0;
ULONG       g_cSchedListCount = 0;
ULONG       g_cThrottleItemCount = 0;
ULONG       g_cRefPorts = 0;
ULONG       g_cRefProcesses = 0;
ULONG       g_urgCDestPortMax = DESTINATIONPORT_MAX;
ULONG       g_urgScheduleListItemtMax = SCHEDULELISITEM_MAX;
ULONG       g_urgThrottleItemtMax = THROTTLEITEM_MAX;
DestinationPort  g_DefaultDestPort = {0, 0, 0};
DestinationPort  g_rgCDestPorts[DESTINATIONPORT_MAX] = {{0,0},{0,0},{0,0}};
SCHEDLISTITEMKEY g_rgSortListItem[SCHEDULELISITEM_MAX]=  {{0, 0, 0, 0}};
THROTTLE_ITEM    g_rgThrottleItem[THROTTLEITEM_MAX] = {{0,-1,0,0,0},{0,-1,0,0,0} };

#pragma data_seg()

CFileTime   g_DateLastSync;

//
// the ProcessPacket is called for global notifications
//
ULONG GetGlobalCounter()
{
    return GetGlobalNotfMgr()->GetPackageCount(TRUE);
}

ULONG GetUserIdleCounter()
{
    NotfAssert(( GetGlobalNotfMgr() ));
    return GetGlobalNotfMgr()->GetUserIdleCount(TRUE);
}

DWORD GetGlobalState()
{
    return GetGlobalNotfMgr()->GetGlobalState();
}

DWORD SetGlobalStateOn(DWORD dwFlags)
{
    return GetGlobalNotfMgr()->SetGlobalStateOn(dwFlags);
}
DWORD SetGlobalStateOff(DWORD dwFlags)
{
    return GetGlobalNotfMgr()->SetGlobalStateOff(dwFlags);
}


//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::CGlobalNotfMgr
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CGlobalNotfMgr::CGlobalNotfMgr() : CCourierAgent()
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::CGlobalNotfMgr\n", NULL));
    //
    // create the mutex and get the lock
    _Smxs.Init("NotificationMgrSharedData", TRUE);
    if (_Smxs.Created())
    {
        NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::CGlobalNotfMgr INIT TABLES!!\n", NULL));
        // ok, the first intance init the shared data
        memset(g_rgSortListItem, 0, sizeof(SCHEDLISTITEMKEY) * g_urgScheduleListItemtMax);
        memset(g_rgThrottleItem, 0, sizeof(THROTTLE_ITEM) * g_urgThrottleItemtMax);
        memset(g_rgCDestPorts, 0, sizeof(DestinationPort) * g_urgCDestPortMax);

        g_cRefPorts = 0;
        g_cRefProcesses = 0;
        g_cSchedListCount = 0;
        g_cThrottleItemCount = 0;
        g_dateLastSchedListChange = GetCurrentDtTime();
        SetGlobalStateOff(GL_STATE_USERIDLE);
        DeletRegSetting(c_pszRegKeyPackageDelete, NULL);
        DeletRegSetting(c_pszRegKeyRunningDestDelete, NULL);
    }

    _uProcessCount = ++g_cRefProcesses;
    // release the lock
    _Smxs.Release();
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::CGlobalNotfMgr (g_cRefProcesses:%lx)\n", this,g_cRefProcesses));
}


//
//  Helper which will use the default process window for timers or
//  fallback to the old behavior with a callback proc.
//
UINT CGlobalNotfMgr::SetScheduleTimer(UINT idTimer, UINT uTimeout, TIMERPROC tmprc)
{
    UINT timer;
    CDestinationPort cDefDest;
    DWORD dwDefProcessId;
    
    if (GetDefaultDestinationPort(&cDefDest) == S_OK && 
        GetWindowThreadProcessId(cDefDest.GetPort(), &dwDefProcessId) &&
        (dwDefProcessId == ::GetCurrentProcessId()))
    {
        timer = ::SetTimer(cDefDest.GetPort(), idTimer, uTimeout, NULL);
    }
    else
    {
        TNotfDebugOut((DEB_TSCHEDULE, "WARNING - no default process in SetScheduleTimer\n"));
        timer = ::SetTimer(NULL, idTimer, uTimeout, tmprc);
    }

    return timer;
}

HRESULT CGlobalNotfMgr::SetLastUpdateDate(CPackage *pCPackage)
{
    HRESULT hr = E_FAIL;
    if (pCPackage)
    {
        CLockSmMutex lck(_Smxs);

        for (ULONG i = 0; i < g_urgScheduleListItemtMax; i ++)
        {
            if (g_rgSortListItem[i].notfCookie == pCPackage->GetNotificationCookie())
            {
                SYSTEMTIME st;
                CFileTime ft;
                
                GetLocalTime(&st);
                SystemTimeToFileTime(&st, &ft);

                g_rgSortListItem[i].lastUpdateDate = FileTimeToInt64(ft);
                hr = S_OK;
                break;
            }
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::SetDefaultDestinationPort
//
//  Synopsis:
//
//  Arguments:  [rCDestPort] --
//              [pCDestPortPrev] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::SetDefaultDestinationPort(CDestinationPort &rCDestPort, CDestinationPort *pCDestPortPrev)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::SetDefaultDestinationPort\n", this));
    HRESULT hr = NOERROR;
    do
    {   // BEGIN LOCK BLOCK
        CLockSmMutex lck(_Smxs);
        if (pCDestPortPrev)
        {
            *pCDestPortPrev = g_DefaultDestPort;
        }
        g_DefaultDestPort = rCDestPort;

        break;
    } while (TRUE); // END LOCK BLOCK

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::SetDefaultDestinationPort (hr:%lx, hwnd:%lx)\n",this, hr, rCDestPort.GetPort()));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::GetDefaultDestinationPort
//
//  Synopsis:
//
//  Arguments:  [pCDestPort] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::GetDefaultDestinationPort(CDestinationPort *pCDestPort)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::GetDefaultDestinationPort\n", this));
    HRESULT hr = NOERROR;
    do
    {   // BEGIN LOCK BLOCK
        CLockSmMutex lck(_Smxs);

        if (!pCDestPort)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (!IsWindow(g_DefaultDestPort._hWndPort))
        {
            // not default prot registered
            hr = S_FALSE;
        }
        else
        {
            *pCDestPort = g_DefaultDestPort;
        }

        break;
    } while (TRUE); // END LOCK BLOCK

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::GetDefaultDestinationPort (hr:%lx)\n",this, hr));
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::GetAllDestinationPorts
//
//  Synopsis:
//
//  Arguments:  [ppCDestPort] --
//              [pCount] --
//
//  Returns:
//
//  History:    2-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::GetAllDestinationPorts(CDestinationPort **ppCDestPort, ULONG *pCount)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::GetAllDestinationPorts\n", this));
    HRESULT hr = NOERROR;
    NotfAssert((ppCDestPort && pCount));

    ULONG uElements = 0;
    ULONG pos = 0;

    do
    {   // BEGIN LOCK BLOCK
        CLockSmMutex lck(_Smxs);

        if (g_cRefPorts == 0)
        {
            // nothing to do
            break;
        }

        CDestinationPort *phWnd = new CDestinationPort [g_cRefPorts];

        if (phWnd == 0)
        {
            *pCount = 0;
            *ppCDestPort = 0;
            hr = E_OUTOFMEMORY;
            break;
        }

        CDestinationPort *pDestPort = (CDestinationPort *)g_rgCDestPorts;

        for (ULONG i = 0;
             (   (i < g_urgCDestPortMax)
              && (pos < g_cRefPorts))
             ;i++)
        {
            if ( pDestPort[i].GetPort() != 0)
            {
                if ( IsWindow(pDestPort[i].GetPort()) )
                {
                    phWnd[pos] = g_rgCDestPorts[i];
                    pos++;
                }
                else
                {
                    pDestPort[i].SetPort(0);
                    g_cRefPorts--;
                }
            }
        }
        *ppCDestPort  = phWnd;
        *pCount = pos;
        uElements = g_cRefPorts;

        hr = (pos) ? S_OK : E_FAIL;

        break;
    } while (TRUE); // END LOCK BLOCK

    NotfAssert((uElements == pos));

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::GetAllDestinationPorts (hr:%lx, pCount:%lx)\n",this, hr, *pCount));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::AddDestinationPort
//
//  Synopsis:
//
//  Arguments:  [rCDestPort] --
//              [pCDestination] --
//
//  Returns:
//
//  History:    2-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::AddDestinationPort(CDestinationPort &rCDestPort, CDestination *pCDestination)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::AddDestinationPort\n", this));
    HRESULT hr = E_FAIL;

    NotfAssert((pCDestination));

    // save the destination in the running dest key
    hr = pCDestination->SaveToPersist(c_pszRegKeyRunningDest);
    if (hr == NOERROR)
    {
        CLockSmMutex lck(_Smxs);
        hr = E_FAIL;
        CDestinationPort *pDestPort = (CDestinationPort *)g_rgCDestPorts;

        //
        // check if the port is already registered
        //
        for (ULONG i = 0; i < g_urgCDestPortMax; i ++)
        {
            if ( pDestPort[i].GetPort() == rCDestPort.GetPort())
            {
                // ok, found port; just addref it
                pDestPort[i].AddRef();

                i = g_urgCDestPortMax;
                hr = NOERROR;
            }
        }

        //
        // not found; find an empty slot and add it
        //
        pDestPort = (CDestinationPort *)g_rgCDestPorts;

        if (hr != NOERROR)
        {
            for (ULONG i = 0; i < g_urgCDestPortMax; i ++)
            {
                if (pDestPort[i].GetPort() == 0)
                {
                    pDestPort[i] = rCDestPort;
                    g_cRefPorts++;
                    i = g_urgCDestPortMax;
                    hr = NOERROR;
                }
            }
        }
    }

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::AddDestinationPort (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::RemoveDestinationPort
//
//  Synopsis:
//
//  Arguments:  [hWnd] --
//              [pCDestination] --
//
//  Returns:
//
//  History:    2-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::RemoveDestinationPort(CDestinationPort &rCDestPort, CDestination *pCDestination)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::RemoveDestinationPort\n", this));
    HRESULT hr = E_FAIL;

    NotfAssert((pCDestination));

    // save the destination in the running dest key
    hr = pCDestination->RemovePersist(c_pszRegKeyRunningDest);
    if (rCDestPort.GetPort())
    {
        CLockSmMutex lck(_Smxs);

        CDestinationPort *pDestPort = (CDestinationPort *)g_rgCDestPorts;
        //
        // find the slot with this port
        //
        for (ULONG i = 0; i < g_urgCDestPortMax; i ++)
        {
            if (pDestPort[i].GetPort() == rCDestPort.GetPort())
            {
                if ((pDestPort+i)->Release() == 0)
                {
                    // if last release clean the slot
                    pDestPort[i].SetPort(0);
                    g_cRefPorts--;
                }

                i = g_urgCDestPortMax;
                hr = NOERROR;
            }
        }
    }

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::RemoveDestinationPort (cPorts:%lx, hr:%lx)\n",this,g_cRefPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::LookupDestinationPort
//
//  Synopsis:
//
//  Arguments:  [rCDestPort] --
//              [pCDest] --
//
//  Returns:
//
//  History:    2-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::LookupDestinationPort(CDestinationPort &rCDestPort, CDestination *pCDest)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::LookupDestinationPort\n", this));
    HRESULT hr = E_FAIL;
    ULONG cPorts = 0;

    {
        CLockSmMutex lck(_Smxs);
        CDestinationPort *pDestPort = (CDestinationPort *)g_rgCDestPorts;
        cPorts = g_cRefPorts;

        for (ULONG i = 0; i < g_urgCDestPortMax; i ++)
        {
            if (pDestPort[i].GetPort() == rCDestPort.GetPort())
            {
                // got exchanged
                i = g_urgCDestPortMax;
                hr = NOERROR;
            }
        }
    }

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::LookupDestinationPort (cPorts:%lx, hr:%lx)\n",this, cPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::AddScheduleItem
//
//  Synopsis:   add a package to the global table
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::AddScheduleItem(CPackage *pCPackage, BOOL fCheck)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::AddScheduleItem\n", this));
    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage));

    do
    {
        if (!pCPackage)
        {
            hr = E_INVALIDARG;
            break;
        }
        NotfAssert(( pCPackage->GetNextRunDate() != 0 ));

        if ( pCPackage->GetNextRunDate() == 0 )
        {
            hr = E_INVALIDARG;
            break;
        }

        CLockSmMutex lck(_Smxs);

        ULONG iPos = g_urgScheduleListItemtMax;


        if (g_cSchedListCount >= g_urgScheduleListItemtMax)
        {
            break;
        }

        if (fCheck)
        {
            if (pCPackage)
            {
                iPos = pCPackage->GetIndex();
            }

            if (iPos < g_urgScheduleListItemtMax)
            {
                if (g_rgSortListItem[iPos].notfCookie != pCPackage->GetNotificationCookie())
                {
                    iPos = g_urgScheduleListItemtMax;
                }
            }

            if (iPos >= g_urgScheduleListItemtMax)
            {
                //
                // BUGBUG: need better algorithm here
                //
                NOTIFICATIONCOOKIE notfCookie = pCPackage->GetNotificationCookie();
                for (ULONG i = 0; i < g_urgScheduleListItemtMax; i++)
                {
                    if (g_rgSortListItem[i].notfCookie == notfCookie)
                    {
                        hr = NOERROR;
                        iPos = i;
                        i = g_urgScheduleListItemtMax;
                    }
                }
            }

        }

        hr = pCPackage->SaveToPersist(c_pszRegKey);
        BREAK_ONERROR(hr);

        //g_dateLastSchedListChange = GetCurrentDtTime();

        if (iPos >= g_urgScheduleListItemtMax)
        {
            //
            // did not find entry - find empty slot and add item
            //
            g_cSchedListCount++;
            for (ULONG i = 0; i < g_urgScheduleListItemtMax; i ++)
            {
                if (g_rgSortListItem[i].date == 0)
                {
                    //NotfAssert(( pCPackage->GetNextRunDate() ));
                    g_rgSortListItem[i].date = FileTimeToInt64(pCPackage->GetNextRunDate());
                    g_rgSortListItem[i].notfCookie = pCPackage->GetNotificationCookie();
                    pCPackage->SetIndex(i);
                    i = g_urgScheduleListItemtMax;
                }
            }
        }
        else
        {
            // use the found position
            g_rgSortListItem[iPos].date = FileTimeToInt64(pCPackage->GetNextRunDate());
            NotfAssert(( g_rgSortListItem[iPos].notfCookie == pCPackage->GetNotificationCookie() ));
            g_rgSortListItem[iPos].notfCookie = pCPackage->GetNotificationCookie();
            pCPackage->SetIndex(iPos);
        }

        g_dateLastSchedListChange = GetCurrentDtTime();


        break;
    } while (TRUE);

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::AddScheduleItem (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::RemoveScheduleItem
//
//  Synopsis:   remove a package from the global table
//
//  Arguments:  [CPackage] --
//              [pCPackage] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::RemoveScheduleItem(SCHEDLISTITEMKEY &rSchItem,CPackage *pCPackage)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::RemoveScheduleItem\n", this));
    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage));

    do
    {
        if (!pCPackage)
        {
            hr = E_INVALIDARG;
            break;
        }

        { //BEGIN LOCK BLOCK

            CLockSmMutex lck(_Smxs);

            if (g_cSchedListCount == 0)
            {
                break;
            }
            ULONG index = 0xffffffff;

            index = pCPackage->GetIndex();

            if (index < g_urgScheduleListItemtMax)
            {
                if (g_rgSortListItem[index].notfCookie == rSchItem.notfCookie)
                {
                    hr = NOERROR;
                }
                else
                {
                    NotfDebugOut((DEB_TRACE, "CGlobalNotfMgr::LookupScheduleItem: List not in sync!"));
                    NotfAssert((FALSE && "CGlobalNotfMgr::RemoveScheduleItem not in sync!"));
                }
            }
            if (hr != NOERROR)
            {
                //
                // BUGBUG: need better algorithm here
                //
                for (ULONG i = 0; i < g_urgScheduleListItemtMax; i ++)
                {
                    if (g_rgSortListItem[i].notfCookie == rSchItem.notfCookie)
                    {
                        index = i;
                        hr = NOERROR;
                        i = g_urgScheduleListItemtMax;
                    }
                }
            }
            BREAK_ONERROR(hr);

            if ((index < g_urgScheduleListItemtMax) &&
                (g_rgSortListItem[index].date != 0))
            {
                g_rgSortListItem[index].date = 0;
                g_rgSortListItem[index].notfCookie = COOKIE_NULL;

                g_cSchedListCount--;
                g_dateLastSchedListChange = GetCurrentDtTime();
            }
        } // END LOCK BLOCK

        if (pCPackage)
        {
            // don't care if this fails
            pCPackage->SetIndex(0XFFFFFFFF);
            HRESULT hr1 = pCPackage->RemovePersist(c_pszRegKey);

            //NotfAssert((hr1 == NOERROR));
        }

        break;
    } while (TRUE);


    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::RemoveScheduleItem (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::LookupScheduleItem
//
//  Synopsis:   find all package item currently in the global table
//
//  Arguments:  [CPackage] --
//              [pCPackage] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::LookupScheduleItem(SCHEDLISTITEMKEY &rSchItem, CPackage *pCPackage)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::LookupScheduleItem\n", this));
    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage));

    do
    {
        if (!pCPackage)
        {
            hr = E_INVALIDARG;
            break;
        }
        CLockSmMutex lck(_Smxs);

        ULONG index = 0xffffffff;

        if (g_cSchedListCount == 0)
        {
            break;
        }
        if (pCPackage)
        {
            index = pCPackage->GetIndex();
        }

        if (index < g_urgScheduleListItemtMax)
        {
            if (g_rgSortListItem[index].notfCookie == pCPackage->GetNotificationCookie())
            {
                hr = NOERROR;
            }
            else
            {
                NotfDebugOut((DEB_TRACE, "CGlobalNotfMgr::LookupScheduleItem: List not in sync!"));
                NotfAssert((FALSE && "CGlobalNotfMgr::LookupScheduleItem: List not in sync!"));
            }
        }

        if (hr != NOERROR)
        {
            //
            // BUGBUG: need better algorithm here
            //
            for (ULONG i = 0; i < g_urgScheduleListItemtMax; i ++)
            {
                if (g_rgSortListItem[i].notfCookie == rSchItem.notfCookie)
                {
                    rSchItem = g_rgSortListItem[i];
                    hr = NOERROR;
                    i = g_urgScheduleListItemtMax;
                }
            }
        }

        break;
    } while (TRUE);


    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::LookupScheduleItem (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::SetScheduleItemState
//
//  Synopsis:   find all package item currently in the global table
//
//  Arguments:  [CPackage] --
//              [pCPackage] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::SetScheduleItemState(PNOTIFICATIONCOOKIE pNotfCookie, DWORD dwStateNew, SCHEDLISTITEMKEY *pSchItem)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::SetScheduleItemState\n", this));
    HRESULT hr = E_FAIL;

    do
    {
        if (!pNotfCookie)
        {
            hr = E_INVALIDARG;
            break;
        }

        CLockSmMutex lck(_Smxs);

        if (g_cSchedListCount == 0)
        {
            break;
        }

        if (hr != NOERROR)
        {
            //
            // BUGBUG: need better algorithm here
            //
            for (ULONG i = 0; i < g_urgScheduleListItemtMax; i ++)
            {
                if (g_rgSortListItem[i].notfCookie == *pNotfCookie)
                {
                    if (pSchItem)
                    {
                        *pSchItem = g_rgSortListItem[i];
                    }
                    g_rgSortListItem[i].dwState |= dwStateNew;
                    hr = NOERROR;
                    i = g_urgScheduleListItemtMax;
                }
            }
        }
        break;
    } while (TRUE);

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::SetScheduleItemState (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::SetScheduleItemState
//
//  Synopsis:   find all package item currently in the global table
//
//  Arguments:  [CPackage] --
//              [pCPackage] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::SetScheduleItemState(DWORD dwIndex, DWORD dwStateNew, SCHEDLISTITEMKEY *pSchItem)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::SetScheduleItemState\n", this));
    HRESULT hr = E_FAIL;

    do
    {
        CLockSmMutex lck(_Smxs);

        if (g_cSchedListCount == 0)
        {
            break;
        }

        if (dwIndex >= g_urgScheduleListItemtMax)
        {
            break;
        }
        ULONG i = dwIndex;

        if (g_rgSortListItem[i].date != 0)
        {
            if (pSchItem)
            {
                *pSchItem = g_rgSortListItem[i];
            }
            g_rgSortListItem[i].dwState |= dwStateNew;
            hr = NOERROR;
            i = g_urgScheduleListItemtMax;
        }

        break;
    } while (TRUE);

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::SetScheduleItemState (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::LoadScheduleItemPackage
//
//  Synopsis:   load a CPackage based on the SchItem key information
//
//  Arguments:  [rSchItem] --
//              [ppCPackage] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::LoadScheduleItemPackage(
                            SCHEDLISTITEMKEY &rSchItem,
                            CPackage **ppCPackage)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::LoadScheduleItemPackage\n", this));
    HRESULT hr = E_FAIL;
    NotfAssert((ppCPackage));

    hr = CPackage::LoadPersistedPackage(
                       c_pszRegKey
                       ,&rSchItem.notfCookie
                       ,0
                       ,ppCPackage);


    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::LoadScheduleItemPackage (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::GetScheduleItemCount
//
//  Synopsis:   get number of elements currently in the global table
//
//  Arguments:  [pulCount] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::GetScheduleItemCount(ULONG *pulCount)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::GetScheduleItemCount\n", this));
    HRESULT hr = E_FAIL;
    NotfAssert((pulCount));
    CLockSmMutex lck(_Smxs);

    *pulCount = g_cSchedListCount;

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::GetScheduleItemCount (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::IsScheduleItemSyncd
//
//  Synopsis:   checks if the date and count match with the global table
//              date and count
//
//  Arguments:  [rdate] --
//              [uCount] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::IsScheduleItemSyncd(CFileTime &rdate, ULONG uCount)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::IsScheduleItemSynced\n", this));
    HRESULT hr = E_FAIL;

    {   // BEGIN LOCK BLOCK
        CLockSmMutex lck(_Smxs);

        hr = (   (g_dateLastSchedListChange == rdate)
              && (uCount == g_cSchedListCount))
             ? S_OK : S_FALSE;

    }  // END LOCK BLOCK

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::IsScheduleItemSyncd (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::GetAllScheduleItems
//
//  Synopsis:   retrieves all item keys from the global table
//
//  Arguments:  [ppSchItems] --
//              [pCount] --
//              [rdate] --
//
//  Returns:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::GetAllScheduleItems(SCHEDLISTITEMKEY **ppSchItems, ULONG *pCount, CFileTime &rdate)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::GetAllScheduleItems\n", this));
    HRESULT hr = NOERROR;
    NotfAssert((ppSchItems && pCount));

    ULONG uElements = 0;
    ULONG pos = 0;

    do
    {   // BEGIN LOCK BLOCK

        CLockSmMutex lck(_Smxs);

        if (g_cSchedListCount == 0)
        {
            *pCount = 0;
            *ppSchItems = 0;
            break;
        }

        SCHEDLISTITEMKEY *pSchItems = new SCHEDLISTITEMKEY [g_cSchedListCount];

        if (pSchItems == 0)
        {
            *pCount = 0;
            *ppSchItems = 0;
            hr = E_OUTOFMEMORY;
            break;
        }

        ULONG cBogus = 0;
        ULONG   i;

        for (i = 0, pos = 0
             ;(   (i < g_urgScheduleListItemtMax)
               && (pos < g_cSchedListCount))
             ; i ++)
        {
            if (g_rgSortListItem[i].date != 0)
            {
                if (CPackage::IsPersisted(c_pszRegKey,&g_rgSortListItem[i].notfCookie) == NOERROR)
                {
                    pSchItems[pos] = g_rgSortListItem[i];
                    pos++;
                }
                else
                {
                    g_rgSortListItem[i].date = 0;
                    g_rgSortListItem[i].notfCookie = CLSID_NULL;
                }
            }
        }
        if (g_cSchedListCount != pos)
        {
            g_dateLastSchedListChange = GetCurrentDtTime();
            g_cSchedListCount = pos;
        }

        *ppSchItems  = pSchItems;
        *pCount = pos;
        rdate = g_dateLastSchedListChange;
        uElements = (ULONG)g_cSchedListCount;

        hr = (pos) ? S_OK : E_FAIL;

        break;
    } while (TRUE); // END LOCK BLOCK

    NotfAssert((uElements >= pos));

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::GetAllScheduleItems (hr:%lx, pCount:%lx)\n",this, hr, *pCount));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::HandlePackage
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    2-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::HandlePackage(CPackage *pCPackage)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::HandlePackage\n", this));
    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage));

    BOOL fBroadcast = pCPackage->IsBroadcast();
    BOOL fReport    = pCPackage->IsReport();
    ULONG uXProcess = 0;
    BOOL  fSaved = FALSE;
    DWORD dwMode = 0;
    CHAR szPackageSubKey[SZREGVALUE_MAX] = {0};

    do
    {
        // save the package first
        ULONG uPkgCount = GetPackageCount();
        wsprintf(szPackageSubKey,"%lx",uPkgCount);

        if (fBroadcast)
        {
            //
            // find all destination ports
            //
            ULONG uHWndCount = 0;
            CDestinationPort *pDestPort= 0;

            hr = GetAllDestinationPorts(&pDestPort, &uHWndCount);

            if (hr == NOERROR)
            {
                for (ULONG i = 0; i < uHWndCount; i++)
                {
                    HWND hWnd = (pDestPort+i)->GetPort();
                    NotfAssert((hWnd));

                    DWORD dwProcessId = 0;
                    DWORD dwThreadId = GetWindowThreadProcessId(hWnd, &dwProcessId);

                    if (GetCurrentProcessId() == dwProcessId)
                    {
                        dwMode = 0;
                    }
                    else
                    {
                        dwMode = uPkgCount;
                        uXProcess++;
                        if (!fSaved)
                        {
                            pCPackage->SetDestPort(GetThreadNotificationWnd(), 
                                                    GetCurrentThreadId());
                            hr = pCPackage->SaveToPersist(c_pszRegKeyPackage,szPackageSubKey);
                            BREAK_ONERROR(hr);
                            pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_CROSSPROCESS);
                            fSaved = TRUE;
                        }
                    }

                    //  BUGBUGBUG
                    HRESULT hr1 = pCPackage->Deliver(hWnd, dwMode);
                    //  Doesn't make sense to reset the port.
                    //  pCPackage->SetDestPort(hWnd, dwThreadId);
                }
            }

            if (pDestPort)
            {
                delete [] pDestPort;
            }
            
            break;
        }
        else if (fReport && (pCPackage->GetCDestPort().GetPort()) )
        {
            CDestinationPort &rCDest = pCPackage->GetCDestPort();

            HWND hWnd = rCDest.GetPort();

            if (!hWnd)
            {
                hr = E_FAIL;
                break;
            }

            DWORD dwProcessId = 0;
            DWORD dwThreadId = GetWindowThreadProcessId(hWnd, &dwProcessId);

            if (GetCurrentProcessId() == dwProcessId)
            {
                dwMode = 0;
            }
            else
            {
                dwMode = uPkgCount;
                uXProcess++;
                if (!fSaved)
                {
                    pCPackage->SetDestPort(GetThreadNotificationWnd(), 
                                            GetCurrentThreadId());
                    hr = pCPackage->SaveToPersist(c_pszRegKeyPackage,szPackageSubKey);
                    pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_CROSSPROCESS);
                    BREAK_ONERROR(hr);
                    fSaved = TRUE;
                }

            }

            hr = pCPackage->Deliver(hWnd, dwMode);
            //  Reset it back.
            pCPackage->SetDestPort(hWnd, dwThreadId);
            break;
        }
        else if (pCPackage->IsForDefaultProcess())
        {
            CDestinationPort cDefDest;

            hr = GetDefaultDestinationPort(&cDefDest);
            if (hr == NOERROR)
            {
                HWND hWnd = cDefDest.GetPort();
                NotfAssert((hWnd));
                DWORD dwProcessId = 0;
                DWORD dwThreadId = GetWindowThreadProcessId(hWnd, &dwProcessId);

                if (GetCurrentProcessId() == dwProcessId)
                {
                    dwMode = 0;
                }
                else
                {
                    dwMode = uPkgCount;
                    uXProcess++;
                    if (!fSaved)
                    {
                        pCPackage->SetDestPort(GetThreadNotificationWnd(), 
                                                GetCurrentThreadId());
                        hr = pCPackage->SaveToPersist(c_pszRegKeyPackage,szPackageSubKey);
                        pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_CROSSPROCESS);
                        BREAK_ONERROR(hr);
                        fSaved = TRUE;
                    }
                }

                hr = pCPackage->Deliver(hWnd, dwMode);
                //  Reset it back.
                pCPackage->SetDestPort(hWnd, dwThreadId);
                break;
            }
        }

        // default case
        {
            CDestination *pCDestination = 0;
            hr = LookupDestination(pCPackage, &pCDestination,0);
            if (hr == NOERROR)
            {
                NotfAssert((pCDestination));
                HWND hWnd = pCDestination->GetPort();
                NotfAssert((hWnd));

                DWORD dwProcessId = 0;
                DWORD dwThreadId = GetWindowThreadProcessId(hWnd, &dwProcessId);

                if (GetCurrentProcessId() == dwProcessId)
                {
                    dwMode = 0;
                }
                else
                {
                    dwMode = uPkgCount;
                    uXProcess++;
                    if (!fSaved)
                    {
                        pCPackage->SetDestPort(GetThreadNotificationWnd(), 
                                                GetCurrentThreadId());
                        hr = pCPackage->SaveToPersist(c_pszRegKeyPackage,szPackageSubKey);
                        BREAK_ONERROR(hr);
                        fSaved = TRUE;
                    }

                }

                hr = pCPackage->Deliver(hWnd, dwMode);
                //  Reset it back.
                pCPackage->SetDestPort(hWnd, dwThreadId);
            }
        }

        break;
    } while (TRUE);

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::LookupDestination
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//              [ppCDestination] --
//              [dwFlags] --
//
//  Returns:
//
//  History:    2-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::LookupDestination(
                        CPackage              *pCPackage,
                        CDestination         **ppCDestination,
                        DWORD                 dwFlags
                        )
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::LookupDestination\n", this));
    HRESULT hr = E_FAIL;

    NotfAssert((pCPackage));
    NotfAssert((ppCDestination));
    *ppCDestination = 0;

    BOOL fSynchronous = pCPackage->IsSynchronous();
    BOOL fBroadcast   = pCPackage->IsBroadcast();
    BOOL fReport      = pCPackage->IsReport();
    NOTIFICATIONTYPE nofType = pCPackage->GetNotificationID();
    CLSID clsDest = pCPackage->GetDestID();

    DELIVERMODE delMode = pCPackage->GetDeliverMode();
    CDestination *pCDestination = 0;

    {
    LPSTR pszWhere = 0;
    LPSTR pszSubKeyIn = 0;

    LPCSTR    pszStr = c_pszRegKey; // the global registry key
    CDestination *pCDest = 0;
    HKEY      hKey = 0;
    const int cbStrKeyLen = 1024;
    DWORD dwValueLen = SZREGVALUE_MAX;
    char szValue[SZREGVALUE_MAX];

    do
    {
        long   lRes;
        DWORD  dwDisposition, dwIndex = 0;
        CHAR   szLocation[SZREGSETTING_MAX];

        // construct the new location
        strcpy(szLocation,c_pszRegKeyRunningDest);

        lRes = RegCreateKeyEx(HKEY_CURRENT_USER, szLocation, 0, NULL, 0,
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
                NotfAssert((pCDest == 0));

                HRESULT hr1 = CDestination::LoadFromPersist(szLocation,szValue, 0, &pCDest);

                if (hr1 == NOERROR)
                {
                    NotfAssert((pCDest));
                    DESTINATIONDATA destData;

                    pCDest->GetDestinationData(&destData);

                    NotfAssert(( pCDest->GetNotfTypes() ));

                    if (clsDest == *pCDest->GetDestId())
                    {
                        // the class IDs match
                        if (!IsWindow(pCDest->GetPort()))
                        {
                            // dete the entry
                            pCDest->RemovePersist(szLocation);
                        }
                        else if (   pCDest->IsDestMode(NM_ACCEPT_DIRECTED_NOTIFICATION)
                                 || (pCDest->LookupNotificationType(nofType) == NOERROR)
                                )

                        {
                            // addref gets transfered
                            *ppCDestination = pCDest;
                            pCDest = 0;
                            hr = NOERROR;
                            lRes = ERROR_NO_MORE_ITEMS;
                        }
                    }
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

    } // end reg lookup

    // Note: ppCDestination is addref if it contains a valid destination
    //

    NotfAssert((   ((hr == NOERROR) && (*ppCDestination))
                 || ((hr != NOERROR) && (!*ppCDestination)) ));

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::LookupDestination (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   NotfMgrWndProc
//
//  Synopsis:   the transaction callback function
//
//  Arguments:  [hWnd] --
//              [WPARAM] --
//              [wParam] --
//              [lParam] --
//
//  Returns:
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LRESULT CALLBACK NotfMgrWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (   (msg >= WM_TRANS_FIRST && msg <= WM_TRANS_LAST)
        || (msg == WM_TIMECHANGE) 
        || ((msg == WM_TIMER) && ((wParam == NOTF_SCHED_TIMER) || (wParam == NOTF_DELAY_TIMER))))

    {
        switch (msg)
        {
#ifdef WITH_EXEPTION
                _try
#endif //WITH_EXEPTIO
                {
                }
#ifdef WITH_EXEPTION
                _except(UrlMonInvokeExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
                {
                    dwFault = GetExceptionCode();

                    #if DBG == 1
                    //
                    // UrlMon catches exceptions when the server generates them. This is so we can
                    // cleanup properly, and allow the client to continue.
                    //
                    if (   dwFault == STATUS_ACCESS_VIOLATION
                        || dwFault == 0xC0000194 /*STATUS_POSSIBLE_DEADLOCK*/
                        || dwFault == 0xC00000AA /*STATUS_INSTRUCTION_MISALIGNMENT*/
                        || dwFault == 0x80000002 /*STATUS_DATATYPE_MISALIGNMENT*/ )
                    {
                        WCHAR iidName[256];
                        iidName[0] = 0;
                        char achProgname[256];
                        achProgname[0] = 0;

                        GetModuleFileNameA(NULL,achProgname,sizeof(achProgname));
                        NotfDebugOut((DEB_FORCE,
                                       "UrlMon has caught a fault 0x%08x on behalf of application %s\n",
                                       dwFault, achProgname));
                        NotfAssert((!"The application has faulted processing. Check the kernel debugger for useful output.URLMon can continue but you probably want to stop and debug the application."));

                    }
                    #endif
                }
#endif //WITH_EXEPTIO

        case WM_THREADPACKET_POST:
        case WM_THREADPACKET_SEND:
            {
                CThreadPacket  *pCThreadPacket = (CThreadPacket *) lParam;
                NotfAssert((pCThreadPacket != NULL));
                NotfDebugOut((DEB_MGR, "%p _IN CThreadPacket::NotfMgrWndProc (Msg:%x)\n", pCThreadPacket, wParam));

                pCThreadPacket->OnPacket(msg,(DWORD)wParam);

                NotfDebugOut((DEB_MGR, "%p OUT CThreadPacket::NotfMgrWndProc (Msg:%x) WM_TRANS_PACKET \n", pCThreadPacket, wParam));

            }
            break;
        case WM_PROCESSPACKET_POST :
        case WM_PROCESSPACKET_SEND :
        {
            DWORD dwParam = lParam;
            NotfAssert((dwParam));
            CPackage *pCPkg = 0;
            HRESULT hr = NOERROR;

            CHAR szPackageSubKey[SZREGVALUE_MAX] = {0};
            wsprintf(szPackageSubKey,"%lx",dwParam);
            CHAR szTemp[512];
            strcpy(szTemp, c_pszRegKeyPackage);
            strcat(szTemp, szPackageSubKey);

            hr = RegIsPersistedKey(HKEY_CURRENT_USER, szTemp, szPackageSubKey);
            if (hr == NOERROR)
            {
                //unpersist the package
                hr = CPackage::LoadFromPersist(c_pszRegKeyPackage,szPackageSubKey, 0, &pCPkg);
                // the new package is addref'd
                // release is called inside OnPackage

                if ((hr == NOERROR) && pCPkg)
                {
                    pCPkg->SetCrossProcessId(dwParam);
                    pCPkg->SetNotificationState(pCPkg->GetNotificationState() | PF_CROSSPROCESS);
                    pCPkg->OnPacket(msg, 0);
                }
            }
        }
        break;
        case WM_PROCESSWAKEUP :
        {
            CThrottleListAgent *pCThrottleAgent = GetThrottleListAgent();

            NotfAssert(( pCThrottleAgent ));
            pCThrottleAgent->OnWakeup(WT_NEXTITEM, 0);

        }
        break;

        case WM_TIMER:
        {
            CScheduleAgent *pthis = GetScheduleAgent();

            NotfAssert((pthis));

            switch (wParam)
            {
                case NOTF_SCHED_TIMER:
                    pthis->OnWakeup(WT_SCHED);
                    break;

                case NOTF_DELAY_TIMER:
                    pthis->OnWakeup(WT_DELAY);
                    break;
            }

        }
        break;
        
        case WM_TIMECHANGE :
        {
            CSchedListAgent *pListAgent = GetScheduleListAgent();
            if (pListAgent)
            {
                pListAgent->OnWakeup(WT_TIMECHANGE);
            }
        }
        break;

        case WM_SYNC_DEF_PROC_NOTIFICATIONS:
        {
            CSchedListAgent *pSchedListAgent = GetScheduleListAgent();
            if (pSchedListAgent)
            {
                pSchedListAgent->Resync();
            }
        }
        break;

        case WM_ENDSESSION:
        {
            if (wParam == TRUE)
            {
                //  Clean up running items by delivering NOTIFICATIONTYPE_TASKS_ABORT 
                //  to all notifications on this thread.                
            }
        }
        break;

        }
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::AddItemToRunningList
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::AddItemToRunningList(CPackage *pCPackage, DWORD dwMode)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::AddItemToRunningList\n", this));
    HRESULT hr = S_FALSE;
    NotfAssert((pCPackage));
    PNOTIFICATIONTYPE pNotificationType = 0;;

    do
    {
        if (!pCPackage)
        {
            hr = E_INVALIDARG;
            break;
        }
        CLockSmMutex lck(_Smxs);

        ULONG index = 0xffffffff;

        if (g_cThrottleItemCount == 0)
        {
            hr = S_OK;
            break;
        }

        hr = S_OK;  // assume the item can be dispatched
        LONG posItem = -1;
        for (ULONG i = 0; i < g_cThrottleItemCount; i++)
        {
            if (g_rgThrottleItem[i].NotificationType == *pCPackage->GetNotificationType())
            {
                posItem =  i;
                i = g_cThrottleItemCount;
            }
        }

        if ((posItem < 0)  || (posItem >= THROTTLE_WAITING_MAX))
        {
            hr = S_OK;
            break;
        }

        //
        // do not deliver scheduled notifications if disabled
        //
        #define TF_RESTRICTIONS     (TF_DONT_DELIVER_SCHEDULED_ITEMS | \
                                     TF_APPLY_UPDATEINTERVAL | \
                                     TF_APPLY_EXCLUDE_RANGE)
                                     
        if ((pCPackage->GetNotificationState() & PF_SCHEDULED) &&
            (g_rgThrottleItem[posItem].dwFlags & TF_RESTRICTIONS))
        {
            if (g_rgThrottleItem[posItem].dwFlags & TF_DONT_DELIVER_SCHEDULED_ITEMS)
            {
                hr = S_FALSE;
            }

            if ((hr == S_OK) && (g_rgThrottleItem[posItem].dwFlags & TF_APPLY_UPDATEINTERVAL))
            {
                __int64 intervalMin,
                        intervalNow;
                SYSTEMTIME st;
                CFileTime ftNow;
                
                GetLocalTime(&st);
                SystemTimeToFileTime(&st, &ftNow);
#ifndef unix                
                intervalMin = __int64(g_rgThrottleItem[posItem].dwIntervalMin) *
#else
                // Looks like the Solaris C++ compiler cant handle new
                // style casts.
                intervalMin = (__int64)(g_rgThrottleItem[posItem].dwIntervalMin) * 
#endif /* unix */ 
                              ONE_MINUTE_IN_FILETIME;
                intervalNow = FileTimeToInt64(ftNow - pCPackage->GetPrevRunDate());

                if (intervalNow < intervalMin)
                {
                    pCPackage->SetNextRunInterval(intervalMin);
                    hr = S_FALSE;
                }                
            }

            if ((hr == S_OK) && (g_rgThrottleItem[posItem].dwFlags & TF_APPLY_EXCLUDE_RANGE))
            {
                SYSTEMTIME st;
                CFileTime ftNow,
                          ftBegin,
                          ftEnd;
                
                GetLocalTime(&st);

                SystemTimeToFileTime(&st, &ftNow);
                
                st.wSecond = 0;
                st.wMilliseconds = 0;
                
                st.wHour   = g_rgThrottleItem[posItem].dateExcludeBegin.wHour;
                st.wMinute = g_rgThrottleItem[posItem].dateExcludeBegin.wMinute;
                SystemTimeToFileTime(&st, &ftBegin);
                
                st.wHour   = g_rgThrottleItem[posItem].dateExcludeEnd.wHour;
                st.wMinute = g_rgThrottleItem[posItem].dateExcludeEnd.wMinute;
                SystemTimeToFileTime(&st, &ftEnd);

                //  if these values are normalized (ie. begin comes before end)
                if (ftBegin <= ftEnd)
                {
                    //  Then just check to see if time now is between begin 
                    //  and end.  (ie.  ftEnd >= ftNow >= ftBegin)
                    if ((ftNow >= ftBegin) && (ftNow <= ftEnd))
                    {
                        hr = S_FALSE;
                    }
                }
                else
                {
                    //  Begin and end are not normalized.  So we check to see if
                    //  now is before end or now is after begin.

                    //  For example:
                    //  Assuming begin is 6pm and end is 6am.  If now is 5 pm, the
                    //  notification should run.  If now is 10pm or 4am, the 
                    //  notification should not run.
                    if ((ftNow <= ftEnd) || (ftNow >= ftBegin))
                    {
                        hr = S_FALSE;
                    }
                }
            }
            if (hr == S_FALSE)
            {
                CSchedListAgent *pCSchLst = GetScheduleListAgent();
                if (pCSchLst)
                {
                    GetThrottleListAgent()->RevokePackage(&pCPackage->GetNotificationCookie(),
                                                          NULL, 
                                                          0);
                    //  Reschedule this package
                    pCPackage->SetNotificationState(pCPackage->GetNotificationState() 
                                                    & ~PF_DELIVERED);
                    pCSchLst->HandlePackage(pCPackage);
                }
                break;
            }
        }

        if (g_rgThrottleItem[posItem].nParallel == -1)
        {
            // do not throttle at all
            hr = S_OK;
            break;
        }

        //
        // found the notification type
        //
        BOOL fFound = FALSE;
        hr = S_FALSE;
        NotfAssert(( g_rgThrottleItem[posItem].nParallel ));

        if ((g_rgThrottleItem[posItem].nRunning < g_rgThrottleItem[posItem].nParallel) 
            && (!(dwMode & TL_ADD_TO_WAIT)))
        {
            //
            // item can run - add it to the running list
            //
            LONG    posVacant = -1;
            BOOL    fRunning = FALSE;
            for (LONG j = 0; (j < THROTTLEITEM_MAX) && (j < g_rgThrottleItem[posItem].nRunning + 1); j++)
            {
                if (g_rgThrottleItem[posItem].nRunningCookie[j] == pCPackage->GetNotificationCookie() )
                {
                    // already added
                    j = g_urgThrottleItemtMax + 1;
                    fRunning = TRUE;
                    TNotfDebugOut((DEB_TFLOW, "%p Already on throttle list in CGlobalNotfMgr::AddItemToRunningList\n", pCPackage));
                }
                else if (g_rgThrottleItem[posItem].nRunningCookie[j] == COOKIE_NULL)
                {
                //  We still need to loop through all items in case we will
                //  find a running instance.
//                    g_rgThrottleItem[posItem].nRunningCookie[j] = pCPackage->GetNotificationCookie();
//                    j = g_urgThrottleItemtMax;
                    if (!fFound)    {
                        posVacant = j;
                        fFound = TRUE;
                    }
                    TNotfDebugOut((DEB_TFLOW, "%p Added to throttle list in CGlobalNotfMgr::AddItemToRunningList\n", pCPackage));
                }
            }
            if (fRunning)   {
                //  We are going to return S_FALSE. So we'd better add the
                //  waiting counter.
                g_rgThrottleItem[posItem].nWaiting++;
            } else if (fFound == TRUE)  {
                // found a slot and item can run
                NotfAssert((posVacant != -1));
                g_rgThrottleItem[posItem].nRunningCookie[posVacant] = pCPackage->GetNotificationCookie();
                g_rgThrottleItem[posItem].nRunning++;
                hr = S_OK;
            }
        }
        else
        {
            //
            // item can not run - add it to the waiting list
            //
            DWORD dwFoundInvalid = 0;
            BOOL  fCheckInvalid = TRUE;

            do
            {
                dwFoundInvalid = 0;

                NotfAssert(( g_rgThrottleItem[posItem].nRunning == g_rgThrottleItem[posItem].nParallel ));

                DWORD dwPriorityMinWaiting = 0;
                LONG pos = -1;
                LONG posEmpty = -1;
                BOOL fAlreadyWaiting = FALSE;
                for (LONG j = 0; j < THROTTLE_WAITING_MAX; j++)
                {
                    if (g_rgThrottleItem[posItem].nWaitItem[j].NotificationCookie == pCPackage->GetNotificationCookie())
                    {
                        // stop the loop - nothing to do
                        pos = -1;
                        j = THROTTLE_WAITING_MAX;
                        fAlreadyWaiting = TRUE;
                        TNotfDebugOut((DEB_TFLOW, "%p Already on waiting list in CGlobalNotfMgr::AddItemToRunningList\n", pCPackage));
                    }
                    else if (g_rgThrottleItem[posItem].nWaitItem[j].NotificationCookie != COOKIE_NULL)
                    {
                        // remember the position and the item with the lowest priority
                        if (g_rgThrottleItem[posItem].nWaitItem[j].dwCurrentPriority < dwPriorityMinWaiting)
                        {
                            dwPriorityMinWaiting = g_rgThrottleItem[posItem].nWaitItem[j].dwCurrentPriority;
                            pos =  j;
                        }
                    }
                    else
                    {
                        // found and empty slot - keep looping; our cookie might be in the list
                        dwPriorityMinWaiting = 0;
                        posEmpty = j;
                    }
                }

                //  We still want to increase this count, so we can wake up the
                //  process later.
                g_rgThrottleItem[posItem].nWaiting++;
                if (fAlreadyWaiting)
                {
                    // stop nothing to do
                    break;
                }

                if ( (posEmpty > -1) && (posEmpty < THROTTLE_WAITING_MAX) )
                {
                    pos = posEmpty;
                }

                if ( (pos > -1) && (pos < THROTTLE_WAITING_MAX) )
                {
                    //
                    // add item to waiting list

                    if (g_rgThrottleItem[posItem].dwPriorityMaxWaiting < pCPackage->GetPriority())
                    {
                        g_rgThrottleItem[posItem].dwPriorityMaxWaiting =  pCPackage->GetPriority();
                    }
                    TNotfDebugOut((DEB_TFLOW, "%p Added to waiting list in CGlobalNotfMgr::AddItemToRunningList\n", pCPackage));

                    g_rgThrottleItem[posItem].nWaitItem[pos].NotificationCookie = pCPackage->GetNotificationCookie();
                    g_rgThrottleItem[posItem].nWaitItem[pos].hWnd = GetThreadNotificationWnd();
                    g_rgThrottleItem[posItem].nWaitItem[pos].dwCurrentPriority = pCPackage->GetPriority();
                }
                else if (fCheckInvalid)
                {
                    // validate the entries
                    for (j = 0; j < THROTTLE_WAITING_MAX; j++)
                    {
                        if (!IsWindow(g_rgThrottleItem[posItem].nWaitItem[j].hWnd) )
                        {
                            //BUGBUG should we be looking elsewhere to clean up also
                            
                            // invalid port (hwnd) remove it!
                            g_rgThrottleItem[posItem].nWaitItem[pos].NotificationCookie = COOKIE_NULL;
                            g_rgThrottleItem[posItem].nWaitItem[pos].hWnd = 0;
                            g_rgThrottleItem[posItem].nWaitItem[pos].dwCurrentPriority = 0;
                            g_rgThrottleItem[posItem].nWaiting--;
                            dwFoundInvalid++;
                        }
                    }

                    fCheckInvalid = FALSE;
                }
                
            } while (dwFoundInvalid);
        }

        break;
    } while (TRUE);

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::AddItemToRunningList (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::RemoveItemFromRunningList
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::RemoveItemFromRunningList(CPackage *pCPackage, DWORD dwMode)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::RemoveItemFromRunningList\n", this));
    HRESULT hr = S_FALSE;
    NotfAssert((pCPackage));
    BOOL fWakeup = FALSE;
    PNOTIFICATIONTYPE pNotificationType = 0;
    PNOTIFICATIONCOOKIE pNotificationCookie = 0;

    do
    {
        if (!pCPackage)
        {
            hr = E_INVALIDARG;
            break;
        }

        pNotificationType = pCPackage->GetNotificationType();
        pNotificationCookie = &pCPackage->GetNotificationCookie();

        CLockSmMutex lck(_Smxs);

        ULONG index = 0xffffffff;

        if (g_cThrottleItemCount == 0)
        {
            hr = S_OK;
            break;
        }

        hr = S_FALSE;
        LONG posItem = -1;
        for (ULONG i = 0; i < g_cThrottleItemCount; i++)
        {
            if (g_rgThrottleItem[i].NotificationType == *pCPackage->GetNotificationType())
            {
                posItem =  i;
                i = g_cThrottleItemCount;
            }
        }

        if ((posItem < 0)  || (posItem >= THROTTLE_WAITING_MAX))
        {
            hr = S_FALSE;
            break;
        }
        NotfAssert((g_rgThrottleItem[posItem].NotificationType == *pNotificationType));

        if (g_rgThrottleItem[posItem].nParallel == -1)
        {
            // ok
            hr = S_OK;
        }

        if (g_rgThrottleItem[posItem].nRunning == 0)
        {
            hr = S_OK;
            break;
        }
        
        // need to find the slot of the cookie
        //

        // This for loop used to exit at nRunning, but that doesn't
        // work since the notifications aren't necessarily the first nRunning
        // in the array
        for (ULONG j = 0; j < g_urgThrottleItemtMax; j++)
        {
            if (g_rgThrottleItem[posItem].nRunningCookie[j] == *pNotificationCookie)
            {
                g_rgThrottleItem[posItem].nRunningCookie[j] = COOKIE_NULL;
                g_rgThrottleItem[posItem].nRunning--;
                j = g_urgThrottleItemtMax;
                hr = S_OK;
            }
        }

        // done if none is waiting
        if (g_rgThrottleItem[posItem].nWaiting == 0)
        {
            break;
        }

        //
        // find the item with highest priority
        //
        ULONG PriorityMax = 0;
        LONG pos = -1;
        for (j = 0; j < THROTTLE_WAITING_MAX; j++)
        {
            //
            // find the item with the highest priority
            //
            if (   (g_rgThrottleItem[posItem].nWaitItem[j].NotificationCookie != COOKIE_NULL )
                && (g_rgThrottleItem[posItem].nWaitItem[j].dwCurrentPriority > PriorityMax))
            {
                if (IsWindow(g_rgThrottleItem[posItem].nWaitItem[j].hWnd) )
                {
                    PriorityMax = g_rgThrottleItem[posItem].nWaitItem[j].dwCurrentPriority;
                    pos = j;
                }
                else
                {
                    // invalid port (hwnd) remove it!
                    g_rgThrottleItem[posItem].nWaitItem[pos].NotificationCookie = COOKIE_NULL;
                    g_rgThrottleItem[posItem].nWaitItem[pos].hWnd = 0;
                    g_rgThrottleItem[posItem].nWaitItem[pos].dwCurrentPriority = 0;
                    g_rgThrottleItem[posItem].nWaiting--;
                }
            }
        }

        //NotfAssert((    ((pos != -1) && (pos < THROTTLE_WAITING_MAX))
        //            ||  (g_rgThrottleItem[posItem].nWaiting == 0)           ));

        if (pos >= 0  && pos < THROTTLE_WAITING_MAX)
        {
            if ( IsWindow(g_rgThrottleItem[posItem].nWaitItem[pos].hWnd) )
            {
                NotfDebugOut((DEB_MGR, "%p CPackage:%p === SendNotifyMessage (hwnd:%lx) WM_THREADPACKET_SEND\n", this,this, g_rgThrottleItem[posItem].nWaitItem[pos].hWnd ));
                fWakeup = !PostMessage(g_rgThrottleItem[posItem].nWaitItem[pos].hWnd, WM_PROCESSWAKEUP, 0, 0);
                NotfDebugOut((DEB_MGR, "%p Out CGlobalNotfMgr:%p === SendNotifyMessage (hwnd:%lx, fSend:%lx) WM_THREADPACKET_POST\n", this,this, g_rgThrottleItem[posItem].nWaitItem[pos].hWnd, fWakeup));
            }
            g_rgThrottleItem[posItem].nWaitItem[pos].NotificationCookie = COOKIE_NULL;
            g_rgThrottleItem[posItem].nWaitItem[pos].hWnd = 0;
            g_rgThrottleItem[posItem].nWaitItem[pos].dwCurrentPriority = 0;
            g_rgThrottleItem[posItem].nWaiting--;
        }
        else if (g_rgThrottleItem[posItem].nWaiting)
        {
            //  We want to nuke the count so we can get a fresh count by calling
            //  up all waiting process.
            g_rgThrottleItem[posItem].nWaiting = 0;
            fWakeup = TRUE;
        }

        break;
    } while (TRUE);

    if (fWakeup)
    {
        // broadcast the wakeup all threats who wait to run
        // a throttle notification
        WakeupAll();
    }

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::RemoveItemFromRunningList (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::RemoveItemFromWaitingList
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//              [dwMode] --
//
//  Returns:
//
//  History:    8-1-1997   DZhang(Dachuan Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::RemoveItemFromWaitingList(CPackage *pCPackage, DWORD dwMode)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::RemoveItemFromWaitingList\n", this));
    NotfAssert((pCPackage));
    PNOTIFICATIONTYPE pNotificationType = 0;

    if (!pCPackage)
        return E_INVALIDARG;

    pNotificationType = pCPackage->GetNotificationType();
    NOTIFICATIONCOOKIE &NotificationCookie = pCPackage->GetNotificationCookie();

    CLockSmMutex lck(_Smxs);

    ULONG index = 0xffffffff;

    if (g_cThrottleItemCount == 0)
        return S_OK;

    LONG posItem = -1;
    for (ULONG i = 0; i < g_cThrottleItemCount; i++)
    {
        if (g_rgThrottleItem[i].NotificationType == *pCPackage->GetNotificationType())
        {
            posItem =  i;
            i = g_cThrottleItemCount;
        }
    }

    if ((posItem < 0)  || (posItem >= THROTTLE_WAITING_MAX))
        return S_FALSE;


    if (g_rgThrottleItem[posItem].nParallel == -1)
        return S_OK;
 

    THROTTLE_ITEM * pThrottleItem = &(g_rgThrottleItem[posItem]);
    //
    // find the item
    //
    LONG pos = -1;
    for (ULONG j = 0; j < THROTTLE_WAITING_MAX; j++)
    {
        if (pThrottleItem->nWaitItem[j].NotificationCookie ==NotificationCookie)
        {
            pThrottleItem->nWaitItem[j].NotificationCookie = COOKIE_NULL;
            pThrottleItem->nWaitItem[j].hWnd = 0;
            pThrottleItem->nWaitItem[j].dwCurrentPriority = 0;
            if (IsWindow(pThrottleItem->nWaitItem[j].hWnd) )
            {
                g_rgThrottleItem[posItem].nWaiting--;
            }
        }
    }

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::RemoveItemFromWaitingList\n",this));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::RegisterThrottleNotificationType
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
HRESULT CGlobalNotfMgr::RegisterThrottleNotificationType(ULONG cItems, PTHROTTLEITEM  pThrottleItems,
                                                         ULONG *pcItemsOut, PTHROTTLEITEM  *ppThrottleItemsOut,
                                                         DWORD dwMode, DWORD dwReserved)
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::RegisterThrottleNotificationType\n", this));
    HRESULT hr = NOERROR;

    BOOL fBroadcast = FALSE;

    do
    {
        if (   !cItems
            || !pThrottleItems
            || dwMode
            || dwReserved
            //|| pcItemsOut
            //|| ppThrottleItemsOut
            )
        {
            hr = E_INVALIDARG;
            break;
        }

        for (ULONG i = 0; i < cItems; i++)
        {
            if (!IsThrottleableNotificationType(pThrottleItems[i].NotificationType))
            {
                // can not throttle this notification
                hr = E_FAIL;
                i = cItems;
            }
            else if (pThrottleItems[i].nParallel < -1)
            {
                //  Invalid throttling count.
                hr = E_INVALIDARG;
                i = cItems;
            }
            else if (pThrottleItems[i].nParallel > THROTTLEITEM_MAX)
            {
                //  Invalid throttling count.
                hr = E_INVALIDARG;
                i = cItems;
            }
            //  REVIEW
            //  Looks like nParallel( = 0) is valid. It effectively
            //  UnRegistered the type!
            
            //  REVIEW
            //  What about invalid NotificationType?
        }

        BREAK_ONERROR(hr);

        CLockSmMutex lck(_Smxs);

        //
        // set the new settings
        //


        //  How could this used to work with multiple item case? No way!

        hr = NOERROR;
        for (ULONG n = 0; n < cItems; n ++)
        {
            LONG    posFound = -1, posVacant = -1;
            for (i = 0; i < g_urgThrottleItemtMax; i++)
            {
                if (g_rgThrottleItem[i].NotificationType
                                == pThrottleItems[n].NotificationType)
                {
                    posFound = i;
                    break;
                }
                else if (g_rgThrottleItem[i].nParallel == 0)
                {
                    if (posVacant == -1)
                        posVacant = i;
                }
            }

            if (posFound != -1)
            {
                i = posFound;
                if (pThrottleItems[n].nParallel != g_rgThrottleItem[posFound].nParallel)
                {
                    if ((pThrottleItems[n].nParallel == -1) || (g_rgThrottleItem[i].nRunning < pThrottleItems[n].nParallel))
                        fBroadcast = TRUE;
                    //  REVIEW. We are not suspending anything!
//                    else
//                        suspend those extra items?
                }
                g_rgThrottleItem[i].nParallel = pThrottleItems[n].nParallel;
                //  We should not nuke the count 'coz we could be in something
                //  and the count holds a valid non-zero value. We don't want
                //  to decrease it to something like -1 :) in RemoveFromThrList.
//              g_rgThrottleItem[i].nRunning = 0;
                g_rgThrottleItem[i].dwPriority = 0;
                g_rgThrottleItem[i].dwIntervalMin = pThrottleItems[n].dwMinItemUpdateInterval;
                g_rgThrottleItem[i].dateExcludeBegin = pThrottleItems[n].stBegin;
                g_rgThrottleItem[i].dateExcludeEnd = pThrottleItems[n].stEnd;
                g_rgThrottleItem[i].dwFlags = pThrottleItems[n].dwFlags;
            }
            else if (posVacant != -1)
            {
                i = posVacant;
                g_rgThrottleItem[i].NotificationType = pThrottleItems[n].NotificationType;
                g_rgThrottleItem[i].nParallel = pThrottleItems[n].nParallel;
                g_rgThrottleItem[i].dwFlags = pThrottleItems[n].dwFlags;
                g_rgThrottleItem[i].dwIntervalMin = pThrottleItems[n].dwMinItemUpdateInterval;
                g_rgThrottleItem[i].dateExcludeBegin = pThrottleItems[n].stBegin;
                g_rgThrottleItem[i].dateExcludeEnd = pThrottleItems[n].stEnd;
                g_rgThrottleItem[i].nRunning = 0;
                g_rgThrottleItem[i].dwPriority = 0;
            }
            else
            {
                //  no more vacancy?
                hr = S_FALSE;
                break;
            }
        }

        //  At this point, we can be only half way through.
//        NotfAssert((n == cItems));
        g_cThrottleItemCount = 0;
        for (i = 0; i < g_urgThrottleItemtMax; i++)
        {
            if (g_rgThrottleItem[i].nParallel)
                g_cThrottleItemCount ++;
        }
        break;
    } while (TRUE);

    if (fBroadcast)
    {
        // global broadcast
        WakeupAll();
    }

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::RegisterThrottleNotificationType (cPorts:%lx, hr:%lx)\n",this, g_cRefPorts, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::WakeupAll
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::WakeupAll()
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::WakeupAll\n", this));
    HRESULT hr = NOERROR;

    ULONG uHWndCount = 0;
    CDestinationPort *pDestPort= 0;

    hr = GetAllDestinationPorts(&pDestPort, &uHWndCount);

    if (hr == NOERROR)
    {
        for (ULONG i = 0; i < uHWndCount; i++)
        {
            HWND hWnd = (pDestPort+i)->GetPort();
            NotfAssert((hWnd));
            PostMessage(hWnd, WM_PROCESSWAKEUP, 0, 0);
        }
    }

    if (pDestPort)
    {
        delete [] pDestPort;
    }


    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::WakeupAll (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CGlobalNotfMgr::IsConnectedToInternet
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CGlobalNotfMgr::IsConnectedToInternet()
{
    NotfDebugOut((DEB_MGRGLOBAL, "%p _IN CGlobalNotfMgr::IsConnectedToInternet\n", this));
    HRESULT hr = S_FALSE;
    
    DWORD dwState = INTERNET_CONNECTION_MODEM | INTERNET_CONNECTION_LAN | INTERNET_CONNECTION_PROXY;

    if (InternetGetConnectedState(&dwState ,0) )
    {
        if (dwState & (INTERNET_CONNECTION_MODEM | INTERNET_CONNECTION_LAN | INTERNET_CONNECTION_PROXY))
        {
            hr = S_OK;
        }
    }

    NotfDebugOut((DEB_MGRGLOBAL, "%p OUT CGlobalNotfMgr::IsConnectedToInternet (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsThrottleableNotificationType
//
//  Synopsis:
//
//  Arguments:  [riid] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsThrottleableNotificationType(REFIID riid)
{
    return  (   (riid == NOTIFICATIONTYPE_AGENT_START)
             || (riid == NOTIFICATIONTYPE_START_1)
             || (riid == NOTIFICATIONTYPE_START_2)
             || (riid == NOTIFICATIONTYPE_START_3)
             || (riid == NOTIFICATIONTYPE_START_4)
             || (riid == NOTIFICATIONTYPE_START_5)
             || (riid == NOTIFICATIONTYPE_START_6)
             || (riid == NOTIFICATIONTYPE_START_7)
             || (riid == NOTIFICATIONTYPE_START_8)
             || (riid == NOTIFICATIONTYPE_START_9)
             || (riid == NOTIFICATIONTYPE_START_A)
             || (riid == NOTIFICATIONTYPE_START_B)
             || (riid == NOTIFICATIONTYPE_START_C)
             || (riid == NOTIFICATIONTYPE_START_D)
             || (riid == NOTIFICATIONTYPE_START_E)
             || (riid == NOTIFICATIONTYPE_START_F));
}

