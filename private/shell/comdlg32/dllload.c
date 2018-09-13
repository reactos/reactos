#define _OLE32_         // for DECLSPEC_IMPORT delay load

#include <windows.h>
#include <winnetwk.h>
#include <winnetp.h>

void _GetProcFromDLL(HMODULE* phinst, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc)
{
    // If it's already loaded, return.
    if (*ppfn)
        return;

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

#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err)    DELAY_LOAD_MAP(_hinst, _dll, _ret, _fn, _fn, _args, _nargs, _err)

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_DWORD(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, DWORD, _fn, _args, _nargs, 0)
#define DELAY_LOAD_BOOL(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, BOOL, _fn, _args, _nargs, 0)
#define DELAY_LOAD_UINT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, UINT, _fn, _args, _nargs, 0)

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

// --------- MPR.DLL ---------------

HMODULE g_hmodMPR = NULL;

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetConnectionDialog, (HWND  hwnd, DWORD dwType), (hwnd, dwType));

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetCloseEnum, (HANDLE hEnum), (hEnum));

#ifdef UNICODE

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetEnumResourceW,
       (HANDLE  hEnum, LPDWORD lpcCount, LPVOID  lpBuffer, LPDWORD lpBufferSize),
       (hEnum, lpcCount, lpBuffer, lpBufferSize));

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetOpenEnumW,
       (DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEW lpNetResource, LPHANDLE lphEnum),
       (dwScope, dwType, dwUsage, lpNetResource, lphEnum));

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetFormatNetworkNameW,
    (LPCWSTR lpProvider, LPCWSTR lpRemoteName, LPWSTR lpFormattedName,
     LPDWORD lpnLength, DWORD dwFlags, DWORD dwAveCharPerLine),
    (lpProvider, lpRemoteName, lpFormattedName, lpnLength,  dwFlags, dwAveCharPerLine));

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetRestoreConnectionW,
       (HWND hwnd, LPCWSTR psz),
       (hwnd, psz));

#else

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetEnumResourceA,
       (HANDLE  hEnum, LPDWORD lpcCount, LPVOID  lpBuffer, LPDWORD lpBufferSize),
       (hEnum, lpcCount, lpBuffer, lpBufferSize));

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetOpenEnumA,
       (DWORD dwScope, DWORD dwType, DWORD dwUsage, LPNETRESOURCEA lpNetResource, LPHANDLE lphEnum),
       (dwScope, dwType, dwUsage, lpNetResource, lphEnum));

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetFormatNetworkNameA,
    (LPCSTR lpProvider, LPCSTR lpRemoteName, LPSTR lpFormattedName,
     LPDWORD lpnLength, DWORD dwFlags, DWORD dwAveCharPerLine),
    (lpProvider, lpRemoteName, lpFormattedName, lpnLength,  dwFlags, dwAveCharPerLine));

DELAY_LOAD_WNET(g_hmodMPR, MPR.DLL, WNetRestoreConnectionA,
       (HWND hwnd, LPCSTR psz),
       (hwnd, psz));

#endif


// -----------ole32.dll---------------

HMODULE g_hmodOLE32 = NULL;

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32.DLL, CoCreateInstance,
    (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, void ** ppv), (rclsid, pUnkOuter, dwClsContext, riid, ppv));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32.DLL, CoInitialize, (void *pv), (pv));
DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32.DLL, CoInitializeEx, (void *pv, DWORD dw), (pv, dw));
DELAY_LOAD_VOID(g_hmodOLE32, OLE32.DLL, CoUninitialize, (void), ());

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32.DLL, OleInitialize, (void *pv), (pv));
DELAY_LOAD_VOID(g_hmodOLE32, OLE32.DLL, OleUninitialize, (void), ());

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32.DLL, GetClassFile,
    (LPCOLESTR szFilename, CLSID FAR* pclsid),
    (szFilename, pclsid));

DELAY_LOAD_HRESULT(g_hmodOLE32, OLE32.DLL, CreateBindCtx,
    (DWORD reserved, LPBC FAR* ppbc),
    (reserved, ppbc));
    
// -----------version.dll-------------------
HMODULE g_hmodVersion = NULL;

#ifdef UNICODE

DELAY_LOAD_BOOL(g_hmodVersion, version, GetFileVersionInfoW,
                (LPTSTR pszFilename, DWORD dwHandle, DWORD dwLen, void *lpData),
                (pszFilename, dwHandle, dwLen, lpData))

DELAY_LOAD(g_hmodVersion, version, DWORD, GetFileVersionInfoSizeW,
                (LPTSTR pszFilename,  LPDWORD lpdwHandle),
                (pszFilename, lpdwHandle))
 
DELAY_LOAD_BOOL(g_hmodVersion, version, VerQueryValueW,
                (const void *pBlock, LPTSTR lpSubBlock, void **ppBuffer, PUINT puLen),
                (pBlock, lpSubBlock, ppBuffer, puLen))

DELAY_LOAD_DWORD(g_hmodVersion, version, VerLanguageNameW,
                (DWORD wLang, LPTSTR szLang, DWORD nSize),
                (wLang, szLang, nSize))

DELAY_LOAD_BOOL(g_hmodVersion, version, VerQueryValueIndexW,
                (const void *pBlock, LPTSTR lpSubBlock, DWORD dwIndex, void **ppBuffer, void **ppValue, PUINT puLen),
                (pBlock, lpSubBlock, dwIndex, ppBuffer, ppValue, puLen))
#else

DELAY_LOAD_BOOL(g_hmodVersion, version, GetFileVersionInfoA,
                (LPTSTR pszFilename, DWORD dwHandle, DWORD dwLen, void *lpData),
                (pszFilename, dwHandle, dwLen, lpData))

DELAY_LOAD(g_hmodVersion, version, DWORD, GetFileVersionInfoSizeA,
                (LPTSTR pszFilename,  LPDWORD lpdwHandle),
                (pszFilename, lpdwHandle))
 
DELAY_LOAD_BOOL(g_hmodVersion, version, VerQueryValueA,
                (const void *pBlock, LPTSTR lpSubBlock, void **ppBuffer, PUINT puLen),
                (pBlock, lpSubBlock, ppBuffer, puLen))

DELAY_LOAD_DWORD(g_hmodVersion, version, VerLanguageNameA,
                (DWORD wLang, LPTSTR szLang, DWORD nSize),
                (wLang, szLang, nSize))
#endif

// -----------imm32.dll---------------

HMODULE g_hmodIMM32 = NULL;

DELAY_LOAD(g_hmodIMM32, IMM32.DLL, HIMC, ImmGetContext, (HWND hWnd), (hWnd))
 
DELAY_LOAD(g_hmodIMM32, IMM32.DLL, LONG, ImmGetCompositionString,
                (HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen),
                (hIMC, dwIndex, lpBuf, dwBufLen))

DELAY_LOAD_BOOL(g_hmodIMM32, IMM32.DLL, ImmReleaseContext, (HWND hWnd, HIMC hIMC), (hWnd, hIMC))
