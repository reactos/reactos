// You are expected to #include this file from your private dllload.c.
//

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
//      DELAY_LOAD_VOID_ORD
//
// Use these macros for APIs that only exist on the integrated-shell
// installations (i.e., a new shell32 is on the system).
//
//      DELAY_LOAD_SHELL
//      DELAY_LOAD_SHELL_HRESULT
//      DELAY_LOAD_SHELL_VOID
//
//
// Use DELAY_LOAD_IE_* for APIs that come from BrowseUI.  This used
// to be important when BrowseUI was in the IEXPLORE directory, but
// now it's in the System directory so the difference is pretty
// meaningless.
//
// Use DELAY_LOAD_OCX_* for APIs that come from OCXs and not DLLs.
//

/**********************************************************************/

#ifdef DEBUG

void _DumpLoading(LPTSTR pszDLL, LPTSTR pszFunc)
{
#ifdef DF_DELAYLOADDLL
    if (g_dwDumpFlags & DF_DELAYLOADDLL)
    {
        TraceMsg(TF_ALWAYS, "DLLLOAD: Loading %s for the first time for %s",
                 pszDLL, pszFunc);
    }
#endif
}

#define ENSURE_LOADED(_hmod, _dll, _ext, pszfn)         \
    (_hmod ? (_hmod) : (_DumpLoading(TEXT(#_dll) TEXT(".") TEXT(#_ext), pszfn), \
                        _hmod = LoadLibraryA(#_dll "." #_ext)))

#else

#define ENSURE_LOADED(_hmod, _dll, _ext, pszfn)         \
    (_hmod ? (_hmod) : (_hmod = LoadLibraryA(#_dll "." #_ext)))

#endif  // DEBUG


/**********************************************************************/

void _GetProcFromDLL(HMODULE* phmod, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
#ifdef DEBUG
    CHAR szProcD[MAX_PATH];
    if (!IS_INTRESOURCE(pszProc)) {
        lstrcpynA(szProcD, pszProc, ARRAYSIZE(szProcD));
    } else {
        wsprintfA(szProcD, "(ordinal %d)", LOWORD((DWORD_PTR)pszProc));
    }
#endif
    // If it's already loaded, return.
    if (*ppfn) {
        return;
    }

    if (*phmod == NULL) {
#ifdef DEBUG
#ifdef DF_DELAYLOADDLL
        if (g_dwDumpFlags & DF_DELAYLOADDLL)
        {
            TraceMsg(TF_ALWAYS, "DLLLOAD: Loading %s for the first time for %s",
                 pszDLL, szProcD);
        }
#endif
        if (g_dwBreakFlags & 0x00000080)
        {
            DebugBreak();
        }
#endif
        *phmod = LoadLibraryA(pszDLL);
#ifdef UNIX
        if (*phmod == NULL) {
           if (lstrcmpiA(pszDLL, "inetcpl.dll") == 0) {
               *phmod = LoadLibraryA("inetcpl.cpl");
           }
        }
#endif
        if (*phmod == NULL) {
            return;
        }
    }

#if defined(DEBUG) && defined(DF_DELAYLOADDLL)
    if (g_dwDumpFlags & DF_DELAYLOADDLL) {
        TraceMsg(TF_ALWAYS, "DLLLOAD: GetProc'ing %s from %s for the first time",
             pszDLL, szProcD);
    }
#endif
    *ppfn = GetProcAddress(*phmod, pszProc);
}

#if defined(DEBUG) && defined(BROWSEUI_IN_IEXPLORE_DIRECTORY)
void _GetProcFromSystemDLL(HMODULE* phmod, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{

#ifdef UNIX
    if (lstrcmpiA(pszDLL, "inetcpl.dll") == 0) {
        _GetProcFromDLL(phmod, "inetcpl.cpl", ppfn, pszProc);
        return;
    }
#endif

    // You must use DELAY_LOAD_IE for BROWSEUI since BROWSEUI lives in the
    // IE directory, not the System directory.
    if (lstrcmpiA(pszDLL, "BROWSEUI.DLL") == 0) {
        ASSERT(!"Somebody used DELAY_LOAD instead of DELAY_LOAD_IE on BROWSEUI");
    }
    _GetProcFromDLL(phmod, pszDLL, ppfn, pszProc);
}
#else
#define _GetProcFromSystemDLL           _GetProcFromDLL
#endif

// NOTE: this takes two parameters that are the function name. the First (_fn) is the name that
// NOTE: the function will be called in this DLL and the other (_fni) is the
// NOTE: name of the function we will GetProcAddress. This helps get around functions that
// NOTE: are defined in the header files with _declspec...

//
//  HMODULE _hmod - where we cache the HMODULE (aka HINSTANCE)
//           _dll - Basename of the target DLL, not quoted
//           _ext - Extension of the target DLL, not quoted (usually DLL)
//           _ret - Data type of return value
//        _fnpriv - Local name for the function
//            _fn - Exported name for the function
//          _args - Argument list in the form (TYPE1 arg1, TYPE2 arg2, ...)
//         _nargs - Argument list in the form (arg1, arg2, ...)
//           _err - Return value if we can't call the actual function
//
#define DELAY_LOAD_NAME_EXT_ERR(_hmod, _dll, _ext, _ret, _fnpriv, _fn, _args, _nargs, _err) \
_ret __stdcall _fnpriv _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromSystemDLL(&_hmod, #_dll "." #_ext, (FARPROC*)&_pfn##_fn, #_fn); \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#define     DELAY_LOAD_NAME_ERR(_hmod, _dll,       _ret, _fnpriv, _fn, _args, _nargs, _err) \
        DELAY_LOAD_NAME_EXT_ERR(_hmod, _dll,  DLL, _ret, _fnpriv, _fn, _args, _nargs, _err)

#define DELAY_LOAD_ERR(_hmod, _dll, _ret, _fn,      _args, _nargs, _err) \
   DELAY_LOAD_NAME_ERR(_hmod, _dll, _ret, _fn, _fn, _args, _nargs, _err)

#define DELAY_LOAD(_hmod, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_UINT(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, UINT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_INT(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, INT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_BOOL(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, BOOL, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_BOOLEAN(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, BOOLEAN, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_DWORD(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, DWORD, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_LRESULT(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, LRESULT, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_WNET(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, DWORD, _fn, _args, _nargs, WN_NOT_SUPPORTED)
#define DELAY_LOAD_LPVOID(_hmod, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hmod, _dll, LPVOID, _fn, _args, _nargs, 0)

// the NAME variants allow the local function to be called something different from the imported
// function to avoid dll linkage problems.
#define DELAY_LOAD_NAME(_hmod, _dll, _ret, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, _ret, _fn, _fni, _args, _nargs, 0)
#define DELAY_LOAD_NAME_HRESULT(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, HRESULT, _fn, _fni, _args, _nargs, E_FAIL)
#define DELAY_LOAD_NAME_SAFEARRAY(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, SAFEARRAY *, _fn, _fni, _args, _nargs, NULL)
#define DELAY_LOAD_NAME_UINT(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, UINT, _fn, _fni, _args, _nargs, 0)
#define DELAY_LOAD_NAME_BOOL(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, BOOL, _fn, _fni, _args, _nargs, FALSE)
#define DELAY_LOAD_NAME_DWORD(_hmod, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hmod, _dll, DWORD, _fn, _fni, _args, _nargs, 0)

#define DELAY_LOAD_NAME_VOID(_hmod, _dll, _fn, _fni, _args, _nargs)                               \
void __stdcall _fn _args                                                                \
{                                                                                       \
    static void (__stdcall *_pfn##_fni) _args = NULL;                                   \
    if (!ENSURE_LOADED(_hmod, _dll, DLL, TEXT(#_fni)))                                       \
    {                                                                                   \
        AssertMsg(BOOLFROMPTR(_hmod), TEXT("LoadLibrary failed on ") ## TEXT(#_dll));         \
        return;                                                                         \
    }                                                                                   \
    if (_pfn##_fni == NULL)                                                              \
    {                                                                                   \
        *(FARPROC*)&(_pfn##_fni) = GetProcAddress(_hmod, #_fni);                         \
        AssertMsg(BOOLFROMPTR(_pfn##_fni), TEXT("GetProcAddress failed on ") ## TEXT(#_fni));    \
        if (_pfn##_fni == NULL)                                                          \
            return;                                                                     \
    }                                                                                   \
    _pfn##_fni _nargs;                                                                   \
}

#define DELAY_LOAD_VOID(_hmod, _dll, _fn, _args, _nargs)   DELAY_LOAD_NAME_VOID(_hmod, _dll, _fn, _fn, _args, _nargs)



// For private entrypoints exported by ordinal.
#define DELAY_LOAD_ORD_ERR(_hmod, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromSystemDLL(&_hmod, #_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#define DELAY_LOAD_ORD(_hmod, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hmod, _dll, _ret, _fn, _ord, _args, _nargs, 0)
#define DELAY_LOAD_EXT_ORD(_hmod, _dll, _ext, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hmod, #_dll "." #_ext, _ret, _fn, _ord, _args, _nargs, 0)


#define DELAY_LOAD_ORD_VOID(_hmod, _dll, _fn, _ord, _args, _nargs)                     \
void __stdcall _fn _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromSystemDLL(&_hmod, #_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)              \
        _pfn##_fn _nargs;       \
    return;                     \
}
#define DELAY_LOAD_VOID_ORD DELAY_LOAD_ORD_VOID // cuz people screw this up all the time

#define DELAY_LOAD_ORD_BOOL(_hmod, _dll, _fn, _ord, _args, _nargs) \
    DELAY_LOAD_ORD_ERR(_hmod, _dll, BOOL, _fn, _ord, _args, _nargs, 0)

#define DELAY_LOAD_EXT(_hmod, _dll, _ext, _ret, _fn, _args, _nargs) \
        DELAY_LOAD_NAME_EXT_ERR(_hmod, _dll, _ext, _ret, _fn, _fn, _args, _nargs, 0)

#define DELAY_LOAD_EXT_WRAP(_hmod, _dll, _ext, _ret, _fnWrap, _fnOrig, _args, _nargs) \
        DELAY_LOAD_NAME_EXT_ERR(_hmod, _dll, _ext, _ret, _fnWrap, _fnOrig, _args, _nargs, 0)

#if defined(BROWSEUI_IN_IEXPLORE_DIRECTORY) || defined(UNIX)
/*----------------------------------------------------------
Purpose: Loads the DLL via a CLSID it is known to be registered for
*/
void _GetProcFromCLSID(HMODULE* phmod, const CLSID *pclsid, FARPROC* ppfn, LPCSTR pszProc)
{
    if (*phmod == NULL) {
        //
        //  SHPinDLLOfCLSID does all the annoying work of opening the
        //  appropriate registry key, doing REG_EXPAND_SZ, etc.
        //  It also loads the DLL with exactly the same name that OLE does,
        //  which is important because NT4 SP3 didn't like it when you loaded
        //  a DLL sometimes via SFN and sometimes via LFN.  (It would
        //  think they were different DLLs, and two copies of it got loaded
        //  into memory.  Aigh!)
        //
        *phmod = (HMODULE)SHPinDllOfCLSID(pclsid);
        if (!*phmod) 
            return;
    }

    // We don't know the name of the DLL, but fortunately _GetProcFromDLL
    // doesn't need it if *phmod is already filled in.
    ASSERT(*phmod);
    _GetProcFromDLL(phmod, "", ppfn, pszProc);
}

//
//  Private exports by ordinal for browseui.  loads from the browseui in apppath dir
//

#ifndef UNIX

#define DELAY_LOAD_IE_ORD_ERR(_hmod, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromCLSID(&_hmod, &CLSID_##_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#else

#define DELAY_LOAD_IE_ORD_ERR(_hmod, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromCLSID(&_hmod, &CLSID_##_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)#_fn);   \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#endif
    
#ifndef UNIX

#define DELAY_LOAD_IE_ORD_VOID(_hmod, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromCLSID(&_hmod, &CLSID_##_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord); \
    if (_pfn##_fn)              \
        _pfn##_fn _nargs;       \
    return;                     \
}

#else

#define DELAY_LOAD_IE_ORD_VOID(_hmod, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromCLSID(&_hmod, &CLSID_##_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)#_fn); \
    if (_pfn##_fn)              \
        _pfn##_fn _nargs;       \
    return;                     \
}

#endif

#else // BrowseUI is in the System directory

#define DELAY_LOAD_IE_ORD_ERR       DELAY_LOAD_ORD_ERR
#define DELAY_LOAD_IE_ORD_VOID      DELAY_LOAD_ORD_VOID

#endif

#define DELAY_LOAD_IE(_hmod, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_IE_ORD_ERR(_hmod, _dll, _ret, _fn, _ord, _args, _nargs, 0)
#define DELAY_LOAD_IE_HRESULT(_hmod, _dll, _fn, _ord, _args, _nargs) DELAY_LOAD_IE_ORD_ERR(_hmod, _dll, HRESULT, _fn, _ord, _args, _nargs, E_FAIL)
#define DELAY_LOAD_IE_BOOL(_hmod, _dll, _fn, _ord, _args, _nargs) DELAY_LOAD_IE_ORD_ERR(_hmod, _dll, BOOL, _fn, _ord, _args, _nargs, FALSE)

#define DELAY_LOAD_IE_ORD(_hmod, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_IE_ORD_ERR(_hmod, _dll, _ret, _fn, _ord, _args, _nargs, 0)

#ifndef NO_LOADING_OF_SHDOCVW_ONLY_FOR_WHICHPLATFORM

/*----------------------------------------------------------
Purpose: Performs a loadlibrary on the DLL only if the machine
     has the integrated shell installation.

*/
void _SHGetProcFromDLL(HINSTANCE* phinst, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
    if (PLATFORM_INTEGRATED == WhichPlatform())
        _GetProcFromSystemDLL(phinst, pszDLL, ppfn, pszProc);
    else
        TraceMsg(TF_ERROR, "Could not load integrated shell version of %s for %d", pszDLL, pszProc);
}

#endif // NO_LOADING_OF_SHDOCVW_ONLY_FOR_WHICHPLATFORM

//
//  Private exports by ordinal for integrated-shell installs
//


#define DELAY_LOAD_SHELL_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _SHGetProcFromDLL(&_hinst, #_dll ".DLL", (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#define DELAY_LOAD_SHELL(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_SHELL_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)
#define DELAY_LOAD_SHELL_HRESULT(_hinst, _dll, _fn, _ord, _args, _nargs ) DELAY_LOAD_SHELL_ERR(_hinst, _dll, HRESULT, _fn, _ord, _args, _nargs, E_FAIL )


#define DELAY_LOAD_SHELL_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
    _SHGetProcFromDLL(&_hinst, #_dll ".DLL", (FARPROC*)&_pfn##_fn, (LPCSTR)_ord); \
    if (_pfn##_fn)              \
        _pfn##_fn _nargs;       \
    return;                     \
}

// Following Macros are functionally  same as above only that they are
// using function name on UNIX rather than ordinals. The above macros 
// are left untouched because other dlls like shdocvw/shdoc401 still use
// them.

#ifndef UNIX

#define DELAY_LOAD_SHELL_ERR_FN(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err, _realfn) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _SHGetProcFromDLL(&_hinst, #_dll ".DLL", (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#else

#define DELAY_LOAD_SHELL_ERR_FN(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err, _realfn) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _SHGetProcFromDLL(&_hinst, #_dll ".DLL", (FARPROC*)&_pfn##_fn, (LPCSTR)#_realfn);   \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#endif

#define DELAY_LOAD_SHELL_FN(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _realfn) DELAY_LOAD_SHELL_ERR_FN(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0, _realfn)
#define DELAY_LOAD_SHELL_HRESULT_FN(_hinst, _dll, _fn, _ord, _args, _nargs, realfn) DELAY_LOAD_SHELL_ERR_FN(_hinst, _dll, HRESULT, _fn, _ord, _args, _nargs, E_FAIL, _realfn)


#ifndef UNIX

#define DELAY_LOAD_SHELL_VOID_FN(_hinst, _dll, _fn, _ord, _args, _nargs, _realfn) \
void __stdcall _fn _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
    _SHGetProcFromDLL(&_hinst, #_dll ".DLL", (FARPROC*)&_pfn##_fn, (LPCSTR)_ord); \
    if (_pfn##_fn)              \
        _pfn##_fn _nargs;       \
    return;                     \
}

#else

#define DELAY_LOAD_SHELL_VOID_FN(_hinst, _dll, _fn, _ord, _args, _nargs, _realfn) \
void __stdcall _fn _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
    _SHGetProcFromDLL(&_hinst, #_dll ".DLL", (FARPROC*)&_pfn##_fn, (LPCSTR)#_realfn); \
    if (_pfn##_fn)              \
        _pfn##_fn _nargs;       \
    return;                     \
}

#endif
