#include "priv.h"

#include "uemapp.h"
// fake out mimeole.h's dll linkage directives
#define _MIMEOLE_
#include <mimeole.h>

// BugBug: ../lib/dllload.c uses an unsafe buffer function

#undef  wsprintfA

#include "../lib/dllload.c"

#define wsprintfA  Do_not_use_wsprintfA_use_wnsprintfA

// IEUNIX : On UNIX we cannot load functions by ordinals. The following macros
// are redefined ( copied ) to take an extra function name param. On windows
// we still use ordinal numbers but on unix we will use the new paramater as the
// function name.

#undef  DELAY_LOAD_SHELL 
#undef  DELAY_LOAD_SHELL_ERR 
#undef  DELAY_LOAD_SHELL_VOID 
#undef  DELAY_LOAD_SHELL_HRESULT

#define DELAY_LOAD_SHELL          DELAY_LOAD_SHELL_FN
#define DELAY_LOAD_SHELL_ERR      DELAY_LOAD_SHELL_ERR_FN
#define DELAY_LOAD_SHELL_VOID     DELAY_LOAD_SHELL_VOID_FN
#define DELAY_LOAD_SHELL_HRESULT  DELAY_LOAD_SHELL_HRESULT_FN


/**********************************************************************/
/**********************************************************************/



// --------- SHELL32.DLL ---------------

//
// ----  delay load post win95 shell32 private functions
//

HINSTANCE g_hinstShell32 = NULL;

DELAY_LOAD_SHELL(g_hinstShell32, shell32, LPSHChangeNotificationLock, _SHChangeNotification_Lock, 644,
           (HANDLE hChangeNotification, DWORD dwProcessId, LPITEMIDLIST **pppidl, LONG *plEvent),
           (hChangeNotification, dwProcessId, pppidl,  plEvent), SHChangeNotification_Lock)

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, _SHChangeNotification_Unlock, 645,
           (LPSHChangeNotificationLock pshcnl), (pshcnl), SHChangeNotification_Unlock)

// 653 WINSHELLAPI LONG WINAPI PathProcessCommand( LPCTSTR lpSrc, LPTSTR lpDest, int iMax, DWORD dwFlags );
DELAY_LOAD_SHELL_ERR(g_hinstShell32, shell32, LONG, _PathProcessCommand, 653,
               (LPCTSTR lpSrc, LPTSTR lpDest, int iMax, DWORD dwFlags), (lpSrc, lpDest, iMax, dwFlags), -1, PathProcessCommand)
// SHStringFromGUIDA->shlwapi
// 3 SHSTDAPI  SHDefExtractIconA(LPCSTR pszIconFile, int iIndex, UINT uFlags, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize);
DELAY_LOAD_SHELL_HRESULT(g_hinstShell32, shell32, SHDefExtractIconA, 3,
           (LPCSTR pszIconFile, int iIndex, UINT uFlags, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize), 
           (pszIconFile, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize), SHDefExtractIconA)
// 7 SHSTDAPI_(int)  SHLookupIconIndexA(LPCSTR pszFile, int iIconIndex, UINT uFlags);
DELAY_LOAD_SHELL(g_hinstShell32, shell32, int, _SHLookupIconIndexA, 7,
           (LPCSTR pszFile, int iIconIndex, UINT uFlags),  
           (pszFile, iIconIndex, uFlags), SHLookupIconIndexA)

// 8 SHSTDAPI_(int)  SHLookupIconIndexW(LPCWSTR pszFile, int iIconIndex, UINT uFlags);
DELAY_LOAD_SHELL(g_hinstShell32, shell32, int, _SHLookupIconIndexW, 8,
           (LPCWSTR pszFile, int iIconIndex, UINT uFlags),   
           (pszFile, iIconIndex, uFlags), SHLookupIconIndexA)
// 12 SHSTDAPI SHStartNetConnectionDialogA(HWND hwnd, LPCSTR pszRemoteName, DWORD dwType);
DELAY_LOAD_SHELL(g_hinstShell32, shell32, HRESULT, SHStartNetConnectionDialogA, 12,
           (HWND hwnd, LPCSTR pszRemoteName, DWORD dwType), 
           (hwnd, pszRemoteName, dwType), SHStartNetConnectionDialogA)

DELAY_LOAD_NAME_HRESULT(g_hinstShell32, shell32, _SHPathPrepareForWriteW, SHPathPrepareForWriteW,
         (HWND hwnd, IUnknown *punkEnableModless, LPCWSTR pwzPath, DWORD dwFlags),
         (hwnd, punkEnableModless, pwzPath, dwFlags));

//
//  These functions are new for the NT5 shell and therefore must be
//  wrapped specially.
//
// 22 STDAPI_(BOOL) DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject)

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, __DAD_DragEnterEx2, 22,
           (HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject),
           (hwndTarget, ptStart, pdtObject), _DAD_DragEnterEx2);

STDAPI_(BOOL) DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject)
{
    if (GetUIVersion() >= 5)
    {
        // BUGBUG: this should really test the shell32 version, not just the UI setting
        return __DAD_DragEnterEx2(hwndTarget, ptStart, pdtObject);
    }
    else
        return DAD_DragEnterEx(hwndTarget, ptStart);
}

// VOID WINAPI CheckWinIniForAssocs();
DELAY_LOAD_SHELL_VOID(g_hinstShell32, SHELL32, CheckWinIniForAssocs, 711, (), (), CheckWinIniForAssocs);




//---------- BROWSEUI.DLL --------------

//
//--- delay load browseui functions

HINSTANCE g_hinstBrowseui = NULL;


DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, BOOL, SHOpenFolderWindow, 102,
               (IETHREADPARAM* pieiIn),
                (pieiIn))

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, HRESULT,
                SHOpenNewFrame, 103, (LPITEMIDLIST pidlNew, ITravelLog *ptl, DWORD dwBrowserIndex, UINT uFlags),
               (pidlNew, ptl, dwBrowserIndex, uFlags))

DELAY_LOAD_IE_HRESULT(g_hinstBrowseui, BROWSEUI,
                  SHGetSetDefFolderSettings, 107,
                  (DEFFOLDERSETTINGS *pdfs, int cbDfs, UINT flags),
                  (pdfs, cbDfs, flags))

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, IDropTarget* , 
                  DropTargetWrap_CreateInstance, 119,
                  (IDropTarget* pdtPrimary, IDropTarget* pdtSecondary, HWND hwnd, IDropTarget* pdt3),
                  (pdtPrimary, pdtSecondary, hwnd, pdt3));

