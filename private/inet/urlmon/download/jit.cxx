// ===========================================================================
// File: JIT.CXX
//    The Just In Time setup of IE addon components
//

#include <cdlpch.h>
#include <mshtmhst.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <inseng.h>

#ifdef UNIX
#include <unixfile.h>
#endif /* UNIX */

HRESULT
GetComponentIDFromIEFeature(LPCSTR pszIEFeature, LPSTR *ppszComponentID)
{

    HRESULT hr = S_OK;
    HKEY hkeyFeatureComponentID =0;
    DWORD lResult;
    char szComponentID[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    DWORD dwType;


    *ppszComponentID = NULL;

    if ((lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
        REGKEY_ACTIVESETUP_FEATURECOMPID, 0, KEY_READ,
        &hkeyFeatureComponentID)) != ERROR_SUCCESS) {

        hr = HRESULT_FROM_WIN32(lResult);
        goto Exit;
    }


    szComponentID[0] = '\0';
    if (SHQueryValueEx(hkeyFeatureComponentID, pszIEFeature, NULL, &dwType, 
                        (unsigned char *)&szComponentID, &dwSize) == ERROR_SUCCESS) {

        *ppszComponentID = new char [dwSize+1];
        if (*ppszComponentID == NULL) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        lstrcpy(*ppszComponentID, szComponentID);

        // FALLTHROUGH to exit

    } else {
        hr = E_UNEXPECTED;
        // FALLTHROUGH to exit
    }

Exit:

    if (hkeyFeatureComponentID)
        ::RegCloseKey(hkeyFeatureComponentID);

    return hr;
}


