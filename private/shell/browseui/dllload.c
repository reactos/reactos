#include "priv.h"

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
    _GetProcFromDLL(&g_hinstCOMDLG32, "COMDLG32.DLL",  "GetOpenFileNameA", &pfnGetoptnFileNameA);
    if (pfnGetOpenFileNameA)
    return pfnGetOpenFileNameA(pof);
    return -1;
}
#endif

BOOL _EnsureLoaded(HINSTANCE* phinst, LPCSTR pszDLL)
{
    if (*phinst == NULL) {
#ifdef DEBUG
        if (g_dwDumpFlags & DF_DELAYLOADDLL)
        {
            TraceMsg(TF_ALWAYS, "DLLLOAD: Loading %s for the first time",
                     pszDLL);
        }

        if (g_dwBreakFlags & 0x00000080)
        {
            DebugBreak();
        }
#endif
        *phinst = LoadLibraryA(pszDLL);
        if (*phinst == NULL) {
            return FALSE;
        }
    }
    return TRUE;
}

/**********************************************************************/

void _GetProcFromDLL(HINSTANCE* phinst, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
#ifdef DEBUG
    CHAR szProcD[MAX_PATH];
    if (HIWORD(pszProc)) {
    StrCpyNA(szProcD, pszProc, ARRAYSIZE(szProcD));
    } else {
    wnsprintfA(szProcD, ARRAYSIZE(szProcD), "(ordinal %d)", LOWORD(pszProc));
    }
#endif
    // If it's already loaded, return.
    if (*ppfn)
        return;

    if (!_EnsureLoaded(phinst, pszDLL))
        return;

#ifdef DEBUG
    if (g_dwDumpFlags & DF_DELAYLOADDLL) 
    {
        TraceMsg(TF_ALWAYS, "DLLLOAD: GetProc'ing %s from %s for the first time",
         pszDLL, szProcD);
    }
#endif
    *ppfn = GetProcAddress(*phinst, pszProc);

#ifdef DEBUG
    AssertMsg((WhichPlatform() == PLATFORM_BROWSERONLY) || (INT_PTR)*ppfn, TEXT("GetProcAddress failed on %hs"), szProcD);
#endif
}

/*----------------------------------------------------------
Purpose: Performs a loadlibrary on the DLL only if the machine
     has the integrated shell installation.

*/
void _SHGetProcFromDLL(HINSTANCE* phinst, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
    if (PLATFORM_INTEGRATED == WhichPlatform())
    {
    _GetProcFromDLL(phinst, pszDLL, ppfn, pszProc);
    }
    else
    {
    TraceMsg(TF_ERROR, "Could not load integrated shell version of %s for %d", pszDLL, pszProc);
    }
}

#define DELAY_LOAD_MAP(_hinst, _dll, _ret, _fnpriv, _fn, _args, _nargs, _err) \
_ret __stdcall _fnpriv _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, #_fn); \
    if (_pfn##_fn)               \
    return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}

#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err)    DELAY_LOAD_MAP(_hinst, _dll, _ret, _fn, _fn, _args, _nargs, _err)

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_DWORD(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, DWORD, _fn, _args, _nargs, 0)
#define DELAY_LOAD_UINT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, UINT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_INT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, INT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_WNET(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, DWORD, _fn, _args, _nargs, WN_NOT_SUPPORTED)

#define DELAY_MAP_DWORD(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, DWORD, _fnpriv, _fn, _args, _nargs, 0)

#define DELAY_LOAD_VOID(_hinst, _dll, _fn, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
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
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)               \
    return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}
    
#define DELAY_LOAD_ORD(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)


#define DELAY_LOAD_ORD_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
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
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
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
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
    _SHGetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord); \
    if (_pfn##_fn)              \
    _pfn##_fn _nargs;       \
    return;                     \
}



/**********************************************************************/
/**********************************************************************/



// --------- SHELL32.DLL ---------------


