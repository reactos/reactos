#include "private.h"
#include "throttle.h"
#include "subsmgrp.h"

#include <mluisupp.h>

#define TF_THISMODULE TF_THROTTLER

const int MAX_AUTOCACHESIZE_ASK = 2;
const int MIN_CACHE_INCREASE = 1024; // in KB

// Strings for cache restrictions
const TCHAR c_szKeyRestrict[] = TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel");
const TCHAR c_szCache[] = TEXT("Cache");         

CThrottler  *CThrottler::s_pThrottler = NULL;

const CFactoryData CThrottler::s_ThrottlerFactoryData = 
{
    &CLSID_SubscriptionThrottler, CreateInstance, 0
};

#ifdef DEBUG
void DUMPITEM(CHAR *pszMsg, const SUBSCRIPTIONCOOKIE *pCookie)
{
    ISubscriptionItem *psi;
    
    if (SUCCEEDED(SubscriptionItemFromCookie(FALSE, pCookie, &psi)))
    {
        BSTR bstrName;
        if (SUCCEEDED(ReadBSTR(psi, c_szPropName, &bstrName)))
        {
            TraceMsgA(TF_THISMODULE, "%s: %S", pszMsg, bstrName);
            SysFreeString(bstrName);
        }
        psi->Release();
    }
}

#else

#define DUMPITEM(pszMsg, pCookie)

#endif

// dwSyncFlags has 8 bits of enum (EVENTMASK) and the rest is flags
inline BOOL IsSyncEvent(DWORD dwSyncFlags, DWORD dwSyncEvent)
{
    return (dwSyncFlags & SYNCMGRFLAG_EVENTMASK) == dwSyncEvent;
}

inline BOOL IsSyncEventFlag(DWORD dwSyncFlags, DWORD dwSyncEvent)
{
    return (dwSyncFlags & dwSyncEvent) != 0;
}

inline BOOL IsIgnoreIdleSyncEvent(DWORD dwSyncFlags)
{
    return !IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_IDLE);
/*
    return IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_CONNECT) ||
           IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_PENDINGDISCONNECT) ||
           IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_MANUAL) ||
           IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_INVOKE);
*/
}

inline BOOL IsScheduleSyncEvent(DWORD dwSyncFlags)
{
    return IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_SCHEDULED) ||
           IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_IDLE);
}

class CThrottlerProxy : public ISubscriptionThrottler
{
  
public:
    CThrottlerProxy(CThrottler *pThrottler)
    {
        m_cRef = 1;
        m_pThrottler = pThrottler;

        m_pThrottler->ExternalAddRef();
    }

    STDMETHODIMP QueryInterface(REFIID riid, void **punk)
    {
        if ((riid == IID_IUnknown) || (riid == IID_ISubscriptionThrottler))
        {
            *punk = (ISubscriptionThrottler *)this;
            AddRef();
            return S_OK;
        }
        else
        {
            *punk = NULL;
            return E_NOINTERFACE;
        }
    }
    
    STDMETHODIMP_(ULONG) AddRef()
    { 
        return ++m_cRef;
    }
    
    STDMETHODIMP_(ULONG) Release()
    {
        if (--m_cRef == 0)
        {
            delete this;
            return 0;
        }

        return m_cRef;
    }

    STDMETHODIMP GetSubscriptionRunState( 
        /* [in] */ DWORD dwNumCookies,
        /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies,
        /* [size_is][out] */ DWORD *pdwRunState)
    {
        return m_pThrottler->GetSubscriptionRunState(dwNumCookies, pCookies, pdwRunState);
    }
    
    STDMETHODIMP AbortItems( 
        /* [in] */ DWORD dwNumCookies,
        /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies)
    {
        return m_pThrottler->AbortItems(dwNumCookies, pCookies);
    }
    
    STDMETHODIMP AbortAll()
    {
        return m_pThrottler->AbortAll();
    }

private:
    ULONG       m_cRef;
    CThrottler  *m_pThrottler;
    
    ~CThrottlerProxy()
    {
        m_pThrottler->ExternalRelease();
    }

};

HRESULT CThrottler::CreateInstance(IUnknown *punkOuter, IUnknown **ppunk)
{
    HRESULT hr;

    ASSERT(NULL == punkOuter);
    ASSERT(NULL != ppunk);

    if (NULL != CThrottler::s_pThrottler)
    {
        *ppunk = new CThrottlerProxy(CThrottler::s_pThrottler);

        if (NULL != *ppunk) 
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        TraceMsg(TF_ALWAYS, "UNEXPECTED ERROR: Failed to attached to throttler in CreateInstance");
        hr = E_UNEXPECTED;
    }

    return hr;
}

CThrottler::CThrottler()
{
    ASSERT(NULL == s_pThrottler);

    ASSERT(NULL == m_pItemsHead);
    ASSERT(NULL == m_pItemsTail);
    ASSERT(NULL == m_updateQueue[0]);

    //  BUGBUG - this is only until msidle is multi-client aware.
    IdleEnd();
    //m_fUserIsIdle = TRUE; //  TODO: need to determine this better
    IdleBegin(NULL);

    m_cRef = 1;
}

CThrottler::~CThrottler()
{
    DBG("Destroying Throttler");

    ASSERT(GetCurrentThreadId() == m_dwThreadId);

    IdleEnd();

    // Destroy window
    if (m_hwndThrottler)
    {
        SetWindowLongPtr(m_hwndThrottler, GWLP_USERDATA, 0);
        DestroyWindow(m_hwndThrottler);
        m_hwndThrottler = NULL;
    }

    s_pThrottler = NULL;

    RevokeClassObject();
}