DELAY_LOAD_IE_ORD_VOID(g_hinstBrowseui, BROWSEUI, SHCreateSavedWindows, 105,
               (),
                ())

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, BOOL,
                  SHOnCWMCommandLine, 127, (LPARAM lParam), (lParam))

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, BOOL,
                  SHCreateFromDesktop, 106, (PNEWFOLDERINFO pfi), (pfi))

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, IETHREADPARAM*,
                  SHCreateIETHREADPARAM, 123,
                  (LPCWSTR pszCmdLineIn, int nCmdShowIn, ITravelLog *ptlIn, IEFreeThreadedHandShake* piehsIn),
                  (pszCmdLineIn, nCmdShowIn, ptlIn, piehsIn))

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, BOOL,
                  SHParseIECommandLine, 125,
                  (LPCWSTR * ppszCmdLine, IETHREADPARAM * piei),
                  (ppszCmdLine, piei))

DELAY_LOAD_IE_ORD_VOID(g_hinstBrowseui, BROWSEUI, SHDestroyIETHREADPARAM, 126,
                       (IETHREADPARAM* piei), (piei))

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, LPITEMIDLIST,
                  Channel_GetFolderPidl, 128, (void), ())

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, IDeskBand *,
                  ChannelBand_Create, 129, (LPCITEMIDLIST pidlDefault),
                  (pidlDefault))

DELAY_LOAD_IE_ORD_VOID(g_hinstBrowseui, BROWSEUI,
                       Channels_SetBandInfoSFB, 130, (IUnknown* punkBand), (punkBand))

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, HRESULT,
                  IUnknown_SetBandInfoSFB, 131,
                  (IUnknown *punkBand, BANDINFOSFB *pbi), (punkBand, pbi))

#ifdef NO_MARSHALLING
DELAY_LOAD_IE_ORD_VOID(g_hinstBrowseui, BROWSEUI, IEFrameNewWindowSameThread, 105,
                       (IETHREADPARAM* piei), (piei))
#endif

// BUGBUG the following two functions are TEMPORARILY exported for 
// the favorites to shdocvw split
DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, HRESULT,
                  SHGetNavigateTarget, 134,
                  (IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl, DWORD *pdwAttribs),
                  (psf, pidl, ppidl, pdwAttribs))


DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, BOOL,
                  GetInfoTip, 135,
                  (IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax),
                  (psf, pidl, pszText, cchTextMax))


// --------- COMDLG32.DLL ---------------


HINSTANCE g_hinstCOMDLG32 = NULL;

DELAY_LOAD(g_hinstCOMDLG32, COMDLG32, BOOL, GetOpenFileNameA, (LPOPENFILENAMEA pof), (pof))
DELAY_LOAD(g_hinstCOMDLG32, COMDLG32, BOOL, GetSaveFileNameA, (LPOPENFILENAMEA pof), (pof))
DELAY_LOAD(g_hinstCOMDLG32, COMDLG32, DWORD, CommDlgExtendedError, (void), ());

// --------- OLEAUT32.DLL ---------------

HINSTANCE g_hinstOLEAUT32 = NULL;

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, RegisterTypeLib,
    (ITypeLib *ptlib, OLECHAR *szFullPath, OLECHAR *szHelpDir),
    (ptlib, szFullPath, szHelpDir))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, LoadTypeLib,
    (const OLECHAR *szFile, ITypeLib **pptlib), (szFile, pptlib))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, SetErrorInfo,
   (unsigned long dwReserved, IErrorInfo*perrinfo), (dwReserved, perrinfo))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, LoadRegTypeLib,
    (REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid, ITypeLib **pptlib),
    (rguid, wVerMajor, wVerMinor, lcid, pptlib))

#undef VariantClear
#undef VariantCopy

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VariantClear,
    (VARIANTARG *pvarg), (pvarg))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VariantCopy,
    (VARIANTARG *pvargDest, VARIANTARG *pvargSrc), (pvargDest, pvargSrc))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VariantCopyInd,
    (VARIANT * pvarDest, VARIANTARG * pvargSrc), (pvarDest, pvargSrc))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VariantChangeType,
    (VARIANTARG *pvargDest, VARIANTARG *pvarSrc, unsigned short wFlags, VARTYPE vt),
    (pvargDest, pvarSrc, wFlags, vt))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VariantChangeTypeEx,
    (VARIANTARG *pvargDest, VARIANTARG *pvarSrc, LCID lcid, unsigned short wFlags, VARTYPE vt),
    (pvargDest, pvarSrc, lcid, wFlags, vt))

DELAY_LOAD_INT(g_hinstOLEAUT32, OLEAUT32, VariantTimeToSystemTime,
    (DOUBLE vtime, LPSYSTEMTIME lpSystemTime),
    (vtime, lpSystemTime))

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32, BSTR, SysAllocStringLen,
    (const OLECHAR*pch, unsigned int i), (pch, i))

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32, BSTR, SysAllocString,
    (const OLECHAR*pch), (pch))

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32, BSTR, SysAllocStringByteLen,
     (LPCSTR psz, UINT i), (psz, i))

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32, SysStringByteLen,
     (BSTR bstr), (bstr))

DELAY_LOAD_VOID(g_hinstOLEAUT32, OLEAUT32, SysFreeString, (BSTR bs), (bs))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, DispGetIDsOfNames,
    (ITypeInfo*ptinfo, OLECHAR **rgszNames, UINT cNames, DISPID*rgdispid),
    (ptinfo, rgszNames, cNames, rgdispid))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, CreateErrorInfo,
    (ICreateErrorInfo **pperrinfo), (pperrinfo))

DELAY_LOAD_SAFEARRAY(g_hinstOLEAUT32, OLEAUT32, SafeArrayCreateVector,
    (VARTYPE vt, long iBound, ULONG cElements), (vt, iBound, cElements) )

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, SafeArrayAccessData,
    (SAFEARRAY * psa, void HUGEP** ppvData), (psa, ppvData))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, SafeArrayUnaccessData,
    (SAFEARRAY * psa), (psa) )

