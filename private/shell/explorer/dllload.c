#include "cabinet.h"
#include "fsmenu.h"
#include <desktray.h>
#include <dbgmem.h>
#include "uemapp.h"

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


#ifdef DEBUG

void _DumpLoading(LPTSTR pszDLL, LPTSTR pszFunc)
{
    if (g_dwDumpFlags & DF_DELAYLOADDLL)
    {
        TraceMsg(TF_ALWAYS, "DLLLOAD: Loading %s for the first time for %s",
                 pszDLL, pszFunc);
    }
}
#define ENSURE_LOADED(_hinst, _dll, pszfn)         (_hinst ? _hinst : (_DumpLoading(TEXT(#_dll), pszfn), _hinst = LoadLibrary(TEXT(#_dll))))

#else

#define ENSURE_LOADED(_hinst, _dll, pszfn)         (_hinst ? _hinst : (_hinst = LoadLibrary(TEXT(#_dll))))

#endif  // DEBUG


#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, TEXT(#_fn)))   \
    {                                   \
        /*ASSERT_MSG((BOOL)_hinst, "LoadLibrary failed on " ## #_dll);*/ \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " ## #_dll); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        /*ASSERT_MSG(NULL != _pfn##_fn, "GetProcAddress failed on " ## #_fn);*/ \
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
#define DELAY_LOAD_BOOL(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, BOOL, _fn, _args, _nargs, FALSE)

#define DELAY_LOAD_VOID(_hinst, _dll, _fn, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, TEXT(#_fn)))   \
    {                                   \
        /*AssertMsg((BOOL)_hinst, "LoadLibrary failed on " ## #_dll);*/ \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " ## #_dll); \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        /*AssertMsg(NULL != _pfn##_fn, "GetProcAddress failed on " ## #_fn);*/ \
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

#define DELAY_LOAD_ORD(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)


#define DELAY_LOAD_ORD_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll, TEXT("(ordinal ") TEXT(#_ord) TEXT(")")))   \
    {                                   \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " ## #_dll); \
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


// --------- SHDOCVW.DLL ----------------

HINSTANCE g_hinstSHDOCVW = NULL;

DELAY_LOAD_ORD_VOID(g_hinstSHDOCVW, SHDOCVW.DLL, RunInstallUninstallStubs, 125,
                    (), ());

DELAY_LOAD_ORD(g_hinstSHDOCVW, SHDOCVW.DLL, BOOL, CreateFromDesktop, 115,
               (PNEWFOLDERINFO pfi), (pfi));

DELAY_LOAD_ORD_VOID(g_hinstSHDOCVW, SHDOCVW.DLL, WinList_Init, 110,
               (), ());

DELAY_LOAD_ORD_VOID(g_hinstSHDOCVW, SHDOCVW.DLL, WinList_Terminate, 111,
               (), ());

#ifdef DEBUG
DELAY_LOAD_ORD(g_hinstSHDOCVW, SHDOCVW.DLL, BOOL, GetLeakDetectionFunctionTable, 161,
               (LEAKDETECTFUNCS *pTable), (pTable));
#endif

// --------- BROWSEUI.DLL ----------------
HINSTANCE g_hinstBROWSEUI = NULL;

DELAY_LOAD_ORD(g_hinstBROWSEUI, BROWSEUI.DLL, BOOL, SHExplorerParseCmdLine, 107, 
            (PNEWFOLDERINFO pfi), (pfi));

DELAY_LOAD_ORD(g_hinstBROWSEUI, BROWSEUI.DLL, BOOL, SHCreateFromDesktop, 106, 
            (PNEWFOLDERINFO pfi), (pfi));

// --------- USER32.DLL ----------------
HINSTANCE g_hinstUSER32 = NULL;

DELAY_LOAD_BOOL(g_hinstUSER32, USER32.DLL, AllowSetForegroundWindow,
               (DWORD dwProcessId), (dwProcessId));


// --------- kernel32.DLL ----------------
HINSTANCE g_hinstKERNEL32 = NULL;

DELAY_LOAD_BOOL(g_hinstKERNEL32, KERNEL32.DLL, AssignProcessToJobObject,
               (HANDLE hJob, HANDLE hProcess ),
               (hJob, hProcess ));
DELAY_LOAD_BOOL(g_hinstKERNEL32, KERNEL32.DLL, SetInformationJobObject,
               (HANDLE hJob, JOBOBJECTINFOCLASS info, LPVOID pv, DWORD dw),
               (hJob, info, pv, dw));
DELAY_LOAD(g_hinstKERNEL32, KERNEL32.DLL, HANDLE, CreateJobObjectW,
               (LPSECURITY_ATTRIBUTES psa, LPCTSTR psz),
               (psa, psz));

// --------- TsAppCmp.DLL ----------------
HINSTANCE g_hinstTsAppCMP = NULL;
DELAY_LOAD(g_hinstTsAppCMP, TsAppCMP.DLL, ULONG, GetTermsrCompatFlagsEx,
               (LPWSTR psz, LPDWORD pdw, enum TERMSRV_COMPATIBILITY_CLASS info),
               (psz, pdw, info));


#pragma warning(default:4229)