HRESULT CThrottler::RevokeClassObject()
{
    HRESULT hr;
    
    if (m_dwRegister)
    {
        hr = CoRevokeClassObject(m_dwRegister);
        m_dwRegister = 0;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT /* static */ CThrottler::GetThrottler(CThrottler **ppThrottler)
{
    HRESULT hr = S_OK;
    
    ASSERT(NULL != ppThrottler);

    if (NULL != ppThrottler)
    {
        *ppThrottler = NULL;

        //  If there is no throttler create a new one
        if (NULL == s_pThrottler)
        {
            DBG("Creating new throttler in GetThrottler");

            s_pThrottler = new CThrottler;
            if (NULL != s_pThrottler)
            {
                IClassFactory *pcf = new CClassFactory(&s_ThrottlerFactoryData);
                if (NULL != pcf)
                {
                    HRESULT hrRegister = CoRegisterClassObject(CLSID_SubscriptionThrottler, 
                                                               pcf, 
                                                               CLSCTX_LOCAL_SERVER, 
                                                               REGCLS_MULTIPLEUSE, 
                                                               &s_pThrottler->m_dwRegister);
                    if (FAILED(hrRegister))
                    {
                        TraceMsg(TF_ALWAYS, "CoRegisterClassObject failed - other processes can't talk to us!");
                    }
                    pcf->Release();
                }
                *ppThrottler = s_pThrottler;
#ifdef DEBUG
                s_pThrottler->m_dwThreadId = GetCurrentThreadId();
#endif
            }
            else
            {
                DBG("Failed to create Throttler class factory");
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            //  Attach to existing throttler
            ASSERT(GetCurrentThreadId() == s_pThrottler->m_dwThreadId);
            s_pThrottler->AddRef();
            *ppThrottler = s_pThrottler;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

void CThrottler::OnIdleStateChange(DWORD dwState)
{
    if (NULL != s_pThrottler)
    {
        switch(dwState)
        {
            case STATE_USER_IDLE_BEGIN:
                DBG("OnIdleStateChange: Idle Begin");
            #ifdef DEBUG
                LogEvent(TEXT("Idle state begins"));
            #endif

                s_pThrottler->OnIdleBegin();
                break;

            case STATE_USER_IDLE_END:
                DBG("OnIdleStateChange: Idle End");
            #ifdef DEBUG
                LogEvent(TEXT("Idle state ends"));
            #endif

                s_pThrottler->OnIdleEnd();
                break;
        }
    }
}

void CThrottler::OnIdleBegin()
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    m_fUserIsIdle = TRUE;
    FillTheQueue();
}

void CThrottler::OnIdleEnd()
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    m_fUserIsIdle = FALSE;

    for (int i = 0; i < ARRAYSIZE(m_updateQueue); i++)
    {
        if ((NULL != m_updateQueue[i]) && 
            (m_updateQueue[i]->m_dwRunState & RS_SUSPENDONIDLE))
        {
            DUMPITEM("Suspending in CThrottler::OnIdleEnd", &m_updateQueue[i]->m_cookie);

            ISubscriptionAgentControl *pSubsAgentCtl;
            CUpdateItem *pUpdateItem = m_updateQueue[i];
           
            pSubsAgentCtl = m_updateQueue[i]->m_pSubsAgentCtl;

            m_updateQueue[i]->m_dwRunState &= ~RS_UPDATING;
            m_updateQueue[i]->m_dwRunState |= RS_SUSPENDED;

            m_updateQueue[i] = NULL;

            ASSERT(NULL != pSubsAgentCtl);

            if (SUCCEEDED(pSubsAgentCtl->PauseUpdate(0)))
            {
                WCHAR wszMsg[256];

                MLLoadStringW(IDS_UPDATE_PAUSED, wszMsg, ARRAYSIZE(wszMsg));
                NotifyHandlers(NH_UPDATEPROGRESS, &pUpdateItem->m_cookie, -1,
                               -1, -1, WC_INTERNAL_S_PAUSED, wszMsg);
            }
        }
    }

    FillTheQueue();
}

STDMETHODIMP CThrottler::QueryInterface(REFIID riid, void **ppv)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    if (NULL == ppv)
    {
        return E_INVALIDARG;
    }

    if ((IID_IUnknown == riid) || (IID_ISubscriptionAgentEvents == riid))
    {
        *ppv = (ISubscriptionAgentEvents *)this;
    }
    else if (IID_ISubscriptionThrottler == riid)
    {
        *ppv = (ISubscriptionThrottler *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    
    return S_OK;
}


ULONG CThrottler::ExternalAddRef()
{
    AddRef();
    
    return ++m_cExternalRef;
}

ULONG CThrottler::ExternalRelease()
{
    ULONG cRef = --m_cExternalRef;

    Release();

    return cRef;
}

STDMETHODIMP_(ULONG) CThrottler::AddRef()
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CThrottler::Release()
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

HRESULT CThrottler::NotifyHandlers(int idCmd, const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, ...)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr = S_OK;
    va_list va;
    long lSizeDownloaded = -1;
    long lProgressCurrent = -1;
    long lProgressMax = -1;
    HRESULT hrParam = E_UNEXPECTED;
    LPCWSTR wszParam = NULL;

    va_start(va, pSubscriptionCookie);

    //  First extract args
    switch (idCmd)
    {
        case NH_UPDATEBEGIN:
            //  Nothing to do for now
            break;

        case NH_UPDATEPROGRESS:
            lSizeDownloaded = va_arg(va, long);
            lProgressCurrent = va_arg(va, long);
            lProgressMax = va_arg(va, long);
            hrParam = va_arg(va, HRESULT);
            wszParam = va_arg(va, LPCWSTR);
            break;

        case NH_UPDATEEND:
            lSizeDownloaded = va_arg(va, long);
            hrParam = va_arg(va, HRESULT);
            wszParam = va_arg(va, LPCWSTR);
            break;

        case NH_REPORTERROR:
            hrParam = va_arg(va, HRESULT);
            wszParam = va_arg(va, LPCWSTR);
            break;

        default:
            ASSERT(0);  //  Don't know what to do
            hr = E_UNEXPECTED;
            break;
    }

    //  Now loop
    HRESULT hrTemp = S_OK;
    CSyncMgrNode *pSyncMgrNode = m_pSyncMgrs;

    while (pSyncMgrNode)
    {
        COfflineSync *pOfflineSync = pSyncMgrNode->m_pOfflineSync;
        pSyncMgrNode = pSyncMgrNode->m_pNext;

        switch (idCmd)
        {
            case NH_UPDATEBEGIN:
                hrTemp = pOfflineSync->UpdateBegin(pSubscriptionCookie);
                break;

            case NH_UPDATEPROGRESS:
                hrTemp = pOfflineSync->UpdateProgress(pSubscriptionCookie, 
                                                      lSizeDownloaded, 
                                                      lProgressCurrent, 
                                                      lProgressMax, 
                                                      hrParam, 
                                                      wszParam);

                break;

            case NH_UPDATEEND:
                hrTemp = pOfflineSync->UpdateEnd(pSubscriptionCookie, 
                                                 lSizeDownloaded, 
                                                 hrParam, 
                                                 wszParam);
                break;

            case NH_REPORTERROR:
                hrTemp = pOfflineSync->ReportError(pSubscriptionCookie, 
                                                hrParam, 
                                                wszParam);
                break;
        }

        if (FAILED(hrTemp))
        {
            hr = hrTemp;
        }
    }

    va_end(va);

    return hr;
}

// ISubscriptionAgentEvents members
STDMETHODIMP CThrottler::UpdateBegin(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr;
    CUpdateItem *pUpdateItem;

    hr = FindCookie(pSubscriptionCookie, &pUpdateItem);

    ASSERT(SUCCEEDED(hr));

    if (SUCCEEDED(hr))
    {
        DUMPITEM("CThrottler::UpdateBegin", pSubscriptionCookie);
        pUpdateItem->m_dwRunState &= ~(RS_READY | RS_SUSPENDED);
        pUpdateItem->m_dwRunState |= RS_UPDATING;

        hr = NotifyHandlers(NH_UPDATEBEGIN, pSubscriptionCookie);
    }

    return hr;
}

STDMETHODIMP CThrottler::UpdateProgress(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
        long lSizeDownloaded,
        long lProgressCurrent,
        long lProgressMax,
        HRESULT hrStatus,
        LPCWSTR wszStatus)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr;

    CUpdateItem *pUpdateItem;

    //  TODO: Get rid of this bogus crap!!!
    //  Adjust the max to fool syncmgr
    if (SUCCEEDED(FindCookie(pSubscriptionCookie, &pUpdateItem)))
    {
        if ((lProgressMax < 0) || (lProgressMax <= lProgressCurrent))
        {
            if (pUpdateItem->m_nMax <= lProgressCurrent)
            {
                pUpdateItem->m_nMax = (lProgressCurrent * 3) / 2;
            }

            lProgressMax = pUpdateItem->m_nMax;
        }
    }

    hr = NotifyHandlers(NH_UPDATEPROGRESS, pSubscriptionCookie, lSizeDownloaded,
                            lProgressCurrent, lProgressMax, hrStatus, wszStatus);
    return hr;
}

STDMETHODIMP CThrottler::UpdateEnd(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
        long lSizeDownloaded,
        HRESULT hrResult,
        LPCWSTR wszResult)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr;
    CUpdateItem *pUpdateItem;
    SUBSCRIPTIONCOOKIE cookie = *pSubscriptionCookie;

    hr = FindCookie(pSubscriptionCookie, &pUpdateItem);

    if (SUCCEEDED(hr))
    {
        DUMPITEM("CThrottler::UpdateEnd", pSubscriptionCookie);

        pUpdateItem->m_dwRunState &= ~(RS_READY | RS_SUSPENDED | RS_UPDATING | RS_SUSPENDONIDLE);
        pUpdateItem->m_dwRunState |= RS_COMPLETED;

        RemoveItemFromList(pUpdateItem, TRUE);

        //  ************************************************************************
        //  Don't use anything that could have come from pUpdateItem after this 
        //  including the pSubscriptionCookie above which came from an agent which
        //  probably no longer exists!
        //  (actually, the agent keeps itself alive until this call returns)
        //  ************************************************************************

    }

    hr = NotifyHandlers(NH_UPDATEEND, &cookie,
                        lSizeDownloaded, hrResult, wszResult);

    FireSubscriptionEvent(SUBSNOTF_SYNC_STOP, &cookie);

    FillTheQueue();

    return hr;
}

STDMETHODIMP CThrottler::ReportError(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
        HRESULT hrError, 
        LPCWSTR wszError)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr;

    if (INET_E_AGENT_EXCEEDING_CACHE_SIZE == hrError)
    {
        // Agent is notifying us that they're about to exceed the cache size.
        hr = AutoCacheSizeRequest(pSubscriptionCookie);
    }
    else
        hr = NotifyHandlers(NH_REPORTERROR, pSubscriptionCookie, hrError, wszError);

    return hr;
}

STDMETHODIMP CThrottler::GetSubscriptionRunState( 
    /* [in] */ DWORD dwNumCookies,
    /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies,
    /* [size_is][out] */ DWORD *pdwRunState)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
  
    if ((0 == dwNumCookies) ||
        (NULL == pCookies) ||
        (NULL == pdwRunState))
    {
        return E_INVALIDARG;
    }

    for (DWORD i = 0; i < dwNumCookies; i++, pCookies++, pdwRunState++)
    {
        CUpdateItem *pUpdateItem;

        if (SUCCEEDED(FindCookie(pCookies, &pUpdateItem)))
        {
            *pdwRunState = pUpdateItem->m_dwRunState;
        }
        else
        {
            *pdwRunState = 0;
        }
    }

    return S_OK;
}