DELAY_LOAD_SAFEARRAY(g_hinstOLEAUT32, OLEAUT32, SafeArrayCreate,
    (VARTYPE vt, UINT cDims, SAFEARRAYBOUND * rgsabound), (vt, cDims, rgsabound));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, SafeArrayPutElement,
     (SAFEARRAY * psa, LONG * rgIndices, void * pv), (psa, rgIndices, pv));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32, SafeArrayGetElemsize,
    (SAFEARRAY * psa), (psa) )

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, SafeArrayGetUBound,
    (SAFEARRAY * psa, UINT nDim, LONG * plUBound),
    (psa,nDim,plUBound))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, SafeArrayGetElement,
    (SAFEARRAY * psa, LONG * rgIndices, void * pv), (psa, rgIndices, pv))

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32, SafeArrayGetDim,
    (SAFEARRAY * psa), (psa))

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32, SysStringLen,
    (BSTR bstr), (bstr))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, SafeArrayDestroy,
    (SAFEARRAY * psa), (psa))

DELAY_LOAD_INT(g_hinstOLEAUT32, OLEAUT32, DosDateTimeToVariantTime,
    (USHORT wDosDate, USHORT wDosTime, DOUBLE * pvtime), (wDosDate, wDosTime, pvtime))

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VarI4FromStr,
    (OLECHAR FAR * strIn, LCID lcid, DWORD dwFlags, LONG * plOut), (strIn, lcid, dwFlags, plOut));


// --------- WININET.DLL ---------------

// BUGBUG: Remember to put following in PRIV.H right before #include <urlmon.h>
//
// #define _WINX32_  // get DECLSPEC_IMPORT stuff right for WININET API
// #define _URLCACHEAPI  // get DECLSPEC_IMPORT stuff right for WININET CACHE API
//

HINSTANCE g_hinstWININET = NULL;

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetCanonicalizeUrlW,
    (LPCWSTR lpszUrl, LPWSTR lpszBuffer, LPDWORD lpdwBufferLength, DWORD dwFlags),
    (lpszUrl, lpszBuffer, lpdwBufferLength, dwFlags))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetCrackUrlW,
    (LPCWSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, LPURL_COMPONENTSW lpUrlComponents),
    (lpszUrl, dwUrlLength, dwFlags, lpUrlComponents))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetCrackUrlA,
    (LPCSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, LPURL_COMPONENTSA lpUrlComponents),
    (lpszUrl, dwUrlLength, dwFlags, lpUrlComponents))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetCombineUrlA,
    (const char* lpszBaseUrl, const char* lpszRelativeUrl, char *lpszBuffer, DWORD *len, DWORD dwFlags ),
    (lpszBaseUrl, lpszRelativeUrl, lpszBuffer, len, dwFlags ))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetCreateUrlA,
    (LPURL_COMPONENTSA lpUrlComponents, DWORD dwFlags, char *p, DWORD *len),
    (lpUrlComponents, dwFlags, p, len ))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetCreateUrlW,
    (LPURL_COMPONENTSW lpUrlComponents, DWORD dwFlags, WCHAR *p, DWORD *len),
    (lpUrlComponents, dwFlags, p, len ))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetGoOnline,
    (LPTSTR lpszURL, HWND hwndParent, DWORD    dwReserved),
    (lpszURL, hwndParent, dwReserved))

DELAY_LOAD(g_hinstWININET, WININET, BOOL , InternetInitializeAutoProxyDll,
    (DWORD dwReserved),
    (dwReserved))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetGetConnectedStateExA,
    (LPDWORD lpdwFlags, LPSTR lpszConnectionName,DWORD dwBufLen, DWORD dwCrap),
    (lpdwFlags, lpszConnectionName, dwBufLen, dwCrap ))
    
DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetSetOptionA,
    (HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength),
    (hInternet, dwOption, lpBuffer, dwBufferLength))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetQueryOptionA,
    (HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, LPDWORD lpdwBufferLength),
    (hInternet, dwOption, lpBuffer, lpdwBufferLength))

DELAY_LOAD(g_hinstWININET, WININET, DWORD, InternetConfirmZoneCrossing,
     (HWND hWnd, LPWSTR szUrlPrev, LPWSTR szUrlNew, BOOL bPost),
     (hWnd, szUrlPrev, szUrlNew, bPost))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, FindCloseUrlCache,
    (HANDLE hEnumHandle),
    (hEnumHandle))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, DeleteUrlCacheEntryW,
    (LPCWSTR lpszUrlName),
    (lpszUrlName))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, DeleteUrlCacheEntryA,
    (LPCSTR lpszUrlName),
    (lpszUrlName))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, FindNextUrlCacheEntryW,
    (HANDLE hEnumHandle, LPINTERNET_CACHE_ENTRY_INFOW lpNextCacheEntryInfo, LPDWORD lpdwNextCacheEntryInfoBufferSize),
    (hEnumHandle, lpNextCacheEntryInfo, lpdwNextCacheEntryInfoBufferSize))
DELAY_LOAD(g_hinstWININET, WININET, BOOL, FindNextUrlCacheEntryA,
    (HANDLE hEnumHandle, LPINTERNET_CACHE_ENTRY_INFOA lpNextCacheEntryInfo, LPDWORD lpdwNextCacheEntryInfoBufferSize),
    (hEnumHandle, lpNextCacheEntryInfo, lpdwNextCacheEntryInfoBufferSize))

DELAY_LOAD(g_hinstWININET, WININET, HANDLE, FindFirstUrlCacheEntryW,
    (LPCWSTR lpszUrlSearchPattern, LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize),
    (lpszUrlSearchPattern, lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize))
DELAY_LOAD(g_hinstWININET, WININET, HANDLE, FindFirstUrlCacheEntryA,
    (LPCSTR lpszUrlSearchPattern, LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize),
    (lpszUrlSearchPattern, lpFirstCacheEntryInfo, lpdwFirstCacheEntryInfoBufferSize))


DELAY_LOAD(g_hinstWININET, WININET, BOOL, GetUrlCacheEntryInfoA,
    (LPCSTR lpszUrlName,LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,LPDWORD lpdwCacheEntryInfoBufferSize),
    (lpszUrlName, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, GetUrlCacheEntryInfoExW,
    (LPCWSTR lpszUrlName,LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,LPDWORD lpdwCacheEntryInfoBufferSize,
    LPWSTR lpszRedirectUrl, LPDWORD lpdwRedirectUrlBufSize,LPVOID lpReserved, DWORD dwReserved),
    (lpszUrlName, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize,lpszRedirectUrl,lpdwRedirectUrlBufSize, lpReserved, dwReserved))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, GetUrlCacheEntryInfoExA,
    (LPCSTR lpszUrlName,LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,LPDWORD lpdwCacheEntryInfoBufferSize,
    LPSTR lpszRedirectUrl, LPDWORD lpdwRedirectUrlBufSize,LPVOID lpReserved, DWORD dwReserved),
    (lpszUrlName, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize,lpszRedirectUrl,lpdwRedirectUrlBufSize, lpReserved, dwReserved))
    
