#ifndef _SHDOC401_PRIV_H_

#define _SHDOC401_PRIV_H_

#ifndef SHDOC401_DLL
#define SHDOC401_DLL
#endif

#define _OLEAUT32_                  // We delay-load oleaut32
#define _BROWSEUI_                  // We delay-load browseui
#define _WINMM_                     // We delay-load winmm
#define _FSMENU_                    // We implement our own FileMenu

// This stuff must run on NT4
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

//--------------------------------------------------------------------------
//
//  The order of these is critical for ATL.
//
//  1.  ATL has methods called SubclassWindow, which conflicts with a
//      macro in <windowsx.h>, so we must include <windowsx.h> after ATL.
//
//  2.  We want ATL to use the shell debug macros, so we must include
//      <debug.h> before ATL so it can see the shell debug macros.
//
//  3.  VariantInit is such a trivial function that we inline it in order
//      to avoid pulling in OleAut32.

#include <windows.h>
#define _ATL_NO_DEBUG_CRT   // Use the shell debug macros
#include <debug.h>          // Get the shell debug macros

#define VariantInit(p) memset(p, 0, sizeof(*(p)))

#ifdef __cplusplus

#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <atlctl.h>
#include <atlconv.h>
#include <atliface.h>
#include <atlwin.h>

#endif  /* __cplusplus */

// end of ATL Stuff
//--------------------------------------------------------------------------

#include <windowsx.h>

// Include file dependencies:
//
// wininet.h must come before shlobj.h
// shlobj.h must come before shlobjp.h
//
// shlobj.h, hlink.h and ole.h must come before shellp.h for LPCBASEBROWSER
// to be defined properly.
//
// shsemip.h must come before shellp.h
//
// shlwapi.h must come before shlwapip.h
//

#include <port32.h>
#include <hlink.h>

#include <wininet.h>
#include <shlobj.h>
#include <shlobjp.h>

#include <shsemip.h>
#include <shellp.h>

#include <shlwapi.h>
#include <shlwapip.h>

#include <ccstock.h>
#include <debug.h>
#include "brutil.h"

#include <shlguid.h>
#include <shlguidp.h>
#include <shdguid.h>
#include <shguidp.h>

#include <shdocvw.h>
#include <desktopp.h>
#include <iethread.h>
#include <browseui.h>
#include <krnlcmn.h> // GetProcessDword
#include <regstr.h>

#include <help.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <debug.h>
#include "unicpp\utils.h"
#include <iethread.h>

STDAPI_(void) DllAddRef();
STDAPI_(void) DllRelease();

extern HINSTANCE g_hinst;
#define HINST_THISDLL g_hinst

extern BOOL g_fRunningOnNT;
extern BOOL g_bRunOnNT5;
extern BOOL g_bRunOnMemphis;
extern DWORD g_dwShell32;
extern UINT g_msgMSWheel;

//
//  g_dwShell is in the form
//
//      MAKELONG(dwMinor, dwMajor)
//
//  g_uiShell32 returns the shell major version.
//
#define g_uiShell32     HIWORD(g_dwShell32)
#define c_dwIE400       0x00040047  // 4.71
#define c_dwIE401       0x00040048  // 4.72
#define c_dwIE500       0x00050000  // 5.00

#define SAFERELEASE(p)   IUnknown_AtomicRelease((LPVOID *)&(p))

typedef struct _SA_BSTR {
    ULONG   cb;
    WCHAR   wsz[INTERNET_MAX_URL_LENGTH];
} SA_BSTR;

//
//  The cb field of a BSTR is the count of bytes, not including the
//  terminating L('\0').
//
//
//  DECLARE_CONST_BSTR - Goes into header file (if any)
//  DEFINE_CONST_BSTR  - Creates the variable, must already be declared
//  MAKE_CONST_BSTR    - Combines DECLARE and DEFINE
//
#define DECLARE_CONST_BSTR(name, str) \
 extern const struct BSTR##name { ULONG cb; WCHAR wsz[sizeof(str)/sizeof(WCHAR)]; } name

#define DEFINE_CONST_BSTR(name, str) \
        const struct BSTR##name name = { sizeof(str) - sizeof(WCHAR), str }

#define MAKE_CONST_BSTR(name, str) \
        const struct BSTR##name { ULONG cb; WCHAR wsz[sizeof(str)/sizeof(WCHAR)]; } \
                                name = { sizeof(str) - sizeof(WCHAR), str }

DECLARE_CONST_BSTR(s_sstrIDMember,         L"id");
DECLARE_CONST_BSTR(s_sstrSubSRCMember,     L"subscribed_url");