// DoAbortItem will cause the CThrottler to get released if the last running
//  agent is aborted (Agent notifies it's done, SyncMgr releases throttler,
//   then agent releases throttler)
HRESULT CThrottler::DoAbortItem(CUpdateItem *pUpdateItem)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr;

    ASSERT(((pUpdateItem->m_dwRunState & (RS_UPDATING | RS_SUSPENDED)) && 
            (NULL != pUpdateItem->m_pSubsAgentCtl))
           ||
           (NULL == pUpdateItem->m_pSubsAgentCtl));

    if ((pUpdateItem->m_dwRunState & (RS_UPDATING | RS_SUSPENDED)) &&
        (NULL != pUpdateItem->m_pSubsAgentCtl))
    {
        DUMPITEM("CThrottler::DoAbortItem with existing Agent", &pUpdateItem->m_cookie);
        hr = pUpdateItem->m_pSubsAgentCtl->AbortUpdate(0);
    }
    else
    {
        WCHAR wszMsg[256];

        MLLoadStringW(IDS_STATUS_ABORTED, wszMsg, ARRAYSIZE(wszMsg));

        DUMPITEM("CThrottler::DoAbortItem with no Agent", &pUpdateItem->m_cookie);
        
        ReportThrottlerError(&pUpdateItem->m_cookie, E_ABORT, wszMsg);
        hr = UpdateEnd(&pUpdateItem->m_cookie, 0, E_ABORT, wszMsg);
    }

    return hr;
}

STDMETHODIMP CThrottler::AbortItems( 
    /* [in] */ DWORD dwNumCookies,
    /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr;

    if ((0 == dwNumCookies) ||
        (NULL == pCookies))
    {
        return E_INVALIDARG;
    }

    if (FAILED(CreateThrottlerWnd()))
        return E_FAIL;

    hr = S_OK;

    void *pItem = MemAlloc(LMEM_FIXED, dwNumCookies * sizeof(SUBSCRIPTIONCOOKIE));
    if (pItem)
    {

#ifdef DEBUG
        for (DWORD i = 0; i < dwNumCookies; i++)
        {
            DUMPITEM("Aborting in CThrottler::AbortItems", &pCookies[i]);
        }
#endif

        memcpy(pItem, pCookies, dwNumCookies * sizeof(SUBSCRIPTIONCOOKIE));
        PostMessage(m_hwndThrottler, WM_THROTTLER_ABORTITEM, (WPARAM)dwNumCookies, (LPARAM)pItem);
    }
    else
    {
        DBG_WARN("Memory alloc failed in CThrottler::AbortItems");
        hr = S_FALSE;
    }
    return hr;
}
        
