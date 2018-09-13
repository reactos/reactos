#ifndef _DOWNLD_HXX_
#define _DOWNLD_HXX_

#include <mshtmdid.h>
#include <mshtml.h>
#include <hlink.h>
#include "packager.h"

#ifndef GUIDSTR_MAX
// GUIDSTR_MAX is 39 and includes the terminating zero.
// == Copied from OLE source code =================================
// format for string form of GUID is (leading identifier ????)
// ????{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}
#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)
// ================================================================
#endif

// Trace and debug flags
#define TF_WEBCHECKCORE 0x00001000
//#define TF_SCHEDULER    0x00002000
#define TF_WEBCRAWL     0x00004000
//#define TF_FAVORITES    0x00008000
#define TF_CDFAGENT     0x00010000
#define TF_STRINGLIST   0x00020000
#define TF_URLDOWNLOAD  0x00040000
#define TF_DOWNLD       0x00080000
#define TF_DIALMON      0x00100000
#define TF_MAILAGENT    0x00200000
#define TF_TRAYAGENT    0x00400000
#define TF_SUBSFOLDER   0x00800000
#define TF_MEMORY       0x01000000
#define TF_UPDATEAGENT  0x02000000
#define TF_POSTAGENT    0x04000000
#define TF_DELAGENT     0x08000000
#define TF_TRACKCACHE   0x10000000
#define TF_SYNCMGR      0x20000000
#define TF_THROTTLER    0x40000000

#define PSM_QUERYSIBLINGS_WPARAM_RESCHEDULE 0XF000

#undef DBG
#define DBG(sz)             TraceMsg(TF_THISMODULE, sz)
#define DBG2(sz1, sz2)      TraceMsg(TF_THISMODULE, sz1, sz2)
#define DBG_WARN(sz)        TraceMsg(TF_WARNING, sz)
#define DBG_WARN2(sz1, sz2) TraceMsg(TF_WARNING, sz1, sz2)

#ifdef DEBUG
#define DBGASSERT(expr,sz)  do { if (!(expr)) TraceMsg(TF_WARNING, (sz)); } while (0)
#define DBGIID(sz,iid)      DumpIID(sz,iid)
#else
#define DBGASSERT(expr,sz)  ((void)0)
#define DBGIID(sz,iid)      ((void)0)
#endif

// shorthand
#ifndef SAFERELEASE
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; } else
#endif
#ifndef ATOMICRELEASE
#define ATOMICRELEASET(p,type) { type* punkT=p; p=NULL; punkT->Release(); }
#define ATOMICRELEASE(p) ATOMICRELEASET(p, IUnknown)
#endif
#ifndef SAFEFREEBSTR
#define SAFEFREEBSTR(p) if ((p) != NULL) { SysFreeString(p); (p) = NULL; } else
#endif
#ifndef SAFEFREEOLESTR
#define SAFEFREEOLESTR(p) if ((p) != NULL) { CoTaskMemFree(p); (p) = NULL; } else
#endif
#ifndef SAFELOCALFREE
#define SAFELOCALFREE(p) if ((p) != NULL) { LocalFree(p); (p) = NULL; } else
#endif
#ifndef SAFEDELETE
#define SAFEDELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; } else
#endif

#define URLDL_WNDCLASS  TEXT("TridentThicketUrlDlClass")

#define ACCEPT_LANG_MAX     256

// Options for BeginDownloadURL2
typedef enum {
    BDU2_BROWSER    // always download into the browser
} BDUMethod;

typedef DWORD BDUOptions;

// BDUOptions
#define BDU2_NONE               0
#define BDU2_NEEDSTREAM         1   // keep an istream around from bdu2_urlmon download

// OnDownloadComplete error codes
#define BDU2_ERROR_NONE         0
#define BDU2_ERROR_GENERAL      1
#define BDU2_ERROR_ABORT        2
#define BDU2_ERROR_MAXSIZE      3
#define BDU2_ERROR_TIMEOUT      4


// CUrlDowload hosts one browser and can handle one download at a time.
//
// Use of class CUrlDownload:
//
// 1) Create and AddRef it
// 1.5) call put_Flags() to set the bind status callback IBrowseControl::Flags
// 2) Call BeginDownloadURL2 to start a download
// 3) Call BeginDownloadURL2 to start another download, reusing browser
// 4) Call DoneDownloading() when finished
// 5) Release()

// DoneDownloading() must be called before Release() or the CUrlDownload instance may
//  continue to receive notifications from the browser and attempt to pass them to
//  the parent. It unhooks itself as soon as OnProgress(-1) is received. But be safe.


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
    

public:
    CUrlDownload( CThicketProgress *ptp, HRESULT *phr, UINT cpDL );
    ~CUrlDownload();

    void SetFormSubmitted(BOOL fFormSubmitted) { m_fFormSubmitted = fFormSubmitted; }
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

    void    DoneDownloading();  // Call before releasing. Will destroy browser and windows.
    void    DestroyBrowser();   // Destroy hosted browser, leave all else alone

    // URL manipulation functions
static HRESULT StripAnchor(LPWSTR lpURL);
static BOOL IsHTMLURL(LPCWSTR lpURL); // TRUE (yes) or FALSE (maybe)
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
    HWND                m_hwndMe;
    CThicketProgress*   m_ptp;
    UINT                m_cpDL;
    HRESULT             *m_phr;
    DWORD               m_dwProgMax;

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


    // General download data
    BDUMethod           m_iMethod;
    BDUOptions          m_iOptions;
    LPWSTR              m_pwszURL;      // gives us the current url after redirections
    BOOL                m_fSetResync;   // need RESYNCHRONIZE?
    DWORD               m_dwMaxSize;    // in bytes

    // IBrowseControl
    long                m_lBindFlags;

    // allow caching GetScript calls
    IHTMLWindow2        *m_pScript;

    // Client pull
    LPWSTR              m_pwszClientPullURL;
    int                 m_iNumClientPull;

    // other internal stuff
    HRESULT     CreateMyWindow();
    HRESULT     GetBrowser();   // Get browser and set us on connection point
    void        UnAdviseMe();   // Unhook our advise sink

    void        CleanUpBrowser();
    void        CleanUp();      // Clean up, including releasing browser

    HRESULT     BeginDownloadWithBrowser(LPCWSTR);

    HRESULT     HandleRefresh(LPWSTR pwszEquivString, LPWSTR pwszContent, BOOL fDone);

    HRESULT     OnDownloadComplete(int iError);     // cancel timeout, send OnDownloadComplete

    HRESULT     ProgressBytes(DWORD dwBytes);       // Will abort if too many
};



#endif // _DWNLOAD_HXX_