HINSTANCE g_hinstOLEAUT32 = NULL;

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, RegisterTypeLib,
    (ITypeLib *ptlib, OLECHAR *szFullPath, OLECHAR *szHelpDir),
    (ptlib, szFullPath, szHelpDir));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, LoadTypeLib,
    (const OLECHAR *szFile, ITypeLib **pptlib), (szFile, pptlib));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SetErrorInfo,
   (unsigned long dwReserved, IErrorInfo*perrinfo), (dwReserved, perrinfo));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, LoadRegTypeLib,
    (REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid, ITypeLib **pptlib),
    (rguid, wVerMajor, wVerMinor, lcid, pptlib));

#undef VariantClear
#undef VariantCopy

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantClear,
    (VARIANTARG *pvarg), (pvarg));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantCopy,
    (VARIANTARG *pvargDest, VARIANTARG *pvargSrc), (pvargDest, pvargSrc));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantCopyInd,
    (VARIANT * pvarDest, VARIANTARG * pvargSrc), (pvarDest, pvargSrc));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantChangeType,
    (VARIANTARG *pvargDest, VARIANTARG *pvarSrc, unsigned short wFlags, VARTYPE vt),
    (pvargDest, pvarSrc, wFlags, vt));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32.DLL, BSTR, SysAllocStringLen,
    (const OLECHAR*pch, unsigned int i), (pch, i));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32.DLL, BSTR, SysAllocString,
    (const OLECHAR*pch), (pch));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32.DLL, BSTR, SysAllocStringByteLen,
     (LPCSTR psz, UINT i), (psz, i));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SysStringByteLen,
     (BSTR bstr), (bstr));

DELAY_LOAD_VOID(g_hinstOLEAUT32, OLEAUT32.DLL, SysFreeString, (BSTR bs), (bs));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, DispGetIDsOfNames,
    (ITypeInfo*ptinfo, OLECHAR **rgszNames, UINT cNames, DISPID*rgdispid),
    (ptinfo, rgszNames, cNames, rgdispid));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, CreateErrorInfo,
    (ICreateErrorInfo **pperrinfo), (pperrinfo));

DELAY_LOAD_SAFEARRAY(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayCreateVector,
    (VARTYPE vt, long iBound, ULONG cElements), (vt, iBound, cElements) );

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayAccessData,
    (SAFEARRAY * psa, void HUGEP** ppvData), (psa, ppvData));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayUnaccessData,
    (SAFEARRAY * psa), (psa) );

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayGetElemsize,
    (SAFEARRAY * psa), (psa) );

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayGetUBound,
    (SAFEARRAY * psa, UINT nDim, LONG * plUBound),
    (psa,nDim,plUBound));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayGetElement,
    (SAFEARRAY * psa, LONG * rgIndices, void * pv), (psa, rgIndices, pv));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayGetDim,
    (SAFEARRAY * psa), (psa));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SysStringLen,
    (BSTR bstr), (bstr));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayDestroy,
    (SAFEARRAY * psa), (psa));

DELAY_LOAD_INT(g_hinstOLEAUT32, OLEAUT32.DLL, DosDateTimeToVariantTime,
    (USHORT wDosDate, USHORT wDosTime, DOUBLE * pvtime), (wDosDate, wDosTime, pvtime));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VarI4FromStr,
    (OLECHAR FAR * strIn, LCID lcid, DWORD dwFlags, LONG * plOut), (strIn, lcid, dwFlags, plOut));


// --------- MPR.DLL ---------------

HINSTANCE g_hinstMPR = NULL;

DELAY_LOAD_WNET(g_hinstMPR, MPR.DLL, WNetGetLastErrorA,
       ( LPDWORD lpError, LPSTR lpErrorBuf, DWORD  nErrorBufSize, LPSTR lpNameBuf, DWORD nNameBufSize),
       (lpError,   lpErrorBuf,    nErrorBufSize,    lpNameBuf,      nNameBufSize  ));