HRESULT GetComponentIDFromCLSSPEC(uCLSSPEC *pClassspec, LPSTR * ppszComponentID)
{
    HRESULT hr = S_OK;
    *ppszComponentID = NULL;
    LPWSTR pwszIEFeature = NULL;
    LPSTR pszIEFeature = NULL;
    LPCWSTR pwszMimeType = NULL;
    CLSID inclsid = CLSID_NULL;
    QUERYCONTEXT qcInternal;
    memset(&qcInternal, 0, sizeof(QUERYCONTEXT));
    BOOL bUnknown = FALSE;
    int iRes = 0;
    int length = 0;

    switch(pClassspec->tyspec)
    {
        case TYSPEC_PROGID:

            // BUGBUG: we don't have JIT tables for progid to feature id
            // so this will only work for PEEK for installed versions
            if (FAILED((hr=CLSIDFromProgID(pClassspec->tagged_union.pProgId, 
                    &inclsid))))
                goto Exit;

            hr = GetIEFeatureFromClass(&pwszIEFeature, inclsid, &qcInternal);
            if (hr != S_OK)
                goto Exit;

            break;

        case TYSPEC_CLSID:
            inclsid = pClassspec->tagged_union.clsid;
            hr = GetIEFeatureFromClass(&pwszIEFeature, inclsid, &qcInternal);

            if (hr != S_OK)
                goto Exit;

            break;

        case TYSPEC_MIMETYPE:
            pwszMimeType = (LPCWSTR) pClassspec->tagged_union.pMimeType;
            hr = GetIEFeatureFromMime(&pwszIEFeature, pwszMimeType, &qcInternal);

            if (hr != S_OK)
                goto Exit;

            break;

        case TYSPEC_FILENAME:

            // overload filename to be Active Setup feature ID
            hr = CDLDupWStr( &pwszIEFeature, pClassspec->tagged_union.pFileName);
            if (hr != S_OK)
                goto Exit;

            break;

        default:
            hr = E_INVALIDARG;
            goto Exit;
    }

    if(pwszIEFeature == NULL)
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // Change the wide character IEFeature to a multibyte string
    length = lstrlenW(pwszIEFeature) + 1;
    pszIEFeature = new CHAR[length];
    if(pszIEFeature == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    bUnknown = FALSE;
    iRes = WideCharToMultiByte(CP_ACP, 0, pwszIEFeature,length, 
                                   pszIEFeature, length,NULL,&bUnknown);
    // unknown multibyte character
    if(bUnknown == TRUE)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
    // failed for other reason
    if(iRes == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    
    // Get a ComponentID from the FeatureID
    hr = GetComponentIDFromIEFeature(pszIEFeature, ppszComponentID);
    if(FAILED(hr))
    {
        goto Exit;
    }

    hr = S_OK;

Exit:
    SAFEDELETE(pszIEFeature);
    SAFEDELETE(pwszIEFeature);
    return hr;
}


#define FEATURE_VERSION_SEPARATOR       ';'
#define ALT_FEATURE_VERSION_SEPARATOR   '!'

// if the IE Feature for this mime/clsid has an IE recommended version
// then modify the querycontext to reflect this as if the caller wanted this
// version. If the caller already has specifies a version that overrides
// the recommnedation.
// format: featurename;a,b,c,d

HRESULT
ParseIEFeature(LPSTR pszIEFeatureWithVersion,
    QUERYCONTEXT *pQuery,
    LPWSTR *ppwszIEFeature)
{
    HRESULT hr = S_OK;
    DWORD dwIERecommendedVersionHi = 0;
    DWORD dwIERecommendedVersionLo = 0;

    LPSTR pchVersion = StrChr(pszIEFeatureWithVersion, FEATURE_VERSION_SEPARATOR);

    if (!pchVersion) {
        pchVersion = StrChr(pszIEFeatureWithVersion, ALT_FEATURE_VERSION_SEPARATOR);
    }

    if (pchVersion) {

        *pchVersion = '\0';  // stomp version separator so 
                            // pszIEFetaureWithVersion has just the feature name
        pchVersion++;

        if (*pchVersion && (!(pQuery->dwVersionHi|pQuery->dwVersionLo)) ) {
            // now pointing at version string
            // of format a,b,c,d

            hr = GetVersionFromString(pchVersion,
                    &dwIERecommendedVersionHi, &dwIERecommendedVersionLo);

            if ( SUCCEEDED(hr)) {

                pQuery->dwVersionHi = dwIERecommendedVersionHi;
                pQuery->dwVersionLo = dwIERecommendedVersionLo;
            }

        }
    }

    if ( SUCCEEDED(hr) && ppwszIEFeature) 
        hr=Ansi2Unicode(pszIEFeatureWithVersion, ppwszIEFeature);

    return hr;
}

HRESULT
GetIEFeatureFromMime(LPWSTR *ppwszIEFeature, LPCWSTR pwszMimeType, QUERYCONTEXT *pQuery)
{

    HRESULT hr = S_OK;
    HKEY hkeyMimeFeature =0;
    LPSTR pszMime = NULL;
    DWORD lResult;
    char szFeature[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    DWORD dwType;

    if (FAILED((hr=::Unicode2Ansi(pwszMimeType, &pszMime))))
    {
        goto Exit;
    }

    if ((lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
        REGKEY_ACTIVESETUP_MIMEFEATURE, 0, KEY_READ,
        &hkeyMimeFeature)) != ERROR_SUCCESS) {

#if JIT_IN_BUILD
        hr = HRESULT_FROM_WIN32(lResult);
#else
        hr = S_FALSE;
#endif
        goto Exit;
    }


    szFeature[0] = '\0';
    if (SHQueryValueEx(hkeyMimeFeature, pszMime, NULL, &dwType, 
                        (unsigned char *)szFeature, &dwSize) == ERROR_SUCCESS) {

        if (dwSize > MAX_PATH) {
            // limit the feature name to MAX_PATH as buffer overrun protection
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            goto Exit;
        }

        hr = ParseIEFeature(szFeature, pQuery, ppwszIEFeature);

        // FALLTHROUGH to exit

    } else {
        hr = S_FALSE;
        // FALLTHROUGH to exit
    }

Exit:

    if (hkeyMimeFeature)
        ::RegCloseKey(hkeyMimeFeature);

    return hr;
}


HRESULT
GetIEFeatureFromClass(LPWSTR *ppwszIEFeature, REFCLSID clsid, QUERYCONTEXT *pQuery)
{

    HRESULT hr = S_OK;
    LPOLESTR pwcsClsid = NULL;
    LPSTR pszClsid = NULL;
    HKEY hkeyClsidFeature =0;
    DWORD lResult;
    char szFeature[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    DWORD dwType;


    // return if we can't get a valid string representation of the CLSID
    if (FAILED((hr=StringFromCLSID(clsid, &pwcsClsid))))
        goto Exit;

    if (FAILED((hr=::Unicode2Ansi(pwcsClsid, &pszClsid))))
    {
        goto Exit;
    }

    if ((lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
        REGKEY_ACTIVESETUP_CLSIDFEATURE, 0, KEY_READ,
        &hkeyClsidFeature)) != ERROR_SUCCESS) {
#if JIT_IN_BUILD
        hr = HRESULT_FROM_WIN32(lResult);
#else
        hr = S_FALSE;
#endif
        goto Exit;
    }


    szFeature[0] = '\0';
    if (SHQueryValueEx(hkeyClsidFeature, pszClsid, NULL, &dwType, 
                        (unsigned char *)szFeature, &dwSize) == ERROR_SUCCESS) {

        if (dwSize > MAX_PATH) {
            // limit the feature name to MAX_PATH as buffer overrun protection
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            goto Exit;
        }

        hr = ParseIEFeature(szFeature, pQuery, ppwszIEFeature);
        // FALLTHROUGH to exit

    } else {
        hr = S_FALSE;
        // FALLTHROUGH to exit
    }


Exit:

    if (hkeyClsidFeature)
        ::RegCloseKey(hkeyClsidFeature);

    SAFEDELETE(pwcsClsid);
    SAFEDELETE(pszClsid);

    return hr;
}


HRESULT
IsActiveSetupFeatureLocallyInstalled(
    LPCSTR pszDist,
    QUERYCONTEXT *pQuery,
    LPSTR *ppszComponentID,
    DWORD *pdwVersionHi,
    DWORD *pdwVersionLo,
    DWORD dwFlags)
{
    HRESULT hr = S_FALSE;       // Not installed by active setup.
    LONG    lResult = ERROR_SUCCESS;
    char    szKey[2*MAX_PATH];
    char    szVersion[MAX_PATH];
    HKEY    hKey    = NULL;
    DWORD   dwSize;
    DWORD   dwValue;
    DWORD   dwType;
    BOOL    fIsInstalled = FALSE;
    WORD    wVersion[4];
    DWORD   dwCurMS = 0;
    DWORD   dwCurLS = 0;

    const static char * szLocale = "Locale";
    const static char * szIsInstalled = "IsInstalled";
    const static char * szActVersion = "Version";
    const static char * szCOMPONENTID = "ComponentID";
    const static char * szKEYFILENAME = "KeyFileName";

    if (pdwVersionHi && pdwVersionLo) {
        *pdwVersionHi = 0;
        *pdwVersionLo = 0;
    }

    if (ppszComponentID) { // if we need a componentID
        GetComponentIDFromIEFeature(pszDist, ppszComponentID);
    }

    // buffer overrun: assumes that lstrlen(pszDist) < MAX_PATH
    lstrcpy(szKey, REGKEY_ACTIVESETUP_COMPONENTS);
    lstrcat(szKey, "\\");
    lstrcat(szKey, pszDist);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {

        dwSize = sizeof(dwValue);
        // Old format of the Installed components did not have the IsInstalled value.
        if (RegQueryValueEx( hKey, szIsInstalled, NULL, NULL, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS)
        {
            dwValue = 0;
        }
        fIsInstalled = (dwValue != 0);
        if (fIsInstalled)
        {
            hr = S_OK;

        } else {
            // hr is S_FALSE;
            goto Exit;
        }

        if (pQuery && ((pQuery->dwVersionHi|pQuery->dwVersionLo) ||
                        (dwFlags & FIEF_FLAG_PEEK)) ) {

            dwSize = sizeof(szVersion);
            if ( (lResult = RegQueryValueEx(hKey, szActVersion, NULL, &dwType, (LPBYTE)szVersion, &dwSize)) == ERROR_SUCCESS )
            {
                Assert(dwType == REG_SZ);

                if ( SUCCEEDED(GetVersionFromString(szVersion, &dwCurMS, &dwCurLS)))
                {
                    if (dwCurMS > pQuery->dwVersionHi ||
                     (dwCurMS == pQuery->dwVersionHi && dwCurLS >= pQuery->dwVersionLo)) {
                        hr = S_OK;
                    } else {
                        hr = S_FALSE;
                    }

                    if (pdwVersionHi && pdwVersionLo) {
                        *pdwVersionHi = dwCurMS;
                        *pdwVersionLo = dwCurLS;
                    }

                } else {
                    hr = S_FALSE;
                }

            } else {
                hr = S_FALSE;
            }
        }



        if (hr == S_OK) {

            // we think it is installed
            // robustify with key file presence check
            dwSize = sizeof(szVersion);
            dwType = REG_SZ;
            if ( SHQueryValueEx(hKey, szKEYFILENAME, NULL, &dwType, (LPBYTE)szVersion, &dwSize) == ERROR_SUCCESS ) {
                // value present, check if file is present
                if (GetFileAttributes(szVersion) == -1)
                    hr = S_FALSE;
            }
        }
    }

Exit:

    SAFEREGCLOSEKEY(hKey);
    return hr;
}

typedef HRESULT (*PFNGETICIFFILEFROMFILE)(ICifFile **, LPCSTR pszFile);

HRESULT
IsCIFVersionGoodEnough(
    QUERYCONTEXT *pQuery,
    LPCSTR pszID)
{
    // BUGBUG: not side by side aware!!!
    const static char * szCIFPATH = "iesetup.cif";

    HRESULT hr = S_OK;
    ICifFile *pcif;
    ICifComponent *pcomp;
    DWORD dwVersionHi = 0;
    DWORD dwVersionLo = 0;
    PFNGETICIFFILEFROMFILE pfnGetICifFileFromFile = NULL;
    HMODULE hMod = LoadLibrary("inseng.dll");

    if ( !hMod ||
        (!(pfnGetICifFileFromFile = (PFNGETICIFFILEFROMFILE)GetProcAddress(hMod,
        "GetICifFileFromFile")))) {

        return E_UNEXPECTED;
    }

    if(SUCCEEDED(pfnGetICifFileFromFile(&pcif, szCIFPATH)))   // szCifPath is full path to cif, or 8.3 and it defaults to look in ie dir for it
    {
       if(SUCCEEDED(pcif->FindComponent(pszID, &pcomp)))   // pszID is id of what you are interested in
       {
          pcomp->GetVersion(&dwVersionHi, &dwVersionLo);
          // BUGBUG: ?
          // pcomp->Release();

        if (pQuery->dwVersionHi > dwVersionHi ||
         (dwVersionHi == pQuery->dwVersionHi && pQuery->dwVersionLo > dwVersionLo)) {
            hr = HRESULT_FROM_WIN32(ERROR_UNKNOWN_REVISION);
        }

       }
       pcif->Release();
    }


    if (hMod)
        FreeLibrary(hMod);

    return hr;
}


/* Returns:
 *
 *      S_OK:    Is Installed
 *      S_FALSE: Is Available, Needs to be faulted in.
 *      ERROR
 *
 */

HRESULT
IsIEFeatureInstalled(
    LPCWSTR pwszIEFeature,
    QUERYCONTEXT *pQuery,
    LPWSTR *ppwszSetupPage,
    LPWSTR *ppwszCompID,
    DWORD dwFlags
    )
{
    HRESULT hr = S_FALSE;   // assume need to fault in
    LPSTR pszIEFeature = NULL;
    LPSTR pszComponentID = NULL;
    const static char * szSETUPPAGE = "JITSetupPage";
    const static char * szQUERYFEATURE = "feature=";
    LONG lResult;
    HKEY hkeyActiveSetup = 0;
    char szJITPage[INTERNET_MAX_URL_LENGTH];
    DWORD dwType;
    DWORD dwSize = INTERNET_MAX_URL_LENGTH;
    DWORD dwInstalledVersionHi =0;
    DWORD dwInstalledVersionLo =0;
    BOOL  bComponentDeclined = FALSE;

#ifdef UNIX
    {
        /* Unix special handling for features like java VM
         * For instance, libmsjava.so will always be installed.
         * But, is the Sun VM installed?
         */
        hr = CheckIEFeatureOnUnix(pwszIEFeature, &dwInstalledVersionHi, &dwInstalledVersionLo, dwFlags);
        if (hr == S_OK || hr == ERROR_PRODUCT_UNINSTALLED)
           goto Exit; /* Found on Unix and Handled */
    }
#endif /* UNIX */


    if (FAILED((hr=::Unicode2Ansi(pwszIEFeature, &pszIEFeature))))
    {
        goto Exit;
    }

    hr = IsActiveSetupFeatureLocallyInstalled(pszIEFeature, pQuery, &pszComponentID, &dwInstalledVersionHi, &dwInstalledVersionLo, dwFlags);

    if (hr == S_FALSE) {

        // not installed, need to JIT
        // find out if this is JITDeclined

        if (SHRegGetUSValue( REGKEY_DECLINED_IOD, pszIEFeature, NULL, NULL, NULL, 0,NULL,0) == ERROR_SUCCESS)
        {
            bComponentDeclined = TRUE;
        }
    }


    if (((hr != S_OK) || dwFlags & FIEF_FLAG_SKIP_INSTALLED_VERSION_CHECK)
        && ppwszSetupPage && ppwszCompID) {

        if ((lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
            REGKEY_ACTIVESETUP, 0, KEY_READ,
            &hkeyActiveSetup)) != ERROR_SUCCESS) {

            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }


        szJITPage[0] = '\0';
        if (SHQueryValueEx(hkeyActiveSetup, szSETUPPAGE, NULL, &dwType, 
            (unsigned char *)szJITPage, &dwSize) == ERROR_SUCCESS) {

                HRESULT hr1 = ::Ansi2Unicode(szJITPage, ppwszSetupPage);
                if(FAILED(hr1))
                    hr = hr1;       // else hr still = S_FALSE;

                if (pszComponentID) {

                    if ((sizeof(szQUERYFEATURE) + lstrlen(pszComponentID) +1)
                        < INTERNET_MAX_URL_LENGTH){

                        lstrcpy(szJITPage, szQUERYFEATURE);

                        lstrcat(szJITPage, pszComponentID);

                        if ( (dwFlags & FIEF_FLAG_CHECK_CIFVERSION) &&
                                pQuery && 
                                (pQuery->dwVersionHi|pQuery->dwVersionLo)) {

                            hr1=IsCIFVersionGoodEnough(pQuery, pszComponentID);
                            if (FAILED(hr1)) {
                                hr = hr1;
                                goto Exit;
                            }
                        }

                        HRESULT hr1 = ::Ansi2Unicode(szJITPage, ppwszCompID);
                        if(FAILED(hr1))
                            hr = hr1;       // else hr still = S_FALSE;

                    } else {
                        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                    }

                }

        }


    }


Exit:

    if (pQuery && (dwFlags & FIEF_FLAG_PEEK)) {
        pQuery->dwVersionHi = dwInstalledVersionHi;
        pQuery->dwVersionLo = dwInstalledVersionLo;
    }

    SAFEDELETE(pszIEFeature);
    SAFEDELETE(pszComponentID);

    if (hkeyActiveSetup)
        RegCloseKey(hkeyActiveSetup);

    if (bComponentDeclined) {
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    return hr;
}
    

HRESULT
IsJITRestricted()
{

    HKEY hkeyRest = 0;
    BOOL bJITRest = FALSE;
    DWORD dwValue = 0;
    DWORD dwLen = sizeof(DWORD);
    HRESULT hr = S_OK;
    OSVERSIONINFO osvi;

    // BUGBUG: NT RAID #216898--when you install IE5 and the shell cabs
    // on NT5, the NoJITSetup reg value is blown away, but we can't figure
    // out who is doing this. JIT should not occur for NT5. This hack
    // is a work-around so JIT does not occur under NT5 and above.


    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
        osvi.dwMajorVersion >= 5) {
        hr = E_ACCESSDENIED;
        goto Exit;
    }

    // per-machine JIT off policy
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_INFODEL_REST, 0, KEY_READ, &hkeyRest) == ERROR_SUCCESS) {

        if (RegQueryValueEx( hkeyRest, REGVAL_JIT_REST, NULL, NULL,
                      (LPBYTE)&dwValue, &dwLen) == ERROR_SUCCESS && dwValue)
            hr = E_ACCESSDENIED;

        RegCloseKey(hkeyRest);
    }

    if (SUCCEEDED(hr)) {

        // Check to see if the user has turned it off under advanced options
        // make this appear in UI similar to the admin restricted case
        // hence the return value of access_denied
        dwValue = 0;
        dwLen = sizeof(DWORD);
        if (SHRegGetUSValue( REGSTR_PATH_IE_MAIN, REGVAL_JIT_REST, NULL, (LPBYTE)&dwValue, &dwLen, 0,NULL,0) == ERROR_SUCCESS && dwValue)
                hr = E_ACCESSDENIED;
    }

Exit:
    return hr;
}
    
