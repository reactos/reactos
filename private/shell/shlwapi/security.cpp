#include "priv.h"
#include <shlobj.h>
#include <shellp.h>
#include <shdguid.h>
#include "ids.h"
#include <objbase.h>
#include <wininet.h>            // INTERNET_MAX_URL_LENGTH
#include <shellp.h>
#include <commctrl.h>
#include <mluisupp.h>
#include <inetcpl.h>
#include <crypto\md5.h>

#ifdef UNIX
#include <urlmon.h>
#endif

// This will automatically be freed when the process shuts down.
// Creating the ClassFactory for CLSID_InternetSecurityManager
// is really slow, so we cache it because dragging and dropping
// files does a lot of zone checking.
IClassFactory * g_pcf = NULL;

HRESULT _GetCachedZonesManager(REFIID riid, void **ppv)
{
    HRESULT hr;

    if (!g_pcf)
    {
        CoGetClassObject(CLSID_InternetSecurityManager, CLSCTX_INPROC_SERVER, NULL, IID_IClassFactory, (void **)&g_pcf);
        SHPinDllOfCLSID(&CLSID_InternetSecurityManager);
    }

    if (g_pcf) 
    {
        hr = g_pcf->CreateInstance(NULL, riid, ppv);
    }
    else
    {
        *ppv = NULL;
        hr = E_FAIL;
    }
    return hr;
}


             
/**********************************************************************\
    FUNCTION: ZoneCheckUrlExCacheW

    DESCRIPTION:

        Call IInternetSecurityManager::ProcessUrlAction using the
        cached one if available.

        pwszUrl - URL to check
        pdwPolicy - Receives resulting policy (optional)
        dwPolicySize - size of policy buffer (usually sizeof(DWORD))
        pdwContext - context (optional)
        dwContextSize - size of context buffer (usually sizeof(DWORD))
        dwActionType - ProcessUrlAction action type code
        dwFlags - Flags for ProcessUrlAction
        pisms - IInternetSecurityMgrSite to use during
                ProcessUrlAction (optional)
        ppismCache - (in/out) IInternetSecurityManager to use

        If ppismCache is NULL, then no cacheing is performed;
        we use a brand new IInternetSecurityManager.

        If ppismCache is non-null, then it used to cache an
        IInternetSecurityManager.  If there is one there already, we
        use it.  If there isn't one there already, we create one and
        save it there.

        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckUrlExCacheW(LPCWSTR pwzUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext,
                        DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms, IInternetSecurityManager ** ppismCache)
{
    HRESULT hr = E_INVALIDARG;

    if (pwzUrl)
    {
        IInternetSecurityManager *psim;

        if (ppismCache && *ppismCache)
        {
            hr = (*ppismCache)->QueryInterface(IID_PPV_ARG(IInternetSecurityManager, &psim));
        }
        else
        {
            hr = _GetCachedZonesManager(IID_PPV_ARG(IInternetSecurityManager, &psim));
            if (SUCCEEDED(hr) && ppismCache)
                psim->QueryInterface(IID_PPV_ARG(IInternetSecurityManager, ppismCache));
        }

        if (SUCCEEDED(hr))
        {
            DWORD dwPolicy = 0;
            DWORD dwContext = 0;

            if (pisms)
                psim->SetSecuritySite(pisms);

            hr = psim->ProcessUrlAction(pwzUrl, dwActionType, 
                                    (BYTE *)(pdwPolicy ? pdwPolicy : &dwPolicy), 
                                    (pdwPolicy ? dwPolicySize : sizeof(dwPolicy)), 
                                    (BYTE *)(pdwContext ? pdwContext : &dwContext), 
                                    (pdwContext ? dwContextSize : sizeof(dwContext)), 
                                    dwFlags, 0);
            TraceMsg(TF_GENERAL, "ZoneCheckUrlExW(\"%ls\") IsFile=%s; NoUI=%s; dwActionType=0x%lx; dwFlags=0x%lx; hr=%lx>",
                     pwzUrl, (dwFlags & PUAF_ISFILE) ? TEXT("Yes") : TEXT("No"),
                     (dwFlags & PUAF_NOUI) ? TEXT("Yes") : TEXT("No"),
                     dwActionType, dwFlags, hr);

            if (pisms)
                psim->SetSecuritySite(NULL);

            psim->Release();
        }
    }
    return hr;
}


/**********************************************************************\
    FUNCTION: ZoneCheckUrlExCacheA

    DESCRIPTION:

        ANSI version of ZoneCheckUrlExCacheW.

        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckUrlExCacheA(LPCSTR pszUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext,
                        DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms, IInternetSecurityManager ** ppismCache)
{
    WCHAR wzUrl[INTERNET_MAX_URL_LENGTH];

    ASSERT(ARRAYSIZE(wzUrl) > lstrlenA(pszUrl));        // We only work for Urls of INTERNET_MAX_URL_LENGTH or shorter.
    SHAnsiToUnicode(pszUrl, wzUrl, ARRAYSIZE(wzUrl));

    return ZoneCheckUrlExCacheW(wzUrl, pdwPolicy, dwPolicySize, pdwContext, dwContextSize, dwActionType, dwFlags, pisms, ppismCache);
}


/**********************************************************************\
    FUNCTION: ZoneCheckUrlExW

    DESCRIPTION:

        Just like ZoneCheckUrlExCacheW, except never caches.

        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckUrlExW(LPCWSTR pwzUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext,
                        DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms)
{
    return ZoneCheckUrlExCacheW(pwzUrl, pdwPolicy, dwPolicySize, pdwContext, dwContextSize, dwActionType, dwFlags, pisms, NULL);
}


/**********************************************************************\
    FUNCTION: ZoneCheckUrlExA

    DESCRIPTION:

        ANSI version of ZoneCheckUrlExW.

        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckUrlExA(LPCSTR pszUrl, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext, DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms)
{
    WCHAR wzUrl[INTERNET_MAX_URL_LENGTH];

    ASSERT(ARRAYSIZE(wzUrl) > lstrlenA(pszUrl));        // We only work for Urls of INTERNET_MAX_URL_LENGTH or shorter.
    SHAnsiToUnicode(pszUrl, wzUrl, ARRAYSIZE(wzUrl));

    return ZoneCheckUrlExW(wzUrl, pdwPolicy, dwPolicySize, pdwContext, dwContextSize, dwActionType, dwFlags, pisms);
}

             
/**********************************************************************\
    FUNCTION: ZoneCheckUrlW

    DESCRIPTION:

        Just like ZoneCheckUrlExW, except that no context or policy
        information are used.

        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckUrlW(LPCWSTR pwzUrl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms)
{
    return ZoneCheckUrlExW(pwzUrl, NULL, 0, NULL, 0, dwActionType, dwFlags, pisms);
}


/**********************************************************************\
    FUNCTION: ZoneCheckUrlA

    DESCRIPTION:
        ANSI version of ZoneCheckUrlW,

        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckUrlA(LPCSTR pszUrl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms)
{
    WCHAR wzUrl[INTERNET_MAX_URL_LENGTH];

    ASSERT(ARRAYSIZE(wzUrl) > lstrlenA(pszUrl));        // We only work for Urls of INTERNET_MAX_URL_LENGTH or shorter.
    SHAnsiToUnicode(pszUrl, wzUrl, ARRAYSIZE(wzUrl));

    return ZoneCheckUrlW(wzUrl, dwActionType, dwFlags, pisms);
}


/**********************************************************************\
    FUNCTION: ZoneCheckPathW

    DESCRIPTION:

        Just like ZoneCheckUrlW, except for filenames instead of URLs.

        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckPathW(LPCWSTR pwzPath, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms)
{
    ASSERT(!PathIsRelativeW(pwzPath));
    return ZoneCheckUrlW(pwzPath, dwActionType, (dwFlags | PUAF_ISFILE), pisms);
}


/**********************************************************************\
    FUNCTION: ZoneCheckPathA

    DESCRIPTION:
        ANSI version of ZoneCheckPathW,

        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckPathA(LPCSTR pszPath, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms)
{
    WCHAR wzPath[INTERNET_MAX_URL_LENGTH];

    ASSERT(ARRAYSIZE(wzPath) > lstrlenA(pszPath));        // We only work for Urls of INTERNET_MAX_URL_LENGTH or shorter.
    SHAnsiToUnicode(pszPath, wzPath, ARRAYSIZE(wzPath));

    return ZoneCheckPathW(wzPath, dwActionType, dwFlags, pisms);
}

/**********************************************************************\
    FUNCTION: ZoneCheckHostEx

    DESCRIPTION:
        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckHostEx(IInternetHostSecurityManager * pihsm, DWORD * pdwPolicy, DWORD dwPolicySize, DWORD * pdwContext,
                        DWORD dwContextSize, DWORD dwActionType, DWORD dwFlags)
{
    HRESULT hr;
    DWORD dwPolicy = 0;
    DWORD dwContext = 0;

    ASSERT(IsFlagClear(dwFlags, PUAF_ISFILE));  // This flag is not appropriate here.
    if (!EVAL(pihsm))
        return E_INVALIDARG;

    hr = pihsm->ProcessUrlAction(dwActionType, 
                            (BYTE *)(pdwPolicy ? pdwPolicy : &dwPolicy), 
                            (pdwPolicy ? dwPolicySize : sizeof(dwPolicy)), 
                            (BYTE *)(pdwContext ? pdwContext : &dwContext), 
                            (pdwContext ? dwContextSize : sizeof(dwContext)), 
                            dwFlags, 0);
    TraceMsg(TF_GENERAL, "ZoneCheckHostEx() NoUI=%s; hr=%lx", (dwFlags & PUAF_NOUI) ? TEXT("Yes") : TEXT("No"), hr);

    return hr;
}


/**********************************************************************\
    FUNCTION: ZoneCheckHost

    DESCRIPTION:
        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
LWSTDAPI ZoneCheckHost(IInternetHostSecurityManager * pihsm, DWORD dwActionType, DWORD dwFlags)
{
    return ZoneCheckHostEx(pihsm, NULL, 0, NULL, 0, dwActionType, dwFlags);
}

/**********************************************************************\
    FUNCTION: ZoneComputePaneSize
    
    DESCRIPTION:
        Computes the necessary size for the zones pane in a status bar.

    NOTES
        The longest zone is the following:

        Width of longest zone name +
        Width of " (Mixed)" +
        Width of small icon (SM_CXSMICON) +
        Width of gripper (SM_CXVSCROLL) +
        Four edges (4 * SM_CXEDGE)

    Why four edges?  Because the rectangle is framed in a DrawEdge(),
    which adds two edges on the left and two on the right, for a total
    of four.

    We cache the results of the font measurements for performance.

\**********************************************************************/