extern HKEY g_hkcuExplorer;
extern HKEY g_hklmExplorer;

#define TUCHAR  TBYTE

#define g_cyIcon        GetSystemMetrics(SM_CYICON)
#define g_cySmIcon      (GetSystemMetrics(SM_CYICON)/2)
#define g_cxIcon        GetSystemMetrics(SM_CXICON)
#define g_cxSmIcon      (GetSystemMetrics(SM_CXICON)/2)

//-------------------------------------------------------------------------
//
//  #define away some functions which did not exist in Win95 or which
//  exist differently between Win95 and WinNT so we can reimplement
//  them in shdup.cpp.
//
#undef  SHGetSpecialFolderPath
#define SHGetSpecialFolderPath      _SHGetSpecialFolderPath
STDAPI_(BOOL) SHGetSpecialFolderPath(HWND hwnd, LPTSTR pszPath, int nFolder, BOOL fCreate);

#undef  ILGetDisplayNameEx
#define ILGetDisplayNameEx          _ILGetDisplayNameEx
STDAPI_(BOOL) ILGetDisplayNameEx(IShellFolder *psfRoot, LPCITEMIDLIST pidl, LPTSTR pszName, int fType);

// Believe it or not, this function name already begins with an underscore!
#undef  _ILCreate
#define _ILCreate                   __ILCreate
STDAPI_(LPITEMIDLIST) _ILCreate(UINT cbSize);

#undef  ILCreateFromPathW
#define ILCreateFromPathW           _ILCreateFromPathW
STDAPI_(LPITEMIDLIST) ILCreateFromPathW(IN LPCWSTR pszPath);

#undef  StrRetToStrN
#define StrRetToStrN                _StrRetToStrN
BOOL WINAPI StrRetToStrN(LPSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl);

#undef  GetProcessDword
#define GetProcessDword             _GetProcessDword
STDAPI_(DWORD) GetProcessDword(DWORD idProcess, LONG iIndex);

//
//  Shell32 exports these functions as TCHAR, which means we have to do
//  charset detection at runtime to thunk the arguments properly.
//
#undef  Shell_GetCachedImageIndex
#define Shell_GetCachedImageIndex   _Shell_GetCachedImageIndex
STDAPI_(int) Shell_GetCachedImageIndex(LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags);

#undef  IsLFNDrive
#define IsLFNDrive                  _IsLFNDrive
STDAPI_(BOOL) IsLFNDrive(LPCTSTR pszPath);

#undef  ILCreateFromPath
#define ILCreateFromPath            _ILCreateFromPath
STDAPI_(LPITEMIDLIST) ILCreateFromPath(LPCTSTR pszPath);

#undef  SHSimpleIDListFromPath
#define SHSimpleIDListFromPath      _SHSimpleIDListFromPath
STDAPI_(LPITEMIDLIST) SHSimpleIDListFromPath(LPCTSTR pszPath);

#undef  PathResolve
#define PathResolve                 _PathResolve
STDAPI_(BOOL) PathResolve(LPTSTR lpszPath, LPCTSTR rgpszDirs[], UINT fFlags);

#undef  Win32DeleteFile
#define Win32DeleteFile             _Win32DeleteFile
STDAPI_(BOOL) Win32DeleteFile(LPCTSTR pszFile);

#undef  PathYetAnotherMakeUniqueName
#define PathYetAnotherMakeUniqueName _PathYetAnotherMakeUniqueName
STDAPI_(BOOL) PathYetAnotherMakeUniqueName(LPTSTR pszUniqueName,
               LPCTSTR pszPath, LPCTSTR pszShort, LPCTSTR pszFileSpec);

#undef  PathQualify
#define PathQualify                 _PathQualify
STDAPI_(void) PathQualify(LPTSTR pszDir);

#undef  SHRunControlPanel
#define SHRunControlPanel           _SHRunControlPanel
STDAPI_(BOOL) _SHRunControlPanel(LPCTSTR lpcszCmdLine, HWND hwndMsgParent);

//-------------------------------------------------------------------------
//
//  #define away some functions that we dynamically connect to because
//  they exist only in integrated mode.
//
//  List them here in the same order that they appear in dllload.c.
//

#undef  SHSetShellWindowEx
#define SHSetShellWindowEx          _SHSetShellWindowEx
STDAPI_(BOOL) SHSetShellWindowEx(HWND hwnd, HWND hwndChild);

#undef  RealDriveTypeFlags
#define RealDriveTypeFlags          _RealDriveTypeFlags
STDAPI_(int) RealDriveTypeFlags(int iDrive, BOOL fOKToHitNet);