HRESULT
SetJITDeclined(
    LPCWSTR pwszIEFeature)
{
    HRESULT hr = S_FALSE;   // assume need to fault in
    LPSTR pszIEFeature = NULL;
    LONG    lResult = ERROR_SUCCESS;
    HKEY    hKey    = NULL;
    DWORD   dwSize;
    DWORD   dwValue;
    LPSTR szNull = "";

    if (FAILED((hr=::Unicode2Ansi(pwszIEFeature, &pszIEFeature))))
    {
        goto Exit;
    }


    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_DECLINED_IOD, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
    {
        if ((lResult = RegCreateKey( HKEY_CURRENT_USER,
                   REGKEY_DECLINED_IOD, &hKey)) != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }
    }

    if (((lResult = RegSetValueEx (hKey, pszIEFeature, 0, REG_SZ,
            (unsigned char *)szNull, 1))) != ERROR_SUCCESS) {

        hr = HRESULT_FROM_WIN32(lResult);
    }


Exit:

    if (hKey)
        RegCloseKey(hKey);

    SAFEDELETE(pszIEFeature);
    return hr;
}

HRESULT
HasFeatureAlreadyBeenRejected(
    LPCWSTR szDistUnit)
{
    HRESULT hr = S_OK;
    LISTPOSITION curpos;
    LPCWSTR pwszRejectedFeature = NULL;
    int iNumRejected;
    int i;

    CUrlMkTls tls(hr); // hr passed by reference!

    if (FAILED(hr))
        goto Exit;

    iNumRejected = tls->pRejectedFeaturesList->GetCount();
    curpos = tls->pRejectedFeaturesList->GetHeadPosition();

    // walk thru all the rejected features in the thread and check for match
    for (i=0; i < iNumRejected; i++) {

        pwszRejectedFeature = tls->pRejectedFeaturesList->GetNext(curpos);

        if (StrCmpIW(szDistUnit, pwszRejectedFeature) == 0) {

            hr = S_FALSE;
            break;
        }
    }

Exit:

    return hr;
}

