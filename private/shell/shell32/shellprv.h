#ifndef _SHELLPRV_H_
#define _SHELLPRV_H_

#define OVERRIDE_SHLWAPI_PATH_FUNCTIONS     // see comment in shsemip.h

#define _WIN32_DCOM     // for COINIT_DISABLE_OLE1DDE
#define _OLE32_         // for DECLSPEC_IMPORT delay load
#define _SHELL32_       // for DECLSPEC_IMPORT
#define _FSMENU_        // for DECLSPEC_IMPORT
#define _BROWSEUI_      // Make functions exported from browseui as stdapi (as they are delay loaded)
#define _USERENV_

//
// Enable channels for the IE4 upgrades.
//
#define ENABLE_CHANNELS

#ifdef __cplusplus
#define NO_INCLUDE_UNION
#endif  /* __cplusplus */

#define NOWINDOWSX
#define STRICT
#define OEMRESOURCE // FSMenu needs the menu triangle

#define INC_OLE2
#define CONST_VTABLE

#ifdef WINNT
//
// Disable a few warnings so we can include the system header files at /W4.
//
#include "warning.h"

//
//  These NT headers must come before <windows.h> or you get redefinition
//  errors!  It's a miracle the system builds at all...
//
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
#include <nt.h>         // Some of the NT specific code needs Rtl functions
#include <ntrtl.h>      // which requires all of these header files...
#include <nturtl.h>
#include <ntdddfs.h>
#include <ntseapi.h>
#ifdef __cplusplus
}       /* End of extern "C" */
#endif  /* __cplusplus */

#endif // WINNT

#define CC_INTERNAL   // this is because docfind uses the commctrl internal prop sheet structures

// This stuff must run on Win95
#ifndef WINVER
#define WINVER              0x0400
#endif

// And this stuff must run on NT4 (except for apithk.c)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

// For dllload: change function prototypes to not specify declspec import
#define _OLEAUT32_
#define _SHDOCVW_
#define _LINKINFO_

//--------------------------------------------------------------------------
//
//  The order of these is critical for ATL.
//
//  1.  ATL has its own definition of InlineIsEqualGUID that conflicts with
//      the definition in <objbase.h>, so we must explicitly include
//      <ole2.h> to get the <objbase.h> definition, then use a hacky macro
//      to disable the ATL version so it doesn't conflict with the OLE one.
//
//  2.  ATL has methods called SubclassWindow, which conflicts with a
//      macro in <windowsx.h>, so we must include <windowsx.h> after ATL.
//
//  3.  We want ATL to use the shell debug macros, so we must include
//      <debug.h> before ATL so it can see the shell debug macros.
//
//  4.  VariantInit is such a trivial function that we inline it in order
//      to avoid pulling in OleAut32.
//
//  5.  We want ATL to use the shell version of the ANSI/UNICODE conversion
//      functions (because the shell versions can be called from C).
//

#include <windows.h>
#include <ole2.h>           // Get the real InlineIsEqualGUID
#define _ATL_NO_DEBUG_CRT   // Use the shell debug macros
#include <debug.h>          // Get the shell debug macros
#include <shconv.h>         // Shell version of <atlconv.h>

#define VariantInit(p) memset(p, 0, sizeof(*(p)))

#ifdef __cplusplus

#define _ATL_APARTMENT_THREADED

#ifndef _SYS_GUID_OPERATORS_
// Re-route the ATL version of InlineIsEqualGUID
#define InlineIsEqualGUID ATL_InlineIsEqualGUID
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <atlctl.h>
#include <atliface.h>
#include <atlwin.h>

#ifndef _SYS_GUID_OPERATORS_
#undef InlineIsEqualGUID    // Return InlineIsEqualGUID to its normal state
#endif

#endif  /* __cplusplus */

// end of ATL Stuff
//--------------------------------------------------------------------------
#ifndef _SYS_GUID_OPERATORS_
#ifdef _OLE32_ // {
// turning on _OLE32_ (which we did for delay load stuff) gives us f-a-t
// versions of IsEqualGUID.  undo that here (hack on top of a hack...)
#undef IsEqualGUID
#ifdef __cplusplus
__inline BOOL IsEqualGUID(IN REFGUID rguid1, IN REFGUID rguid2)
{
    return !memcmp(&rguid1, &rguid2, sizeof(GUID));
}
#else   //  ! __cplusplus
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#endif  //  __cplusplus
#endif // }
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

// This flag indicates that we are on a system where data alignment is a concern

#if (defined(UNICODE) && (defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)))
#define ALIGNMENT_SCENARIO
#endif

#include <windowsx.h>
#ifdef WINNT_ENV
#include <winnetp.h>
#endif