DELAY_LOAD(g_hinstWININET, WININET, BOOL, GetUrlCacheEntryInfoW,
    (LPCWSTR lpszUrlName,LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,LPDWORD lpdwCacheEntryInfoBufferSize),
    (lpszUrlName, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize));   

DELAY_LOAD(g_hinstWININET, WININET, BOOL, SetUrlCacheEntryInfoW,
    (LPCWSTR lpszUrlName,LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,DWORD dwFieldControl),
    (lpszUrlName, lpCacheEntryInfo, dwFieldControl));   

DELAY_LOAD(g_hinstWININET, WININET, BOOL, CommitUrlCacheEntryW,
    (LPCWSTR lpszUrlName, LPCWSTR lpszLocalFileName, FILETIME ExpireTime, FILETIME LastModifiedTime,
    DWORD CacheEntryType, LPWSTR lpHeaderInfo, DWORD dwHeaderSize, LPCWSTR lpszFileExtension, LPCWSTR lpszOriginalUrl),
    (lpszUrlName, lpszLocalFileName, ExpireTime, LastModifiedTime,
    CacheEntryType, lpHeaderInfo, dwHeaderSize, lpszFileExtension, lpszOriginalUrl))
DELAY_LOAD(g_hinstWININET, WININET, BOOL, CommitUrlCacheEntryA,
    (LPCSTR lpszUrlName, LPCSTR lpszLocalFileName, FILETIME ExpireTime, FILETIME LastModifiedTime,
    DWORD CacheEntryType, LPBYTE lpHeaderInfo, DWORD dwHeaderSize, LPCSTR lpszFileExtension, LPCSTR lpszOriginalUrl),
    (lpszUrlName, lpszLocalFileName, ExpireTime, LastModifiedTime,
    CacheEntryType, lpHeaderInfo, dwHeaderSize, lpszFileExtension, lpszOriginalUrl))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, CreateUrlCacheEntryW,
    (LPCWSTR lpszUrlName, DWORD dwExpectedFileSize, LPCWSTR lpszFileExtension, LPWSTR lpszFileName, DWORD dwReserved),
    (lpszUrlName, dwExpectedFileSize, lpszFileExtension, lpszFileName, dwReserved))


DELAY_LOAD(g_hinstWININET, WININET, BOOL, CreateUrlCacheContainerA,
    (LPCSTR Name, LPCSTR lpCachePrefix, LPCSTR lpszCachePath, DWORD KBCacheLimit, DWORD dwContainerType, DWORD dwOptions, LPVOID pvBuffer, LPDWORD cbBuffer),
    (Name, lpCachePrefix, lpszCachePath, KBCacheLimit, dwContainerType, dwOptions, pvBuffer, cbBuffer))


DELAY_LOAD(g_hinstWININET, WININET, BOOL, DeleteUrlCacheContainerA,
    (LPCSTR Name, DWORD dwOptions),
    (Name, dwOptions))



DELAY_LOAD(g_hinstWININET, WININET, HANDLE, FindFirstUrlCacheContainerW,
    (LPDWORD pdwModified, LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo, LPDWORD lpdwContainerInfoBufferSize, DWORD dwOptions),
    (pdwModified, lpContainerInfo, lpdwContainerInfoBufferSize, dwOptions))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, FindNextUrlCacheContainerW,
    (HANDLE hEnumHandle, LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo, LPDWORD lpdwContainerInfoBufferSize),
    (hEnumHandle, lpContainerInfo, lpdwContainerInfoBufferSize))

DELAY_LOAD(g_hinstWININET, WININET, HANDLE, FindFirstUrlCacheContainerA,
    (LPDWORD pdwModified, LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo, LPDWORD lpdwContainerInfoBufferSize, DWORD dwOptions),
    (pdwModified, lpContainerInfo, lpdwContainerInfoBufferSize, dwOptions))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, FindNextUrlCacheContainerA,
    (HANDLE hEnumHandle, LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo, LPDWORD lpdwContainerInfoBufferSize),
    (hEnumHandle, lpContainerInfo, lpdwContainerInfoBufferSize))

DELAY_LOAD(g_hinstWININET, WININET, DWORD, InternetAttemptConnect,
    (DWORD dwReserved),(dwReserved))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, GetUrlCacheConfigInfoA,
    (LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo, LPDWORD lpdwCacheConfigInfoBufferSize, DWORD dwFieldControl), (lpCacheConfigInfo, lpdwCacheConfigInfoBufferSize, dwFieldControl))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, GetUrlCacheConfigInfoW,
    (LPINTERNET_CACHE_CONFIG_INFOW lpCacheConfigInfo, LPDWORD lpdwCacheConfigInfoBufferSize, DWORD dwFieldControl), (lpCacheConfigInfo, lpdwCacheConfigInfoBufferSize, dwFieldControl))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetSetOptionW,
    (HINTERNET hinternet, DWORD dwOptions, LPVOID lpBuffer, DWORD dwBufferLength),
    (hinternet, dwOptions, lpBuffer, dwBufferLength))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetGetConnectedState,
    (LPDWORD lpdwFlags, DWORD dwReserved),
    (lpdwFlags, dwReserved))

DELAY_LOAD(g_hinstWININET, WININET, DWORD, ShowX509EncodedCertificate,
           (HWND hWndParent, LPBYTE lpCert, DWORD cbCert),
           (hWndParent, lpCert, cbCert))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetGetCertByURL,
           (LPSTR lpszURL, LPSTR lpszCertText, DWORD dwcbCertText),
           (lpszURL, lpszCertText, dwcbCertText))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetTimeToSystemTime,
    (LPCWSTR lpszTime, SYSTEMTIME *pst, DWORD dwReserved),
    (lpszTime, pst, dwReserved))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, FreeUrlCacheSpace,
           (LPCTSTR lpszCachePath, DWORD dwSize, DWORD dwFilter),
           (lpszCachePath, dwSize, dwFilter))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetShowSecurityInfoByURL,
            (LPWSTR lpszURL, HWND hwndParent),
            (lpszURL, hwndParent))


