#ifndef __DOWNLD_H
#define __DOWNLD_H

#include <mshtmdid.h>

#define URLDL_WNDCLASS  TEXT("WCUrlDlClass")

class CUrlDownload_BSC;

#define ACCEPT_LANG_MAX     256

// Options for BeginDownloadURL2
typedef enum {
    BDU2_BROWSER,    // always download into the browser
    BDU2_URLMON,     // always download with urlmon only
    BDU2_SMART,      // browser if HTML; aggressive guessing
    BDU2_SNIFF,      // browser if HTML; no guessing (urlmon then browser)
    BDU2_HEADONLY,   // get HEAD info only (including Last-Modified time)
} BDUMethod;

typedef DWORD BDUOptions;

// BDUOptions
#define BDU2_NONE                       0
#define BDU2_NEEDSTREAM                 1   // keep an istream around from bdu2_urlmon download
#define BDU2_DOWNLOADNOTIFY_REQUIRED    2   // require IDownloadNotify callbacks for MSHTML
#define BDU2_FAIL_IF_NOT_HTML           4   // only download if url is html (can't use w/BDU2_BROWSER)

// OnDownloadComplete error codes
#define BDU2_ERROR_NONE         0
#define BDU2_ERROR_GENERAL      1
#define BDU2_ERROR_ABORT        2
#define BDU2_ERROR_MAXSIZE      3
#define BDU2_ERROR_TIMEOUT      4
#define BDU2_ERROR_NOT_HTML     5


// CUrlDowload hosts one browser and can handle one download at a time.
//
// CUrlDownloadSink is defined in private.h
//
// Use of class CUrlDownload:
//
// 1) Create and AddRef it
// 1.5) call put_Flags() to set the bind status callback IBrowseControl::Flags
// 2) Call BeginDownloadURL2 to start a download
// 3) Retrieve notifications through CUrlDownloadSink
// 4) Call BeginDownloadURL2 to start another download, reusing browser
// 5) Call DoneDownloading() when finished
// 6) Release()

// DoneDownloading() MUST be called before Release() or the CUrlDownload instance may
//  continue to receive notifications from the browser and attempt to pass them to
//  the parent. It unhooks itself as soon as OnProgress(-1) is received. But be safe.

// AbortDownload() may cause DownloadComplete(TRUE) notification
//   to be sent to the CUrlDownloadSink

