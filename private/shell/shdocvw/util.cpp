/***************************************************************************/
/* WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! */
/***************************************************************************/
/* As part of the shdocvw/browseui split, parts of this file are moving to */
/* shell32/shlwapi (#ifdef POSTPOSTSPLIT).  Make sure you make your delta  */
/* to the shell32/shlwapi version also!                                    */
/***************************************************************************/


#include "priv.h"
#include "sccls.h"
#include "shlobj.h"

#include <fsmenu.h>
#include <tchar.h>

#ifndef UNIX
#include <webcheck.h>
#else
#include <subsmgr.h>
#endif

#include "resource.h"
#include "mshtml.h"     // for IHTMLElement
#include "mlang.h"  // fo char conversion
#include <advpub.h> // for IE activesetup GUID
#include "winineti.h" // For name of a mutex used in IsWininetLoadedAnywhere()
#include "htregmng.h"
#include <ntverp.h>
#include "mruex.h"
#include "mttf.h"
#include <platform.h>
#include <mobsync.h>
#include <mobsyncp.h>
#include <winuser.h>
#include <mluisupp.h>
#include "shdocfl.h"
#include <shlwapip.h>

#include "../lib/brutil.cpp"

STDAPI CDelegateMalloc_Create(void *pv, SIZE_T cbSize, WORD wOuter, IMalloc **ppmalloc);

const VARIANT c_vaEmpty = {0};

const TCHAR c_szRegKeyTypedURLs[]     = TEXT("Software\\Microsoft\\Internet Explorer\\TypedURLs");

#define DM_SESSIONCOUNT     0

int     g_cxIcon = 0;
int     g_cyIcon = 0;
int     g_cxSmIcon = 0;
int     g_cySmIcon = 0;


const DISPPARAMS c_dispparamsNoArgs = {NULL, NULL, 0, 0};
const LARGE_INTEGER c_li0 = { 0, 0 };

const ITEMIDLIST s_idlNULL = { 0 } ;


int InitColorDepth(void)
{
    static int s_lrFlags = 0;              // Flags passed to LoadImage
    if (s_lrFlags == 0)
    {
        int nColorRes, nIconDepth = 0;
        HKEY hkey;

        // Determine the color depth so we can load the best image
        // (This code was stolen from FileIconInit in shell32)

        // Get the user prefered icon size (and color depth) from the
        // registry.
        //
        if (NO_ERROR == RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_METRICS, &hkey))
        {
            nIconDepth = SHRegGetIntW(hkey, L"Shell Icon Bpp", nIconDepth);
            RegCloseKey(hkey);
        }

        nColorRes = GetCurColorRes();

        if (nIconDepth > nColorRes)
            nIconDepth = 0;

        if (nColorRes <= 8)
            nIconDepth = 0; // wouldn't have worked anyway

        if (nColorRes > 4 && nIconDepth <= 4)
            s_lrFlags = LR_VGACOLOR;
        else
            s_lrFlags = LR_DEFAULTCOLOR;
    }
    return s_lrFlags;
}

HICON   g_hiconSplat = NULL;
HICON   g_hiconSplatSm = NULL;      // small version

void LoadCommonIcons(void)
{
    if (NULL == g_hiconSplat)
    {
        // Use LoadLibraryEx so we don't load code pages
        HINSTANCE hinst = LoadLibrary(TEXT("url.dll"));
        if (hinst)
        {
            int lrFlags = InitColorDepth();
            g_hiconSplat   = (HICON)LoadImage(hinst, MAKEINTRESOURCE(IDI_URL_SPLAT), IMAGE_ICON, g_cxIcon, g_cyIcon, lrFlags);
            g_hiconSplatSm = (HICON)LoadImage(hinst, MAKEINTRESOURCE(IDI_URL_SPLAT), IMAGE_ICON, g_cxSmIcon, g_cySmIcon, lrFlags);

            FreeLibrary(hinst);
        }
    }
}

STDAPI_(BOOL) UrlHitsNetW(LPCWSTR pszURL)
{
    BOOL fResult;

    // Handle the easy ones on our own and call URLMON for the others.

    switch (GetUrlScheme(pszURL))
    {
    case URL_SCHEME_FILE:
    case URL_SCHEME_RES:
        fResult = FALSE;
        break;

    case URL_SCHEME_HTTP:
    case URL_SCHEME_HTTPS:
    case URL_SCHEME_FTP:
    case URL_SCHEME_GOPHER:
    case URL_SCHEME_TELNET:
    case URL_SCHEME_WAIS:
        fResult = TRUE;
        break;

    default:
        {
        DWORD fHitsNet;
        DWORD dwSize;
        fResult = SUCCEEDED(CoInternetQueryInfo(
                            pszURL, QUERY_USES_NETWORK,
                            0, &fHitsNet, sizeof(fHitsNet), &dwSize, 0)) && fHitsNet;
        }
    }

    return fResult;
}

STDAPI_(BOOL) CallCoInternetQueryInfo(LPCTSTR pszURL, QUERYOPTION QueryOption)
{
    DWORD fRetVal;
    DWORD dwSize;
    return SUCCEEDED(CoInternetQueryInfo(
                        pszURL, QueryOption,
                        0, &fRetVal, sizeof(fRetVal), &dwSize, 0)) && fRetVal;
}

// see if a given URL is in the cache
STDAPI_(BOOL) UrlIsInCache(LPCTSTR pszURL)
{
    return CallCoInternetQueryInfo(pszURL, QUERY_IS_CACHED);
}

// See if a give URL is actually present as an installed entry
STDAPI_(BOOL) UrlIsInstalledEntry(LPCTSTR pszURL)
{
    return CallCoInternetQueryInfo(pszURL, QUERY_IS_INSTALLEDENTRY);
}


// see if a given URL is in the cache OR if it is mapped

STDAPI_(BOOL) UrlIsMappedOrInCache(LPCTSTR pszURL)
{
    return CallCoInternetQueryInfo(pszURL, QUERY_IS_CACHED_OR_MAPPED);
}

BOOL IsFileUrlW(LPCWSTR pcwzUrl)
{
    return (GetUrlSchemeW(pcwzUrl) == URL_SCHEME_FILE);
}

BOOL IsFileUrl(LPCSTR psz)
{
    return (GetUrlSchemeA(psz) == URL_SCHEME_FILE);
}

BOOL PathIsFilePath(LPCWSTR lpszPath)
{
#ifdef UNIX
    if (lpszPath[0] == TEXT('/'))
#else
    if ((lpszPath[0] == TEXT('\\')) || (lpszPath[1] == TEXT(':')))
#endif
        return TRUE;

    return IsFileUrlW(lpszPath);
}

BOOL IsSubscribableW(LPCWSTR pszUrl)
{
    //  BUGBUG: this should be method on the subscription mgr interface - zekel
    DWORD dwScheme = GetUrlSchemeW(pszUrl);
    return (dwScheme == URL_SCHEME_HTTP) || (dwScheme == URL_SCHEME_HTTPS);
}

BOOL IsSubscribableA(LPCSTR pszUrl)
{
    //  BUGBUG: this should be method on the subscription mgr interface - zekel
    DWORD dwScheme = GetUrlSchemeA(pszUrl);
    return (dwScheme == URL_SCHEME_HTTP) || (dwScheme == URL_SCHEME_HTTPS);
}

DWORD SHRandom(void)
{
    GUID guid;
    DWORD dw;

    CoCreateGuid(&guid);
    HashData((LPBYTE)&guid, SIZEOF(guid), (LPBYTE)&dw, SIZEOF(dw));

    return dw;
}

// See if we are hosted by IE (explorer.exe or iexplore.exe)
BOOL IsInternetExplorerApp()
{
    if ((g_fBrowserOnlyProcess) ||                  // if in iexplore.exe process,
        (GetModuleHandle(TEXT("EXPLORER.EXE"))))        // or explorer.exe process,
    {
        return TRUE;                                // then we are IE
    }

    return FALSE;
}

BOOL IsTopFrameBrowser(IServiceProvider *psp, IUnknown *punk)
{
    IShellBrowser *psb;

    ASSERT(psp);
    ASSERT(punk);

    BOOL fRet = FALSE;
    if(SUCCEEDED(psp->QueryService(SID_STopFrameBrowser, IID_IShellBrowser, (void **)&psb)))
    {
        fRet = IsSameObject(psb, punk);
        psb->Release();
    }
    return fRet;
}

DWORD GetUrlSchemePidl(LPITEMIDLIST pidl)
{
    if (pidl && IsURLChild(pidl, TRUE))
    {
        TCHAR szUrl[MAX_URL_STRING];
        if (SUCCEEDED(IEGetDisplayName(pidl, szUrl, SHGDN_FORPARSING)))
            return GetUrlScheme(szUrl);
    }
    return URL_SCHEME_INVALID;
}


//-----------------------------------------------------------------------------


#ifndef POSTPOSTSPLIT


STDAPI_(BSTR) LoadBSTR(UINT uID)
{
    WCHAR wszBuf[MAX_PATH];
    if (MLLoadStringW(uID, wszBuf, ARRAYSIZE(wszBuf)))
    {
        return SysAllocString(wszBuf);
    }
    return NULL;
}

#endif

BOOL StringIsUTF8A(LPCSTR psz, DWORD cb)
{
    BOOL fRC = FALSE;
    CHAR *pb;
    CHAR b;
    DWORD dwCnt;
    DWORD dwUTF8Cnt;

    if(!psz || !(*psz) || cb == 0)
        return(FALSE);

    pb = (CHAR*)psz;
    while(cb-- && *pb)
    {
        if((*pb & 0xc0) == 0xc0) // bit pattern starts with 11
        {
            dwCnt = dwUTF8Cnt = 0;
            b = *pb;
            while((b & 0xc0) == 0xc0)
            {
                dwCnt++;
                if((*(pb+dwCnt) & 0xc0) == 0x80)   // bits at dwCnt bytes from current offset in str aren't 10
                    dwUTF8Cnt++;
                b = (b << 1) & 0xff;
            }
            if(dwCnt == dwUTF8Cnt)
                fRC = TRUE;       // Found UTF8 encoded chars
                
            pb += ++dwCnt;
        }
        else
        {
            pb++;
        }
    }

    return(fRC);
}


BOOL StringIsUTF8W(LPCWSTR pwz, DWORD cb)
{
    BOOL  fRC = FALSE;
    WCHAR *pb;
    WCHAR b;
    DWORD dwCnt;
    DWORD dwUTF8Cnt;

    if(!pwz || !(*pwz) || cb == 0)
        return(FALSE);

    pb = (WCHAR*)pwz;
    while(cb-- && *pb)
    {
        if(*pb > 255)   // Non ansi so bail
            return(FALSE);
            
        if((*pb & 0xc0) == 0xc0) // bit pattern starts with 11
        {
            dwCnt = dwUTF8Cnt = 0;
            b = *pb;
            while((b & 0xc0) == 0xc0)
            {
                dwCnt++;
                if((*(pb+dwCnt) & 0xc0) == 0x80)   // bits at dwCnt bytes from current offset in str aren't 10
                    dwUTF8Cnt++;
                b = (b << 1) & 0xff;
            }
            if(dwCnt == dwUTF8Cnt)
                fRC = TRUE;       // Found UTF8 encoded chars
                
            pb += ++dwCnt;
        }
        else
        {
            pb++;
        }
    }

    return(fRC);
}

//
//  StringContainsHighAnsi
//
//  Determine if string contains high-ANSI characters. Search is
//    stopped when we hit the first high-ANSI character, when we hit the terminator
//    or when we have decremented dwInLen to zero
//
//  Return Value:
//      TRUE    - pszIn contains one or more high-ANSI characters
//
//      FALSE   - pszIn (or substring of length dwInLen) does not contain
//                high-ANSI characters
//
BOOL StringContainsHighAnsiA(LPCSTR pszIn, DWORD dwInLen)
{
    while (dwInLen-- && *pszIn) 
    {
        if (*pszIn++ & 0x80)
            return TRUE;
    }
    return FALSE;
}

BOOL StringContainsHighAnsiW(LPCWSTR pszIn, DWORD dwInLen)
{
    while (dwInLen-- && *pszIn) 
    {
        if (*pszIn++ & 0x80)
            return TRUE;
    }
    return FALSE;
}

BOOL UTF8Enabled(VOID)
{
    static DWORD   dwIE = URL_ENCODING_NONE;
    DWORD   dwOutLen = sizeof(DWORD);
    
    if(dwIE == URL_ENCODING_NONE)
        UrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &dwIE, sizeof(DWORD), &dwOutLen, NULL);
    return(dwIE == URL_ENCODING_ENABLE_UTF8);
}

//
// PrepareURLForDisplay
//
//     Decodes without stripping file:// prefix
//

#undef PrepareURLForDisplay
BOOL PrepareURLForDisplayW(LPCWSTR pwz, LPWSTR pwzOut, LPDWORD pcbOut)
{
    if (PathIsFilePath(pwz))
    {
        if (IsFileUrlW(pwz))
            return SUCCEEDED(PathCreateFromUrlW(pwz, pwzOut, pcbOut, 0));

        StrCpyNW(pwzOut, pwz, *pcbOut);
        *pcbOut = lstrlenW(pwzOut);
        return TRUE;
    }
    
    return SUCCEEDED(UrlUnescapeW((LPWSTR)pwz, pwzOut, pcbOut, 0));
}

//
// PrepareURLForDisplayUTF8W
//
// pwz -          [In] UTF8 encoded string like "%e6%aa%e4%a6.doc".
// pwzOut -       [Out]  UTF8 decoded string.
// pcchOut -      [In/Out] Count of characters in pwzOut on input.  Number of chars copies to pwzOut on output
//                         including the terminating null.
// fUTF8Enabled - [In] Flag to indicated whether UTF8 is enabled.
//
// Returns:
//    S_OK upon success.
//    E_FAIL for failure.
//    ERROR_BUFFER_OVERFLOW if the number of converted chars is greater than the passed in size of output buffer.
//
//    Note: If UTF8 is not enabled or the string does not contain UTF8 the output string will be unescaped
//    and will return S_OK.
//
HRESULT PrepareURLForDisplayUTF8W(LPCWSTR pwz, LPWSTR pwzOut, LPDWORD pcchOut, BOOL fUTF8Enabled)
{
    HRESULT hr = E_FAIL;
    DWORD   cch;
    DWORD   cch1;
    CHAR    szBuf[MAX_URL_STRING];
    CHAR    *pszBuf = szBuf;

    if(!pwz || !pwzOut || !pcchOut)
    {
        if(pcchOut)
            *pcchOut = 0;
        return(hr);
    }
        
    cch = *pcchOut;
    cch1 = ARRAYSIZE(szBuf);
    hr = UrlUnescapeW((LPWSTR)pwz, pwzOut, pcchOut, 0);
    if(SUCCEEDED(hr))
    {
        if (fUTF8Enabled && StringIsUTF8W(pwzOut, cch))
        {
            if(*pcchOut > ARRAYSIZE(szBuf)) // Internal buffer not big enough so alloc one
            {
                if((pszBuf = (CHAR *)LocalAlloc(LPTR, ((*pcchOut)+1) * sizeof(CHAR))) == NULL)
                {
                    *pcchOut = 0;
                    return(E_OUTOFMEMORY);
                }
                cch1 = *pcchOut;
            }

            // Compress wide string
            CHAR *pIn = (CHAR *)pwzOut;
            CHAR *pOut = pszBuf;
            while((*pIn != '\0') || (*(pIn+1) != '\0') && --cch1)
            {
                if(*pIn != '\0')
                {
                    *pOut = *pIn;
                    pOut++;
                }
                pIn++;
            }
            *pOut = '\0';

            // Convert to UTF8 wide string
            if((cch1 = SHAnsiToUnicodeCP(CP_UTF8, pszBuf, pwzOut, cch)) != 0)
            {
                hr = S_OK;
                *pcchOut = cch1;
            }

            // SHAnsiToUnicode doesn't tell us if it has truncated the convertion to fit the output buffer
            RIPMSG(cch1 != cch, "PrepareUrlForDisplayUTF8: Passed in size of out buf equal to converted size; buffer might be truncated");

            if((pszBuf != NULL) && (pszBuf != szBuf))
                LocalFree((CHAR *)pszBuf);
        }
        else
        {
            hr = S_OK;;
        }
    }

    return(hr);
}

//
// PrepareURLForExternalApp -
//
//   Decodes and strips, if needed, file:// prefix
//

//  BUGBUGCOMPAT - for IE30 compatibility reasons, we have to Unescape all Urls - zekel - 1-JUL-97
//  before passing them to an APP.  this does limit their use, but
//  people already depend on this behavior.  specifically MS Chat.
BOOL PrepareURLForExternalApp (LPCWSTR psz, LPWSTR pszOut, LPDWORD pcchOut)
{
    if(IsFileUrlW(psz))
        return SUCCEEDED(PathCreateFromUrl(psz, pszOut, pcchOut, 0));
    else
        return SUCCEEDED(UrlUnescape((LPWSTR)psz, pszOut, pcchOut, 0));

}


BOOL ParseURLFromOutsideSourceW (LPCWSTR psz, LPWSTR pszOut, LPDWORD pcchOut, LPBOOL pbWasSearchURL)
{
    // This is our hardest case.  Users and outside applications might
    // type fully-escaped, partially-escaped, or unescaped URLs at us.
    // We need to handle all these correctly.  This API will attempt to
    // determine what sort of URL we've got, and provide us a returned URL
    // that is guaranteed to be FULLY escaped.

    IURLQualify(psz, UQF_DEFAULT, pszOut, pbWasSearchURL, NULL);

    //
    //  go ahead and canonicalize this appropriately
    //
    if (FAILED(UrlCanonicalize(pszOut, pszOut, pcchOut, URL_ESCAPE_SPACES_ONLY)))
    {
        //
        //  we cant resize from here.
        //  NOTE UrlCan will return E_POINTER if it is an insufficient buffer
        //
        TraceMsg(DM_ERROR, "sdv PUFOS:UC() failed.");
        return FALSE;
    }

    return TRUE;
} // ParseURLFromOutsideSource
BOOL ParseURLFromOutsideSourceA (LPCSTR psz, LPSTR pszOut, LPDWORD pcchOut, LPBOOL pbWasSearchURL)
{
    SHSTRW strw;
    DWORD cch ;

    ASSERT(psz);
    ASSERT(pszOut);
    ASSERT(pcchOut && *pcchOut);

    //
    //  BUGBUG we arent guaranteed to have the correct cch's here - zekel - 27-jan-97
    //  but for now this is adequate.
    //
    if(SUCCEEDED(strw.SetStr(psz)) && SUCCEEDED(strw.SetSize(cch = *pcchOut)) &&
        ParseURLFromOutsideSourceW((LPCWSTR) strw, (LPWSTR) strw, pcchOut, pbWasSearchURL))
    {
        return SHUnicodeToAnsi((LPCWSTR)strw, pszOut, cch);
    }

    return FALSE;
}

int DPA_ILFreeCallback(void * p, void * d)
{
    Pidl_Set((LPITEMIDLIST*)&p, NULL);
    return 1;
}

void _DeletePidlDPA(HDPA hdpa)
{
    DPA_DestroyCallback(hdpa, (PFNDPAENUMCALLBACK)DPA_ILFreeCallback, 0);
}

BOOL _InitComCtl32()
{
    static BOOL fInitialized = FALSE;

    if (!fInitialized)
    {
        INITCOMMONCONTROLSEX icc;

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = ICC_USEREX_CLASSES | ICC_COOL_CLASSES | ICC_INTERNET_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_NATIVEFNTCTL_CLASS;
        fInitialized = InitCommonControlsEx(&icc);
    }
    return fInitialized;
}

#ifndef ALPHA_WARNING_IS_DUMB

#pragma message("building with alpha warning enabled")

void AlphaWarning(HWND hwnd)
{
    static BOOL fShown = FALSE;
    TCHAR szTemp[265];
    TCHAR szFull[2048];
    szFull[0] = TEXT('\0');
    int i = IDS_ALPHAWARNING;

    if (fShown)
        return;

    fShown = TRUE;

    while(MLLoadShellLangString (i++, szTemp, ARRAYSIZE(szTemp))) {
        StrCatBuff(szFull, szTemp, ARRAYSIZE(szFull));
    }

    MessageBox(hwnd, szFull, TEXT("Internet Explorer"), MB_ICONINFORMATION | MB_OK);
}
#endif