#define ZONES_PANE_WIDTH    220 // Size to use if we are desperate

int _ZoneComputePaneStringSize(HWND hwndStatus, HFONT hf)
{
    HDC hdc = GetDC(hwndStatus);
    HFONT hfPrev = SelectFont(hdc, hf);
    SIZE siz, sizMixed;
    HRESULT hrInit, hr;
    int cxZone;
    ZONEATTRIBUTES za;

    // Start with the length of the phrase " (Mixed)"
    MLLoadStringW(IDS_MIXED, za.szDisplayName, ARRAYSIZE(za.szDisplayName));
    GetTextExtentPoint32WrapW(hdc, za.szDisplayName, lstrlenW(za.szDisplayName), &sizMixed);

    cxZone = 0;

    hrInit = SHCoInitialize();
    IInternetZoneManager *pizm;
    hr = CoCreateInstance(CLSID_InternetZoneManager, NULL, CLSCTX_INPROC_SERVER, IID_IInternetZoneManager, (void **)&pizm);
    if (SUCCEEDED(hr)) {
        DWORD dwZoneEnum, dwZoneCount;
        hr = pizm->CreateZoneEnumerator(&dwZoneEnum, &dwZoneCount, 0);
        if (SUCCEEDED(hr)) {
            for (int nIndex=0; (DWORD)nIndex < dwZoneCount; nIndex++)
            {
                DWORD dwZone;
                za.cbSize = sizeof(ZONEATTRIBUTES);
                pizm->GetZoneAt(dwZoneEnum, nIndex, &dwZone);
                pizm->GetZoneAttributes(dwZone, &za);
                GetTextExtentPoint32WrapW(hdc, za.szDisplayName, lstrlenW(za.szDisplayName), &siz);
                if (cxZone < siz.cx)
                    cxZone = siz.cx;
            }
            pizm->DestroyZoneEnumerator(dwZoneEnum);
        }
        pizm->Release();
    }
    SHCoUninitialize(hrInit);

    SelectFont(hdc, hfPrev);
    ReleaseDC(hwndStatus, hdc);

    // If we couldn't get any zones, then use the panic value.
    if (cxZone == 0)
        return ZONES_PANE_WIDTH;
    else
        return cxZone + sizMixed.cx;
}