DELAY_LOAD_WNET(g_hinstMPR, MPR.DLL, WNetDisconnectDialog,
       ( HWND  hwnd, DWORD dwType), (hwnd, dwType));
DELAY_LOAD_WNET(g_hinstMPR, MPR.DLL, WNetCloseEnum, (HANDLE hEnum), (hEnum));
DELAY_LOAD_WNET(g_hinstMPR, MPR.DLL, WNetEnumResourceA,
       (HANDLE  hEnum, LPDWORD lpcCount, LPVOID  lpBuffer, LPDWORD lpBufferSize),
       (hEnum, lpcCount, lpBuffer, lpBufferSize));

DELAY_LOAD_WNET(g_hinstMPR, MPR.DLL, WNetEnumResourceW,
       (HANDLE  hEnum, LPDWORD lpcCount, LPVOID  lpBuffer, LPDWORD lpBufferSize),
       (hEnum, lpcCount, lpBuffer, lpBufferSize));

DELAY_LOAD_WNET(g_hinstMPR, MPR.DLL, WNetOpenEnumA,
       (DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEA lpNetResource, LPHANDLE lphEnum),
       (dwScope, dwType, dwUsage, lpNetResource, lphEnum));

DELAY_LOAD_WNET(g_hinstMPR, MPR.DLL, WNetOpenEnumW,
       (DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEW lpNetResource, LPHANDLE lphEnum),
       (dwScope, dwType, dwUsage, lpNetResource, lphEnum));

DELAY_LOAD_WNET(g_hinstMPR, MPR.DLL, WNetGetLastErrorW,
       (LPDWORD lpError, LPWSTR lpErrorBuf, DWORD nErrorBufSize, LPWSTR lpNameBuf, DWORD nNameBufSize),
       (lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize));

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


// --------- SHELL32.DLL ---------------
//
// ----  delay load post win95 shell32 private functions
//

HINSTANCE g_hinstShell32 = NULL;

DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, BOOL, _WriteCabinetState, 652,
       (LPCABINETSTATE lpState), (lpState));
DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, BOOL, __ReadCabinetState, 654,
       (LPCABINETSTATE lpState, int cLength), (lpState, cLength));
DELAY_LOAD_ORD(g_hinstShell32, shell32.dll, BOOL, __OldReadCabinetState, 651,
       (LPCABINETSTATE lpState, int cLength), (lpState, cLength));
BOOL _ReadCabinetState(LPCABINETSTATE lpState, int iSize)
{
    if (!g_fRunningOnNT && WhichPlatform() == PLATFORM_BROWSERONLY)
    {
        // We at least need decent defaults for this case...
        lpState->cLength = sizeof(*lpState);
        lpState->fSimpleDefault            = TRUE;
        lpState->fFullPathTitle            = FALSE;
        lpState->fSaveLocalView            = TRUE;
        lpState->fNotShell                 = FALSE;
        lpState->fNewWindowMode            = FALSE; // can't simulate this one, use FALSE
        lpState->fShowCompColor            = FALSE;
        lpState->fDontPrettyNames          = FALSE;
        lpState->fAdminsCreateCommonGroups = TRUE;
        lpState->fUnusedFlags              = 0;
        lpState->fMenuEnumFilter           = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

        // Lie and say we read from the registry,
        // this avoids us calling WriteCabinetState
        return TRUE;
    }
    else
    {
        // this leaves two cases
        // if we're integrated then we use ordinal 654
        // otherwise we're on NT4 browseronly and need
        // to use the old ordinal, 651.

        if (WhichPlatform() == PLATFORM_INTEGRATED)
        {
            return __ReadCabinetState(lpState, iSize);
        }
        else
        {
            return __OldReadCabinetState(lpState, iSize);
        }
    }
}

DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, LPSHChangeNotificationLock, _SHChangeNotification_Lock, 644,
           (HANDLE hChangeNotification, DWORD dwProcessId, LPITEMIDLIST **pppidl, LONG *plEvent),
           (hChangeNotification, dwProcessId, pppidl,  plEvent));
DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, BOOL, _SHChangeNotification_Unlock, 645,
           (LPSHChangeNotificationLock pshcnl), (pshcnl));


// 653 WINSHELLAPI LONG WINAPI PathProcessCommand( LPCTSTR lpSrc, LPTSTR lpDest, int iMax, DWORD dwFlags );
DELAY_LOAD_SHELL_ERR(g_hinstShell32, shell32.dll, LONG, _PathProcessCommand, 653,
           (LPCTSTR lpSrc, LPTSTR lpDest, int iMax, DWORD dwFlags), (lpSrc, lpDest, iMax, dwFlags), -1);
// SHStringFromGUIDA->shlwapi
// 3 SHSTDAPI  SHDefExtractIconA(LPCSTR pszIconFile, int iIndex, UINT uFlags, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize);
//             imported as DL_SHDefExtractIconA
DELAY_LOAD_SHELL_HRESULT(g_hinstShell32, shell32.dll, DL_SHDefExtractIconA, 3,
           (LPCSTR pszIconFile, int iIndex, UINT uFlags, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize), 
           (pszIconFile, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize));
// 6 SHSTDAPI  SHDefExtractIconW(LPWCSTR pszIconFile, int iIndex, UINT uFlags, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize);
//             imported as DL_SHDefExtractIconW

// 7 SHSTDAPI_(int)  SHLookupIconIndexA(LPCSTR pszFile, int iIconIndex, UINT uFlags);
DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, int, _SHLookupIconIndexA, 7,
           (LPCSTR pszFile, int iIconIndex, UINT uFlags), 
           (pszFile, iIconIndex, uFlags));

// 8 SHSTDAPI_(int)  SHLookupIconIndexW(LPCWSTR pszFile, int iIconIndex, UINT uFlags);
DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, int, _SHLookupIconIndexW, 8,
           (LPCWSTR pszFile, int iIconIndex, UINT uFlags), 
           (pszFile, iIconIndex, uFlags));

// 12 SHSTDAPI SHStartNetConnectionDialogA(HWND hwnd, LPCSTR pszRemoteName, DWORD dwType);
DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, HRESULT, SHStartNetConnectionDialogA, 12,
           (HWND hwnd, LPCSTR pszRemoteName, DWORD dwType), 
           (hwnd, pszRemoteName, dwType));

// 14 SHSTDAPI SHStartNetConnectionDialogW(HWND hwnd, LPCWSTR pszRemoteName, DWORD dwType);
DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, HRESULT, SHStartNetConnectionDialogW, 14,
           (HWND hwnd, LPCWSTR pszRemoteName, DWORD dwType),
           (hwnd, pszRemoteName, dwType));

// VOID WINAPI CheckWinIniForAssocs();
DELAY_LOAD_SHELL_VOID(g_hinstShell32, shell32.dll, CheckWinIniForAssocs, 711, (), ());

//   Delay load NT (Unicode) Shell APIs

#undef ShellExecuteExW
DELAY_LOAD_MAP(g_hinstShell32, shell32.DLL, BOOL,
              DL_ShellExecuteExW, ShellExecuteExW,
              (LPSHELLEXECUTEINFOW pseiW),
              (pseiW), FALSE);

#undef SHGetPathFromIDListW
DELAY_LOAD_MAP(g_hinstShell32, shell32.DLL, BOOL,
              DL_SHGetPathFromIDListW, SHGetPathFromIDListW,
              (LPCITEMIDLIST pidl, LPWSTR pszPath),
              (pidl, pszPath), FALSE);