STDMETHODIMP CThrottler::ActuallyAbortItems( 
    /* [in] */ DWORD dwNumCookies,
    /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr;

    if ((0 == dwNumCookies) ||
        (NULL == pCookies))
    {
        return E_INVALIDARG;
    }

    hr = S_OK;

    // DoAbortItem will cause the CThrottler to get released if the last
    //  running agent is aborted. Protect against that.
    AddRef();

    for (DWORD i = 0; i < dwNumCookies; i++, pCookies++)
    {
        HRESULT hrItem;
        CUpdateItem *pUpdateItem;

        hrItem = FindCookie(pCookies, &pUpdateItem);
        if (SUCCEEDED(hrItem))
        {
            hrItem = DoAbortItem(pUpdateItem);

            //  ************************************************************************
            //  pUpdateItem is no longer valid!
            //  ************************************************************************
        }

        if (FAILED(hrItem))
        {
            hr = S_FALSE;
        }
    }

    Release();

    return hr;
}

HRESULT CThrottler::CreateThrottlerWnd()
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    if (!m_hwndThrottler)
    {
        WNDCLASS wc;

        wc.style = 0;
        wc.lpfnWndProc = CThrottler::ThrottlerWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_hInst;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = (HBRUSH)NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = THROTTLER_WNDCLASS;

        RegisterClass(&wc);

        m_hwndThrottler = CreateWindow(THROTTLER_WNDCLASS, TEXT("YO"), WS_OVERLAPPED,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, g_hInst, (LPVOID)this);

        if (NULL == m_hwndThrottler)
        {
            DBG_WARN("CThrottler CreateWindow failed");
            return E_FAIL;
        }
    }

    return S_OK;
}

LRESULT CThrottler::ThrottlerWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    CThrottler *pThis = (CThrottler*) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    // Validate pThis
#ifdef DEBUG
    if (pThis && IsBadWritePtr(pThis, sizeof(*pThis)))
    {
        TraceMsg(TF_THISMODULE|TF_WARNING,
            "Invalid 'this' in ThrottlerWndProc (0x%08x) - already destroyed?", pThis);
    }
    if (pThis)
    {
        ASSERT(GetCurrentThreadId() == pThis->m_dwThreadId);
        ASSERT(GetCurrentThreadId() == GetWindowThreadProcessId(hWnd, NULL));
    }
#endif

    switch (Msg)
    {
        case WM_CREATE :
        {
            LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

            if (!pcs || !(pcs->lpCreateParams))
            {
                DBG_WARN("Invalid param ThrottlerWndProc Create");
                return -1;
            }
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) pcs->lpCreateParams);
            return 0;
        }

        case WM_THROTTLER_ABORTALL:
            if (pThis)
                pThis->ActuallyAbortAll();
            break;

        case WM_THROTTLER_ABORTITEM:
            if (pThis)
                pThis->ActuallyAbortItems((ULONG) wParam, (SUBSCRIPTIONCOOKIE*) lParam);
            MemFree((HLOCAL)lParam);
            break;

        case WM_THROTTLER_AUTOCACHESIZE_ASK:
            if (pThis)
                pThis->AutoCacheSizeAskUser((DWORD)lParam);
            break;

        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

STDMETHODIMP CThrottler::AbortAll()
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    if (FAILED(CreateThrottlerWnd()))
        return E_FAIL;

    DBG("Aborting all items");

    PostMessage(m_hwndThrottler, WM_THROTTLER_ABORTALL, 0, 0);

    return S_OK;
}

STDMETHODIMP CThrottler::ActuallyAbortAll()
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr = S_OK;
    CUpdateItem *pItem = m_pItemsHead;

    if (FALSE == m_fAbortingAll)
    {
        m_fAbortingAll = TRUE;

        while (pItem)
        {
            CUpdateItem *pUpdateItem = pItem;

            //  Move forward now since this item should get yanked!
            pItem = pItem->m_pNext;

            if (FAILED(DoAbortItem(pUpdateItem)))
            {
                hr = S_FALSE;
            }
        }
        m_fAbortingAll = FALSE;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CThrottler::Advise(COfflineSync *pOfflineSync)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr;
    CSyncMgrNode *pSyncMgrNode;

    ASSERT(NULL != pOfflineSync);

#ifdef DEBUG
    pSyncMgrNode = m_pSyncMgrs;

    while (pSyncMgrNode)
    {
        if (pSyncMgrNode->m_pOfflineSync == pOfflineSync)
        {
            ASSERT(0);      //  Shouldn't advise more than once!
        }
        pSyncMgrNode = pSyncMgrNode->m_pNext;
    }
 #endif

    ASSERT(!m_hwndParent || (m_hwndParent == pOfflineSync->GetParentWindow()));

    m_hwndParent = pOfflineSync->GetParentWindow();

    pSyncMgrNode = new CSyncMgrNode(pOfflineSync, m_pSyncMgrs);

    if (NULL != pSyncMgrNode)
    {
        pOfflineSync->AddRef();
        m_pSyncMgrs = pSyncMgrNode;
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CThrottler::Unadvise(COfflineSync *pOfflineSync)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr = E_FAIL;
    CSyncMgrNode *pSyncMgrNode;
    CSyncMgrNode **ppSyncMgrPrev;

    ASSERT(NULL != pOfflineSync);

    pSyncMgrNode = m_pSyncMgrs;
    ppSyncMgrPrev = &m_pSyncMgrs;

    while (pSyncMgrNode)
    {
        if (pSyncMgrNode->m_pOfflineSync == pOfflineSync)
        {
            *ppSyncMgrPrev = pSyncMgrNode->m_pNext;
            delete pSyncMgrNode;
            hr = S_OK;
            break;
        }
        ppSyncMgrPrev = &pSyncMgrNode->m_pNext;
        pSyncMgrNode = pSyncMgrNode->m_pNext;
    }

    ASSERT(SUCCEEDED(hr));  //  This is internal goo so should not fail!

    if (NULL == m_pSyncMgrs)
    {
        //  Everyone has lost interest in us...
        RevokeClassObject();
        s_pThrottler = NULL;

        while (m_cExternalRef > 0)
        {
            TraceMsg(TF_WARNING, "CThrottle::Unadvise m_cExternalRef = %d", m_cExternalRef);
            MSG msg;
            
            if (PeekMessage(&msg, NULL, 0, 0, TRUE))
            {
                DispatchMessage(&msg);
            }
        }
    }
    
    return hr;
}

int CThrottler::GetCookieIndexInQueue(const SUBSCRIPTIONCOOKIE *pCookie)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    int index = -1;
    
    for (int i = 0; i < ARRAYSIZE(m_updateQueue); i++)
    {
        if ((NULL != m_updateQueue[i]) && (m_updateQueue[i]->m_cookie == *pCookie))
        {   
            index = i;
            break;
        }
    }

    return index;
}

