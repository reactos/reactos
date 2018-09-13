#ifndef _PRIV_H_
#define _PRIV_H_

// For use with VC6
#pragma warning(4:4242)  //'initializing' : conversion from 'unsigned int' to 'unsigned short', possible loss of data

// Sundown
#pragma warning(disable: 4800)  // conversion to bool

// Extra error checking (catches false errors, but useful to run every so often)
#if 0
#pragma warning(3:4701)   // local may be used w/o init
#pragma warning(3:4702)   // Unreachable code
#pragma warning(3:4705)   // Statement has no effect
#pragma warning(3:4709)   // command operator w/o index expression
#endif


#define ASSERT_PRIV_H_INCLUDED


// This stuff must run on Win95
#define _WIN32_WINDOWS      0x0400

#ifndef WINVER
#define WINVER              0x0400
#endif

#define _OLEAUT32_      // get DECLSPEC_IMPORT stuff right, we are defing these
#define _FSMENU_        // for DECLSPEC_IMPORT
#define _WINMM_         // for DECLSPEC_IMPORT in mmsystem.h
#define _SHDOCVW_       // for DECLSPEC_IMPORT in shlobj.h
#define _WINX32_        // get DECLSPEC_IMPORT stuff right for WININET API
#define _BROWSEUI_      // Make functions exported from browseui as stdapi (as they are delay loaded)

#define _URLCACHEAPI_   // get DECLSPEC_IMPORT stuff right for wininet urlcache
#ifndef STRICT
#define STRICT
#endif

//
// Channels are enabled for IE4 upgrades.
//
#define ENABLE_CHANNELS

#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */
#include <nt.h>

// WARNING!  NTDLL is manually delay-loaded! bcause it is crippled on Win95.
// We used to use automatic delay-loading, but people who didn't realize that
// Win95 doesn't have full support for NTDLL would accidentally call NTDLL
// functions and cause us to crash on Win95.
#undef NTSYSAPI
#define NTSYSAPI
#include <ntrtl.h>
#include <nturtl.h>
#undef NTSYSAPI
#define NTSYSAPI DECLSPEC_IMPORT

#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */
#define POST_IE5_BETA // turn on post-split iedev stuff
#include <w95wraps.h>
#include <windows.h>
#include <windowsx.h>

// VariantInit is a trivial function -- avoid using OleAut32, use intrinsic
// version of memset for a good size win
// (it's here so that atl (in stdafx.h) gets it too)
#define VariantInit(p) memset(p, 0, sizeof(*(p)))

// Smartly delay load OLEAUT32
HRESULT VariantClearLazy(VARIANTARG *pvarg);
#define VariantClear VariantClearLazy
WINOLEAUTAPI VariantCopyLazy(VARIANTARG * pvargDest, VARIANTARG * pvargSrc);
#define VariantCopy VariantCopyLazy

// Must do this before including <exdisp.h> or the build will break.
// See comments at declaration of FindWindowD much further below.
#ifdef DEBUG
#undef  FindWindow
#undef  FindWindowEx
#define FindWindow              FindWindowD
#define FindWindowEx            FindWindowExD
#endif

#define _FIX_ENABLEMODELESS_CONFLICT  // for shlobj.h
//WinInet need to be included BEFORE ShlObjp.h
#include <hlink.h>
#include <wininet.h>
#include <urlmon.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <exdisp.h>
#include <objidl.h>

#define WANT_SHLWAPI_POSTSPLIT
#include <shlwapi.h>
#include <shlwapip.h>

#undef SubclassWindow
#if defined(__cplusplus) && !defined(DONT_USE_ATL)
// (stdafx.h must come before windowsx.h)
#include "stdafx.h"             // ATL header file for this component
#endif

#include <shellapi.h>

#include <shsemip.h>
#include <crtfree.h>

#include <ole2ver.h>
#include <olectl.h>
#include <shellp.h>
#include <shdocvw.h>
#include <shguidp.h>
#include <isguids.h>
#include <shdguid.h>
#include <mimeinfo.h>
#include <hlguids.h>
#include <mshtmdid.h>
#include <dispex.h>     // IDispatchEx
#include <perhist.h>
#include <iethread.h>

