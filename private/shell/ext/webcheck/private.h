#define INC_OLE2
#define _SHDOCVW_
#ifdef UNICODE
#define POST_IE5_BETA
#include <w95wraps.h>
#endif
#include <windows.h>
#include <windowsx.h>
#include <ccstock.h>
#include <ole2.h>
#include <ole2ver.h>
#include <oleauto.h>
#include <docobj.h>
#include <shlwapi.h>
#include <wininet.h>   // INTERNET_MAX_URL_LENGTH.  Must be before shlobjp.h!
#include <winineti.h>
#include <shlobj.h>
#include <shlobjp.h>
//#include <crtfree.h>
#include <inetsdk.h>
#include <intshcut.h>
#include <mshtml.h>
#include <notftn.h>
#include <webcheck.h>
#include <exdisp.h>
#include <inetreg.h>
#include <advpub.h>
#include <htiframe.h>
#include <shsemip.h>        // in ccshell\inc
#include <shellp.h>
#include <lmcons.h>         // for UNLEN/PWLEN
#include <ipexport.h>       // for ping
#include <icmpapi.h>        // for ping
#include <mobsync.h>
#include <mobsyncp.h>
#undef MAX_STATUS   // goofy
#include "debug.h"
#include "resource.h"
#include "rsrchdr.h"

#include "pstore.h"
#include <shdocvw.h> // to get SHRestricted2*
#include <dwnnot.h> // IDownloadNotify

#pragma warning(3:4701)   // local may be used w/o init
#pragma warning(3:4702)   // Unreachable code
#pragma warning(3:4705)   // Statement has no effect
#pragma warning(3:4706)   // assignment w/i conditional expression
#pragma warning(3:4709)   // command operator w/o index expression

// How user of CUrlDownload receives notifications from it.
class CUrlDownloadSink
{
public:
    virtual HRESULT OnDownloadComplete(UINT iID, int iError) = 0;
    virtual HRESULT OnAuthenticate(HWND *phwnd, LPWSTR *ppszUsername, LPWSTR *ppszPassword)
                        { return E_NOTIMPL; }
    virtual HRESULT OnClientPull(UINT iID, LPCWSTR pwszOldURL, LPCWSTR pwszNewURL)
                        { return S_OK; }
    virtual HRESULT OnOleCommandTargetExec(const GUID *pguidCmdGroup, DWORD nCmdID,
                                DWORD nCmdexecopt, VARIANTARG *pvarargIn, 
                                VARIANTARG *pvarargOut)
                        { return OLECMDERR_E_NOTSUPPORTED; }

    // returns free threaded callback interface
    // If you use this, make your implementation of IDownloadNotify fast
    virtual HRESULT GetDownloadNotify(IDownloadNotify **ppOut)
                        { return E_NOTIMPL; }
};

class CUrlDownload;

#include "filetime.h"
#include "offline.h"
#include "utils.h"
#include "delagent.h"
#include "cdfagent.h"
#include "webcrawl.h"
#include "trkcache.h"
#include "postagnt.h"
#include "cdlagent.h"

// Note: dialmon.h changes winver to 0x400
#include "dialmon.h"

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
#define TF_SEPROX       0x00008000
#define TF_CDFAGENT     0x00010000
#define TF_STRINGLIST   0x00020000
#define TF_URLDOWNLOAD  0x00040000
#define TF_DOWNLD       0x00080000
#define TF_DIALMON      0x00100000
#define TF_MAILAGENT    0x00200000
//#define TF_TRAYAGENT    0x00400000
#define TF_SUBSFOLDER   0x00800000
#define TF_MEMORY       0x01000000
#define TF_UPDATEAGENT  0x02000000
#define TF_POSTAGENT    0x04000000
#define TF_DELAGENT     0x08000000
#define TF_TRACKCACHE   0x10000000
#define TF_SYNCMGR      0x20000000
#define TF_THROTTLER    0x40000000
#define TF_ADMIN        0x80000000  //  Admin and IE upgrade

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
#define SAFELOCALFREE(p) if ((p) != NULL) { MemFree(p); (p) = NULL; } else
#endif
#ifndef SAFEDELETE
#define SAFEDELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; } else
#endif