DELAY_LOAD(g_hinstWININET, WININET, BOOL, SetUrlCacheEntryGroupW,
           (LPCWSTR lpszUrlName, DWORD dwFlags, GROUPID GroupId, LPBYTE pbGroupAttributes, DWORD cbGroupAttributes, LPVOID lpReserved),
           (lpszUrlName, dwFlags, GroupId, pbGroupAttributes, cbGroupAttributes, lpReserved))

DELAY_LOAD(g_hinstWININET, WININET, HINTERNET, InternetOpenA,
           (LPCSTR lpszAgent, DWORD dwAccessType, LPCSTR lpszProxy OPTIONAL, LPCSTR lpszProxyBypass OPTIONAL, DWORD dwFlags),
           (lpszAgent, dwAccessType, lpszProxy OPTIONAL, lpszProxyBypass OPTIONAL, dwFlags))

DELAY_LOAD(g_hinstWININET, WININET, HINTERNET, InternetConnectA,
           (HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort, LPCSTR lpszUserName OPTIONAL, LPCSTR lpszPassword OPTIONAL, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext),
           (hInternet, lpszServerName, nServerPort, lpszUserName OPTIONAL, lpszPassword OPTIONAL, dwService, dwFlags, dwContext))

DELAY_LOAD(g_hinstWININET, WININET, HINTERNET, HttpOpenRequestA,
           (HINTERNET hConnect, LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion, LPCSTR lpszReferrer OPTIONAL, LPCSTR FAR * lplpszAcceptTypes OPTIONAL, DWORD dwFlags, DWORD_PTR dwContext),
           (hConnect, lpszVerb, lpszObjectName, lpszVersion, lpszReferrer OPTIONAL, lplpszAcceptTypes OPTIONAL, dwFlags, dwContext))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, HttpAddRequestHeadersA,
           (HINTERNET hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwModifiers),
           (hRequest, lpszHeaders, dwHeadersLength, dwModifiers))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, HttpSendRequestA,
           (HINTERNET hRequest, LPCSTR lpszHeaders OPTIONAL, DWORD dwHeadersLength, LPVOID lpOptional OPTIONAL, DWORD dwOptionalLength),
           (hRequest, lpszHeaders OPTIONAL, dwHeadersLength, lpOptional OPTIONAL, dwOptionalLength))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, InternetCloseHandle,
           (HINTERNET hInternet),
           (hInternet))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, HttpQueryInfo,
           (HINTERNET hRequest, DWORD dwInfoLevel, LPVOID lpBuffer OPTIONAL, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex OPTIONAL),
           (hRequest, dwInfoLevel, lpBuffer OPTIONAL, lpdwBufferLength, lpdwIndex OPTIONAL))

DELAY_LOAD(g_hinstWININET, WININET, INTERNET_STATUS_CALLBACK, InternetSetStatusCallbackA,
           (HINTERNET hInternet, INTERNET_STATUS_CALLBACK lpfnInternetCallback),
           (hInternet, lpfnInternetCallback))

#ifdef UNIX

DELAY_LOAD_VOID(g_hinstWININET, WININET, unixGetWininetCacheLockStatus,
            (BOOL *pBoolReadOnly, char **ppszLockingHost),
            (pBoolReadOnly, ppszLockingHost))

DELAY_LOAD_VOID(g_hinstWININET, WININET, unixCleanupWininetCacheLockFile,
            (),
            ())
#endif

DELAY_LOAD(g_hinstWININET, WININET, HANDLE, RetrieveUrlCacheEntryStreamW,
            (LPCWSTR  lpszUrlName, LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
             LPDWORD lpdwCacheEntryInfoBufferSize, BOOL fRandomRead, DWORD dwReserved),
            (lpszUrlName, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize, 
             fRandomRead, dwReserved))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, ReadUrlCacheEntryStream,
            (HANDLE hUrlCacheStream, DWORD dwLocation, LPVOID lpBuffer, LPDWORD lpdwLen, DWORD dwReserved),
            (hUrlCacheStream, dwLocation, lpBuffer, lpdwLen, dwReserved))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, UnlockUrlCacheEntryStream,
            (HANDLE hUrlCacheStream, DWORD dwReserved),
            (hUrlCacheStream, dwReserved))

DELAY_LOAD(g_hinstWININET, WININET, BOOL, RegisterUrlCacheNotification,
     (IN  HWND        hWnd,
      IN  UINT        uMsg,
      IN  GROUPID     gid,
      IN  DWORD       dwOpsFilter,
      IN  DWORD       dwReserved),
     (hWnd, uMsg, gid, dwOpsFilter, dwReserved))


#ifndef UNIX
DELAY_LOAD_IE_ORD(g_hinstWININET, WININET, BOOL,
                  ImportCookieFileA, 108,
                  (LPCSTR szFilename),
                  (szFilename))

DELAY_LOAD_IE_ORD(g_hinstWININET, WININET, BOOL,
                  ExportCookieFileA, 109,
                  (LPCSTR szFilename, BOOL fAppend),
                  (szFilename, fAppend))

DELAY_LOAD_IE_ORD(g_hinstWININET, WININET, BOOL,
                  ImportCookieFileW, 110,
                  (LPCTSTR szFilename),
                  (szFilename))

DELAY_LOAD_IE_ORD(g_hinstWININET, WININET, BOOL,
                  ExportCookieFileW, 111,
                  (LPCTSTR szFilename, BOOL fAppend),
                  (szFilename, fAppend))

DELAY_LOAD_IE_ORD(g_hinstWININET, WININET, BOOL,
                  IsProfilesEnabled, 112,
                  (),
                  ())
#else
DELAY_LOAD(g_hinstWININET, WININET, BOOL,
                  ImportCookieFileA,
                  (LPCSTR szFilename),
                  (szFilename))

DELAY_LOAD(g_hinstWININET, WININET, BOOL,
                  ExportCookieFileA,
                  (LPCSTR szFilename, BOOL fAppend),
                  (szFilename, fAppend))

DELAY_LOAD(g_hinstWININET, WININET, BOOL,
                  ImportCookieFileW,
                  (LPCTSTR szFilename),
                  (szFilename))

DELAY_LOAD(g_hinstWININET, WININET, BOOL,
                  ExportCookieFileW,
                  (LPCTSTR szFilename, BOOL fAppend),
                  (szFilename, fAppend))

DELAY_LOAD(g_hinstWININET, WININET, BOOL,
                  IsProfilesEnabled, 
                  (),
                  ())
#endif /* UNIX */



// --------- MPR.DLL ---------------