#include <shldispp.h>

#include <help.h>
#include <krnlcmn.h>    // GetProcessDword

#include <multimon.h>

#define DISALLOW_Assert             // Force to use ASSERT instead of Assert
#define DISALLOW_DebugMsg           // Force to use TraceMsg instead of DebugMsg
#include <debug.h>

#include <urlhist.h>
#include <regapix.h>    // MAXIMUM_SUB_KEY_LENGTH, MAXIMUM_VALUE_NAME_LENGTH, MAXIMUM_DATA_LENGTH

#include <regstr.h>     // for REGSTR_PATH_EXPLORE

#define USE_SYSTEM_URL_MONIKER
#include <urlmon.h>
#include <winineti.h>    // Cache APIs & structures
#include <inetreg.h>

#define _INTSHCUT_    // get DECLSPEC_IMPORT stuff right for INTSHCUT.h
#include <intshcut.h>

#include <propset.h>        // BUGBUG (scotth): remove this once OLE adds an official header

#define HLINK_NO_GUIDS
#include <hlink.h>
#include <hliface.h>
#include <docobj.h>
#define DLL_IS_ROOTABLE
#include <ccstock.h>
#include <ccstock2.h>
#include <port32.h>

#include <browseui.h>

#ifdef OLD_HLIFACE
#define HLNF_OPENINNEWWINDOW HLBF_OPENINNEWWINDOW
#endif

#define ISVISIBLE(hwnd)  ((GetWindowStyle(hwnd) & WS_VISIBLE) == WS_VISIBLE)

// shorthand
#ifndef ATOMICRELEASE
#ifdef __cplusplus
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->Release();} }
#else
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->lpVtbl->Release(punkT);} }
#endif

// doing this as a function instead of inline seems to be a size win.
//
#ifdef NOATOMICRELESEFUNC
#define ATOMICRELEASE(p) ATOMICRELEASET(p, IUnknown)
#else
#define ATOMICRELEASE(p) IUnknown_AtomicRelease((LPVOID*)&p)
#endif
#endif //ATOMICRELEASE

#ifdef SAFERELEASE
#undef SAFERELEASE
#endif
#define SAFERELEASE(p) ATOMICRELEASE(p)


// Include the automation definitions...
#include <exdisp.h>
#include <exdispid.h>
#include <ocmm.h>
#include <htmlfilter.h>
#include <mshtmhst.h>
#include <simpdata.h>
#include <htiface.h>
#include <objsafe.h>

#include "util.h"
#include "brutil.h"
#include "../lib/qistub.h"
#ifdef DEBUG
#include "../lib/dbutil.h"
#endif

#define DLL_IS_UNICODE         (sizeof(TCHAR) == sizeof(WCHAR))

//
// Neutral ANSI/UNICODE types and macros... 'cus Chicago seems to lack them
// IEUNIX - we do have them in MainWin
//
#ifndef MAINWIN
#ifdef  UNICODE

   typedef WCHAR TUCHAR, *PTUCHAR;

#else   /* UNICODE */

   typedef unsigned char TUCHAR, *PTUCHAR;

#endif /* UNICODE */
#endif /* !MAINWIN */

#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */

// Count of images Currently - 2 (Normal and Hot)
#define CIMLISTS                2


typedef struct tagIMLCACHE
{
    HIMAGELIST arhiml[CIMLISTS];
    HIMAGELIST arhimlPendingDelete[CIMLISTS];
    COLORREF cr3D;
    BOOL fSmallIcons;
} IMLCACHE;

typedef struct tagBMPCACHE
{
    HBITMAP hbmp;
    COLORREF cr3D;
} BMPCACHE;

void IMLCACHE_CleanUp(IMLCACHE * pimlCache, DWORD dwFlags);
#define IML_DELETEPENDING   0x01
#define IML_DESTROY         0x02

#include "dllload.h"


extern const ITEMIDLIST c_idlDesktop;
typedef const BYTE *LPCBYTE;