// MAX_WEBCRAWL_LEVELS is the max crawl depth for site subscriptions
// MAX_CDF_CRAWL_LEVELS is the max crawl depth for the "LEVEL" attrib value for CDFs
#define MAX_WEBCRAWL_LEVELS 3
#define MAX_CDF_CRAWL_LEVELS 3

#define MY_MAX_CACHE_ENTRY_INFO 6144

//
// Define the location of the webcheck registry key.
//
#define WEBCHECK_REGKEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Webcheck")
#define WEBCHECK_REGKEY_NOTF TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Webcheck\\Notification Handlers")
#define WEBCHECK_REGKEY_STORE (WEBCHECK_REGKEY TEXT("\\Store.1"))



//
// Registry Keys
//
extern const TCHAR c_szRegKey[];                // registry key for webcheck stuff
extern const TCHAR c_szRegKeyUsernames[];       // registry key for webcheck stuff
extern const TCHAR c_szRegKeyPasswords[];       // registry key for webcheck stuff
extern const TCHAR c_szRegKeyStore[];
extern const TCHAR c_szRegPathInternetSettings[];
// extern const TCHAR c_szRegKeyRestrictions[];    // HKCU\Policies\...\Infodelivery\Restrictions
// extern const TCHAR c_szRegKeyModifications[];   // HKCU\Policies\...\Infodelivery\Modification
// extern const TCHAR c_szRegKeyCompletedMods[];   // HKCU\Policies\...\Infodelivery\CompletedModifications

//
// Registry Values
//
extern const TCHAR c_szNoChannelLogging[];

//
// Globals
//
extern HINSTANCE    g_hInst;                // dll instance
extern ULONG        g_cLock;                // outstanding locks
extern ULONG        g_cObj;                 // outstanding objects
extern BOOL         g_fIsWinNT;             // are we on winNT? Always initialized.
extern BOOL         g_fIsWinNT5;            // are we on winNT5? Always initialized.
extern const TCHAR c_szEnable[];            // enable unattended dialup

extern const TCHAR  c_szStrEmpty[];

inline ULONG DllLock()     { return ++g_cLock; }
inline ULONG DllUnlock()   { return --g_cLock; }
inline ULONG DllGetLock()  { return g_cLock; }

inline ULONG DllAddRef()   { return ++g_cObj; }
inline ULONG DllRelease()  { return --g_cObj; }
inline ULONG DllGetRef()   { return g_cObj; }

//
// Subscription property names; webcheck.cpp
//
// Agent Start
extern const WCHAR  c_szPropURL[];          // BSTR
extern const WCHAR  c_szPropName[];         // BSTR; friendly name
extern const WCHAR  c_szPropAgentFlags[];   // I4
extern const WCHAR  c_szPropCrawlLevels[];  // I4; webcrawler
extern const WCHAR  c_szPropCrawlFlags[];   // I4; webcrawler
extern const WCHAR  c_szPropCrawlMaxSize[]; // I4; webcrawler (in KB)
extern const WCHAR  c_szPropCrawlChangesOnly[];  // BOOL
extern const WCHAR  c_szPropChangeCode[];   // I4 or CY
extern const WCHAR  c_szPropEmailNotf[];    // BOOL;
extern const WCHAR  c_szPropCrawlUsername[];  // BSTR
extern const WCHAR  c_szPropCrawlLocalDest[]; // BSTR
extern const WCHAR  c_szPropEnableShortcutGleam[]; // I4
extern const WCHAR  c_szPropChannelFlags[];     // I4; channel agent specific flags
extern const WCHAR  c_szPropChannel[];          // I4; indicates a channel
extern const WCHAR  c_szPropDesktopComponent[]; // I4; indicates a desktop component
// set by agents in Agent Start
extern const WCHAR  c_szPropCrawlGroupID[];  // cache group ID
extern const WCHAR  c_szPropCrawlNewGroupID[]; // ID for new (existing) cache group
extern const WCHAR  c_szPropCrawlActualSize[];  // in KB
extern const WCHAR  c_szPropActualProgressMax[]; // Progress Max at end of last update
extern const WCHAR  c_szStartCookie[];      // The cookie of Start Notification.
extern const WCHAR  c_szPropStatusCode[];       // SCODE
extern const WCHAR  c_szPropStatusString[];     // BSTR (sentence)
extern const WCHAR  c_szPropCompletionTime[];   // DATE
extern const WCHAR  c_szPropPassword[];   // BSTR
// End Report
extern const WCHAR  c_szPropEmailURL[];         // BSTR
extern const WCHAR  c_szPropEmailFlags[];       // I4
extern const WCHAR  c_szPropEmailTitle[];       // BSTR
extern const WCHAR  c_szPropEmailAbstract[];    // BSTR
extern const WCHAR  c_szPropCharSet[];          // BSTR

