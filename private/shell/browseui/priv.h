#ifndef _PRIV_H_
#define _PRIV_H_

// For use with VC6
#pragma warning(4:4519)  //default template arguments are only allowed on a class template; ignored
#pragma warning(4:4242)  //'initializing' : conversion from 'unsigned int' to 'unsigned short', possible loss of data

// Extra error checking (catches false errors, but useful to run every so often)
#if 0
#pragma warning(3:4701)   // local may be used w/o init
#pragma warning(3:4702)   // Unreachable code
#pragma warning(3:4705)   // Statement has no effect
#pragma warning(3:4709)   // command operator w/o index expression
#endif

// Sundown: forcing value to bool
#pragma warning(disable:4800)

// This stuff must run on Win95
#define _WIN32_WINDOWS      0x0400

#ifndef WINVER
#define WINVER              0x0400
#endif

#define _OLEAUT32_       // get DECLSPEC_IMPORT stuff right, we are defing these
#define _BROWSEUI_       // define bruiapi as functions exported instead of imported
#define _WINMM_          // for DECLSPEC_IMPORT in mmsystem.h
#define _WINX32_         // get DECLSPEC_IMPORT stuff right for WININET API
#define _URLCACHEAPI     // get DECLSPEC_IMPORT stuff right for WININET CACHE API
#ifndef STRICT
#define STRICT
#endif

// Map KERNEL32 unicode string functions to SHLWAPI
// This is needed way up here.
#define lstrcmpW    StrCmpW
#define lstrcmpiW   StrCmpIW
#define lstrcpyW    StrCpyW
#define lstrcpynW   StrCpyNW
#define lstrcatW    StrCatW


//
// Enable channel code for IE4 upgrades.
//
#define ENABLE_CHANNELS

#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

/////////////////////////////////////////////////////////////////////////
//
//  ATL / OLE HACKHACK
//
//  Include <w95wraps.h> before anything else that messes with names.
//  Although everybody gets the wrong name, at least it's *consistently*
//  the wrong name, so everything links.
//
//  NOTE:  This means that while debugging you will see functions like
//  CWindowImplBase__DefWindowProcWrapW when you expected to see
//  CWindowImplBase__DefWindowProc.
//
#define POST_IE5_BETA // turn on post-split iedev stuff
#include <w95wraps.h>

#include <windows.h>

// VariantInit is a trivial function -- avoid using OleAut32, use intrinsic
// version of memset for a good size win
// (it's here so that atl (in stdafx.h) gets it too)
#define VariantInit(p) memset(p, 0, sizeof(*(p)))

#define _FIX_ENABLEMODELESS_CONFLICT  // for shlobj.h
//WinInet need to be included BEFORE ShlObjp.h
#define HLINK_NO_GUIDS
#include <hlink.h>
#include <wininet.h>
#include <winineti.h>
#include <urlmon.h>
#undef GetClassInfo
#include <shlobj.h>
#include <shlobjp.h>
#include <exdispid.h>
#undef GetClassInfo
#include <objidl.h>

#define WANT_SHLWAPI_POSTSPLIT
#include <shlwapi.h>

#include <shconv.h>

#ifdef ATL_ENABLED
#if defined(__cplusplus) && !defined(DONT_USE_ATL)
// (stdafx.h must come before windowsx.h)
//#include "stdafx.h"             // ATL header file for this component
#endif
#endif

#include <windowsx.h>

#include <shellapi.h>

#include <shsemip.h>
#include <crtfree.h>

#include <ole2ver.h>
#include <olectl.h>
#include <hliface.h>
#include <docobj.h>
#define DLL_IS_ROOTABLE
#include <ccstock.h>
#include <ccstock2.h>
#include <port32.h>

#include <shellp.h>
#include <shguidp.h>
#include <isguids.h>
#include <shdguid.h>
#include <mimeinfo.h>
#include <hlguids.h>
#include <mshtmdid.h>
#include <dispex.h>     // IDispatchEx
#include <perhist.h>


#include <help.h>
#include <krnlcmn.h>    // GetProcessDword

#include <multimon.h>

#define DISALLOW_Assert             // Force to use ASSERT instead of Assert
#define DISALLOW_DebugMsg           // Force to use TraceMsg instead of DebugMsg
#include <debug.h>

#include <urlhist.h>

#include <regstr.h>     // for REGSTR_PATH_EXPLORE