STDAPI MonikerFromURLPidl(LPCITEMIDLIST pidlURLItem, IMoniker** ppmk);
STDAPI MonikerFromURL(LPCWSTR wszPath, IMoniker** ppmk);
STDAPI MonikerFromString(LPCTSTR szPath, IMoniker** ppmk);

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

extern IShellFolder* g_psfInternet;
HRESULT InitPSFInternet(void);

//
// Debug helper functions
//

#ifdef DEBUG

LPCTSTR Dbg_GetCFName(UINT ucf);
LPCTSTR Dbg_GetHRESULTName(HRESULT hr);
LPCTSTR Dbg_GetREFIIDName(REFIID riid);
LPCTSTR Dbg_GetVTName(VARTYPE vt);

BOOL    IsStringContained(LPCTSTR pcszBigger, LPCTSTR pcszSuffix);

#endif // DEBUG

//
// we may not be part of the namespace on IE3/Win95
//
#define ILIsEqual(p1, p2)       IEILIsEqual(p1, p2, FALSE)


extern LPCITEMIDLIST c_pidlURLRoot;

//
// Trace/dump/break flags specific to shell32\.
//   (Standard flags defined in shellp.h)
//

// Break flags
#define BF_ONDUMPMENU       0x10000000      // Stop after dumping menus
#define BF_ONLOADED         0x00000010      // Stop when loaded

// Trace flags
#define TF_INTSHCUT         0x00000010      // Internet shortcuts
#define TF_REGCHECK         0x00000100      // Registry check stuff
#define TF_SHDLIFE          0x00000200
#define TF_SHDREF           0x00000400
#define TF_SHDPERF          0x00000800
#define TF_SHDAUTO          0x00001000
#define TF_MENUBAND         0x00002000      // Menu band messages
#define TF_SITEMAP          0x00004000      // Sitemap messages
#define TF_SHDTHREAD        0x00008000      // Thread management
#define TF_SHDCONTROL       0x00010000      // ActiveX Control
#define TF_SHDAPPHACK       0x00020000      // Hack for app-bug
#define TF_SHDBINDING       0x00040000      // Moniker binding
#define TF_SHDPROGRESS      0x00080000      // Download progress
#define TF_SHDNAVIGATE      0x00100000      // Navigation
#define TF_SHDUIACTIVATE    0x00200000      // UI-Activation/Deactivation
#define TF_OCCONTROL        0x00400000      // OC Hosting Window Control
#define TF_PIDLWRAP         0x00800000      // Pidl / Protocol wrapping
#define TF_AUTOCOMPLETE     0x01000000      // AutoCompletion
#define TF_COCREATE         0x02000000      // WinList/CoCreate(Browser only)
#define TF_URLNAMESPACE     0x04000000      // URL Name Space
#define TF_BAND             0x08000000      // Bands (ISF Band, etc)
#define TF_TRAVELLOG        0x10000000      // TravelLog and Navigation stack
#define TF_DDE              0x20000000      // PMDDE traces
#define TF_CUSTOM1          0x40000000      // Custom messages #1
#define TF_CUSTOM2          0x80000000      // Custom messages #2

//BUGBUGREMOVE
#define TF_OBJECTCACHE      TF_TRAVELLOG

// (Re-use TF_CUSTOM1 and TF_CUSTOM2 by defining a TF_ value in your
// local file to one of these values while you have the file checked
// out.)

// Dump flags
#define DF_SITEMAP          0x00000001      // Sitemap
#define DF_MEMLEAK          0x00000002      // Dump leaked memory at the end
#define DF_DEBUGQI          0x00000004      // Alloc stub object for each QI
#define DF_DEBUGQINOREF     0x00000008      // No AddRef/Release QI stub
#define DF_DEBUGMENU        0x00000010      // Dump menu handles
#define DF_URL              0x00000020      // Display URLs
#define DF_AUTOCOMPLETE     0x00000040      // AutoCompletion
#define DF_DELAYLOADDLL     0x00000080      // Delay-loaded DLL
#define DF_SHELLLIST        0x00000100      // CShellList contents
#define DF_INTSHCUT         0x00000200      // Internet shortcut structs
#define DF_URLPROP          0x00000400      // URL properties
#define DF_MSGHOOK          0x00000800      // Menu MessageFilter
#define DF_GETMSGHOOK       0x00001000      // GetMessageFilter
#define DF_TRANSACCELIO     0x00002000      // GetMessageFilter