#undef SHBrowseForFolderW
DELAY_LOAD_MAP(g_hinstShell32, shell32.DLL, LPITEMIDLIST,
              DL_SHBrowseForFolderW, SHBrowseForFolderW,
              (LPBROWSEINFOW pbiW),
              (pbiW), NULL);

#undef ExtractIconExW
DELAY_LOAD_MAP(g_hinstShell32, shell32.DLL, UINT,
              DL_ExtractIconExW, ExtractIconExW,
              (LPCWSTR lpszFile, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons),
              (lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons), 0);

#undef SHFileOperationW
DELAY_LOAD_MAP(g_hinstShell32, shell32.DLL, int,
              DL_SHFileOperationW, SHFileOperationW,
              (LPSHFILEOPSTRUCTW lpfo),
              (lpfo), ERROR_CALL_NOT_IMPLEMENTED);

// SHGetNewLinkInfo (undecorated) was exported NONAME in Win95, but for 
// some reason made public in NT4.  Implicitly linking to this API will
// fail on Win95's shell because the loader looks by name since we link
// with the NT version of shell32.lib.  So we must delay load this API
// by ordinal.

// 179  BOOL SHGetNewLinkInfoA(LPCSTR pszLinkTo, LPCSTR pszDir, LPSTR pszName, BOOL * pfMustCopy, UINT uFlags);
DELAY_LOAD_ORD(g_hinstShell32, shell32.dll, BOOL, SHGetNewLinkInfoA, 179,
           (LPCSTR pszLinkTo, LPCSTR pszDir, LPSTR pszName, BOOL * pfMustCopy, UINT uFlags), 
           (pszLinkTo, pszDir, pszName, pfMustCopy, uFlags) );

//
//  These functions are new for the NT5 shell and therefore must be
//  wrapped specially.
//
// 22 STDAPI_(BOOL) DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject)

DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, BOOL, __DAD_DragEnterEx2, 22,
           (HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject),
           (hwndTarget, ptStart, pdtObject));

STDAPI_(BOOL) DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject)
{
    if (GetUIVersion() >= 5)
        return __DAD_DragEnterEx2(hwndTarget, ptStart, pdtObject);
    else
        return DAD_DragEnterEx(hwndTarget, ptStart);
}

// 187 BOOL WINAPI ILGetPseudoNameW(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlBase, LPWSTR pwzName, int fType);
DELAY_LOAD_SHELL(g_hinstShell32, shell32.dll, BOOL, __ILGetPseudoNameW, 187,
    (LPCITEMIDLIST pidl, LPCITEMIDLIST pidlSpec, WCHAR *pszBuf, int fType),
    (pidl, pidlSpec, pszBuf, fType));

STDAPI_(BOOL) ILGetPseudoNameW(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlSpec, WCHAR *pszBuf, int fType)
{
    if (GetUIVersion() >= 5)
        return __ILGetPseudoNameW(pidl, pidlSpec, pszBuf, fType);
    else
    {
        pszBuf[0] = 0;     // Most people don't check the return code
        return FALSE;
    }
}


// --------- MSJAVA.DLL ---------------
HINSTANCE g_hinstMSJAVA = NULL;
DELAY_LOAD_VOID(g_hinstMSJAVA, MSJAVA.DLL, ShowJavaConsole, (), () );


// --------- COMDLG32.DLL ---------------

HINSTANCE g_hinstCOMDLG32 = NULL;

#ifdef UNICODE
DELAY_LOAD(g_hinstCOMDLG32, COMDLG32.DLL, BOOL, GetOpenFileNameW, (LPOPENFILENAME pof), (pof));
DELAY_LOAD(g_hinstCOMDLG32, COMDLG32.DLL, BOOL, GetSaveFileNameW, (LPOPENFILENAME pof), (pof));
#else
DELAY_LOAD(g_hinstCOMDLG32, COMDLG32.DLL, BOOL, GetOpenFileNameA, (LPOPENFILENAME pof), (pof));
DELAY_LOAD(g_hinstCOMDLG32, COMDLG32.DLL, BOOL, GetSaveFileNameA, (LPOPENFILENAME pof), (pof));
#endif