HRESULT
RelinquishCodeDownloadCookies()
{
    HRESULT hr = S_OK;

    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))
        goto Exit;

    Assert(tls->pTrustCookie);
    Assert(tls->pSetupCookie);

    if (!tls->pTrustCookie || !tls->pSetupCookie) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    tls->pSetupCookie->JITRelinquish();
    tls->pTrustCookie->JITRelinquish();

Exit:

    return hr;
}

HRESULT
AcquireCodeDownloadCookies()
{
    HRESULT hr = S_OK;
    HRESULT hrJITTrustCookie = E_FAIL;
    HRESULT hrJITSetupCookie = E_FAIL;

    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))
        goto Exit;

    // are both cookies free?
    if (!(tls->pTrustCookie->IsFree() && tls->pSetupCookie->IsFree())) {
        hr = E_FAIL;
        goto Exit;
    }

    // if both cookies are free, grab them
    // thius will ensure that code downloads are
    // frozen or pended to the point JIT completes

    hrJITTrustCookie = tls->pTrustCookie->JITAcquire();

    if (FAILED(hrJITTrustCookie)) {
        Assert(TRUE);
        hr = hrJITTrustCookie;
        goto Exit;
    }

    hrJITSetupCookie = tls->pSetupCookie->JITAcquire();
    if (FAILED(hrJITSetupCookie)) {
        Assert(TRUE);
        hr = hrJITSetupCookie;
        goto Exit;
    }

    // taken both the cookies