// Prototype flags
#define PF_USERMENUS        0x00000001      // Use traditional USER menu bar
#define PF_NEWFAVMENU       0x00000002      // New favorites menu
#define PF_FORCESHDOC401    0x00000004      // force shdoc401 even on NT5
//efine PF_                 0x00000008      // Used by dochost.cpp
//efine PF_                 0x00000010      // Unused
//efine PF_                 0x00000020      // Used by urlhist.cpp
//efine PF_                 0x00000040      // Unused
//efine PF_                 0x00000100      // Unused
//efine PF_                 0x00000200      // Used by shembed.cpp
//efine PF_                 0x00000400      // Unused?
//efine PF_                 0x00000800      // Unused?

//
// global object array - used for class factory, auto registration, type libraries, oc information
//

#include "cfdefs.h"

#define OIF_ALLOWAGGREGATION  0x0001

//
// global variables
//
//
// Function prototypes
//
STDAPI CMyHlinkSrc_CreateInstance(REFCLSID rclsid, DWORD grfContext, REFIID riid, LPVOID* ppvOut);
STDAPI CMyHlinkSrc_OleCreate(CLSID rclsid, REFIID riid, DWORD renderOpt,
                             FORMATETC* pFormatEtc, IOleClientSite* pclient,
                             IStorage* pstg, LPVOID* ppvOut);

STDAPI CMyHlinkSrc_OleLoad(IStorage* pstg, REFIID riid, IOleClientSite* pclient, LPVOID* ppvOut);

HRESULT SHRegisterTypeLib(void);
VOID SHCheckRegistry(void);

// htregmng.cpp
BOOL CenterWindow (HWND hwndChild, HWND hwndParent);

#define OleAlloc(cb)    CoTaskMemAlloc(cb)
#define OleFree(pv)     CoTaskMemFree(pv)

STDAPI_(IBindCtx *) BCW_Create(IBindCtx* pibc);

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);


#define MAX_URL_STRING      INTERNET_MAX_URL_LENGTH
#define MAX_NAME_STRING     INTERNET_MAX_PATH_LENGTH
#define MAX_BROWSER_WINDOW_TITLE   128

// Stack allocated BSTR (to avoid calling SysAllocString)
typedef struct _SA_BSTR {
    ULONG   cb;
    WCHAR   wsz[MAX_URL_STRING];
} SA_BSTR;

typedef struct _SA_BSTRGUID {
    UINT  cb;
    WCHAR wsz[39];
} SA_BSTRGUID;

STDAPI _SetStdLocation(LPTSTR szPath, UINT id);

