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
#include "private.h"
#include <winsock.h>

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

#define ENSURE_LOADED(_hinst, _dll, pszfn)   (_hinst ? _hinst : (_hinst = LoadLibrary(#_dll)))

#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err) \
static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
_ret __stdcall _fn _args                \
{                                       \
    if (!ENSURE_LOADED(_hinst, _dll, #_fn))   \
    {                                   \
        AssertMsg(_hinst != NULL, "LoadLibrary failed on " ## #_dll); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        AssertMsg(_pfn##_fn != NULL, "GetProcAddress failed on " ## #_fn); \
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
        AssertMsg(_hinst != NULL, "LoadLibrary failed on " ## #_dll); \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        AssertMsg(_pfn##_fn != NULL, "GetProcAddress failed on " ## #_fn); \
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

// --------- OLEAUT32.DLL ---------------

HINSTANCE g_hinstOLEAUT32 = NULL;

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, RegisterTypeLib,
    (ITypeLib *ptlib, OLECHAR *szFullPath, OLECHAR *szHelpDir), (ptlib, szFullPath, szHelpDir));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, LoadTypeLib,
    (const OLECHAR *szFile, ITypeLib **pptlib), (szFile, pptlib));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, CreateErrorInfo,
    (ICreateErrorInfo **pperrinfo), (pperrinfo));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SetErrorInfo,
   (unsigned long dwReserved, IErrorInfo*perrinfo), (dwReserved, perrinfo));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, LoadRegTypeLib,
    (REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid, ITypeLib **pptlib),
    (rguid, wVerMajor, wVerMinor, lcid, pptlib));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32.DLL, BSTR, SysAllocString, (const OLECHAR*pch), (pch));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32.DLL, BSTR, SysAllocStringLen, 
    (const OLECHAR*pch, unsigned int i), (pch, i));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32.DLL, BSTR, SysAllocStringByteLen,
    (LPCSTR psz, UINT i), (psz, i));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SysStringLen, (BSTR bstr), (bstr));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SysStringByteLen, (BSTR bstr), (bstr));

DELAY_LOAD_VOID(g_hinstOLEAUT32, OLEAUT32.DLL, SysFreeString, (BSTR bs), (bs));

DELAY_LOAD_VOID(g_hinstOLEAUT32, OLEAUT32.DLL, VariantInit, (VARIANTARG *pvarg), (pvarg));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantCopy,
    (VARIANTARG *pvargDest, VARIANTARG *pvargSrc), (pvargDest, pvargSrc));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantChangeType,
    (VARIANTARG *pvargDest, VARIANTARG *pvargSrc, unsigned short wFlags, VARTYPE vt),
    (pvargDest, pvargSrc, wFlags, vt));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantClear, (VARIANTARG *pvarg), (pvarg));

#pragma warning(default:4229)

