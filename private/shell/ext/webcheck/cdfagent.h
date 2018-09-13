#ifndef _CDFAGENT_H
#define _CDFAGENT_H

#include "msxml.h"

class CProcessElement;
class CProcessRoot;

class CUrlTrackingCache;

class CRunDeliveryAgentSink
{
public:
    // OnAgentProgress not currently called
    virtual HRESULT OnAgentProgress()
                    { return E_NOTIMPL; }
    // OnAgentEnd called when agent is complete. fSynchronous means that StartAgent call
    //  has not yet returned; hrResult will be returned from StartAgent
    virtual HRESULT OnAgentEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                               long lSizeDownloaded, HRESULT hrResult, LPCWSTR wszResult,
                               BOOL fSynchronous)
                    { return E_NOTIMPL; }
};

class CProcessElementSink
{
public:
    virtual HRESULT OnChildDone(CProcessElement *pChild, HRESULT hr) = 0;
    virtual LPCWSTR GetBaseUrl() = 0;   // Returned pointer doesn't need to get freed
    virtual BOOL    IsGlobalLog() = 0;
};

typedef struct CDF_TIME
{
    WORD   wDay;
    WORD   wHour;
    WORD   wMin;
    WORD   wReserved;
    DWORD  dwConvertedMinutes;      // Day/Hour/Min in Minutes
} CDF_TIME;


//////////////////////////////////////////////////////////////////////////
//
// Channel Agent object
//
//////////////////////////////////////////////////////////////////////////
class CChannelAgent : public CDeliveryAgent,
                      public CUrlDownloadSink,
                      public CProcessElementSink
{
    friend CProcessElement; // for SendUpdateProgress
    friend CProcessRoot;    // for laziness
protected:
// properties
    LPWSTR      m_pwszURL;
    DWORD       m_dwChannelFlags;

// used during updating
    CUrlDownload    *m_pCurDownload;
    IExtractIcon    *m_pChannelIconHelper;

    BOOL            m_fHasInitCookie;   // One time deal, don't try again.

    VARIANT         m_varChange;

    GROUPID         m_llCacheGroupID;
    GROUPID         m_llOldCacheGroupID;

    // other agent flags
    enum {
        FLAG_CDFCHANGED  =  0x80000000  // did the CDF change?
    };

private:
    ~CChannelAgent(void);

public:
    CChannelAgent(void);

    // CUrlDownloadSink
    HRESULT     OnAuthenticate(HWND *phwnd, LPWSTR *ppszUsername, LPWSTR *ppszPassword);
    HRESULT     OnDownloadComplete(UINT iID, int iError);

    // CProcessElementSink
    HRESULT     OnChildDone(CProcessElement *pChild, HRESULT hr);
    LPCWSTR     GetBaseUrl() { return GetUrl(); }
    BOOL        IsGlobalLog() { return FALSE; }

    // virtual functions overriding CDeliveryAgent
    HRESULT     AgentPause(DWORD dwFlags);
    HRESULT     AgentResume(DWORD dwFlags);
    HRESULT     AgentAbort(DWORD dwFlags);
    STDMETHODIMP GetIconLocation(UINT, LPTSTR, UINT, int *, UINT *);
    STDMETHODIMP Extract(LPCTSTR, UINT, HICON *, HICON *, UINT);

    LPCWSTR     GetUrl() { return m_pwszURL; }
    ISubscriptionItem *GetStartItem() { return m_pSubscriptionItem; }

    BOOL        IsChannelFlagSet(DWORD dwFlag) { return dwFlag & m_dwChannelFlags; }

    void        SetScreenSaverURL(LPCWSTR pwszURL);

protected:
    // CDeliveryAgent overrides
    HRESULT     ModifyUpdateEnd(ISubscriptionItem *pEndItem, UINT *puiRes);
    HRESULT     StartOperation();
    HRESULT     StartDownload();
    void        CleanUp();

    // Used during updates
    CProcessRoot   *m_pProcess;
    LPWSTR      m_pwszScreenSaverURL;

public:
    DWORD           m_dwMaxSizeKB;
};

//////////////////////////////////////////////////////////////////////////
//
// CRunDeliveryAgent object
// Will run a delivery agent and host it for you
// Create, call Init, then call StartAgent
// Use static function SafeRelease to safely release this class.
//
//////////////////////////////////////////////////////////////////////////
class CRunDeliveryAgent : public ISubscriptionAgentEvents
{
protected:
    virtual ~CRunDeliveryAgent();

    CRunDeliveryAgentSink *m_pParent;

    ULONG           m_cRef;

    ISubscriptionItem         *m_pItem;
    ISubscriptionAgentControl *m_pAgent;

    HRESULT     m_hrResult;
    BOOL        m_fInStartAgent;

    CLSID       m_clsidDest;

    void        CleanUp();

public:
    CRunDeliveryAgent();