STDAPI CDocObjectHost_AddPages(LPARAM that, HWND hwnd, HINSTANCE hinst, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
STDAPI_(void) CDocObjectHost_GetCurrentPage(LPARAM that, LPTSTR szBuf, UINT cchMax);

extern BOOL DeleteUrlCacheEntryA(LPCSTR pszUrlName);

//
// a couple bogus pidls
//
#define PIDL_LOCALHISTORY ((LPCITEMIDLIST)-1)
#define PIDL_NOTHING      ((LPCITEMIDLIST)-2)


//
// Globals (per-process)
//
extern UINT g_tidParking;           // parking thread
extern HPALETTE g_hpalHalftone;
extern BOOL g_fBrowserOnlyProcess;  // initialized in IEWinMain()



//
//  In DEBUG, send all our class registrations through a wrapper that
//  checks that the class is on our unregister-at-unload list.
//
#ifdef DEBUG
#undef  SHRegisterClass
#undef    RegisterClass
#define SHRegisterClass       SHRegisterClassD
#define   RegisterClass         RegisterClassD

STDAPI_(BOOL) SHRegisterClassD(CONST WNDCLASS* pwc);
STDAPI_(ATOM)   RegisterClassD(CONST WNDCLASS* pwc);
#ifdef UNICODE
#define RealSHRegisterClass   SHRegisterClassW
#define   RealRegisterClass     RegisterClassWrapW
#else
#define RealSHRegisterClass   SHRegisterClassA
#define   RealRegisterClass     RegisterClassA
#endif // UNICODE
#endif // DEBUG

//
//  In DEBUG, send FindWindow through a wrapper that ensures that the
//  critical section is not taken.  FindWindow sends interthreadmessages,
//  which is not obvious.
//
//  IShellWindows has a method called FindWindow, so we have to define
//  the debug wrapper macros before including <exdisp.h>.  We should've
//  called it FindWindowSW.  In fact, there should be some law against
//  giving a method the same name as a Windows API.
//
#ifdef DEBUG
STDAPI_(HWND) FindWindowD  (LPCTSTR lpClassName, LPCTSTR lpWindowName);
STDAPI_(HWND) FindWindowExD(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpClassName, LPCTSTR lpWindowName);
#ifdef UNICODE
#define RealFindWindowEx        FindWindowExWrapW
#else
#define RealFindWindowEx        FindWindowExA
#endif // UNICODE
#endif // DEBUG

#define CALLWNDPROC WNDPROC

extern const GUID CGID_ShellBrowser;
extern const GUID CGID_PrivCITCommands;

// Map KERNEL32 unicode string functions to SHLWAPI
//#define lstrcmpW    StrCmpW
//#define lstrcmpiW   StrCmpIW
//#define lstrcpyW    StrCpyW
//#define lstrcpynW   StrCpyNW
//#define lstrcatW    StrCatW

//
// Prevent buffer overruns - don't use unsafe functions.
//

//lstrcpy
#define lstrcpyW       Do_not_use_lstrcpyW_use_StrCpyNW
#define lstrcpyA       Do_not_use_lstrcpyA_use_StrCpyNA

#ifdef lstrcpy
    #undef lstrcpy
#endif
#define lstrcpy        Do_not_use_lstrcpy_use_StrCpyN

//StrCpy
//#ifdef StrCpyW
//    #undef StrCpyW
//#endif
#define StrCpyW        Do_not_use_StrCpyW_use_StrCpyNW

#ifdef StrCpyA
    #undef StrCpyA
#endif
#define StrCpyA        Do_not_use_StrCpyA_use_StrCpyNA

#ifdef StrCpy
    #undef StrCpy
#endif
#define StrCpy         Do_not_use_StrCpy_use_StrCpyN


//ualstrcpyW
#ifdef ualstrcpyW
    #undef ualstrcpyW
#endif
#define ualstrcpyW     Do_not_use_ualstrcpyW_ualstrcpynW

//lstrcatW
#define lstrcatW       Do_not_use_lstrcatW_use_StrCatBuffW
#define lstrcatA       Do_not_use_lstrcatA_use_StrCatBuffA

#ifdef lstrcat
    #undef lstrcat
#endif
#define lstrcat        Do_not_use_lstrcat_use_StrCatBuff

//wsprintf
#define wsprintfW      Do_not_use_wsprintfW_use_wnsprintfW
#define wsprintfA      Do_not_use_wsprintfA_use_wnsprintfA

#ifdef wsprintf
    #undef wsprintf
#endif
#define wsprintf       Do_not_use_wsprintf_use_wnsprintf

//wvsprintf
#ifdef wvsprintfW
    #undef wvsprintfW
#endif
#define wvsprintfW     Do_not_use_wvsprintfW_use_wvnsprintfW

#define wvsprintfA     Do_not_use_wvsprintfA_use_wvnsprintfA

#ifdef wvsprintf
    #undef wvsprintf
#endif
#define wvsprintf      Do_not_use_wvsprintf_use_wvnsprintf


//
// Don't use the kernel string functions.  Use shlwapi equivalents.
//

// lstrcmp
#define lstrcmpW       Do_not_use_lstrcmpW_use_StrCmpW
//#define lstrcmpA       Do_not_use_lstrcmpA_use_StrCmpA
#ifdef lstrcmp
    #undef lstrcmp
#endif
#define lstrcmp        Do_not_use_lstrcmp_use_StrCmp

// lstrcmpi
#define lstrcmpiW      Do_not_use_lstrcmpiW_use_StrCmpIW
//#define lstrcmpiA      Do_not_use_lstrcmpiA_use_StrCmpIA
#ifdef lstrcmpi
    #undef lstrcmpi
#endif
#define lstrcmpi       Do_not_use_lstrcmpi_use_StrCmpI

// lstrncmpi
#define lstrncmpiW     Do_not_use_lstrncmpiW_use_StrCmpNIW
//#define lstrncmpiA      Do_not_use_lstrncmpiA_use_StrCmpNIA
#ifdef lstrncmpi
    #undef lstrncmpi
#endif
#define lstrncmpi      Do_not_use_lstrncmpi_use_StrCmpNI


//lstrcpyn
#define lstrcpynW      Do_not_use_lstrcpynW_use_StrCpyNW
//#define lstrcpynA      Do_not_use_lstrcpynA_use_StrCpyNA
#ifdef lstrcpyn
    #undef lstrcpyn
#endif
#define lstrcpyn       Do_not_use_lstrcpyn_use_StrCpyN


extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst

extern BOOL g_fRunningOnNT;
extern BOOL g_bNT5Upgrade;
extern BOOL g_bRunOnNT5;
extern BOOL g_bRunOnMemphis;
extern BOOL g_fRunOnFE;
extern BOOL g_bInDllInstall;
extern UINT g_uiACP;
//
// Is Mirroring APIs enabled (BiDi Memphis and NT5 only)
//
extern BOOL g_bMirroredOS;

#ifdef WINDOWS_ME
//
// This is needed for BiDi localized win95 RTL stuff
//
extern BOOL g_bBiDiW95Loc;

#else // !WINDOWS_ME
#define g_bBiDiW95Loc FALSE
#endif // WINDOWS_ME

extern const TCHAR c_szHelpFile[];
extern const TCHAR c_szHtmlHelpFile[];
extern const TCHAR c_szURLPrefixesKey[];
extern const TCHAR c_szDefaultURLPrefixKey[];
extern const TCHAR c_szShellEmbedding[];
extern const TCHAR c_szViewClass[];

#define c_szNULL        TEXT("")



// Status bar defines
#define STATUS_PANES            5
#define STATUS_PANE_NAVIGATION  0
#define STATUS_PANE_PROGRESS    1
#define STATUS_PANE_OFFLINE     2
//#define STATUS_PANE_PRINTER     3
#define STATUS_PANE_SSL         3
#define STATUS_PANE_ZONE        4

#define ZONES_PANE_WIDTH        70

extern HICON g_hiconSSL;
extern HICON g_hiconFortezza;
extern HICON g_hiconOffline;
extern HICON g_hiconPrinter;

#define MAX_TOOLTIP_STRING 80

#define SID_SOmWindow IID_IHTMLWindow2
#define SID_SDropBlocker CLSID_SearchBand

#define MIN_BROWSER_DISPID              1
#define MAX_BROWSER_DISPID              1000

// We may want to put "Thunks between us and some of the shell private entries as
// some of them will take Ansi strings on Windows 95 and will take unicode strings
// on NT.
#include "runonnt.h"


// Function in IEDISP.CPP
HRESULT CreateBlankURL(BSTR *url, LPCTSTR pszErrorUrl, BSTR oldUrl);
SAFEARRAY * MakeSafeArrayFromData(LPCBYTE pData, DWORD cbData);

#include "idispids.h"
#include "dbgmem.h"

#ifdef __cplusplus
//
// C++ modules only
//
#include <shstr.h>
#include "shembed.h"


extern "C" const ITEMIDLIST s_idlNULL;

// helper routines for view state stuff

IStream *GetDesktopRegStream(DWORD grfMode, LPCTSTR pszName, LPCTSTR pszStreams);
//IStream *GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCTSTR pszName, LPCTSTR pszStreamMRU, LPCTSTR pszStreams);

// StreamHeader Signatures
#define STREAMHEADER_SIG_CADDRESSBAND        0xF432E001
#define STREAMHEADER_SIG_CADDRESSEDITBOX     0x24F92A92

#define CoCreateInstance IECreateInstance
HRESULT IECreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                    DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);