void CThrottler::FailedUpdate(HRESULT hr, const SUBSCRIPTIONCOOKIE *pCookie)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    WCHAR wszMsg[256];
    int resID;

    switch (hr)
    {
        case INET_E_SCHEDULED_UPDATES_DISABLED:
            resID = IDS_SCHEDULED_UPDATES_DISABLED;
            break;
            
        case INET_E_SCHEDULED_UPDATES_RESTRICTED:
            resID = IDS_SCHEDULED_UPDATES_RESTRICTED;
            break;
            
        case INET_E_SCHEDULED_UPDATE_INTERVAL:
            resID = IDS_SCHEDULED_UPDATE_INTERVAL;
            break;
            
        case INET_E_SCHEDULED_EXCLUDE_RANGE:
            resID = IDS_SCHEDULED_EXCLUDE_RANGE;
            break;
            
        default:
            resID = IDS_CRAWL_STATUS_NOT_OK;
            break;
    }

    MLLoadStringW(resID, wszMsg, ARRAYSIZE(wszMsg));
    ReportThrottlerError(pCookie, hr, wszMsg);
    UpdateEnd(pCookie, 0, hr, wszMsg);
}

void CThrottler::RunItem(int queueSlot, CUpdateItem *pUpdateItem)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr; 

    ASSERT(NULL == m_updateQueue[queueSlot]);

    m_updateQueue[queueSlot] = pUpdateItem;

    if (pUpdateItem->m_dwRunState & RS_SUSPENDED)
    {
        DUMPITEM("Resuming suspended item in CThrottler::RunItem", &pUpdateItem->m_cookie);
        ASSERT(NULL != pUpdateItem->m_pSubsAgentCtl);

        pUpdateItem->m_dwRunState |= RS_UPDATING;
        pUpdateItem->m_dwRunState &= ~RS_SUSPENDED;
        hr = pUpdateItem->m_pSubsAgentCtl->ResumeUpdate(0);

        if (SUCCEEDED(hr))
        {
            WCHAR wszMsg[256];

            MLLoadStringW(IDS_UPDATE_RESUMING, wszMsg, ARRAYSIZE(wszMsg));
            NotifyHandlers(NH_UPDATEPROGRESS, &pUpdateItem->m_cookie, -1,
                           -1, -1, WC_INTERNAL_S_RESUMING, wszMsg);
        }
    }
    else
    {
        ISubscriptionItem *psi;

        hr = SubscriptionItemFromCookie(FALSE, &pUpdateItem->m_cookie, &psi);


        if (SUCCEEDED(hr))
        {
            SUBSCRIPTIONITEMINFO sii;
            
            sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);
            hr = psi->GetSubscriptionItemInfo(&sii);
            if (SUCCEEDED(hr))
            {
                hr = CoCreateInstance(sii.clsidAgent, 
                                      NULL, 
                                      CLSCTX_INPROC_SERVER, 
                                      IID_ISubscriptionAgentControl, 
                                      (void**)&pUpdateItem->m_pSubsAgentCtl);

                if (SUCCEEDED(hr))
                {
                    DUMPITEM("Running item in CThrottler::RunItem", &pUpdateItem->m_cookie);
                    hr = pUpdateItem->m_pSubsAgentCtl->StartUpdate(psi, 
                                (ISubscriptionAgentEvents *)this);

                    FireSubscriptionEvent(SUBSNOTF_SYNC_START, &pUpdateItem->m_cookie);

                }
                else
                {
                    DBG_WARN("CoCreate Agent FAILED in CThrottler::RunItem");
                }
            }
            psi->Release();
        }
        else
        {
            DBG_WARN("SubscriptionItemFromCookie FAILED in CThrottler::RunItem");
        }
    }

    if (FAILED(hr))
    {
        FailedUpdate(hr, &pUpdateItem->m_cookie);
    }
}

int CThrottler::GetFreeQueueSlot()
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    int index = -1;
    
    for (int i = 0; i < ARRAYSIZE(m_updateQueue); i++)
    {
        if (NULL == m_updateQueue[i])
        {   
            index = i;
            break;
        }
    }

    return index;
}

void CThrottler::FillTheQueue()
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    if ((FALSE == m_fFillingTheQueue) &&        // avoid re-entrancy
        (FALSE == m_fAbortingAll) &&            // avoid re-entrancy
        (FALSE == m_fAutoCacheSizePending))     // we have a dialog up for the user
    {
        m_fFillingTheQueue = TRUE;

        CUpdateItem *pNextItem = m_pItemsHead;
        CUpdateItem *pItem;

        while (NULL != pNextItem)
        {
            pItem = pNextItem;

            //  Move ahead since this item may not be here 
            //  if we run it and it fails

            pNextItem = pNextItem->m_pNext;

            if (!(pItem->m_dwRunState & (RS_COMPLETED | RS_UPDATING)))
            {
                int freeSlot = GetFreeQueueSlot();

                if ((freeSlot >= 0) &&
                    (m_fUserIsIdle || (!(pItem->m_dwRunState & RS_SUSPENDONIDLE))))
                {
                    RunItem(freeSlot, pItem);
                }
                else 
                {
                    //  If we didn't run it then let's make sure the UI reflects the current
                    //  state properly

                    HRESULT hrStatus;
                    WCHAR wszMsg[256];

                    if ((pItem->m_dwRunState & RS_SUSPENDONIDLE) && (!m_fUserIsIdle))
                    {
                        MLLoadStringW(IDS_UPDATE_PAUSED, wszMsg, ARRAYSIZE(wszMsg));
                        hrStatus = WC_INTERNAL_S_PAUSED;
                    }
                    else
                    {
                        StrCpyW(wszMsg, L" ");  //  Don't say it, I know what you're thinking...
                                                //  ...if we don't do this, then the status
                                                //  text won't change.
                        hrStatus = WC_INTERNAL_S_PENDING;
                    }

                    NotifyHandlers(NH_UPDATEPROGRESS, &pItem->m_cookie, -1,
                                   -1, -1, hrStatus, wszMsg);
                }
            }
        }

        m_fFillingTheQueue = FALSE;
    }
}