    HRESULT Init(CRunDeliveryAgentSink *pParent, ISubscriptionItem *pItem, REFCLSID rclsidDest);

    void LeaveMeAlone() { m_pParent = NULL; }

inline static void SafeRelease(CRunDeliveryAgent * &pThis)
{ if (pThis) { pThis->m_pParent=NULL; pThis->Release(); pThis=NULL; } }

static HRESULT CreateNewItem(ISubscriptionItem **ppItem, REFCLSID rclsidAgent);

    // StartAgent will return E_PENDING if agent is running. Otherwise it will return
    //  synchronous result code from agent.
    HRESULT     StartAgent();

    HRESULT     AgentPause(DWORD dwFlags);
    HRESULT     AgentResume(DWORD dwFlags);
    HRESULT     AgentAbort(DWORD dwFlags);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **ppunk);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ISubscriptionAgentEvents members
    STDMETHODIMP UpdateBegin(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie);
    STDMETHODIMP UpdateProgress(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                        long lSizeDownloaded, long lProgressCurrent, long lProgressMax,
                        HRESULT hrStatus, LPCWSTR wszStatus);
    STDMETHODIMP UpdateEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                            long lSizeDownloaded,
                            HRESULT hrResult, LPCWSTR wszResult);
    STDMETHODIMP ReportError(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                             HRESULT hrError, LPCWSTR wszError);
};

class CChannelAgentHolder : public CRunDeliveryAgent,
                            public IServiceProvider
{
protected:
    ~CChannelAgentHolder();

public:
    CChannelAgentHolder(CChannelAgent *pChannelAgent, CProcessElement *pProcess);

    // IUnknown
    STDMETHODIMP        QueryInterface(REFIID riid, void **ppunk);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ServiceProvider
    STDMETHODIMP        QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

protected:
    CChannelAgent *m_pChannelAgent;
    CProcessElement *m_pProcess;
};


//////////////////////////////////////////////////////////////////////////
//
// Process Element objects
//
// User of this class
//  1) Creates & passes in self & element
//  2) Calls Run
//  3) If E_PENDING, will receive call back "OnChildDone"
//
// The purpose of this class is simply to allow us to save our state of
//  walking the XML OM, so that we can host another deliver agent
//  (webcrawler). This requires us to return out to the thread's message
//  pump after sending the "agent start" to the web crawler.
// The if a webcrawl is initiated the class creates its own sink. Classes
//  also keep references to their spawned enumerations in case of an
//  abort, which comes from the root element (CProcessRoot instance)
//
//////////////////////////////////////////////////////////////////////////
class CProcessElement : public CProcessElementSink, public CRunDeliveryAgentSink
{
public:
    CProcessElement(CProcessElementSink *pParent, CProcessRoot *pRoot, IXMLElement *pEle);
    ~CProcessElement();

    // From CRunDeliveryAgent
    HRESULT OnAgentEnd(const SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                       long lSizeDownloaded, HRESULT hrResult, LPCWSTR wszResult,
                       BOOL fSynchronous);

    typedef HRESULT (CChannelAgent::*PFNHANDLETAG)(LPCWSTR pwszTagName, IXMLElement *pEle);
    typedef struct
    {
        LPCWSTR         pwszTagName;
        PFNHANDLETAG    pfnHandleTag;
    } TAGTABLE;

    // E_FAIL, E_PENDING, or S_OK
    virtual HRESULT    Run();

    // Called when E_PENDING DoChild returns (from m_pCurChild)
    HRESULT     OnChildDone(CProcessElement *pChild, HRESULT hr);


    HRESULT     Pause(DWORD dwFlags);
    HRESULT     Resume(DWORD dwFlags);
    HRESULT     Abort(DWORD dwFlags);

    IXMLElement *GetCurrentElement() { return m_pChildElement; }

protected:
    // Returns E_PENDING, or S_OK if enumeration complete
    HRESULT     DoEnumeration();

    // E_PENDING if webcrawl pending
    HRESULT     DoChild(CProcessElement *pChild);

    // Should return E_PENDING, or S_OK if processing done
    // Can return E_ABORT to abort entire CDF processing
    virtual HRESULT ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem) = 0;

    // Called by DoEnumeration when it's done. Return value ignored.
    virtual HRESULT EnumerationComplete() { return S_OK; }

    // E_PENDING, or E_FAIL
    HRESULT     DoDeliveryAgent(ISubscriptionItem *pItem, REFCLSID rclsid, LPCWSTR pwszURL=NULL);
    HRESULT     DoWebCrawl(IXMLElement *pItem, LPCWSTR pwszURL=NULL);
    HRESULT     DoSoftDist(IXMLElement *pItem);

    BOOL    ShouldDownloadLogo(IXMLElement *pLogo);

    // If relative url, will combine with most recent base URL
    // *ppwszRetUrl should be NULL & will be LocalAlloced if needed
    HRESULT     CombineWithBaseUrl(LPCWSTR pwszUrl, LPWSTR *ppwszRetUrl);

    // Returned pointer doesn't need to get freed
    LPCWSTR     GetBaseUrl() { return m_pParent->GetBaseUrl(); }
    BOOL        IsGlobalLog() { return m_pParent->IsGlobalLog(); }

    CProcessRoot    *m_pRoot;

    CProcessElement *m_pCurChild;
    IXMLElementCollection *m_pCollection;
    long            m_lIndex;
    long            m_lMax;
    BOOL            m_fStartedEnumeration;
    BOOL            m_fSentEnumerationComplete;

    IXMLElement    *m_pElement;
    IXMLElement    *m_pChildElement;

    CProcessElementSink *m_pParent;

    CRunDeliveryAgent   *m_pRunAgent;
};