#if 0
// t-neilb: Should be able to remove
/*******************************************************************

 NAME:       InitHistoryList

 SYNOPSIS:   Reads persistent addresses from registry and puts them
             into a combo box.

********************************************************************/
BOOL InitHistoryList(HWND hwndCB)
{
    ASSERT(hwndCB); // window needs to have been created
    if (!hwndCB)
        return FALSE;

    // open the registry key containing addresses
    HKEY hKey;
    DWORD result;
    result = RegCreateKey(HKEY_CURRENT_USER, c_szRegKeyTypedURLs, &hKey);
    if (result != ERROR_SUCCESS)
        return FALSE;
    ASSERT(hKey);

    // read values from registry and put them in combo box
    UINT nTrys;
    UINT nCount = 0;
    TCHAR szValueName[10];   // big enough for "url99"
    TCHAR szAddress[MAX_URL_STRING+1];
    DWORD dwAddress;

    for (nTrys = 0; nTrys < MAX_SAVE_TYPED_URLS; nTrys++) {
        // make a value name a la "url1" (1-based for historical reasons)
        wnsprintf(szValueName, ARRAYSIZE(szValueName), TEXT("url%u"), nTrys+1);

        dwAddress = ARRAYSIZE(szAddress);
        if (RegQueryValueEx(hKey, szValueName, NULL, NULL, (LPBYTE)szAddress,
                            &dwAddress) == ERROR_SUCCESS) {
            if (szAddress[0]) {
                ComboBox_InsertString(hwndCB, nCount, szAddress);
                nCount++;
            }
        }
    }

    RegCloseKey(hKey);
    return TRUE;
}


BOOL SaveHistoryList(HWND hwndCB)
{
    ASSERT(hwndCB); // address window needs to have been created
    if (!hwndCB)
        return FALSE;

    // open the registry key containing addresses
    HKEY hKey;
    DWORD result = RegCreateKey(HKEY_CURRENT_USER, c_szRegKeyTypedURLs, &hKey);
    if (result != ERROR_SUCCESS)
        return FALSE;
    ASSERT(hKey);

    // get the number of items in combo box
    int nItems = ComboBox_GetCount(hwndCB);

    int nTrys;
    TCHAR szValueName[10];   // big enough for "url99"
    TCHAR szAddress[MAX_URL_STRING+1];

    // loop through every potential saved URL in registry.
    for (nTrys = 0; nTrys < MAX_SAVE_TYPED_URLS; nTrys++) {
        // make a value name a la "url1" (1-based for historical reasons)
        wnsprintf(szValueName,ARRAYSIZE(szValueName), TEXT("url%u"),nTrys+1);

        // for every combo box item we have, get the corresponding
        // text and save it in the registry
        if (nTrys < nItems) {
            // get text from combo box
            if (ComboBox_GetLBText(hwndCB, nTrys, szAddress)) {
                // store it in registry
                RegSetValueEx(hKey,szValueName,0,REG_SZ,(LPBYTE)
                              szAddress,(lstrlen(szAddress)*sizeof(TCHAR))+1);
                continue;
            }
        }

        // if we get here, we've run out of combo box items (or
        // failed to retrieve text for one of them).  Delete any
        // extra items that may be lingering in the registry.
        RegDeleteValue(hKey,szValueName);
    }

    RegCloseKey(hKey);

    return TRUE;
}
#endif

#ifndef POSTPOSTSPLIT

#endif

#define DM_NAV              TF_SHDNAVIGATE
#define DM_ZONE             TF_SHDNAVIGATE
#define DM_IEDDE            DM_TRACE
#define DM_CANCELMODE       0
#define DM_UIWINDOW         0
#define DM_ENABLEMODELESS   0
#define DM_EXPLORERMENU     0
#define DM_BACKFORWARD      0
#define DM_PROTOCOL         0
#define DM_ITBAR            0
#define DM_STARTUP          0
#define DM_AUTOLIFE         0
#define DM_PALETTE          0

PFNSHCHANGENOTIFYREGISTER    g_pfnSHChangeNotifyRegister = NULL;
PFNSHCHANGENOTIFYDEREGISTER  g_pfnSHChangeNotifyDeregister = NULL;

BOOL g_fNewNotify = FALSE;   // Are we using classic mode (W95 or new mode?

BOOL CALLBACK AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < MAX_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}

BOOL SHIsRegisteredClient(LPCTSTR pszClient)
{
    LONG cbSize = 0;
    TCHAR szKey[80];

    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("Software\\Clients\\%s"), pszClient);
    return (RegQueryValue(HKEY_LOCAL_MACHINE, szKey, NULL, &cbSize) == ERROR_SUCCESS) &&
           (cbSize > sizeof(TCHAR));
}

// Exporting by ordinal is not available on UNIX.
// But we have all these symbols exported because it's UNIX default.
#ifdef UNIX
#define GET_PRIVATE_PROC_ADDRESS(_hinst, _fname, _ord) GetProcAddress(_hinst, _fname)
#else
#define GET_PRIVATE_PROC_ADDRESS(_hinst, _fname, _ord) GetProcAddress(_hinst, _ord)
#endif

ULONG RegisterNotify(HWND hwnd, UINT nMsg, LPCITEMIDLIST pidl, DWORD dwEvents, UINT uFlags, BOOL fRecursive)
{
    SHChangeNotifyEntry fsne;

    // See if we need to still figure out which version of SHChange Notify to call?
    if  (g_pfnSHChangeNotifyDeregister == NULL)
    {

        HMODULE hmodShell32 = ::GetModuleHandle(TEXT("SHELL32"));
        if (!hmodShell32)
            return 0;   // Nothing registered...

        g_pfnSHChangeNotifyRegister = (PFNSHCHANGENOTIFYREGISTER)GET_PRIVATE_PROC_ADDRESS(hmodShell32,
                                                                                          "NTSHChangeNotifyRegister",
                                                                                          (LPSTR)640);
        if (g_pfnSHChangeNotifyRegister && (WhichPlatform() == PLATFORM_INTEGRATED))
        {
            g_pfnSHChangeNotifyDeregister = (PFNSHCHANGENOTIFYDEREGISTER)GET_PRIVATE_PROC_ADDRESS(hmodShell32,
                                                                                                  "NTSHChangeNotifyDeregister",
                                                                                                  (LPSTR)641);
            g_fNewNotify = TRUE;
        }
        else
        {
            g_pfnSHChangeNotifyRegister = (PFNSHCHANGENOTIFYREGISTER)GET_PRIVATE_PROC_ADDRESS(hmodShell32,
                                                                                              "SHChangeNotifyRegister",
                                                                                              (LPSTR)2);
            g_pfnSHChangeNotifyDeregister = (PFNSHCHANGENOTIFYDEREGISTER)GET_PRIVATE_PROC_ADDRESS(hmodShell32,
                                                                                                  "SHChangeNotifyDeregister",
                                                                                                  (LPSTR)4);
        }

        if  (g_pfnSHChangeNotifyDeregister == NULL)
            return 0;   // Could not get either to work...
    }

    uFlags |= SHCNRF_ShellLevel | SHCNRF_InterruptLevel;
    if (g_fNewNotify)
        uFlags |= SHCNRF_NewDelivery;

    fsne.fRecursive = fRecursive;
    fsne.pidl = pidl;
    return g_pfnSHChangeNotifyRegister(hwnd, uFlags, dwEvents, nMsg, 1, &fsne);
}

//----------------------------------------------------------------------------
// Just like shells SHRestricted() only this put up a message if the restricion
// is in effect.
// BUGBUG: this function is identical to shell32's SHIsRestricted
BOOL SHIsRestricted(HWND hwnd, RESTRICTIONS rest)
{
    if (SHRestricted(rest))
    {
        SHRestrictedMessageBox(hwnd);
        return TRUE;
    }
    return FALSE;
}

BOOL SHIsRestricted2W(HWND hwnd, BROWSER_RESTRICTIONS rest, LPCWSTR pwzUrl, DWORD dwReserved)
{
    if (SHRestricted2W(rest, pwzUrl, dwReserved))
    {
        SHRestrictedMessageBox(hwnd);
        return TRUE;
    }
    return FALSE;
}


BOOL bIsValidString(LPCSTR pszString, ULONG cbLen)
{
    if (cbLen == 0) return TRUE;
    while (cbLen--)
        if (*pszString++ == '\0') return TRUE;
    return FALSE;
}

BOOL ViewIDFromViewMode(UINT uViewMode, SHELLVIEWID *pvid)
{
    switch (uViewMode)
    {
    case FVM_ICON:
        *pvid = VID_LargeIcons;
        break;

    case FVM_SMALLICON:
        *pvid = VID_SmallIcons;
        break;

    case FVM_LIST:
        *pvid = VID_List;
        break;

    case FVM_DETAILS:
        *pvid = VID_Details;
        break;

    default:
        *pvid = VID_LargeIcons;
        return(FALSE);
    }

    return(TRUE);
}

HIMAGELIST g_himlSysSmall = NULL;
HIMAGELIST g_himlSysLarge = NULL;

void _InitSysImageLists()
{
    if (!g_himlSysSmall)
    {
        Shell_GetImageLists(&g_himlSysLarge, &g_himlSysSmall);

        ImageList_GetIconSize(g_himlSysLarge, &g_cxIcon, &g_cyIcon);
        ImageList_GetIconSize(g_himlSysSmall, &g_cxSmIcon, &g_cySmIcon);
    }
}

#define     SZAPPNAME   TEXT("Explorer")
#define     SZDEFAULT   TEXT(".Default")
void IEPlaySound(LPCTSTR pszSound, BOOL fSysSound)
{
#if !defined(UNIX)
    TCHAR szKey[256];

    // check the registry first
    // if there's nothing registered, we blow off the play,
    // but we don't set the MM_DONTLOAD flag so that if they register
    // something we will play it
    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("AppEvents\\Schemes\\Apps\\%s\\%s\\.current"),
        (fSysSound ? SZDEFAULT : SZAPPNAME), pszSound);

    TCHAR szFileName[MAX_PATH];
    szFileName[0] = 0;
    LONG cbSize = SIZEOF(szFileName);

    // note the test for an empty string, PlaySound will play the Default Sound if we
    // give it a sound it cannot find...

    if ((RegQueryValue(HKEY_CURRENT_USER, szKey, szFileName, &cbSize) == ERROR_SUCCESS)
        && cbSize && szFileName[0] != 0)
    {
        //
        // Unlike SHPlaySound in shell32.dll, we get the registry value
        // above and pass it to PlaySound with SND_FILENAME instead of
        // SDN_APPLICATION, so that we play sound even if the application
        // is not Explroer.exe (such as IExplore.exe or WebBrowserOC).
        //
        PlaySoundWrapW(szFileName, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    }
#endif //!UNIX
}



#ifndef POSTPOSTSPLIT
void* DataObj_GetDataOfType(IDataObject* pdtobj, UINT cfType, STGMEDIUM *pstg)
{
    void * pret = NULL;
    FORMATETC fmte = {cfType, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (pdtobj->GetData(&fmte, pstg) == S_OK) {
        pret = GlobalLock(pstg->hGlobal);
        if (!pret)
            ReleaseStgMedium(pstg);
    }
    return pret;
}

void ReleaseStgMediumHGLOBAL(STGMEDIUM *pstg)
{
    ASSERT(pstg->tymed == TYMED_HGLOBAL);

    GlobalUnlock(pstg->hGlobal);
    ReleaseStgMedium(pstg);
}




#endif


#ifndef POSTPOSTSPLIT
// Copied from shell32 (was _ILCreate), which does not export this.
// The fsmenu code needs this function.
STDAPI_(LPITEMIDLIST) IEILCreate(UINT cbSize)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)SHAlloc(cbSize);
    if (pidl)
        memset(pidl, 0, cbSize);      // needed for external task allicator

    return pidl;
}

#endif

DWORD CommonDragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt)
{
    DWORD dwEffect = DROPEFFECT_NONE;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (pdtobj->QueryGetData(&fmte) == S_OK)
        dwEffect = DROPEFFECT_COPY | DROPEFFECT_LINK;
    else
    {
        InitClipboardFormats();

        fmte.cfFormat = g_cfHIDA;
        if (pdtobj->QueryGetData(&fmte) == S_OK)
            dwEffect = DROPEFFECT_LINK;
        else {
            fmte.cfFormat = g_cfURL;

            if (pdtobj->QueryGetData(&fmte) == S_OK)
                dwEffect = DROPEFFECT_LINK | DROPEFFECT_COPY | DROPEFFECT_MOVE;
        }
    }

    return dwEffect;
}



// MapNbspToSp
//
// Purpose:
//     Unicode character code point 0x00a0 is designated to HTML
//     entity &nbsp, but some windows code pages don't have code
//     point that can map from 0x00a0. In the most occasion in the
//     shell, NBSP is just a space when it's rendered so we can
//     replace it with 0x0020 safely.
//     This function takes lpwszIn as a string that has
//     non-displayable characters in it, and tries to translate
//     it again after removing NBSP (00a0) from it.
//     returns S_OK if this re-translation is successful.
//
#define nbsp 0x00a0
HRESULT SHMapNbspToSp(LPCWSTR lpwszIn, LPSTR lpszOut, int cbszOut)
{
    BOOL fFoundNbsp = FALSE;
    BOOL fNotDisplayable = TRUE; // assumes FAIL
    LPWSTR pwsz, p;

    if (!lpwszIn || !lpszOut || cbszOut == 0)
        return E_FAIL;

    ASSERT(IS_VALID_STRING_PTRW(lpwszIn, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(lpszOut, TCHAR, cbszOut));

    int cch = lstrlenW(lpwszIn) + 1;
    pwsz = (LPWSTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));
    if (pwsz)
    {
        StrCpyNW(pwsz, lpwszIn, cch);
        p = pwsz;
        while (*p)
        {
            if (*p== nbsp)
            {
                *p= 0x0020; // replace with space
                if (!fFoundNbsp)
                    fFoundNbsp = TRUE;
            }
            p++;
        }

        // don't call WC2MB unless we found Nbsp - for perf reason
        if (fFoundNbsp)
        {
            int iret = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, lpszOut,
                                           cbszOut, NULL, &fNotDisplayable);

            if (!fNotDisplayable && iret == 0)
            {
                // truncated. make it dbcs safe.
                SHTruncateString(lpszOut, cbszOut);
            }
        }
        LocalFree((LOCALHANDLE)pwsz);
    }

    return (fFoundNbsp && !fNotDisplayable) ? S_OK : S_FALSE;
}
#undef nbsp


int PropBag_ReadInt4(IPropertyBag* pPropBag, LPWSTR pszKey, int iDefault)
{
    VARIANT var = {VT_I4};      // VT_I4 (not 0) so get coercion

    HRESULT hres = pPropBag->Read(pszKey, &var, NULL);
    if (hres==S_OK) {
        if (var.vt==VT_I4) {
            iDefault = var.lVal;
        }
        else {
            VariantClear(&var);
        }
    }
    return iDefault;
}

HRESULT _SetPreferedDropEffect(IDataObject *pdtobj, DWORD dwEffect)
{
    InitClipboardFormats();

    HRESULT hres = E_OUTOFMEMORY;
    DWORD *pdw = (DWORD *)GlobalAlloc(GPTR, sizeof(DWORD));
    if (pdw)
    {
        STGMEDIUM medium;
        FORMATETC fmte = {g_cfPreferedEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        *pdw = dwEffect;

        medium.tymed = TYMED_HGLOBAL;
        medium.hGlobal = pdw;
        medium.pUnkForRelease = NULL;

        hres = pdtobj->SetData(&fmte, &medium, TRUE);

        if (FAILED(hres))
            GlobalFree((HGLOBAL)pdw);
    }
    return hres;
}

#ifndef POSTPOSTSPLIT
HRESULT DragDrop(HWND hwnd, IShellFolder * psfParent, LPCITEMIDLIST pidl, DWORD dwPrefEffect, DWORD *pdwEffect)
{
    HRESULT hres = E_FAIL;
    LPCITEMIDLIST pidlChild;

    if (!psfParent)
        IEBindToParentFolder(pidl, &psfParent, &pidlChild);
    else 
    {
        pidlChild = pidl;
        psfParent->AddRef();
    }

    if (psfParent)
    {
        DWORD dwAttrib = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;

        psfParent->GetAttributesOf(1, &pidlChild, &dwAttrib);

        IDataObject *pdtobj;
        hres = psfParent->GetUIObjectOf(NULL, 1, &pidlChild, IID_IDataObject, NULL, (void**)&pdtobj);
        if (SUCCEEDED(hres))
        {
            DWORD dwEffect = (DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK) & dwAttrib;

            if (dwPrefEffect)
            {
                //win95 shell32 doesn't know about prefered drop effect, so make it the only effect
                if (IsOS(OS_WIN95) && (WhichPlatform() == PLATFORM_BROWSERONLY))
                {
                    dwEffect = DROPEFFECT_LINK & dwAttrib;
                }
                else if (dwPrefEffect & dwEffect)
                {
                    _SetPreferedDropEffect(pdtobj, dwPrefEffect);
                }
            }
            ASSERT(dwEffect);

            // Win95 Browser Only - the shell32 in this process doesn't know
            // ole is loaded, even though it is.
            SHLoadOLE(SHELLNOTIFY_OLELOADED);
            IDragSourceHelper* pDragImages;

            if (SUCCEEDED(CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IDragSourceHelper, (void**)&pDragImages)))
            {
                pDragImages->InitializeFromWindow(hwnd, 0, pdtobj);
                pDragImages->Release();
            }

            hres = SHDoDragDrop(hwnd, pdtobj, NULL, dwEffect, &dwEffect);
            if (pdwEffect)
                *pdwEffect = dwEffect;

            pdtobj->Release();
        }

        psfParent->Release();
    }

    return hres;
}
#endif

#define IEICONTYPE_GETFILEINFO              0x00000001
#define IEICONTYPE_DEFAULTICON              0x00000002

typedef struct tagIEICONS
{
    int nDefaultIcon;
    int nIEIcon;
    LPCTSTR szFile;
    LPCTSTR szFileExt;
    int nIconResourceNum;
    LPCTSTR szCLSID;
    DWORD dwType;
} IEICONS;

IEICONS g_IEIcons[] = {
    {-1, -1, TEXT("MSHTML.DLL"), TEXT(".htm"), 1, NULL, IEICONTYPE_GETFILEINFO},
    {-1, -1, TEXT("URL.DLL"), TEXT("http\\DefaultIcon"), 0, TEXT("{FBF23B42-E3F0-101B-8488-00AA003E56F8}"), IEICONTYPE_DEFAULTICON}
};

//This function returns the IE icon regardless of the which browser is  default


void _GenerateIEIcons(void)
{
    int nIndex;

    for (nIndex = 0; nIndex < ARRAYSIZE(g_IEIcons); nIndex++)
    {
        SHFILEINFO sfi;
        TCHAR szModule[MAX_PATH];

        HMODULE hmod = GetModuleHandle(g_IEIcons[nIndex].szFile);
        if (hmod)
        {
            GetModuleFileName(hmod, szModule, ARRAYSIZE(szModule));
        }
        else
        {   //HACKHACK : This is a hack to get the mstml
            TCHAR   szKey[GUIDSTR_MAX * 4];
            TCHAR   szGuid[GUIDSTR_MAX];

            //The CLSID used here belongs to MS HTML Generic Page. If someone changes the guid then we
            // are  tossed.
            if (!g_IEIcons[nIndex].szCLSID)
                SHStringFromGUID(CLSID_HTMLDocument, szGuid, GUIDSTR_MAX);
            wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("CLSID\\%s\\InProcServer32"), g_IEIcons[nIndex].szCLSID ? g_IEIcons[nIndex].szCLSID : szGuid);

            long cb = SIZEOF(szModule);
            RegQueryValue(HKEY_CLASSES_ROOT, szKey, szModule, &cb);

        }
        g_IEIcons[nIndex].nIEIcon = Shell_GetCachedImageIndex(szModule, g_IEIcons[nIndex].nIconResourceNum, 0);

        switch(g_IEIcons[nIndex].dwType)
        {
        case IEICONTYPE_GETFILEINFO:
            sfi.iIcon = 0;
            StrCpyN(szModule, TEXT("c:\\notexist"), ARRAYSIZE(szModule));
            StrCatBuff(szModule, g_IEIcons[nIndex].szFileExt, ARRAYSIZE(szModule));
            SHGetFileInfo(szModule, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
            g_IEIcons[nIndex].nDefaultIcon = sfi.iIcon;
            break;

        case IEICONTYPE_DEFAULTICON:
            {
                TCHAR szPath[MAX_PATH];
                DWORD cbSize = SIZEOF(szPath);

                SHGetValue(HKEY_CLASSES_ROOT, g_IEIcons[nIndex].szFileExt, TEXT(""), NULL, szPath, &cbSize);
                g_IEIcons[nIndex].nDefaultIcon = Shell_GetCachedImageIndex(szPath, PathParseIconLocation(szPath), 0);
            }
            break;
        }
    }
}