HRESULT CThrottler::AddItemToListTail(CUpdateItem *pAddItem)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr = S_OK;
    
    ASSERT(NULL != pAddItem);

    if (NULL != pAddItem)
    {
        if (NULL == m_pItemsTail)
        {
            //  Nothing in the list
            ASSERT(NULL == m_pItemsHead);
            m_pItemsHead = pAddItem;
        }
        else
        {
            m_pItemsTail->m_pNext = pAddItem;
        }
        m_pItemsTail = pAddItem;
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    ASSERT(NULL != m_pItemsHead);
    ASSERT(NULL != m_pItemsTail);
    ASSERT(NULL == m_pItemsTail->m_pNext);

    ASSERT(SUCCEEDED(hr));

    return hr;
}

HRESULT CThrottler::RemoveItemFromList(CUpdateItem *pRemoveItem, BOOL fDelete)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr = E_UNEXPECTED;
    CUpdateItem *pItem = m_pItemsHead;
    CUpdateItem *pPrevItem = NULL;

    ASSERT(NULL != pRemoveItem);
    ASSERT(NULL != m_pItemsHead);
    ASSERT(NULL != m_pItemsTail);

    if (NULL != pRemoveItem)
    {
        int queueIndex = GetCookieIndexInQueue(&pRemoveItem->m_cookie);

        if (queueIndex >= 0)
        {
            m_updateQueue[queueIndex] = NULL;
        }
        
        while (pItem)
        {
            if (pItem == pRemoveItem)
            {
                if (NULL != pPrevItem)
                {
                    //  Removing beyond the head
                    pPrevItem->m_pNext = pItem->m_pNext;
                }
                else
                {
                    //  Removing the head
                    m_pItemsHead = pItem->m_pNext;
                }

                //  Now fix the tail
                if (m_pItemsTail == pRemoveItem)
                {
                    m_pItemsTail = pPrevItem;
                }
                hr = S_OK;
                break;
            }
            pPrevItem = pItem;
            pItem = pItem->m_pNext;
        }
        if (fDelete)
        {
            delete pRemoveItem;
        }
    }

    ASSERT(((NULL != m_pItemsHead) && (NULL != m_pItemsTail) && (NULL == m_pItemsTail->m_pNext)) ||
           ((NULL == m_pItemsHead) && (NULL == m_pItemsTail)));
    ASSERT(SUCCEEDED(hr));

    // If we have just removed our last item from the list, check to see if we forced
    //  global online mode or autodialed and fix up if so.
    if ((NULL == m_pItemsHead) && (m_fForcedGlobalOnline || m_fAutoDialed))
    {
        if (m_fForcedGlobalOnline)
        {
            SetGlobalOffline(TRUE);
            m_fForcedGlobalOnline = FALSE;
            m_fAutoDialed = FALSE;
        }
        else
        {
            ASSERT(m_fAutoDialed);
            InternetAutodialHangup(0);
            m_fAutoDialed=FALSE;
        }
    }

    return hr;
}

HRESULT CThrottler::CanScheduledItemRun(ISubscriptionItem *pSubsItem)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    //  If this item is running as a result of a schedule invocation, then
    //  we need to check time/range restrictions.
    HRESULT hr = S_OK;
    const TCHAR c_szNoScheduledUpdates[] = TEXT("NoScheduledUpdates");
    DWORD dwData;
    DWORD cbData = sizeof(dwData);

    //  First check if the user has disabled scheduled updates in inetcpl.
    if ((ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szRegKey, 
        c_szNoScheduledUpdates, NULL, &dwData, &cbData))
        && dwData)
    {
        hr = INET_E_SCHEDULED_UPDATES_DISABLED;
    }

    if (SUCCEEDED(hr))
    {
        //  Check if admin has disabled scheduled updates altogether
        if (SHRestricted2W(REST_NoScheduledUpdates, NULL, 0))
        {
            hr = INET_E_SCHEDULED_UPDATES_RESTRICTED;
        }
    }

    if (SUCCEEDED(hr))
    {
        //  Check if admin has set a minimum update interval.
        DWORD dwMinUpdateInterval = SHRestricted2W(REST_MinUpdateInterval, NULL, 0);

        if (dwMinUpdateInterval > 0)
        {
            DATE dt;

            if (SUCCEEDED(ReadDATE(pSubsItem, c_szPropCompletionTime, &dt)))
            {
                SYSTEMTIME st;
                GetLocalTime(&st);

                CFileTime lastUpdate;
                CFileTime currentTime;

                VariantTimeToFileTime(dt, lastUpdate);
                SystemTimeToFileTime(&st, &currentTime);

                if ((currentTime - lastUpdate) < 
                    ((__int64)dwMinUpdateInterval * ONE_MINUTE_IN_FILETIME))
                {
                    hr = INET_E_SCHEDULED_UPDATE_INTERVAL;
                }
            }
        }
    }
    
    if (SUCCEEDED(hr))
    {
        DWORD dwBegin = SHRestricted2W(REST_UpdateExcludeBegin, NULL, 0);
        DWORD dwEnd = SHRestricted2W(REST_UpdateExcludeEnd, NULL, 0);

        //  Check if admin has specified a blackout time for scheduled updates.
        if (dwBegin && dwEnd)
        {
            SYSTEMTIME st;
            CFileTime ftNow,
                      ftBegin,
                      ftEnd;
            
            GetLocalTime(&st);

            SystemTimeToFileTime(&st, &ftNow);
            
            st.wSecond = 0;
            st.wMilliseconds = 0;
            
            st.wHour   = (WORD)dwBegin / 60;
            st.wMinute = (WORD)dwBegin % 60;
            SystemTimeToFileTime(&st, &ftBegin);
            
            st.wHour   = (WORD)dwEnd / 60;
            st.wMinute = (WORD)dwEnd % 60;
            SystemTimeToFileTime(&st, &ftEnd);

            //  if these values are normalized (ie. begin comes before end)
            if (ftBegin <= ftEnd)
            {
                //  Then just check to see if time now is between begin 
                //  and end.  (ie.  ftEnd >= ftNow >= ftBegin)
                if ((ftNow >= ftBegin) && (ftNow <= ftEnd))
                {
                    hr = INET_E_SCHEDULED_EXCLUDE_RANGE;
                }
            }
            else
            {
                //  Begin and end are not normalized.  So we check to see if
                //  now is before end or now is after begin.

                //  For example:
                //  Assuming begin is 6pm and end is 6am.  If now is 5 pm, the
                //  item should run.  If now is 10pm or 4am, it should not run.

                if ((ftNow <= ftEnd) || (ftNow >= ftBegin))
                {
                    hr = INET_E_SCHEDULED_EXCLUDE_RANGE;
                }
            }

        }
    }

    return hr;
}

