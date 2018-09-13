//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       delagent.cxx
//
//  Contents:   the deliver agent implementation
//
//  Classes:
//
//  Functions:
//
//  History:    12-17-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>

// global variable used by the deliver agent
CMutexSem                               g_mxs;           // single access to all methods for now
CMap<THREADID , THREADID, HWND , HWND>  g_mapThreadIdToHwnd;
CMapCookieToCVal                        g_mapCookieToCDest;
CMapCookieToCVal                        g_mapClsToCDest;
CMapCookieToCVal                        g_mapPIDToCDest;

CDeliverAgentStart      *g_pDeliverAgent         = 0;
CDeliverAgentThread     *g_pDeliverAgentThread   = 0;
CDeliverAgentAsync      *g_pDeliverAgentAsync    = 0;
CDeliverAgentProcess    *g_pDeliverAgentProcess  = 0;
CDeliverAgentGlobal     *g_pDeliverAgentMachine  = 0;
CGlobalNotfMgr          *g_pGlobalNotfMgr        = 0;

#if DBG == 1
#undef  USE_NOTIFICATION_EXCEPTION_FILTER
#else
#undef  USE_NOTIFICATION_EXCEPTION_FILTER
//#define USE_NOTIFICATION_EXCEPTION_FILTER
#endif //


//
// findflags
//
// ,DM_ONLY_IF_NOT_PENDING     = 0x00001000
// 
#define FF_WITH_ACCEPT_ALL  0x00000001
#define FF_ONLY_RUNNING     0x00000002