#ifndef WINNT
//
//  Win95 doesn't have BroadcastSystemMessageA/W; it just has the plain
//  version.
//
#undef BroadcastSystemMessage
WINAPI BroadcastSystemMessage(DWORD, LPDWORD, UINT, WPARAM, LPARAM);
#endif

//
//  Dependencies among header files:
//
//      <oaidl.h> must come before <shlwapi.h> if you want to have
//      OLE command target helper functions.
//
#include <hlink.h> // must include before shellp in order to get IBrowserService2!
#include <commctrl.h>
#include <shellapi.h>
#include <wininet.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <port32.h>         // in    shell\inc
#define DISALLOW_Assert
#include <linkinfo.h>
#include <shlobjp.h>
#include <shsemip.h>
#include <docobj.h>
#include <shguidp.h>
#include <shellp.h>
#include <shldispp.h>
#include <shdocvw.h>
#include <iethread.h>
#include "browseui.h"
#include <ccstock.h>
#include <ccstock2.h>
#include <msi.h>
#include <objidl.h>
#include "apithk.h"
#define SECURITY_WIN32
#include <security.h>
#include <mlang.h>
#include <krnlcmn.h>    // GetProcessDword

#ifndef NO_MULTIMON
#include "multimon.h"   // for multiple-monitor APIs on pre-Nashville systems
#endif // NO_MULTIMON

#include <heapaloc.h>

#ifdef WINNT
#include <fmifs.h>
#endif // WINNT

#ifdef PW2
#include <penwin.h>
#endif //PW2

#include "util.h"
#include "cstrings.h"
#include "securent.h"

#ifdef WINNT
#include "winprtp.h"
#endif

#include "dllload.h"

#include "../lib/qistub.h"
#ifdef DEBUG
#include "../lib/dbutil.h"
#endif

#ifndef WINNT
//
// Don't use functions that aren't implemented on Win9x
//
#define lstrcatW       Do_not_use_lstrcatW_use_StrCatBuffW
#define wsprintfW      Do_not_use_wsprintfW_use_wnsprintfW
#define lstrncmpiW     Do_not_use_lstrncmpiW_use_StrCmpNIW
#endif

#define CP_HEBREW        1255
#define CP_ARABIC        1256

#define CODESEG


EXTERN_C const UNALIGNED ITEMIDLIST c_idlDesktop;   // NULL IDList


#if !defined(DBCS) || defined(WINNT)
// NB - These are already macros in Win32 land.
#undef CharNext
#undef CharPrev

#define CharNext(x) ((x)+1)
#define CharPrev(y,x) ((x)-1)
#define IsDBCSLeadByte(x) ((x), FALSE)
#endif // !defined(DBCS) || defined(WINNT)

#ifdef WINNT

// nothunk.c

//
// Until we figure out what we want to do with these functions,
// define them to an internal name so that we don't cause the
// linker to spew at us
#undef ReinitializeCriticalSection
#undef LoadLibrary16
#undef FreeLibrary16
#undef GetProcAddress16
#define ReinitializeCriticalSection NoThkReinitializeCriticalSection
#define LoadLibrary16 NoThkLoadLibrary16
#define FreeLibrary16 NoThkFreeLibrary16
#define GetProcAddress16 NoThkGetProcAddress16
#define GetModuleHandle16 NoThkGetModuleHandle16

VOID WINAPI NoThkReinitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    );

HINSTANCE WINAPI NoThkLoadLibrary16(
    LPCTSTR lpLibFileName
    );

BOOL WINAPI NoThkFreeLibrary16(
    HINSTANCE hLibModule
    );

FARPROC WINAPI NoThkGetProcAddress16(
    HINSTANCE hModule,
    LPCSTR lpProcName
    );


DWORD
SetPrivilegeAttribute(
    IN  LPCTSTR PrivilegeName,
    IN  DWORD   NewPrivilegeAttributes,
    OUT DWORD   *OldPrivilegeAttribute
    );

#endif // WINNT

#ifdef WINDOWS_ME
//
// This is needed for BiDi localized win95 RTL stuff
//
extern BOOL g_bBiDiW95Loc;

#else // !WINDOWS_ME
#define g_bBiDiW95Loc FALSE
#endif // WINDOWS_ME

// defcm.c
STDAPI CDefFolderMenu_CreateHKeyMenu(HWND hwndOwner, HKEY hkey, IContextMenu **ppcm);
STDAPI CDefFolderMenu_Create2Ex(LPCITEMIDLIST pidlFolder, HWND hwnd,
                                UINT cidl, LPCITEMIDLIST *apidl,
                                IShellFolder *psf, IContextMenuCB *pcmcb, 
                                UINT nKeys, const HKEY *ahkeyClsKeys, 
                                IContextMenu **ppcm);