#define USE_SYSTEM_URL_MONIKER
#include <urlmon.h>
#include <winineti.h>    // Cache APIs & structures
#include <inetreg.h>

#define _INTSHCUT_    // get DECLSPEC_IMPORT stuff right for INTSHCUT.h
#include <intshcut.h>

#include <propset.h>        // BUGBUG (scotth): remove this once OLE adds an official header

#include <regapix.h>        // MAXIMUM_SUB_KEY_LENGTH, MAXIMUM_VALUE_NAME_LENGTH, MAXIMUM_DATA_LENGTH

#include <browseui.h>
#include <shdocvw.h>


//
// WARNING: Don't add private header files in shdocvw here randomly, which
//  will force us to re-compiling everything. Keep those private headers
//  out of priv.h
//  
// #include <iface.h>
#include "globals.h"
#include "runonnt.h"
#include "dllload.h"
#include "util.h"
#include "brutil.h"
#include "../lib/qistub.h"
#ifdef DEBUG
#include "../lib/dbutil.h"
#endif

// Include the automation definitions...
#include <exdisp.h>
#include <exdispid.h>
#include <ocmm.h>
#include <mshtmhst.h>
#include <simpdata.h>
#include <htiface.h>
#include <objsafe.h>

#include "shui.h"
#define URLID_URLBASE           0
#define URLID_LOCATION          1
/////// URLID_FTPFOLDER         2 // Taken by a pre-release FTP Folder dll
#define URLID_PROTOCOL          3

// If stdshnor.bmp  or stdshhot.bmp are modifed, change this number to reflect the 
// new number of glyphs
#define NUMBER_SHELLGLYPHS      47
#define MAX_SHELLGLYPHINDEX     SHELLTOOLBAR_OFFSET + NUMBER_SHELLGLYPHS - 1

// Increment steps. For changing the with of the TB buttons. For localization
#define WIDTH_FACTOR            4

//
// Neutral ANSI/UNICODE types and macros... 'cus Chicago seems to lack them
//

#define DLL_IS_UNICODE         (sizeof(TCHAR) == sizeof(WCHAR))

#ifdef __cplusplus

//
//  Pseudoclass for the stupid variable-sized OLECMDTEXT structure.
//  You need to declare it as a class (and not a BYTE buffer that is
//  suitable cast) because BYTE buffers are not guaranteed to be aligned.
//
template <int n>
class OLECMDTEXTV : public OLECMDTEXT {
    WCHAR wszBuf[n-1];          // "-1" because OLECMDTEXT includes 1 wchar
};

extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */

// WIN31 APP compatability hacks for the desktop...
VOID WINAPI CheckWinIniForAssocs();
void CInternetToolbar_CleanUp();
void CUserAssist_CleanUp(DWORD dwReason, LPVOID lpvReserved);
void CBrandBand_CleanUp();
STDAPI_(HCURSOR) LoadHandCursor(DWORD dwRes);


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

extern const ITEMIDLIST c_idlDesktop;
typedef const BYTE *LPCBYTE;

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

#define EnterModeless() AddRef()       // Used for selfref'ing
#define ExitModeless() Release()

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
#ifdef DEBUG
#undef  FindWindow
#undef  FindWindowEx
#define FindWindow              FindWindowD
#define FindWindowEx            FindWindowExD

STDAPI_(HWND) FindWindowD  (LPCTSTR lpClassName, LPCTSTR lpWindowName);
STDAPI_(HWND) FindWindowExD(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpClassName, LPCTSTR lpWindowName);
#ifdef UNICODE
#define RealFindWindowEx        FindWindowExWrapW
#else
#define RealFindWindowEx        FindWindowExA
#endif // UNICODE
#endif // DEBUG


//
// Trace/dump/break flags specific to shell32\.
//   (Standard flags defined in shellp.h)
//

// Break flags
#define BF_ONDUMPMENU       0x10000000      // Stop after dumping menus
#define BF_ONLOADED         0x00000010      // Stop when loaded

// Trace flags
#define TF_UEM              0x00000010      // UEM stuff
#define TF_AUTOCOMPLETE     0x00000100      // AutoCompletion

// The following aren't really valid until the ccshell.ini file is updated.
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
#define TF_REGCHECK         0x00000100      // Registry check stuff
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
#define PF_NOBROWSEUI       0x00001000          // don't use browseui
#define PF_FORCEANSI        0x00002000      // Assume that Shell32 is ANSI

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