Exit:

    if (FAILED(hr)) {

        if (SUCCEEDED(hrJITSetupCookie))
            tls->pSetupCookie->JITRelinquish();

        if (SUCCEEDED(hrJITTrustCookie))
            tls->pTrustCookie->JITRelinquish();
    }

    return hr;
}

HRESULT
GetDialogArgs(LPCWSTR pwszCompID, DWORD dwFlags, VARIANT& vtDialogArg)
{
    HRESULT hr = S_OK;
    WCHAR pwszFLAGS[] = L"&flag=forceUI";
    WCHAR pwszArgs[INTERNET_MAX_URL_LENGTH];// large buffer


    StrCpyNW(pwszArgs, pwszCompID, INTERNET_MAX_URL_LENGTH);

    if (dwFlags & FIEF_FLAG_FORCE_JITUI) {

        StrCatBuffW(pwszArgs, pwszFLAGS, INTERNET_MAX_URL_LENGTH);
    }

    vtDialogArg.bstrVal =  SysAllocString(pwszArgs);

    ASSERT(vtDialogArg.bstrVal); 
    if(vtDialogArg.bstrVal)
    {
        vtDialogArg.vt = VT_BSTR;   
    }
    else
    {
        hr =  E_OUTOFMEMORY;
        goto Exit;
    }

Exit:

    return hr;
}


