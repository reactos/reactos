#ifndef _WEBCRAWL_H
#define _WEBCRAWL_H

#include "strlist.h"

//////////////////////////////////////////////////////////////////////////
//
// Webcrawler object
//
//////////////////////////////////////////////////////////////////////////
class CCodeBaseHold 
{
public:
    LPWSTR          szDistUnit;
    DWORD           dwVersionMS;
    DWORD           dwVersionLS;
    DWORD           dwFlags;
};

class CWebCrawler : public CDeliveryAgent,
                    public CUrlDownloadSink,
                    public CRunDeliveryAgentSink
{
protected:
    class  CDownloadNotify;

public:
    // internal flag used to run in offline mode
    enum { WEBCRAWL_PRIV_OFFLINE_MODE = 0x80000000 };
protected:
// properties
    BSTR            m_bstrBaseURL;
    DWORD           m_dwRecurseFlags;
    DWORD           m_dwRecurseLevels;
    DWORD           m_dwMaxSize;            
    LPTSTR          m_pszLocalDest;         // local destination (instead of cache)

// other data
    CWCStringList  *m_pPages;          // always valid during update.
    CWCStringList  *m_pRobotsTxt;      // array of robots.txt arrays, may be NULL
    CWCStringList  *m_pPendingLinks;   // Links from last page to be added to m_pPages
    CWCStringList  *m_pDependencyLinks;// Links from last page to be downloaded now
    CWCStringList  *m_pCodeBaseList;   // List of CodeBase URL's to Crawl
                                       // Dword is ptr to CCodeBaseHold

    CRITICAL_SECTION m_critDependencies;
    CWCStringList  *m_pDependencies;   // all dependencies downloaded
    int             m_iDependenciesProcessed;

    DWORD           m_dwPendingRecurseLevel;   // # to recurse from pending links

    DWORD           m_dwCurSize;    // currently downloaded in BYTES

    GROUPID         m_llCacheGroupID;
    GROUPID         m_llOldCacheGroupID;

    IExtractIcon*   m_pUrlIconHelper;

    int             m_iPagesStarted;    // # m_pPages started
    int             m_iRobotsStarted;   // # m_pRobotsTxt started
    int             m_iDependencyStarted;// # m_pDependencyLinks started
    int             m_iTotalStarted;    // # any toplevel url started
    int             m_iCodeBaseStarted; // # of codebases started

    BSTR            m_bstrHostName;     // host name from first url

    long            m_lMaxNumUrls;      // is -1 until we know total # pages

    int             m_iDownloadErrors;  // have we had any download failures?
    int             m_iSkippedByRobotsTxt; // how many skipped by robots.txt?

    CUrlDownload   *m_pCurDownload;     // current download
    CDownloadNotify     *m_pDownloadNotify; // to get urls downloaded on a page

    int             m_iCurDownloadStringIndex;
    CWCStringList  *m_pCurDownloadStringList;   // can be: m_pRobotsTxt, Pages, CodeBaseList

    int             m_iNumPagesDownloading; // 0 or 1

    BOOL            m_fHasInitCookie;   // One time deal, don't try again.

    // For change detection
    VARIANT         m_varChange;

    CRunDeliveryAgent *m_pRunAgent;      // host CDL/Channel agent
    BOOL            m_fCDFDownloadInProgress;

    // other flags
    enum {
        FLAG_CRAWLCHANGED = 0x80000000, // have we found a change in the crawl?
        FLAG_HEADONLY     = 0x40000000, // should we only get the HEAD data?
    };

// private member functions
    BOOL        IsRecurseFlagSet(DWORD dwFlag) { return dwFlag & m_dwRecurseFlags; }

static HRESULT CheckLink(IUnknown *punkItem, BSTR *pbstrItem, DWORD_PTR dwThis, DWORD *pdwStringData);
static HRESULT CheckFrame(IUnknown *punkItem, BSTR *pbstrItem, DWORD_PTR dwBaseUrl, DWORD *pdwStringData);
static HRESULT CheckImageOrLink(IUnknown *punkItem, BSTR *pbstrItem, DWORD_PTR dwEnumDep, DWORD *pdwStringData);

    HRESULT     MatchNames(BSTR bstrName, BOOL fPassword);
    HRESULT     FindAndSubmitForm(void);

    void        CheckOperationComplete(BOOL fOperationComplete);

    void        FreeRobotsTxt();
    void        FreeCodeBaseList();

private:
    ~CWebCrawler(void);

public:
    CWebCrawler(void);
   
    // CUrlDownloadSink
    HRESULT     OnDownloadComplete(UINT iID, int iError);
    HRESULT     OnClientPull(UINT iID, LPCWSTR pwszOldURL, LPCWSTR pwszNewURL);
    HRESULT     OnAuthenticate(HWND *phwnd, LPWSTR *ppszUsername, LPWSTR *ppszPassword);
    HRESULT     OnOleCommandTargetExec(const GUID *pguidCmdGroup, DWORD nCmdID,
                                DWORD nCmdexecopt, VARIANTARG *pvarargIn, 
                                VARIANTARG *pvarargOut);
    HRESULT     GetDownloadNotify(IDownloadNotify **ppOut);

    // virtual functions overriding CDeliveryAgent
    HRESULT     AgentPause(DWORD dwFlags);
    HRESULT     AgentResume(DWORD dwFlags);
    HRESULT     AgentAbort(DWORD dwFlags);
    STDMETHODIMP GetIconLocation(UINT, LPTSTR, UINT, int *, UINT *);
    STDMETHODIMP Extract(LPCTSTR, UINT, HICON *, HICON *, UINT);

    // CRunDeliveryAgentSink
    HRESULT     OnAgentEnd(const SUBSCRIPTIONCOOKIE *, long, HRESULT, LPCWSTR, BOOL);

protected:
    // CDeliveryAgent overrides
    HRESULT     ModifyUpdateEnd(ISubscriptionItem *pEndItem, UINT *puiRes);
    HRESULT     StartOperation();
    HRESULT     StartDownload();
    void        CleanUp();

    void        _CleanUp();

    // members used during download
    HRESULT     GetRealUrl(int iPageIndex, LPWSTR *ppwszThisUrl);
    HRESULT     MakePageStickyAndGetSize(LPCWSTR pwszURL, DWORD *pdwSize, BOOL *pfDiskFull);
    HRESULT     GetLinksFromPage();
    HRESULT     GetDependencyLinksFromPage(LPCWSTR pwszThisUrl, DWORD dwRecurse);
    HRESULT     ProcessDependencyLinks(CWCStringList **ppslUrls, int *piStarted);
    HRESULT     ProcessPendingLinks();
    HRESULT     ParseRobotsTxt(LPCWSTR pwszRobotsTxtURL, CWCStringList **ppslRet);
    HRESULT     GetRobotsTxtIndex(LPCWSTR pwszUrl, BOOL fAddToList, DWORD *pdwRobotsTxtIndex);
    HRESULT     ValidateWithRobotsTxt(LPCWSTR pwszUrl, int iRobotsIndex, BOOL *pfAllow);


    HRESULT     StartNextDownload();
    HRESULT     StartCDFDownload(WCHAR *pwszCDFURL, WCHAR *pwszBaseUrl);
    HRESULT     ActuallyStartDownload(CWCStringList *pslUrls, int iIndex, BOOL fReStart=FALSE);
    HRESULT     ActuallyDownloadCodeBase(CWCStringList *pslUrls, int iIndex, BOOL fReStart=FALSE);

static HRESULT  GetHostName(LPCWSTR pwszThisUrl, BSTR *pbstrHostName);

    inline HRESULT GetChannelItem(ISubscriptionItem **ppChannelItem);

public:
    // Callbacks from CDownloadNotify (free threaded)
    HRESULT DownloadStart(LPCWSTR pchUrl, DWORD dwDownloadId, DWORD dwType, DWORD dwReserved);
    HRESULT DownloadComplete(DWORD dwDownloadId, HRESULT hrNotify, DWORD dwReserved);

protected:
    class CDownloadNotify : public IDownloadNotify
    {
    public:
        CDownloadNotify(CWebCrawler *pParent);
        ~CDownloadNotify();

        void LeaveMeAlone();

    protected:
        long             m_cRef;
        CWebCrawler     *m_pParent; // we keep a reference
        CRITICAL_SECTION m_critParent;

    public:
        // IUnknown members
        STDMETHODIMP         QueryInterface(REFIID riid, void **ppunk);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IDownloadNotify
        STDMETHODIMP         DownloadStart(LPCWSTR pchUrl, DWORD dwDownloadId, DWORD dwType, DWORD dwReserved);
        STDMETHODIMP         DownloadComplete(DWORD dwDownloadId, HRESULT hrNotify, DWORD dwReserved);
    };
};