// Tray Agent Properties
extern const WCHAR  c_szPropGuidsArr[];     // SAFEARRAY for a list of GUIDs

// Initial cookie used in AGENT_INIT notification.
extern const WCHAR  c_szInitCookie[];      // The cookie of Start Notification.
// Tracking 
extern const WCHAR  c_szTrackingCookie[];   // Channel identity 
extern const WCHAR  c_szTrackingPostURL[];  // tracking post url
extern const WCHAR  c_szPostingRetry[];     // 
extern const WCHAR  c_szPostHeader[];       // specify encoding method of postdata
extern const WCHAR  c_szPostPurgeTime[];    // DATE

//
// Mail agent flags for the c_szPropEmailFlags property
//
enum MAILAGENT_FLAGS {
    MAILAGENT_FLAG_CUSTOM_MSG = 0x1
};

//
// Mail functions
//
HRESULT SendEmailFromItem(ISubscriptionItem *pItem);
HRESULT MimeOleEncodeStreamQP(IStream *pstmIn, IStream *pstmOut);
void ReadDefaultSMTPServer(LPTSTR pszBuf, UINT cch);
void ReadDefaultEmail(LPTSTR pszBuf, UINT cch);

// utils.cpp
interface IChannelMgrPriv;
HRESULT GetChannelPath(LPCTSTR pszURL, LPTSTR pszPath, int cch, IChannelMgrPriv** ppIChannelMgrPriv);

//
// Timer id's for scheduler and dialmon
//
#define TIMER_ID_DIALMON_IDLE   2       // every minute while connected
#define TIMER_ID_DIALMON_SEC    3       // every second for 30 seconds 
#define TIMER_ID_DBL_CLICK      4       // did the user single or double click
#define TIMER_ID_USER_IDLE      5       // used to detect user idle on system

// Custom schedule dialog proc
BOOL CALLBACK CustomDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int SGMessageBox(HWND, UINT, UINT);

//
// Dialmon messages - send to scheduler window by various pieces of the system
// to tell us when dialup related stuff happens
//
#define WM_DIALMON_FIRST        WM_USER+100
#define WM_WINSOCK_ACTIVITY     WM_DIALMON_FIRST + 0
#define WM_REFRESH_SETTINGS     WM_DIALMON_FIRST + 1
#define WM_SET_CONNECTOID_NAME  WM_DIALMON_FIRST + 2
#define WM_IEXPLORER_EXITING    WM_DIALMON_FIRST + 3

// message used to report user mouse or kbd activity
// Note: sage.vxd uses this value and we can't change it.
#define WM_USER_ACTIVITY        WM_USER+5

// message sent by loadwc requesting a dynaload of sens/lce.
#define WM_LOAD_SENSLCE         WM_USER+200
#define WM_IS_SENSLCE_LOADED    WM_USER+201

//
// Random subscription defaults
//
#define DEFAULTLEVEL    0
#define DEFAULTFLAGS  (WEBCRAWL_GET_IMAGES | WEBCRAWL_LINKS_ELSEWHERE | WEBCRAWL_GET_CONTROLS)

#define IsNativeAgent(CLSIDAGENT)       (((CLSIDAGENT) == CLSID_WebCrawlerAgent) || ((CLSIDAGENT) == CLSID_ChannelAgent))
#define IS_VALID_SUBSCRIPTIONTYPE(st)   ((st == SUBSTYPE_URL) || (st == SUBSTYPE_CHANNEL) || (st == SUBSTYPE_DESKTOPCHANNEL) || (st == SUBSTYPE_DESKTOPURL))