LOGFONT s_lfStatusBar;          // status bar font (cached metrics)
int s_cxMaxZoneText;            // size of longest zone text (cached)

LWSTDAPI_(int) ZoneComputePaneSize(HWND hwndStatus)
{
    LOGFONT lf;
    HFONT hf = GetWindowFont(hwndStatus);
    GetObject(hf, sizeof(lf), &lf);

    // Warning:  lf.lfFaceName is an ASCIIZ string, and there might be
    // uninitialized garbage there, so zero-fill it for consistency.
    UINT cchFaceName = lstrlen(lf.lfFaceName);
    ZeroMemory(&lf.lfFaceName[cchFaceName], sizeof(TCHAR) * (LF_FACESIZE - cchFaceName));

    if (memcmp(&lf, &s_lfStatusBar, sizeof(LOGFONT)) != 0)
    {
        ENTERCRITICAL;
        s_cxMaxZoneText = _ZoneComputePaneStringSize(hwndStatus, hf);
        s_lfStatusBar = lf;         // Update the cache
        LEAVECRITICAL;
    }

    return s_cxMaxZoneText + 
           GetSystemMetrics(SM_CXSMICON) +
           GetSystemMetrics(SM_CXVSCROLL) +
           GetSystemMetrics(SM_CXEDGE) * 4;
}