int IEMapPIDLToSystemImageListIndex(IShellFolder *psfParent, LPCITEMIDLIST pidlChild, int *piSelectedImage)
{
    int nIndex;
    int nIcon = SHMapPIDLToSystemImageListIndex(psfParent, pidlChild, piSelectedImage);

    if (-1 == g_IEIcons[0].nDefaultIcon)
        _GenerateIEIcons();

    for (nIndex = 0; nIndex < ARRAYSIZE(g_IEIcons); nIndex++)
    {
        if((nIcon == g_IEIcons[nIndex].nDefaultIcon) ||
            (piSelectedImage && *piSelectedImage == g_IEIcons[nIndex].nDefaultIcon))
        {
            nIcon = g_IEIcons[nIndex].nIEIcon;
            if (piSelectedImage)
                *piSelectedImage = nIcon;
            break;
        }
    }
    return nIcon;
}

void IEInvalidateImageList(void)
{
    g_IEIcons[0].nDefaultIcon = -1;
}

int _GetIEHTMLImageIndex()
{
    if (-1 == g_IEIcons[0].nDefaultIcon)
        _GenerateIEIcons();

    return g_IEIcons[0].nIEIcon;
}

// Checks to see if any process at all
// has loaded wininet
static BOOL g_fWininetLoadedSomeplace = FALSE;
BOOL IsWininetLoadedAnywhere()
{
    HANDLE hMutex = NULL;
    BOOL fRet;

    if(g_fWininetLoadedSomeplace)
        return TRUE;

    //
    // Use OpenMutexA so it works on W95.
    // wininet is ansi and created this mutex with CreateMutexA
    hMutex = OpenMutexA(SYNCHRONIZE, FALSE, WININET_STARTUP_MUTEX);

    if(hMutex)
    {
        fRet = TRUE;
        g_fWininetLoadedSomeplace = TRUE;
        CloseHandle(hMutex);
    }
    else
    {
        fRet = FALSE;
    }
    return fRet;
}



//   Checks if global state is offline
BOOL SHIsGlobalOffline(void)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;

    if(!IsWininetLoadedAnywhere())
        return FALSE;

    // Since wininet is already loaded someplace
    // We have to load wininet to check if offline

    if(InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

void SetGlobalOffline(BOOL fOffline)
{
    INTERNET_CONNECTED_INFO ci;

    memset(&ci, 0, sizeof(ci));
    if(fOffline) {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    } else {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
}

// This API is documented and is called by apps outside
// the shell such as OE
#ifdef UNIX
extern "C"
#endif
void SetShellOfflineState(BOOL fPutOffline)
{
    BOOL fWasOffline = SHIsGlobalOffline();
    if(fWasOffline != fPutOffline)
    {   
        SetGlobalOffline(fPutOffline); // Set the state
        // Tell all browser windows to update their title   
        SendShellIEBroadcastMessage(WM_WININICHANGE,0,0, 1000); 
    }
}


BOOL GetHistoryFolderPath(LPTSTR pszPath, int cchPath)
{
    INTERNET_CACHE_CONFIG_INFO cci;
    DWORD cbcci = sizeof(INTERNET_CACHE_CONFIG_INFO);

    if (GetUrlCacheConfigInfo(&cci, &cbcci, CACHE_CONFIG_HISTORY_PATHS_FC))
    {
        StrCpyN(pszPath, cci.CachePaths[0].CachePath, cchPath);
        return TRUE;
    }
    return FALSE;
}

#ifndef POSTPOSTSPLIT
// in:
//      pidlRoot    root part of pidl.
//      pidl        equal to or child below pidlRoot
//      pszKey      root key to store stuff under, should match pidlRoot
//      grfMode     read/write
//
// example:
//      pidlRoot = c:\win\favorites
//      pidl     = c:\win\favorites\channels
//      pszKey   = "MenuOrder\Favorites"
//      result -> stream comes from HKCU\...\MenuOrder\Favorites\channels
//

IStream * OpenPidlOrderStream(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidl, LPCSTR pszKey, DWORD grfMode)
{
    LPITEMIDLIST pidlAlloc = NULL;
    TCHAR   szRegPath[MAX_URL_STRING];
    TCHAR szKey[MAXIMUM_SUB_KEY_LENGTH];

    SHAnsiToTChar(pszKey, szKey, ARRAYSIZE(szKey));
    StrCpyN(szRegPath, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), ARRAYSIZE(szRegPath));
    StrCatBuff(szRegPath, szKey, ARRAYSIZE(szRegPath));

    // deal with ordinal vs true pidls
    if (HIWORD(pidlRoot) == 0)
    {
        // Sundown: coercion to int since we are assuming ordinal pidl
        SHGetSpecialFolderLocation(NULL, PtrToLong(pidlRoot), &pidlAlloc);
        pidlRoot = pidlAlloc;
    }

    // build a reg key from the names of the items below the pidlRoot folder. we do
    // this because IEGetDisplayName(SFGAO_FORPARSING) has a bug for file system
    // junctions (channel contents) that returns garbage path names.

    if (pidlRoot)
    {
        LPITEMIDLIST pidlCopy = ILClone(pidl);
        if (pidlCopy)
        {
            LPCITEMIDLIST pidlTail = ILFindChild(pidlRoot, pidlCopy);
            if (pidlTail)
            {
                LPITEMIDLIST pidlNext;
                for (pidlNext = ILGetNext(pidlTail); pidlNext; pidlNext = ILGetNext(pidlNext))
                {
                    WORD cbSave = pidlNext->mkid.cb;
                    pidlNext->mkid.cb = 0;

                    IShellFolder *psf;
                    LPCITEMIDLIST pidlChild;

                    // we do a full bind every time, we could skip this for sub items
                    // and bind from this point down but this code is simpler and binds
                    // aren't that bad...

                    if (SUCCEEDED(IEBindToParentFolder(pidlCopy, &psf, &pidlChild)))
                    {
                        STRRET sr;
                        if (SUCCEEDED(psf->GetDisplayNameOf(pidlChild, SHGDN_NORMAL, &sr)))
                        {
                            LPTSTR pszName;
                            if (SUCCEEDED(StrRetToStr(&sr, pidlChild, &pszName)))
                            {
                                StrCatBuff(szRegPath, TEXT("\\"), ARRAYSIZE(szRegPath));
                                StrCatBuff(szRegPath, pszName, ARRAYSIZE(szRegPath));
                                CoTaskMemFree(pszName);
                            }
                        }
                        psf->Release();
                    }
                    pidlNext->mkid.cb = cbSave;
                }
            }
            ILFree(pidlCopy);
        }
        if (pidlAlloc)
            ILFree(pidlAlloc);
        return SHOpenRegStream(HKEY_CURRENT_USER, szRegPath, TEXT("Order"), grfMode);
    }
    return NULL;
}
#endif

HRESULT GetHTMLElementID(IHTMLElement *pielem, LPTSTR pszName, DWORD cchSize)
{
    // only do this persistence thing if we're in ( or completing ) an operation
    BSTR bstrID = NULL;
    HRESULT hr;

    if (!pielem)
        hr = E_INVALIDARG;
    else if (SUCCEEDED(hr = pielem->get_id(&bstrID)))
    {
        SHUnicodeToTChar(bstrID, pszName, cchSize);
        SysFreeString(bstrID);
    }

    return hr;
}

/**********************************************************************
* SHRestricted2
*
* These are new restrictions that apply to browser only and integrated
* mode.  (Since we're not changing shell32 in browser only mode, we
* need to duplicate the functionality.)
*
* BUGBUG: What window will listen to the WM_WININICHANGE
*           lParam="Policy" message and invalidate the cache?
*           Remember not to cache the per zone values.
\**********************************************************************/

// The ZAW compliant policy location.
const TCHAR c_szInfodeliveryBase[] = TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery");
const TCHAR c_szInfodeliveryKey[] = TEXT("Restrictions");

// The normal policy location.
const TCHAR c_szExplorerBase[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies");
const TCHAR c_szExplorerKey[] = TEXT("Explorer");

// The browser policy location that SP2 used
const TCHAR c_szBrowserBase[] = TEXT("Software\\Policies\\Microsoft\\Internet Explorer");
const TCHAR c_szBrowserKey[]  = TEXT("Restrictions");
const TCHAR c_szToolbarKey[]  = TEXT("Toolbars\\Restrictions");

const SHRESTRICTIONITEMS c_rgRestrictionItems[] =
{
    // explorer restrictions
    { REST_NOTOOLBARCUSTOMIZE,      c_szExplorerKey,    TEXT("NoToolbarCustomize") },
    { REST_NOBANDCUSTOMIZE,         c_szExplorerKey,    TEXT("NoBandCustomize")    },
    { REST_SMALLICONS,              c_szExplorerKey,    TEXT("SmallIcons")        },
    { REST_LOCKICONSIZE,            c_szExplorerKey,    TEXT("LockIconSize")      },
    { REST_SPECIFYDEFAULTBUTTONS,   c_szExplorerKey,    TEXT("SpecifyDefaultButtons") },
    { REST_BTN_BACK,                c_szExplorerKey,    TEXT("Btn_Back")      },
    { REST_BTN_FORWARD,             c_szExplorerKey,    TEXT("Btn_Forward")   },
    { REST_BTN_STOPDOWNLOAD,        c_szExplorerKey,    TEXT("Btn_Stop")      },
    { REST_BTN_REFRESH,             c_szExplorerKey,    TEXT("Btn_Refresh")    },
    { REST_BTN_HOME,                c_szExplorerKey,    TEXT("Btn_Home")      },
    { REST_BTN_SEARCH,              c_szExplorerKey,    TEXT("Btn_Search")    },
    { REST_BTN_HISTORY,             c_szExplorerKey,    TEXT("Btn_History")   },
    { REST_BTN_FAVORITES,           c_szExplorerKey,    TEXT("Btn_Favorites") },
    { REST_BTN_ALLFOLDERS,          c_szExplorerKey,    TEXT("Btn_Folders")       },
    { REST_BTN_THEATER,             c_szExplorerKey,    TEXT("Btn_Fullscreen") },
    { REST_BTN_TOOLS,               c_szExplorerKey,    TEXT("Btn_Tools")     },
    { REST_BTN_MAIL,                c_szExplorerKey,    TEXT("Btn_MailNews")  },
    { REST_BTN_FONTS,               c_szExplorerKey,    TEXT("Btn_Size")      },
    { REST_BTN_PRINT,               c_szExplorerKey,    TEXT("Btn_Print")     },
    { REST_BTN_EDIT,                c_szExplorerKey,    TEXT("Btn_Edit")          },
    { REST_BTN_DISCUSSIONS,         c_szExplorerKey,    TEXT("Btn_Discussions")   },
    { REST_BTN_CUT,                 c_szExplorerKey,    TEXT("Btn_Cut")           },
    { REST_BTN_COPY,                c_szExplorerKey,    TEXT("Btn_Copy")          },
    { REST_BTN_PASTE,               c_szExplorerKey,    TEXT("Btn_Paste")         },
    { REST_BTN_ENCODING,            c_szExplorerKey,    TEXT("Btn_Encoding")          },
    { REST_NoUserAssist,            c_szExplorerKey,    TEXT("NoInstrumentation"),      },
    { REST_NoWindowsUpdate,         c_szExplorerKey,    TEXT("NoWindowsUpdate"),        },
    { REST_NoExpandedNewMenu,       c_szExplorerKey,    TEXT("NoExpandedNewMenu"),      },
    // ported from SP1
    { REST_NOFILEURL,               c_szExplorerKey,       TEXT("NoFileUrl"),          },
    // infodelivery restrictions
    { REST_NoChannelUI,             c_szInfodeliveryKey,   TEXT("NoChannelUI")        },
    { REST_NoAddingChannels,        c_szInfodeliveryKey,   TEXT("NoAddingChannels") },
    { REST_NoEditingChannels,       c_szInfodeliveryKey,   TEXT("NoEditingChannels") },
    { REST_NoRemovingChannels,      c_szInfodeliveryKey,   TEXT("NoRemovingChannels") },
    { REST_NoAddingSubscriptions,   c_szInfodeliveryKey,   TEXT("NoAddingSubscriptions") },
    { REST_NoEditingSubscriptions,  c_szInfodeliveryKey,   TEXT("NoEditingSubscriptions") },
    { REST_NoRemovingSubscriptions, c_szInfodeliveryKey,   TEXT("NoRemovingSubscriptions") },
    { REST_NoChannelLogging,        c_szInfodeliveryKey,   TEXT("NoChannelLogging")         },
    { REST_NoManualUpdates,         c_szInfodeliveryKey,   TEXT("NoManualUpdates")        },
    { REST_NoScheduledUpdates,      c_szInfodeliveryKey,   TEXT("NoScheduledUpdates")     },
    { REST_NoUnattendedDialing,     c_szInfodeliveryKey,   TEXT("NoUnattendedDialing")    },
    { REST_NoChannelContent,        c_szInfodeliveryKey,   TEXT("NoChannelContent")       },
    { REST_NoSubscriptionContent,   c_szInfodeliveryKey,   TEXT("NoSubscriptionContent")  },
    { REST_NoEditingScheduleGroups, c_szInfodeliveryKey,   TEXT("NoEditingScheduleGroups") },
    { REST_MaxChannelSize,          c_szInfodeliveryKey,   TEXT("MaxChannelSize")         },
    { REST_MaxSubscriptionSize,     c_szInfodeliveryKey,   TEXT("MaxSubscriptionSize")    },
    { REST_MaxChannelCount,         c_szInfodeliveryKey,   TEXT("MaxChannelCount")        },
    { REST_MaxSubscriptionCount,    c_szInfodeliveryKey,   TEXT("MaxSubscriptionCount")   },
    { REST_MinUpdateInterval,       c_szInfodeliveryKey,   TEXT("MinUpdateInterval")      },
    { REST_UpdateExcludeBegin,      c_szInfodeliveryKey,   TEXT("UpdateExcludeBegin")     },
    { REST_UpdateExcludeEnd,        c_szInfodeliveryKey,   TEXT("UpdateExcludeEnd")       },
    { REST_UpdateInNewProcess,      c_szInfodeliveryKey,   TEXT("UpdateInNewProcess")     },
    { REST_MaxWebcrawlLevels,       c_szInfodeliveryKey,   TEXT("MaxWebcrawlLevels")      },
    { REST_MaxChannelLevels,        c_szInfodeliveryKey,   TEXT("MaxChannelLevels")       },
    { REST_NoSubscriptionPasswords, c_szInfodeliveryKey,   TEXT("NoSubscriptionPasswords")},
    { REST_NoBrowserSaveWebComplete,c_szInfodeliveryKey,   TEXT("NoBrowserSaveWebComplete") },
    { REST_NoSearchCustomization,   c_szInfodeliveryKey,   TEXT("NoSearchCustomization"),  },
    { REST_NoSplash,                c_szInfodeliveryKey,   TEXT("NoSplash"),  },

    // browser restrictions ported from SP2
    { REST_NoFileOpen,              c_szBrowserKey,         TEXT("NoFileOpen"),             },
    { REST_NoFileNew,               c_szBrowserKey,         TEXT("NoFileNew"),              },
    { REST_NoBrowserSaveAs ,        c_szBrowserKey,         TEXT("NoBrowserSaveAs"),        },
    { REST_NoBrowserOptions,        c_szBrowserKey,         TEXT("NoBrowserOptions"),       },
    { REST_NoFavorites,             c_szBrowserKey,         TEXT("NoFavorites"),            },
    { REST_NoSelectDownloadDir,     c_szBrowserKey,         TEXT("NoSelectDownloadDir"),    },
    { REST_NoBrowserContextMenu,    c_szBrowserKey,         TEXT("NoBrowserContextMenu"),   },
    { REST_NoBrowserClose,          c_szBrowserKey,         TEXT("NoBrowserClose"),         },
    { REST_NoOpeninNewWnd,          c_szBrowserKey,         TEXT("NoOpeninNewWnd"),         },
    { REST_NoTheaterMode,           c_szBrowserKey,         TEXT("NoTheaterMode"),          },
    { REST_NoFindFiles,             c_szBrowserKey,         TEXT("NoFindFiles"),            },
    { REST_NoViewSource,            c_szBrowserKey,         TEXT("NoViewSource"),           },
    { REST_GoMenu,                  c_szBrowserKey,         TEXT("RestGoMenu"),             },
    { REST_NoToolbarOptions,        c_szToolbarKey,         TEXT("NoToolbarOptions"),       },
    { REST_AlwaysPromptWhenDownload,c_szBrowserKey,         TEXT("AlwaysPromptWhenDownload"),},

    { REST_NoHelpItem_TipOfTheDay,  c_szBrowserKey,         TEXT("NoHelpItemTipOfTheDay"),  },
    { REST_NoHelpItem_NetscapeHelp, c_szBrowserKey,         TEXT("NoHelpItemNetscapeHelp"), },
    { REST_NoHelpItem_Tutorial,     c_szBrowserKey,         TEXT("NoHelpItemTutorial"),     },
    { REST_NoHelpItem_SendFeedback, c_szBrowserKey,         TEXT("NoHelpItemSendFeedback"), },

    { REST_NoNavButtons,            c_szBrowserKey,         TEXT("NoNavButtons"),           },
    { REST_NoHelpMenu,              c_szBrowserKey,         TEXT("NoHelpMenu"),             },
    { REST_NoBrowserBars,           c_szBrowserKey,         TEXT("NoBrowserBars"),          },
    { REST_NoToolBar,               c_szToolbarKey,         TEXT("NoToolBar"),              },
    { REST_NoAddressBar,            c_szToolbarKey,         TEXT("NoAddressBar"),           },
    { REST_NoLinksBar,              c_szToolbarKey,         TEXT("NoLinksBar"),             },
    
    {0, NULL, NULL},
};

typedef struct {
    BROWSER_RESTRICTIONS rest;
    DWORD dwAction;    
} ACTIONITEM;

const ACTIONITEM c_ActionItems[] = {
    { REST_NoAddingChannels,        URLACTION_INFODELIVERY_NO_ADDING_CHANNELS },
    { REST_NoEditingChannels,       URLACTION_INFODELIVERY_NO_EDITING_CHANNELS },
    { REST_NoRemovingChannels,      URLACTION_INFODELIVERY_NO_REMOVING_CHANNELS },
    { REST_NoAddingSubscriptions,   URLACTION_INFODELIVERY_NO_ADDING_SUBSCRIPTIONS },
    { REST_NoEditingSubscriptions,  URLACTION_INFODELIVERY_NO_EDITING_SUBSCRIPTIONS },
    { REST_NoRemovingSubscriptions, URLACTION_INFODELIVERY_NO_REMOVING_SUBSCRIPTIONS },
    { REST_NoChannelLogging,        URLACTION_INFODELIVERY_NO_CHANNEL_LOGGING },
};

#define REST_WITHACTION_FIRST   REST_NoAddingChannels
#define REST_WITHACTION_LAST    REST_NoChannelLogging

#define RESTRICTIONMAX (c_rgRestrictionItems[ARRAYSIZE(c_rgRestrictionItems) - 1].rest)

DWORD g_rgRestrictionItemValues[ARRAYSIZE(c_rgRestrictionItems)];

DWORD SHRestricted2W(BROWSER_RESTRICTIONS rest, LPCWSTR pwzUrl, DWORD dwReserved)
{
    // Validate restriction and dwReserved
    if (dwReserved)
    {
        RIPMSG(0, "SHRestricted2W: Invalid dwReserved");
        return 0;
    }

    if (!(InRange(rest, REST_EXPLORER_FIRST, REST_EXPLORER_LAST))
        && !(InRange(rest, REST_INFO_FIRST, REST_INFO_LAST))
        && !(InRange(rest, REST_BROWSER_FIRST, REST_BROWSER_LAST)))
    {
        RIPMSG(0, "SHRestricted2W: Invalid browser restriction");
        return 0;
    }

    // See if the restriction is in place in the URL zone
    // BUGBUG: Should we assert on NULL URLs if the restriction is per zone?
    // It might be reasonable to query the global setting.
    if (pwzUrl && InRange(rest, REST_WITHACTION_FIRST, REST_WITHACTION_LAST))
    {
        // Compute the index into the table
        int index = rest - REST_WITHACTION_FIRST;

        ASSERT(c_ActionItems[index].dwAction);

        IInternetSecurityManager *pism = NULL;
        HRESULT hr;
        hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                              IID_IInternetSecurityManager, (void**)&pism);
        if (SUCCEEDED(hr) && pism)
        {
            DWORD dwPolicy = 0;
            DWORD dwContext = 0;
            hr = pism->ProcessUrlAction(pwzUrl,
                                        c_ActionItems[index].dwAction,
                                        (BYTE *)&dwPolicy,
                                        sizeof(dwPolicy),
                                        (BYTE *)&dwContext,
                                        sizeof(dwContext),
                                        PUAF_NOUI,
                                        0);
            pism->Release();
            if (SUCCEEDED(hr))
            {
                if (GetUrlPolicyPermissions(dwPolicy) == URLPOLICY_ALLOW)
                    return 0;
                else
                    return 1;    // restrict for query and disallow
            }
        }
    }

    // The cache may be invalid. Check first! We have to use
    // a global named semaphore in case this function is called
    // from a process other than the shell process. (And we're
    // sharing the same count between shell32 and shdocvw.)
    static HANDLE hRestrictions = NULL;
    static long lRestrictionCount = -1;
    if (hRestrictions == NULL)
        hRestrictions = SHGlobalCounterCreate(GUID_Restrictions);
    long lGlobalCount = SHGlobalCounterGetValue(hRestrictions);
    if (lGlobalCount != lRestrictionCount)
    {
        memset((LPBYTE)g_rgRestrictionItemValues, (BYTE)-1, SIZEOF(g_rgRestrictionItemValues));

        lRestrictionCount = lGlobalCount;
    }

    LPCWSTR pszBaseKey;
    if (InRange(rest, REST_EXPLORER_FIRST, REST_EXPLORER_LAST))
        pszBaseKey = c_szExplorerBase;
    else
    {
        if (InRange(rest, REST_BROWSER_FIRST, REST_BROWSER_LAST))
            pszBaseKey = c_szBrowserBase;
        else 
            pszBaseKey = c_szInfodeliveryBase;
    }

    return SHRestrictionLookup(rest, pszBaseKey, c_rgRestrictionItems, g_rgRestrictionItemValues);
}

DWORD SHRestricted2A(BROWSER_RESTRICTIONS rest, LPCSTR pszUrl, DWORD dwReserved)
{
    if (pszUrl)
    {
        WCHAR wzUrl[MAX_URL_STRING];

        ASSERT(ARRAYSIZE(wzUrl) > lstrlenA(pszUrl));        // We only work for Urls of MAX_URL_STRING or shorter.
        AnsiToUnicode(pszUrl, wzUrl, ARRAYSIZE(wzUrl));

        return SHRestricted2W(rest, wzUrl, dwReserved);
    }
    else
    {
        return SHRestricted2W(rest, NULL, dwReserved);
    }
}

/**********************************************************************
*
\**********************************************************************/

#define MAX_SUBSTR_SIZE     100
typedef struct tagURLSub
{
    LPCTSTR szTag;
    DWORD dwType;
} URLSUB;

const static URLSUB c_UrlSub[] = {
    {TEXT("{SUB_CLSID}"), URLSUB_CLSID},
    {TEXT("{SUB_PRD}"), URLSUB_PRD},
    {TEXT("{SUB_PVER}"), URLSUB_PVER},
    {TEXT("{SUB_OS}"), URLSUB_OS},
    {TEXT("{SUB_RFC1766}"), URLSUB_RFC1766}
};

void GetWebLocaleAsRFC1766(LPTSTR pszLocale, int cchLocale)
{
    LCID lcid;
    TCHAR szValue[MAX_PATH];

    DWORD cbVal = sizeof(szValue);
    DWORD dwType;

    ASSERT(NULL != pszLocale);

    *pszLocale = TEXT('\0');
    
    if ((SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_INTERNATIONAL,
                    REGSTR_VAL_ACCEPT_LANGUAGE, 
                    &dwType, szValue, &cbVal) == ERROR_SUCCESS) &&
        (REG_SZ == dwType))
    {
        TCHAR *psz = szValue;

        //  Use the first one we find so terminate at the comma or semicolon
        while (*psz && (*psz != TEXT(',')) && (*psz != TEXT(';')))
        {
            psz = CharNext(psz);
        }
        *psz = TEXT('\0');

        //  If it's user defined, this will fail and we will fall back
        //  to the system default.
        if (SUCCEEDED(Rfc1766ToLcid(&lcid, szValue)))
        {
            StrCpyN(pszLocale, szValue, cchLocale);
        }
    }

    if (TEXT('\0') == *pszLocale)
    {
        //  No entry in the registry or it's a user defined header.
        //  Either way we fall back to the system default.

        LcidToRfc1766(GetUserDefaultLCID(), pszLocale, cchLocale);
    }
}