#undef  SHChangeNotifyReceive
#define SHChangeNotifyReceive       _SHChangeNotifyReceive
STDAPI_(void) SHChangeNotifyReceive(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra);

#undef  SHChangeNotification_Lock
#define SHChangeNotification_Lock   _SHChangeNotification_Lock
STDAPI_(LPSHChangeNotificationLock)
SHChangeNotification_Lock(HANDLE hChangeNotification, DWORD dwProcessId, LPITEMIDLIST **pppidl, LONG *plEvent);

#undef  SHChangeNotification_Unlock
#define SHChangeNotification_Unlock _SHChangeNotification_Unlock
STDAPI_(BOOL) SHChangeNotification_Unlock(LPSHChangeNotificationLock pshcnl);

#undef  SHChangeRegistrationReceive
#define SHChangeRegistrationReceive _SHChangeRegistrationReceive
STDAPI_(BOOL) SHChangeRegistrationReceive(HANDLE hChangeNotification, DWORD dwProcId);

#undef  SHWaitOp_Operate
#define SHWaitOp_Operate            _SHWaitOp_Operate
STDAPI_(void) SHWaitOp_Operate( HANDLE hWaitOp, DWORD dwProcId);

#undef  WriteCabinetState
#define WriteCabinetState           _WriteCabinetState
STDAPI_(BOOL) WriteCabinetState( LPCABINETSTATE lpState );

#undef  ReadCabinetState
#define ReadCabinetState            _ReadCabinetState
STDAPI_(BOOL) ReadCabinetState( LPCABINETSTATE lpState, int iSize );

#undef  FileIconInit
#define FileIconInit                _FileIconInit
STDAPI_(BOOL) FileIconInit( BOOL fRestoreCache );

#undef  IsUserAnAdmin
#define IsUserAnAdmin               _IsUserAnAdmin
STDAPI_(BOOL) IsUserAnAdmin(void);

#undef  CheckWinIniForAssocs
#define CheckWinIniForAssocs        _CheckWinIniForAssocs
STDAPI_(void) CheckWinIniForAssocs(void);

//-------------------------------------------------------------------------
//
//  Miscellaneous weirdness
//
//  Win95 exported ShellMessageBoxA under the name ShellMessageBox.
//  Fortunately, NT kept the same ordinal.
//

// For now, shdocvw\srcw sets _SHDOC401_UNICODE_ because it knows
// it's being built UNICODE and has taken the steps to make sure
// it's doing the right thing.  (In other words, _SHDOC401_UNICODE_
// is the "Trust me, I know what I'm doing" flag.)
//
#ifndef _SHDOC401_UNICODE_
#ifdef UNICODE
#error This file assumes that we're being built ANSI.
#endif
#endif // _SHDOC401_UNICODE_

//
//  If linking with Win95 header files, then call it ShellMessageBox.
//
#ifdef _M_IX86

#undef ShellMessageBox
int FAR _cdecl ShellMessageBox(
    HINSTANCE hAppInst,
    HWND hWnd,
    LPCSTR lpcText,
    LPCSTR lpcTitle,
    UINT fuStyle, ...);
#endif

//-------------------------------------------------------------------------
//
// We can steal these from Shlwapi.
//
#undef  OpenRegStream
#define OpenRegStream               SHOpenRegStream

#undef  StrToOleStrN
#define StrToOleStrN(wsz, cwch, sz, cch) \
        SHTCharToUnicode(sz, wsz, cwch)

#undef  OleStrToStrN
#define OleStrToStrN(sz, cch, wsz, cwch) \
        SHUnicodeToTChar(wsz, sz, cch)

//-------------------------------------------------------------------------
//
// We implement this privately instead of using the shdocvw version.
//
#undef  IEBindToObject
#define IEBindToObject              _IEBindToObject

void IEPlaySound(LPCTSTR pszSound, BOOL fSysSound);

//-------------------------------------------------------------------------
//
//  NT5-specific functions which must be GetProcAddress()d.
//
typedef BOOL (WINAPI *t_AllowSetForegroundWindow)(DWORD dwProcessID);
extern t_AllowSetForegroundWindow g_pfnAllowSetForegroundWindow;

#define AllowSetForegroundWindow  g_pfnAllowSetForegroundWindow

//-------------------------------------------------------------------------
//
// Goofy strings
//
#define c_szNULL TEXT("")

//-------------------------------------------------------------------------
//
// Trace flags.  Values up to 0x00000008 are reserved by debug.h
//
#define TF_DEFVIEW          0x00000010      //
#define TF_LIFE             0x00000020      //