#endif

extern HRESULT LoadHistoryShellFolder(IUnknown *punkSFHistory, IHistSFPrivate **pphsfHistory); // from urlhist.cpp
extern void CUrlHistory_CleanUp();

#define c_szHelpFile     TEXT("iexplore.hlp")



/////// mappings...
////// these functions moved from being private utilities to being exported (mostly from shlwapi)
///// and thus need a new name to avoid name collisions
#define IsRegisteredClient SHIsRegisteredClient
#define IE_ErrorMsgBox SHIEErrorMsgBox
#define SetDefaultDialogFont SHSetDefaultDialogFont
#define RemoveDefaultDialogFont SHRemoveDefaultDialogFont
#define IsGlobalOffline SHIsGlobalOffline
#define SetWindowBits SHSetWindowBits
#define IsSameObject SHIsSameObject
#define SetParentHwnd SHSetParentHwnd
#define IsEmptyStream SHIsEmptyStream
#define PropagateMessage SHPropagateMessage
#define MenuIndexFromID  SHMenuIndexFromID
#define Menu_RemoveAllSubMenus SHRemoveAllSubMenus
#define _EnableMenuItem SHEnableMenuItem
#define _CheckMenuItem SHCheckMenuItem
#define SimulateDrop SHSimulateDrop
#define GetMenuFromID  SHGetMenuFromID
#define GetCurColorRes SHGetCurColorRes
#define VerbExists  SHVerbExists
#define IsExpandableFolder SHIsExpandableFolder
#define WaitForSendMessageThread SHWaitForSendMessageThread
#define FillRectClr  SHFillRectClr
#define SearchMapInt SHSearchMapInt
#define IsChildOrSelf SHIsChildOrSelf
#define StripMneumonic SHStripMneumonic
#define MapNbspToSp SHMapNbspToSp
#define GetViewStream SHGetViewStream
#define HinstShdocvw() HINST_THISDLL