HINSTANCE g_hinstMPR = NULL;

DELAY_LOAD_WNET(g_hinstMPR, MPR, WNetGetLastErrorA,
       ( LPDWORD lpError, LPSTR lpErrorBuf, DWORD  nErrorBufSize, LPSTR lpNameBuf, DWORD nNameBufSize),
       (lpError,   lpErrorBuf,    nErrorBufSize,    lpNameBuf,      nNameBufSize  ))
DELAY_LOAD_WNET(g_hinstMPR, MPR, WNetDisconnectDialog,
       ( HWND  hwnd, DWORD dwType), (hwnd, dwType))

DELAY_LOAD_WNET(g_hinstMPR, MPR, WNetCancelConnection2A,
       (LPCSTR lpName, DWORD dwFlags, BOOL fForce),
       (lpName, dwFlags, fForce))
DELAY_LOAD_WNET(g_hinstMPR, MPR, WNetCancelConnection2W,
       (LPCWSTR lpName, DWORD dwFlags, BOOL fForce),
       (lpName, dwFlags, fForce))
DELAY_LOAD_WNET(g_hinstMPR, MPR, WNetCloseEnum, (HANDLE hEnum), (hEnum))
DELAY_LOAD_WNET(g_hinstMPR, MPR, WNetEnumResourceA,
       (HANDLE  hEnum, LPDWORD lpcCount, LPVOID  lpBuffer, LPDWORD lpBufferSize),
       (hEnum, lpcCount, lpBuffer, lpBufferSize))
DELAY_LOAD_WNET(g_hinstMPR, MPR, WNetGetConnectionA,
       (LPCSTR lpLocalName, LPSTR lpRemoteName, LPDWORD lpnLength),
       (lpLocalName, lpRemoteName, lpnLength))

// --------- URLMON.DLL ---------------

HINSTANCE g_hinstURLMON = NULL;

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, CreateFormatEnumerator,
    (UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC** ppenumfmtetc),
    (cfmtetc, rgfmtetc, ppenumfmtetc))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, RegisterMediaTypes,
    (UINT ctypes, const LPCSTR* rgszTypes, CLIPFORMAT* rgcfTypes),
    (ctypes, rgszTypes, rgcfTypes))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, RegisterFormatEnumerator,
    (LPBC pBC, IEnumFORMATETC *pEFetc, DWORD reserved), (pBC, pEFetc, reserved))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, CreateURLMoniker,
    (IMoniker* pMkCtx, LPCWSTR pwsURL, IMoniker ** ppimk), (pMkCtx, pwsURL, ppimk))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, CreateAsyncBindCtxEx,
    (IBindCtx *pbc,DWORD dwOption, IBindStatusCallback *pBSCb, IEnumFORMATETC *pEnum, IBindCtx **ppBC, DWORD reserved), (pbc, dwOption, pBSCb, pEnum, ppBC, reserved))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, RegisterBindStatusCallback,
    (LPBC pBC, IBindStatusCallback *pBSCb,  IBindStatusCallback **ppBSCbPrev, DWORD dwReserved), (pBC, pBSCb, ppBSCbPrev, dwReserved))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, RevokeBindStatusCallback,
    (LPBC pBC, IBindStatusCallback *pBSCb), (pBC, pBSCb))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, RegisterMediaTypeClass,
    (LPBC pBC, UINT ctypes, const LPCSTR* rgszTypes, CLSID *rgclsID, DWORD reserved),
    (pBC, ctypes, rgszTypes, rgclsID, reserved))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, GetClassFileOrMime,
    (LPBC pBC, LPCWSTR szFilename, LPVOID pBuffer, DWORD cbSize, LPCWSTR szMime, DWORD dwReserved, CLSID *pclsid),
    (pBC, szFilename, pBuffer, cbSize, szMime, dwReserved, pclsid))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, IsValidURL,
    (LPBC pBC, LPCWSTR szURL, DWORD dwReserved),
    (pBC, szURL, dwReserved))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, URLDownloadToCacheFile,
     (IUnknown *pCaller, LPCTSTR szURL, LPTSTR szFileName, DWORD fCache, DWORD dwResv, LPBINDSTATUSCALLBACK lpfnCB),
     (pCaller, szURL, szFileName, fCache, dwResv, lpfnCB))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, GetSoftwareUpdateInfo,
    (LPCWSTR szDistUnit, LPSOFTDISTINFO psdi),
    (szDistUnit, psdi))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, SetSoftwareUpdateAdvertisementState,
    (LPCWSTR szDistUnit, DWORD dwAdState, DWORD dwAdvertisedVersionMS, DWORD dwAdvertisedVersionLS),
    (szDistUnit, dwAdState, dwAdvertisedVersionMS, dwAdvertisedVersionLS))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, CoInternetQueryInfo,
    (LPCWSTR pwzUrl, QUERYOPTION QueryOptions, DWORD dwQueryFlags, LPVOID pvBuffer, DWORD cbBuffer, DWORD *pcbBuffer, DWORD dwReserved),
    (pwzUrl, QueryOptions, dwQueryFlags, pvBuffer, cbBuffer, pcbBuffer, dwReserved))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, ObtainUserAgentString,
    (DWORD dwOption, LPSTR pszUAOut, DWORD* cbSize),
    (dwOption, pszUAOut, cbSize))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, CoInternetGetSecurityUrl,
    (LPCWSTR pwzUrl, LPWSTR *ppwzSecUrl, PSUACTION psuAction, DWORD dwReserved),
    (pwzUrl, ppwzSecUrl, psuAction, dwReserved))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, CoInternetParseUrl,
    (LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwFlags, LPWSTR pszResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved),
    (pwzUrl, ParseAction, dwFlags, pszResult, cchResult, pcchResult, dwReserved))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, CopyStgMedium,
    (const STGMEDIUM * pcstgmedSrc, STGMEDIUM * pstgmedDest),
    (pcstgmedSrc, pstgmedDest))

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, CopyBindInfo,
    (const BINDINFO * pcbiSrc, BINDINFO * pbiDest),
    (pcbiSrc, pbiDest))

DELAY_LOAD_VOID(g_hinstURLMON, URLMON, ReleaseBindInfo,
    ( BINDINFO * pbindinfo ),
    ( pbindinfo ))


DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, FaultInIEFeature,
    (HWND hWnd, uCLSSPEC *pClassSpec, QUERYCONTEXT *pQuery, DWORD dwFlags),
    (hWnd, pClassSpec, pQuery, dwFlags));

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, URLDownloadToFile,
    (IUnknown *pCaller, LPCTSTR szURL, LPCTSTR szFileName, DWORD dwResv, LPBINDSTATUSCALLBACK lpfnCB),
    (pCaller, szURL, szFileName,  dwResv, lpfnCB)); 

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, CoInternetCreateSecurityManager,
    (IServiceProvider *pSP, IInternetSecurityManager **ppSM, DWORD dwReserved),
    (pSP, ppSM, dwReserved));

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, GetMarkOfTheWeb,
    (LPCSTR pszURL, LPCSTR pszFile, DWORD dwFlags, LPSTR *ppszMark),
    ( pszURL,  pszFile, dwFlags, ppszMark)); 

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, URLOpenBlockingStreamW,
    (IUnknown *lpunk, LPCWSTR lpcstrURL, IStream **ppstm, DWORD dw, LPBINDSTATUSCALLBACK bsc),
    ( lpunk,lpcstrURL, ppstm, dw, bsc)); 

DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, FindMimeFromData,
    (LPBC pBC, LPCWSTR pwzUrl, void *pBuffer, DWORD cbSize, LPCWSTR pwzMimeProposed, DWORD dwMimeFlags, LPWSTR *ppwzMimeOut, DWORD dwReserved),
    (pBC, pwzUrl, pBuffer, cbSize, pwzMimeProposed, dwMimeFlags, ppwzMimeOut, dwReserved)); 
 
DELAY_LOAD_HRESULT(g_hinstURLMON, URLMON, UrlMkGetSessionOption,
    (DWORD dwOption, LPVOID pBuffer, DWORD dwBufferLength, DWORD *pdwBufferLength, DWORD dwReserved),
    (dwOption, pBuffer, dwBufferLength, pdwBufferLength, dwReserved));
#ifdef FEATURE_PICS

// --------- MSRATING.DLL ---------------

HINSTANCE g_hinstMSRATING = NULL;
DELAY_LOAD_HRESULT(g_hinstMSRATING, MSRATING, RatingEnabledQuery,
            (),
            ())
DELAY_LOAD_HRESULT(g_hinstMSRATING, MSRATING, RatingCheckUserAccess,
            (LPCSTR pszUsername, LPCSTR pszURL, LPCSTR pszRatingInfo, LPBYTE pData,
             DWORD cbData, void **ppRatingDetails),
            (pszUsername, pszURL, pszRatingInfo, pData, cbData, ppRatingDetails))
DELAY_LOAD_HRESULT(g_hinstMSRATING, MSRATING, RatingAccessDeniedDialog,
            (HWND hDlg, LPCSTR pszUsername, LPCSTR pszContentDescription,
             void *pRatingDetails),
            (hDlg, pszUsername, pszContentDescription, pRatingDetails))
DELAY_LOAD_HRESULT(g_hinstMSRATING, MSRATING, RatingAccessDeniedDialog2,
            (HWND hDlg, LPCSTR pszUsername, void *pRatingDetails),
            (hDlg, pszUsername, pRatingDetails))
DELAY_LOAD_HRESULT(g_hinstMSRATING, MSRATING, RatingFreeDetails,
            (void *pRatingDetails),
            (pRatingDetails))
DELAY_LOAD_HRESULT(g_hinstMSRATING, MSRATING, RatingObtainCancel,
            (HANDLE hRatingObtainQuery),
            (hRatingObtainQuery))
DELAY_LOAD_HRESULT(g_hinstMSRATING, MSRATING, RatingObtainQuery,
            (LPCTSTR pszTargetUrl, DWORD dwUserData, void (*fCallback)(DWORD dwUserData, HRESULT hr, LPCTSTR pszRating, void *lpvRatingDetails), HANDLE *phRatingObtainQuery),
            (pszTargetUrl, dwUserData, fCallback, phRatingObtainQuery))

#endif
// --------- MSJAVA.DLL ---------------
HINSTANCE g_hinstMSJAVA = NULL;
DELAY_LOAD_VOID(g_hinstMSJAVA, MSJAVA, ShowJavaConsole, (), () )

// --------- MSHTML.DLL ---------------
HINSTANCE g_hinstMSHTML = NULL;
DELAY_LOAD_HRESULT(g_hinstMSHTML, MSHTML, ShowHTMLDialog,
            (HWND hwndParent, IMoniker *pmk, VARIANT *pvarArgIn, WCHAR* pchOptions, VARIANT *pvArgOut),
            (hwndParent, pmk, pvarArgIn, pchOptions, pvArgOut))

DELAY_LOAD_HRESULT(g_hinstMSHTML, MSHTML, ShowModelessHTMLDialog,
            (HWND hwndParent, IMoniker *pmk, VARIANT *pvarArgIn, VARIANT* pvarOptions, IHTMLWindow2 **ppWindow),
            (hwndParent, pmk, pvarArgIn, pvarOptions, ppWindow))

DELAY_LOAD_HRESULT(g_hinstMSHTML, MSHTML, CreateHTMLPropertyPage,
            (IMoniker * pmk, IPropertyPage ** ppPP),
            (pmk, ppPP))

// --------- MLANG.DLL ---------------


HINSTANCE g_hinstMLANG = NULL;

DELAY_LOAD_HRESULT(g_hinstMLANG, MLANG, ConvertINetMultiByteToUnicode,
            (LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount),
            (lpdwMode, dwEncoding, lpSrcStr, lpnMultiCharCount, lpDstStr, lpnWideCharCount))

DELAY_LOAD_HRESULT(g_hinstMLANG, MLANG, ConvertINetUnicodeToMultiByte,
            (LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount),
            (lpdwMode, dwEncoding, lpSrcStr, lpnWideCharCount, lpDstStr, lpnMultiCharCount))

DELAY_LOAD_HRESULT(g_hinstMLANG, MLANG, LcidToRfc1766W,
            (LCID Locale, LPWSTR pszRfc1766, int nChar),
            (Locale, pszRfc1766, nChar))

DELAY_LOAD_HRESULT(g_hinstMLANG, MLANG, Rfc1766ToLcidW,
            (LCID *pLocale, LPCWSTR pszRfc1766),
            (pLocale, pszRfc1766))

// --------- IMM32.DLL ---------------
HINSTANCE g_hinstImm32 = NULL;

DELAY_LOAD(g_hinstImm32, IMM32, UINT, ImmGetVirtualKey,
            (HWND hWnd), (hWnd))