/**********************************************************************\
    FUNCTION: ZoneConfigure
    
    DESCRIPTION:
        Displays the Zones configuration control panel.

        pwszUrl is used to specify which zone is chosen as default.
        Inetcpl will choose the zone that the URL belongs to.

\**********************************************************************/

#define MAX_CPL_PAGES   16

BOOL CALLBACK _ZoneAddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < MAX_CPL_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}

LWSTDAPI_(void) ZoneConfigureW(HWND hwnd, LPCWSTR pwszUrl)
{
    HMODULE hModInetCpl;

    if (hModInetCpl = LoadLibrary(TEXT("inetcpl.cpl")))
    {
        PFNADDINTERNETPROPERTYSHEETSEX pfnAddSheet = (PFNADDINTERNETPROPERTYSHEETSEX)GetProcAddress(hModInetCpl, STR_ADDINTERNETPROPSHEETSEX);
        if (pfnAddSheet)
        {
            IEPROPPAGEINFO iepi = {SIZEOF(iepi)};
            // Load the current url into the properties page
            CHAR szBufA[INTERNET_MAX_URL_LENGTH];
            SHUnicodeToAnsi(pwszUrl, szBufA, ARRAYSIZE(szBufA));
            iepi.pszCurrentURL = szBufA;

            PROPSHEETHEADER psh;
            HPROPSHEETPAGE rPages[MAX_CPL_PAGES];

            psh.dwSize = SIZEOF(psh);
            psh.dwFlags = PSH_PROPTITLE;
            psh.hInstance = MLGetHinst();
            psh.hwndParent = hwnd;
            psh.pszCaption = MAKEINTRESOURCE(IDS_INTERNETSECURITY);
            psh.nPages = 0;
            psh.nStartPage = 0;
            psh.phpage = rPages;

             // we just want the security page.
            iepi.dwFlags = INET_PAGE_SECURITY;

            pfnAddSheet(_ZoneAddPropSheetPage, (LPARAM)&psh, 0, 0, &iepi);

            //
            // Display the property sheet only if the "security" page was 
            // successfully added (it will fail if an IEAK setting says so)
            //
            if (psh.nPages > 0)
            {
                PropertySheet(&psh);
            }
            else
            {
                SHRestrictedMessageBox(hwnd);
            }
        }
        FreeLibrary(hModInetCpl);
    }
}