/* 
 * FaultInIEFeature
 *
 * Description:
 *
 *      Synchronous API that is to be invoked by client of an IE feature before
 *      accessing the feature. If the feature is already installed then it 
 *      succeeds and the client should attempt to access the feature. Success
 *      return by the API does not guarantee that the feature is fully installed
 *      or will work. The client should still robustify access to the feature 
 *      with proper error checking.
 *      If the API detects that the feature is missing on the client then it
 *      maps the feature identified by a clsid or mimetype (in the classspec)
 *      into a feature name/id and then invokes a HTML dialog with the feature
 *      name in the form of http://...../setuppage.htm?FeatureName=<featurename>
 *      The page can communicate back a return value thru 
 *      window.external.returnValue. The following are recognized now:
 *
 *  #define JITPAGE_RETVAL_SUCCESS              0x0 // successfully installed
 *  #define JITPAGE_RETVAL_CANCELLED            0x1 // was cancelled by user
 *  #define JITPAGE_RETVAL_DONTASK_THISWINDOW   0x2 // don't ask again in this
 *
 * Parameters:
 *
 *      hWnd:           parent hwnd for htmldialog
 *      pClassSpec:     ptr to union {clsid, mimetype ...} 
 *      querycontext:   to specify version, lcid of feature(lcid not impl)
 *                      This is an IN/OUT param. The installed version
 *                      number is returned is FIEF_FLAG_PEEK is passed in
 *      dwFlags:        control behaviour such as UI/no UI, peek versus faultin
 *                      FIEF_FLAG_PEEK
 *
 *              FIEF_FLAG_FORCE_JITUI
 *                  Force JIT even if user has cancelled a previous JIT in
 *                  the same session or even asked to Never JIT this feature.
 *
 *              FIEF_FLAG_PEEK
 *                  Don't fault in just peek. Peek has the side effect of
 *                  returning currently installed version in the querycontext
 *
 *              FIEF_FLAG_SKIP_INSTALLED_VERSION_CHECK
 *                  Ignore local version being satisfactory and force JIT
 *                  download. Typically called by code download or other caller
 *                  after a CoCreateInstance call has failed with
 *                  REGDB_E_CLASSNOTREG or ERROR_MOD_NOT_FOUND (missing
 *                  dependency dll). The registry probably still looks like
 *                  this feature is installed, but really isn't or is damaged.
 *
 *
 * Returns:
 *
 *      S_OK:    Is Installed. Caller needs to CoCreate or BindToObject or some
 *               other system service to invoke the class or mime handler.
 *      S_FALSE: Class or Mime is not part of an IE feature. Caller needs to
 *               CoCreate or BindToObject or some other system service to 
 *               invoke the class or mime handler.
 *      E_ACCESSDENIED
 *               Admin has turned off JIT feature.
 *      HRESULT_FROM_WIN32(ERROR_CANCELLED):
 *               Could have been cancelled by the user or has been requested
 *               by user not to bother again in this browser window.
 *      HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED):
 *               Returned if the feature is not installed, but FIEF_FLAG_PEEK
 *               was specified
 *      HRESULT_FROM_WIN32(ERROR_REVISION_UNKNOWN);
 *               Returned when dwFlag FIEF_FLAG_CHECK_CIFVERSION is passed and
 *               the local version is not good enough AND the version in the CIF
 *               that would get JIT installed is also less then the version
 *               requested by the caller. This flag is typically passed by a
 *               caller such as code download in URLMON that has the means to
 *               get the requested version by installing the CODEBASE object.
 *
 *      ERROR:
 *               Other error.
 *
 */