// See webcrawl.h and webcrawl.cpp for example
class CUrlDownload :  public IOleClientSite         // e_notimpl
                    , public IPropertyNotifySink    // for readystate change notifications
                    , public IOleCommandTarget      // for client pull callbacks
                    , public IDispatch              // for ambient properties
                    , public IServiceProvider       // for IAuthenticate and IHlinkFrame
                    , public IAuthenticate          // for Basic and NTLM authentication
                    , public IHlinkFrame            // for catching the post of a form
                    , public IInternetSecurityManager // for allowing the post of a form
{
    friend CUrlDownload_BSC;
    
private:
    ~CUrlDownload();

public:
    CUrlDownload(CUrlDownloadSink *pParent, UINT iID=0);

    void LeaveMeAlone() { m_pParent=NULL; }

    void SetFormSubmitted(BOOL fFormSubmitted) { m_fFormSubmitted = fFormSubmitted; StartTimer(); }
    BOOL GetFormSubmitted(void) { return m_fFormSubmitted; }

    // An E_ return code from this function may be ignored if desired. The
    //  client's OnDownloadComplete will be called with fAborted==TRUE after this
    //  function returns with an error value.
    HRESULT BeginDownloadURL2(LPCWSTR, BDUMethod, BDUOptions, LPTSTR, DWORD);

    HRESULT SetDLCTL(long lFlags);  // DLCTL flags used for browser control

    HRESULT AbortDownload(int iErrorCode=-1);   // S_OK, S_FALSE, E_FAIL

    HRESULT GetRealURL(LPWSTR *ppwszURL);   // Gets URL accounting for any and all redirections (MemFree)

    HRESULT GetScript(IHTMLWindow2 **pWin);    // Will cache an *additional* reference internally
    void    ReleaseScript() { SAFERELEASE(m_pScript); } // Releases internal reference

    HRESULT GetDocument(IHTMLDocument2 **ppDoc);

    HRESULT GetStream(IStream **ppStm); // Only if BDU2_NEEDSTREAM was specified
    void    ReleaseStream() { SAFERELEASE(m_pStm); } // Release our internal reference

    HRESULT GetLastModified(SYSTEMTIME *pstLastModified);   // Only if BDU2_HEADONLY was used
    HRESULT GetResponseCode(DWORD *pdwResponseCode);

    void    DoneDownloading();  // Call before releasing. Will destroy browser and windows.
    void    DestroyBrowser();   // Destroy hosted browser, leave all else alone

    LPCWSTR GetUserAgent();     // Get our webcrawler user-agent string

    // URL manipulation functions
static HRESULT StripAnchor(LPWSTR lpURL);
static BOOL IsHtmlUrl(LPCWSTR lpURL); // TRUE (yes) or FALSE (don't know)
static BOOL IsNonHtmlUrl(LPCWSTR lpURL); // TRUE (yes) or FALSE (don't know)
static BOOL IsValidURL(LPCWSTR lpURL);  // TRUE (get it) or FALSE (skip it)

    // Should only be called from CUrlDownloadMsgProc
    BOOL HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //      IDispatch (ambient properties)
    STDMETHODIMP         GetTypeInfoCount(UINT *pctinfo);
    STDMETHODIMP         GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHODIMP         GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames,
                                LCID lcid, DISPID *rgdispid);
    STDMETHODIMP         Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pdispparams, VARIANT *pvarResult,
                                EXCEPINFO *pexcepinfo, UINT *puArgErr);

    // IOleClientSite
    STDMETHODIMP        SaveObject(void);
    STDMETHODIMP        GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk);
    STDMETHODIMP        GetContainer(IOleContainer **ppContainer);
    STDMETHODIMP        ShowObject(void);
    STDMETHODIMP        OnShowWindow(BOOL fShow);
    STDMETHODIMP        RequestNewObjectLayout(void);

    // IPropertyNotifySink
    STDMETHODIMP        OnChanged(DISPID dispID);
    STDMETHODIMP        OnRequestEdit(DISPID dispID);

    // IOleCommandTarget
    STDMETHODIMP         QueryStatus(const GUID *pguidCmdGroup,
                                     ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
    STDMETHODIMP         Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
                              DWORD nCmdexecopt, VARIANTARG *pvaIn,
                              VARIANTARG *pvaOut);

    // IServiceProvider
    STDMETHODIMP        QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // IAuthenticate
    STDMETHODIMP        Authenticate(HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword);
    
    // IHlinkFrame
    STDMETHODIMP        SetBrowseContext(IHlinkBrowseContext *pihlbc);
    STDMETHODIMP        GetBrowseContext(IHlinkBrowseContext **ppihlbc);
    STDMETHODIMP        Navigate(DWORD grfHLNF, LPBC pbc, IBindStatusCallback *pibsc, IHlink *pihlNavigate);
    STDMETHODIMP        OnNavigate(DWORD grfHLNF, IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved);
    STDMETHODIMP        UpdateHlink(ULONG uHLID, IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName);

    // IInternetSecurityManager
    STDMETHODIMP        SetSecuritySite(IInternetSecurityMgrSite *pSite);
    STDMETHODIMP        GetSecuritySite(IInternetSecurityMgrSite **ppSite);
    STDMETHODIMP        MapUrlToZone(LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags);
    STDMETHODIMP        GetSecurityId(LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved);
    STDMETHODIMP        ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE __RPC_FAR *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved);
    STDMETHODIMP        QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved);
    STDMETHODIMP        SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags);
    STDMETHODIMP        GetZoneMappings(DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags);