STDAPI CoCreateInternetExplorer( REFIID iid, DWORD dwClsContext, void **ppvunk );

#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

// Although, UNIX is not exactly right here, because i386
// doesn't require any alignment, but that holds true for
// all the UNIXs we plan IE for.
#ifdef UNIX
#define ALIGN4(cb)         (((unsigned)(cb) % 4)? (unsigned)(cb)+(4-((unsigned)(cb)%4)) : (unsigned)(cb))
#define ALIGN4_IF_UNIX(cb) ALIGN4(cb)
#else
#define ALIGN4_IF_UNIX(cb)
#define QUAD_PART(a) ((a)##.QuadPart)
#endif

// Sundown macros
#define PtrDiff(x,y)        ((LPBYTE)(x)-(LPBYTE)(y))

// Dummy union macros for code compilation on platforms not
// supporting nameless stuct/union

#ifdef NONAMELESSUNION
#define DUMMYUNION_MEMBER(member)   DUMMYUNIONNAME.##member
#define DUMMYUNION2_MEMBER(member)  DUMMYUNIONNAME2.##member
#define DUMMYUNION3_MEMBER(member)  DUMMYUNIONNAME3.##member
#define DUMMYUNION4_MEMBER(member)  DUMMYUNIONNAME4.##member
#define DUMMYUNION5_MEMBER(member)  DUMMYUNIONNAME5.##member
#else
#define DUMMYUNION_MEMBER(member)    member
#define DUMMYUNION2_MEMBER(member)   member
#define DUMMYUNION3_MEMBER(member)   member
#define DUMMYUNION4_MEMBER(member)   member
#define DUMMYUNION5_MEMBER(member)   member
#endif

#define REG_SUBKEY_FAVORITESA            "\\MenuOrder\\Favorites"
#define REG_SUBKEY_FAVORITES             TEXT(REG_SUBKEY_FAVORITESA)

#undef ExpandEnvironmentStrings
#define ExpandEnvironmentStrings #error "Use SHExpandEnvironmentStrings instead"

#endif // _PRIV_H_