HRESULT URLSubstitution(LPCWSTR pszUrlIn, LPWSTR pszUrlOut, DWORD cchSize, DWORD dwSubstitutions)
{
    HRESULT hr = S_OK;
    DWORD dwIndex;
    WCHAR szTempUrl[MAX_URL_STRING];
    ASSERT(cchSize <= ARRAYSIZE(szTempUrl));    // We will truncate anything longer than MAX_URL_STRING

    StrCpyNW(szTempUrl, pszUrlIn, ARRAYSIZE(szTempUrl));

    for (dwIndex = 0; dwIndex < ARRAYSIZE(c_UrlSub); dwIndex++)
    {
        while (IsFlagSet(dwSubstitutions, c_UrlSub[dwIndex].dwType))
        {
            LPWSTR pszTag = StrStr(szTempUrl, c_UrlSub[dwIndex].szTag);

            if (pszTag)
            {
                TCHAR szCopyUrl[MAX_URL_STRING];
                TCHAR szSubStr[MAX_SUBSTR_SIZE];  // The Substitution

                // Copy URL Before Substitution.
                StrCpyN(szCopyUrl, szTempUrl, (int)(pszTag-szTempUrl+1));
                pszTag += lstrlen(c_UrlSub[dwIndex].szTag);

                switch (c_UrlSub[dwIndex].dwType)
                {
                case URLSUB_CLSID:
                {
                    //  REVIEW (tnoonan)
                    //  Should we be using the inetcpl locale settings here?
                    LCID lcid = GetUserDefaultLCID();
                    wnsprintf(szSubStr, ARRAYSIZE(szSubStr), TEXT("%#04lx"), lcid);
                }
                    break;
                case URLSUB_PRD:
                    MLLoadString(IDS_SUBSTR_PRD, szSubStr, ARRAYSIZE(szSubStr));
                    break;
                case URLSUB_PVER:
                    MLLoadString(IDS_SUBSTR_PVER, szSubStr, ARRAYSIZE(szSubStr));
                    break;
                case URLSUB_OS:
                    if (g_bRunOnMemphis)
                    {
                        StrCpyN(szSubStr, TEXT("98"), ARRAYSIZE(szSubStr));
                    }
                    else if (g_fRunningOnNT)
                    {
                        if (g_bRunOnNT5)
                            StrCpyN(szSubStr, TEXT("N5"), ARRAYSIZE(szSubStr));
                        else
                            StrCpyN(szSubStr, TEXT("N4"), ARRAYSIZE(szSubStr));
                    }
                    else
                    {
                        StrCpyN(szSubStr, TEXT("95"), ARRAYSIZE(szSubStr));
                    }
                    break;

                case URLSUB_RFC1766:
                    GetWebLocaleAsRFC1766(szSubStr, ARRAYSIZE(szSubStr));
                    break;
                    
                default:
                    szSubStr[0] = TEXT('\0');
                    ASSERT(FALSE);  // Not Impl.
                    hr = E_NOTIMPL;
                    break;
                }
                // Add the Substitution String to the end (will become the middle)
                StrCatBuff(szCopyUrl, szSubStr, ARRAYSIZE(szCopyUrl));
                // Add the rest of the URL after the substitution substring.
                StrCatBuff(szCopyUrl, pszTag, ARRAYSIZE(szCopyUrl));
                StrCpyN(szTempUrl, szCopyUrl, ARRAYSIZE(szTempUrl));
            }
            else
                break;  // This will allow us to replace all the occurances of this string.
        }
    }
    StrCpyN(pszUrlOut, szTempUrl, cchSize);

    return hr;
}


// inetcpl.cpl uses this.
STDAPI URLSubRegQueryA(LPCSTR pszKey, LPCSTR pszValue, BOOL fUseHKCU,
                           LPSTR pszUrlOut, DWORD cchSizeOut, DWORD dwSubstitutions)
{
    HRESULT hr;
    TCHAR szKey[MAX_PATH];
    TCHAR szValue[MAX_PATH];
    TCHAR szUrlOut[MAX_URL_STRING];

    AnsiToTChar(pszKey, szKey, ARRAYSIZE(szKey));
    AnsiToTChar(pszValue, szValue, ARRAYSIZE(szValue));
    hr = URLSubRegQueryW(szKey, szValue, fUseHKCU, szUrlOut, ARRAYSIZE(szUrlOut), dwSubstitutions);
    TCharToAnsi(szUrlOut, pszUrlOut, cchSizeOut);

    return hr;
}


HRESULT URLSubRegQueryW(LPCWSTR pszKey, LPCWSTR pszValue, BOOL fUseHKCU,
                           LPWSTR pszUrlOut, DWORD cchSizeOut, DWORD dwSubstitutions)
{
    HRESULT hr = E_FAIL;
    WCHAR szTempUrl[MAX_URL_STRING];
    DWORD ccbSize = sizeof(szTempUrl);
    if (ERROR_SUCCESS == SHRegGetUSValueW(pszKey, pszValue, NULL, szTempUrl,
                                &ccbSize, !fUseHKCU, NULL, NULL))
    {
        hr = URLSubstitution(szTempUrl, pszUrlOut, cchSizeOut, dwSubstitutions);
    }

    return hr;
}

// note that anyone inside shdocvw should pass hInst==NULL to
// ensure that pluggable UI works correctly. anyone outside of shdocvw
// must pass an hInst for their appropriate resource dll
HRESULT URLSubLoadString(HINSTANCE hInst, UINT idRes, LPWSTR pszUrlOut,
                         DWORD cchSizeOut, DWORD dwSubstitutions)
{
    HRESULT hr = E_FAIL;
    WCHAR   szTempUrl[MAX_URL_STRING];
    int     nStrLen;

    nStrLen = 0;

    if (hInst == NULL)
    {
        // this is for internal users who want pluggable UI to work
        nStrLen = MLLoadStringW(idRes, szTempUrl, ARRAYSIZE(szTempUrl));
    }
    else
    {
        // this is for external users who use us to load some
        // of their own resources but whom we can't change (like shell32)
        nStrLen = LoadStringWrap(hInst, idRes, szTempUrl, ARRAYSIZE(szTempUrl));
    }

    if (nStrLen > 0)
    {
        hr = URLSubstitution(szTempUrl, pszUrlOut, cchSizeOut, dwSubstitutions);
    }

    return hr;
}


/**********************************************************************\
    FUNCTION: ILIsWeb

    DESCRIPTION:
        ILIsUrlChild() will find pidls that exist under "Desktop\The Internet"
    section of the Shell Name Space.  This function includes those items
    and file system items that have a "txt/html.
\**********************************************************************/
BOOL ILIsWeb(LPCITEMIDLIST pidl)
{
    BOOL fIsWeb = FALSE;

    if (pidl)
    {
        if (IsURLChild(pidl, TRUE))
            fIsWeb = TRUE;
        else
        {
            TCHAR szPath[MAX_PATH];

            fIsWeb = (!ILIsRooted(pidl)
            && SUCCEEDED(SHGetPathFromIDList(pidl, szPath)) 
            && (PathIsHTMLFile(szPath) ||
                 PathIsContentType(szPath, TEXT("text/xml"))));
        }
    }

    return fIsWeb;
}

//
// in:
//      pidlTo

STDAPI CreateLinkToPidl(LPCITEMIDLIST pidlTo, LPCTSTR pszDir, LPCTSTR pszTitle, LPTSTR pszOut, int cchOut)
{
    HRESULT hres = E_FAIL;
    TCHAR szPathDest[MAX_URL_STRING];
    BOOL fCopyLnk;

    if (SHGetNewLinkInfo((LPCTSTR)pidlTo, pszDir, szPathDest, &fCopyLnk, SHGNLI_PIDL))
    {
        IShellLinkA *pslA;  // Use A version for W95.
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (void**)&pslA)))
        {
            TCHAR szPathSrc[MAX_URL_STRING];
            DWORD dwAttributes = SFGAO_FILESYSTEM | SFGAO_FOLDER;
            // get source
            BOOL fPath = SUCCEEDED(SHGetNameAndFlags(pidlTo, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szPathSrc, ARRAYSIZE(szPathSrc), &dwAttributes));
            if (fCopyLnk) 
            {
                if (((dwAttributes & (SFGAO_FILESYSTEM | SFGAO_FOLDER)) == SFGAO_FILESYSTEM) && CopyFile(szPathSrc, szPathDest, TRUE))
                {
                    SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szPathDest, NULL);
                    SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, szPathDest, NULL);
                    hres = S_OK;
                }
                else
                {
                    // load the source object that will be "copied" below (with the ::Save call)
                    SAFERELEASE(pslA);
                    hres = SHGetUIObjectFromFullPIDL(pidlTo, NULL, IID_IShellLinkA, (void **)&pslA);
                    // this pslA is released at the end of the topmost if
                    if (SUCCEEDED(hres))
                    {
                        IPersistFile *ppf;
                        hres = pslA->QueryInterface(IID_IPersistFile, (void**)&ppf);
                        if (SUCCEEDED(hres))
                        {
                            hres = ppf->Save(szPathDest, TRUE);
                            ppf->Release();
                        }
                    }
                }
            } 
            else 
            {
                IPersistFile *ppf;

                pslA->SetIDList(pidlTo);

                //
                // make sure the working directory is set to the same
                // directory as the app (or document).
                //
                // dont do this for non-FS pidls (ie control panel)
                //
                // what about a UNC directory? we go ahead and set
                // it, wont work for a WIn16 app.
                //
                if (fPath && !PathIsDirectory(szPathSrc)) {
                    ASSERT(!PathIsRelative(szPathSrc));
                    PathRemoveFileSpec(szPathSrc);
                    // Try to get the W version.
                    IShellLinkW* pslW;

                    if (SUCCEEDED(pslA->QueryInterface(IID_IShellLinkW, (void**)&pslW)))
                    {
                        ASSERT(pslW);

                        pslW->SetWorkingDirectory(szPathSrc);

                        pslW->Release();
                    }
                    else
                    {
                        CHAR szPathSrcA[MAX_URL_STRING];
                        SHUnicodeToAnsi(szPathSrc, szPathSrcA, ARRAYSIZE(szPathSrcA));

                        pslA->SetWorkingDirectory(szPathSrcA);
                    }
                }

                hres = pslA->QueryInterface(IID_IPersistFile, (void **)&ppf);
                if (SUCCEEDED(hres)) {
                    WCHAR wszPath[ARRAYSIZE(szPathDest)];

                    if (pszTitle && pszTitle[0]) {
                        PathRemoveFileSpec(szPathDest);
                        PathAppend(szPathDest, pszTitle);
                        StrCatBuff(szPathDest, TEXT(".lnk"), ARRAYSIZE(szPathDest));
                    }
                    SHTCharToUnicode(szPathDest, wszPath, ARRAYSIZE(wszPath));
                    hres = ppf->Save(wszPath, TRUE);
                    if (pszOut)
                    {
                        StrCpyN(pszOut, wszPath, cchOut);
                    }
                    ppf->Release();
                }
            }

            pslA->Release();
        }
    }

    return hres;
}



#ifndef POSTPOSTSPLIT
int GetColorComponent(LPTSTR *ppsz)
{
    int iColor = 0;
    if (*ppsz) {
        LPTSTR pBuf = *ppsz;
        iColor = StrToInt(pBuf);

        // find the next comma
        while(pBuf && *pBuf && *pBuf!=TEXT(','))
            pBuf++;

        // if valid and not NULL...
        if (pBuf && *pBuf)
            pBuf++;         // increment

        *ppsz = pBuf;
    }
    return iColor;
}

// Read the registry for a string (REG_SZ) of comma separated RGB values
COLORREF RegGetColorRefString( HKEY hkey, LPTSTR RegValue, COLORREF Value)
{
    TCHAR SmallBuf[80];
    TCHAR *pBuf;
    DWORD cb;
    int iRed, iGreen, iBlue;

    cb = ARRAYSIZE(SmallBuf);
    if (RegQueryValueEx(hkey, RegValue, NULL, NULL, (LPBYTE)&SmallBuf, &cb)
        == ERROR_SUCCESS)
    {
        pBuf = SmallBuf;

        iRed = GetColorComponent(&pBuf);
        iGreen = GetColorComponent(&pBuf);
        iBlue = GetColorComponent(&pBuf);

        // make sure all values are valid
        iRed    %= 256;
        iGreen  %= 256;
        iBlue   %= 256;

        Value = RGB(iRed, iGreen, iBlue);
    }

    return Value;
}
#endif

#ifdef DEBUG // {
//***   SearchDW -- scan for DWORD in buffer
// ENTRY/EXIT
//  pdwBuf  buffer
//  cbBuf   size of buffer in *bytes* (*not* DWORDs)
//  dwVal   DWORD we're looking for
//  dOff    (return) byte offset in buffer; o.w. -1 if not found
//
int SearchDW(DWORD *pdwBuf, int cbBuf, DWORD dwVal)
{
    int dOff;

    for (dOff = 0; dOff < cbBuf; dOff += SIZEOF(DWORD), pdwBuf++) {
        if (*pdwBuf == dwVal)
            return dOff;
    }

    return -1;
}
#endif // }

// NOTE: These are directly copied from fsnotify.c in shell32. Please make sure
// NOTE: any changes are reflected there also.

// this is the NEW SHCNE_UPDATEIMAGE stuff, it passes renough data so that the recieving process
// has a vague chance that it can find the right index to refresh.
extern "C" void WINAPI _SHUpdateImageA( LPCSTR pszHashItem, int iIndex, UINT uFlags, int iImageIndex )
{
    WCHAR szWHash[MAX_PATH];

    MultiByteToWideChar( CP_ACP, 0, pszHashItem, -1, szWHash, MAX_PATH );

    _SHUpdateImageW( szWHash, iIndex, uFlags, iImageIndex );
}

extern "C" void WINAPI _SHUpdateImageW( LPCWSTR pszHashItem, int iIndex, UINT uFlags, int iImageIndex )
{
    SHChangeUpdateImageIDList rgPidl;
    SHChangeDWORDAsIDList rgDWord;

    int cLen = MAX_PATH - (lstrlenW( pszHashItem ) + 1);
    cLen *= sizeof( WCHAR );

    if ( cLen < 0 )
        cLen = 0;

    // make sure we send a valid index
    if ( iImageIndex == -1 )
        iImageIndex = II_DOCUMENT;

    rgPidl.dwProcessID = GetCurrentProcessId();
    rgPidl.iIconIndex = iIndex;
    rgPidl.iCurIndex = iImageIndex;
    rgPidl.uFlags = uFlags;
    StrCpyNW( rgPidl.szName, pszHashItem, MAX_PATH );
    rgPidl.cb = (USHORT)(sizeof( rgPidl ) - cLen);
    _ILNext( (LPITEMIDLIST) &rgPidl )->mkid.cb = 0;

    rgDWord.cb = (unsigned short) PtrDiff(&rgDWord.cbZero, &rgDWord);
    rgDWord.dwItem1 = (DWORD) iImageIndex;
    rgDWord.dwItem2 = 0;
    rgDWord.cbZero = 0;

    // pump it as an extended event
    SHChangeNotify( SHCNE_UPDATEIMAGE, SHCNF_IDLIST, &rgDWord, &rgPidl );
}