STDAPI CDefFolderMenu_CreateEx(LPCITEMIDLIST pidlFolder,
                             HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                             IShellFolder *psf, IContextMenuCB *pcmcb, 
                             HKEY hkeyProgID, HKEY hkeyBaseProgID,
                             IContextMenu **ppcm);


// drivesx.c
BOOL IsUnavailableNetDrive(int iDrive);
BOOL IsDisconnectedNetDrive(int iDrive);
BOOL IsAudioDisc(LPTSTR pszDrive);
BOOL IsDVDDisc(int iDrive);

// futil.c
BOOL  IsShared(LPNCTSTR pszPath, BOOL fUpdateCache);
DWORD GetConnection(LPCTSTR lpDev, LPTSTR lpPath, UINT cbPath, BOOL bConvertClosed);

// rundll32.c
HWND _CreateStubWindow(POINT* ppt, HWND hwndParent);
#define STUBM_SETDATA       (WM_USER)
#define STUBM_GETDATA       (WM_USER + 1)
#define STUBM_SETICONTITLE  (WM_USER + 2)

#define STUBCLASS_PROPSHEET     1
#define STUBCLASS_FORMAT        2

// shlexe.c
BOOL IsDarwinEnabled();
STDAPI ParseDarwinID(LPTSTR pszDarwinDescriptor, LPTSTR pszDarwinCommand, DWORD cchDarwinCommand);

// browse.cpp
#define MYDOCS_CLSID TEXT("{450d8fba-ad25-11d0-98a8-0800361b1103}") // CLSID_MyDocuments
STDAPI_(LPITEMIDLIST) MyDocsIDList();
STDAPI GetMyDocumentsDisplayName(LPTSTR pszPath, UINT cch);

// shprsht.c
typedef struct _UNIQUESTUBINFO {
    HWND    hwndStub;
    HANDLE  hClassPidl;
    HICON   hicoStub;
} UNIQUESTUBINFO;
STDAPI_(BOOL) EnsureUniqueStub(LPITEMIDLIST pidl, int iClass, POINT *ppt, UNIQUESTUBINFO *pusi);
STDAPI_(void) FreeUniqueStub(UNIQUESTUBINFO *pusi);
STDAPI_(void) SHFormatDriveAsync(HWND hwnd, UINT drive, UINT fmtID, UINT options);