STDAPI_(void*) OleAlloc(UINT cb);
STDAPI_(void)  OleFree(LPVOID pb);
STDAPI_(IBindCtx *) BCW_Create(IBindCtx* pibc);

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);


#define MAX_URL_STRING      INTERNET_MAX_URL_LENGTH
#define MAX_NAME_STRING     INTERNET_MAX_PATH_LENGTH
#define MAX_BROWSER_WINDOW_TITLE   128

#define REG_SUBKEY_FAVORITESA            "\\MenuOrder\\Favorites"
#define REG_SUBKEY_FAVORITES             TEXT(REG_SUBKEY_FAVORITESA)

//
//  Class names
//
extern const TCHAR c_szOTClass[];

#define c_szExploreClass TEXT("ExploreWClass")
#define c_szIExploreClass TEXT("IEFrame")
#ifdef IE3CLASSNAME
#define c_szCabinetClass TEXT("IEFrame")
#else
#define c_szCabinetClass TEXT("CabinetWClass")
#endif
#define c_szAutoSuggestClass TEXT("Auto-Suggest Dropdown")

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

//
// a couple bogus pidls
//
#define PIDL_LOCALHISTORY ((LPCITEMIDLIST)-1)
#define PIDL_NOTHING      ((LPCITEMIDLIST)-2)


#define CALLWNDPROC WNDPROC

#ifdef POSTPOSTSPLIT

HRESULT IEGetDisplayName(LPCITEMIDLIST pidl, LPTSTR pszName, UINT uFlags);
#ifndef UNICODE
HRESULT IEGetDisplayNameW(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags);
#else
#define IEGetDisplayNameW IEGetDisplayName
#endif
HRESULT IEGetAttributesOf(LPCITEMIDLIST pidl, DWORD* pdwAttribs);
HRESULT IEBindToObject(LPCITEMIDLIST pidl, LPSHELLFOLDER* ppsf);
#endif

// Smartly delay load OLEAUT32
HRESULT VariantClearLazy(VARIANTARG *pvarg);
#define VariantClear VariantClearLazy
WINOLEAUTAPI VariantCopyLazy(VARIANTARG * pvargDest, VARIANTARG * pvargSrc);
#define VariantCopy VariantCopyLazy

#ifdef UNICODE
#define StrToOleStrN(wsz, cchWsz, pstr, cchPstr) StrCpyNW(wsz, pstr, cchWsz)
#else
#pragma message("need to thunk StrToOleStrN")
#endif

#define ILIsEqual(p1, p2)   IEILIsEqual(p1, p2, FALSE)


#ifdef __cplusplus
//
// C++ modules only
//
#include <shstr.h>

extern "C" const ITEMIDLIST s_idlNULL;

// helper routines for view state stuff

IStream *GetDesktopRegStream(DWORD grfMode, LPCTSTR pszName, LPCTSTR pszStreams);
IStream *GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCTSTR pszName, LPCTSTR pszStreamMRU, LPCTSTR pszStreams);

// StreamHeader Signatures
#define STREAMHEADER_SIG_CADDRESSBAND        0xF432E001
#define STREAMHEADER_SIG_CADDRESSEDITBOX     0x24F92A92

#define CoCreateInstance IECreateInstance
HRESULT IECreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                    DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);

#endif

extern HRESULT LoadHistoryShellFolder(IUnknown *punkSFHistory, IHistSFPrivate **pphsfHistory); // from urlhist.cpp
extern void CUrlHistory_CleanUp();

#ifdef UNIX
UINT GetCurColorRes(void);
EXTERN_C HRESULT CoCreateInternetExplorer( REFIID iid, DWORD dwClsContext, void **ppvunk );
// Although, UNIX is not exactly right here, because i386
// doesn't require any alignment, but that holds true for
// all the UNIXs we plan IE for.
#define ALIGN4(cb)         (((unsigned)(cb) % 4)? (unsigned)(cb)+(4-((unsigned)(cb)%4)) : (unsigned)(cb))
#define ALIGN4_IF_UNIX(cb) ALIGN4(cb)
#else
#define ALIGN4_IF_UNIX(cb)
#endif


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


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

#undef ExpandEnvironmentStrings
#define ExpandEnvironmentStrings #error "Use SHExpandEnvironmentStrings instead"

#endif // _PRIV_H_
