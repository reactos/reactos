#include "priv.h"

#pragma warning(disable:4273)  // Dont' whine when we private-define a dllexport

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


#define ENSURE_LOADED(_hinst, _dll, pszfn)         (_hinst ? TRUE : ((_hinst = LoadLibrary(TEXT(#_dll))) != NULL))

/**********************************************************************/

void _GetProcFromDLL(HINSTANCE* phinst, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
    // If it's already loaded, return.
    if (*ppfn) {
        return;
    }

    if (*phinst == NULL) {
        *phinst = LoadLibraryA(pszDLL);
        if (*phinst == NULL) {
            return;
        }
    }

    *ppfn = GetProcAddress(*phinst, pszProc);
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

#define DELAY_LOAD_MAP_VOID(_hinst, _dll, _fnpriv, _fn, _args, _nargs) \
void __stdcall _fnpriv _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, #_fn); \
    if (_pfn##_fn)              \
    _pfn##_fn _nargs; \
    return;           \
}



// NOTE: this takes two parameters that are the function name. the First (_fn) is the name that
// NOTE: the function will be called in the context of shell32.dll and the other (_fni) is the 
// NOTE: name of the function we will GetProcAddress. This helps get around functions that
// NOTE: are defined in the header files with _declspec...
#define DELAY_LOAD_NAME_ERR(_hinst, _dll, _ret, _fnpriv, _fn, _args, _nargs, _err) \
_ret __stdcall _fnpriv _args                \
{                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, #_fn); \
    if (_pfn##_fn)               \
    return _pfn##_fn _nargs; \
    return (_ret)_err;           \
}
#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err)    DELAY_LOAD_NAME_ERR(_hinst, _dll, _ret, _fn, _fn, _args, _nargs, _err)

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_UINT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, UINT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_INT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, INT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_BOOL(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, BOOL, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_BOOLEAN(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, BOOLEAN, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_DWORD(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, DWORD, _fn, _args, _nargs, FALSE)
#define DELAY_LOAD_LPVOID(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, LPVOID, _fn, _args, _nargs, 0)

// the NAME variants allow the local function to be called something different from the imported
// function to avoid dll linkage problems.
#define DELAY_LOAD_NAME(_hinst, _dll, _ret, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hinst, _dll, _ret, _fn, _fni, _args, _nargs, 0)
#define DELAY_LOAD_NAME_HRESULT(_hinst, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hinst, _dll, HRESULT, _fn, _fni, _args, _nargs, E_FAIL)
#define DELAY_LOAD_NAME_SAFEARRAY(_hinst, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hinst, _dll, SAFEARRAY *, _fn, _fni, _args, _nargs, NULL)
#define DELAY_LOAD_NAME_UINT(_hinst, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hinst, _dll, UINT, _fn, _fni, _args, _nargs, 0)
#define DELAY_LOAD_NAME_BOOL(_hinst, _dll, _fn, _fni, _args, _nargs) DELAY_LOAD_NAME_ERR(_hinst, _dll, BOOL, _fn, _fni, _args, _nargs, FALSE)

#define DELAY_MAP_DWORD(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, DWORD, _fnpriv, _fn, _args, _nargs, 0)
#define DELAY_MAP_UINT(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, UINT, _fnpriv, _fn, _args, _nargs, 0)
#define DELAY_MAP_INT(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, INT, _fnpriv, _fn, _args, _nargs, 0)
#define DELAY_MAP_BOOL(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, BOOL, _fnpriv, _fn, _args, _nargs, 0)
#define DELAY_MAP_LPITEMIDLIST(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, LPITEMIDLIST, _fnpriv, _fn, _args, _nargs, 0)
#define DELAY_MAP_HRESULT(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, HRESULT, _fnpriv, _fn, _args, _nargs, 0)
#define DELAY_MAP_HICON(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, HICON, _fnpriv, _fn, _args, _nargs, 0)
#define DELAY_MAP_DWPTR(_hinst, _dll, _fnpriv, _fn, _args, _nargs) DELAY_LOAD_MAP(_hinst, _dll, DWORD_PTR, _fnpriv, _fn, _args, _nargs, 0)


#define DELAY_LOAD_NAME_VOID(_hinst, _dll, _fn, _fni, _args, _nargs)                               \
void __stdcall _fn _args                                                                \
{                                                                                       \
    static void (__stdcall *_pfn##_fni) _args = NULL;                                   \
    if (!ENSURE_LOADED(_hinst, _dll, TEXT(#_fni)))                                       \
    {                                                                                   \
        AssertMsg(_hinst != NULL, TEXT("LoadLibrary failed on ") ## TEXT(#_dll));         \
        return;                                                                         \
    }                                                                                   \
    if (_pfn##_fni == NULL)                                                              \
    {                                                                                   \
        *(FARPROC*)&(_pfn##_fni) = GetProcAddress(_hinst, #_fni);                         \
        AssertMsg(_pfn##_fni != NULL, TEXT("GetProcAddress failed on ") ## TEXT(#_fni));    \
        if (_pfn##_fni == NULL)                                                          \
            return;                                                                     \
    }                                                                                   \
    _pfn##_fni _nargs;                                                                   \
}

#define DELAY_LOAD_VOID(_hinst, _dll, _fn, _args, _nargs)   DELAY_LOAD_NAME_VOID(_hinst, _dll, _fn, _fn, _args, _nargs)



// For private entrypoints exported by ordinal.
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


#define DELAY_LOAD_VOID_ORD(_hinst, _dll, _fn, _ord, _args, _nargs)                     \
void __stdcall _fn _args                \
{                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;   \
    _GetProcFromDLL(&_hinst, #_dll, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord);   \
    if (_pfn##_fn)              \
    _pfn##_fn _nargs;       \
    return;                     \
}

// -----------ole32.dll---------------
HINSTANCE g_hinstOLE32 = NULL;

DELAY_LOAD_HRESULT(g_hinstOLE32, OLE32.DLL, CoCreateInstance,
    (REFCLSID rclsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, void ** ppv), (rclsid, pUnkOuter, dwClsContext, riid, ppv));

DELAY_LOAD_HRESULT(g_hinstOLE32, OLE32.DLL, CoInitialize, (void *pv), (pv));
DELAY_LOAD_HRESULT(g_hinstOLE32, OLE32.DLL, CoInitializeEx,
                   (void *pv, DWORD dw), (pv, dw));
DELAY_LOAD_VOID(g_hinstOLE32, OLE32.DLL, CoUninitialize, (void), ());

DELAY_LOAD_HRESULT(g_hinstOLE32, OLE32.DLL, CoGetClassObject,
    (REFCLSID rclsid, DWORD dwClsContext, void *pvReserved, REFIID riid, void **ppv),
    (rclsid, dwClsContext, pvReserved, riid, ppv));

DELAY_LOAD_VOID(g_hinstOLE32, OLE32.DLL, ReleaseStgMedium,
                   (LPSTGMEDIUM pmedium), (pmedium));

DELAY_LOAD_HRESULT(g_hinstOLE32, OLE32.DLL, GetHGlobalFromStream,
    (IStream *pstm, HGLOBAL * phglobal),
    (pstm, phglobal));

DELAY_LOAD_HRESULT(g_hinstOLE32, OLE32.DLL, CreateStreamOnHGlobal,
    (HGLOBAL hGlobal, BOOL fDeleteOnRelease, IStream ** ppstm),
    (hGlobal, fDeleteOnRelease, ppstm));

DELAY_LOAD_HRESULT(g_hinstOLE32, OLE32.DLL, OleLoadFromStream,
    (IStream *pStm, REFIID iidInterface, void ** ppvObj),
    (pStm, iidInterface, ppvObj));

DELAY_LOAD_HRESULT(g_hinstOLE32, OLE32.DLL, OleSaveToStream,
    (LPPERSISTSTREAM pPStm, IStream *pStm),
    (pPStm, pStm));

DELAY_LOAD_LPVOID(g_hinstOLE32, OLE32.DLL, CoTaskMemAlloc, (SIZE_T cb), (cb));
DELAY_LOAD_VOID(g_hinstOLE32, OLE32.DLL, CoTaskMemFree, (void *pv), (pv));

#undef CLSIDFromProgID
DELAY_MAP_HRESULT(g_hinstOLE32, OLE32.DLL, _CLSIDFromProgID, CLSIDFromProgID, 
                   (LPCOLESTR lpszProgID, LPCLSID lpclsid),
                   (lpszProgID, lpclsid));


// --------- MLANG.DLL ---------------

HINSTANCE g_hinstMLANG = NULL;

DELAY_LOAD_HRESULT(g_hinstMLANG, MLANG.DLL, ConvertINetMultiByteToUnicode,
            (LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount),
            (lpdwMode, dwEncoding, lpSrcStr, lpnMultiCharCount, lpDstStr, lpnWideCharCount));

DELAY_LOAD_HRESULT(g_hinstMLANG, MLANG.DLL, ConvertINetUnicodeToMultiByte,
            (LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount),
            (lpdwMode, dwEncoding, lpSrcStr, lpnWideCharCount, lpDstStr, lpnMultiCharCount));

// --------- COMCTL32.DLL ---------------

HINSTANCE g_hinstCOMCTL32 = NULL;

DELAY_LOAD_ORD(g_hinstCOMCTL32, COMCTL32.DLL, BOOL, SetWindowSubclass, 410,
            (HWND hwnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR dwRefData),
            (hwnd, pfnSubclass, uIdSubclass, dwRefData));

DELAY_LOAD_ORD(g_hinstCOMCTL32, COMCTL32.DLL, BOOL, GetWindowSubclass, 411,
            (HWND hwnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR *pdwRefData),
            (hwnd, pfnSubclass, uIdSubclass, pdwRefData));

DELAY_LOAD_ORD(g_hinstCOMCTL32, COMCTL32.DLL, BOOL, RemoveWindowSubclass, 412,
            (HWND hwnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass),
            (hwnd, pfnSubclass, uIdSubclass));

DELAY_LOAD_ORD(g_hinstCOMCTL32, COMCTL32.DLL, LRESULT, DefSubclassProc, 413,
            (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam),
            (hwnd, uMsg, wParam, lParam));

DELAY_LOAD_ORD(g_hinstCOMCTL32, COMCTL32.DLL, HDPA, DPA_Create, 328, (int cItemGrow), (cItemGrow));
DELAY_LOAD_ORD(g_hinstCOMCTL32, COMCTL32.DLL, BOOL, DPA_Destroy, 329, (HDPA hdpa), (hdpa));
DELAY_LOAD_ORD(g_hinstCOMCTL32, COMCTL32.DLL, int, DPA_InsertPtr, 334, (HDPA hdpa, int i, void *p), (hdpa, i, p));
DELAY_LOAD_ORD(g_hinstCOMCTL32, COMCTL32.DLL, LPVOID, DPA_DeletePtr, 336, (HDPA hdpa, int i), (hdpa, i));
DELAY_LOAD_ORD(g_hinstCOMCTL32, COMCTL32.DLL, BOOL, DPA_DeleteAllPtrs, 337, (HDPA hdpa), (hdpa));

DELAY_LOAD(g_hinstCOMCTL32, COMCTL32.DLL, INT_PTR, PropertySheetA, (LPCPROPSHEETHEADERA ppsh), (ppsh));
DELAY_LOAD(g_hinstCOMCTL32, COMCTL32.DLL, INT_PTR, PropertySheetW, (LPCPROPSHEETHEADERW ppsh), (ppsh));

// --------- shell32.dll ---------------

HINSTANCE g_hinstSHELL32 = NULL;

#undef SHGetFileInfoW
DELAY_MAP_DWPTR(g_hinstSHELL32, SHELL32.DLL, _SHGetFileInfoW, SHGetFileInfoW,
            (LPCWSTR pwzPath, DWORD dwFileAttributes, SHFILEINFOW *psfi, UINT cbFileInfo, UINT uFlags),
            (pwzPath, dwFileAttributes, psfi, cbFileInfo, uFlags));

#undef SHGetFileInfoA
DELAY_MAP_DWPTR(g_hinstSHELL32, SHELL32.DLL, _SHGetFileInfoA, SHGetFileInfoA,
            (LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA *psfi, UINT cbFileInfo, UINT uFlags),
            (pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags));

#undef DragQueryFileW
DELAY_MAP_UINT(g_hinstSHELL32, SHELL32.DLL, _DragQueryFileW, DragQueryFileW,
            (HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch),
            (hDrop, iFile, lpszFile, cch));

#undef DragQueryFileA
DELAY_MAP_UINT(g_hinstSHELL32, SHELL32.DLL, _DragQueryFileA, DragQueryFileA,
            (HDROP hDrop, UINT iFile, LPSTR lpszFile, UINT cch),
            (hDrop, iFile, lpszFile, cch));

#undef SHBrowseForFolderW
DELAY_MAP_LPITEMIDLIST(g_hinstSHELL32, SHELL32.DLL, _SHBrowseForFolderW, SHBrowseForFolderW,
            (LPBROWSEINFOW pbiW),
            (pbiW));

#undef SHBrowseForFolderA
DELAY_MAP_LPITEMIDLIST(g_hinstSHELL32, SHELL32.DLL, _SHBrowseForFolderA, SHBrowseForFolderA,
            (LPBROWSEINFOA pbiA),
            (pbiA));

#undef SHGetPathFromIDListW
DELAY_MAP_BOOL(g_hinstSHELL32, SHELL32.DLL, _SHGetPathFromIDListW, SHGetPathFromIDListW,
            (LPCITEMIDLIST pidl, LPWSTR pwzPath),
            (pidl, pwzPath));

#undef SHGetPathFromIDListA
DELAY_MAP_BOOL(g_hinstSHELL32, SHELL32.DLL, _SHGetPathFromIDListA, SHGetPathFromIDListA,
            (LPCITEMIDLIST pidl, LPSTR pszPath),
            (pidl, pszPath));

#undef ShellExecuteExW
DELAY_MAP_BOOL(g_hinstSHELL32, SHELL32.DLL, _ShellExecuteExW, ShellExecuteExW,
            (LPSHELLEXECUTEINFOW pExecInfoW),
            (pExecInfoW));

#undef ShellExecuteExA
DELAY_MAP_BOOL(g_hinstSHELL32, SHELL32.DLL, _ShellExecuteExA, ShellExecuteExA,
            (LPSHELLEXECUTEINFOA pExecInfoA),
            (pExecInfoA));

#undef SHFileOperationW
DELAY_MAP_INT(g_hinstSHELL32, SHELL32.DLL, _SHFileOperationW, SHFileOperationW,
            (LPSHFILEOPSTRUCTW pFileOpW),
            (pFileOpW));

#undef SHFileOperationA
DELAY_MAP_INT(g_hinstSHELL32, SHELL32.DLL, _SHFileOperationA, SHFileOperationA,
            (LPSHFILEOPSTRUCTA pFileOpA),
            (pFileOpA));

#undef ExtractIconExW
DELAY_MAP_UINT(g_hinstSHELL32, SHELL32.DLL, _ExtractIconExW, ExtractIconExW,
            (LPCWSTR pwzFile, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons),
            (pwzFile, nIconIndex, phiconLarge, phiconSmall, nIcons));

#undef ExtractIconExA
DELAY_MAP_UINT(g_hinstSHELL32, SHELL32.DLL, _ExtractIconExA, ExtractIconExA,
            (LPCSTR pszFile, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons),
            (pszFile, nIconIndex, phiconLarge, phiconSmall, nIcons));

#undef SHFormatDrive
DELAY_MAP_UINT(g_hinstSHELL32, SHELL32.DLL, _SHFormatDrive, SHFormatDrive,
            (HWND hwnd, UINT drive, UINT fmtID, UINT options),
            (hwnd, drive, fmtID, options));

#define IsNetDriveORD           66
#undef IsNetDrive
DELAY_LOAD_ORD_ERR(g_hinstSHELL32, SHELL32.DLL, INT, _IsNetDrive, IsNetDriveORD,
            (int iDrive),
            (iDrive), 0);

#undef DriveType
DELAY_MAP_INT(g_hinstSHELL32, SHELL32.DLL, _DriveType, DriveType,
            (int iDrive),
            (iDrive));

#undef RealDriveType
DELAY_MAP_INT(g_hinstSHELL32, SHELL32.DLL, _RealDriveType, RealDriveType,
            (int iDrive, BOOL fOKToHitNet),
            (iDrive, fOKToHitNet));


#undef SHGetNewLinkInfoW
DELAY_MAP_BOOL(g_hinstSHELL32, SHELL32.DLL, _SHGetNewLinkInfoW, SHGetNewLinkInfoW,
            (LPCWSTR pszpdlLinkTo, LPCWSTR pszDir, LPWSTR pszName, BOOL *pfMustCopy, UINT uFlags),
            (pszpdlLinkTo, pszDir, pszName, pfMustCopy, uFlags));

#undef SHGetNewLinkInfoA
DELAY_LOAD_ORD(g_hinstSHELL32, SHELL32.DLL, BOOL, _SHGetNewLinkInfoA, 179,
            (LPCSTR pszpdlLinkTo, LPCSTR pszDir, LPSTR pszName, BOOL *pfMustCopy, UINT uFlags),
            (pszpdlLinkTo, pszDir, pszName, pfMustCopy, uFlags));

#undef SHDefExtractIconW
DELAY_MAP_HRESULT(g_hinstSHELL32, SHELL32.DLL, _SHDefExtractIconW, SHDefExtractIconW,
            (LPCWSTR pszFile, int nIconIndex, UINT  uFlags, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize),
            (pszFile, nIconIndex, uFlags, phiconLarge, phiconSmall, nIconSize));

#undef SHDefExtractIconA
DELAY_MAP_HRESULT(g_hinstSHELL32, SHELL32.DLL, _SHDefExtractIconA, SHDefExtractIconA,
            (LPCSTR pszFile, int nIconIndex, UINT  uFlags, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize),
            (pszFile, nIconIndex, uFlags, phiconLarge, phiconSmall, nIconSize));

#undef ExtractIconA
DELAY_MAP_HICON(g_hinstSHELL32, SHELL32.DLL, _ExtractIconA, ExtractIconA,
            (HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex),
            (hInst, lpszExeFileName, nIconIndex));

#undef ExtractIconW
DELAY_MAP_HICON(g_hinstSHELL32, SHELL32.DLL, _ExtractIconW, ExtractIconW,
            (HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex),
            (hInst, lpszExeFileName, nIconIndex));

DELAY_LOAD_MAP_VOID(g_hinstSHELL32, SHELL32.DLL, _SHChangeNotify, SHChangeNotify,
                    (LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2),
                    (wEventId, uFlags, dwItem1, dwItem2));


DELAY_LOAD_MAP_VOID(g_hinstSHELL32, SHELL32.DLL, _SHFlushSFCache, SHFlushSFCache, (), ());

DELAY_MAP_HRESULT(g_hinstSHELL32, SHELL32.DLL, _SHGetInstanceExplorer, SHGetInstanceExplorer, (IUnknown **ppunk), (ppunk));

DELAY_LOAD_ORD(g_hinstSHELL32, SHELL32.DLL, int, _Shell_GetCachedImageIndexW, 72,
               (LPVOID pszIconPath, int iIconIndex, UINT uIconFlags),
               (pszIconPath, iIconIndex, uIconFlags));

// --------- winmm.dll ---------------

HINSTANCE g_hinstWINMM = NULL;

#undef PlaySoundA
DELAY_MAP_BOOL(g_hinstWINMM, WINMM.DLL, _PlaySoundA, PlaySoundA,
            (LPCSTR pszSound, HMODULE hMod, DWORD fFlags),
            (pszSound, hMod, fFlags));

#undef PlaySoundW
DELAY_MAP_BOOL(g_hinstWINMM, WINMM.DLL, _PlaySoundW, PlaySoundW,
            (LPCWSTR pszSound, HMODULE hMod, DWORD fFlags),
            (pszSound, hMod, fFlags));

// --------- mpr.dll ---------------

HINSTANCE g_hinstMPR = NULL;

#undef WNetGetResourceInformationA
DELAY_LOAD_DWORD(g_hinstMPR, MPR.DLL, WNetGetResourceInformationA,
                 (LPNETRESOURCEA pNetResourceA, void *lpBuffer, LPDWORD lpcbBuffer, LPSTR* ppszSystem),
                 (pNetResourceA, lpBuffer, lpcbBuffer, ppszSystem));

#undef WNetGetResourceInformationW
DELAY_LOAD_DWORD(g_hinstMPR, MPR.DLL, WNetGetResourceInformationW,
                 (LPNETRESOURCEW pNetResourceW, void *pBuffer, LPDWORD pcbBuffer, LPWSTR* ppszSystem),
                 (pNetResourceW, pBuffer, pcbBuffer, ppszSystem));

#undef WNetRestoreConnectionA
DELAY_LOAD_DWORD(g_hinstMPR, MPR.DLL, WNetRestoreConnectionA,
                 (HWND hwndParent, LPCSTR pszDevice),
                 (hwndParent, pszDevice));

#undef WNetRestoreConnectionW
DELAY_LOAD_DWORD(g_hinstMPR, MPR.DLL, WNetRestoreConnectionW,
                 (HWND hwndParent, LPCWSTR pwzDevice),
                 (hwndParent, pwzDevice));

#undef WNetGetLastErrorA
DELAY_LOAD_DWORD(g_hinstMPR, MPR.DLL, WNetGetLastErrorA,
                 (LPDWORD pdwError, OUT LPSTR pszErrorBuf, DWORD cchErrorBufSize, LPSTR pszNameBuf, DWORD cchNameBufSize),
                 (pdwError, pszErrorBuf, cchErrorBufSize, pszNameBuf, cchNameBufSize));

#undef WNetGetLastErrorW
DELAY_LOAD_DWORD(g_hinstMPR, MPR.DLL, WNetGetLastErrorW,
                 (LPDWORD pdwError, LPWSTR pwzErrorBuf, DWORD cchErrorBufSize, LPWSTR pwzNameBuf, DWORD cchNameBufSize),
                 (pdwError, pwzErrorBuf, cchErrorBufSize, pwzNameBuf, cchNameBufSize));


// --------- version.dll ------------
HINSTANCE g_hinstVERSION = NULL;


#undef GetFileVersionInfoA
DELAY_LOAD_BOOL(g_hinstVERSION, VERSION.DLL, GetFileVersionInfoA,
                 (LPSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, void *lpData),
                 (lptstrFilename, dwHandle, dwLen, lpData));
                 
#undef GetFileVersionInfoW
DELAY_LOAD_BOOL(g_hinstVERSION, VERSION.DLL, GetFileVersionInfoW,
                 (LPWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, void *lpData),
                 (lptstrFilename, dwHandle, dwLen, lpData));

#undef VerQueryValueA
DELAY_LOAD_BOOL(g_hinstVERSION, VERSION.DLL, VerQueryValueA,
                 (const void *pBlock, LPSTR lpSubBlock, void **ppBuffer, PUINT puLen),
                 (pBlock, lpSubBlock, ppBuffer, puLen));

#undef VerQueryValueW
DELAY_LOAD_BOOL(g_hinstVERSION, VERSION.DLL, VerQueryValueW,
                 (const void *pBlock, LPWSTR lpSubBlock, void **ppBuffer, PUINT puLen),
                 (pBlock, lpSubBlock, ppBuffer, puLen));

#undef GetFileVersionInfoSizeA
DELAY_LOAD_DWORD(g_hinstVERSION, VERSION.DLL, GetFileVersionInfoSizeA,
                 (LPSTR lptstrFilename, LPDWORD lpdwHandle),
                 (lptstrFilename, lpdwHandle));

#undef GetFileVersionInfoSizeW
DELAY_LOAD_DWORD(g_hinstVERSION, VERSION.DLL, GetFileVersionInfoSizeW,
                 (LPWSTR lptstrFilename, LPDWORD lpdwHandle),
                 (lptstrFilename, lpdwHandle));

// --------- MSI.DLL ----------------
HMODULE g_hmodMSI = NULL;

#undef GetProductInfoA
DELAY_LOAD_UINT(g_hmodMSI, MSI.DLL, MsiGetProductInfoA,
                (LPCSTR szProduct, LPCSTR szAttribute, LPSTR lpValueBuf, DWORD *pcchValueBuf),
                (szProduct, szAttribute, lpValueBuf, pcchValueBuf));

#undef GetProductInfoW
DELAY_LOAD_UINT(g_hmodMSI, MSI.DLL, MsiGetProductInfoW,
                (LPCWSTR szProduct, LPCWSTR szAttribute, LPWSTR lpValueBuf, DWORD *pcchValueBuf),
                (szProduct, szAttribute, lpValueBuf, pcchValueBuf));


// --------- COMDLG32.DLL ----------------
HMODULE g_hinstCOMDLG32 = NULL;

#undef GetSaveFileNameA
DELAY_MAP_BOOL(g_hinstCOMDLG32, COMDLG32.DLL, _GetSaveFileNameA,
               GetSaveFileNameA, (LPOPENFILENAMEA lpofn), (lpofn));

#undef GetSaveFileNameW
DELAY_MAP_BOOL(g_hinstCOMDLG32, COMDLG32.DLL, _GetSaveFileNameW,
               GetSaveFileNameW, (LPOPENFILENAMEW lpofn), (lpofn));

#undef GetOpenFileNameA
DELAY_MAP_BOOL(g_hinstCOMDLG32, COMDLG32.DLL, _GetOpenFileNameA,
               GetOpenFileNameA, (LPOPENFILENAMEA lpofn), (lpofn));

#undef GetOpenFileNameW
DELAY_MAP_BOOL(g_hinstCOMDLG32, COMDLG32.DLL, _GetOpenFileNameW,
               GetOpenFileNameW, (LPOPENFILENAMEW lpofn), (lpofn));

#undef PrintDlgA
DELAY_MAP_BOOL(g_hinstCOMDLG32, COMDLG32.DLL, _PrintDlgA,
               PrintDlgA, (LPPRINTDLGA lppd), (lppd));

#undef PrintDlgW
DELAY_MAP_BOOL(g_hinstCOMDLG32, COMDLG32.DLL, _PrintDlgW,
               PrintDlgW, (LPPRINTDLGW lppd), (lppd));

#undef PageSetupDlgA
DELAY_MAP_BOOL(g_hinstCOMDLG32, COMDLG32.DLL, _PageSetupDlgA,
               PageSetupDlgA, (LPPAGESETUPDLGA lppsd), (lppsd));

#undef PageSetupDlgW
DELAY_MAP_BOOL(g_hinstCOMDLG32, COMDLG32.DLL, _PageSetupDlgW,
               PageSetupDlgW, (LPPAGESETUPDLGW lppsd), (lppsd));

// --------- USERENV.DLL ----------------
HMODULE g_hmodUserEnv = NULL;

#undef ExpandEnvironmentStringsForUserW

DELAY_LOAD_BOOL(g_hmodUserEnv, USERENV, ExpandEnvironmentStringsForUserW,
                (HANDLE hToken, LPCWSTR lpSrc, LPWSTR lpDest, DWORD dwSize),
                (hToken, lpSrc, lpDest, dwSize));

#ifdef TRY_NtPowerInformation
// --------- NTDLL.DLL ----------------
HMODULE g_hmodNTDLL = NULL;

DELAY_LOAD_HRESULT(g_hmodNTDLL, NTDLL.DLL, NtPowerInformation,
            (POWER_INFORMATION_LEVEL level, PVOID In, ULONG cbIn, PVOID Out, ULONG cbOut),
            (level, In, cbIn, Out, cbOut));

#endif // TRY_NtPowerInformation

// --------- SETUPAPI.DLL ----------------
HMODULE g_hinstSETUPAPI = NULL;

DELAY_LOAD_BOOL(g_hinstSETUPAPI, SETUPAPI.DLL, SetupDiDestroyDeviceInfoList,
                 (IN HDEVINFO DeviceInfoSet),
                 (DeviceInfoSet));

DELAY_LOAD_BOOL(g_hinstSETUPAPI, SETUPAPI.DLL, SetupDiGetDeviceInterfaceDetailA,
  ( IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
  ), (                                     DeviceInfoSet,
                                           DeviceInterfaceData,
                                           DeviceInterfaceDetailData,
                                           DeviceInterfaceDetailDataSize,
                                           RequiredSize,
                                           DeviceInfoData));

DELAY_LOAD_BOOL(g_hinstSETUPAPI, SETUPAPI.DLL, SetupDiEnumDeviceInterfaces,
  ( IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVINFO_DATA           DeviceInfoData,     OPTIONAL
    IN  CONST GUID                *InterfaceClassGuid,
    IN  DWORD                      MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData
  ), (                             DeviceInfoSet,
                                   DeviceInfoData,
                                   InterfaceClassGuid,
                                   MemberIndex,
                                   DeviceInterfaceData));

DELAY_LOAD_ERR(g_hinstSETUPAPI, SETUPAPI.DLL, HANDLE, SetupDiGetClassDevsA,
  ( IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCSTR       Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
  ), (ClassGuid, Enumerator, hwndParent, Flags), INVALID_HANDLE_VALUE);

void FreeDynamicLibraries()
{
    if (g_hinstOLE32)
        FreeLibrary(g_hinstOLE32);
    if (g_hinstMLANG)
        FreeLibrary(g_hinstMLANG);
    if (g_hinstCOMCTL32)
        FreeLibrary(g_hinstCOMCTL32);
    if (g_hinstSHELL32)
        FreeLibrary(g_hinstSHELL32);
    if (g_hinstWINMM)
        FreeLibrary(g_hinstWINMM);
    if (g_hinstMPR)
        FreeLibrary(g_hinstMPR);
    if (g_hinstVERSION)
        FreeLibrary(g_hinstVERSION);
}