/**********************************************************************\
    DESCRIPTION:
        Registers or validates an htt/htm template with the shell.

        The WebView customization wizard and the code that installs the default
        WebView templates calls this API to register the templates.

        The shell object model uses this API to grant privileges to execute
        unsafe method calls (e.g. SHELL.APPLICATION) to templates registered
        with this API.  If they aren't registered, they can't call the unsafe methods.

\**********************************************************************/

#define REGSTR_TEMPLATE_REGISTRY (REGSTR_PATH_EXPLORER TEXT("\\TemplateRegistry"))
#define REGSTR_VALUE_KEY (TEXT("Value"))

BOOL GetTemplateValueFromReg(LPTSTR pszValue, DWORD *pdwValue)
{
    DWORD cbValue = sizeof(DWORD);
    BOOL bSuccess;

    if (!(bSuccess = (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_TEMPLATE_REGISTRY, pszValue, NULL, pdwValue, &cbValue))))
    {
        cbValue = sizeof(DWORD);
        bSuccess = (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_TEMPLATE_REGISTRY, pszValue, NULL, pdwValue, &cbValue));
    }
    return bSuccess;
}

BOOL SetTemplateValueInReg(LPTSTR pszValue, DWORD *pdwValue)
{
    return ((ERROR_SUCCESS == SHSetValue(HKEY_LOCAL_MACHINE, REGSTR_TEMPLATE_REGISTRY, pszValue, REG_DWORD, pdwValue, sizeof(pdwValue))) ||
            (ERROR_SUCCESS == SHSetValue(HKEY_CURRENT_USER, REGSTR_TEMPLATE_REGISTRY, pszValue, REG_DWORD, pdwValue, sizeof(pdwValue))));
}