//
// Useful functions
//
int MyOleStrToStrN(LPTSTR psz, int cchMultiByte, LPCOLESTR pwsz);
int MyStrToOleStrN(LPOLESTR pwsz, int cchWideChar, LPCTSTR psz);
void DumpIID(LPCSTR psz, REFIID riid);

// String comparison routines ; assume 8-bit characters. Return 0 or nonzero.
// Will work correctly if one or both strings are entirely 8-bit characters.
int MyAsciiCmpW(LPCWSTR pwsz1, LPCWSTR pwsz2);
int MyAsciiCmpNIW(LPCWSTR pwsz1, LPCWSTR pwsz2, int iLen);
inline
 int MyAsciiCmpNW(LPCWSTR pwsz1, LPCWSTR pwsz2, int iLen)
 { return memcmp(pwsz1, pwsz2, iLen*sizeof(WCHAR)); }

// Implementation in CDFagent.cpp
HRESULT XMLScheduleElementToTaskTrigger(IXMLElement *pRootEle, TASK_TRIGGER *ptt);
HRESULT ScheduleToTaskTrigger(TASK_TRIGGER *ptt, SYSTEMTIME *pstStartDate, SYSTEMTIME *pstEndDate,
                              long lInterval, long lEarliest, long lLatest, int iZone=9999);

// Admin related functions
HRESULT ProcessInfodeliveryPolicies(void);

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

//  IE 5 versions - sans notfmgr support
HRESULT ReadDWORD       (ISubscriptionItem *pItem, LPCWSTR szName, DWORD *pdwRet);
HRESULT ReadLONGLONG    (ISubscriptionItem *pItem, LPCWSTR szName, LONGLONG *pllRet);
HRESULT ReadGUID        (ISubscriptionItem *pItem, LPCWSTR szName, GUID *);
HRESULT ReadDATE        (ISubscriptionItem *pItem, LPCWSTR szName, DATE *dtVal);
HRESULT ReadBool        (ISubscriptionItem *pItem, LPCWSTR szName, VARIANT_BOOL *pBoolRet);
HRESULT ReadBSTR        (ISubscriptionItem *pItem, LPCWSTR szName, BSTR *bstrRet);
HRESULT ReadOLESTR      (ISubscriptionItem *pItem, LPCWSTR szName, LPWSTR *pszRet);
HRESULT ReadAnsiSTR     (ISubscriptionItem *pItem, LPCWSTR szName, LPSTR *ppszRet);
HRESULT ReadSCODE       (ISubscriptionItem *pItem, LPCWSTR szName, SCODE *pscRet);
HRESULT ReadVariant     (ISubscriptionItem *pItem, LPCWSTR szName, VARIANT *pvarRet);

HRESULT WriteDWORD      (ISubscriptionItem *pItem, LPCWSTR szName, DWORD dwVal);
HRESULT WriteLONGLONG   (ISubscriptionItem *pItem, LPCWSTR szName, LONGLONG llVal);
HRESULT WriteGUID       (ISubscriptionItem *pItem, LPCWSTR szName, GUID *);
HRESULT WriteDATE       (ISubscriptionItem *pItem, LPCWSTR szName, DATE *dtVal);
HRESULT WriteOLESTR     (ISubscriptionItem *pItem, LPCWSTR szName, LPCWSTR szVal);
HRESULT WriteResSTR     (ISubscriptionItem *pItem, LPCWSTR szName, UINT uID);
HRESULT WriteAnsiSTR    (ISubscriptionItem *pItem, LPCWSTR szName, LPCSTR szVal);
HRESULT WriteSCODE      (ISubscriptionItem *pItem, LPCWSTR szName, SCODE scVal);
HRESULT WriteEMPTY      (ISubscriptionItem *pItem, LPCWSTR szName);
HRESULT WriteVariant    (ISubscriptionItem *pItem, LPCWSTR szName, VARIANT *pvarVal);

