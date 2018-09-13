//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//
//----------------------------------------------------------------------------
#include "stdinc.h"

#pragma warning(disable:4229)  // No warnings when modifiers used on data

//----------------------------------------------------------------------------
// Delay loading mechanism.  [Stolen from shdocvw.]
//
// This allows you to write code as if you are
// calling implicitly linked APIs, and yet have these APIs really be
// explicitly linked.  You can reduce the initial number of DLLs that 
// are loaded (load on demand) using this technique.
//
// Use the following macros to indicate which APIs/DLLs are delay-linked
// and -loaded.
//
//      DELAY_LOAD
//      DELAY_LOAD_HRESULT
//      DELAY_LOAD_SAFEARRAY
//      DELAY_LOAD_UINT
//      DELAY_LOAD_INT
//      DELAY_LOAD_VOID
//
// Use these macros for APIs that are exported by ordinal only.
//
//      DELAY_LOAD_ORD
//      DELAY_LOAD_ORD_VOID     
//
// Use these macros for APIs that only exist on the integrated-shell
// installations (i.e., a new shell32 is on the system).
//
//      DELAY_LOAD_SHELL
//      DELAY_LOAD_SHELL_HRESULT
//      DELAY_LOAD_SHELL_VOID     
//
//----------------------------------------------------------------------------

#define ENSURE_LOADED(_hinst, _dll, pszfn)   (_hinst ? TRUE : NULL!=(_hinst = LoadLibraryA(#_dll)))

#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err) \
static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
_ret __stdcall _fn _args                \
{                                       \
    if (!ENSURE_LOADED(_hinst, _dll, #_fn))   \
    {                                   \
        AssertMsg((BOOL)_hinst, "LoadLibrary failed on " ## #_dll); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        AssertMsg((BOOL)_pfn##_fn, "GetProcAddress failed on " ## #_fn); \
        if (_pfn##_fn == NULL)          \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
 }

#define DELAY_LOAD_VOID(_hinst, _dll, _fn, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, #_fn))   \
    {                                   \
        AssertMsg((BOOL)_hinst, "LoadLibrary failed on " ## #_dll); \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        AssertMsg((BOOL)_pfn##_fn, "GetProcAddress failed on " ## #_fn); \
        if (_pfn##_fn == NULL)          \
            return;                     \
    }                                   \
    _pfn##_fn _nargs;                   \
 }

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_DWORD(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, DWORD, _fn, _args, _nargs, 0)
#define DELAY_LOAD_UINT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, UINT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_INT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, INT, _fn, _args, _nargs, 0)

#define DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, "(ordinal " ## #_ord ## ")"))   \
    {                                   \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " ## #_dll); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, (LPSTR) _ord); \
                                        \
        /* GetProcAddress always returns non-NULL, even for bad ordinals.   \
           But do the check anyways...  */                                  \
                                        \
        if (_pfn##_fn == NULL)          \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
 }

#define DELAY_LOAD_ORD_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, "(ordinal " ## #_ord ## ")"))   \
    {                                   \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " ## #_dll); \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, (LPSTR) _ord); \
                                        \
        /* GetProcAddress always returns non-NULL, even for bad ordinals.   \
           But do the check anyways...  */                                  \
                                        \
        if (_pfn##_fn == NULL)          \
            return;                     \
    }                                   \
    _pfn##_fn _nargs;                   \
}
        
#define DELAY_LOAD_ORD(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)

//
// And now the DLLs which are delay loaded
//

// --------- SHDOCVW.DLL ---------------

// BUGBUG: These may not remain private.
HINSTANCE g_hinstSHDOCVW = NULL;

DELAY_LOAD_ORD(g_hinstSHDOCVW, SHDOCVW.DLL, DWORD, SHRestricted2A, 158,
                (BROWSER_RESTRICTIONS rest, LPCSTR pszUrl, DWORD dwReserved), (rest, pszUrl, dwReserved));

DELAY_LOAD_ORD(g_hinstSHDOCVW, SHDOCVW.DLL, DWORD, SHRestricted2W, 159,
                (BROWSER_RESTRICTIONS rest, LPCWSTR pwzUrl, DWORD dwReserved), (rest, pwzUrl, dwReserved));

DELAY_LOAD_ORD_ERR(g_hinstSHDOCVW, SHDOCVW.DLL, HRESULT, CDDEAuto_Navigate, 162,
                (BSTR str, HWND* phwnd, long lTransID), (str, phwnd, lTransID), E_FAIL);

#ifdef UNIX
DELAY_LOAD_ORD(g_hinstSHDOCVW, SHDOCVW.DLL, HRESULT, SHAddSubscribeFavorite,163, (HWND hwnd, LPCWSTR pwszURL, LPCWSTR pwszName, DWORD dwFlags, SUBSCRIPTIONTYPE subsType, SUBSCRIPTIONINFO* pInfo), (hwnd, pwszURL, pwszName, dwFlags, subsType, pInfo));
#endif /* UNIX */

#pragma warning(default:4229)

