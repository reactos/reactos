#include "priv.h"
#include <wininet.h>

#pragma warning(disable:4229)  // No warnings when modifiers used on data

// Delay loading mechanism.  This allows you to write code as if you are
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
// 


/**********************************************************************/

void _GetProcFromDLL(HINSTANCE* phinst, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
#ifdef DEBUG
    CHAR szProcD[MAX_PATH];
    if (HIWORD(pszProc)) {
        lstrcpynA(szProcD, pszProc, ARRAYSIZE(szProcD));
    } else {
        wnsprintfA(szProcD, ARRAYSIZE(szProcD), "(ordinal %d)", LOWORD(pszProc));
    }
#endif // DEBUG
    // If it's already loaded, return.
    if (*ppfn) {
    return;
    }

    if (*phinst == NULL) {
#ifdef DEBUG
    TraceMsg(TF_FTP_DLLLOADING, "DLLLOAD: Loading %s for the first time for %s", pszDLL, szProcD);
    
    if (g_dwBreakFlags & 0x00000080)
    {
        DebugBreak();
    }
#endif // DEBUG
    *phinst = LoadLibraryA(pszDLL);
    if (*phinst == NULL) {
        return;
    }
    }

#ifdef DEBUG
    TraceMsg(TF_FTP_DLLLOADING, "DLLLOAD: GetProc'ing %s from %s for the first time", pszDLL, szProcD);
#endif // DEBUG
    *ppfn = GetProcAddress(*phinst, pszProc);
}

/*----------------------------------------------------------
Purpose: Performs a loadlibrary on the DLL only if the machine
     has the integrated shell installation.

*/
void _SHGetProcFromDLL(HINSTANCE* phinst, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
    _GetProcFromDLL(phinst, pszDLL, ppfn, pszProc);
}

#define DELAY_LOAD_MAP(_hinst, _dll, _ret, _fnpriv, _fn, _args, _nargs, _err) \
_ret __stdcall _fnpriv _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, #_fn); \
    if (_pfn##_fn)               \
    return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#define DELAY_MAP_HRESULT(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, HRESULT, _fnpriv, _fn, _args, _nargs, E_FAIL)
#define DELAY_MAP_DWORD(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, DWORD, _fnpriv, _fn, _args, _nargs, 0)


#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err)    DELAY_LOAD_MAP(_hinst, _dll, _ret, _fn, _fn, _args, _nargs, _err)

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_DWORD(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, DWORD, _fn, _args, _nargs, 0)
#define DELAY_LOAD_UINT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, UINT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_INT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, INT, _fn, _args, _nargs, 0)

#define DELAY_LOAD_VOID(_hinst, _dll, _fn, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, #_fn); \
    if (_pfn##_fn)              \
    _pfn##_fn _nargs;       \
    return;                     \
}

//
// For private entrypoints exported by ordinal.
// 

#define DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)               \
    return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}
    
#define DELAY_LOAD_ORD(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)
#define DELAY_LOAD_ORD_HRESULT(_hinst, _dll, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD(_hinst, _dll, HRESULT, _fn, _ord, _args, _nargs)


#define DELAY_LOAD_ORD_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)              \
    _pfn##_fn _nargs;       \
    return;                     \
}


//
//  Private exports by ordinal for integrated-shell installs
//


#define DELAY_LOAD_SHELL_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    _SHGetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)               \
    return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}
    
#define DELAY_LOAD_SHELL(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_SHELL_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)
#define DELAY_LOAD_SHELL_HRESULT(_hinst, _dll, _fn, _ord, _args, _nargs) DELAY_LOAD_SHELL_ERR(_hinst, _dll, HRESULT, _fn, _ord, _args, _nargs, E_FAIL)


#define DELAY_LOAD_SHELL_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    _SHGetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord); \
    if (_pfn##_fn)              \
    _pfn##_fn _nargs;       \
    return;                     \
}



/**********************************************************************/
/**********************************************************************/


// --------- MLANG.DLL ---------------

HINSTANCE g_hinstMLANG = NULL;

DELAY_LOAD_HRESULT(g_hinstMLANG, MLANG.DLL, ConvertINetMultiByteToUnicode,
            (LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount),
            (lpdwMode, dwEncoding, lpSrcStr, lpnMultiCharCount, lpDstStr, lpnWideCharCount));

DELAY_LOAD_HRESULT(g_hinstMLANG, MLANG.DLL, ConvertINetUnicodeToMultiByte,
            (LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount),
            (lpdwMode, dwEncoding, lpSrcStr, lpnWideCharCount, lpDstStr, lpnMultiCharCount));

DELAY_LOAD_HRESULT(g_hinstMLANG, MLANG.DLL, LcidToRfc1766W,
            (LCID Locale, LPWSTR pszRfc1766, int nChar),
            (Locale, pszRfc1766, nChar));


// --------- MSHTML.DLL ---------------

HINSTANCE g_hinstMSHTML = NULL;

DELAY_LOAD_HRESULT(g_hinstMSHTML, MSHTML.DLL, ShowHTMLDialog,
            (HWND hwnd, IMoniker * pmk, VARIANT * pvarArgIn, LPWSTR pchOptions, VARIANT * pvarArgOut),
            (hwnd, pmk, pvarArgIn, pchOptions, pvarArgOut));


// --------- SHELL32.DLL ---------------

HINSTANCE g_hinstSHELL32 = NULL;

DELAY_LOAD_ORD_HRESULT(g_hinstSHELL32, SHELL32.DLL, _SHCreateShellFolderView, SHCreateShellFolderViewORD,
                 (const SFV_CREATE* pcsfv, LPSHELLVIEW FAR* ppsv),
                 (pcsfv, ppsv));


#pragma warning(default:4229)