#define TF_CUSTOM1          0x40000000      // Custom messages #1
#define TF_CUSTOM2          0x80000000      // Custom messages #2

//-------------------------------------------------------------------------
//
// Dump flags
//

#define DF_DEBUGQI          0x00000001
#define DF_DEBUGQINOREF     0x00000002


//-------------------------------------------------------------------------
//
// Debugging strings
//

#define GEN_DEBUGSTRW(str)  ((str) ? (str) : L"<Null Str>")
#define GEN_DEBUGSTRA(str)  ((str) ? (str) : "<Null Str>")

#ifdef UNICODE
#define GEN_DEBUGSTR  GEN_DEBUGSTRW
#else // UNICODE
#define GEN_DEBUGSTR  GEN_DEBUGSTRA
#endif // UNICODE

//-------------------------------------------------------------------------
//
// Random helper functions.
//

STDAPI Invoke_OnConnectionPointerContainer(IUnknown * punk, REFIID riidCP, DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);
STDAPI Shell32GetTypeInfo(LCID lcid, UUID uuid, ITypeInfo **ppITypeInfo);
STDAPI_(DWORD) SHWNetGetConnection(LPCTSTR lpLocalName, LPTSTR lpRemoteName, LPDWORD lpnLength);

#define SetWindowBits       SHSetWindowBits

// GetUrlScheme -----------------------------------------------------------

DWORD GetUrlSchemeA(IN LPCSTR pcszUrl);
DWORD GetUrlSchemeW(IN LPCWSTR pcszUrl);

#ifdef UNICODE
#define GetUrlScheme        GetUrlSchemeW
#else // UNICODE
#define GetUrlScheme        GetUrlSchemeA
#endif // UNICODE

// TrimWhiteSpace ---------------------------------------------------------

#define TrimWhiteSpaceW(psz)        StrTrimW(psz, L" \t")
#define TrimWhiteSpaceA(psz)        StrTrimA(psz, " \t")

#ifdef UNICODE
#define TrimWhiteSpace      TrimWhiteSpaceW
#else
#define TrimWhiteSpace      TrimWhiteSpaceA
#endif

// SysAllocStringT et al --------------------------------------------------

#ifdef UNICODE
#define SysAllocStringT     SysAllocString
#define AllocBStrFromString(psz)    SysAllocString(psz)
#define TCharSysAllocString(psz)    SysAllocString(psz)
#else // UNICODE
#define SysAllocStringT     SysAllocStringA
extern BSTR AllocBStrFromString(LPTSTR);
#define TCharSysAllocString(psz)    AllocBStrFromString(psz)
#endif // UNICODE

//-------------------------------------------------------------------------
//
// Goofy custom delay-load
//

typedef BOOL (* PFNGETOPENFILENAME)(OPENFILENAME * pofn);
extern PFNGETOPENFILENAME g_pfnGetOpenFileName;
BOOL Comdlg32DLL_Init(void);

#undef GetOpenFileName
BOOL  APIENTRY GetOpenFileName(LPOPENFILENAME);

//-------------------------------------------------------------------------
//
// Trace flags and stuff
//

// Trace flags
#define TF_DDE              0x00000010      // PMDDE traces

// Dump flags
#define DF_DELAYLOADDLL     0x00000001      // delayload

// Function trace flags
#define FTF_DDE             0x00000004      // DDE functions

#ifdef DEBUG
extern BOOL  g_bInDllEntry;
#undef SendMessage
#define SendMessage  SendMessageD
LRESULT WINAPI SendMessageD(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif

#define ASSERTDLLENTRY      ASSERT(g_bInDllEntry);

int     MLDialogBoxWrap(HINSTANCE hInstance,
                        LPCTSTR lpTemplateName,
                        HWND hwndParent,
                        DLGPROC lpDialogFunc);
int     MLDialogBoxParamWrap(HINSTANCE hInstance,
                             LPCTSTR lpTemplateName,
                             HWND hwndParent,
                             DLGPROC lpDialogFunc,
                             LPARAM dwInitParam);
BOOL    MLEndDialogWrap(HWND hDlg,
                        int nResult);
HWND    MLHtmlHelpWrap(HWND hwndCaller,
                       LPCTSTR pszFile,
                       UINT uCommand,
                       DWORD dwData,
                       DWORD dwCrossCodePage);
BOOL    MLWinHelpWrap(HWND hwndCaller,
                      LPCTSTR lpszHelp,
                      UINT uCommand,
                      DWORD dwData);

#ifdef __cplusplus
}
#endif

#endif // _SHDOC401_PRIV_H_
