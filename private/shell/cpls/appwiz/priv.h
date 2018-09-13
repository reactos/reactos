#ifndef _PRIV_H_
#define _PRIV_H_

#define STRICT

/* disable "non-standard extension" warnings in our code
 */
#ifndef RC_INVOKED
#pragma warning(disable:4001)
#endif

// Define these before including dldecl.h
#define DL_OLEAUT32
#define DL_OLE32

#include "dldecl.h"             // dldecl.h needs to go before everything else

#define CC_INTERNAL             // This is for some internal prshtp.h stuff


#ifndef WINNT
#undef ShellMessageBoxA
#define ShellMessageBoxA ShellMessageBox

#undef SHGetNewLinkInfo
#define SHGetNewLinkInfoA SHGetNewLinkInfo
#endif


// copied from old 
#ifdef WINNT
//
// need Wx86 definitions from ntpsapi.h
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#endif

#include <windows.h>

// VariantInit is a trivial function -- avoid using OleAut32, use intrinsic
// version of memset for a good size win
// (it's here so that atl (in stdafx.h) gets it too)
#define VariantInit(p) memset(p, 0, sizeof(*(p)))

#ifdef __cplusplus
// (stdafx.h must come before windowsx.h)
#include "stdafx.h"             // ATL header file for this component
#endif

#include <windowsx.h>
#include <ole2.h>               // to get IStream for image.c
#include "..\inc\port32.h"
#include <winerror.h>
#include <winnlsp.h>
#include <docobj.h>
#include <lm.h>
#define WANT_SHLWAPI_POSTSPLIT
#include <shlobj.h>

#define _SHLWAPI_
#include <shlwapi.h>

#include <ccstock.h>
#include <crtfree.h>
#define DISALLOW_Assert
#include <debug.h>
#include <regstr.h>
#include <msi.h>                // Darwin APIs
#include <msiquery.h>           // Darwin Datebase Query APIs
#include <wininet.h>            // For INTERNET_MAX_URL_LENGTH

#include <shappmgr.h>           // Header from idl file
#include "shellp.h"

#include <appmgmt.h>
#include "apithk.h"
#include "awthunk.h"

//
// Local includes
//

//
// Wrappers so our Unicode calls work on Win95
//

#ifdef WINNT

#define StrCmpIW            lstrcmpiW

#else

#define lstrcmpW            StrCmpW
#define lstrcmpiW           StrCmpIW
#define lstrcatW            StrCatW
#define lstrcpyW            StrCpyW
#define lstrcpynW           StrCpyNW

#endif

// This DLL needs to run correctly on Win95.  CharNextW is only stubbed
// on Win95, so we need to do this...
#define CharNextW(x)        ((x) + 1)
#define CharPrevW(y, x)     ((x) - 1)

// This is a TCHAR export on win9x and NT4, and since we need to link to
// the old shell32.nt4/shell32.w95 we #undef it here
#undef ILCreateFromPath
STDAPI_(LPITEMIDLIST) ILCreateFromPath(LPCTSTR pszPath);


//
// Trace/dump/break flags specific to shell32.
//   (Standard flags defined in debug.h)
//

// Trace flags
#define TF_OBJLIFE          0x00000010      // Object lifetime
#define TF_DSO              0x00000020      // Data source object
#define TF_FINDAPP          0x00000040      // Find app heuristic stuff
#define TF_INSTAPP          0x00000080      
#define TF_SLOWFIND         0x00000100
#define TF_TASKS            0x00000200
#define TF_CTL              0x00000400
#define TF_VERBOSEDSO       0x00000800      // squirts html and stuff

// Break flags
#define BF_ONDLLLOAD        0x00000010


// Prototype flags
#define PF_NEWADDREMOVE     0x00000001
#define PF_NOSECURITYCHECK  0x00000002
#define PF_FAKEUNINSTALL    0x00000004

// Debug functions
#ifdef DEBUG
#define TraceAddRef(classname, cref)    TraceMsg(TF_OBJLIFE, #classname "(%#08lx) %d>", (DWORD_PTR)this, cref)
#define TraceRelease(classname, cref)   TraceMsg(TF_OBJLIFE, #classname "(%#08lx) %d<", (DWORD_PTR)this, cref)
#else
#define TraceAddRef(classname, cref)    
#define TraceRelease(classname, cref)   
#endif