protected:
    // main object stuff
    ULONG               m_cRef;
    UINT                m_iID;          // our ID for callbacks
    CUrlDownloadSink    *m_pParent;     // to pass on WebBrowserEvents
    HWND                m_hwndMe;
    UINT                m_nTimeout;     // Max timeout between notifications (seconds)

    // GetBrowser/CleanUpBrowser (browser download data)
    IPersistMoniker     *m_pPersistMk;
    IHTMLDocument2      *m_pDocument;
    IOleCommandTarget   *m_pOleCmdTarget;
    BOOL                m_fWaitingForReadyState;
    BOOL                m_fFormSubmitted;
    IConnectionPoint    *m_pCP;         // connection point for DIID_DWebBrowserEvents
    BOOL                m_fAdviseOn;    // our sink is hooked up? (ConnectionCookie valid)
    DWORD               m_dwConnectionCookie;
    BOOL                m_fBrowserValid;    // Browser pointing to 'current' URL?

    // UrlMon download data
    CUrlDownload_BSC    *m_pCbsc;
    BOOL                m_fbscValid;    // pCbsc alive for 'current' URL?
    IStream             *m_pStm;
    SYSTEMTIME          *m_pstLastModified;     // Last Modified time
    DWORD               m_dwResponseCode;
    WCHAR               m_achLang[ACCEPT_LANG_MAX];
    UINT                m_iLangStatus;  // 0=uninit, 1=init, 2=failed
    HRESULT             m_hrStatus;

    // General download data
    BDUMethod           m_iMethod;
    BDUOptions          m_iOptions;
    UINT_PTR            m_iTimerID;
    LPWSTR              m_pwszURL;      // gives us the current url after redirections
    BOOL                m_fSetResync;   // need RESYNCHRONIZE?
    DWORD               m_dwMaxSize;    // in bytes
    LPWSTR              m_pwszUserAgent;

    // IBrowseControl
    long                m_lBindFlags;

    // allow caching GetScript calls
    IHTMLWindow2        *m_pScript;

    // Client pull
    LPWSTR              m_pwszClientPullURL;
    int                 m_iNumClientPull;

    // methods that our bindstatuscallback calls back
    void        BSC_OnStopBinding(HRESULT hrStatus, IStream *pStm);
    void        BSC_OnStartBinding();
    void        BSC_OnProgress(ULONG ulProgress, ULONG ulProgressMax);
    void        BSC_FoundLastModified(SYSTEMTIME *pstLastModified);
    void        BSC_FoundMimeType(CLIPFORMAT cf);

    // other internal stuff
    HRESULT     CreateMyWindow();
    HRESULT     GetBrowser();   // Get browser and set us on connection point
    void        UnAdviseMe();   // Unhook our advise sink

    void        CleanUpBrowser();
    void        CleanUp();      // Clean up, including releasing browser

    void        StartTimer();    // for 60 second timeout
    void        StopTimer();

    LPCWSTR     GetAcceptLanguages();   // NULL if failed

    HRESULT     BeginDownloadWithUrlMon(LPCWSTR, LPTSTR, IEnumFORMATETC *);
    HRESULT     BeginDownloadWithBrowser(LPCWSTR);

    HRESULT     HandleRefresh(LPWSTR pwszEquivString, LPWSTR pwszContent, BOOL fDone);

    HRESULT     OnDownloadComplete(int iError);     // cancel timeout, send OnDownloadComplete

    HRESULT     ProgressBytes(DWORD dwBytes);       // Will abort if too many
};

class CUrlDownload_BSC :    public IBindStatusCallback,
                            public IHttpNegotiate,        // To set User-Agent
                            public IAuthenticate          // for Basic and NTLM authentication
{
public:
    // IUnknown methods
    STDMETHODIMP    QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef()    { return m_cRef++; }
    STDMETHODIMP_(ULONG)    Release()   { if (--m_cRef == 0) { delete this; return 0; } return m_cRef; }

    // IBindStatusCallback methods
    STDMETHODIMP    OnStartBinding(DWORD dwReserved, IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority);
    STDMETHODIMP    OnLowResource(DWORD dwReserved);
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                        LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc,
                        STGMEDIUM* pstgmed);
    STDMETHODIMP    OnObjectAvailable(REFIID riid, IUnknown* punk);

    // IHttpNegotiate methods
    STDMETHODIMP    BeginningTransaction(LPCWSTR szURL, LPCWSTR szHeaders,
                        DWORD dwReserved, LPWSTR *pszAdditionalHeaders);
    
    STDMETHODIMP    OnResponse(DWORD dwResponseCode, LPCWSTR szResponseHeaders,
                        LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders);
        
    // IAuthenticate methods
    STDMETHODIMP        Authenticate(HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword);
    


    // constructors/destructors
    CUrlDownload_BSC(BDUMethod, BDUOptions, LPTSTR);
    ~CUrlDownload_BSC();

    // other methods
    HRESULT         Abort();
    void            SetParent(CUrlDownload *pUrlDownload);

    // data members
protected:
    DWORD           m_cRef;
    IBinding*       m_pBinding;
    IStream*        m_pstm;
    LPTSTR          m_pszLocalFileDest;
    LPWSTR          m_pwszLocalFileSrc;
    BDUMethod       m_iMethod;
    BDUOptions      m_iOptions;
    BOOL            m_fSentMimeType;
    CUrlDownload    *m_pParent;
    BOOL            m_fTriedAuthenticate;
};

#endif