class CProcessRoot : public CProcessElement
{
public:
    CProcessRoot(CChannelAgent *pParent, IXMLElement *pRoot);
    ~CProcessRoot();

    CChannelAgent      *m_pChannelAgent;
    DWORD               m_dwCurSizeKB;
    int                 m_iTotalStarted;
    BOOL                m_fMaxSizeExceeded;

protected:
    ISubscriptionItem  *m_pDefaultStartItem;
    CUrlTrackingCache  *m_pTracking;

public:
    HRESULT     CreateStartItem(ISubscriptionItem **ppItem);
    IUnknown   *DefaultStartItem() { return m_pDefaultStartItem; }

    HRESULT     Run();

    // Called when E_PENDING DoChild returns (from m_pCurChild, a CProcessChannel)
    HRESULT     OnChildDone(CProcessElement *pChild, HRESULT hr);
    HRESULT     OnAgentEnd(const SUBSCRIPTIONCOOKIE *, long, HRESULT, LPCWSTR, BOOL);

    BOOL        IsPaused() { return m_pChannelAgent->IsPaused(); }
    BOOL        IsChannelFlagSet(DWORD dw) { return m_pChannelAgent->IsChannelFlagSet(dw); }

//  HRESULT     ProcessLogin(IXMLElement *pElement);
    HRESULT     DoTrackingFromItem(IXMLElement *pItem, LPCWSTR pwszUrl, BOOL fForceLog);
    HRESULT     DoTrackingFromLog(IXMLElement *pItem);

protected:
    HRESULT     ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem);

    LPCWSTR     GetBaseUrl() { return m_pChannelAgent->GetUrl(); }
};

class CProcessChannel : public CProcessElement
{
public:
    CProcessChannel(CProcessElementSink *pParent, CProcessRoot *pRoot, IXMLElement *pItem);
    ~CProcessChannel();
 
    HRESULT     Run();

    void        SetGlobalLogFlag(BOOL flag) { m_fglobalLog = flag; }

protected:
    HRESULT     ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem);

    LPCWSTR     GetBaseUrl() { if (m_bstrBaseUrl) return m_bstrBaseUrl; return m_pParent->GetBaseUrl(); }

    BOOL        IsGlobalLog() { return m_fglobalLog; }

    HRESULT     CheckPreCache();

    BOOL        m_fDownloadedHREF;
    BSTR        m_bstrBaseUrl;
    BOOL        m_fglobalLog;
};

class CProcessItem : public CProcessElement
{
public:
    CProcessItem(CProcessElementSink *pParent, CProcessRoot *pRoot, IXMLElement *pItem);
    ~CProcessItem();

protected:
    HRESULT     ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem);
    HRESULT     EnumerationComplete();

    BSTR        m_bstrAnchorURL;
    BOOL        m_fScreenSaver;
    BOOL        m_fDesktop;
    BOOL        m_fEmail;
};

class CProcessSchedule : public CProcessElement
{
public:
    CProcessSchedule(CProcessElementSink *pParent, CProcessRoot *pRoot, IXMLElement *pItem);
 
    HRESULT     Run();

protected:
    HRESULT     ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem);
    HRESULT     EnumerationComplete();

    CDF_TIME    m_timeInterval;
    CDF_TIME    m_timeEarliest;
    CDF_TIME    m_timeLatest;

    SYSTEMTIME  m_stStartDate;
    SYSTEMTIME  m_stEndDate;

public:
    TASK_TRIGGER m_tt;
};

class CExtractSchedule : public CProcessElement
{
public:
    CExtractSchedule(IXMLElement *pEle, CExtractSchedule *m_pExtractRoot);
    HRESULT     ProcessItemInEnum(LPCWSTR pwszTagName, IXMLElement *pItem);
    HRESULT     GetTaskTrigger(TASK_TRIGGER *ptt);

    virtual HRESULT    Run();

    TASK_TRIGGER m_tt;
    CExtractSchedule *m_pExtractRoot;

protected:    
    LPCWSTR     GetBaseUrl() { return NULL; }
};

#endif // _CDFAGENT_H
