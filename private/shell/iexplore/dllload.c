#include "iexplore.h"

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

// These macros produce code that looks like
#if 0

BOOL GetOpenFileNameA(LPOPENFILENAME pof)
{
    static BOOL (*pfnGetOpenFileNameA)(LPOPENFILENAME pof);

    if (ENSURE_LOADED(g_hinstCOMDLG32, "COMDLG32.DLL"))
    {
        if (pfnGetOpenFileNameA == NULL)
            pfnGetOpenFileNameA = (BOOL (*)(LPOPENFILENAME))GetProcAddress(g_hinstCOMDLG32, "GetOpenFileNameA");

        if (pfnGetOpenFileNameA)
            return pfnGetOpenFileNameA(pof);
    }
    return -1;
}
#endif

/**********************************************************************/


#ifdef DEBUG

void _DumpLoading(LPTSTR pszDLL, LPTSTR pszFunc)
{
    if (g_dwDumpFlags & DF_DELAYLOADDLL)
    {
        TraceMsg(TF_ALWAYS, "DLLLOAD: Loading %s for the first time for %s",
                 pszDLL, pszFunc);
    }
}
#define ENSURE_LOADED(_hinst, _dll, pszfn)         (_hinst ? _hinst : (_DumpLoading(#_dll, pszfn), _hinst = LoadLibrary(#_dll)))
#define ENSURE_LOADED_SHELL(_hinst, _dll, pszfn)   (_hinst ? _hinst : (_DumpLoading(#_dll, pszfn), _hinst = LoadShell(#_dll, pszfn)))

#else

#define ENSURE_LOADED(_hinst, _dll, pszfn)         (_hinst ? _hinst : (_hinst = LoadLibrary(#_dll)))
#define ENSURE_LOADED_SHELL(_hinst, _dll, pszfn)   (_hinst ? _hinst : (_hinst = LoadShell(#_dll, pszfn)))

#endif  // DEBUG


/*----------------------------------------------------------
Purpose: Performs a loadlibrary on the DLL only if the machine
         has the integrated shell installation.

*/
HINSTANCE LoadShell(LPCTSTR pszDll, LPCTSTR pszFn)
{
    if (PLATFORM_INTEGRATED != WhichPlatform())
    {
        TraceMsg(TF_ERROR, "Could not load integrated shell version of %s for %s", pszDll, pszFn);
        return NULL;
    }
    return LoadLibrary(pszDll);
}    

// IEUNIX : changed "## #" to "#" in two places -- JayF
#define DELAY_LOAD_MAP(_hinst, _dll, _ret, _fnpriv, _fn, _args, _nargs, _err) \
_ret __stdcall _fnpriv _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, #_fn))   \
    {                                   \
        AssertMsg((BOOL)_hinst, "LoadLibrary failed on " #_dll); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        AssertMsg((WhichPlatform()==PLATFORM_IE3) || (BOOL)_pfn##_fn, "GetProcAddress failed on " #_fn ); \
        if (_pfn##_fn == NULL)          \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
 }

#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err)    DELAY_LOAD_MAP(_hinst, _dll, _ret, _fn, _fn, _args, _nargs, _err)

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_DWORD(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, DWORD, _fn, _args, _nargs, 0)
#define DELAY_LOAD_UINT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, UINT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_INT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, INT, _fn, _args, _nargs, 0)

#define DELAY_MAP_DWORD(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, DWORD, _fnpriv, _fn, _args, _nargs, 0)

// IEUNIX : changed "## #" to "#" in two places -- JayF
#define DELAY_LOAD_VOID(_hinst, _dll, _fn, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, #_fn))   \
    {                                   \
        AssertMsg((BOOL)_hinst, "LoadLibrary failed on " #_dll); \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        AssertMsg((WhichPlatform()==PLATFORM_IE3) || (BOOL)_pfn##_fn, "GetProcAddress failed on " #_fn); \
        if (_pfn##_fn == NULL)          \
            return;                     \
    }                                   \
    _pfn##_fn _nargs;                   \
 }


//
// For private entrypoints exported by ordinal.
// 

// Exporting by ordinal is not available on UNIX.
// But we have all these symbols exported because it's UNIX default.
#ifdef UNIX
#define GET_PRIVATE_PROC_ADDRESS(_hinst, _fname, _ord) GetProcAddress(_hinst, #_fname)
#else
#define GET_PRIVATE_PROC_ADDRESS(_hinst, _fname, _ord) GetProcAddress(_hinst, (LPSTR) _ord)
#endif

// IEUNIX : changed "## #" to "#" in two places -- JayF
#define DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, "(ordinal " #_ord ## ")"))   \
    {                                   \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " #_dll); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GET_PRIVATE_PROC_ADDRESS(_hinst, _fn, _ord); \
                                        \
        /* GetProcAddress always returns non-NULL, even for bad ordinals.   \
           But do the check anyways...  */                                  \
                                        \
        if (_pfn##_fn == NULL)          \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
 }
        
#define DELAY_LOAD_ORD(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)


// IEUNIX : changed "## #" to "#" in two places -- JayF
#define DELAY_LOAD_ORD_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, "(ordinal " #_ord ## ")"))   \
    {                                   \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " #_dll); \
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


//
//  Private exports by ordinal for integrated-shell installs
//


// IEUNIX : changed "## #" to "#" in two places -- JayF
#define DELAY_LOAD_SHELL_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED_SHELL(_hinst, _dll, "(ordinal " #_ord ## ")"))   \
    {                                   \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " #_dll); \
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
        
#define DELAY_LOAD_SHELL(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_SHELL_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)
#define DELAY_LOAD_SHELL_HRESULT(_hinst, _dll, _fn, _ord, _args, _nargs) DELAY_LOAD_SHELL_ERR(_hinst, _dll, HRESULT, _fn, _ord, _args, _nargs, E_FAIL)


// IEUNIX : changed "## #" to "#" in two places -- JayF
#define DELAY_LOAD_SHELL_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED_SHELL(_hinst, _dll, "(ordinal " #_ord ## ")"))   \
    {                                   \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " #_dll); \
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



/**********************************************************************/
/**********************************************************************/



// --------- SHDOCVW.DLL ---------------

//
// ----  delay load post win95 shell32 private functions
//

HINSTANCE g_hinstShdocvw = NULL;

// 101 int IEWinMain(LPTSTR lpszCmdLine, int nCmdShow);
DELAY_LOAD_ORD(g_hinstShdocvw, shdocvw.dll, int, IEWinMain, 101,
    (LPTSTR pszCmdLine, int nCmdShow),
    (pszCmdLine, nCmdShow))


#pragma warning(default:4229)