#ifdef UNICODE
#define ReadTSTR        ReadOLESTR
#define WriteTSTR       WriteOLESTR
#else
#define ReadTSTR        ReadAnsiSTR
#define WriteTSTR       WriteAnsiSTR
#endif


HRESULT WritePassword   (ISubscriptionItem *pItem, BSTR szPassword); 
HRESULT ReadPassword    (ISubscriptionItem *pItem, BSTR *ppszPassword); 

// WEBCRAWL.CPP helper functions

// Do cool stuff to a single URL in the cache. Make sticky, get size, put in group...
// Returns E_OUTOFMEMORY if make sticky fails
HRESULT GetUrlInfoAndMakeSticky(
            LPCTSTR pszBaseUrl,         // Base URL. May be NULL if pszThisUrl is absolute
            LPCTSTR pszThisUrl,         // Absolute or relative url
            LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo,       // Required
            DWORD   dwBufSize,          // Size of *lpCacheEntryInfo
            GROUPID llCacheGroupID);    // Group ID ; may be 0

// PreCheckUrlForChange and PostCheckUrlForChange are documented in webcrawl.cpp
HRESULT PreCheckUrlForChange(LPCTSTR lpURL, VARIANT *pvarChange, BOOL *pfGetContent);
HRESULT PostCheckUrlForChange(VARIANT *pvarChange,
                              LPINTERNET_CACHE_ENTRY_INFO lpInfo, FILETIME ftNewLastModified);

HRESULT WriteCookieToInetDB(LPCTSTR pszURL, SUBSCRIPTIONCOOKIE *pCookie, BOOL bRemove);
HRESULT ReadCookieFromInetDB(LPCTSTR pszURL, SUBSCRIPTIONCOOKIE *pCookie);

#define MAX_RES_STRING_LEN 128      // max resource string len for WriteStringRes

// IntSite helper function
HRESULT IntSiteHelper(LPCTSTR pszURL, const PROPSPEC *pReadPropspec,
        PROPVARIANT *pReadPropvar, UINT uPropVarArraySize, BOOL fWrite);
extern const PROPSPEC c_rgPropRead[];
#define PROP_SUBSCRIPTION   0
#define PROP_FLAGS          1
#define PROP_TRACKING       2
#define PROP_CODEPAGE       3

//=============================================================================
// Helper class for aggregation. Inherit from this like another interface, then
//  implement InnerQI and include IMPLEMENT_DELEGATE_UNKNOWN in your class declaration
class CInnerUnknown
{
public:
    CInnerUnknown() { m_cRef = 1; m_punkOuter=(IUnknown *)(CInnerUnknown *)this; }

    void InitAggregation(IUnknown *punkOuter, IUnknown **punkInner)
    {
        if (punkOuter)
        {
            m_punkOuter = punkOuter;
            *punkInner = (IUnknown *)(CInnerUnknown *)this;
        }
    }

    virtual HRESULT STDMETHODCALLTYPE InnerQI(REFIID riid, void **ppunk) = 0;
    virtual ULONG STDMETHODCALLTYPE InnerAddRef() { return ++m_cRef; }
    virtual ULONG STDMETHODCALLTYPE InnerRelease() = 0;

protected:
    long     m_cRef;
    IUnknown *m_punkOuter;
};

#define IMPLEMENT_DELEGATE_UNKNOWN() \
STDMETHODIMP         QueryInterface(REFIID riid, void **ppunk) \
    { return m_punkOuter->QueryInterface(riid, ppunk); } \
STDMETHODIMP_(ULONG) AddRef() { return m_punkOuter->AddRef(); } \
STDMETHODIMP_(ULONG) Release() { return m_punkOuter->Release(); } \
STDMETHODIMP_(ULONG) InnerRelease() { \
        if (0L != --m_cRef) return m_cRef; \
        delete this; \
        return 0L; }
// end aggregation helpers
//=============================================================================



// Registry helper functions
BOOL ReadRegValue(HKEY hkeyRoot, const TCHAR *pszKey, const TCHAR *pszValue, 
                   void *pData, DWORD dwBytes);
BOOL WriteRegValue(HKEY hkeyRoot, const TCHAR *pszKey, const TCHAR *pszValue,
                    void *pData, DWORD dwBytes, DWORD dwType);

