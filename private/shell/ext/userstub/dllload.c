#undef WINSHELLAPI
#define _SHELL32_
#include <windows.h>
#include <shellapi.h>

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


#define ENSURE_LOADED(_hinst, _dll, pszfn)         (_hinst ? _hinst : (_hinst = LoadLibrary(TEXT(#_dll))))


#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, TEXT(#_fn)))   \
    {                                   \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        if (_pfn##_fn == NULL)          \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
 }

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
    if (!ENSURE_LOADED(_hinst, _dll, TEXT(#_fn)))   \
    {                                   \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        if (_pfn##_fn == NULL)          \
            return;                     \
    }                                   \
    _pfn##_fn _nargs;                   \
 }


//
// For private entrypoints exported by ordinal.
// 

#define DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, TEXT("(ordinal ") TEXT(#_ord) TEXT(")")))   \
    {                                   \
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
        
#define DELAY_LOAD_ORD(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)


#define DELAY_LOAD_ORD_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, TEXT("(ordinal ") TEXT(#_ord) TEXT(")")))   \
    {                                   \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, (LPSTR)_ord); \
                                        \
        /* GetProcAddress always returns non-NULL, even for bad ordinals.   \
           But do the check anyways...  */                                  \
                                        \
        if (_pfn##_fn == NULL)          \
            return;                     \
    }                                   \
    _pfn##_fn _nargs;                   \
}



// --------- SHELL32.DLL ----------------

HINSTANCE g_hinstSHELL32 = NULL;

#ifdef UNICODE
DELAY_LOAD(g_hinstSHELL32, SHELL32.DLL, BOOL, ShellExecuteExW,
	(LPSHELLEXECUTEINFOW lpExecInfo), (lpExecInfo));
#else
DELAY_LOAD(g_hinstSHELL32, SHELL32.DLL, BOOL, ShellExecuteExA,
	(LPSHELLEXECUTEINFOA lpExecInfo), (lpExecInfo));
#endif
DELAY_LOAD_ORD_ERR(g_hinstSHELL32, SHELL32.DLL, LONG, PathProcessCommand,
	653, (LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags),
	(lpSrc, lpDest, iDestMax, dwFlags), -1);

#pragma warning(default:4229)