// --------- WININET.DLL ---------------

// BUGBUG: Remember to put following in PRIV.H right before #include <urlmon.h>
//
// #define _WINX32_  // get DECLSPEC_IMPORT stuff right for WININET API
// #define _URLCACHEAPI  // get DECLSPEC_IMPORT stuff right for WININET CACHE API
//
HINSTANCE g_hinstWININET = NULL;


DELAY_LOAD(g_hinstWININET, WININET.DLL, BOOL, GetUrlCacheEntryInfoExW,
   (IN LPCWSTR lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
    OUT LPWSTR     lpszRedirectUrl,
    IN OUT LPDWORD lpcbRedirectUrl,
    LPVOID         lpReserved,
    DWORD          dwReserved
   ),
    (lpszUrl, lpCacheEntryInfo, lpdwCacheEntryInfoBufSize,
     lpszRedirectUrl, lpcbRedirectUrl, lpReserved, dwReserved));

DELAY_LOAD(g_hinstWININET, WININET.DLL, BOOL, GetUrlCacheEntryInfoExA,
   (IN LPCSTR lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
    OUT LPSTR      lpszRedirectUrl,
    IN OUT LPDWORD lpcbRedirectUrl,
    LPVOID         lpReserved,
    DWORD          dwReserved
   ),
    (lpszUrl, lpCacheEntryInfo, lpdwCacheEntryInfoBufSize,
     lpszRedirectUrl, lpcbRedirectUrl, lpReserved, dwReserved));



#ifdef UNICODE

DELAY_LOAD(g_hinstWININET, WININET.DLL, BOOL, InternetCrackUrlW,
    (IN LPCWSTR lpszUrl,
     IN DWORD dwUrlLength,
     IN DWORD dwFlags,
     IN OUT LPURL_COMPONENTSW lpUrlComponents),
    (lpszUrl, dwUrlLength, dwFlags, lpUrlComponents));

#else

DELAY_LOAD(g_hinstWININET, WININET.DLL, BOOL, InternetCrackUrlA,
    (IN LPCSTR lpszUrl,
     IN DWORD dwUrlLength,
     IN DWORD dwFlags,
     IN OUT LPURL_COMPONENTSA lpUrlComponents),
    (lpszUrl, dwUrlLength, dwFlags, lpUrlComponents));

#endif // !UNICODE

DELAY_LOAD(g_hinstWININET, WININET.DLL, BOOL, InternetSetOptionA,
    (HINTERNET hinternet, DWORD dwOptions, LPVOID lpBuffer, DWORD dwBufferLength),
    (hinternet, dwOptions, lpBuffer, dwBufferLength));

DELAY_LOAD(g_hinstWININET, WININET.DLL, DWORD, InternetConfirmZoneCrossingA,
     (HWND hWnd, LPSTR szUrlPrev, LPSTR szUrlNew, BOOL bPost),
     (hWnd, szUrlPrev, szUrlNew, bPost));

DELAY_LOAD(g_hinstWININET, WININET.DLL, DWORD, InternetConfirmZoneCrossingW,
     (HWND hWnd, LPWSTR pwzUrlPrev, LPWSTR pwzUrlNew, BOOL bPost),
     (hWnd, pwzUrlPrev, pwzUrlNew, bPost));

DELAY_LOAD(g_hinstWININET, WININET.DLL, BOOL, RegisterUrlCacheNotification,
     (IN  HWND        hWnd,
      IN  UINT        uMsg,
      IN  GROUPID     gid,
      IN  DWORD       dwOpsFilter,
      IN  DWORD       dwReserved),
     (hWnd, uMsg, gid, dwOpsFilter, dwReserved));