HRESULT GetTemplateInfoFromHandle(HANDLE h, UCHAR * pKey, DWORD *pdwSize)
{
    HRESULT hres = E_FAIL;
    DWORD  dwSize = GetFileSize(h, NULL);
    LPBYTE pFileBuff = (LPBYTE)LocalAlloc(0, dwSize);
    if (pFileBuff)
    {
        DWORD dwBytesRead;
        if (ReadFile(h, pFileBuff, dwSize, &dwBytesRead, NULL))
        {
            MD5_CTX md5;

            MD5Init(&md5);
            MD5Update(&md5, pFileBuff, dwBytesRead);
            MD5Final(&md5);

            memcpy(pKey, md5.digest, MD5DIGESTLEN);
            *pdwSize = dwSize;
            hres = S_OK;
        }
        LocalFree(pFileBuff);
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

// in:
//      pszPath         URL or file system path
// return:
//      S_OK            if pszPath is in the local zone
//      E_ACCESSDENIED  we are not in a local zone

STDAPI LocalZoneCheckPath(LPCWSTR pszPath)
{
    HRESULT hr = E_ACCESSDENIED;
    IInternetSecurityManager *pSecMgr;
    if (SUCCEEDED(_GetCachedZonesManager(IID_PPV_ARG(IInternetSecurityManager, &pSecMgr)))) 
    {
        DWORD dwZoneID = URLZONE_UNTRUSTED;
        if (SUCCEEDED(pSecMgr->MapUrlToZone(pszPath, &dwZoneID, 0))) 
        {
            if (dwZoneID == URLZONE_LOCAL_MACHINE)
            {
                hr = S_OK;      // we are good
            }
        }       
        pSecMgr->Release();
    }
    return hr;
}

// this API takes a Win32 file path
// in:
//      dwFlags     SHRVT_ falgs in shlwapi.h
// out:
//      S_OK        happy

LWSTDAPI SHRegisterValidateTemplate(LPCWSTR pszPath, DWORD dwFlags)
{
    if ((dwFlags & SHRVT_VALID) != dwFlags)
        return E_INVALIDARG;

    HRESULT hr = (dwFlags & SHRVT_VALIDATE) ? LocalZoneCheckPath(pszPath) : S_OK;
    if (S_OK == hr)
    {
        DWORD dwSize;
        UCHAR pKey[MD5DIGESTLEN];

        HANDLE hfile;
        if (g_bRunningOnNT)
        {
            hfile = CreateFileW(pszPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        }
        else
        {
            CHAR szTemp[MAX_PATH];
            SHUnicodeToAnsi(pszPath, szTemp, ARRAYSIZE(szTemp));
            hfile = CreateFileA(szTemp, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        }

        if (INVALID_HANDLE_VALUE != hfile)
        {
            hr = GetTemplateInfoFromHandle(hfile, pKey, &dwSize);
            CloseHandle(hfile);
        }
        else
            hr = E_INVALIDARG;

        if (SUCCEEDED(hr))
        {
            BOOL bSuccess;
            TCHAR szTemplate[MAX_PATH];

            DWORD *pdw = (DWORD *)pKey;

            ASSERT(MD5DIGESTLEN == (4 * sizeof(DWORD)));
        
            wsprintf(szTemplate, TEXT("%u%u%u%u"), pdw[0], pdw[1], pdw[2], pdw[3]);

            if (dwFlags & SHRVT_VALIDATE)
            {
                DWORD dwSizeReg;
                bSuccess = (GetTemplateValueFromReg(szTemplate, &dwSizeReg) && (dwSizeReg == dwSize));
                if (!bSuccess && (dwFlags & SHRVT_PROMPTUSER))
                {
                    MSGBOXPARAMS mbp = {sizeof(MSGBOXPARAMS), NULL, g_hinst, MAKEINTRESOURCE(IDS_TEMPLATENOTSECURE), MAKEINTRESOURCE(IDS_SECURITY),
                                        MB_YESNO | MB_DEFBUTTON2 | MB_TASKMODAL | MB_USERICON, MAKEINTRESOURCE(IDI_SECURITY), 0, NULL, 0};

                    // BUGBUG: posting a msg box with NULL hwnd, this should
                    // could use a site pointer to get an hwnd to go modal against
                    // if one was provided to the API

                    bSuccess = (MessageBoxIndirect(&mbp) == IDYES);

                    if (bSuccess && (dwFlags & SHRVT_REGISTERIFPROMPTOK))
                        SetTemplateValueInReg(szTemplate, &dwSize);
                }
            }
            else  if (dwFlags & SHRVT_REGISTER)
            {
                bSuccess = SetTemplateValueInReg(szTemplate, &dwSize);
            }
            hr = bSuccess ? S_OK : E_ACCESSDENIED;
        }
    }
    return hr;
}