// ancient shell API wrapper...
extern "C" int _WorA_Shell_GetCachedImageIndex(LPCWSTR pszIconPath, int iIconIndex, UINT uIconFlags);

#ifndef POSTPOSTSPLIT
extern "C" int WINAPI _SHHandleUpdateImage( LPCITEMIDLIST pidlExtra )
{
    SHChangeUpdateImageIDList * pUs = (SHChangeUpdateImageIDList*) pidlExtra;

    if ( !pUs )
    {
        return -1;
    }

    // if in the same process, or an old style notification
    if ( pUs->dwProcessID == GetCurrentProcessId())
    {
        return (int) pUs->iCurIndex;
    }
    else
    {
        WCHAR szBuffer[MAX_PATH];
        int iIconIndex = *(int UNALIGNED *)((BYTE *)&pUs->iIconIndex);
        UINT uFlags = *(UINT UNALIGNED *)((BYTE *)&pUs->uFlags);

        ualstrcpynW( szBuffer, pUs->szName, ARRAYSIZE(szBuffer) );

        // we are in a different process, look up the hash in our index to get the right one...

        return _WorA_Shell_GetCachedImageIndex( szBuffer, iIconIndex, uFlags );
    }
}
#endif

HRESULT FormatUrlForDisplay(LPWSTR pwzURL, LPWSTR pwzFriendly, UINT cchBuf, BOOL fSeperate, DWORD dwCodePage)
{
    const   DWORD       dwMaxPathLen        = 32;
    const   DWORD       dwMaxHostLen        = 32;
    const   DWORD       dwMaxTemplateLen    = 64;
    const   DWORD       dwElipsisLen        = 3;
    const   CHAR        rgchElipsis[]       = "...";

    HRESULT hrRC = E_FAIL;
    HRESULT hr;

    if (pwzURL==NULL || pwzFriendly==NULL)
        return E_POINTER;

    *pwzFriendly = '\0';

    if (!*pwzURL)
        return S_OK;

    if (!cchBuf)
        return E_FAIL;

    // Wininet can't deal with code pages other than CP_ACP so convert the URL ourself and call InterCrackUrlA
    URL_COMPONENTSA urlComp;
    CHAR   rgchScheme[INTERNET_MAX_SCHEME_LENGTH];
    CHAR   rgchHostName[INTERNET_MAX_HOST_NAME_LENGTH];
    CHAR   rgchUrlPath[MAX_PATH];
    CHAR   rgchCanonicalUrl[MAX_URL_STRING];
    LPSTR  pszURL;
    DWORD  dwLen;

    if((pszURL = (LPSTR)LocalAlloc(LPTR, (cchBuf*2) * sizeof(CHAR))) != NULL)
    {
        SHUnicodeToAnsiCP(dwCodePage, pwzURL, pszURL, cchBuf);

        dwLen = ARRAYSIZE(rgchCanonicalUrl);
        hr = UrlCanonicalizeA(pszURL, rgchCanonicalUrl, &dwLen, 0);
        if (SUCCEEDED(hr))
        {
            ZeroMemory(&urlComp, sizeof(urlComp));

            urlComp.dwStructSize = sizeof(urlComp);
            urlComp.lpszHostName = rgchHostName;
            urlComp.dwHostNameLength = ARRAYSIZE(rgchHostName);
            urlComp.lpszUrlPath = rgchUrlPath;
            urlComp.dwUrlPathLength = ARRAYSIZE(rgchUrlPath);
            urlComp.lpszScheme = rgchScheme;
            urlComp.dwSchemeLength = ARRAYSIZE(rgchScheme);

            hr = InternetCrackUrlA(rgchCanonicalUrl, lstrlenA(rgchCanonicalUrl), 0, &urlComp);
            if (SUCCEEDED(hr))
            {
                DWORD dwPathLen = lstrlenA(rgchUrlPath);
                DWORD dwHostLen = lstrlenA(rgchHostName);
                DWORD dwSchemeLen = lstrlenA(rgchScheme);

                CHAR   rgchHostForDisplay[INTERNET_MAX_HOST_NAME_LENGTH];
                CHAR   rgchPathForDisplay[MAX_PATH];

                ZeroMemory(rgchHostForDisplay, sizeof(rgchHostForDisplay));
                ZeroMemory(rgchPathForDisplay, sizeof(rgchPathForDisplay));

                if (dwHostLen>dwMaxHostLen)
                {
                    DWORD   dwOverFlow = dwHostLen - dwMaxHostLen + dwElipsisLen + 1;
                    wnsprintfA(rgchHostForDisplay, ARRAYSIZE(rgchHostForDisplay), "%s%s", rgchElipsis, rgchHostName+dwOverFlow);
                    dwHostLen = dwMaxHostLen;
                }
                else
                    StrCpyNA(rgchHostForDisplay, rgchHostName, ARRAYSIZE(rgchHostForDisplay));

                if (dwPathLen>dwMaxPathLen)
                {
                    DWORD   dwOverFlow = dwPathLen - dwMaxPathLen + dwElipsisLen;
                    wnsprintfA(rgchPathForDisplay, ARRAYSIZE(rgchPathForDisplay), "/%s%s", rgchElipsis, rgchUrlPath+dwOverFlow);
                    dwPathLen = dwMaxPathLen;
                }
                else
                    StrCpyNA(rgchPathForDisplay, rgchUrlPath, ARRAYSIZE(rgchPathForDisplay));

                WCHAR   rgwchScheme[INTERNET_MAX_SCHEME_LENGTH];
                WCHAR   rgwchHostForDisplay[INTERNET_MAX_HOST_NAME_LENGTH];
                WCHAR   rgwchPathForDisplay[MAX_PATH];
                WCHAR   rgwchUrlPath[MAX_PATH];

                SHAnsiToUnicodeCP(dwCodePage, rgchScheme, rgwchScheme, ARRAYSIZE(rgwchScheme));
                SHAnsiToUnicodeCP(dwCodePage, rgchHostForDisplay, rgwchHostForDisplay, ARRAYSIZE(rgwchHostForDisplay));
                SHAnsiToUnicodeCP(dwCodePage, rgchPathForDisplay, rgwchPathForDisplay, ARRAYSIZE(rgwchPathForDisplay));
                SHAnsiToUnicodeCP(dwCodePage, rgchUrlPath, rgwchUrlPath, ARRAYSIZE(rgwchUrlPath));
                
                if (fSeperate)
                {
                    // Format string as "X from Y"
                    WCHAR   rgwchTemplate[dwMaxTemplateLen];
                    WCHAR  *pwzFileName = PathFindFileNameW(rgwchPathForDisplay);
                    DWORD   dwCount;

                    //
                    // remove cache decoration goop to map ie5setup[1].exe to ie5setup.exe
                    //
                    PathUndecorateW(pwzFileName);

                    ZeroMemory(rgwchTemplate, sizeof(rgwchTemplate));
                    dwCount = MLLoadString(IDS_TARGETFILE, rgwchTemplate, ARRAYSIZE(rgwchTemplate));
                    if (dwCount > 0)
                    {
                        if (urlComp.nScheme == INTERNET_SCHEME_FILE)
                        {
                            StrCpyNW(rgwchHostForDisplay, rgwchUrlPath, ARRAYSIZE(rgwchPathForDisplay));
                            PathRemoveFileSpecW(rgwchHostForDisplay);
                        }

                        if (dwPathLen+lstrlenW(rgwchTemplate)+dwHostLen <= cchBuf)
                        {
                            _FormatMessage(rgwchTemplate, pwzFriendly, cchBuf, pwzFileName, rgwchHostForDisplay);
                            hrRC = S_OK;
                        }
                    }
                }
                else    // !fSeperate
                {
                    if (3+dwPathLen+dwHostLen+dwSchemeLen < cchBuf)
                    {
                        wnsprintf(pwzFriendly, cchBuf, TEXT("%ws://%ws%ws"), rgwchScheme, rgwchHostForDisplay, rgwchPathForDisplay);
                        hrRC = S_OK;
                    }
                }
            }
        }

        LocalFree(pszURL);
    }
    
    return(hrRC);
}

BOOL __cdecl _FormatMessage(LPCWSTR szTemplate, LPWSTR szBuf, UINT cchBuf, ...)
{
    BOOL fRet;
    va_list ArgList;
    va_start(ArgList, cchBuf);

    fRet = FormatMessage(FORMAT_MESSAGE_FROM_STRING, szTemplate, 0, 0, szBuf, cchBuf, &ArgList);

    va_end(ArgList);
    return fRet;
}


// Navigate to a given Url (wszUrl) using IE. Returns an error if IE does not exist.
// fNewWindow = TRUE ==> A new window is compulsory
// fNewWindow = FALSE ==> Do not launch a new window if one already is open.
HRESULT NavToUrlUsingIEW(LPCWSTR wszUrl, BOOL fNewWindow)
{
    HRESULT hr = S_OK;

    if (!EVAL(wszUrl))
        return E_INVALIDARG;

    if(IsIEDefaultBrowser() && !fNewWindow)
    {
        // ShellExecute navigates to the Url using the same browser window,
        // if one is already open.

        SHELLEXECUTEINFOW sei = {0};

        sei.cbSize = sizeof(sei);
        sei.lpFile = wszUrl;
        sei.nShow  = SW_SHOWNORMAL;

        ShellExecuteExW(&sei);

    }
    else
    {
        IWebBrowser2 *pwb2;
#ifndef UNIX
        hr = CoCreateInstance(CLSID_InternetExplorer, NULL,
                              CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (LPVOID*)&pwb2);
#else
        hr = CoCreateInternetExplorer( IID_IWebBrowser2,
                                       CLSCTX_LOCAL_SERVER,
                                       (LPVOID*) &pwb2 );
#endif
        if(SUCCEEDED(hr))
        {
            SA_BSTR sstrURL;
            StrCpyNW(sstrURL.wsz, wszUrl, ARRAYSIZE(sstrURL.wsz));
            sstrURL.cb = lstrlenW(sstrURL.wsz) * SIZEOF(WCHAR);

            VARIANT varURL;
            varURL.vt = VT_BSTR;
            varURL.bstrVal = sstrURL.wsz;

            VARIANT varFlags;
            varFlags.vt = VT_I4;
            varFlags.lVal = 0;

            hr = pwb2->Navigate2(&varURL, &varFlags, PVAREMPTY, PVAREMPTY, PVAREMPTY);
            ASSERT(SUCCEEDED(hr)); // mikesh sez there's no way for Navigate2 to fail
            hr = pwb2->put_Visible( TRUE );
            pwb2->Release();
        }
    }
    return hr;
}

HRESULT NavToUrlUsingIEA(LPCSTR szUrl, BOOL fNewWindow)
{
    WCHAR   wszUrl[INTERNET_MAX_URL_LENGTH];

    AnsiToUnicode(szUrl, wszUrl, ARRAYSIZE(wszUrl));

    return NavToUrlUsingIEW(wszUrl, fNewWindow);
}

STDAPI_(BSTR) SysAllocStringA(LPCSTR pszAnsiStr)
{
    OLECHAR *bstrOut = NULL;
    LPWSTR pwzTemp;
    UINT cchSize = (lstrlenA(pszAnsiStr) + 2);  // Count of characters

    if (!pszAnsiStr)
        return NULL;    // What the hell do you expect?

    pwzTemp = (LPWSTR) LocalAlloc(LPTR, cchSize * sizeof(WCHAR));
    if (pwzTemp)
    {
        SHAnsiToUnicode(pszAnsiStr, pwzTemp, cchSize);
        bstrOut = SysAllocString(pwzTemp);
        LocalFree(pwzTemp);
    }

    return bstrOut;
}


// MultiByteToWideChar doesn't truncate if the buffer is too small.
// these utils do.
// returns:
//      # of chars converted (WIDE chars) into out buffer (pwstr)

int _AnsiToUnicode(UINT uiCP, LPCSTR pstr, LPWSTR pwstr, int cch)
{
    int cchDst = 0;

    ASSERT(IS_VALID_STRING_PTRA(pstr, -1));
    ASSERT(NULL == pwstr || IS_VALID_WRITE_BUFFER(pwstr, WCHAR, cch));

    if (cch && pwstr)
        pwstr[0] = 0;

    switch (uiCP)
    {
        case 1200:                      // UCS-2 (Unicode)
            uiCP = 65001;
            // fall through
        case 50000:                     // "User Defined"
        case 65000:                     // UTF-7
        case 65001:                     // UTF-8
        {
            INT cchSrc, cchSrcOriginal;

            cchSrc = cchSrcOriginal = lstrlenA(pstr) + 1;
            cchDst = cch;

            if (SUCCEEDED(ConvertINetMultiByteToUnicode(NULL, uiCP, pstr,
                &cchSrc, pwstr, &cchDst)) &&
                cchSrc < cchSrcOriginal)
            {
                LPWSTR pwsz = (LPWSTR)LocalAlloc(LPTR, cchDst * SIZEOF(WCHAR));
                if (pwsz)
                {
                    if (SUCCEEDED(ConvertINetMultiByteToUnicode( NULL, uiCP, pstr,
                        &cchSrcOriginal, pwsz, &cchDst )))
                    {
                        StrCpyNW( pwstr, pwsz, cch );
                        cchDst = cch;
                    }
                    LocalFree(pwsz);
                }
            }
            break;
        }

        default:
            cchDst = MultiByteToWideChar(uiCP, 0, pstr, -1, pwstr, cch);
            if (!cchDst) {

                // failed.

                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    int cchNeeded = MultiByteToWideChar(uiCP, 0, pstr, -1, NULL, 0);

                    if (cchNeeded) {
                        LPWSTR pwsz = (LPWSTR)LocalAlloc(LPTR, cchNeeded * SIZEOF(WCHAR));
                        if (pwsz) {
                            cchDst = MultiByteToWideChar(uiCP, 0, pstr, -1, pwsz, cchNeeded);
                            if (cchDst) {
                                StrCpyNW(pwstr, pwsz, cch);
                                cchDst = cch;
                            }
                            LocalFree(pwsz);
                        }
                    }
                }
            }
            break;
    }
    return cchDst;
}


#ifndef POSTPOSTSPLIT
HRESULT ContextMenu_GetCommandStringVerb(IContextMenu *pcm, UINT idCmd, LPTSTR pszVerb, int cchVerb)
{
    HRESULT hres = E_FAIL;

    // Try the WCHAR string first on NT systems
    if (g_fRunningOnNT)
    {
        hres = pcm->GetCommandString(idCmd, GCS_VERBW, NULL, (LPSTR)pszVerb, cchVerb);
    }

    // Try the ANSI string next
    if (FAILED(hres))
    {
        char szVerbAnsi[80];
        hres = pcm->GetCommandString(idCmd, GCS_VERBA, NULL, szVerbAnsi, ARRAYSIZE(szVerbAnsi));
        SHAnsiToTChar(szVerbAnsi, pszVerb, cchVerb);
    }
    return hres;
}
#endif

UINT    g_cfURL = 0;
UINT    g_cfFileDescA = 0;
UINT    g_cfFileContents = 0;
UINT    g_cfPreferedEffect = 0;

#ifndef POSTPOSTSPLIT
UINT    g_cfHIDA = 0;
UINT    g_cfFileDescW = 0;
#endif

void InitClipboardFormats()
{
    if (g_cfURL == 0)
    {
        g_cfURL = RegisterClipboardFormat(CFSTR_SHELLURL);
        g_cfFileDescA = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
        g_cfFileContents = RegisterClipboardFormat(CFSTR_FILECONTENTS);
        g_cfPreferedEffect = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
#ifndef POSTPOSTSPLIT
        g_cfHIDA = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
        g_cfFileDescW = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
#endif
    }
}


// BUGBUG [raymondc] use SHGlobalCounter

// We need to use a cross process browser count.
// We use a named semaphore.
//
EXTERN_C HANDLE g_hSemBrowserCount = NULL;

#define SESSION_COUNT_SEMAPHORE_NAME "_ie_sessioncount"

HANDLE GetSessionCountSemaphoreHandle()
{
    if (!g_hSemBrowserCount)
    {
        //
        // Use the "A" version so it works on W95 w/o a wrapper.
        //
        // Too afraid to use CreateAllAccessSecurityAttributes because
        // who knows what'll happen when two people with different
        // security attributes try to access the same winlist...
        g_hSemBrowserCount = CreateSemaphoreA(NULL, 0, 0x7FFFFFFF, SESSION_COUNT_SEMAPHORE_NAME);
        if (!g_hSemBrowserCount)
        {
            ASSERT(GetLastError() == ERROR_ALREADY_EXISTS);
            g_hSemBrowserCount = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS,
                                 FALSE, SESSION_COUNT_SEMAPHORE_NAME);
        }
    }
    return g_hSemBrowserCount;
}

LONG GetSessionCount()
{
    LONG lPrevCount = 0x7FFFFFFF;
    HANDLE hSem = GetSessionCountSemaphoreHandle();

    ASSERT(hSem);
    if(hSem)
    {
        ReleaseSemaphore(hSem, 1, &lPrevCount);
        WaitForSingleObject(hSem, 0);
    }
    return lPrevCount;


}

LONG IncrementSessionCount()
{
    LONG lPrevCount = 0x7FFFFFFF;
    HANDLE hSem = GetSessionCountSemaphoreHandle();

    ASSERT(hSem);
    if(hSem)
    {
        ReleaseSemaphore(hSem, 1, &lPrevCount);
    }
    return lPrevCount;
}

LONG DecrementSessionCount()
{
    LONG lPrevCount = 0x7FFFFFFF;
    HANDLE hSem = GetSessionCountSemaphoreHandle();
    ASSERT(hSem);
    if(hSem)
    {
        ReleaseSemaphore(hSem, 1, &lPrevCount); // increment first to make sure deadlock
                                                 // never occurs
        ASSERT(lPrevCount > 0);
        if(lPrevCount > 0)
        {
            WaitForSingleObject(hSem, 0);
            WaitForSingleObject(hSem, 0);
            lPrevCount--;
        }
        else
        {
            // Oops - Looks like a bug !
            // Just return it back to normal and leave
            WaitForSingleObject(hSem, 0);
        }
    }
    return lPrevCount;
}




//
// The following is the message that autodial monitors expect to receive
// when it's a good time to hang up
//
#define WM_IEXPLORER_EXITING    (WM_USER + 103)

long SetQueryNetSessionCount(enum SessionOp Op)
{
    long lCount = 0;
    
    switch(Op) {
        case SESSION_QUERY:
            lCount = GetSessionCount();
            TraceMsg(DM_SESSIONCOUNT, "SetQueryNetSessionCount SessionCount=%d (query)", lCount);
            break;

        case SESSION_INCREMENT_NODEFAULTBROWSERCHECK:
        case SESSION_INCREMENT:
            lCount = IncrementSessionCount();
            TraceMsg(DM_SESSIONCOUNT, "SetQueryNetSessionCount SessionCount=%d (incr)", lCount);

            
            if ((PLATFORM_INTEGRATED == WhichPlatform()))
            {
                // Weird name here... But in integrated mode we make every new browser window
                // look like a new session wrt how we use the cache. Basically this is the way things appear to the
                // user. This effects the way we look for new pages vs doing an if modified
                // since.  The ie3/ie4 switch says "look for new pages on each session start"
                // but wininet folks implemented this as a end session name. Woops.
                // Note that things like authentication etc aren't reset by this, but rather
                // only when all browsers are closed via the INTERNET_OPTION_END_BROWSER_SESSION option.
                InternetSetOption(NULL, INTERNET_OPTION_RESET_URLCACHE_SESSION, NULL, 0);
            }

            if (!lCount && (Op == SESSION_INCREMENT))
            {
                // this forces a reload of the title
                DetectAndFixAssociations();
            }
            break;

        case SESSION_DECREMENT:
            lCount = DecrementSessionCount();
            TraceMsg(DM_SESSIONCOUNT, "SetQueryNetSessionCount SessionCount=%d (decr)", lCount);

            if(!lCount) {
                // if we've closed all the net browsers, we need to flush the cache
                InternetSetOption(NULL, INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0);
                InternetSetOption(NULL, INTERNET_OPTION_RESET_URLCACHE_SESSION, NULL, 0);

                // flush the Java VM cache too (if the Java VM is loaded in this process
                // and we're in integrated mode)
                if (WhichPlatform() == PLATFORM_INTEGRATED)
                {
                    HMODULE hmod = GetModuleHandle(TEXT("msjava.dll"));
                    if (hmod)
                    {
                        typedef HRESULT (*PFNNOTIFYBROWSERSHUTDOWN)(LPVOID);
                        FARPROC fp = GetProcAddress(hmod, "NotifyBrowserShutdown");
                        if (fp)
                        {
                            HRESULT hr = ((PFNNOTIFYBROWSERSHUTDOWN)fp)(NULL);
                            ASSERT(SUCCEEDED(hr));
                        }
                    }
                }

                // Inform dial monitor that it's a good time to hang up
                HWND hwndMonitorWnd = FindWindow(TEXT("MS_AutodialMonitor"),NULL);
                if (hwndMonitorWnd) {
                    PostMessage(hwndMonitorWnd,WM_IEXPLORER_EXITING,0,0);
                }
                hwndMonitorWnd = FindWindow(TEXT("MS_WebcheckMonitor"),NULL);
                if (hwndMonitorWnd) {
                    PostMessage(hwndMonitorWnd,WM_IEXPLORER_EXITING,0,0);
                }

                // reset offline mode on all platforms except Win2K.
                OSVERSIONINFOA vi;
                vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
                GetVersionExA(&vi);
                if( vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ||
                    vi.dwMajorVersion < 5)
                {
                    // wininet is loaded - tell it to go online
                    INTERNET_CONNECTED_INFO ci;
                    memset(&ci, 0, sizeof(ci));
                    ci.dwConnectedState = INTERNET_STATE_CONNECTED;
                    InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
                }
            }
            break;
    }

    return lCount;
}