HRESULT CThrottler::RunCookies(DWORD dwNumCookies, 
                               const SUBSCRIPTIONCOOKIE *pSubscriptionCookies, 
                               DWORD dwSyncFlags)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr = S_OK;
    DWORD i;
    CUpdateItem *pUpdateItem;
    DWORD nValidCookies;

    ASSERT(NULL != m_pSyncMgrs);

    ASSERT(0 != dwNumCookies);
    ASSERT(NULL != pSubscriptionCookies);

    if ((0 == dwNumCookies) || 
        (NULL == pSubscriptionCookies))
    {
        return E_INVALIDARG;
    }

    // Check for global offline mode.
    if (!m_fForcedGlobalOnline && IsGlobalOffline())
    {
        // Force global online mode so that our update will succeed.
        DBG("CThrottler::RunCookies; forcing global online mode");
        SetGlobalOffline(FALSE);
        m_fForcedGlobalOnline = TRUE;
    }

    if (IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_MANUAL) ||
        IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_INVOKE))
    {
        if (!InternetGetConnectedStateEx(NULL, NULL, 0, 0))
        {
            if (!InternetAutodial(INTERNET_AUTODIAL_FORCE_ONLINE, 0))
            {
                // BUGBUG clean up this extra addref/release/return stuff
                AddRef();

                DBG("CThrottler::RunCookies autodial failed");

                // Uh-oh. The user cancelled the dial after starting a
                //  manual update. Silly user. Clean up and return.
                if (m_fForcedGlobalOnline)
                {
                    SetGlobalOffline(TRUE);
                    m_fForcedGlobalOnline=FALSE;
                }

                WCHAR wszMsg[256];

                MLLoadStringW(IDS_STATUS_ABORTED, wszMsg, ARRAYSIZE(wszMsg));
        
                for (i=0; i<dwNumCookies; i++)
                {
                    ReportThrottlerError(&pSubscriptionCookies[i], E_ABORT, wszMsg);
                    UpdateEnd(&pSubscriptionCookies[i], 0, E_ABORT, wszMsg);
                }

                Release();

                return S_FALSE; // E_ABORT;
            }

            // Autodial succeeded
            m_fAutoDialed = TRUE;
        }
    }

    SUBSCRIPTIONCOOKIE *pCookies = new SUBSCRIPTIONCOOKIE[dwNumCookies];

    if (NULL != pCookies)
    {
        SUBSCRIPTIONCOOKIE *pCookie = pCookies;

        memcpy(pCookies, pSubscriptionCookies, dwNumCookies * sizeof(SUBSCRIPTIONCOOKIE));
    
        //  ************************************************************************
        //  Don't add any return statements in the loop!  We keep a ref on ourselves
        //  during this call in case we are Released by all of the sync handlers.
        //  ************************************************************************

        AddRef();

        nValidCookies = 0;

        for (i = 0; i < dwNumCookies; i++, pCookie++)
        {
            if (*pCookie == GUID_NULL)
            {
                continue;
            }

            nValidCookies++;

            if (IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_IDLE))
            {
                m_fUserIsIdle = TRUE;
            }

            if (SUCCEEDED(FindCookie(pCookie, &pUpdateItem)))
            {
                if (IsIgnoreIdleSyncEvent(dwSyncFlags))
                {
                    DUMPITEM("Removing RS_SUSPENDONIDLE in CThrottler::RunCookies", pCookie);
                    
                    //  Items updated manually are no longer subject to idle detection.
                    pUpdateItem->m_dwRunState &= ~RS_SUSPENDONIDLE;
                }

                if (IsSyncEventFlag(dwSyncFlags, SYNCMGRFLAG_MAYBOTHERUSER))
                {
                    // We may now bother user for this item
                    pUpdateItem->m_dwRunState |= RS_MAYBOTHERUSER;
                }
            }
            else
            {
                ISubscriptionItem *psi;
                HRESULT hrItem = SubscriptionItemFromCookie(FALSE, pCookie, &psi);

                if (SUCCEEDED(hrItem))
                {
                    SUBSCRIPTIONITEMINFO sii;

                    sii.cbSize = sizeof(SUBSCRIPTIONITEMINFO);

                    hrItem = psi->GetSubscriptionItemInfo(&sii);

                    if (SUCCEEDED(hrItem))
                    {
                        DWORD dwRunState = RS_READY;

                        if (IsSyncEvent(dwSyncFlags, SYNCMGRFLAG_IDLE))
                        {
                            dwRunState |= RS_SUSPENDONIDLE;
                        }

                        if (IsSyncEventFlag(dwSyncFlags, SYNCMGRFLAG_MAYBOTHERUSER))
                        {
                            dwRunState |= RS_MAYBOTHERUSER;
                        }

                        if (IsScheduleSyncEvent(dwSyncFlags))
                        {
                            hrItem = CanScheduledItemRun(psi);
                        }

                        if (SUCCEEDED(hrItem))
                        {
                        
                            pUpdateItem = new CUpdateItem(*pCookie, dwRunState);

                            if (NULL != pUpdateItem)
                            {
                                AddItemToListTail(pUpdateItem);
                            }
                            else
                            {
                                hrItem = E_OUTOFMEMORY;
                            }
                        }
                    }
                    psi->Release();
                }

                if (FAILED(hrItem))
                {
                    //  If we fail on an item, we will continue to try others, but
                    //  we need to indicate failure for this one.
                    FailedUpdate(hrItem, pCookie);
                    hr = S_FALSE;
                }
            }

            if (NULL == m_pSyncMgrs)
            {
                //  We have been unadvised!
                break;
            }
        }

        //  No point in trying to update if nobody wants to listen
        if (NULL != m_pSyncMgrs)
        {
            FillTheQueue();
        }

        Release();

        delete [] pCookies;

        if (0 == nValidCookies)
        {
            hr = E_FAIL;
        }

        //  ************************************************************************
        //  No member variable access after this since we could be dead!!!!
        //  ************************************************************************
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