//////////////////////////////////////////////////////////////////////////
//
//  CHelperOM
//
// Helps with MSHTML object model
//////////////////////////////////////////////////////////////////////////

class CHelperOM
{
    IHTMLDocument2  *m_pDoc;

public:
    typedef enum {
        CTYPE_LINKS,    // Get all links (<a href>) on a page
        CTYPE_MAPS,     // Get all maps on page
        CTYPE_MAP,      // Get all links within a map
        CTYPE_META,     // Get meta tags (name\ncontent)
        CTYPE_FRAMES,   // Get all frame urls on a page
    } CollectionType;

    typedef HRESULT (*PFNHELPERCALLBACK)(IUnknown *punkItem, /*inout*/BSTR *pbstrURL, DWORD_PTR dwCBData, DWORD *pdwStringData);
    typedef PFNHELPERCALLBACK PFN_CB;

public:
    CHelperOM(IHTMLDocument2 *pDoc);
    ~CHelperOM();

    static HRESULT GetTagCollection(
                        IHTMLDocument2          *pDoc,
                        LPCWSTR                  wszTagName,
                        IHTMLElementCollection **ppCollection);

//  static HRESULT WinFromDoc(IHTMLDocument2 *pDoc, IHTMLWindow2 **ppWin);

    static HRESULT GetCollection (IHTMLDocument2 *pDoc, CWCStringList *psl, CollectionType Type, PFN_CB pfnCB, DWORD_PTR dwData);
    static HRESULT EnumCollection(IHTMLElementCollection *pCollection,
                                  CWCStringList *pStringList, CollectionType Type, PFN_CB pfnCB, DWORD_PTR dwData);

    HRESULT GetTagCollection(LPCWSTR wszTagName, IHTMLElementCollection **ppCollection)
    { return GetTagCollection(m_pDoc, wszTagName, ppCollection); }
    HRESULT	GetCollection(CWCStringList *psl, CollectionType Type, PFN_CB pfnCB, DWORD_PTR dwData)
    { return GetCollection(m_pDoc, psl, Type, pfnCB, dwData); }

protected:
    static HRESULT _GetCollection(IHTMLDocument2 *pDoc, CWCStringList *psl, CollectionType Type, PFN_CB pfnCB, DWORD_PTR dwData);
};

#endif _WEBCRAWL_H