#define MAX_URL_STRING      INTERNET_MAX_URL_LENGTH

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);


//
// Info string MAX length
//
#define MAX_INFO_STRING MAX_PATH * 3

//
// Global variables
//
EXTERN_C HINSTANCE g_hinst;

#define HINST_THISDLL   g_hinst

#ifdef WINNT
#define g_bRunOnNT  TRUE
#ifdef DOWNLEVEL
#define g_bRunOnNT5 FALSE
#else
#define g_bRunOnNT5 TRUE
#endif
#else
#define g_bRunOnNT  FALSE
#define g_bRunOnNT5 FALSE
#endif

EXTERN_C BOOL g_bRunOnIE4Shell;

#ifdef DOWNLEVEL

#pragma message("DOWNLEVEL is defined")

#undef PathBuildRoot
#undef PathCombine
#undef PathFileExists
#undef PathIsDirectory
#undef PathIsUNC
#undef PathRemoveArgs

#undef PathFindExtension
STDAPI_(LPTSTR) PathFindExtension(LPCTSTR pszPath);

#undef PathFindFileName
STDAPI_(LPTSTR) PathFindFileName(LPCTSTR pPath);

#undef PathIsRoot
STDAPI_(BOOL) PathIsRoot(LPCTSTR pPath);

#undef PathQuoteSpaces
STDAPI_(void) PathQuoteSpaces(LPTSTR lpsz);

#undef PathRemoveBlanks
STDAPI_(void) PathRemoveBlanks(LPTSTR lpszString);

#undef PathRemoveFileSpec
STDAPI_(BOOL) PathRemoveFileSpec(LPTSTR pFile);

#undef PathGetArgs
STDAPI_(LPTSTR) PathGetArgs(LPCTSTR pszPath);

#undef PathStripToRoot
STDAPI_(BOOL) PathStripToRoot(LPTSTR szRoot);

#undef PathUnquoteSpaces
STDAPI_(void) PathUnquoteSpaces(LPTSTR lpsz);

#undef SHAnsiToUnicode
STDAPI_(int)
SHAnsiToUnicode(LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);

#undef SHDeleteKey

#ifdef UNICODE
STDAPI_(DWORD)
SHDeleteKeyW(
    IN HKEY    hkey, 
    IN LPCWSTR pwszSubKey);
#define SHDeleteKey SHDeleteKeyW
#else
STDAPI_(DWORD)
SHDeleteKeyA(
    IN HKEY   hkey, 
    IN LPCSTR pszSubKey);
#define SHDeleteKey SHDeleteKeyA
#endif

#undef SHQueryValueEx
#undef SHQueryValueExA
#undef SHQueryValueExW

STDAPI_(DWORD)
SHQueryValueExW(
    IN     HKEY    hkey,
    IN     LPCWSTR pwszValue,
    IN     LPDWORD lpReserved,
    OUT    LPDWORD pdwType,
    OUT    LPVOID  pvData,
    IN OUT LPDWORD pcbData);

STDAPI_(DWORD)
SHQueryValueExA(
    IN     HKEY    hkey,
    IN     LPCSTR  pszValue,
    IN     LPDWORD lpReserved,
    OUT    LPDWORD pdwType,
    OUT    LPVOID  pvData,
    IN OUT LPDWORD pcbData);

#ifdef UNICODE
#define SHQueryValueEx SHQueryValueExW
#else
#define SHQueryValueEx SHQueryValueExA
#endif

#undef SHStrDup

#ifdef UNICODE
STDAPI SHStrDupW(LPCWSTR psz, WCHAR **ppwsz);
#define SHStrDup SHStrDupW
#else
STDAPI SHStrDupA(LPCSTR psz, WCHAR **ppwsz);
#define SHStrDup SHStrDupA
#endif

#define OVERRIDE_SHLWAPI_PATH_FUNCTIONS

#include "shsemip.h"

#endif //DOWNLEVEL

#endif // _PRIV_H_