// --------- VERSION.DLL ---------------
HINSTANCE g_hinstVERSION = NULL;

DELAY_LOAD(g_hinstVERSION, VERSION, BOOL,
           VerQueryValueA, (LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen),
           (pBlock, lpSubBlock, lplpBuffer, puLen))

DELAY_LOAD(g_hinstVERSION, VERSION, DWORD,
           GetFileVersionInfoSizeA, (LPSTR lptstrFilename, LPDWORD lpdwHandle),
           (lptstrFilename, lpdwHandle))

DELAY_LOAD(g_hinstVERSION, VERSION, BOOL,
           GetFileVersionInfoA, (LPSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData),
           (lptstrFilename, dwHandle, dwLen, lpData))
// --------- NTDLL.DLL ---------------
//
// WARNING!  NTDLL doesn't exist on Win95 so make sure you have a fallback

HINSTANCE g_hinstNTDLL = NULL;

//
//  DELAY_LOAD returns zero, which is good since that's not a valid NT
//  product type code and hence can substitute as a reasonable error code.
//
DELAY_LOAD(g_hinstNTDLL, NTDLL, BOOLEAN, RtlGetNtProductType,
            (PNT_PRODUCT_TYPE NtProductType), (NtProductType))

// --------- INETCOMM.DLL ---------------

HINSTANCE g_hinstINETCOMM = NULL;

DELAY_LOAD_HRESULT(g_hinstINETCOMM, INETCOMM, MimeOleCreateMessage,
    (IUnknown  *pUnkOuter, IMimeMessage **ppMessage),
    (pUnkOuter, ppMessage));

DELAY_LOAD_HRESULT(g_hinstINETCOMM, INETCOMM, MimeOleParseMhtmlUrl,
    (LPSTR pszUrl, LPSTR *ppszRootUrl, LPSTR *ppszBodyUrl),
    (pszUrl, ppszRootUrl, ppszBodyUrl));

DELAY_LOAD_HRESULT(g_hinstINETCOMM, INETCOMM, MimeOleCreateVirtualStream,
    (IStream **ppStream),
    (ppStream));

DELAY_LOAD_HRESULT(g_hinstINETCOMM, INETCOMM, MimeOleSetCompatMode,
                   (DWORD dwMode),
                   (dwMode));

// ---------- OLEPRO32.DLL ---------------

HINSTANCE g_hinstOLEPRO32 = NULL;

DELAY_LOAD_HRESULT(g_hinstOLEPRO32, OLEPRO32, OleCreatePropertyFrameIndirect,
    (OCPFIPARAMS * pParams),
    (pParams));

#ifdef UNIX_FEATURE_ALIAS

HINSTANCE g_hinstINETCPL = NULL;

DELAY_LOAD(g_hinstINETCPL, inetcpl, BOOL, LoadAliases,
            (HDPA aliasList), (aliasList))
DELAY_LOAD(g_hinstINETCPL, inetcpl, BOOL, SaveAliases,
            (HDPA aliasList), (aliasList))
DELAY_LOAD(g_hinstINETCPL, inetcpl, BOOL, FreeAliases,
            (HDPA aliasList), (aliasList))
DELAY_LOAD(g_hinstINETCPL, inetcpl, BOOL, AddAliasToListA,
            (HDPA aliasListIn, LPSTR aliasIn, LPSTR szurl, HWND hwnd), (aliasListIn, aliasIn, szurl, hwnd))
DELAY_LOAD(g_hinstINETCPL, inetcpl, BOOL, FindAliasByURLA,
            (HDPA aliasListIn, LPSTR szurl, LPSTR aliasIn, INT cchAlias), (aliasListIn, szurl, aliasIn, cchAlias))

#endif

// --------- ole32.DLL ----------------
HINSTANCE g_hinstOLE32 = NULL;

DELAY_LOAD_HRESULT(g_hinstOLE32, OLE32, CoAllowSetForegroundWindow,
               (IUnknown *pUnk, void* lpvReserved), 
               (pUnk, lpvReserved));


// --------- ADVPACK.DLL ---------------

HINSTANCE g_hinstADVPACK = NULL;

DELAY_LOAD(
    g_hinstADVPACK, 
    ADVPACK, 
    HRESULT, 
    RunSetupCommand,
    (HWND hWnd, LPCSTR szCmdName, LPCSTR szInfSection, LPCSTR szDir, LPCSTR lpszTitle, HANDLE *phEXE, DWORD dwFlags, LPVOID pvReserved),
    (hWnd,szCmdName,szInfSection,szDir,lpszTitle,phEXE,dwFlags,pvReserved))

void FreeDynamicLibraries()
{
    extern HINSTANCE g_hinstSHDOC401;

    if (g_hinstSHDOC401)
        FreeLibrary(g_hinstSHDOC401);
    if (g_hinstShell32)
        FreeLibrary(g_hinstShell32);
    if (g_hinstBrowseui)
        FreeLibrary(g_hinstBrowseui);
    if (g_hinstCOMDLG32)
        FreeLibrary(g_hinstCOMDLG32);
    if (g_hinstOLEAUT32)
        FreeLibrary(g_hinstOLEAUT32);
    if (g_hinstWININET)
        FreeLibrary(g_hinstWININET);
    if (g_hinstMPR)
        FreeLibrary(g_hinstMPR);
    if (g_hinstURLMON)
        FreeLibrary(g_hinstURLMON);
    if (g_hinstMSRATING)
        FreeLibrary(g_hinstMSRATING);
    if (g_hinstMSJAVA)
        FreeLibrary(g_hinstMSJAVA);
    if (g_hinstMSHTML)
        FreeLibrary(g_hinstMSHTML);
    if (g_hinstMLANG)
        FreeLibrary(g_hinstMLANG);
    if (g_hinstImm32)
        FreeLibrary(g_hinstImm32);
    if (g_hinstVERSION)
        FreeLibrary(g_hinstVERSION);
    if (g_hinstNTDLL)
        FreeLibrary(g_hinstNTDLL);
    if (g_hinstINETCOMM)
        FreeLibrary(g_hinstINETCOMM);
#ifdef UNIX
    if (g_hinstOLEPRO32)
        FreeLibrary(g_hinstOLEPRO32);
#endif
#ifdef UNIX_FEATURE_ALIAS
    if (g_hinstINETCPL)
        FreeLibrary(g_hinstINETCPL);
#endif

    if (g_hinstADVPACK)
        FreeLibrary(g_hinstADVPACK);

}