HRESULT CThrottler::FindCookie(
    const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
    CUpdateItem **ppUpdateItem)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr = E_FAIL;
    CUpdateItem *pItem = m_pItemsHead;
    
    ASSERT(NULL != ppUpdateItem);

    *ppUpdateItem = NULL;

    while (pItem)
    {
        if (pItem->m_cookie == *pSubscriptionCookie)
        {
            *ppUpdateItem = pItem;
            hr = S_OK;
            break;
        }
        pItem = pItem->m_pNext;
    }

    return hr;
}


//==============================================================================
//
// Auto cache size increase
//
//==============================================================================
// We can return:
//  E_PENDING - agent will pause and wait to be resumed or aborted
//  INET_S_AGENT_INCREASED_CACHE_SIZE - agent will try making stuff sticky again
//  anything else - agent will abort with INET_E_AGENT_CACHE_SIZE_EXCEEDED
HRESULT CThrottler::AutoCacheSizeRequest(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    HRESULT hr = S_OK;
    DWORD dwCacheSizeKB;
    int queueIndex;

    DWORD dwValue, dwSize = sizeof(dwValue);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, c_szKeyRestrict, c_szCache, NULL, &dwValue, &dwSize)
        && (dwValue != 0))
    {
        // Not allowed to change the cache size.
        hr = E_FAIL;
    }

    queueIndex = GetCookieIndexInQueue(pSubscriptionCookie);

    if (queueIndex >= 0)
    {
        if (!(m_updateQueue[queueIndex]->m_dwRunState & RS_MAYBOTHERUSER))
        {
            // We're not allowed to bother user. Fail.
            hr = E_FAIL;
        }
    }
    else
    {
        DBG_WARN("CThrottler::AutoCacheSizeRequest couldn't find cookie in run queue.");
        hr = E_FAIL;        // Couldn't find this cookie in our queue?!
    }

    if (SUCCEEDED(hr) && m_fAutoCacheSizePending)
    {
        // We're already asking the user to increase the cache size.
        hr = E_PENDING;
    }

    if (SUCCEEDED(hr))
    {
        // Let's try to increase the cache.
        if (SUCCEEDED(IncreaseCacheSize(&dwCacheSizeKB)))
        {
            hr = INET_S_AGENT_INCREASED_CACHE_SIZE;
        }
        else
        {
            // We need to ask the user.
            if ((++ m_nAutoCacheSizeTimesAsked) > MAX_AUTOCACHESIZE_ASK)
            {
                hr = E_ABORT;       // Already bothered them enough.
            }
            else
            {
                // Let's ask the user. We need unwind our call stack now, however.
                // Tell the throttler to ask the user
                if (SUCCEEDED(CreateThrottlerWnd()))
                {
                    PostMessage(m_hwndThrottler, WM_THROTTLER_AUTOCACHESIZE_ASK, 0, dwCacheSizeKB);
                    m_fAutoCacheSizePending = TRUE;
                    hr = E_PENDING;
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
    } // !m_fAutoCacheSizePending

    if (hr == E_PENDING)
    {
        // Mark this agent as paused.
        int queueIndex = GetCookieIndexInQueue(pSubscriptionCookie);

        ASSERT(queueIndex >= 0);

        if (queueIndex >= 0)
        {
            m_updateQueue[queueIndex]->m_dwRunState &= ~RS_UPDATING;
            m_updateQueue[queueIndex]->m_dwRunState |= RS_SUSPENDED;
            m_updateQueue[queueIndex] = NULL;
        }
    }

    return hr;
}

HRESULT CThrottler::AutoCacheSizeAskUser(DWORD dwCacheSizeKB)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    ASSERT(m_fAutoCacheSizePending);
    ASSERT(dwCacheSizeKB);
    ASSERT(m_hwndParent);

    HRESULT hr = E_FAIL;

    // Keep-Alive
    AddRef();

    if (IDOK == ShellMessageBox(MLGetHinst(),
                    m_hwndParent,
                    MAKEINTRESOURCE(IDS_CACHELIMIT_MESSAGE), 
                    MAKEINTRESOURCE(IDS_CACHELIMIT_TITLE),
                    MB_OKCANCEL | MB_SETFOREGROUND | MB_ICONQUESTION))
    {
        // Come up with a good cache size increase and resume agents
        m_dwAutoCacheSizeIncrease = dwCacheSizeKB / 4;

        if (m_dwAutoCacheSizeIncrease < MIN_CACHE_INCREASE)
        {
            m_dwAutoCacheSizeIncrease = MIN_CACHE_INCREASE;
        }

        m_dwMaxAutoCacheSize = dwCacheSizeKB + (2 * m_dwAutoCacheSizeIncrease);

        if (SUCCEEDED(IncreaseCacheSize(NULL)))
        {
            hr = S_OK;
        }
    }
    else
    {
        // Abort agents
    }

    m_fAutoCacheSizePending = FALSE;

    if (FAILED(hr))
    {
        // Hey! This sucks dude. User said no (or we couldn't increase the cache).
        ActuallyAbortAll();
    }
    else
    {
        FillTheQueue();
    }

    Release();

    return hr;
}

// Auto-increase cache size if user previously ok'd it
HRESULT CThrottler::IncreaseCacheSize(DWORD *pdwNewCacheSizeKB)
{
    ASSERT(GetCurrentThreadId() == m_dwThreadId);
    LPINTERNET_CACHE_CONFIG_INFOA pCCI=NULL;
    DWORD  dwSizeInKB=0, dwPercent;
    DWORD  dwNewSizeInKB=0;
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(GetCacheInfo(&pCCI, &dwSizeInKB, &dwPercent)))
    {
        if (dwSizeInKB < m_dwMaxAutoCacheSize)
        {
            ASSERT(m_dwAutoCacheSizeIncrease > 1023);   // At least 1 meg
            if (m_dwAutoCacheSizeIncrease)
            {
                // We still have room to increase cache without asking the user. Use it.
                dwNewSizeInKB = dwSizeInKB + m_dwAutoCacheSizeIncrease;

                if (dwNewSizeInKB > m_dwMaxAutoCacheSize)
                {
                    dwNewSizeInKB = m_dwMaxAutoCacheSize;
                }

                if (SUCCEEDED(SetCacheSize(pCCI, dwNewSizeInKB)))
                {
                    hr = S_OK;
                    dwSizeInKB = dwNewSizeInKB;
                    DBG("Throttler just increased TIF cache size");
                }
            }
        }

        MemFree(pCCI);
    }

    if (pdwNewCacheSizeKB)
    {
        *pdwNewCacheSizeKB = dwSizeInKB;
    }

    return hr;
}