#ifdef DEBUG
//---------------------------------------------------------------------------
// Copy the exception info so we can get debug info for Raised exceptions
// which don't go through the debugger.
void _CopyExceptionInfo(LPEXCEPTION_POINTERS pep)
{
    PEXCEPTION_RECORD per;

    per = pep->ExceptionRecord;
    TraceMsg(DM_ERROR, "Exception %x at %#08x.", per->ExceptionCode, per->ExceptionAddress);

    if (per->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        // If the first param is 1 then this was a write.
        // If the first param is 0 then this was a read.
        if (per->ExceptionInformation[0])
        {
            TraceMsg(DM_ERROR, "Invalid write to %#08x.", per->ExceptionInformation[1]);
        }
        else
        {
            TraceMsg(DM_ERROR, "Invalid read of %#08x.", per->ExceptionInformation[1]);
        }
    }
}
#else
#define _CopyExceptionInfo(x) TRUE
#endif


int WELCallback(LPVOID p, LPVOID pData)
{
    STATURL* pstat = (STATURL*)p;
    if (pstat->pwcsUrl) {
        OleFree(pstat->pwcsUrl);
    }
    return 1;
}

int CALLBACK WELCompare(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    HDSA hdsa = (HDSA)lParam;
    // Sundown: coercion to long because parameter is an index
    STATURL* pstat1 = (STATURL*)DSA_GetItemPtr(hdsa, PtrToLong(p1));
    STATURL* pstat2 = (STATURL*)DSA_GetItemPtr(hdsa, PtrToLong(p2));
    if (pstat1 && pstat2) {
        return CompareFileTime(&pstat2->ftLastVisited, &pstat1->ftLastVisited);
    }

    ASSERT(0);
    return 0;
}

#define MACRO_STR(x) #x
#define VERSION_HEADER_STR "Microsoft Internet Explorer 5.0 Error Log -- " \
                           MACRO_STR(VER_MAJOR_PRODUCTVER) "." \
                           MACRO_STR(VER_MINOR_PRODUCTVER) "." \
                           MACRO_STR(VER_PRODUCTBUILD) "." \
                           MACRO_STR(VER_PRODUCTBUILD_QFE) "\r\n"

void IEWriteErrorLog(const EXCEPTION_RECORD* pexr)
{
    HANDLE hfile = INVALID_HANDLE_VALUE;
    _try
    {
        //
        // Post a message to the MTTF tool if running
        //
        PostMTTFMessage(MTTF_CRASHTRAPPED);

        TCHAR szWindows[MAX_PATH];
        GetWindowsDirectory(szWindows, ARRAYSIZE(szWindows));
        PathAppend(szWindows, TEXT("IE4 Error Log.txt"));
        HANDLE hfile = CreateFile(szWindows, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if(hfile != INVALID_HANDLE_VALUE)
        {
            const static CHAR c_szCRLF[] = "\r\n";
            DWORD cbWritten;
            CHAR szBuf[MAX_URL_STRING];

            // Write the title and product version.
            WriteFile(hfile, VERSION_HEADER_STR, lstrlenA(VERSION_HEADER_STR), &cbWritten, NULL);

            // Write the current time.
            SYSTEMTIME st;
            FILETIME ft;
            GetSystemTime(&st);
            SystemTimeToFileTime(&st, &ft);
            SHFormatDateTimeA(&ft, NULL, szBuf, SIZECHARS(szBuf));
            const static CHAR c_szCurrentTime[] = "CurrentTime: ";
            WriteFile(hfile, c_szCurrentTime, SIZEOF(c_szCurrentTime)-1, &cbWritten, NULL);
            WriteFile(hfile, szBuf, lstrlenA(szBuf), &cbWritten, NULL);
            WriteFile(hfile, c_szCRLF, SIZEOF(c_szCRLF)-1, &cbWritten, NULL);

            if (pexr) {
                const static CHAR c_szExcCode[] = "Exception Info: Code=%x Flags=%x Address=%x\r\n";
                const static CHAR c_szExcParam[] = "Exception Param:";
                wnsprintfA(szBuf, ARRAYSIZE(szBuf), c_szExcCode, pexr->ExceptionCode, pexr->ExceptionFlags, pexr->ExceptionAddress);
                WriteFile(hfile, szBuf, lstrlenA(szBuf), &cbWritten, NULL);

                if (pexr->NumberParameters) {
                    WriteFile(hfile, c_szExcParam, SIZEOF(c_szExcParam)-1, &cbWritten, NULL);
                    for (UINT iParam=0; iParam<pexr->NumberParameters; iParam++) {
                        wnsprintfA(szBuf, ARRAYSIZE(szBuf), " %x", pexr->ExceptionInformation[iParam]);
                        WriteFile(hfile, szBuf, lstrlenA(szBuf), &cbWritten, NULL);
                    }
                }

                WriteFile(hfile, c_szCRLF, SIZEOF(c_szCRLF)-1, &cbWritten, NULL);
                WriteFile(hfile, c_szCRLF, SIZEOF(c_szCRLF)-1, &cbWritten, NULL);
            }

            IUrlHistoryStg* pUrlHistStg;
            HRESULT hres = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER,
                IID_IUrlHistoryStg, (void **)&pUrlHistStg);

            if (SUCCEEDED(hres)) {
                IEnumSTATURL* penum;
                hres = pUrlHistStg->EnumUrls(&penum);
                if (SUCCEEDED(hres)) {
                    // Allocate DSA for an array of STATURL
                    HDSA hdsa = DSA_Create(SIZEOF(STATURL), 32);
                    if (hdsa) {
                        // Allocate DPA for sorting
                        HDPA hdpa = DPA_Create(32);
                        if (hdpa) {
                            STATURL stat;
                            stat.cbSize = SIZEOF(stat.cbSize);
                            while(penum->Next(1, &stat, NULL)==S_OK && stat.pwcsUrl) {
                                DSA_AppendItem(hdsa, &stat);
                                DPA_AppendPtr(hdpa, (LPVOID)(DSA_GetItemCount(hdsa)-1));
                            }

                            DPA_Sort(hdpa, WELCompare, (LPARAM)hdsa);
                            for (int i=0; i<10 && i<DPA_GetPtrCount(hdpa) ; i++) {
                                // Sundown: typecast to long is OK
                                STATURL* pstat = (STATURL*)DSA_GetItemPtr(hdsa, PtrToLong(DPA_GetPtr(hdpa, i)));
                                if (pstat && pstat->pwcsUrl) {

                                    SHFormatDateTimeA(&pstat->ftLastVisited, NULL, szBuf, SIZECHARS(szBuf));
                                    WriteFile(hfile, szBuf, lstrlenA(szBuf), &cbWritten, NULL);
                                    const static TCHAR c_szColumn[] = TEXT(" -- ");
                                    WriteFile(hfile, c_szColumn, SIZEOF(c_szColumn)-1, &cbWritten, NULL);

                                    WideCharToMultiByte(CP_ACP, 0, pstat->pwcsUrl, -1,
                                                        szBuf, ARRAYSIZE(szBuf), NULL, NULL);
                                    WriteFile(hfile, szBuf, lstrlenA(szBuf), &cbWritten, NULL);

                                    WriteFile(hfile, c_szCRLF, SIZEOF(c_szCRLF)-1, &cbWritten, NULL);
                                } else {
                                    ASSERT(0);
                                }
                            }

                            DPA_Destroy(hdpa);
                        }
                        DSA_DestroyCallback(hdsa, WELCallback, NULL);
                    }
                    penum->Release();
                } else {
                    ASSERT(0);
                }
                pUrlHistStg->Release();
            } else {
                ASSERT(0);
            }

            CloseHandle( hfile );
            hfile = INVALID_HANDLE_VALUE;
        }
    }
    _except((SetErrorMode(SEM_NOGPFAULTERRORBOX),
            _CopyExceptionInfo(GetExceptionInformation()),
            UnhandledExceptionFilter(GetExceptionInformation())
            ))
    {
        // We hit an exception while handling an exception.
        // Do nothing; we have already displayed the error dialog box.
        if(hfile != INVALID_HANDLE_VALUE) {
            CloseHandle(hfile);
        }
    }
    __endexcept
}


int CDECL MRUILIsEqual(const void *pidl1, const void *pidl2, size_t cb)
{
    // First cheap hack to see if they are 100 percent equal for performance
    int iCmp = memcmp(pidl1, pidl2, cb);

    if (iCmp == 0)
        return 0;

    if (ILIsEqual((LPITEMIDLIST)pidl1, (LPITEMIDLIST)pidl2))
        return 0;

    return iCmp;
}


IStream* SHGetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCTSTR pszName, LPCTSTR pszStreamMRU, LPCTSTR pszStreams)
{
    IStream *pstm = NULL;
    HANDLE hmru;
    int iFoundSlot = -1;
    UINT cbPidl;
    static DWORD s_dwMRUSize = 0;
    DWORD dwSize = sizeof(s_dwMRUSize);

    if ((0 == s_dwMRUSize) &&
        (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, pszStreamMRU, TEXT("MRU Size"), NULL, (LPVOID) &s_dwMRUSize, &dwSize)))
    {
        s_dwMRUSize = 200;          // The default.
    }

    MRUINFO mi = {
        SIZEOF(MRUINFO),
        s_dwMRUSize,                     // we store this many view streams
        MRU_BINARY,
        HKEY_CURRENT_USER,
        pszStreamMRU,
        (MRUCMPPROC)MRUILIsEqual,
    };

    ASSERT(pidl);

    // should be checked by caller - if this is not true we'll flush the
    // MRU cache with internet pidls!  FTP and other URL Shell Extension PIDLs
    // that act like a folder and need similar persistence and fine.  This
    // is especially true because recently the cache size was increased from
    // 30 or so to 200.
    ASSERT(ILIsEqual(pidl, c_pidlURLRoot) || !IsBrowserFrameOptionsPidlSet(pidl, BFO_BROWSER_PERSIST_SETTINGS));

    cbPidl = ILGetSize(pidl);

    // Now lets try to save away the other information associated with view.
    hmru = CreateMRUListLazyEx(&mi, pidl, cbPidl, &iFoundSlot);
    if (!hmru)
        return NULL;

    // Did we find the item?
    if (iFoundSlot < 0 && ((grfMode & (STGM_READ|STGM_WRITE|STGM_READWRITE)) == STGM_READ))
    {
        // Do not  create the stream if it does not exist and we are
        // only reading
    }
    else
    {
        HKEY hkCabStreams;
        TCHAR szRegPath[MAX_PATH];

        StrCpyN(szRegPath, REGSTR_PATH_EXPLORER TEXT("\\"), ARRAYSIZE(szRegPath));
        StrCatBuff(szRegPath, pszStreams, ARRAYSIZE(szRegPath));

        // Note that we always create the key here, since we have
        // already checked whether we are just reading and the MRU
        // thing does not exist
        if (RegCreateKey(HKEY_CURRENT_USER, szRegPath, &hkCabStreams) == ERROR_SUCCESS)
        {
            HKEY hkValues;
            TCHAR szValue[32], szSubVal[64];

            wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%d"), AddMRUDataEx(hmru, pidl, cbPidl));

            if (iFoundSlot < 0 && RegOpenKey(hkCabStreams, szValue, &hkValues) == ERROR_SUCCESS)
            {
                // This means that we have created a new MRU
                // item for this PIDL, so clear out any
                // information residing at this slot
                // Note that we do not just delete the key,
                // since that could fail if it has any sub-keys
                DWORD dwType, dwSize = ARRAYSIZE(szSubVal);

                while (RegEnumValue(hkValues, 0, szSubVal, &dwSize, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
                {
                    if (RegDeleteValue(hkValues, szSubVal) != ERROR_SUCCESS)
                    {
                        break;
                    }
                }

                RegCloseKey(hkValues);
            }
            pstm = OpenRegStream(hkCabStreams, szValue, pszName, grfMode);
            RegCloseKey(hkCabStreams);
        }
    }

    FreeMRUListEx(hmru);

    return pstm;
}


#define c_szExploreClass TEXT("ExploreWClass")
#define c_szIExploreClass TEXT("IEFrame")
#ifdef IE3CLASSNAME
#define c_szCabinetClass TEXT("IEFrame")
#else
#define c_szCabinetClass TEXT("CabinetWClass")
#endif


BOOL IsNamedWindow(HWND hwnd, LPCTSTR pszClass)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return StrCmp(szClass, pszClass) == 0;
}

BOOL IsTrayWindow(HWND hwnd)
{
    return IsNamedWindow(hwnd, TEXT(WNDCLASS_TRAYNOTIFY));
}

BOOL IsExplorerWindow(HWND hwnd)
{
    return IsNamedWindow(hwnd, c_szExploreClass);
}

BOOL IsFolderWindow(HWND hwnd)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return (StrCmp(szClass, c_szCabinetClass) == 0) || (StrCmp(szClass, c_szIExploreClass) == 0);
}

HRESULT _SendOrPostDispatchMessage(HWND hwnd, WPARAM wParam, LPARAM lParam, BOOL fPostMessage, BOOL fCheckFirst)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_BUSY);
    DWORD idProcess;

    // in case of wParam = DSID_NAVIGATEIEBROWSER, lParam is LocalAlloced structure
    // so we better make sure we are in process 'coz otherwise will fault
    GetWindowThreadProcessId(hwnd, &idProcess);
    if (idProcess == GetCurrentProcessId() && IsWindowEnabled(hwnd) && IsWindowVisible(hwnd))
    {
        if (!fPostMessage || fCheckFirst)
        {
            //  sync or we are querying the windows readiness
            ULONG_PTR result;
            if (SendMessageTimeoutA(hwnd, WMC_DISPATCH, (fCheckFirst ? DSID_NOACTION : wParam),
                lParam, SMTO_ABORTIFHUNG, 400, &result))
                hr = (HRESULT) result;
        }

        //  handle the post only if the window was ready
        if (fPostMessage && (!fCheckFirst || SUCCEEDED(hr)))
            hr = (PostMessage(hwnd, WMC_DISPATCH, wParam, lParam) ? S_OK : E_FAIL);
    }

    return hr;
}

//---------------------------------------------------------------------------

HRESULT FindBrowserWindowOfClass(LPCTSTR pszClass, WPARAM wParam, LPARAM lParam, BOOL fPostMessage, HWND* phwnd)
{
    //If there is no window, assume the user is in the process of shutting down IE, and return E_FAIL

    //Otherwise, if there is at least one window, start cycling through the windows until you find
    //one that's not busy, and give it our message.  If all are busy, return
    //HRESULT_FROM_WIN32(ERROR_BUSY)
    HWND hwnd = NULL;
    HRESULT hr = E_FAIL;

    while (FAILED(hr)
        && (hwnd = FindWindowEx(NULL, hwnd, pszClass, NULL)) != NULL)
    {
        hr = _SendOrPostDispatchMessage(hwnd, wParam, lParam, fPostMessage, fPostMessage);
    }

    *phwnd = hwnd;
    return hr;
}

//This common function gets called when the DDE engine doesn't seem to care in which window something
//happens.  It returns in which window that something happened.  0 means all windows are busy.
//
//phwnd: a pointer the hwnd to which to send the message.  <= 0 means any window will do.
//       this is also an out parameter that specifies in which window it happened.
//fPostMessage: when doing navigations, we have to do a PostMessage instead of a SendMessageTimeout
//       or a CoCreateInstance later on in CDocObjectHost::_BindFileMoniker will fail.  So when
//       this function is called from CDDEAuto_Navigate, we make this flag TRUE
HRESULT CDDEAuto_Common(WPARAM wParam, LPARAM lParam, HWND *phwnd, BOOL fPostMessage)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_BUSY);
    HWND hwnd;

    //if we're told to go to a specific window
    if (phwnd && (*phwnd != (HWND)-1))
    {
        hr = _SendOrPostDispatchMessage(*phwnd, wParam, lParam, fPostMessage, FALSE);
    }

    if (HRESULT_FROM_WIN32(ERROR_BUSY) == hr)
    {
        hr = FindBrowserWindowOfClass(c_szIExploreClass, wParam, lParam, fPostMessage, &hwnd);
        if (!hwnd)
            hr = FindBrowserWindowOfClass(c_szCabinetClass, wParam, lParam, fPostMessage, &hwnd);

        if (phwnd)
            *phwnd = hwnd;
    }
    return hr;
}

//
//  Before changing the behavior of this function look at itemmenu.cpp in
//  cdfview.
//
HRESULT CDDEAuto_Navigate(BSTR str, HWND *phwnd, long) //the long is for lTransID, which is currently
                                                       //turned off
{
    DDENAVIGATESTRUCT *pddens = NULL;
    HRESULT hres = E_FAIL;

    if (phwnd == NULL)
        return E_INVALIDARG;

    pddens = new DDENAVIGATESTRUCT;
    if (!pddens)
        hres = E_OUTOFMEMORY;
    else
    {
        pddens->wszUrl = StrDupW(str);
        if (!pddens->wszUrl)
            hres = E_OUTOFMEMORY;
        else
        {
            // Don't do the navigate if *phwnd == 0, in that case we want to either
            // create a new window or activate an existing one that already is viewing
            // this URL.
            
            if (*phwnd != NULL)
            {
                BOOL fForceWindowReuse = FALSE;
                BSTR bstrUrl = NULL;
                // If there is even a single window with a location 
                // you are basically assured that you cannot force a 
                // reuse of windows. essentially

               // case 1 : only iexplore -nohome windows implies we want to force reuse
               // case 2 : only windows that have a location - we don't want to force reuse
               //          just follow user's preference
               // case 3: mix of iexplore -nohome windows and windows with location. we don't
               //         know what state we are in - don't force reuse
                hres = CDDEAuto_get_LocationURL(&bstrUrl, *phwnd);

                if(FAILED(hres) ||
                   (!bstrUrl)   ||
                   (SUCCEEDED(hres) && (*bstrUrl == L'\0')))
                {
                    fForceWindowReuse = TRUE;
                }
                if(bstrUrl)
                    SysFreeString(bstrUrl);
                    
                if( !(GetAsyncKeyState(VK_SHIFT) < 0)
                    && (fForceWindowReuse || SHRegGetBoolUSValue(REGSTR_PATH_MAIN, TEXT("AllowWindowReuse"), FALSE, TRUE)))
                {
                    hres = CDDEAuto_Common(DSID_NAVIGATEIEBROWSER, (LPARAM)pddens, phwnd, FALSE);
                }
            }

            

            if (SUCCEEDED(hres) && (*phwnd != 0) && (*phwnd != (HWND)-1))
            {
                // We found an existing browser window and successfully sent the
                // navigate message to it. Make the window foreground.
                SetForegroundWindow(*phwnd);

                if (IsIconic(*phwnd))
                    ShowWindowAsync(*phwnd,SW_RESTORE);
            }

            //
            // If we are using whatever window and all the browser windows are busy
            // (*phwnd == 0), or if there's no browser window opened (*phwnd == -1)
            // or we are asked to create a new one, then take the official OLE automation
            // route to start a new window.
            //
            if ((*phwnd == 0) ||
                (*phwnd == (HWND)-1))
            {
                //BUGBUG: this route doesn't give us the ability to return the hwnd of the window
                //in which the navigation took place (while we could - it's too hard and not worth it)
                LPITEMIDLIST pidlNew;
                TCHAR szURL[MAX_URL_STRING];
                OleStrToStrN(szURL, ARRAYSIZE(szURL), str, (UINT)-1);
                hres = IECreateFromPath(szURL, &pidlNew);
                if (SUCCEEDED(hres))
                {
                    // See if there is already a browser viewing this URL, if so just
                    // make him foreground otherwise create a new browser.
                    if ((hres = WinList_FindFolderWindow(pidlNew, NULL, phwnd, NULL)) == S_OK)
                    {
                        ILFree(pidlNew);
                        SetForegroundWindow(*phwnd);
                        ShowWindow(*phwnd, SW_SHOWNORMAL);
                    }
                    else
                    {
                        SHOpenNewFrame(pidlNew, NULL, 0, 0);
                    }
                }
            }
        }

        // It will be set to NULL if we don't need to free it.
        if (pddens)
        {
            if (pddens->wszUrl)
            {
                LocalFree(pddens->wszUrl);
            }
            delete pddens;
        }    
    }

    return hres;
}