DELAY_LOAD(g_hinstWININET, WININET.DLL, BOOL, InternetGoOnline,
    (LPTSTR lpszURL, HWND hwndParent, DWORD    dwReserved),
    (lpszURL, hwndParent, dwReserved))


#ifdef UNIX

DELAY_LOAD_VOID(g_hinstWININET, WININET.DLL, unixGetWininetCacheLockStatus,
            (BOOL *pBoolReadOnly, char **ppszLockingHost),
            (pBoolReadOnly, ppszLockingHost))

DELAY_LOAD_VOID(g_hinstWININET, WININET.DLL, unixCleanupWininetCacheLockFile,
            (),
            ())
#endif

DELAY_LOAD(g_hinstWININET, WININET.DLL, BOOL, InternetFortezzaCommand,
           (DWORD dwCommand, HWND hwnd, DWORD_PTR dwReserved),
           (dwCommand, hwnd, dwReserved));

DELAY_LOAD(g_hinstWININET, WININET.DLL, BOOL, InternetQueryFortezzaStatus,
           (DWORD *pdwStatus, DWORD_PTR dwReserved),
           (pdwStatus, dwReserved));


// --------- URLMON.DLL ---------------

HINSTANCE g_hinstURLMON = NULL;

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON.DLL, URLDownloadToCacheFile,
     (LPUNKNOWN pCaller, LPCTSTR szURL, LPTSTR szFileName, DWORD fCache, DWORD dwResv, LPBINDSTATUSCALLBACK lpfnCB),
     (pCaller, szURL, szFileName, fCache, dwResv, lpfnCB));

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON.DLL, CoInternetQueryInfo,
    (LPCWSTR pwzUrl, QUERYOPTION QueryOptions, DWORD dwQueryFlags, LPVOID pvBuffer, DWORD cbBuffer, DWORD *pcbBuffer, DWORD dwReserved),
    (pwzUrl, QueryOptions, dwQueryFlags, pvBuffer, cbBuffer, pcbBuffer, dwReserved));

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON.DLL, CreateURLMoniker,
    (IMoniker* pMkCtx, LPCWSTR pwsURL, IMoniker ** ppimk), (pMkCtx, pwsURL, ppimk));

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON.DLL, CoInternetParseUrl,
    (LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwFlags, LPWSTR pszResult,
     DWORD cchResult, DWORD *pcchResult, DWORD dwReserved ),
    (pwzUrl, ParseAction, dwFlags, pszResult, cchResult, pcchResult, dwReserved));

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, FaultInIEFeature,
    (HWND hWnd, uCLSSPEC *pClassSpec, QUERYCONTEXT *pQuery, DWORD dwFlags),
    (hWnd, pClassSpec, pQuery, dwFlags));


// --------- IMM32.DLL ---------------
HINSTANCE g_hinstImm32 = NULL;

DELAY_LOAD(g_hinstImm32, IMM32.DLL, UINT, ImmGetVirtualKey,
            (HWND hWnd), (hWnd));


// --------- MSHTML.DLL ---------------
HINSTANCE g_hinstMSHTML = NULL;

DELAY_LOAD_HRESULT(g_hinstMSHTML, MSHTML.DLL, ShowHTMLDialog,
            (HWND hwndParent, IMoniker *pmk, VARIANT *pvarArgIn, WCHAR* pchOptions, VARIANT *pvArgOut),
            (hwndParent, pmk, pvarArgIn, pchOptions, pvArgOut));
#ifdef UNIX
DELAY_LOAD_HRESULT(g_hinstMSHTML, MSHTML.DLL, TranslateModelessAccelerator,
            (MSG* msg, HWND hwnd),
            (msg, hwnd));
#endif

// --------- SHDOCVW.DLL ---------------
HINSTANCE g_hinstShdocvw = NULL;