//+---------------------------------------------------------------------------
//
//  Function:   NotificationMgrInvokeExceptionFilter
//
//  Synopsis:
//
//  Arguments:  [lCode] --
//              [lpep] --
//
//  Returns:
//
//  History:    2-14-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LONG NotificationMgrInvokeExceptionFilter( DWORD lCode, LPEXCEPTION_POINTERS lpep )
{
    #if DBG == 1
    NotfDebugOut((DEB_DELIVER,  "XXX NotificationMgrInvokeExceptionFilter Exception 0x%x at address 0x%x",
                                lCode, lpep->ExceptionRecord->ExceptionAddress));
    
    NotfAssert((FALSE));
    #endif

    return EXCEPTION_EXECUTE_HANDLER;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetDeliverAgentStart
//
//  Synopsis:   creates a deliver agent
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
CDeliverAgentStart * GetDeliverAgentStart(BOOL fAddRef)
{
    NotfDebugOut((DEB_MGRGLOBAL, "API _IN GetDeliverAgent\n"));

    if (!g_pDeliverAgentThread)
    {
        g_pDeliverAgentThread   = new CDeliverAgentThread     ();
    }

    if (!g_pDeliverAgentAsync)
    {
        g_pDeliverAgentAsync    = new CDeliverAgentAsync      ();
    }

    if (!g_pDeliverAgentProcess)
    {
        g_pDeliverAgentProcess  = new CDeliverAgentProcess    ();
    }

    if (!g_pDeliverAgentMachine)
    {
        g_pDeliverAgentMachine  = new CDeliverAgentGlobal    ();
    }

    if (!g_pGlobalNotfMgr)
    {
        g_pGlobalNotfMgr  = new CGlobalNotfMgr    ();
    }

    if (!g_pDeliverAgent)
    {
        g_pDeliverAgent = new CDeliverAgentStart();
    }


    if (   g_pDeliverAgentThread
        && g_pDeliverAgentAsync
        && g_pDeliverAgentProcess
        && g_pDeliverAgentMachine
        && g_pGlobalNotfMgr
        && g_pDeliverAgent)
    {
        if (fAddRef)
        {
            g_pDeliverAgent->AddRef();
        }
    }
    else
    {

        // failed - delete what was created

        if (!g_pDeliverAgent)
        {
            delete g_pDeliverAgent;
            g_pDeliverAgent = 0;
        }

        if (g_pDeliverAgentThread)
        {
            delete g_pDeliverAgentThread;
            g_pDeliverAgentThread = 0;
        }
        if (g_pDeliverAgentProcess)
        {
            delete g_pDeliverAgentProcess;
            g_pDeliverAgentProcess = 0;
        }
        if (!g_pGlobalNotfMgr)
        {
            delete g_pGlobalNotfMgr;
            g_pGlobalNotfMgr = 0;
        }

        if (g_pDeliverAgentAsync)
        {
            delete g_pDeliverAgentAsync;
            g_pDeliverAgentAsync = 0;
        }
    }

    NotfDebugOut((DEB_MGRGLOBAL, "API OUT GetDeliverAgent (pDelAgentStart:%p)\n", g_pDeliverAgent));
    return g_pDeliverAgent;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgent::Register
//
//  Synopsis:
//
//  Arguments:  [pNotificationSink] --
//              [pDestID] --
//              [asMode] --
//              [cNotifications] --
//              [pNotificationIDs] --
//              [pRegisterCookie] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgent::Register(
    LPNOTIFICATIONSINK  pNotificationSink,      // can be null - see mode
    LPDESTID            pDestID,
    NOTFSINKMODE        asMode,
    ULONG               cNotifications,
    PNOTIFICATIONTYPE   pNotificationIDs,
    PNOTIFICATIONCOOKIE pRegisterCookie,
    DWORD               dwReserved
    )
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgent::Register\n", this));
    HRESULT hr = NOERROR;

    CDestination *pCDestination = 0;
    PNOTIFICATIONTYPE   pNotTypes = 0;

    CThreadId  cthreadId;
    HWND hwnd = 0;

    do
    {
        CLock lck(_mxs);

        if (   !pRegisterCookie
            || (asMode & ~NOTFSINKMODE_ALL))
        {
            hr = E_INVALIDARG;
            break;
        }
        // create a new cookie and pass it back!
        CPkgCookie ccookie;
        *pRegisterCookie = ccookie;

        if (cNotifications)
        {
            pNotTypes = new NOTIFICATIONTYPE [cNotifications];
        }

        if (!pNotTypes)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        #if 0
        need to check if already registered
        if (pDestID)
        {
            CPkgCookie ccookieCls = *pDestID;
            hr = _mapClsToCDest.FindFirst(&ccookieCls, (CObject *&)pCDestination);

            if (hr == NOERROR)
            {
                NotfAssert((pCDestination));
            }
            NotfAssert((hr != NOERROR));
            
        }
        #endif // 0

        if (cNotifications)
        {
            memcpy(pNotTypes,pNotificationIDs, sizeof(NOTIFICATIONTYPE) * cNotifications);
        }

        pCDestination = new CDestination(pNotificationSink,pDestID,asMode,ccookie,cNotifications,pNotTypes,0);

        if (!pCDestination)
        {

            if (pNotTypes)
            {
                delete pNotTypes;
            }
            hr = E_OUTOFMEMORY;
            break;
        }

        // check if thread window is registered
        THREADID threadId = GetCurrentThreadId();
        hwnd = GetThreadNotificationWnd();
        _mapThreadIdToHwnd.SetAt(threadId, hwnd);

        // add the destination to the registercookie list
        hr = _mapCookieToCDest.AddVal(&ccookie,pCDestination);
        if (hr != NOERROR)
        {
            break;
        }

        // add it to each package list
        for (ULONG i = 0; i < cNotifications; i++)
        {
            CPkgCookie ccookieNot = *(pNotificationIDs+i);

            //DumpIID(ccookieNot);
            if (ccookieNot == NOTIFICATIONTYPE_ALL)
            {
                // remember that this wants all
                pCDestination->SetDestMode(NM_ACCEPT_ALL);
            }

            hr = _mapPIDToCDest.AddVal(&ccookieNot, pCDestination);
            if (hr != NOERROR)
            {
                break;
            }
        }

        if (pDestID)
        {
            CPkgCookie ccookieCls = *pDestID;

            hr = _mapClsToCDest.AddVal(&ccookieCls,pCDestination);
        }

        if (asMode & NM_PERMANENT)
        {
            //
            // persist the destination
            hr = pCDestination->SaveToPersist(c_pszRegKeyDestination);
        }

        CGlobalNotfMgr *pGlobalNofMgr =  GetGlobalNotfMgr();

        NotfAssert((pGlobalNofMgr));
        CDestinationPort cdestPort(hwnd);
        hr = pGlobalNofMgr->AddDestinationPort(cdestPort, pCDestination);

        break;

    } while (TRUE);


    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgent::Register (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgent::Unregister
//
//  Synopsis:
//
//  Arguments:  [pDestID] --
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
HRESULT CDeliverAgent::Unregister(PNOTIFICATIONCOOKIE pRegisterCookie)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgent::Unregister\n", this));
    HRESULT hr = NOERROR;

    CDestination *pCDestination = 0;

    do
    {
        CLock lck(_mxs);

        if (!pRegisterCookie)
        {
            hr = E_INVALIDARG;
            break;
        }
        CPkgCookie ccookie = *pRegisterCookie;

        hr = _mapCookieToCDest.FindFirst(&ccookie,(CObject *&)pCDestination);
        if (hr != NOERROR)
        {
            break;
        }
        LPDESTID pDestID = pCDestination->GetDestId();
        ULONG             cNotifications = pCDestination->GetNotfTypeCount();
        PNOTIFICATIONTYPE pNotificationIDs = pCDestination->GetNotfTypes();

        hr = _mapCookieToCDest.RemoveVal(&ccookie,pCDestination);
        if (hr != NOERROR)
        {
            break;
        }
        NotfAssert((pCDestination));
        // add it to each package list
        for (ULONG i = 0; i < cNotifications; i++)
        {
            CPkgCookie ccookieNot =  *(pNotificationIDs+i);

            hr = _mapPIDToCDest.RemoveVal(&ccookieNot, pCDestination);
            if (hr != NOERROR)
            {
                break;
            }
        }

        if (pDestID)
        {
            CPkgCookie ccookieCls = *pDestID;
            hr = _mapClsToCDest.RemoveVal(&ccookieCls,pCDestination);
        }

        if (hr != NOERROR)
        {
            break;
        }

        if (pCDestination->IsDestMode(NM_PERMANENT))
        {
            //
            // remove the persisted destination
            hr = pCDestination->RemovePersist(c_pszRegKeyDestination);
        }


        CGlobalNotfMgr *pGlobalNofMgr =  GetGlobalNotfMgr();

        NotfAssert((pGlobalNofMgr));
        CDestinationPort cdestPort(GetThreadNotificationWnd());
        HRESULT hr1 = pGlobalNofMgr->RemoveDestinationPort(cdestPort, pCDestination);

        NotfAssert((pCDestination));
        pCDestination->Release();

        break;
    } while (1);

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgent::Unregister (hr:%lx)\n",this, hr));
    return hr;
}

//
// internal methods
//
//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgent::FindFirst
//
//  Synopsis:
//
//  Arguments:  [rNotificationDest] --
//              [rNotfType] --
//              [rValue] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgent::FindFirst(REFDESTID rNotificationDest,
                                 REFNOTIFICATIONTYPE rNotfType,
                                 INotificationSink*& rValue,
                                 NOTFSINKMODE NotfSinkMode,
                                 PNOTIFICATIONCOOKIE pPosCookie,
                                 DWORD dwMode
                                 )
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgent::FindFirst\n", this));
    HRESULT hr = S_OK;
    CLock lck(_mxs);

    CDestination *pCDestination = 0;

    //
    // find the first destination of this thread
    //
    if (   (rNotificationDest == CLSID_THREAD_BROADCAST)
        || (rNotificationDest == CLSID_PROCESS_BROADCAST)
        || (rNotificationDest == CLSID_GLOBAL_BROADCAST))
    {
        if (   (pPosCookie)
            && (*pPosCookie == NOTIFICATIONTYPE_ALL) )
        {
            //
            // look for sink registered for all notifications
            CPkgCookie ccookieAll = NOTIFICATIONTYPE_ALL;

            // multicast - find first destination on this thread
            hr = _mapPIDToCDest.FindFirst(&ccookieAll, (CObject *&)pCDestination);

            while (   (hr == NOERROR)
                   && (!pCDestination->IsApartmentThread()) )
            {
                hr = _mapPIDToCDest.FindNext(&ccookieAll, (CObject *&)pCDestination);
                NotfAssert((   ((hr == NOERROR) && pCDestination)
                             || ((hr != NOERROR) && !pCDestination)  ));
            }

            if (hr == NOERROR)
            {
                // found a destination - remember the position
                *pPosCookie = NOTIFICATIONTYPE_ALL;
            }
        }
        else
        {
            hr = E_FAIL;
        }

        if (hr != NOERROR)
        {
            // this is the broad cast case
            CPkgCookie ccookieNot = rNotfType;
            *pPosCookie = COOKIE_NULL;


            // multicast - find first destination on this thread
            hr = _mapPIDToCDest.FindFirst(&ccookieNot, (CObject *&)pCDestination);

            while (   (hr == NOERROR)
                   && (   (!pCDestination->IsApartmentThread()) 
                       || (   (dwMode & FF_WITH_ACCEPT_ALL) 
                           && pCDestination->IsDestMode(NM_ACCEPT_ALL)) ) )
            {
                hr = _mapPIDToCDest.FindNext(&ccookieNot, (CObject *&)pCDestination);
                NotfAssert((   ((hr == NOERROR) && pCDestination)
                             || ((hr != NOERROR) && !pCDestination)  ));
            }
        }
    }
    else
    {
        //
        // normal case - destination is one particular clsid
        //

        // check if the class actually is registered
        CPkgCookie ccookieCls = rNotificationDest;

        hr = _mapClsToCDest.FindFirst(&ccookieCls, (CObject *&)pCDestination);
        // BUGBUG: will be send to the first CDestination
        NotfAssert((   ((hr == S_OK) && rNotificationDest == *pCDestination->GetDestId())
                     || (hr != S_OK) ));

        if (   (hr == S_OK) 
            && NotfSinkMode    
            && !(pCDestination->IsDestMode(NotfSinkMode)))
        {
            hr = E_FAIL;
        }
    }

    // get the destination
    if (hr == S_OK)
    {
        NotfAssert((pCDestination));
        hr= pCDestination->GetAcceptor(rValue);
    }
    //
    // did not get the notificationsink yet
    // check if it should be loaded
    //
    if (   (hr != NOERROR)
        && (pCDestination) )
    {
        NotfAssert(( pCDestination->GetDestId() ));

        //
        // BUGBUG: need to figure out how long we keep this destination loaded
        //
        if ( !(dwMode & FF_ONLY_RUNNING))
        {
            hr = CreateDestination(*pCDestination->GetDestId(), rValue);
        }

        if (   (hr == NOERROR)
            //&& (!pCDestination->IsDestMode(NM_Instantiate))
            && !(NotfSinkMode & NM_ACCEPT_DIRECTED_NOTIFICATION)
            )
        {
            // the destination wants to be instantiated every time
            // do not hold on to the pointer
            hr = pCDestination->SetNotificationSink(rValue);
        }
    }

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgent::FindFirst (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgent::FindNext
//
//  Synopsis:
//
//  Arguments:  [rNotificationDest] --
//              [rNotfType] --
//              [rValue] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgent::FindNext(REFDESTID rNotificationDest,
                                REFNOTIFICATIONTYPE rNotfType,
                                INotificationSink*& rValue,
                                NOTFSINKMODE NotfSinkMode,
                                PNOTIFICATIONCOOKIE pPosCookie,
                                DWORD dwMode
                                )
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgent::FindNext\n", this));
    HRESULT hr = S_OK;
    CLock lck(_mxs);

    CDestination *pCDestination = 0;

    if (   (rNotificationDest == CLSID_THREAD_BROADCAST)
        || (rNotificationDest == CLSID_PROCESS_BROADCAST)
        || (rNotificationDest == CLSID_GLOBAL_BROADCAST))
    {


        if (    pPosCookie
            && (*pPosCookie == NOTIFICATIONTYPE_ALL))
        {
            // find the next desitination registered for all notifications
            CPkgCookie ccookieAll = NOTIFICATIONTYPE_ALL;

            // multicast - find first destination on this thread
            hr = _mapPIDToCDest.FindNext(&ccookieAll, (CObject *&)pCDestination);

            while (   (hr == NOERROR)
                    && (!pCDestination->IsApartmentThread()) )
            {
                hr = _mapPIDToCDest.FindNext(&ccookieAll, (CObject *&)pCDestination);
                NotfAssert((   ((hr == NOERROR) && pCDestination)
                             || ((hr != NOERROR) && !pCDestination)  ));
            }

            if (hr != NOERROR)
            {
                // reset the cookie position and try to find
                // the first NONE-ALL types destination
                *pPosCookie = COOKIE_NULL;
                dwMode |= FF_WITH_ACCEPT_ALL;
                hr = FindFirst(rNotificationDest, rNotfType, rValue, NotfSinkMode, pPosCookie, dwMode);
            }
        }
        else
        {
            // multicast
            CPkgCookie ccookieNot = rNotfType;

            // multicast
            hr = _mapPIDToCDest.FindNext(&ccookieNot, (CObject *&)pCDestination);

            while (   (hr == NOERROR)
                   && (!pCDestination->IsApartmentThread()) )
            {
                hr = _mapPIDToCDest.FindNext(&ccookieNot, (CObject *&)pCDestination);

                NotfAssert((   ((hr == NOERROR) && pCDestination)
                             || ((hr != NOERROR) && !pCDestination)  ));

            }
        }

        if (hr == S_OK && pCDestination)
        {
            //NotfAssert((pCDestination));
            hr = pCDestination->GetAcceptor(rValue);
        }

        //
        // did not get the notificationsink yet
        // check if it should be loaded
        //
        if (   (hr != NOERROR)
            && (pCDestination))
        {
            NotfAssert(( pCDestination->GetDestId() ));

            //
            // BUGBUG: need to figure out how long we keep this destination loaded
            //
            
            if ( !(dwMode & FF_ONLY_RUNNING))
            {
                hr = CreateDestination(*pCDestination->GetDestId(), rValue);
            }

            if (   (hr == NOERROR)
                //&& (!pCDestination->IsDestMode(NM_Instantiate))
               )
            {
                // the destination wants to be instantiated every time
                // do not hold on to the pointer
                hr = pCDestination->SetNotificationSink(rValue);
            }
        }

    }
    else
    {
        // fail for non multicast
        hr = E_FAIL;
    }

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgent::FindNext (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgent::CreateDestination
//
//  Synopsis:   created a destination by calling CoCreateInstance
//
//  Arguments:  [rNotificationDest] --
//              [rValue] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgent::CreateDestination(REFDESTID rNotificationDest, INotificationSink*& rValue)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgent::CreateDestination\n", this));
    HRESULT hr = S_OK;

    REFIID riidResult = IID_INotificationSink;
    IUnknown *pUnk = 0;
    INotificationSink *pNotfSink = 0;

    hr = CoCreateInstance(rNotificationDest, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pUnk);

    if (hr == NOERROR)
    {
        hr = pUnk->QueryInterface(riidResult, (void **) &pNotfSink);
    }

    if (FAILED(hr))
    {
        NotfDebugOut((DEB_MGR | DEB_DELIVER | DEB_TRACE, "%p === CDeliverAgent::InstantiateObject InProcServer (hr:%lx) \n", this, hr));
        hr = CoCreateInstance(rNotificationDest, NULL, CLSCTX_LOCAL_SERVER, IID_IUnknown, (void**)&pUnk);

        if (FAILED(hr))
        {
            DumpIID(rNotificationDest);
            NotfDebugOut((DEB_MGR | DEB_DELIVER | DEB_TRACE, "%p === CDeliverAgent::InstantiateObject LocalServer (hr:%lx) \n", this, hr));
        }
        else
        {
            hr = pUnk->QueryInterface(riidResult, (void **) &pNotfSink);
        }
        
    }
    DumpIID(rNotificationDest);

    if (hr == NOERROR)
    {
        NotfAssert((pUnk && pNotfSink));
        //rValue = (INotificationSink *)pUnk;
        rValue = (INotificationSink *)pNotfSink;
    }

    if (pUnk)
    {
        pUnk->Release();
    }

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgent::CreateDestination (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
 //
//  Method:     CDeliverAgent::HandlePackage
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
HRESULT CDeliverAgentStart::HandlePackage(CPackage *pCPackage)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgent::HandlePackage\n", this));
    HRESULT hr;
    CLock lck(_mxs);

    NotfAssert((pCPackage));
    CDeliverAgent *pCourierAgent = 0;

    do
    {
        hr = FindDeliverAgent(pCPackage, &pCourierAgent, 0);

        if (hr == NOERROR)
        {
            NotfAssert((pCourierAgent));
            hr = pCourierAgent->HandlePackage(pCPackage);
        }
        
        break;
    } while (TRUE);

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgent::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+--------- ------------------------------------------------------------------
//
//  Method:     CDeliverAgentStart::FindDeliverAgent
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//              [ppCourierAgent] --
//              [dwFlags] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentStart::FindDeliverAgent(
    CPackage              *pCPackage,
    CDeliverAgent       **ppCourierAgent,
    DWORD                 dwFlags
    )
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgent::FindDeliverAgent\n", this));
    HRESULT hr = NOERROR;

    NotfAssert((pCPackage));
    NotfAssert((ppCourierAgent));
    *ppCourierAgent = 0;

    BOOL fSynchronous = pCPackage->IsSynchronous();
    BOOL fBroadcast   = pCPackage->IsBroadcast();
    BOOL fReport      = pCPackage->IsReport();
    BOOL fXProcess    = (pCPackage->GetNotificationState() & PF_CROSSPROCESS);
    BOOL fDefaultProcess = FALSE;


    DELIVERMODE delMode = pCPackage->GetDeliverMode();
    CDestination *pCDestination = 0;
    CDestinationPort cDefDest;

    if (pCPackage->IsForDefaultProcess())
    {
        NotfAssert((g_pGlobalNotfMgr));
        hr = g_pGlobalNotfMgr->GetDefaultDestinationPort(&cDefDest);
        if (hr == S_OK)
        {
            NotfAssert(( cDefDest.GetDestThreadId() ));
            fDefaultProcess = TRUE;
            pCDestination = 0;
        }
    }

    if (fDefaultProcess  || fXProcess)
    {
        //
        // CASE: deliver to the DEFAULT PROCESS
        //

        // used the global DeliverAgent
        NotfAssert((_pDeliverAgentGlobal));
        // the process agent takes of delivering to other processes as well
        *ppCourierAgent = _pDeliverAgentGlobal;
        hr = NOERROR;
    }
    else if (fReport)
    {
        //
        // CASE: REPORT NOTIFICATION
        //

        THREADID threadId = pCPackage->GetThreadId();
        NotfAssert((threadId));

        // set the destination thread id of the thread where the sink is
        //pCPackage->SetThreadId(GetCurrentThreadId());

        // deliver with async/sync agent
        NotfAssert((_pDeliverAgentAsync));
        *ppCourierAgent = _pDeliverAgentAsync;
        hr = NOERROR;

    }
    else if (fSynchronous && fBroadcast)
    {
        //
        // CASE: SYNCHRONOUS BROADCAST
        //

        // we do not allow synchronous broadcast
        hr = E_INVALIDARG;
    }
    else if (fSynchronous)
    {
        //
        // CASE: SYNCHRONOUS TO ONE PARTICULAR DESTINATION
        //

        HRESULT hrReg = GetRegisteredDestination(pCPackage, &pCDestination);

        // BUGBUG: we only allow sync if registered yet
        // also need to check if running global
        if (   (delMode & DM_ONLY_IF_RUNNING)
            && (hrReg != NOERROR))
        {
            // the destination class object is not registered
            hr = CLASS_E_CLASSNOTAVAILABLE;
        }
        else if (hrReg == NOERROR)
        {
            //
            // destination is registered - deliver it with
            // the async courier to the correct thread
            //
            NotfAssert((pCDestination));
            NotfAssert(( pCDestination->GetThreadId() ));

            // set the destination thread id
            pCPackage->SetThreadId(pCDestination->GetThreadId());

            // deliver with async/sync agent
            NotfAssert((_pDeliverAgentAsync));
            *ppCourierAgent = _pDeliverAgentAsync;
            hr = NOERROR;
        }
        #ifdef _sync_deliver_
        else
        {
            //
            // not registered - use the async courier which
            // will try to cocreate it later
            //
            NotfAssert((!pCDestination));

            // set the thread id of the current thread
            pCPackage->SetThreadId(GetCurrentThreadId());

            // deliver with async/sync agent
            NotfAssert((_pDeliverAgentAsync));
            *ppCourierAgent = _pDeliverAgentAsync;
            hr = NOERROR;
        }
        #else   // !_sync_deliver_
        else
        {
            CGlobalNotfMgr *pGlbNotfMgr = GetGlobalNotfMgr();

            NotfAssert((pGlbNotfMgr));
            
            hr = pGlbNotfMgr->LookupDestination(pCPackage, &pCDestination,0);
            
            if (hr == NOERROR)
            {
                NotfAssert((pCDestination));
                pCDestination->Release();
                // check if the destination is registered globally
                NotfAssert((_pDeliverAgentGlobal));
                // the process agent takes of delivering to other processes as well
                *ppCourierAgent = _pDeliverAgentGlobal;
                hr = NOERROR;
            }
            else
            {
                NotfAssert((!pCDestination));
                pCPackage->SetThreadId(GetCurrentThreadId());
                NotfAssert(( IsWindow( GetThreadNotificationWnd() ) ));

                // deliver with async/sync agent
                NotfAssert((_pDeliverAgentAsync));
                *ppCourierAgent = _pDeliverAgentAsync;
                hr = NOERROR;

            }
            
        }
        #endif //!_sync_deliver_
    }
    else if (fBroadcast)
    {
        //
        // CASE: BROADCAST
        //

        // broadcast is always async - should we allow sync (not inputsync)
        REFDESTID   rPackageDest = pCPackage->GetDestID();

        if (CLSID_THREAD_BROADCAST == rPackageDest)
        {
            // set the thread id of the current thread
            pCPackage->SetThreadId(GetCurrentThreadId());

            NotfAssert((_pDeliverAgentAsync));
            *ppCourierAgent = _pDeliverAgentAsync;
        }
        else if (CLSID_PROCESS_BROADCAST == rPackageDest)
        {
            NotfAssert((_pDeliverAgentProcess));
            // the process agent takes of delivering to other processes as well
            *ppCourierAgent = _pDeliverAgentProcess;
        }
        else if (CLSID_GLOBAL_BROADCAST == rPackageDest)
        {
            NotfAssert((_pDeliverAgentGlobal));
            // the process agent takes of delivering to other processes as well
            *ppCourierAgent = _pDeliverAgentGlobal;
        }
        else
        {
            NotfAssert((FALSE));
        }
    }
    else
    {
        //
        // CASE: DELIVER TO ONE PARTICULAR DESTINATION
        //

        hr = GetRegisteredDestination(pCPackage, &pCDestination, DF_VERIFY);
        if (hr == NOERROR)
        {
            NotfAssert((pCDestination));
            NotfAssert(( pCDestination->GetThreadId() ));

            // set the destination thread id
            pCPackage->SetThreadId(pCDestination->GetThreadId());

            // deliver with async/sync agent
            NotfAssert((_pDeliverAgentAsync));
            *ppCourierAgent = _pDeliverAgentAsync;
        }
        else
        {
            // now check if the destination is globally
            // registered

            CDestination *pDestination = 0;

            CGlobalNotfMgr *pGlobalNofMgr =  GetGlobalNotfMgr();

            NotfAssert((pGlobalNofMgr));
            hr = pGlobalNofMgr->LookupDestination(pCPackage, &pDestination,0);

            if (hr == NOERROR)
            {
                NotfAssert((pDestination));
                NotfAssert((_pDeliverAgentGlobal));
                // the process agent takes of delivering to other processes as well
                *ppCourierAgent = _pDeliverAgentGlobal;
                pDestination->Release();
                pDestination = 0;
            }
            else
            {
                if (delMode & DM_ONLY_IF_RUNNING)
                {
                    // the destination class object is not registered
                    hr = CLASS_E_CLASSNOTAVAILABLE;
                }
                else
                {
                    NotfAssert((pCDestination == 0));
                    // set the destination thread id for the current thread
                    pCPackage->SetThreadId(GetCurrentThreadId());
                    
                    NotfAssert(( IsWindow( GetThreadNotificationWnd() ) ));

                    // deliver with async/sync agent
                    NotfAssert((_pDeliverAgentAsync));
                    *ppCourierAgent = _pDeliverAgentAsync;
                    hr = NOERROR;
                }
            }
        }
    }

    if (*ppCourierAgent)
    {
        (*ppCourierAgent)->AddRef();
    }

    NotfAssert((   ((hr == NOERROR) && (*ppCourierAgent))
                 || ((hr != NOERROR) && (!*ppCourierAgent)) ));

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgent::FindDeliverAgent (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgentStart::GetRegisteredDestination
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentStart::GetRegisteredDestination(CPackage *pCPackage, CDestination **ppCDestination, DWORD dwMode)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgentStart::GetRegisteredDestination\n", this));
    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage && ppCDestination));

    CDestination *pCDestination = 0;
    // check if the class actually is registered
    CPkgCookie ccookieCls = pCPackage->GetDestID();

    hr = _mapClsToCDest.FindFirst(&ccookieCls, (CObject *&)pCDestination);

    do
    {
        if (hr != NOERROR)
        {
            break;
        }
        
        if (!(dwMode & DF_VERIFY))
        {
            *ppCDestination = pCDestination;
            break;
        }
        hr = VerifyDestination(pCDestination, 0);

        if (hr == NOERROR)
        {
            *ppCDestination = pCDestination;
            break;
        }
        
        // find the first notification with the same clsid 
        hr = _mapClsToCDest.FindFirst(&ccookieCls, (CObject *&)pCDestination);

        // loop again
        
    } while (TRUE);

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgentStart::GetRegisteredDestination (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgentThread::HandlePackage
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentThread::HandlePackage(CPackage *pCPackage)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgentThread::HandlePackage\n", this));
    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage));
    NotfAssert((pCPackage->GetNotification()));

    BOOL fBroadcast = pCPackage->IsBroadcast();
    NOTIFICATIONCOOKIE posCookie = NOTIFICATIONTYPE_ALL;
    PNOTIFICATIONCOOKIE pPosCookie =  (fBroadcast) ? &posCookie : NULL;
    DWORD dwFindMode = (pCPackage->GetDeliverMode() & DM_ONLY_IF_RUNNING) ? FF_ONLY_RUNNING : 0;

    ADDREF(pCPackage);

    // deliver the package
    INotificationSink  *pNotificationSink = 0;
    do
    {
    
        if (pCPackage->IsDeliverIfConnectToInternet())
        {
            CGlobalNotfMgr *pGlNotfMgr = GetGlobalNotfMgr();
            
            NotfAssert((pGlNotfMgr));
            
            if (pGlNotfMgr->IsConnectedToInternet() != S_OK)
            {
                // do not deliver this package        
                break;
            }
        }

        if (pCPackage->IsUserIdle())
        {
            // got a user idle notification
            // wake up the idle list agent
            CThrottleListAgent *pDelAgent = GetThrottleListAgent();
            if (pDelAgent)
            {
                pCPackage->SetThreadId(GetCurrentThreadId());
                hr = pDelAgent->OnWakeup(WT_NONE, pCPackage);
            }
            // done
            break;
        }

        if (pCPackage->IsReport() )
        {
            PACKAGE_TYPE packagetype = pCPackage->GetPackageType();
            NotfAssert(( (packagetype == PT_REPORT_TO_DEST) || (packagetype == PT_REPORT_TO_SENDER) ));

            pNotificationSink = pCPackage->GetNotificationSinkDest(TRUE);
            
            if (!pNotificationSink)
            {
                //  XProcess case.
                //      Following assert is not always valid.
                //  NotfAssert(( pCPackage->GetRunningCookie() ));

                CPackage *pCPkg = NULL;
                NOTIFICATIONCOOKIE * pBaseCookie = pCPackage->GetBaseCookie();
                NotfAssert(pBaseCookie);
                hr = E_FAIL;

                //  We should firt try to find the running package in the
                //  throttle list.

                CThrottleListAgent *pThrottleAgnt = GetThrottleListAgent();
                NotfAssert(pThrottleAgnt);

                hr = pThrottleAgnt->FindPackage(pBaseCookie,
                                                &pCPkg,
                                                LM_LOCALCOPY);

                if ((NOERROR != hr) || !pCPkg)   {
                    if (pCPkg)  {
                        pCPkg->Release();
                        pCPkg = NULL;
                    }
                    CSchedListAgent *pListAgent = GetScheduleListAgent();
                    NotfAssert((pListAgent));
                    hr = pListAgent->FindPackage(pBaseCookie,
                                                &pCPkg,
                                                LM_LOCALCOPY);
                }

                if (hr == NOERROR)
                {
                    NotfAssert((pCPkg));

                    if ((packagetype == PT_REPORT_TO_DEST))
                    {
                        pNotificationSink = pCPkg->GetNotificationSinkDest(TRUE);

                        //  Why do we create a new Sender instance below but not                        //  check anything here?
                    }
                    else
                    {
                        pNotificationSink = pCPkg->GetNotificationSinkSender(TRUE);
        
                        if (!pNotificationSink)
                        {
                            if (pCPkg->GetPackageContent() & PC_CLSIDSENDER)
                            {
                                REFCLSID rclsid = pCPkg->GetClassIdSender();
                            
                                hr = CreateDestination(rclsid, pNotificationSink);
                                if (hr == NOERROR)
                                {
                                    pCPkg->SetNotificationSinkSender(pNotificationSink);
                                }
                            } else  {
                                NotfAssert(FALSE);
                            }
                        }
                    }

                    RELEASE(pCPkg);
                }
            }

            NotfAssert(( !(pCPackage->GetDeliverMode() & DM_NEED_COMPLETIONREPORT) ));

            if (pNotificationSink)
            {
                hr = NOERROR;
            }
            else
            {
                hr = E_FAIL;
            }
        }

        if (   (!pNotificationSink)
            && (pCPackage->GetPackageContent() & PC_CLSIDDEST) )
        {
            REFDESTID           rclsDest = pCPackage->GetDestID();
            NOTIFICATIONTYPE    PackageID = pCPackage->GetNotificationID();

            // bugbug: the nutshell test cases have to be fixed before turning on the correct
            // NM_ACCEPT_DIRECTED_NOTIFICATION behaviour!!
            NOTFSINKMODE        NotfSinkMode = (fBroadcast) ? 0 : NM_ACCEPT_DIRECTED_NOTIFICATION;
            //NOTFSINKMODE        NotfSinkMode = 0; 
            
            hr = FindFirst(rclsDest,PackageID, pNotificationSink, NotfSinkMode,pPosCookie,dwFindMode);
        }

        if (   pCPackage->IsReport() 
            && (hr != NOERROR))
        {
            // sink not found - might have shut down in meanwhile
            break;
        }

        if (hr == NOERROR)
        {
            //
            // ok, got the first destination
            //
            HRESULT hrNext = E_FAIL;

            do
            {
                NotfAssert((pNotificationSink));
                NotfAssert((pCPackage->GetNotification()));
                NotfDebugOut((DEB_DELIVER, "%p >>> NotificationSink:: OnNotification\n",pNotificationSink));

                hr = DispatchOnNotification(pCPackage, pNotificationSink);
                TNotfDebugOut((DEB_TFLOW, "%p CDeliverAgentThread::HandlePackage after DispatchOnNotification1 (hr:%lx)\n", pCPackage, hr));

                NotfDebugOut((DEB_DELIVER, "%p === CDeliverAgentThread:: Release Sink:(hr:%lx)\n",this, pNotificationSink));
                INotificationSink  *pNotificationSinkNext = 0;
                if (pCPackage->IsBroadcast())
                {
                    REFDESTID           rclsDest = pCPackage->GetDestID();
                    NOTIFICATIONTYPE    PackageID = pCPackage->GetNotificationID();

                    hrNext = FindNext(rclsDest,PackageID, pNotificationSinkNext, 0, pPosCookie, dwFindMode);
                }

                if (pNotificationSinkNext == pNotificationSink)
                {
                    pNotificationSinkNext->Release();
                    hrNext = E_FAIL;
                    pNotificationSink->Release();
                    pNotificationSink = 0;
                    pNotificationSinkNext = 0;
                }
                else
                {
                    pNotificationSink->Release();
                    pNotificationSink = pNotificationSinkNext;
                    pNotificationSinkNext = 0;
                }
                    
            } while ((hr == NOERROR) && (hrNext == NOERROR)) ;

        }
        else if (!pCPackage->IsBroadcast() )
        {
            //
            // not registered - no broadcast
            //
            REFDESTID   rclsDest = pCPackage->GetDestID();

            NotfAssert(( !pCPackage->IsBroadcast() ));

            if (! (dwFindMode & FF_ONLY_RUNNING))
            {
                hr = CreateDestination(rclsDest, pNotificationSink);
            }
            if (hr == NOERROR)
            {
                NotfAssert((pNotificationSink));
                NotfAssert((pCPackage->GetNotification()));
                NotfDebugOut((DEB_DELIVER, "%p >>> NotificationSink:: OnNotification\n",pNotificationSink));

                hr = DispatchOnNotification(pCPackage, pNotificationSink);
                TNotfDebugOut((DEB_TFLOW, "%p CDeliverAgentThread::HandlePackage after DispatchOnNotification2 (hr:%lx)\n", pCPackage, hr));

                NotfDebugOut((DEB_DELIVER, "%p === CDeliverAgentThread:: Release Sink:(hr:%lx)\n",this, pNotificationSink));
                pNotificationSink->Release();
                pNotificationSink = 0;
            }
        }

        break;
    } while (TRUE);

    RELEASE(pCPackage);

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgentThread::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgentAsync::HandlePackage
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentAsync::HandlePackage(CPackage *pCPackage)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgentAsync::HandlePackage\n", this));
    HRESULT hr = E_FAIL;
    BOOL fCleanup = FALSE;
    NotfAssert((pCPackage));
    NotfAssert((pCPackage->GetNotification()));


    // the CPackage needs to carry the destination hwnd
    THREADID threadId = pCPackage->GetThreadId();
    HWND hwndDestination = 0;
    NotfAssert((threadId));

    if (threadId == GetCurrentThreadId())
    {
        hwndDestination = GetThreadNotificationWnd();
        
        NotfAssert(( IsWindow( hwndDestination ) ));
        hr = pCPackage->Deliver(hwndDestination, 0);
    }
    else if (_mapThreadIdToHwnd.Lookup(threadId, hwndDestination))
    {
        NotfAssert(( IsWindow(hwndDestination) ));

        NotfAssert((hwndDestination));
        hr = pCPackage->Deliver(hwndDestination, 0);
    }
    else
    {
        if (pCPackage->IsReport() )
        {
            NotfAssert((FALSE && "Report package without valid port (hwnd)!"));
        }

        // this thread does not have a window yet
        hwndDestination = GetThreadNotificationWnd();
        // we should always get an hwnd - assert but fail
        NotfAssert((hwndDestination));
        if (hwndDestination)
        {
            if (IsWindow(hwndDestination) )
            {
                _mapThreadIdToHwnd.SetAt(threadId, hwndDestination);
                hr = pCPackage->Deliver(hwndDestination, 0);
            }
            else
            {
                fCleanup = TRUE;
            }
        }
        else
        {
            NotfAssert((FALSE && "Deliver package: could not get thread  window"));
            hr = E_FAIL;
        }
    }

    if (fCleanup)
    {
        POSITION pos = _mapThreadIdToHwnd.GetStartPosition();

        // loop over all hwnd and deliver package to each thread
        while (pos)
        {
            HWND hwndDestination = 0;
            THREADID threadId = 0;

            _mapThreadIdToHwnd.GetNextAssoc(pos, threadId, hwndDestination);

            if (hwndDestination)
            {
                NotfAssert((threadId));
                if (!IsWindow(hwndDestination) )
                {
                    _mapThreadIdToHwnd.RemoveKey(threadId);
                }
            }

            hwndDestination = 0;
        }
    
    }


    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgentAsync::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgentProcess::HandlePackage
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentProcess::HandlePackage(CPackage *pCPackage)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgentProcess::HandlePackage\n", this));
    HRESULT hr = E_FAIL;
    BOOL fCleanup = FALSE;
    NotfAssert((pCPackage));
    NotfAssert((pCPackage->GetNotification()));

    NotfAssert(( (pCPackage->GetThreadId() == NULL) && "CPackage should not have a thread ID" ));
    NotfAssert(( (pCPackage->IsBroadcast() == TRUE) && "CPackage should be of type broadcast" ));

    POSITION pos = _mapThreadIdToHwnd.GetStartPosition();

    // loop over all hwnd and deliver package to each thread
    while (pos)
    {
        HWND hwndDestination = 0;
        THREADID threadId = 0;

        _mapThreadIdToHwnd.GetNextAssoc(pos, threadId, hwndDestination);

        if (hwndDestination)
        {
            NotfAssert((threadId));
            if (IsWindow(hwndDestination) )
            {
                //got an hwnd - should have and threadid
                hr = pCPackage->Deliver(hwndDestination, 0);
            }
            else
            {
                fCleanup = TRUE;
            }
        }

        hwndDestination = 0;
    }

    if (fCleanup)
    {
        POSITION pos = _mapThreadIdToHwnd.GetStartPosition();

        // loop over all hwnd and deliver package to each thread
        while (pos)
        {
            HWND hwndDestination = 0;
            THREADID threadId = 0;

            _mapThreadIdToHwnd.GetNextAssoc(pos, threadId, hwndDestination);

            if (hwndDestination)
            {
                NotfAssert((threadId));
                if (!IsWindow(hwndDestination) )
                {
                    _mapThreadIdToHwnd.RemoveKey(threadId);
                }
            }

            hwndDestination = 0;
        }
    
    }
    

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgentProcess::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgentGlobal::HandlePackage
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentGlobal::HandlePackage(CPackage *pCPackage)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgentGlobal::HandlePackage\n", this));
    HRESULT hr = E_FAIL;
    NotfAssert((pCPackage));
    NotfAssert((pCPackage->GetNotification()));

    CGlobalNotfMgr *pGlobalNofMgr =  GetGlobalNotfMgr();

    NotfAssert((pGlobalNofMgr));
    hr = pGlobalNofMgr->HandlePackage(pCPackage);

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgentGlobal::HandlePackage (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgentStart::Create
//
//  Synopsis:
//
//  Arguments:  [ppDelAgentStart] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:      NOT USED YET!!!
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentStart::Create(CDeliverAgentStart **ppDelAgentStart)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgent::Create \n", NULL));
    HRESULT hr = NOERROR;

    CDeliverAgentStart      *pDeliverAgentStart    = 0;
    CDeliverAgentThread     *pDeliverAgentThread   = 0;
    CDeliverAgentAsync      *pDeliverAgentAsync    = 0;
    CDeliverAgentProcess    *pDeliverAgentProcess  = 0;

    CMutexSem                               *pmxs  = 0;
    CMap<THREADID , THREADID, HWND , HWND>  *pmapThreadIdToHwnd = 0;
    CMapCookieToCVal                        *pmapCookieToCDest  = 0;
    CMapCookieToCVal                        *pmapClsToCDest     = 0;
    CMapCStrToCVal                          *pmapPIDToCDest     = 0;

    do
    {
        pmxs               = new CMutexSem                              ();
        pmapThreadIdToHwnd = new CMap<THREADID , THREADID, HWND , HWND> ();
        pmapCookieToCDest  = new CMapCookieToCVal                       ();
        pmapClsToCDest     = new CMapCookieToCVal                       ();
        pmapPIDToCDest     = new CMapCStrToCVal                         ();

        if (!pDeliverAgentThread)
        {
            pDeliverAgentThread   = new CDeliverAgentThread     ();
        }
        if (!pDeliverAgentAsync)
        {
            pDeliverAgentAsync    = new CDeliverAgentAsync      ();
        }
        if (!pDeliverAgentProcess)
        {
            pDeliverAgentProcess  = new CDeliverAgentProcess    ();
        }
        if (!pDeliverAgentStart)
        {
            pDeliverAgentStart = new CDeliverAgentStart();
        }

        break;
    } while (TRUE);

    if (pDeliverAgentStart)
    {
        *ppDelAgentStart = pDeliverAgentStart;
        hr = NOERROR;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgent::Create (hr:%lx)\n",pDeliverAgentStart, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgentThread::DispatchOnNotification
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//              [pNotificationSink] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentThread::DispatchOnNotification(CPackage *pCPackage, INotificationSink *pNotificationSink)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgentThread::DispatchOnNotification (pCPackage:%lx, pNotificationSink:%lx) \n", this, pCPackage, pNotificationSink));
    HRESULT hr = NOERROR;

    NotfAssert((pCPackage->GetNotification() && pNotificationSink));
    NotfDebugOut((DEB_DELIVER, "%p >>> NotificationSink:: OnNotification\n",pNotificationSink));

    if (   (!pCPackage->IsBroadcast())
        && (pCPackage->IsReportRequired() ) )
    {
        CNotificationReport *pNotfctnReport = pCPackage->GetNotificationReportDest();
        NotfAssert((pNotfctnReport));
        if (!pNotfctnReport->GetNotificationSink() )
        {
            pNotfctnReport->SetNotificationSink(pNotificationSink);
        }
        if (!pCPackage->GetThreadId() )
        {
            pNotfctnReport->SetThreadId( GetCurrentThreadId() );
        }
        else
        {
            pNotfctnReport->SetThreadId(pCPackage->GetThreadId());
        }
        if (!pCPackage->GetNotificationReport())
        {
            pCPackage->SetNotificationReportDest();
        }
        
        NotfAssert(( pCPackage->GetNotificationReport() ));
    }
    
    if (pCPackage->GetNotification() && pNotificationSink)
    {
        pCPackage->PreDispatch();

        INotificationReport *pNotfReport = (!pCPackage->IsBroadcast()) ? pCPackage->GetNotificationReport(TRUE) : 0;
        LPNOTIFICATION pNotification = pCPackage->GetNotification();        // LPNOTIFICATION          pNotification,

        NotfDebugOut((DEB_MGR, "%p _IN OnNotification:pSink:%lx, Cookie:[%ws]\n        NotfType:[%ws]\n", 
                                this, pNotificationSink, pCPackage->NotfCookieStr(), pCPackage->NotfTypeStr() ));
        HRESULT hrDest = NOERROR;                                

        #ifdef USE_NOTIFICATION_EXCEPTION_FILTER
        _try
        #endif //USE_NOTIFICATION_EXCEPTION_FILTER
        {
            if (!IS_BAD_INTERFACE(pNotificationSink))
            {
                hrDest = pNotificationSink->OnNotification(pNotification, pNotfReport, 0);
            }
            else
            {
                hrDest = E_POINTER;
            }
        }
        #ifdef USE_NOTIFICATION_EXCEPTION_FILTER
        _except(NotificationMgrInvokeExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
        {
            DWORD dwFault = GetExceptionCode();
            hrDest = HRESULT_FROM_WIN32(dwFault);
            #if DBG == 1
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
                NotfDebugOut((DEB_MGR, "%p NotificationMgr (NotificationSink:%p) has caught a fault 0x%08x on behalf of application %s", 
                               this , pNotificationSink, dwFault, achProgname));
            }
            #endif //DBG
        }
        #endif //USE_NOTIFICATION_EXCEPTION_FILTER
                                       
        NotfDebugOut((DEB_MGR, "%p Out OnNotification Notification:Sink;%xl, Cookie[%ws], hr:%lx\n", this, pNotificationSink, pCPackage->NotfCookieStr(), hrDest));
        if (pNotfReport)
        {
            pCPackage->SetHResultDest(hrDest);
            pNotfReport->Release();
        }

        NotfDebugOut((DEB_DELIVER, "%p <<<< NotificationSink:: OnNotification (hr:%lx)\n",pNotificationSink,hrDest));
    }

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgentThread::DispatchOnNotification (pCPackage:%lx, pNotificationSink:%lx, hr:%lx) \n", this, pCPackage, pNotificationSink, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgentStart::VerifyDestination
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentStart::VerifyDestination(CDestination *pCDestination, DWORD dwMode)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgentStart::VerifyDestination\n", this));
    HRESULT hr = NOERROR;
    NotfAssert((pCDestination));

    do
    {
        // verify that the destination is valid
        NotfAssert((pCDestination));
        NotfAssert(( pCDestination->GetThreadId() ));
        THREADID threadId = pCDestination->GetThreadId();
        HWND hwndDestination = pCDestination->GetPort();
        NotfAssert((threadId));
        BOOL fOk = TRUE;

        if (hwndDestination)
        {
            NotfAssert((hwndDestination));
            if (!IsWindow(hwndDestination) )
            {
                fOk = FALSE;
            }
        }

        if (!fOk)
        {
            PNOTIFICATIONCOOKIE pRegisterCookie = pCDestination->GetRegisterCookie();
            Unregister(pRegisterCookie);
            hr = E_FAIL;
        }
        
        break;
    } while (TRUE);

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgentStart::VerifyDestination (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeliverAgentStart::VerifyDestinationPort
//
//  Synopsis:
//
//  Arguments:  [pCPackage] --
//
//  Returns:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDeliverAgentStart::VerifyDestinationPort(THREADID threadId)
{
    NotfDebugOut((DEB_DELIVER, "%p _IN CDeliverAgentStart::VerifyDestinationPort\n", this));
    HRESULT hr = E_FAIL;

    do
    {
        CLock lck(_mxs);

        if (!threadId)
        {
            threadId = GetCurrentThreadId();
        }
        
        HWND hwndDestination = 0;
        NotfAssert((threadId));

        //
        // make sure thread has a window
        //

        //  REVIEW REVIEW. Using IsWindow always implys potential bugs.
        //  Also the only place we call this, we call without threadID.
/*
        if (_mapThreadIdToHwnd.Lookup(threadId, hwndDestination)
            && (hwndDestination) && IsWindow(hwndDestination))
        {
            hr = NOERROR;
            break;
        }
 */
        hwndDestination = GetThreadNotificationWnd();
        // we should always get an hwnd - assert but fail
        NotfAssert((hwndDestination));
        
        if (!hwndDestination)
        {
            break;    
        }
        
        _mapThreadIdToHwnd.SetAt(threadId, hwndDestination);

        break;
    } while (TRUE);

    NotfDebugOut((DEB_DELIVER, "%p OUT CDeliverAgentStart::VerifyDestinationPort (hr:%lx)\n",this, hr));
    return hr;
}