HRESULT CDDEAuto_get_LocationURL(BSTR * pstr, HWND hwnd)
{
    return CDDEAuto_Common(DSID_GETLOCATIONURL, (LPARAM)pstr, &hwnd, FALSE);
}

HRESULT CDDEAuto_get_LocationTitle(BSTR * pstr, HWND hwnd)
{
    return CDDEAuto_Common(DSID_GETLOCATIONTITLE, (LPARAM)pstr, &hwnd, FALSE);
}

HRESULT CDDEAuto_get_HWND(long * phwnd)
{
    return CDDEAuto_Common(DSID_GETHWND, (LPARAM)phwnd, NULL, FALSE);
}

HRESULT CDDEAuto_Exit()
{
    return CDDEAuto_Common(DSID_EXIT, (LPARAM)NULL, NULL, FALSE);
}

#ifndef POSTPOSTSPLIT

#define DXTRACK 1
void FrameTrack(HDC hdc, LPRECT prc, UINT uFlags)
{
    COLORREF clrSave, clr;
    RECT    rc;

    // upperleft
    switch (uFlags)
    {
    case TRACKHOT:
        clr = GetSysColor(COLOR_BTNHILIGHT);
        break;
    case TRACKNOCHILD:
    case TRACKEXPAND:
        clr = GetSysColor(COLOR_BTNSHADOW);
        break;
    default:
        ASSERT(FALSE);
        clr = GetSysColor(COLOR_BTNSHADOW);
        break;
    }
    clrSave = SetBkColor(hdc, clr);
    rc = *prc;
    rc.bottom = rc.top + DXTRACK;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    rc.bottom = prc->bottom;
    rc.right = rc.left + DXTRACK;
    rc.top = prc->top + DXTRACK;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    // lowerright
    switch (uFlags)
    {
    case TRACKHOT:
        clr = GetSysColor(COLOR_BTNSHADOW);
        break;
    case TRACKNOCHILD:
    case TRACKEXPAND:
        clr = GetSysColor(COLOR_BTNHILIGHT);
        break;
    default:
        ASSERT(FALSE);
        break;
    }
    SetBkColor(hdc, clr);
    if (uFlags & (TRACKHOT | TRACKNOCHILD))
    {
        rc.right = prc->right;
        rc.top = rc.bottom - DXTRACK;
        rc.left = prc->left;
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    }
    rc.right = prc->right;
    rc.left = prc->right - DXTRACK;
    rc.top = prc->top;
    rc.bottom = prc->bottom;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    SetBkColor(hdc, clrSave);
    return;
}
#endif


#undef new // Hack!! Need to remove this (edwardp)

class CDelagateMalloc : public IMalloc
{
public:
    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID,void **);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IMalloc
    virtual STDMETHODIMP_(LPVOID)   Alloc(SIZE_T cb);
    virtual STDMETHODIMP_(LPVOID)   Realloc(void *pv, SIZE_T cb);
    virtual STDMETHODIMP_(void)     Free(void *pv);
    virtual STDMETHODIMP_(SIZE_T)    GetSize(void *pv);
    virtual STDMETHODIMP_(int)      DidAlloc(void *pv);
    virtual STDMETHODIMP_(void)     HeapMinimize();

private:
    CDelagateMalloc(void *pv, SIZE_T cbSize, WORD wOuter);
    ~CDelagateMalloc();
    void* operator new(size_t cbClass, SIZE_T cbSize);

    friend HRESULT CDelegateMalloc_Create(void *pv, SIZE_T cbSize, WORD wOuter, IMalloc **ppmalloc);

protected:
    LONG _cRef;
    WORD _wOuter;           // delegate item outer signature
    WORD _wUnused;          // to allign
#ifdef DEBUG
    UINT _cAllocs;
#endif
    SIZE_T _cb;
    BYTE _data[EMPTY_SIZE];
};

void* CDelagateMalloc::operator new(size_t cbClass, SIZE_T cbSize)
{
    return ::operator new(cbClass + cbSize);
}

CDelagateMalloc::CDelagateMalloc(void *pv, SIZE_T cbSize, WORD wOuter)
{
    _cRef = 1;
    _wOuter = wOuter;
    _cb = cbSize;

    memcpy(_data, pv, _cb);
}

CDelagateMalloc::~CDelagateMalloc()
{
    // This isn't something we need to worry about and it causes way
    // too much noise. -BryanSt
//    DEBUG_CODE( TraceMsg(DM_TRACE, "DelegateMalloc destroyed with %d allocs performed", _cAllocs); )
}

HRESULT CDelagateMalloc::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IMalloc) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IMalloc *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CDelagateMalloc::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDelagateMalloc::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

void *CDelagateMalloc::Alloc(SIZE_T cb)
{
    WORD cbActualSize = (WORD)(
                        SIZEOF(DELEGATEITEMID) - 1 +    // header (-1 sizeof(rgb[0])
                        cb +                            // inner
                        _cb);                           // outer data

    PDELEGATEITEMID pidl = (PDELEGATEITEMID)SHAlloc(cbActualSize + 2);  // +2 for pidl term
    if (pidl)
    {
        pidl->cbSize = cbActualSize;
        pidl->wOuter = _wOuter;
        pidl->cbInner = (WORD)cb;
        memcpy(&pidl->rgb[cb], _data, _cb);
        *(WORD *)&(((BYTE *)pidl)[cbActualSize]) = 0;
#ifdef DEBUG
        _cAllocs++;
#endif
    }
    return pidl;
}

void *CDelagateMalloc::Realloc(void *pv, SIZE_T cb)
{
    return NULL;
}

void CDelagateMalloc::Free(void *pv)
{
    SHFree(pv);
}

SIZE_T CDelagateMalloc::GetSize(void *pv)
{
    return (SIZE_T)-1;
}

int CDelagateMalloc::DidAlloc(void *pv)
{
    return -1;
}

void CDelagateMalloc::HeapMinimize()
{
}

STDAPI CDelegateMalloc_Create(void *pv, SIZE_T cbSize, WORD wOuter, IMalloc **ppmalloc)
{
    CDelagateMalloc *pdm = new(cbSize) CDelagateMalloc(pv, cbSize, wOuter);
    if (pdm)
    {
        HRESULT hres = pdm->QueryInterface(IID_IMalloc, (void **)ppmalloc);
        pdm->Release();
        return hres;
    }
    return E_OUTOFMEMORY;
}


//+-------------------------------------------------------------------------
// This function scans the head of an html document for the desired element
// with a particular attribute.  If a match is found, the first occurance
// of that element is returned in punkDesired and S_OK is returned.
// Otherwise, E_FAIL is returned.
//
// Example:  Find the first meta element with name="ProgID":
//
//   SearchForElementInHead(pHTMLDoc, OLESTR("Name"), OLESTR("ProgId"),
//           IID_IHTMLMetaElement, (IUnknown**)&pMetaElement);
//
//--------------------------------------------------------------------------
HRESULT SearchForElementInHead
(
    IHTMLDocument2* pHTMLDocument,  // [in] document to search
    LPOLESTR        pszAttribName,  // [in] attribute to check for
    LPOLESTR        pszAttrib,      // [in] value the attribute must have
    REFIID          iidDesired,     // [in] element interface to return
    IUnknown**      ppunkDesired    // [out] returned interface
)
{
    ASSERT(NULL != pHTMLDocument);
    ASSERT(NULL != pszAttribName);
    ASSERT(NULL != pszAttrib);
    ASSERT(NULL != ppunkDesired);

    HRESULT hr = E_FAIL;
    *ppunkDesired = NULL;

    BSTR bstrAttribName = SysAllocString(pszAttribName);
    if (NULL == bstrAttribName)
    {
        return E_OUTOFMEMORY;
    }

    //
    // First get all document elements.  Note that this is very fast in
    // ie5 because the collection directly accesses the internal tree.
    //
    IHTMLElementCollection * pAllCollection;
    if (SUCCEEDED(pHTMLDocument->get_all(&pAllCollection)))
    {
        IUnknown* punk;
        IHTMLBodyElement* pBodyElement;
        IHTMLFrameSetElement* pFrameSetElement;
        IDispatch* pDispItem;

        //
        // Now we scan the document for the desired tags.  Since we're only
        // searching the head, and since Trident always creates a body tag
        // (unless there is a frameset), we can stop looking when we hit the
        // body or frameset.
        //
        // Note, the alternative of using pAllCollection->tags to return the
        // collection of desired tags is likely more expensive because it will
        // walk the whole tree (unless Trident optimizes this).
        //
        long lItemCnt;
        VARIANT vEmpty;
        V_VT(&vEmpty) = VT_EMPTY;

        VARIANT vIndex;
        V_VT(&vIndex) = VT_I4;

        EVAL(SUCCEEDED(pAllCollection->get_length(&lItemCnt)));

        for (long lItem = 0; lItem < lItemCnt; lItem++)
        {
            V_I4(&vIndex) = lItem;

            if (S_OK == pAllCollection->item(vIndex, vEmpty, &pDispItem))
            {
                //
                // First see if it's the desired element type
                //
                if (SUCCEEDED(pDispItem->QueryInterface(iidDesired,
                                                    (void **)&punk)))
                {
                    //
                    // Next see if it has the desired attribute
                    //
                    IHTMLElement* pElement;

                    if (SUCCEEDED(pDispItem->QueryInterface(IID_IHTMLElement, (void **)&pElement)))
                    {
                        VARIANT varAttrib;
                        V_VT(&varAttrib) = VT_EMPTY;

                        if (SUCCEEDED(pElement->getAttribute(bstrAttribName, FALSE, &varAttrib)) &&
                            (V_VT(&varAttrib) == VT_BSTR) && varAttrib.bstrVal &&
                            (StrCmpIW(varAttrib.bstrVal, pszAttrib) == 0) )
                        {
                            // Found it!
                            *ppunkDesired = punk;
                            punk = NULL;
                            hr = S_OK;

                            // Terminate the search;
                            lItem = lItemCnt;
                        }
                        pElement->Release();

                        VariantClear(&varAttrib);
                    }

                    if (punk)
                        punk->Release();
                }
                //
                // Next check for the body tag
                //
                else if (SUCCEEDED(pDispItem->QueryInterface(IID_IHTMLBodyElement,
                                                    (void **)&pBodyElement)) )
                {
                    // Found a body tag, so terminate the search
                    lItem = lItemCnt;
                    pBodyElement->Release();
                }
                //
                // Finally, check for a frameset tag
                //
                else if (SUCCEEDED(pDispItem->QueryInterface(IID_IHTMLFrameSetElement,
                                                    (void **)&pFrameSetElement)) )
                {
                    // Found a frameset tag, so terminate the search
                    lItem = lItemCnt;
                    pFrameSetElement->Release();
                }

                pDispItem->Release();
            }
        }
        // Make sure that these don't have to be cleared (should not have been modified)
        ASSERT(vEmpty.vt == VT_EMPTY);
        ASSERT(vIndex.vt == VT_I4);

        pAllCollection->Release();
    }

    SysFreeString(bstrAttribName);

    return hr;
}


//+-------------------------------------------------------------------
//     JITCoCreateInstance
//
//  This function makes sure that the Option pack which
//  has this class id is installed.
//  It attempts to make sure that the Option pack corresponding
//  to the current IE Build.
//  If the feature does get installed correctly, it will
//  attempt to CoCreate the specified CLSID
//
//+------------------------------------------------------------------
HRESULT
JITCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID riid,
    LPVOID FAR* ppv,
    HWND hwndParent,
    DWORD dwJitFlags
)
{
    uCLSSPEC ucs;
    QUERYCONTEXT qc = { 0 };
    HRESULT hr;
    ucs.tyspec = TYSPEC_CLSID;
    ucs.tagged_union.clsid = rclsid;

    ASSERT((dwJitFlags & ~(FIEF_FLAG_FORCE_JITUI | FIEF_FLAG_PEEK)) == 0);

    hr = FaultInIEFeature(hwndParent, &ucs, &qc, dwJitFlags);

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    }

    return hr;
}

BOOL IsFeaturePotentiallyAvailable(REFCLSID rclsid)
{
    uCLSSPEC ucs;
    QUERYCONTEXT qc = { 0 };

    ucs.tyspec = TYSPEC_CLSID;
    ucs.tagged_union.clsid = rclsid;

    return (FaultInIEFeature(NULL, &ucs, &qc, FIEF_FLAG_FORCE_JITUI | FIEF_FLAG_PEEK) != E_ACCESSDENIED);
}

BOOL CreateFromDesktop(PNEWFOLDERINFO pfi)
{
    //
    //  BUGBUGHACKHACK - we need to handle differences in the way we parse the command line
    //  on IE4 integrated.  we should not be called by anybody but IE4's Explorer.exe
    //
    ASSERT(GetUIVersion() == 4);
    if (!pfi->pidl) 
    {
        if ((pfi->uFlags & (COF_ROOTCLASS | COF_NEWROOT)) || pfi->pidlRoot)
        {
            pfi->pidl = ILRootedCreateIDList(pfi->uFlags & COF_ROOTCLASS ? &pfi->clsid : NULL, pfi->pidlRoot);
            pfi->uFlags &= ~(COF_ROOTCLASS | COF_NEWROOT);
            ILFree(pfi->pidlRoot);
            pfi->pidlRoot = NULL;
            pfi->clsid = CLSID_NULL;
        }
        else if (!PathIsURLA(pfi->pszPath))
        {
           CHAR szTemp[MAX_PATH];
           GetCurrentDirectoryA(ARRAYSIZE(szTemp), szTemp);
           PathCombineA(szTemp, szTemp, pfi->pszPath);
           Str_SetPtrA(&(pfi->pszPath), szTemp);
        } 
    }

    ASSERT(!(pfi->uFlags & (COF_ROOTCLASS | COF_NEWROOT)));
    
    return SHCreateFromDesktop(pfi);
}

//***   IsVK_TABCycler -- is key a TAB-equivalent
// ENTRY/EXIT
//  dir     0 if not a TAB, non-0 if a TAB
// NOTES
//  NYI: -1 for shift+tab, 1 for tab
//
int IsVK_TABCycler(MSG *pMsg)
{
    if (!pMsg)
        return 0;

    if (pMsg->message != WM_KEYDOWN)
        return 0;
    if (! (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6))
        return 0;

    return (GetKeyState(VK_SHIFT) < 0) ? -1 : 1;
}

#ifdef DEBUG
//
// This function fully fills the input buffer, trashing stuff so we fault
// when the size is wrong :-)
//
// We should hopefully catch people passing buffers that are too small here by
// having a reproable stack corruption.
//
#define CH_BADCHARA          ((CHAR)0xCC)
#define CH_BADCHARW          ((WCHAR)0xCCCC)

#ifdef UNICODE
#define CH_BADCHAR          CH_BADCHARW
#else // UNICODE
#define CH_BADCHAR          CH_BADCHARA
#endif // UNICODE

void DebugFillInputString(LPTSTR pszBuffer, DWORD cchSize)
{
    while (cchSize--)
    {
        pszBuffer[0] = CH_BADCHAR;
        pszBuffer++;
    }
}

void DebugFillInputStringA(LPSTR pszBuffer, DWORD cchSize)
{
    while (cchSize--)
    {
        pszBuffer[0] = CH_BADCHARA;
        pszBuffer++;
    }
}


#endif // DEBUG

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject)
{
    RECT    rc;
    POINT   pt;

    GetWindowRect(hwndTarget, &rc);
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;
    pt.y = ptStart.y - rc.top;
    DAD_DragEnterEx2(hwndTarget, pt, pdtObject);
    return;
}

void _DragMove(HWND hwndTarget, const POINTL ptStart)
{
    RECT rc;
    POINT pt;

    GetWindowRect(hwndTarget, &rc);
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;
    pt.y = ptStart.y - rc.top;
    DAD_DragMove(pt);
    return;
}

STDAPI_(IBindCtx *) CreateBindCtxForUI(IUnknown * punkSite)
{
    IBindCtx * pbc = NULL;

    if (EVAL(punkSite && SUCCEEDED(CreateBindCtx(0, &pbc))))
    {
        if (FAILED(pbc->RegisterObjectParam(STR_DISPLAY_UI_DURING_BINDING, punkSite)))
        {
            // It failed damn nagit.
            ATOMICRELEASE(pbc);
        }
    }

    return pbc;
}

//
// Return the location of the internet cache
// HRESULT GetCacheLocation(
// dwSize          no. of chars in pszCacheLocation

