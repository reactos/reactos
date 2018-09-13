#ifndef _throttle_h
#define _throttle_h

#include "factory.h"

#define MAX_RUNNING_ITEMS           3

#define STATE_USER_IDLE_BEGIN       1
#define STATE_USER_IDLE_END         2

#define WC_INTERNAL_S_PAUSED        (MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0xF000))
#define WC_INTERNAL_S_RESUMING      (MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0xF001))
#define WC_INTERNAL_S_PENDING       (MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0xF002))


void IdleBegin(HWND hwnd);
void IdleEnd(void);

class COfflineSync;
struct CSyncMgrNode;
struct CUpdateItem;

#define THROTTLER_WNDCLASS  TEXT("WCThrottlerClass")

class CThrottler : public ISubscriptionAgentEvents,
                   public ISubscriptionThrottler
{
public:
    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ISubscriptionAgentEvents members
    STDMETHODIMP UpdateBegin(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie);

    STDMETHODIMP UpdateProgress(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
        long lSizeDownloaded,
        long lProgressCurrent,
        long lProgressMax,
        HRESULT hrStatus,
        LPCWSTR wszStatus);

    STDMETHODIMP UpdateEnd(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie,
        long lSizeDownloaded,
        HRESULT hrResult,
        LPCWSTR wszResult);

    STDMETHODIMP ReportError(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
        HRESULT hrError, 
        LPCWSTR wszError);

    STDMETHODIMP GetSubscriptionRunState( 
        /* [in] */ DWORD dwNumCookies,
        /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies,
        /* [size_is][out] */ DWORD *pdwRunState);
    
    STDMETHODIMP AbortItems( 
        /* [in] */ DWORD dwNumCookies,
        /* [size_is][in] */ const SUBSCRIPTIONCOOKIE *pCookies);
    
    STDMETHODIMP AbortAll();

    static HRESULT GetThrottler(CThrottler **ppThrottler);

    HRESULT RunCookies(
        DWORD dwNumCookies,
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookies, 
        DWORD dwSyncFlags);

    HRESULT Advise(COfflineSync *pOfflineSync);
    HRESULT Unadvise(COfflineSync *pOfflineSync);

    ULONG ExternalAddRef();
    ULONG ExternalRelease();

    static LRESULT ThrottlerWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static void OnIdleStateChange(DWORD dwState);

private:

    enum { WM_THROTTLER_ABORTALL =WM_USER+99,
           WM_THROTTLER_ABORTITEM=WM_USER+100,
           WM_THROTTLER_AUTOCACHESIZE_ASK=WM_USER+101};

    DWORD               m_dwRegister;
#ifdef DEBUG
    DWORD               m_dwThreadId;
#endif    

    static CThrottler   *s_pThrottler;
    const static CFactoryData s_ThrottlerFactoryData;
  
    static HRESULT CreateInstance(IUnknown *punkOuter, IUnknown **ppunk);

    HRESULT RevokeClassObject();
    inline void ReportThrottlerError(const SUBSCRIPTIONCOOKIE *pCookie, HRESULT hrError, 
                                     LPCWSTR pwszErrMsg)
    {
        ReportError(pCookie, hrError, pwszErrMsg);
    }
    
    HRESULT AutoCacheSizeRequest(const SUBSCRIPTIONCOOKIE *pCookie);
    HRESULT AutoCacheSizeAskUser(DWORD dwCacheSizeKB);
    HRESULT IncreaseCacheSize(DWORD *pdwNewCacheSizeKB);

    CThrottler();
    ~CThrottler();

    ULONG           m_cRef;
    ULONG           m_cExternalRef;

    CSyncMgrNode    *m_pSyncMgrs;
    CUpdateItem     *m_pItemsHead;
    CUpdateItem     *m_pItemsTail;
    CUpdateItem     *m_updateQueue[MAX_RUNNING_ITEMS];
    int             m_nUpdating;
    HWND            m_hwndThrottler;
    HWND            m_hwndParent;

    BOOL            m_fAbortingAll:1;
    BOOL            m_fUserIsIdle:1;
    BOOL            m_fFillingTheQueue:1;
    BOOL            m_fForcedGlobalOnline:1;
    BOOL            m_fAutoDialed:1;
    BOOL            m_fAutoCacheSizePending:1;

    DWORD           m_dwMaxAutoCacheSize;
    DWORD           m_dwAutoCacheSizeIncrease;
    int             m_nAutoCacheSizeTimesAsked;

    typedef enum {NH_UPDATEBEGIN, NH_UPDATEPROGRESS, NH_UPDATEEND, NH_REPORTERROR};

    HRESULT AddItemToListTail(CUpdateItem *pAddItem);
    HRESULT RemoveItemFromList(CUpdateItem *pRemoveItem, BOOL fDelete);

    void OnIdleBegin();
    void OnIdleEnd();

    BOOL IsQueueSlotFree() { return m_nUpdating < ARRAYSIZE(m_updateQueue); }
    int GetFreeQueueSlot();
    int GetCookieIndexInQueue(const SUBSCRIPTIONCOOKIE *pCookie);
    void FillTheQueue();
    
    void FailedUpdate(HRESULT hr, const SUBSCRIPTIONCOOKIE *pCookie);
    void RunItem(int queueSlot, CUpdateItem *pUpdateItem);
    HRESULT CanScheduledItemRun(ISubscriptionItem *pSubsItem);

    STDMETHODIMP ActuallyAbortItems(DWORD dwNumCookies, const SUBSCRIPTIONCOOKIE *pCookies);
    STDMETHODIMP ActuallyAbortAll();

    HRESULT DoAbortItem(CUpdateItem *pUpdateItem);

    HRESULT CreateThrottlerWnd();

    HRESULT NotifyHandlers(int idCmd, const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, ...);
    HRESULT FindCookie(
        const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
        CUpdateItem **ppUpdateItem);
};

#include "offsync.h"

struct CSyncMgrNode
{
    CSyncMgrNode(COfflineSync *pOfflineSync, CSyncMgrNode *pNext) :
        m_pOfflineSync(pOfflineSync), 
        m_pNext(pNext) 
    {
        ASSERT(NULL != m_pOfflineSync);
    }

    ~CSyncMgrNode()
    {
        SAFERELEASE(m_pOfflineSync);
    }

    COfflineSync        *m_pOfflineSync;
    CSyncMgrNode        *m_pNext;
};

struct CUpdateItem
{
    CUpdateItem(const SUBSCRIPTIONCOOKIE& cookie,
                DWORD dwRunState) :
        m_cookie(cookie),
        m_dwRunState(dwRunState)
    {
        ASSERT(NULL == m_pNext);
        ASSERT(NULL == m_pSubsAgentCtl);
        ASSERT(CLSID_NULL != m_cookie);
        m_nMax = 128;
    }

    ~CUpdateItem()
    {
        SAFERELEASE(m_pSubsAgentCtl);
    }

    ISubscriptionAgentControl   *m_pSubsAgentCtl;
    SUBSCRIPTIONCOOKIE          m_cookie;
    DWORD                       m_dwRunState;
    CUpdateItem                 *m_pNext;
    LONG                        m_nMax;
};

#endif _throttle_h