DWORD ReadRegDWORD(HKEY hkeyRoot, const TCHAR *pszKey, const TCHAR *pszValue);

// Event logging function
DWORD __cdecl LogEvent(LPTSTR pszEvent, ...);

// Thread proc for firing up sens and lce
DWORD WINAPI ExternalsThread(LPVOID lpData);

// Used to set syncmgr "warning" level
#define INET_E_AGENT_WARNING 0x800C0FFE
//
// Main WebCheck class
//
class CWebCheck : public IOleCommandTarget
{
protected:
    ULONG           m_cRef;

public:
    CWebCheck(void);
    ~CWebCheck(void);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IWebCheck members
    STDMETHODIMP         StartService(BOOL fForceExternals);
    STDMETHODIMP         StopService(void);

    // IOleCommandTarget members
    STDMETHODIMP         QueryStatus(const GUID *pguidCmdGroup,
                                     ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
    STDMETHODIMP         Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
                              DWORD nCmdexecopt, VARIANTARG *pvaIn,
                              VARIANTARG *pvaOut);

    // External handling members
    BOOL                 ShouldLoadExternals(void);
    BOOL                 AreExternalsLoaded(void);
    void                 LoadExternals(void);
    void                 UnloadExternals(void);

    // thread for handling external bits
    HANDLE               _hThread;

    // events to synchronize with external thread
    HANDLE               _hInitEvent;
    HANDLE               _hTerminateEvent;
};

//
// CMemStream class
//
class CMemStream
{
protected:
    IStream *   m_pstm;
    BOOL        m_fDirty;
    BOOL        m_fError;

public:
    BOOL        m_fNewStream;

public:
    CMemStream(BOOL fNewStream=TRUE);
    ~CMemStream();

    BOOL        IsError() { return m_fError; }

    HRESULT     Read(void *pv, ULONG cb, ULONG *cbRead);
    HRESULT     Write(void *pv, ULONG cb, ULONG *cbWritten);
    HRESULT     Seek(long lMove, DWORD dwOrigin, DWORD *dwNewPos);
    HRESULT     SaveToStream(IUnknown *punk);
    HRESULT     LoadFromStream(IUnknown **ppunk);

    HRESULT     CopyToStream(IStream *pStm);
};



extern BOOL IsGlobalOffline(void);
extern void SetGlobalOffline(BOOL fOffline);

extern BOOL IsADScreenSaverActive();
extern HRESULT MakeADScreenSaverActive();


#define  _NO_DBGMEMORY_REDEFINITION_
#include "dbgmem.h"

#ifdef DEBUG
extern LEAKDETECTFUNCS LeakDetFunctionTable;
extern BOOL g_fInitTable;
HLOCAL MemAlloc(IN UINT fuFlags, IN UINT cbBytes);
HLOCAL MemFree(HLOCAL hMem);
HLOCAL MemReAlloc(HLOCAL hMem, UINT uBytes, UINT uFlags);
#else
#define MemAlloc LocalAlloc
#define MemFree LocalFree
#define MemReAlloc LocalReAlloc
#endif // DEBUG

typedef HRESULT (* CREATEPROC)(IUnknown *, IUnknown **);

// Helper functions to read and write passwords to an encrypted store.

STDAPI ReadNotificationPassword(LPCWSTR wszUrl, BSTR * pbstrPassword);
STDAPI WriteNotificationPassword(LPCWSTR wszUrl, BSTR  bstrPassword);

// dialmon functions
BOOL DialmonInit(void);
void DialmonShutdown(void);

// LCE dynaload entry points
typedef HRESULT (* LCEREGISTER)(HMODULE);
typedef HRESULT (* LCEUNREGISTER)(HMODULE);
typedef HRESULT (* LCESTART)(void);
typedef HRESULT (* LCESTOP)(void);

// SENS dynaload entry points
typedef HRESULT (* SENSREGISTER)(void);
typedef BOOL (* SENSSTART)(void);
typedef BOOL (* SENSSTOP)(void);

// event dispatching
DWORD DispatchEvent(DWORD dwEvent, LPWSTR pwsEventDesc, DWORD dwEventData);