STDAPI GetCacheLocation(LPTSTR pszCacheLocation, DWORD dwSize)
{
    HRESULT hr = S_OK;
    DWORD dwLastErr;
    LPINTERNET_CACHE_CONFIG_INFO lpCCI = NULL;  // init to suppress bogus C4701 warning
    DWORD dwCCISize = sizeof(INTERNET_CACHE_CONFIG_INFO);
    BOOL fOnceErrored = FALSE;

    while (TRUE)
    {
        if ((lpCCI = (LPINTERNET_CACHE_CONFIG_INFO) LocalAlloc(LPTR,
                                                        dwCCISize)) == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        if (!GetUrlCacheConfigInfo(lpCCI, &dwCCISize,
                                            CACHE_CONFIG_CONTENT_PATHS_FC))
        {
            if ((dwLastErr = GetLastError()) != ERROR_INSUFFICIENT_BUFFER  ||
                fOnceErrored)
            {
                hr = HRESULT_FROM_WIN32(dwLastErr);
                goto cleanup;
            }

            //
            // We have insufficient buffer size; reallocate a buffer with the
            //      new dwCCISize set by GetUrlCacheConfigInfo
            // Set fOnceErrored to TRUE so that we don't loop indefinitely
            //
            fOnceErrored = TRUE;
        }
        else
        {
            LPTSTR pszPath = lpCCI->CachePaths[0].CachePath;
            INT iLen;

            PathRemoveBackslash(pszPath);
            iLen = lstrlen(pszPath) + 1;        // + 1 is for the null char

            if ((DWORD) iLen < dwSize)
            {
                StrCpyN(pszCacheLocation, pszPath, iLen);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }

            break;
        }

        LocalFree(lpCCI);
        lpCCI = NULL;
    }

cleanup:
    if (lpCCI != NULL)
    {
        LocalFree(lpCCI);
    }

    return hr;
}

STDAPI_(UINT) GetWheelMsg()
{
    static UINT s_msgMSWheel = 0;
    if (s_msgMSWheel == 0)
        s_msgMSWheel = RegisterWindowMessage(TEXT("MSWHEEL_ROLLMSG"));
    return s_msgMSWheel;
}

STDAPI StringToStrRet(LPCTSTR pString, STRRET *pstrret)
{
    HRESULT hr = SHStrDup(pString, &pstrret->pOleStr);
    if (SUCCEEDED(hr))
    {
        pstrret->uType = STRRET_WSTR;
    }
    return hr;
}

// these two functions are duplicated from browseui
HINSTANCE GetComctl32Hinst()
{
    static HINSTANCE s_hinst = NULL;
    if (!s_hinst)
        s_hinst = GetModuleHandle(TEXT("comctl32.dll"));
    return s_hinst;
}

// since we don't define the proper WINVER we do this ourselves
#ifndef IDC_HAND
#define IDC_HAND            MAKEINTRESOURCE(32649)
#endif

STDAPI_(HCURSOR) LoadHandCursor(DWORD dwRes)
{
    if (g_bRunOnNT5 || g_bRunOnMemphis)
    {
        HCURSOR hcur = LoadCursor(NULL, IDC_HAND);  // from USER, system supplied
        if (hcur)
            return hcur;
    }
    return LoadCursor(GetComctl32Hinst(), IDC_HAND_INTERNAL);
}



//+-------------------------------------------------------------------------
// Returns true if this type of url may not be available when offline unless
// it is cached by wininet
//--------------------------------------------------------------------------
BOOL MayBeUnavailableOffline(LPTSTR pszUrl)
{
    BOOL fRet = FALSE;
    URL_COMPONENTS uc = {0};
    uc.dwStructSize = sizeof(uc);

    if (SUCCEEDED(InternetCrackUrl(pszUrl, 0, 0, &uc)))
    {
        fRet = uc.nScheme == INTERNET_SCHEME_HTTP ||
            uc.nScheme == INTERNET_SCHEME_HTTPS ||
            uc.nScheme == INTERNET_SCHEME_FTP ||
            uc.nScheme == INTERNET_SCHEME_GOPHER;
    }
    return fRet;
}

//+-------------------------------------------------------------------------
// If the folder is a link, the associated URL is returned.
//--------------------------------------------------------------------------
HRESULT GetNavTargetName(IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszUrl, UINT cMaxChars)
{
    LPITEMIDLIST pidlTarget;
    HRESULT hr = SHGetNavigateTarget(psf, pidl, &pidlTarget, NULL);
    if (SUCCEEDED(hr))
    {
        // Get the URL
        hr = IEGetNameAndFlags(pidlTarget, SHGDN_FORADDRESSBAR, pszUrl, cMaxChars, NULL);
        ILFree(pidlTarget);
    }
    else
        *pszUrl = 0;
    return hr;
}

//+-------------------------------------------------------------------------
// Returns info about whether this item is available offline. Returns E_FAIL
// if the item is not a link.
// if we navigate to this item
//  (true if we're online, items in the cache or otherwise available)
// if item is a sticky cache entry
//--------------------------------------------------------------------------
// BUGBUG: this should use an interface to bind to this information abstractly
// psf->GetUIObjectOf(IID_IAvailablility, ...);

HRESULT GetLinkInfo(IShellFolder* psf, LPCITEMIDLIST pidlItem, BOOL* pfAvailable, BOOL* pfSticky)
{
    if (pfAvailable)
        *pfAvailable = TRUE;

    if (pfSticky)
        *pfSticky = FALSE;
    //
    // See if it is a link. If it is not, then it can't be in the wininet cache and can't
    // be pinned (sticky cache entry) or greyed (unavailable when offline)
    //
    WCHAR szUrl[MAX_URL_STRING];
    DWORD dwFlags = 0;

    HRESULT hr = GetNavTargetName(psf, pidlItem, szUrl, ARRAYSIZE(szUrl));

    if (SUCCEEDED(hr))
    {
        CHAR szUrlAnsi[MAX_URL_STRING];

        //
        // Get the cache info for this item.  Note that we use GetUrlCacheEntryInfoEx instead
        // of GetUrlCacheEntryInfo because it follows any redirects that occured.  This wacky
        // api uses a variable length buffer, so we have to guess the size and retry if the
        // call fails.
        //
        BOOL fInCache = FALSE;
        WCHAR szBuf[512];
        LPINTERNET_CACHE_ENTRY_INFOA pCE = (LPINTERNET_CACHE_ENTRY_INFOA)szBuf;
        DWORD dwEntrySize = ARRAYSIZE(szBuf);

        SHTCharToAnsi(szUrl, szUrlAnsi, ARRAYSIZE(szUrlAnsi));
        if (!(fInCache = GetUrlCacheEntryInfoExA(szUrlAnsi, pCE, &dwEntrySize, NULL, NULL, NULL, 0)))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                // We guessed too small for the buffer so allocate the correct size & retry
                pCE = (LPINTERNET_CACHE_ENTRY_INFOA)LocalAlloc(LPTR, dwEntrySize);
                if (pCE)
                {
                    fInCache = GetUrlCacheEntryInfoExA(szUrlAnsi, pCE, &dwEntrySize, NULL, NULL, NULL, 0);
                }
            }
        }

        //
        // If we are offline, see if the item is in the cache.
        //
        if (pfAvailable && SHIsGlobalOffline() && MayBeUnavailableOffline(szUrl) && !fInCache)
        {
            // Not available
            *pfAvailable = FALSE;
        }

        //
        // See if it's a sticky cache entry
        //
        if (pCE)
        {
            if (pfSticky && fInCache && (pCE->CacheEntryType & STICKY_CACHE_ENTRY))
            {
                *pfSticky = TRUE;
            }

            if ((TCHAR*)pCE != szBuf)
            {
                LocalFree(pCE);
            }
        }
    }
    return hr;
}

//
// Get the avg char width given an hwnd
//

int GetAvgCharWidth(HWND hwnd)
{
    ASSERT(hwnd);

    int nWidth = 0;

    HFONT hfont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);

    if (hfont)
    {
        HDC hdc = GetDC(NULL);

        if(hdc)
        {
            HFONT hfontOld = (HFONT)SelectObject(hdc, hfont);

            TEXTMETRIC tm;

            if (GetTextMetrics(hdc, &tm))
                nWidth = tm.tmAveCharWidth;

            SelectObject(hdc, hfontOld);

            ReleaseDC(NULL, hdc);
        }
    }

    return nWidth;
}

//
// Converts all "&" into "&&" so that they show up
// in menus
//
void FixAmpersands(LPWSTR pszToFix, UINT cchMax)
{
    ASSERT(pszToFix && cchMax > 0);

    WCHAR szBuf[MAX_URL_STRING];
    LPWSTR pszBuf = szBuf;
    LPWSTR pszSrc = pszToFix;
    UINT cch = 0;

    while (*pszSrc && cch < ARRAYSIZE(szBuf)-2)
    {
        if (*pszSrc == '&')
        {
            *pszBuf++ = '&';
            ++cch;
        }
        *pszBuf++ = *pszSrc++;
        ++cch;
    }
    *pszBuf = 0;

    StrCpyN(pszToFix, szBuf, cchMax);
}

// EVIL: return an aliased pointer to the pidl in this variant (that is why it is const)
// see VariantToIDList

STDAPI_(LPCITEMIDLIST) VariantToConstIDList(const VARIANT *pv)
{
    if (pv == NULL)
        return NULL;

    LPCITEMIDLIST pidl = NULL;

    if (pv->vt == (VT_BYREF | VT_VARIANT) && pv->pvarVal)
        pv = pv->pvarVal;

    switch (pv->vt)
    {
    case VT_UINT_PTR:
        // HACK in process case, avoid the use of this if possible
        pidl = (LPCITEMIDLIST)pv->byref;
        ASSERT(pidl);
        break;

    case VT_ARRAY | VT_UI1:
        pidl = (LPCITEMIDLIST)pv->parray->pvData;   // alias: PIDL encoded
        break;

    }
    return pidl;
}

STDAPI_(LPITEMIDLIST) VariantToIDList(const VARIANT *pv)
{
    LPITEMIDLIST pidl = NULL;

    if (pv)
    {
        if (pv->vt == (VT_BYREF | VT_VARIANT) && pv->pvarVal)
            pv = pv->pvarVal;

        switch (pv->vt)
        {
        case VT_I2:
            pidl = SHCloneSpecialIDList(NULL, pv->iVal, TRUE);
            break;

        case VT_I4:
            pidl = SHCloneSpecialIDList(NULL, pv->lVal, TRUE);
            break;

        case VT_BSTR:
            pidl = ILCreateFromPath(pv->bstrVal);
            break;

        case VT_ARRAY | VT_UI1:
            pidl = ILClone((LPCITEMIDLIST)pv->parray->pvData);
            break;

        default:
            LPCITEMIDLIST pidlToCopy = VariantToConstIDList(pv);
            if (pidlToCopy)
                pidl = ILClone(pidlToCopy);
            break;
        }
    }

    return pidl;
}

STDAPI VariantToBuffer(const VARIANT* pvar, void *pv, UINT cb)
{
    if (pvar && pvar->vt == (VT_ARRAY | VT_UI1))
    {
        memcpy(pv, pvar->parray->pvData, cb);
        return TRUE;
    }
    return FALSE;
}

STDAPI VariantToGUID(VARIANT *pvar, GUID *pguid)
{
    return VariantToBuffer(pvar, pguid, sizeof(*pguid));
}

STDAPI InitVariantFromBuffer(VARIANT *pvar, const void *pv, UINT cb)
{
    HRESULT hres;
    SAFEARRAY *psa = SafeArrayCreateVector(VT_UI1, 0, cb);   // create a one-dimensional safe array
    if (psa) 
    {
        memcpy(psa->pvData, pv, cb);

        memset(pvar, 0, sizeof(*pvar));  // VariantInit()
        pvar->vt = VT_ARRAY | VT_UI1;
        pvar->parray = psa;
        hres = S_OK;
    }
    else
        hres = E_OUTOFMEMORY;
    return hres;
}

STDAPI_(BOOL) InitVariantFromIDList(VARIANT* pvar, LPCITEMIDLIST pidl)
{
    return SUCCEEDED(InitVariantFromBuffer(pvar, pidl, ILGetSize(pidl)));
}

STDAPI_(BOOL) InitVariantFromGUID(VARIANT *pvar, GUID *pguid)
{
    return SUCCEEDED(InitVariantFromBuffer(pvar, pguid, SIZEOF(*pguid)));
}

// EVIL: if you know the VARIANT will be passed in process only you can
// use this. note code above that decodes this properly
//
// the lifetime of the pidl must equal that of the VARIANT

STDAPI_(void) InitVariantFromIDListInProc(VARIANT *pvar, LPCITEMIDLIST pidl)
{
    memset(pvar, 0, sizeof(*pvar));  // VariantInit()

    pvar->vt = VT_UINT_PTR;
    pvar->byref = (PVOID)pidl;
}

#define SZ_REGKEY_INETCPL_POLICIES   TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel")
BOOL IsInetcplRestricted(LPWSTR pszCommand)
{
    BOOL fDisabled;
    DWORD dwData;
    DWORD dwSize = sizeof(dwData);
    DWORD dwType;

    if (ERROR_SUCCESS == SHRegGetUSValue(
                            SZ_REGKEY_INETCPL_POLICIES,
                            pszCommand,
                            &dwType,
                            (LPVOID)&dwData,
                            &dwSize,
                            FALSE,
                            NULL,
                            0))
    {
        fDisabled = dwData;
    }
    else
    {
        // If the value is missing from the registry, then
        // assume the feature is enabled
        fDisabled = FALSE;
    }
    return fDisabled;
}

BOOL HasExtendedChar(LPCWSTR pszQuery)
{
    BOOL fNonAscii = FALSE;
    for (LPCWSTR psz = pszQuery; *psz; psz++)
    {
        if (*psz > 0x7f)
        {
            fNonAscii = TRUE;
            break;
        }
    }
    return fNonAscii;
}

void ConvertToUtf8Escaped(LPWSTR pszUrl, int cch)
{
    // Convert to utf8
    char szBuf[MAX_URL_STRING];
    SHUnicodeToAnsiCP(CP_UTF8, pszUrl, szBuf, ARRAYSIZE(szBuf));

    // Escape the string into the original buffer
    LPSTR pchIn; 
    LPWSTR pchOut = pszUrl;
    WCHAR ch;
    static const WCHAR hex[] = L"0123456789ABCDEF";

    for (pchIn = szBuf; *pchIn && cch > 3; pchIn++)
    {
        ch = *pchIn;

        if (ch > 0x7f)
        {
            cch -= 3;
            *pchOut++ = L'%';
            *pchOut++ = hex[(ch >> 4) & 15];
            *pchOut++ = hex[ch & 15];
        }
        else
        {
            --cch;
            *pchOut++ = *pchIn;
        }
    }

    *pchOut = L'\0';
}




HRESULT IExtractIcon_GetIconLocation(
    IUnknown *punk,
    IN  UINT   uInFlags,
    OUT LPTSTR pszIconFile,
    IN  UINT   cchIconFile,
    OUT PINT   pniIcon,
    OUT PUINT  puOutFlags)
{
    ASSERT(punk);
    HRESULT hr;
    
    if (g_fRunningOnNT)
    {
        IExtractIcon *pxi;
        hr = punk->QueryInterface(IID_IExtractIcon, (void **)&pxi);

        if (SUCCEEDED(hr))
        {
            hr = pxi->GetIconLocation(uInFlags, pszIconFile, cchIconFile, pniIcon, puOutFlags);

            pxi->Release();
        }
    }
    else
    {
        IExtractIconA *pxi;
        hr = punk->QueryInterface(IID_IExtractIconA, (void **)&pxi);

        if (SUCCEEDED(hr))
        {
            CHAR sz[MAX_PATH];
            hr = pxi->GetIconLocation(uInFlags, sz, SIZECHARS(sz), pniIcon, puOutFlags);

            if (SUCCEEDED(hr))
                SHAnsiToTChar(sz, pszIconFile, cchIconFile);

            pxi->Release();
        }
    }

    return hr;
}
        

HRESULT IExtractIcon_Extract(
    IUnknown *punk,
    IN  LPCTSTR pszIconFile,
    IN  UINT    iIcon,
    OUT HICON * phiconLarge,
    OUT HICON * phiconSmall,
    IN  UINT    ucIconSize)
{
    ASSERT(punk);
    HRESULT hr;
    
    if (g_fRunningOnNT)
    {
        IExtractIcon *pxi;
        hr = punk->QueryInterface(IID_IExtractIcon, (void **)&pxi);

        if (SUCCEEDED(hr))
        {
            hr = pxi->Extract(pszIconFile, iIcon, phiconLarge, phiconSmall, ucIconSize);

            pxi->Release();
        }
    }
    else
    {
        IExtractIconA *pxi;
        hr = punk->QueryInterface(IID_IExtractIconA, (void **)&pxi);

        if (SUCCEEDED(hr))
        {
            CHAR sz[MAX_PATH];
            SHTCharToAnsi(pszIconFile, sz, SIZECHARS(sz));
            hr = pxi->Extract(sz, iIcon, phiconLarge, phiconSmall, ucIconSize);

            pxi->Release();
        }
    }

    return hr;
}


typedef EXECUTION_STATE (__stdcall *PFNSTES) (EXECUTION_STATE);

EXECUTION_STATE _SetThreadExecutionState(EXECUTION_STATE esFlags)
{
    static PFNSTES _pfnSetThreadExecutionState = (PFNSTES)0xffffffff;
    
    if(_pfnSetThreadExecutionState == (PFNSTES)0xffffffff)
        _pfnSetThreadExecutionState = (PFNSTES)GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadExecutionState");

    if(_pfnSetThreadExecutionState != (PFNSTES)NULL)
        return(_pfnSetThreadExecutionState(esFlags));
    else
        return((EXECUTION_STATE)NULL);
}


HRESULT SHPathPrepareForWriteWrap(HWND hwnd, IUnknown *punkEnableModless, LPCTSTR pszPath, UINT wFunc, DWORD dwFlags)
{
    HRESULT hr;

    if (g_bRunOnNT5)
    {
        // NT5's version of the API is better.
        hr = _SHPathPrepareForWriteW(hwnd, punkEnableModless, pszPath, dwFlags);
    }
    else
    {
        hr = SHCheckDiskForMedia(hwnd, punkEnableModless, pszPath, wFunc);
    }

    return hr;
}
void GetPathOtherFormA(LPSTR lpszPath, LPSTR lpszNewPath, DWORD dwSize)
{
    BOOL bQuotes = FALSE;
    LPSTR szStart = lpszPath;
    LPSTR szEnd = NULL;
    LPSTR szNewStart = lpszNewPath;

    ZeroMemory(lpszNewPath, dwSize);

    // Cull out the starting and ending " because GetShortPathName does not
    // like it.
    if (*lpszPath == '"')
    {
        bQuotes = TRUE;

        szStart = lpszPath + 1;
        szEnd   = lpszPath + lstrlenA(lpszPath) - 1; // Point to the last "
        *szEnd  = '\0';

        szNewStart = lpszNewPath + 1;  // So that we can insert the " in it.
        dwSize = dwSize - 2;  // for the two double quotes to be added.
    }

    if (GetShortPathNameA(szStart, szNewStart, dwSize) != 0)
    {
        if (StrCmpIA(szStart, szNewStart) == 0)
        {   // The original Path is a SFN. So NewPath needs to be LFN.
            GetLongPathNameA((LPCSTR)szStart, szNewStart, dwSize);
        }
    }
                                             
    // Now add the " to the NewPath so that it is in the expected form
    if (bQuotes)
    {
        int len = 0;

        // Fix the Original path.
        *szEnd = '"';

        // Fix the New path.
        *lpszNewPath = '"';        // Insert " in the beginning.
        len = lstrlenA(lpszNewPath);
        *(lpszNewPath + len) = '"'; // Add the " in the end.
        *(lpszNewPath + len + 1) = '\0'; // Terminate the string.
    }

    return;
}

int GetUrlSchemeFromPidl(LPCITEMIDLIST pidl)
{
    ASSERT(pidl);
    ASSERT(IsURLChild(pidl, FALSE));

    int nRet = URL_SCHEME_INVALID;

    WCHAR szUrl[MAX_URL_STRING];

    if (SUCCEEDED(IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szUrl,
                                    ARRAYSIZE(szUrl), NULL)))
    {
        nRet = GetUrlScheme(szUrl);
    }

    return nRet;
}

//
// Check if it is safe to create a shortcut for the given url.  Used by add
// to favorites code.
//
BOOL IEIsLinkSafe(HWND hwnd, LPCITEMIDLIST pidl, ILS_ACTION ilsFlag)
{
    ASSERT(pidl);

    BOOL fRet = TRUE;

    if (IsURLChild(pidl, FALSE))
    {
        int nScheme = GetUrlSchemeFromPidl(pidl);

        if (URL_SCHEME_JAVASCRIPT == nScheme || URL_SCHEME_VBSCRIPT == nScheme)
        {
            WCHAR szTitle[MAX_PATH];
            WCHAR szText[MAX_PATH];

            MLLoadString(IDS_SECURITYALERT, szTitle, ARRAYSIZE(szTitle));
            MLLoadString(IDS_ADDTOFAV_WARNING + ilsFlag, szText,
                         ARRAYSIZE(szText));

            fRet = (IDYES == MLShellMessageBox(hwnd, szText, szTitle, MB_YESNO |
                                               MB_ICONWARNING | MB_APPLMODAL |
                                               MB_DEFBUTTON2));
        }
    }

    return fRet;
}

BOOL AccessAllowed(LPCWSTR pwszURL1, LPCWSTR pwszURL2)
{
    BOOL fRet = FALSE;
    IInternetSecurityManager *pSecMgr = NULL;

    if (pwszURL1 && pwszURL2 && SUCCEEDED(CoCreateInstance(CLSID_InternetSecurityManager, 
                                   NULL, 
                                   CLSCTX_INPROC_SERVER,
                                   IID_IInternetSecurityManager, 
                                   (void **)&pSecMgr)))
    {
        BYTE reqSid[MAX_SIZE_SECURITY_ID], docSid[MAX_SIZE_SECURITY_ID];
        DWORD cbReqSid = ARRAYSIZE(reqSid);
        DWORD cbDocSid = ARRAYSIZE(docSid);

        if (   SUCCEEDED(pSecMgr->GetSecurityId(pwszURL1, reqSid, &cbReqSid, 0))
            && SUCCEEDED(pSecMgr->GetSecurityId(pwszURL2, docSid, &cbDocSid, 0))
            && (cbReqSid == cbDocSid)
            && (memcmp(reqSid, docSid, cbReqSid) == 0))                    
        {
            fRet = TRUE;
        }
        pSecMgr->Release();
    }
    return fRet;
}