DELAY_LOAD_ORD(g_hinstShdocvw, shdocvw.dll, LPNMVIEWFOLDER, _DDECreatePostNotify, 116,
       (LPNMVIEWFOLDER pnm), (pnm));

DELAY_LOAD_ORD(g_hinstShdocvw, shdocvw.dll, BOOL, _DDEHandleViewFolderNotify, 117,
       (IShellBrowser* psb, HWND hwnd, LPNMVIEWFOLDER lpnm), (psb, hwnd, lpnm));

// BUGBUG REMOVE approx dec 98
DELAY_LOAD_MAP(g_hinstShdocvw, shdocvw.dll, BOOL,
                SHDOCVW_DllRegisterWindowClasses, DllRegisterWindowClasses,
                (const SHDRC * pshdrc),
                (pshdrc), 0);

// HRESULT ResetWebSettings(HWND hwnd, BOOL *pfHomePageChanged)
DELAY_LOAD_ORD(g_hinstShdocvw, shdocvw.dll, HRESULT, ResetWebSettings, 223,
              (HWND hwnd, BOOL *pfHomePageChanged),
              (hwnd, pfHomePageChanged));

// BOOL IsResetWebSettingsRequired(void)
DELAY_LOAD_ORD(g_hinstShdocvw, shdocvw.dll, BOOL, IsResetWebSettingsRequired, 224,
              (void),
              ());

// HRESULT PrepareURLForDisplayUTF8W(LPCWSTR pwz, LPWSTR pwzOut, LPDWORD pcbOut, BOOL fUTF8Enabled)
DELAY_LOAD_ORD(g_hinstShdocvw, shdocvw.dll, HRESULT, PrepareURLForDisplayUTF8W, 225,
              (LPCWSTR pwz, LPWSTR pwzOut, LPDWORD pcbOut, BOOL fUTF8Enabled),
              (pwz, pwzOut, pcbOut, fUTF8Enabled));

// --------- VERSION.DLL ---------------
HINSTANCE g_hinstVERSION = NULL;
#undef VerQueryValueW
#undef GetFileVersionInfoW
#undef GetFileVersionInfoSizeW

DELAY_LOAD(g_hinstVERSION, VERSION, BOOL, VerQueryValueA,
                (const void *pBlock, LPSTR lpSubBlock, void **ppBuffer, PUINT puLen),
                (pBlock, lpSubBlock, ppBuffer, puLen))

DELAY_LOAD(g_hinstVERSION, VERSION, BOOL, VerQueryValueW,
                (const void *pBlock, LPWSTR lpSubBlock, void **ppBuffer, PUINT puLen),
                (pBlock, lpSubBlock, ppBuffer, puLen))

DELAY_LOAD(g_hinstVERSION, VERSION, BOOL, GetFileVersionInfoA,
            (LPSTR lpstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData),
            (lpstrFilename, dwHandle, dwLen, lpData));

DELAY_LOAD(g_hinstVERSION, VERSION, BOOL, GetFileVersionInfoW,
            (LPWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData),
            (lptstrFilename, dwHandle, dwLen, lpData));

DELAY_LOAD(g_hinstVERSION, VERSION, DWORD, GetFileVersionInfoSizeA,
                (LPSTR pszFilename,  LPDWORD lpdwHandle),
                (pszFilename, lpdwHandle))

DELAY_LOAD(g_hinstVERSION, VERSION, DWORD, GetFileVersionInfoSizeW,
                (LPTSTR pszFilename,  LPDWORD lpdwHandle),
                (pszFilename, lpdwHandle))

#ifdef UNIX_FEATURE_ALIAS
HINSTANCE g_hinstInetcpl = NULL;
DELAY_LOAD(g_hinstInetcpl, inetcpl.cpl, DWORD, GetURLForAliasA,
                (HDPA aliasList, LPSTR alias,  LPSTR szurl, int cch),
                (aliasList, alias, szurl, cch))
#endif /* UNIX_FEATURE_ALIAS */