STDAPI
FaultInIEFeature(
    HWND         hWnd,
    uCLSSPEC     *pClassSpec,
    QUERYCONTEXT *pQuery,
    DWORD        dwFlags
    )
{
    HRESULT hr = S_OK;
    HRESULT hrCDLCookies=E_FAIL;
    WCHAR *pwszMUISetupFile = NULL;
    WCHAR pwszMUISetupPage[MAX_PATH];
    CLSID inclsid = CLSID_NULL;
    LPCWSTR pwszDistUnit=NULL; 
    LPCWSTR pwszFileExt=NULL;
    LPCWSTR pwszMimeType=NULL;
    LPWSTR pwszIEFeature=NULL;
    LPWSTR pwszSetupPage=NULL;
    LPWSTR pwszCompID=NULL;
    HINSTANCE hInst = 0;
    IMoniker *pMk = NULL;
    SHOWHTMLDIALOGFN  *pfnShowHTMLDialog = NULL;
    VARIANT vtRetVal;
    VARIANT vtDialogArg;
    VariantInit(&vtRetVal);
    VariantInit(&vtDialogArg);
    QUERYCONTEXT qcInternal;

    // setup the per thread tls list for rejected features, so we
    // don't bother the user multiple times for the same feature
    // needs review on whether this is per thread or per process

    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))
        goto Exit;

    hr = SetCodeDownloadTLSVars();

    if (FAILED(hr))
        goto Exit;

    if (pQuery) {
        memcpy(&qcInternal, pQuery, sizeof(QUERYCONTEXT));
    } else {

        memset(&qcInternal, 0, sizeof(QUERYCONTEXT));
    }

    //   Get the class spec.
    if(pClassSpec != NULL)
    {
        switch(pClassSpec->tyspec)
        {
        case TYSPEC_PROGID:

            // BUGBUG: we don't have JIT tables for progid to feature id
            // so this will only work for PEEK for installed versions
            if (FAILED((hr=CLSIDFromProgID(pClassSpec->tagged_union.pProgId, 
                    &inclsid))))
                goto Exit;

            hr = GetIEFeatureFromClass(&pwszIEFeature, inclsid, &qcInternal);
            if (hr != S_OK)
                goto Exit;

            break;

        case TYSPEC_CLSID:
            inclsid = pClassSpec->tagged_union.clsid;
            hr = GetIEFeatureFromClass(&pwszIEFeature, inclsid, &qcInternal);

            if (hr != S_OK)
                goto Exit;

            break;

        case TYSPEC_MIMETYPE:
            pwszMimeType = (LPCWSTR) pClassSpec->tagged_union.pMimeType;
            hr = GetIEFeatureFromMime(&pwszIEFeature, pwszMimeType, &qcInternal);

            if (hr != S_OK)
                goto Exit;

            break;

        case TYSPEC_FILENAME:

            // overload filename to be Active Setup feature ID
            hr = CDLDupWStr( &pwszIEFeature, pClassSpec->tagged_union.pFileName);
            if (hr != S_OK)
                goto Exit;

            break;

        default:
            hr = E_INVALIDARG;
            goto Exit;
        }

    } else {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // We have a feature now
    ASSERT(pwszIEFeature);

    if (!pwszIEFeature) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    hr = IsIEFeatureInstalled(pwszIEFeature, &qcInternal, &pwszSetupPage, &pwszCompID, dwFlags);

    if ((dwFlags & FIEF_FLAG_PEEK)  && pQuery) {
            memcpy(pQuery, &qcInternal, sizeof(QUERYCONTEXT));
    }

    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {

        if (!(dwFlags & FIEF_FLAG_FORCE_JITUI))
            goto Exit;
        else
            hr = S_FALSE;
    }


    if (FAILED(hr) || (hr == S_OK && !(dwFlags & FIEF_FLAG_SKIP_INSTALLED_VERSION_CHECK)))
        goto Exit;


    if (SUCCEEDED(hr) && pwszSetupPage) {
        pwszMUISetupFile = PathFindFileNameW(pwszSetupPage);
        hr = SHGetWebFolderFilePathW(pwszMUISetupFile, pwszMUISetupPage, MAX_PATH);
    }

    hr = IsJITRestricted();

    if ( hr == E_ACCESSDENIED ||
         ((hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) &&
          (!(dwFlags & FIEF_FLAG_FORCE_JITUI))
         )
       ) {
        goto Exit;
    }

    if (!(dwFlags & FIEF_FLAG_FORCE_JITUI)) {

        hr = HasFeatureAlreadyBeenRejected(pwszIEFeature);
        if (hr == S_FALSE) {

            // already been rejected
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            goto Exit;
        }
    }

    // We have a setup page for the feature now
    // need to faultin feature
    if (dwFlags & FIEF_FLAG_PEEK) {

        hr = HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED);

        goto Exit;
    }

    if (!pwszMUISetupPage || !pwszCompID) {

        hr = E_UNEXPECTED;
        goto Exit;
    }

    // faultin feature
    // now we need to acuire some code download cookies
    // trust and setup. This is to make sure that we don't
    // have a JIT and WVT dialog up at the same time
    // we aslo want to avoid having to do a JIT and a code download
    // restart machine dialog come up in the middle

    hrCDLCookies = AcquireCodeDownloadCookies();

    //if (FAILED(hrCDLCookies))
    {

        // Just try and make the new JIT dialog plaster on top of the 
        // WVT dialog, but make the WVT dialog disabled while JIT is up
        // this can be done by hopefully just parenting the JIT with the
        // trust dialog instead of the one passed in.

        HWND hWndParent = hWnd;
        HWND hWndTop = hWnd;

        while ( (hWndParent = GetParent(hWndParent)) != NULL) {
            hWndTop = hWndParent;
        }

        hWnd = GetLastActivePopup(hWndTop);

    }

    hr = GetDialogArgs(pwszCompID, dwFlags, vtDialogArg);

    if (FAILED(hr))
        goto Exit;

    hr =  CreateURLMoniker(NULL, pwszMUISetupPage, &pMk);

    if (FAILED(hr))
        goto Exit;

    hInst = LoadLibraryEx(TEXT("MSHTML.DLL"), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (!hInst)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    pfnShowHTMLDialog = (SHOWHTMLDIALOGFN *)GetProcAddress(hInst, "ShowHTMLDialog");
    if (!pfnShowHTMLDialog) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    (*pfnShowHTMLDialog)(hWnd, pMk, &vtDialogArg, NULL, &vtRetVal);

    // below to release inseng.dll and asctrls.ocx the setup helper and
    // control used in the JIT setup page. This is to help people
    // running iesetup for the next build after they have had a JIT
    // to not require a reboot

    CoFreeUnusedLibraries();


    if ((vtRetVal.vt == VT_I4) && 
        (vtRetVal.lVal == JITPAGE_RETVAL_CANCELLED)) {

        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    } else if ((vtRetVal.vt == VT_I4) &&
            vtRetVal.lVal == JITPAGE_RETVAL_NEED_REBOOT) {
        hr = HRESULT_FROM_WIN32(ERROR_SUCCESS_REBOOT_REQUIRED);

    } else if ((vtRetVal.vt == VT_I4) &&
            vtRetVal.lVal == JITPAGE_RETVAL_DONTASK_THISWINDOW) {

        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        tls->pRejectedFeaturesList->AddHead(pwszIEFeature);
        pwszIEFeature = NULL;
    } else if ((vtRetVal.vt == VT_I4) &&
            vtRetVal.lVal == JITPAGE_RETVAL_DONTASK_EVER) {

        SetJITDeclined(pwszIEFeature);
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

    } else if ((vtRetVal.vt == VT_I4) && FAILED(vtRetVal.lVal) ) {

        hr = vtRetVal.lVal;
    }


Exit:

    if (SUCCEEDED(hrCDLCookies)) {

        HRESULT hr1 = RelinquishCodeDownloadCookies();

        if (FAILED(hr1) && SUCCEEDED(hr))
            hr = hr1;
    }

    SAFERELEASE(pMk);
    SAFEDELETE(pwszIEFeature);
    SAFEDELETE(pwszSetupPage);
    SAFEDELETE(pwszCompID);
    VariantClear(&vtDialogArg);

    if (hInst)
        FreeLibrary(hInst);

    return hr;
}

BOOL IsJITInProgress()
{
    HWND             hwnd;

    hwnd = FindWindow(JIT_DIALOG_CLASS_NAME, JIT_DIALOG_CAPTION);

    return (hwnd != NULL);  // not a typo (FindWindow returns NULL for unfound)
}