// bitbuck.c
void  RelayMessageToChildren(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL IsFileInBitBucket(LPCTSTR pszPath);

BOOL CreateWriteCloseFile(HWND hwnd, LPCTSTR pszFileName, void *pv, DWORD cbData);

// idlist.c
STDAPI_(BOOL) SHIsValidPidl(LPCITEMIDLIST pidl);

#ifdef WINNT
// os.c
STDAPI_(BOOL) IsExeTSAware(LPCTSTR pszExe);
#endif // WINNT

// exec stuff

/* common exe code with error handling */
#define SECL_USEFULLPATHDIR     0x00000001
#define SECL_NO_UI              0x00000002
#define SECL_SEPARATE_VDM   0x00000002
BOOL ShellExecCmdLine(HWND hwnd, LPCTSTR lpszCommand, LPCTSTR lpszDir,
        int nShow, LPCTSTR lpszTitle, DWORD dwFlags);
#define ISSHELLEXECSUCCEEDED(hinst) ((UINT_PTR)hinst>32)
#define ISWINEXECSUCCEEDED(hinst)   ((UINT_PTR)hinst>=32)
void _ShellExecuteError(LPSHELLEXECUTEINFO pei, LPCTSTR lpTitle, DWORD dwErr);

// fsnotify.c (private stuff) ----------------------

BOOL SHChangeNotifyInit();
void SHChangeNotifyTerminate(BOOL);
void SHChangeNotifyReceiveEx(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime);
LRESULT SHChangeNotify_OnNotify(WPARAM wParam, LPARAM lParam);
LRESULT SHChangeNotify_OnChangeRegistration(WPARAM wParam, LPARAM lParam);
LRESULT SHChangeNotify_OnNotifySuspendResume(WPARAM wParam, LPARAM lParam);
LRESULT SHChangeNotify_OnDeviceChange(ULONG_PTR code, struct _DEV_BROADCAST_HDR *pbh);
void    SHChangeNotify_DesktopInit();
void    SHChangeNotify_DesktopTerm();

void _Shell32ThreadAddRef(BOOL fLeaveSuspended);
void _Shell32ThreadRelease(UINT nClients);
void _Shell32ThreadAwake(void);

// Entry points for managing registering name to IDList translations.
void NPTRegisterNameToPidlTranslation(LPCTSTR pszPath, LPCITEMIDLIST pidl);
LPCTSTR NPTMapNameToPidl(LPCTSTR pszPath, LPCITEMIDLIST *ppidl);


HKEY SHGetExplorerHkey(HKEY hkeyRoot, BOOL bCreate);
HKEY SHGetExplorerSubHkey(HKEY hkRoot, LPCTSTR szSubKey, BOOL bCreate);

// path.c (private stuff) ---------------------

#define PQD_NOSTRIPDOTS 0x00000001

STDAPI_(void) PathQualifyDef(LPTSTR psz, LPCTSTR szDefDir, DWORD dwFlags);

STDAPI_(BOOL) PathIsRemovable(LPCTSTR pszPath);
STDAPI_(BOOL) PathIsTemporary(LPCTSTR pszPath);
STDAPI_(BOOL) PathIsWild(LPCTSTR pszPath);
STDAPI_(BOOL) PathIsLnk(LPCTSTR pszFile);
STDAPI_(BOOL) PathIsSlow(LPCTSTR pszFile, DWORD dwFileAttr);
STDAPI_(BOOL) PathIsHighLatency(LPCTSTR pszFile, DWORD dwFileAttr);
STDAPI_(BOOL) PathIsInvalid(LPCTSTR pPath);
STDAPI_(BOOL) PathIsBinaryExe(LPCTSTR szFile);
STDAPI_(BOOL) PathMergePathName(LPTSTR pPath, LPCTSTR pName);
STDAPI_(BOOL) PathGetMountPointFromPath(LPCTSTR pcszPath, LPTSTR pszMountPoint, int cchMountPoint);
STDAPI_(BOOL) PathIsShortcutToProgram(LPCTSTR pszFile);

STDAPI_(BOOL) NetPathExists(LPCTSTR lpszPath, DWORD *pdwType);

#if (defined(UNICODE) && (defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)))

#else

#define uaPathFindExtension PathFindExtension

#endif

void SpecialFolderIDTerminate();
void ReleaseRootFolders();
extern HINSTANCE g_hinst;

// get the desktop HWND if it is this process...
HWND GetInProcDesktop();

#ifdef WINNT
extern BOOL g_bRunOnNT5;
#define g_bRunOnMemphis     FALSE
#else
extern BOOL g_bRunOnMemphis;
#define g_bRunOnNT5         FALSE
#endif

//
// Is Mirroring APIs enabled (BiDi Memphis and NT5 only)
//
extern BOOL g_bMirroredOS;

//
// Is DATE_LTRREADING supported by GetDateFormat() API?  (it is supported in all the BiDi platforms.)
//
extern BOOL g_bBiDiPlatform;

//
// NOTE these are the size of the icons in our ImageList, not the system
// icon size.
//
extern int g_cxIcon, g_cyIcon;
extern int g_cxSmIcon, g_cySmIcon;

extern HIMAGELIST g_himlIcons;
extern HIMAGELIST g_himlIconsSmall;

// for control panel and printers folder:
extern TCHAR const c_szNull[];
extern TCHAR const c_szDotDot[];
extern TCHAR const c_szRunDll[];
extern TCHAR const c_szNewObject[];


// lang platform
extern UINT g_uCodePage;


// other stuff
#define HINST_THISDLL   g_hinst

//
// Trace/dump/break flags specific to shell32.
//   (Standard flags defined in shellp.h)
//

// Trace flags
#define TF_IMAGE            0x00000010      // Image/icon related stuff
#define TF_PROPERTY         0x00000020      // Property traces
#define TF_PATH             0x00000040      // Path whacking traces
#define TF_MENU             0x00000080      // Menu stuff
#define TF_ALLOC            0x00000100      // Allocation traces
#define TF_REG              0x00000200      // Registry traces
#define TF_DDE              0x00000400      // Shell progman DDE message tracing
#define TF_HASH             0x00000800      // Hash table stuff
#define TF_ASSOC            0x00001000      // File/URL Association traces
#define TF_FILETYPE         0x00002000      // File Type stuff
#define TF_SHELLEXEC        0x00004000      // ShellExecute stuff
#define TF_OLE              0x00008000      // OLE-specific stuff
#define TF_DEFVIEW          0x00010000      // Defview
#define TF_PERF             0x00020000      // Performance timings
#define TF_FSNOTIFY         0x00040000      // FSNotify stuff
#define TF_LIFE             0x00080000      // Object lifetime traces
#define TF_IDLIST           0x00100000      // "PIDLy" things
#define TF_FSTREE           0x00200000      // FSTree traces
#define TF_PRINTER          0x00400000      // Printer traces
//#define TF_QISTUB          0x00800000      // defined in unicpp\shellprv.h
#define TF_DOCFIND          0x01000000      // DocFind

#define TF_CUSTOM1          0x40000000      // Custom messages #1
#define TF_CUSTOM2          0x80000000      // Custom messages #2

// "Olde names"
#define DM_ALLOC            TF_ALLOC
#define DM_REG              TF_REG

// Function trace flags
#define FTF_DEFVIEW         0x00000004      // DefView calls
#define FTF_DDE             0x00000008      // DDE functions

// Dump flags
#define DF_INTSHCUT         0x00000001      // Internet shortcut structures
#define DF_HASH             0x00000002      // Hash table
#define DF_FSNPIDL          0x00000004      // Pidl for FSNotify
#define DF_URLPROP          0x00000008      // URL property structures
#define DF_DEBUGQI          0x00000010
#define DF_DEBUGQINOREF     0x00000020
#define DF_ICONCACHE        0x00000040      // Icon cache
#define DF_CLASSFLAGS       0x00000080      // File class cache
#define DF_DELAYLOADDLL     0x00000100      // Delay load

// Break flags
#define BF_ONLOADED         0x00000010      // Stop when loaded
#define BF_COCREATEINSTANCE 0x10000000      // On CoCreateInstance failure

// Debugging strings
#define GEN_DEBUGSTRW(str)  ((str) ? (str) : L"<Null Str>")
#define GEN_DEBUGSTRA(str)  ((str) ? (str) : "<Null Str>")

#ifdef UNICODE
#define GEN_DEBUGSTR  GEN_DEBUGSTRW
#else // UNICODE
#define GEN_DEBUGSTR  GEN_DEBUGSTRA
#endif // UNICODE

// Note:  raymondc - ATOMICRELEASE isn't particularly atomic.  There is a race
// condition if two people try to ATOMICRELEASE the same thing simultaneously.

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
#define ATOMICRELEASE(p) IUnknown_AtomicRelease((void **)&p)
#endif
#endif //ATOMICRELEASE

#ifdef SAFERELEASE
#undef SAFERELEASE
#endif
#define SAFERELEASE(p) ATOMICRELEASE(p)


// fileicon.c
void    FileIconTerm(void);


#define CCH_KEYMAX      64          // DOC: max size of a reg key (under shellex)

void ReplaceParams(LPTSTR szDst, LPCTSTR szFile);


#ifdef __IPropertyStorage_INTERFACE_DEFINED__
WINSHELLAPI HRESULT SHPropVariantClear(PROPVARIANT * ppropvar);
WINSHELLAPI HRESULT SHFreePropVariantArray(ULONG cel, PROPVARIANT * ppropvar);
WINSHELLAPI HRESULT SHPropVariantCopy(PROPVARIANT * ppropvar, const PROPVARIANT * ppropvarFrom);
#endif


//
// fsassoc.c
//

#define GCD_MUSTHAVEOPENCMD     0x0001
#define GCD_ADDEXETODISPNAME    0x0002  // must be used with GCD_MUSTHAVEOPENCMD
#define GCD_ALLOWPSUDEOCLASSES  0x0004  // .ext type extensions

// Only valid when used with FillListWithClasses
#define GCD_MUSTHAVEEXTASSOC    0x0008  // There must be at least one extension assoc

BOOL GetClassDescription(HKEY hkClasses, LPCTSTR pszClass, LPTSTR szDisplayName, int cbDisplayName, UINT uFlags);
void FillListWithClasses(HWND hwnd, BOOL fComboBox, UINT uFlags);
void DeleteListAttoms(HWND hwnd, BOOL fComboBox);

//
// Registry key handles
//
extern HKEY g_hkcrCLSID;        // Cached HKEY_CLASSES_ROOT\CLSID
extern HKEY g_hklmExplorer;     // Cached HKEY_LOCAL_MACHINE\...\Explorer

#ifdef WINNT
extern HKEY g_hklmApprovedExt;      // For approved shell extensions
#endif

// always zero, see init.c
extern const LARGE_INTEGER g_li0;
extern const ULARGE_INTEGER g_uli0;


// from link.c
BOOL PathSeperateArgs(LPTSTR pszPath, LPTSTR pszArgs);

// from fstree.cpp and drives.cpp

STDAPI SFVCB_OnAddPropertyPages(IN DWORD pv, IN SFVM_PROPPAGE_DATA * ppagedata);

//
// this used to be in shprst.c
//

#define MAX_FILE_PROP_PAGES 32

HKEY NetOpenProviderClass(HDROP);
void OpenNetResourceProperties(HWND, HDROP);

// msgbox.c
// Constructs strings like ShellMessagebox "xxx %1%s yyy %2%s..."
// BUGBUG: convert to use george's new code in setup
LPTSTR WINCAPI ShellConstructMessageString(HINSTANCE hAppInst, LPCTSTR lpcText, ...);

// fileicon.c
int     SHAddIconsToCache(HICON hIcon, HICON hIconSmall, LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags);
HICON   SimulateDocIcon(HIMAGELIST himl, HICON hIcon, BOOL fSmall);



//  Copy.c
#define SPEED_SLOW  400
DWORD GetPathSpeed(LPCTSTR pszPath);

// shlobjs.c
STDAPI InvokeFolderCommandUsingPidl(LPCMINVOKECOMMANDINFOEX pici,
        LPCTSTR pszPath, LPCITEMIDLIST pidl, HKEY hkClass, ULONG fExecuteFlags);

// mulprsht.c
typedef struct {
    BOOL            bContinue;      // tell thread to stop or mark as done
    ULONGLONG       cbSize;         // total size of all files in folder
    ULONGLONG       cbActualSize;   // total size on disk, taking into account compression and cluster slop
    DWORD           dwClusterSize;  // the size of a cluster
    int             cFiles;         // # files in folder
    int             cFolders;       // # folders in folder
    TCHAR           szPath[MAX_PATH];
    WIN32_FIND_DATA fd;             // for thread stack savings

} FOLDERCONTENTSINFO;

STDAPI_(BOOL) SHEncryptFile(LPCTSTR pszPath, BOOL fEncrypt);

void _FolderSize(LPCTSTR pszDir, FOLDERCONTENTSINFO * pfci);

// wuutil.c
void cdecl SetFolderStatusText(HWND hwndStatus, int iField, UINT ids,...);

#ifdef DEBUG
extern BOOL  g_bInDllEntry;

#undef SendMessage
#define SendMessage  SendMessageD
LRESULT WINAPI SendMessageD(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

//
//  The DEBUG build validates that every class we register is in the
//  unregister list so we don't leak classes at unload.
//
#undef RegisterClass
#undef RegisterClassEx
#define RegisterClass       RegisterClassD
#define RegisterClassEx     RegisterClassExD
ATOM WINAPI RegisterClassD(CONST WNDCLASS *lpWndClass);
ATOM WINAPI RegisterClassExD(CONST WNDCLASSEX *lpWndClass);
#endif // DEBUG

#ifdef UNICODE
#define RealRegisterClass   RegisterClassW
#define RealRegisterClassEx RegisterClassExW
#else
#define RealRegisterClass   RegisterClassA
#define RealRegisterClassEx RegisterClassExA
#endif

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
#define RealFindWindowEx        FindWindowExW
#else
#define RealFindWindowEx        FindWindowExA
#endif // UNICODE
#endif // DEBUG

#ifdef WINNT
// our wrapper for GetCompressedFileSize, which is NT only
STDAPI_(DWORD) SHGetCompressedFileSizeW(LPCWSTR pszFileName, LPDWORD pFileSizeHigh);

#undef GetCompressedFileSize
#define GetCompressedFileSize SHGetCompressedFileSize

#ifdef UNICODE
#define SHGetCompressedFileSize SHGetCompressedFileSizeW
#else
#define SHGetCompressedFileSize #error // not implemented, because its an nt only API
#endif // UNICODE

#endif // WINNT


#define ASSERTDLLENTRY      ASSERT(g_bInDllEntry);

//
// STATIC macro
//
#ifndef STATIC
#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif
#endif

//
// Debug helper functions
//


//
// Validation functions
//

BOOL IsValidPSHELLEXECUTEINFO(LPSHELLEXECUTEINFO pei);


#ifdef WINNT
//
// Defining FULL_DEBUG allows us debug memory problems.
//
#if defined(FULL_DEBUG)
#include <deballoc.h>
#endif // defined(FULL_DEBUG)

#endif

#define FillExecInfo(_info, _hwnd, _verb, _file, _params, _dir, _show) \
        (_info).hwnd            = _hwnd;        \
        (_info).lpVerb          = _verb;        \
        (_info).lpFile          = _file;        \
        (_info).lpParameters    = _params;      \
        (_info).lpDirectory     = _dir;         \
        (_info).nShow           = _show;        \
        (_info).fMask           = 0;            \
        (_info).cbSize          = SIZEOF(SHELLEXECUTEINFO);

STDAPI_(LONG) SHRegQueryValueA(HKEY hKey, LPCSTR lpSubKey, LPSTR lpValue, LONG *lpcbValue);
STDAPI_(LONG) SHRegQueryValueW(HKEY hKey, LPCWSTR lpSubKey, LPWSTR lpValue, LONG *lpcbValue);

#ifdef UNICODE
#define SHRegQueryValue     SHRegQueryValueW
#else
#define SHRegQueryValue     SHRegQueryValueA
#endif

#ifdef DEBUG
#if 1
    __inline DWORD clockrate() {LARGE_INTEGER li; QueryPerformanceFrequency(&li); return li.LowPart;}
    __inline DWORD clock()     {LARGE_INTEGER li; QueryPerformanceCounter(&li);   return li.LowPart;}
#else
    __inline DWORD clockrate() {return 1000;}
    __inline DWORD clock()     {return GetTickCount();}
#endif

    #define TIMEVAR(t)    DWORD t ## T; DWORD t ## N
    #define TIMEIN(t)     t ## T = 0, t ## N = 0
    #define TIMESTART(t)  t ## T -= clock(), t ## N ++
    #define TIMESTOP(t)   t ## T += clock()
    #define TIMEFMT(t)    ((DWORD)(t) / clockrate()), (((DWORD)(t) * 1000 / clockrate())%1000)
    #define TIMEOUT(t)    if (t ## N) TraceMsg(TF_PERF, #t ": %ld calls, %ld.%03ld sec (%ld.%03ld)", t ## N, TIMEFMT(t ## T), TIMEFMT(t ## T / t ## N))
#else
    #define TIMEVAR(t)
    #define TIMEIN(t)
    #define TIMESTART(t)
    #define TIMESTOP(t)
    #define TIMEFMT(t)
    #define TIMEOUT(t)
#endif

// in extract.c
STDAPI_(DWORD) GetExeType(LPCTSTR pszFile);
STDAPI_(UINT)  ExtractIcons(LPCTSTR szFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT *piconid, UINT nIcons, UINT flags);

// defxicon.c

STDAPI SHCreateDefExtIconKey(HKEY hkey, LPCTSTR pszModule, int iIcon, int iIconOpen, UINT uFlags, REFIID riid, void **pxiconOut);
STDAPI SHCreateDefExtIcon(LPCTSTR pszModule, int iIcon, int iIconOpen, UINT uFlags, REFIID riid, void **pxiconOut);


STDAPI_(UINT) SHSysErrorMessageBox(HWND hwnd, LPCTSTR pszTitle, UINT idTemplate, DWORD err, LPCTSTR pszParam, UINT dwFlags);

//======Hash Item=============================================================
typedef struct _HashTable * PHASHTABLE;
#define PHASHITEM LPCTSTR

typedef void (CALLBACK *HASHITEMCALLBACK)(PHASHTABLE pht, LPCTSTR sz, UINT wUsage, DWORD param);

STDAPI_(LPCTSTR) FindHashItem  (PHASHTABLE pht, LPCTSTR lpszStr);
STDAPI_(LPCTSTR) AddHashItem   (PHASHTABLE pht, LPCTSTR lpszStr);
STDAPI_(LPCTSTR) DeleteHashItem(PHASHTABLE pht, LPCTSTR lpszStr);
STDAPI_(LPCTSTR) PurgeHashItem (PHASHTABLE pht, LPCTSTR lpszStr);
#define     GetHashItemName(pht, sz, lpsz, cch)  lstrcpyn(lpsz, sz, cch)

PHASHTABLE  WINAPI CreateHashItemTable(UINT wBuckets, UINT wExtra, BOOL fCaseSensitive);
void        WINAPI DestroyHashItemTable(PHASHTABLE pht);

void        WINAPI SetHashItemData(PHASHTABLE pht, LPCTSTR lpszStr, int n, DWORD_PTR dwData);
DWORD_PTR   WINAPI GetHashItemData(PHASHTABLE pht, LPCTSTR lpszStr, int n);
void *      WINAPI GetHashItemDataPtr(PHASHTABLE pht, LPCTSTR lpszStr);

void        WINAPI EnumHashItems(PHASHTABLE pht, HASHITEMCALLBACK callback, DWORD dwParam);

#ifdef DEBUG
void        WINAPI DumpHashItemTable(PHASHTABLE pht);
#endif


//======== Text thunking stuff ===========================================================

#ifdef WINNT

typedef struct _THUNK_TEXT_
{
    LPTSTR m_pStr[1];
} ThunkText;

#ifdef UNICODE
    typedef CHAR        XCHAR;
    typedef LPSTR       LPXSTR;
    typedef const XCHAR * LPCXSTR;
    #define lstrlenX(r) lstrlenA(r)
#else // unicode
    typedef WCHAR       XCHAR;
    typedef LPWSTR      LPXSTR;
    typedef const XCHAR * LPCXSTR;
    #define lstrlenX(r) lstrlenW(r)
#endif // unicode

ThunkText * ConvertStrings(UINT cCount, ...);

#else //

typedef LPSTR LPXSTR;
#define lstrlenX lstrlenA

#endif  // winnt

#include "uastrfnc.h"
#ifdef __cplusplus
}       /* End of extern "C" { */
extern "C" inline __cdecl _purecall(void) {return 0;}
#ifndef _M_PPC
#pragma intrinsic(memcpy)
#pragma intrinsic(memcmp)
#endif
#endif /* __cplusplus */

#include <help.h>


//======== Discriminate inclusion ========================================

#ifndef NO_INCLUDE_UNION        // define this to avoid including all
                                // of the extra files that were not
                                // previously included in shellprv.h
#include <wchar.h>
#include <tchar.h>

//
// NT header files
//
#ifdef WINNT
#include <process.h>
#include <wowshlp.h>
#include <vdmapi.h>
#include <shell.h>
#include "dde.h"
#endif


//
// Chicago header files
//
#include <regstr.h>
#include "findhlp.h"
#include <krnlcmn.h>
#include <dlgs.h>
#include <err.h>
#include <msprintx.h>
#include <pif.h>
#include <windisk.h>
#include <brfcasep.h>
#include <trayp.h>
#include <brfcasep.h>
#include <wutilsp.h>

//
// SHELLDLL Specific header files
//
#include "bitbuck.h"
#include "defview.h"
#include "drawpie.h"
#include "fileop.h"
#include "filetbl.h"
#include "pidl.h"
#include "idmk.h"
#include "ids.h"
#include "lvutil.h"
#include <newexe.h>
#include "newres.h"
#include "ole2dup.h"
#include "os.h"
#include "privshl.h"
#include "reglist.h"
#include "shell32p.h"
#include "shitemid.h"
#include "shlgrep.h"
#include "shobjprv.h"
#include "undo.h"
#include "vdate.h"
#include "views.h"
#include "ynlist.h"

#ifdef WINNT
// NT shell uses 32-bit version of this pifmgr code.
#ifndef NO_PIF_HDRS
#include "pifmgrp.h"
#include "piffntp.h"
#include "pifinfp.h"
#include "doshelp.h"
#include "machinep.h"   // Japanese domestic machine (NEC) support
#include "oemhard.h"
#endif
#endif


#endif // NO_INCLUDE_UNION

#include "shdguid.h"

#define SetWindowBits SHSetWindowBits
#define IsSameObject SHIsSameObject
#define IsChildOrSelf SHIsChildOrSelf
#define MenuIndexFromID  SHMenuIndexFromID
#define _GetMenuFromID  SHGetMenuFromID
#define GetCurColorRes SHGetCurColorRes
#define WaitForSendMessageThread SHWaitForSendMessageThread

#define MAX_URL_STRING      INTERNET_MAX_URL_LENGTH

// Stack allocated BSTR (to avoid calling SysAllocString)
typedef struct _SA_BSTR {
    ULONG   cb;
    WCHAR   wsz[MAX_URL_STRING];
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
DECLARE_CONST_BSTR(s_sstrSRCMember,        L"src");

// The W version of this API has been implemented in shlwapi, so we save code
// and use that version.  If we include w95wraps.h we'll get this definition
// for us, but shell32 isn't single binary yet so we don't use it.
#define ShellMessageBoxW ShellMessageBoxWrapW

//======== Header file hacks =============================================================

//
//  The compiler will tell us if we are defining these NT5-only parameters
//  incorrectly.  If you get "invalid redefinition" errors, it means that
//  the definition in windows.h changed and we need to change to match.
//

#define ASFW_ANY    ((DWORD)-1)

#if _WIN32_WINNT < 0x0500
EXTERN_C BOOL AllowSetForegroundWindow( DWORD dwProcId );
#endif

#define CMIDM_LINK      0x0001
#define CMIDM_COPY      0x0002
#define CMIDM_MOVE      0x0003

// Downlevel shutdown dialog function
DWORD DownlevelShellShutdownDialog(HWND hwndParent, DWORD dwItems, LPCTSTR szUsername);

#ifdef WINNT
// from shell32\unicode\format.c
STDAPI_(DWORD) SHChkDskDriveEx(HWND hwnd, LPWSTR pszDrive);
#endif

//
// On NT, sometimes CreateDirectory succeeds in creating the directory, but, you can not do
// anything with it that directory. This happens if the directory name being created does
// not have room for an 8.3 name to be tagged onto the end of it,
// i.e., lstrlen(new_directory_name)+12 must be less or equal to MAX_PATH.
//
// the magic # "12" is 8 + 1 + 3 for and 8.3 name.
// 
// The following macro is used in places where we need to detect this to make
// MoveFile to be consistent with CreateDir(files  os.c and copy.c use this)
//

#define  IsDirPathTooLongForCreateDir(pszDir)    ((lstrlen(pszDir) + 12) > MAX_PATH)



// Features that we want to be able to turn on and off
// as appropriate for schedule.

// The feature will include the folder size and names of item level items in the infotip.
// Comment out #define FEATURE_FOLDER_INFOTIP in foldertip.h to remove this (it's not 
// sufficient to remove the below include).
#include "foldertip.h"

#endif // _SHELLPRV_H_
